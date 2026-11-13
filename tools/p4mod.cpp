/* p4mod: the modder's command line (docs/modding/p4mod.md). Host tooling only: it
 * reads mod folders, the composed type definitions and the capability index shipped
 * beside it (p4.d.luau, p4mod-capabilities.txt; tools/p4_capabilities.py writes both),
 * and starts the game only for `p4mod test`. No game state, no retail bytes. */
#include "Luau/Ast.h"
#include "Luau/BuiltinDefinitions.h"
#include "Luau/Error.h"
#include "Luau/FileResolver.h"
#include "Luau/Frontend.h"
#include "Luau/Module.h"
#include "Luau/Parser.h"
#include "Luau/Scope.h"
#include "Luau/Type.h"
#include "Luau/TypeArena.h"
extern "C" {
#include "miniz.h"
}
#include "p4mod_data_edits.h"
#include "p4mod_package.h"
#include "p4mod_tables.h"
#include "p4mod_zip32.h"
#include "package_identifier.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sddl.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char **environ;
#endif

#ifndef P4MOD_API_MINOR
#error "P4MOD_API_MINOR comes from include/p4pc/packages.h (tools/luau/CMakeLists.txt)"
#endif
#ifndef P4MOD_ELF_SHA256
#error "P4MOD_ELF_SHA256 comes from config/retail_fnptr_words.txt (tools/luau/CMakeLists.txt)"
#endif

namespace fs = std::filesystem;

/* ---- small helpers ---------------------------------------------------------------- */

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};
static std::string read_file(const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw Failure("cannot open " + path.string());
    std::string text((std::istreambuf_iterator<char>(input)), {});
    if (input.bad()) throw Failure("cannot read " + path.string());
    return text;
}
/* Create a new output without ever truncating an existing file or following a
 * final-component symlink. The file is ours from its creation: a failed write or
 * close removes it, so no partial output is left. */
static void write_new_file(const fs::path &path, const std::string &text)
{
#ifdef _WIN32
    /* CREATE_NEW never opens an existing entry; OPEN_REPARSE_POINT keeps even a dangling
       link at the final component from being followed to create its target. */
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
            throw Failure(path.string() + " already exists; choose another output");
        throw Failure("cannot create " + path.string() + " (Windows error " + std::to_string(error) + ")");
    }
    size_t offset = 0;
    while (offset < text.size()) {
        DWORD count = (DWORD)std::min<size_t>(text.size() - offset, 1u << 30), written = 0;
        if (!WriteFile(file, text.data() + offset, count, &written, nullptr) || !written) {
            DWORD error = GetLastError();
            CloseHandle(file);
            DeleteFileW(path.c_str());
            throw Failure("cannot write " + path.string() + " (Windows error " + std::to_string(error) + ")");
        }
        offset += written;
    }
    if (!CloseHandle(file)) {
        DeleteFileW(path.c_str());
        throw Failure("cannot close " + path.string());
    }
#else
    int file = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0666);
    if (file < 0) {
        if (errno == EEXIST) throw Failure(path.string() + " already exists; choose another output");
        throw Failure("cannot create " + path.string() + ": " + std::strerror(errno));
    }
    size_t offset = 0;
    while (offset < text.size()) {
        ssize_t written = write(file, text.data() + offset, std::min<size_t>(text.size() - offset, 1u << 30));
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            int error = errno;
            close(file);
            unlink(path.c_str());
            throw Failure("cannot write " + path.string() + ": " + std::strerror(error));
        }
        offset += (size_t)written;
    }
    if (close(file)) {
        const int error = errno;
        unlink(path.c_str());
        throw Failure("cannot close " + path.string() + ": " + std::strerror(error));
    }
#endif
}
static std::string trim(const std::string &text)
{
    size_t begin = 0, end = text.size();
    while (begin < end && std::isspace((unsigned char)text[begin])) ++begin;
    while (end > begin && std::isspace((unsigned char)text[end - 1])) --end;
    return text.substr(begin, end - begin);
}
static std::vector<std::string> split(const std::string &text, char separator)
{
    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(text);
    while (std::getline(stream, part, separator)) parts.push_back(part);
    if (!text.empty() && text.back() == separator) parts.push_back("");
    return parts;
}
/* The game's own rule (package_identifier.h): no NUL inside, no leading or trailing '.'. */
static bool identifier(const std::string &text)
{
    return text.find('\0') == std::string::npos && p4_package_identifier(text.c_str());
}
static bool printable(const std::string &text)
{
    for (unsigned char c : text)
        if (c < 32 || c == 127) return false;
    return !text.empty();
}

/* SHA-256 (FIPS 180-4), for the disc and executable fingerprints `setup` records. */
struct Sha256 {
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    unsigned char block[64];
    size_t used = 0;
    uint64_t bits = 0;
    static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
    void compress()
    {
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16 | (uint32_t)block[i * 4 + 2] << 8 | block[i * 4 + 3];
        for (int i = 16; i < 64; ++i)
            w[i] = w[i - 16] + (rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3)) + w[i - 7] +
                   (rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10));
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    void update(const void *data, size_t size)
    {
        const unsigned char *p = (const unsigned char *)data;
        bits += (uint64_t)size * 8;
        while (size--) {
            block[used++] = *p++;
            if (used == 64) { compress(); used = 0; }
        }
    }
    std::string hex()
    {
        uint64_t total = bits;
        unsigned char pad = 0x80;
        update(&pad, 1);
        pad = 0;
        while (used != 56) update(&pad, 1);
        for (int i = 7; i >= 0; --i) { unsigned char byte = (unsigned char)(total >> (i * 8)); update(&byte, 1); }
        std::string out;
        char digits[9];
        for (uint32_t word : h) { std::snprintf(digits, sizeof digits, "%08x", word); out += digits; }
        return out;
    }
};
/* ---- diagnostics: file:line:col: level: what. why. fix. ---------------------------- */

struct Report {
    int errors = 0, warnings = 0;
    void add(const std::string &file, int line, int column, const char *level, const std::string &text)
    {
        std::cout << file;
        if (line > 0) std::cout << ':' << line << ':' << (column > 0 ? column : 1);
        std::cout << ": " << level << ": " << text << '\n';
        if (!std::strcmp(level, "error")) ++errors;
        else if (!std::strcmp(level, "warning")) ++warnings;
    }
};

/* ---- shipped data: definitions and the capability index ------------------------- */

static fs::path data_dir;
static fs::path data_file(const char *name)
{
    if (data_dir.empty())
        throw Failure("cannot find p4mod's own folder from the system; pass --data DIR naming the folder with p4.d.luau");
    fs::path path = data_dir / name;
    if (!fs::is_regular_file(path))
        throw Failure("missing " + path.string() + ": p4mod needs the p4.d.luau and p4mod-capabilities.txt shipped "
                      "beside it (or --data DIR naming their folder)");
    return path;
}
struct Capabilities {
    std::map<std::string, unsigned> revision;
    std::vector<std::pair<std::string, std::string>> surfaces; /* surface -> key */
    void load()
    {
        std::istringstream lines(read_file(data_file("p4mod-capabilities.txt")));
        std::string line;
        while (std::getline(lines, line)) {
            std::istringstream words(line);
            std::string key, surface;
            unsigned rev = 0;
            if (!(words >> key >> rev)) continue;
            revision[key] = rev;
            while (words >> surface) surfaces.push_back({surface, key});
        }
    }
    /* The key that provides a used surface: an exact or longest dotted-prefix match. */
    std::optional<std::string> provider(const std::string &used) const
    {
        std::optional<std::string> best;
        size_t length = 0;
        for (const auto &[surface, key] : surfaces)
            if ((used == surface || (used.size() > surface.size() && !used.compare(0, surface.size(), surface) &&
                                     used[surface.size()] == '.')) && surface.size() > length) {
                best = key;
                length = surface.size();
            }
        return best;
    }
};
static bool parse_requirement(const std::string &text, std::string &key, unsigned &rev)
{
    std::string entry = trim(text);
    size_t at = entry.find('@');
    key = entry.substr(0, at);
    if (key.empty() || !std::islower((unsigned char)key[0])) return false;
    bool first = true;
    for (size_t i = 0; i < key.size(); ++i) {
        char c = key[i];
        if (c == '.' && !first && i + 1 < key.size()) { first = true; continue; }
        if (!(std::islower((unsigned char)c) || (!first && (std::isdigit((unsigned char)c) || c == '_' || c == '-')))) return false;
        first = false;
    }
    rev = 1;
    if (at != std::string::npos) {
        std::string digits = entry.substr(at + 1);
        if (digits.empty() || digits.size() > 9 || digits[0] == '0' ||
            !std::all_of(digits.begin(), digits.end(), [](char c) { return std::isdigit((unsigned char)c); }))
            return false;
        rev = (unsigned)std::stoul(digits);
    }
    return true;
}

/* ---- package files, with the game's no-follow rules (package_registry.c) ----------- */

/* Match the package indexer's no-symlink/reparse/special-file policy. The
 * package tree is a trusted, unchanged installation during this scan. */
enum class Entry { absent, file, directory, refused };
static Entry entry_kind(const fs::path &path)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? Entry::absent : Entry::refused;
    }
    if (attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE)) return Entry::refused;
    return attributes & FILE_ATTRIBUTE_DIRECTORY ? Entry::directory : Entry::file;
#else
    std::error_code error;
    fs::file_status status = fs::symlink_status(path, error);
    if (error || status.type() == fs::file_type::not_found)
        return error == std::errc::no_such_file_or_directory || status.type() == fs::file_type::not_found ? Entry::absent
                                                                                                         : Entry::refused;
    if (fs::is_directory(status)) return Entry::directory;
    if (fs::is_regular_file(status)) return Entry::file;
    return Entry::refused;
#endif
}
enum class PackageKind { directory, file };
static PackageKind package_kind(const fs::path &path)
{
    switch (entry_kind(path)) {
    case Entry::directory: return PackageKind::directory;
    case Entry::file: return PackageKind::file;
    default: throw Failure(path.string() + ": symlink/reparse-point, special or unreadable package entry refused");
    }
}
/* A package-relative path ("data/edits.xml") as the game opens it (contained_path): every
 * directory on the way and the entry itself checked without following links. A path
 * that is not plainly contained (empty, ".", "..", "\", ":" or absolute) is refused. */
static Entry package_entry(const fs::path &root, const std::string &relative)
{
    const std::vector<std::string> parts = split(relative, '/');
    fs::path path = root;
    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string &part = parts[i];
        if (part.empty() || part == "." || part == ".." || part.find_first_of("\\:") != std::string::npos) return Entry::refused;
        path /= part;
        const Entry kind = entry_kind(path);
        if (i + 1 == parts.size() || kind == Entry::absent || kind == Entry::refused) return kind;
        if (kind != Entry::directory) return Entry::absent;
    }
    return Entry::refused;
}

/* Recheck the final component at open, so a replaced file link is not read. */
static std::string read_package_file(const fs::path &path)
{
    std::string data;
    char buffer[65536];
#ifdef _WIN32
    if (package_kind(path) != PackageKind::file) throw Failure(path.string() + ": not a regular package file");
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw Failure("cannot open package file " + path.string());
    BY_HANDLE_FILE_INFORMATION facts;
    if (!GetFileInformationByHandle(file, &facts) || GetFileType(file) != FILE_TYPE_DISK ||
        (facts.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE))) {
        CloseHandle(file);
        throw Failure(path.string() + ": symlink/reparse-point or special package file refused");
    }
    if (!p4mod_zip32_size_ok(((uint64_t)facts.nFileSizeHigh << 32) | facts.nFileSizeLow)) {
        CloseHandle(file);
        throw Failure(path.string() + ": file exceeds ZIP32 size limit");
    }
    for (;;) {
        DWORD count = 0;
        if (!ReadFile(file, buffer, sizeof buffer, &count, nullptr)) {
            CloseHandle(file);
            throw Failure("cannot read package file " + path.string());
        }
        if (!count) break;
        if (!p4mod_zip32_size_ok((uint64_t)data.size() + count)) {
            CloseHandle(file);
            throw Failure(path.string() + ": file exceeds ZIP32 size limit");
        }
        data.append(buffer, count);
    }
    if (!CloseHandle(file)) throw Failure("cannot close package file " + path.string());
#else
    int file = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (file < 0) throw Failure("cannot open package file " + path.string() + ": " + std::strerror(errno));
    struct stat status;
    if (fstat(file, &status) || !S_ISREG(status.st_mode)) {
        close(file);
        throw Failure(path.string() + ": symlink or special package file refused");
    }
    if (status.st_size < 0 || !p4mod_zip32_size_ok((uint64_t)status.st_size)) {
        close(file);
        throw Failure(path.string() + ": file exceeds ZIP32 size limit");
    }
    for (;;) {
        ssize_t count = read(file, buffer, sizeof buffer);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) {
            int error = errno;
            close(file);
            throw Failure("cannot read package file " + path.string() + ": " + std::strerror(error));
        }
        if (!count) break;
        if (!p4mod_zip32_size_ok((uint64_t)data.size() + (uint64_t)count)) {
            close(file);
            throw Failure(path.string() + ": file exceeds ZIP32 size limit");
        }
        data.append(buffer, (size_t)count);
    }
    if (close(file)) throw Failure("cannot close package file " + path.string());
#endif
    return data;
}
/* A package file check reads: nullopt when absent; a link, special entry, folder or
 * over-limit file is one error naming why the game would not use it. */
static std::optional<std::string> read_checked(const fs::path &root, const std::string &relative, uint64_t limit,
                                              Report &report)
{
    switch (package_entry(root, relative)) {
    case Entry::absent: return std::nullopt;
    case Entry::refused:
        report.add(relative, 0, 0, "error", "is a symlink, reparse point or special file, or a path on the way is. The game "
                                            "reads only ordinary files and folders in a package. Replace it with a real file");
        return std::nullopt;
    case Entry::directory:
        report.add(relative, 0, 0, "error", "is a folder. The game reads a file by this name. Remove or rename the folder");
        return std::nullopt;
    case Entry::file: break;
    }
    std::string data = read_package_file(root / fs::path(relative));
    if (data.size() > limit) {
        report.add(relative, 0, 0, "error", "is larger than " + std::to_string(limit / 1024) + " KiB. The game refuses it. "
                                            "Make it smaller");
        return std::nullopt;
    }
    return data;
}
static std::string utf8(const fs::path &path)
{
    const auto text = path.u8string();
    return std::string(text.begin(), text.end());
}
/* The mod folder as the game indexes it (p4mod_package.h over file_index.c): files are
 * found ASCII case-insensitively, and links, special entries, executables, noncanonical
 * names and case collisions make the whole folder one the game refuses. */
struct Package {
    fs::path root;
    P4ModPackage *index = nullptr;
    Package(const fs::path &dir, const std::string &owner) : root(dir)
    {
        index = p4mod_package_open(utf8(dir).c_str(), owner.c_str());
    }
    ~Package() { p4mod_package_close(index); }
    Package(const Package &) = delete;
    Package &operator=(const Package &) = delete;
    /* The name as stored on disk ("Data/Edits.xml" for "data/edits.xml"), if indexed. */
    std::optional<std::string> find(const std::string &name, bool *directory = nullptr) const
    {
        const char *relative = nullptr;
        int folder = 0;
        if (!index || p4mod_package_find(index, name.c_str(), &relative, &folder)) return std::nullopt;
        if (directory) *directory = folder != 0;
        return std::string(relative);
    }
};
/* A file the game opens through its index (p4_package_file_open): nullopt when absent;
 * a folder or an over-limit file is one error naming why the game would not use it. */
static std::optional<std::string> read_indexed(const Package &package, const std::string &name, uint64_t limit,
                                              Report &report)
{
    bool directory = false;
    const std::optional<std::string> actual = package.find(name, &directory);
    if (!actual) return std::nullopt;
    if (directory) {
        report.add(*actual, 0, 0, "error", "is a folder. The game reads a file by this name. Remove or rename the folder");
        return std::nullopt;
    }
    std::string data = read_package_file(package.root / fs::u8path(*actual));
    if (data.size() > limit) {
        report.add(*actual, 0, 0, "error", "is larger than " + std::to_string(limit / 1024) + " KiB. The game refuses it. "
                                           "Make it smaller");
        return std::nullopt;
    }
    return data;
}

/* ---- mod.ini, with the game's own rules (package_registry.c) ---------------------- */

struct Field { std::string key, value; int line; };
struct Section {
    std::string name, argument;
    int line;
    std::vector<Field> fields; /* in file order, as the game hands them to section handlers */
    const Field *find(const std::string &key) const
    {
        for (const Field &field : fields)
            if (field.key == key) return &field;
        return nullptr;
    }
};
struct Manifest {
    std::map<std::string, std::string> fields;
    std::map<std::string, int> lines;
    std::vector<Section> sections; /* extension sections */
};
static Manifest read_manifest(const fs::path &dir, Report &report, const Capabilities &caps)
{
    Manifest manifest;
    const std::string shown = "mod.ini";
    if (package_entry(dir, shown) == Entry::absent) {
        report.add(shown, 0, 0, "error", "no mod.ini. Every mod folder needs one. Create it with `p4mod new`, or "
                                         "write [mod] with id, name, version, api, priority and order");
        return manifest;
    }
    const std::optional<std::string> contents = read_checked(dir, shown, UINT64_MAX, report);
    if (!contents) return manifest;
    const std::string &text = *contents;
    if (text.size() > 64 * 1024) report.add(shown, 0, 0, "error", "mod.ini is larger than 64 KiB. The game refuses it. Keep only metadata in it");
    std::string section;
    int number = 0;
    bool in_mod = false, seen_mod = false, in_extension = false;
    for (std::string line : split(text, '\n')) {
        ++number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 512) { report.add(shown, number, 1, "error", "line is longer than 511 bytes. The game refuses the manifest. Shorten it"); continue; }
        std::string s = trim(line);
        if (s.empty() || s[0] == '#' || s[0] == ';') continue;
        if (s[0] == '[') {
            if (s.back() != ']') { report.add(shown, number, 1, "error", "section header has no closing ]. Write [name] or [name argument]"); continue; }
            section = trim(s.substr(1, s.size() - 2));
            std::string name = section.substr(0, section.find_first_of(" \t"));
            in_mod = name == "mod";
            in_extension = false;
            if (!identifier(name)) {
                in_mod = false;
                report.add(shown, number, 1, "error", "[" + name + "] is not a section name (lowercase letters, digits, '.', "
                                                      "'_' or '-'). The game refuses the manifest. Fix the name");
            } else if (in_mod) {
                if (seen_mod) report.add(shown, number, 1, "error", "second [mod] section. The game reads exactly one. Merge them");
                seen_mod = true;
            } else {
                manifest.sections.push_back({name, trim(section.substr(name.size())), number, {}});
                in_extension = true;
            }
            continue;
        }
        size_t eq = s.find('=');
        if (section.empty() || eq == std::string::npos) {
            report.add(shown, number, 1, "error", "expected key = value inside a [section]. The game refuses the manifest. Fix or remove the line");
            continue;
        }
        std::string key = trim(s.substr(0, eq)), value = trim(s.substr(eq + 1));
        if (!identifier(key)) {
            report.add(shown, number, 1, "error", "\"" + key + "\" is not a key (lowercase letters, digits, '.', '_' or '-'). "
                                                  "The game refuses the manifest. Fix the key");
            continue;
        }
        if (!in_mod) {
            if (in_extension) manifest.sections.back().fields.push_back({key, value, number});
            continue;
        }
        static const std::set<std::string> known = {"id", "name", "version", "api", "priority", "order", "requires"};
        if (!known.count(key)) {
            report.add(shown, number, 1, "error", "unknown [mod] key \"" + key + "\". The game refuses unknown keys. "
                                                  "Use id, name, version, api, priority, order or requires");
            continue;
        }
        if (manifest.fields.count(key)) {
            report.add(shown, number, 1, "error", "\"" + key + "\" appears twice. The game refuses duplicate keys. Keep one");
            continue;
        }
        manifest.fields[key] = value;
        manifest.lines[key] = number;
    }
    if (!seen_mod) report.add(shown, 0, 0, "error", "no [mod] section. The game needs one. Add [mod] with id, name, version, api, priority and order");
    for (const char *required : {"id", "name", "version", "api", "priority", "order"})
        if (seen_mod && !manifest.fields.count(required))
            report.add(shown, 0, 0, "error", std::string("[mod] has no ") + required + ". The game requires it. Add " + required + " = ...");
    auto at = [&](const char *key) { return manifest.lines.count(key) ? manifest.lines[key] : 0; };
    if (manifest.fields.count("id") && !identifier(manifest.fields["id"]))
        report.add(shown, at("id"), 1, "error", "id \"" + manifest.fields["id"] + "\" is not a mod id. Ids are 1-63 lowercase "
                                                 "letters, digits, '.', '_' or '-', not starting or ending with '.'. "
                                                 "Example: yourname.better-agi");
    if (manifest.fields.count("name") && (!printable(manifest.fields["name"]) || manifest.fields["name"].size() > 127))
        report.add(shown, at("name"), 1, "error", "name must be 1-127 printable characters");
    if (manifest.fields.count("version") && (!printable(manifest.fields["version"]) || manifest.fields["version"].size() > 63))
        report.add(shown, at("version"), 1, "error", "version must be 1-63 printable characters, e.g. 1.0");
    for (const char *key : {"priority", "order"})
        if (manifest.fields.count(key)) {
            const std::string &value = manifest.fields[key];
            char *end = nullptr;
            errno = 0;
            long number_value = std::strtol(value.c_str(), &end, 10);
            if (value.empty() || *end || errno || number_value < INT32_MIN || number_value > INT32_MAX)
                report.add(shown, at(key), 1, "error", std::string(key) + " must be a whole number, e.g. 0");
        }
    if (manifest.fields.count("api")) {
        const std::string &api = manifest.fields["api"];
        unsigned major = 0, minor = 0;
        char tail = 0;
        if (std::sscanf(api.c_str(), "%u.%u%c", &major, &minor, &tail) != 2 || major != 1)
            report.add(shown, at("api"), 1, "error", "api \"" + api + "\" is not 1.N. The game compares it with its API 1." +
                                                     std::to_string(P4MOD_API_MINOR) + ". Write api = 1." + std::to_string(P4MOD_API_MINOR));
        else if (minor > P4MOD_API_MINOR)
            report.add(shown, at("api"), 1, "error", "api 1." + std::to_string(minor) + " is newer than this game's API 1." +
                                                     std::to_string(P4MOD_API_MINOR) + ". The game would refuse the mod. Lower api, "
                                                     "and name newer features in requires= instead");
    }
    if (manifest.fields.count("requires")) {
        for (const std::string &entry : split(manifest.fields["requires"], ',')) {
            std::string key;
            unsigned rev;
            if (!parse_requirement(entry, key, rev)) {
                report.add(shown, at("requires"), 1, "error", "requires entry \"" + trim(entry) + "\" is not a capability key. "
                           "The game refuses the manifest. Use comma-separated keys such as events.ailment@1");
                continue;
            }
            auto found = caps.revision.find(key);
            if (found == caps.revision.end())
                report.add(shown, at("requires"), 1, "error", "requires " + key + "@" + std::to_string(rev) + ", which this build "
                           "does not provide. The game would refuse the mod. Check the spelling (p4mod-capabilities.txt lists every key)");
            else if (found->second < rev)
                report.add(shown, at("requires"), 1, "error", "requires " + key + "@" + std::to_string(rev) + ", but this build "
                           "provides " + key + "@" + std::to_string(found->second) + ". The game would refuse the mod. Lower the "
                           "revision or update Persona4-PC");
        }
    }
    for (const Section &section : manifest.sections)
        if (!caps.provider("section:" + section.name))
            report.add(shown, section.line, 1, "error", "[" + section.name + "] is not a section this build reads. The game "
                                                        "refuses the mod. Check the spelling or update Persona4-PC");
    return manifest;
}

/* ---- main.luau and data.luau: types and the capabilities they use ----------------- */

/* ---- typed uses in scripts: capabilities a field of a typed value needs ------------ */

/* A field of a value whose declared type is one of these needs that key, beyond the
 * function that produced the value (p4.d/data.item_names, data.shop, data.dungeons). */
static const struct { const char *type, *field, *key; } TYPED_USES[] = {
    {"P4ItemEdit", "name", "data.item_names"}, {"P4ItemEdit", "shop", "data.shop"}, {"P4Item", "shop", "data.shop"},
    {"P4DataEdit", "floor", "data.dungeons"}, {"P4DataEdit", "formation", "data.dungeons"},
    {"P4DataEdit", "encounters", "data.dungeons"}, {"P4DataEdit", "treasure", "data.dungeons"},
};
struct TypedUse { std::string what, key; Luau::Location at; };

struct Resolver : Luau::FileResolver {
    std::map<std::string, std::string> sources;
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName &module) override
    {
        auto found = sources.find(module);
        if (found == sources.end()) return std::nullopt;
        return Luau::SourceCode{found->second, Luau::SourceCode::Script};
    }
};
/* data.luau's p4, as lua_data_edit.c initialize() builds it: log, has, capabilities, id,
 * settings.get, and p4.data's readers (lua_data.c read_methods) with edit. Keep in step. */
static const std::map<std::string, std::set<std::string>> DATA_WORLD = {
    {"", {"log", "has", "capabilities", "id", "settings", "data"}},
    {"settings", {"get"}},
    {"data", {"skill", "persona", "enemy", "item", "floor", "formation", "encounters", "treasure", "find", "all", "edit"}},
};
/* The part of a p4.a.b... path data.luau lacks ("p4.events"), or "" when data.luau has it. */
static std::string outside_data_world(const std::string &path)
{
    const std::vector<std::string> parts = split(path, '.');
    if (parts.size() < 2) return "";
    if (!DATA_WORLD.at("").count(parts[1])) return "p4." + parts[1];
    auto inner = DATA_WORLD.find(parts[1]);
    if (parts.size() < 3 || inner == DATA_WORLD.end() || inner->second.count(parts[2])) return "";
    return "p4." + parts[1] + "." + parts[2];
}
static void keep_fields(Luau::TypeId type, const std::set<std::string> &keep)
{
    auto *table = Luau::getMutable<Luau::TableType>(Luau::follow(type));
    if (!table) throw Failure("the shipped p4.d.luau does not declare p4 as tables; reinstall Persona4-PC");
    for (auto field = table->props.begin(); field != table->props.end();)
        field = keep.count(field->first) ? std::next(field) : table->props.erase(field);
}
/* A type alias the shipped declarations define (P4ItemEdit), followed. */
static std::optional<Luau::TypeId> declared_alias(const Luau::GlobalTypes &globals, const std::string &name)
{
    const Luau::Scope &scope = *globals.globalScope;
    for (const auto *bindings : {&scope.exportedTypeBindings, &scope.privateTypeBindings}) {
        auto found = bindings->find(name);
        if (found != bindings->end()) return Luau::follow(found->second.type);
    }
    return std::nullopt;
}
struct IndexNames : Luau::AstVisitor {
    std::vector<Luau::AstExprIndexName *> found;
    bool visit(Luau::AstExprIndexName *node) override
    {
        found.push_back(node);
        return true;
    }
};
/* Strict types for one script, and the typed uses (TYPED_USES) it makes. data_world
 * narrows the declared p4 to what data.luau gets, with settings.get as lua_data_edit.c
 * settings_get returns it: a toggle's boolean or a choice's string, never a number. */
static std::vector<TypedUse> analyze(const std::string &name, const std::string &source, bool data_world, Report &report)
{
    Resolver resolver;
    resolver.sources[name] = source;
    Luau::NullConfigResolver config;
    config.defaultConfig.mode = Luau::Mode::Strict;
    Luau::FrontendOptions options;
    options.moduleTimeLimitSec = 10;
    options.retainFullTypeGraphs = true; /* astTypes, for TYPED_USES */
    Luau::Frontend frontend(Luau::SolverMode::New, &resolver, &config, options);
    Luau::registerBuiltinGlobals(frontend, frontend.globals);
    const fs::path definitions = data_file("p4.d.luau");
    auto loaded = frontend.loadDefinitionFile(frontend.globals, frontend.globals.globalScope, read_file(definitions),
                                               "p4.d.luau", false);
    if (!loaded.success) throw Failure("the shipped p4.d.luau does not load; reinstall Persona4-PC");
    if (data_world) {
        /* This frontend's own copy of the declarations: narrowing it touches nothing else. */
        const Luau::TypeId p4 = Luau::getGlobalBinding(frontend.globals, "p4");
        keep_fields(p4, DATA_WORLD.at(""));
        auto *top = Luau::getMutable<Luau::TableType>(Luau::follow(p4));
        for (const auto &[field, keep] : DATA_WORLD) {
            if (field.empty()) continue;
            auto found = top->props.find(field);
            if (found == top->props.end() || !found->second.readTy)
                throw Failure("the shipped p4.d.luau does not declare p4." + field + "; reinstall Persona4-PC");
            keep_fields(*found->second.readTy, keep);
        }
        static const char *const helper = "p4mod_data_settings_get";
        auto typed = frontend.loadDefinitionFile(frontend.globals, frontend.globals.globalScope,
                                                 std::string("declare ") + helper + ": (key: string) -> (boolean | string)?\n",
                                                 "p4mod-data-world", false);
        if (!typed.success) throw Failure("p4mod's data.luau settings.get declaration does not load");
        auto *settings = Luau::getMutable<Luau::TableType>(Luau::follow(*top->props.at("settings").readTy));
        Luau::Property &get = settings->props.at("get");
        get.readTy = Luau::getGlobalBinding(frontend.globals, helper);
        if (get.writeTy) get.writeTy = get.readTy;
        frontend.globals.globalScope->bindings.erase(Luau::Symbol(frontend.globals.globalNames.names->getOrAdd(helper)));
    }
    Luau::freeze(frontend.globals.globalTypes);
    auto result = frontend.check(name);
    for (const auto &error : result.errors)
        report.add(name, (int)error.location.begin.line + 1, (int)error.location.begin.column + 1, "error",
                   Luau::toString(error) + (data_world ? " (strict Luau types, with only the p4 data.luau gets while the "
                                                         "game builds its tables). Fix the code"
                                                       : " (strict Luau types, as p4.d.luau declares the API). Fix the code"));
    for (const auto &module : result.timeoutHits)
        report.add(module, 0, 0, "warning", "type checking took too long; the file was not fully checked");
    std::vector<TypedUse> typed;
    const Luau::ModulePtr module = frontend.moduleResolver.getModule(name);
    const Luau::SourceModule *parsed = frontend.getSourceModule(name);
    if (!module || !parsed || !parsed->root) return typed;
    IndexNames indexes;
    parsed->root->visit(&indexes);
    for (Luau::AstExprIndexName *node : indexes.found) {
        const Luau::TypeId *type = module->astTypes.find(node->expr);
        if (!type) continue;
        const Luau::TypeId of = Luau::follow(*type);
        for (const auto &use : TYPED_USES)
            if (!std::strcmp(node->index.value, use.field) && declared_alias(frontend.globals, use.type) == of)
                typed.push_back({std::string(use.type) + "." + use.field, use.key, node->location});
    }
    return typed;
}
struct Usage : Luau::AstVisitor {
    std::vector<std::pair<std::string, Luau::Location>> used;
    static bool chain(Luau::AstExpr *expr, std::string &out)
    {
        if (auto *global = expr->as<Luau::AstExprGlobal>()) {
            out = global->name.value;
            return out == "p4";
        }
        if (auto *index = expr->as<Luau::AstExprIndexName>()) {
            if (!chain(index->expr, out)) return false;
            out += ".";
            out += index->index.value;
            return true;
        }
        return false;
    }
    bool visit(Luau::AstExprIndexName *node) override
    {
        std::string path;
        if (chain(node, path)) used.push_back({path, node->location});
        return true;
    }
    bool visit(Luau::AstExprCall *node) override
    {
        std::string function;
        if (node->args.size && chain(node->func, function) && (function == "p4.events.on" || function == "p4.authorize.on"))
            if (auto *literal = node->args.data[0]->as<Luau::AstExprConstantString>())
                used.push_back({std::string(function == "p4.events.on" ? "event:" : "operation:") +
                                    std::string(literal->value.data, literal->value.size), node->location});
        return true;
    }
};

/* ---- table-source files: the features their records use -------------------------- */

/* Records the game's decoder accepted (p4mod_table_source_check) that need a
 * table-source.<feature> capability, as game_table_xml.c record() reads them:
 * status-ailment rows (status), skill derive/effect-source/delivery (skill-derive,
 * skill-source, skill-delivery), item derive and anchor icon source. Keep in step. */
struct TableFeature { std::string name, what; int line; };
static void table_feature(void *user, const char *record, const char *const *attributes, size_t count,
                          unsigned long line)
{
    static const std::map<std::pair<std::string, std::string>, std::string> ATTRIBUTES = {
        {{"skill", "derive"}, "skill-derive"}, {{"skill", "effect-source"}, "skill-source"},
        {{"skill", "delivery"}, "skill-delivery"}, {{"item", "derive"}, "item-derive"},
        {{"item", "icon-source"}, "item-icon-anchor"}};
    auto &found = *static_cast<std::vector<TableFeature> *>(user);
    if (!std::strcmp(record, "status-ailment")) found.push_back({"status", "<status-ailment>", (int)line});
    for (size_t i = 0; i < count; ++i) {
        auto feature = ATTRIBUTES.find({record, attributes[i]});
        if (feature != ATTRIBUTES.end())
            found.push_back({feature->second, "<" + std::string(record) + " " + attributes[i] + "=...>", (int)line});
    }
}

/* ---- commands ------------------------------------------------------------------- */

static const char *const USAGE =
    "p4mod: make, check, test and package Persona4-PC mods\n"
    "\n"
    "usage: p4mod <command> [options]   (p4mod <command> --help for details)\n"
    "\n"
    "  setup --disc <iso>                        record your disc once (copies nothing)\n"
    "  new <id> [--luau | --files] [--dir DIR]   start a mod folder from a template\n"
    "  check [DIR]                               validate a mod the way the game will load it\n"
    "  test [DIR] [--scenario boot|newgame|battle]  run the game quietly with only this mod\n"
    "  pack [DIR] [--out FILE.zip]               check, then zip the mod for publishing\n"
    "\n"
    "Exit status: 0 success, 1 the mod has errors or the test failed, 2 a usage or I/O problem.\n";

static const std::map<std::string, std::string> HELP = {
    {"setup",
     "usage: p4mod setup --disc <your Persona 4 (USA) disc image>\n"
     "\n"
     "Checks that the image is the retail Persona 4 (USA) disc the port is built for (its\n"
     "SLUS_217.82 must match), then records it in the game's settings (p4pc.ini, [game] iso)\n"
     "so the game and `p4mod test` find it. Nothing is copied or changed on the disc.\n"
     "The settings file is the game's own: P4PC_CONFIG if set, otherwise the per-user folder.\n"},
    {"new",
     "usage: p4mod new <id> [--luau | --files] [--dir DIR]\n"
     "\n"
     "Creates DIR/<id> (default: the current folder) with mod.ini, a README.md and either\n"
     "main.luau (--luau, the default: a strict-typed script that shows a status line in battle)\n"
     "or an empty files/ tree (--files: replacement files only). The id is 1-63 lowercase\n"
     "letters, digits, '.', '_' or '-', not starting or ending with '.', e.g. yourname.better-agi.\n"},
    {"check",
     "usage: p4mod check [DIR]\n"
     "\n"
     "Validates the mod in DIR (default: the current folder) the way the game will: mod.ini\n"
     "(keys, id, api, requires= against this build's capabilities, sections), main.luau (strict\n"
     "Luau types against p4.d.luau), data.luau (strict types against the smaller p4 it runs\n"
     "with), data/edits.xml (the game's decoder; rows and values need the disc: p4mod test) and\n"
     "the capabilities the scripts, package files and table-source records use. Links and\n"
     "special files are refused, not followed. Every problem is one line, file:line:column:\n"
     "level: what. why. fix. A capability the mod uses but requires= does not list is a\n"
     "warning: list it so older builds refuse the mod with a clear message.\n"
     "Build-specific raw access (p4.hook, p4.call, p4.mem) is a note. Exit 1 on any error.\n"},
    {"test",
     "usage: p4mod test [DIR] [--scenario boot|newgame] [--game EXE] [--frames N]\n"
     "       p4mod test [DIR] --scenario battle --save SAVE --encounter ID|KEY\n"
     "                  [--policy typical|greedy|cautious|attentive | --plan FILE] [--seed N] [--game EXE] [--frames N]\n"
     "\n"
     "Runs the game with only this mod enabled, quietly: hidden window, no sound, scripted input,\n"
     "a throwaway save folder and settings file (your own are never touched). `boot` (default)\n"
     "stops on the title screen after main.luau ran; `newgame` starts a New Game and plays into\n"
     "the first scene so frame and field callbacks run. `battle` fights one encounter (a native\n"
     "ID or a formation key such as p4:formation:#17) with the party of SAVE (a save slot folder\n"
     "BASLUS-21782Save<N> or its data file, only ever read) in the game's battle simulator, a\n"
     "native party policy (default typical) or a battle plan choosing the commands, until the\n"
     "battle ends; a plan may name skills, items and enemies in quotes (skill:\"Agilao\"), which\n"
     "the game's own catalog resolves first. Prints the mod's log lines and PASS or\n"
     "FAIL (the mod or its data edits were refused, a callback failed, or the game did not reach\n"
     "the scenario).\n"
     "Needs `p4mod setup` first. --game names the game executable (default: beside p4mod).\n"},
    {"pack",
     "usage: p4mod pack [DIR] [--out FILE.zip]\n"
     "\n"
     "Runs `p4mod check`, and on success writes a zip whose single top folder is the mod id,\n"
     "holding every file of DIR except hidden files and zips. Default output: <id>-<version>.zip\n"
     "beside DIR.\n"},
};

static Capabilities load_capabilities()
{
    Capabilities caps;
    caps.load();
    return caps;
}
/* The game on Windows opens paths through the ANSI code page (fopen, CreateProcessA, the
 * environment block): a path beyond ASCII could name another file there. Refuse it plainly. */
static void require_ascii(const fs::path &path, const std::string &what)
{
#ifdef _WIN32
    for (wchar_t c : path.native())
        if (c > 0x7F)
            throw Failure(what + " path " + utf8(path) + " has non-ASCII characters, which this build's Windows file "
                          "layer does not support; use a folder whose path is plain ASCII");
#else
    (void)path; (void)what;
#endif
}
static int command_check(const fs::path &dir, bool quiet_summary = false)
{
    if (!fs::is_directory(dir)) throw Failure(dir.string() + " is not a folder");
    package_kind(dir); /* the folder itself, not a link to one */
    /* The game's Windows package index lists ANSI names: both the path as given and the one it
       resolves to (a junction above the folder can lead somewhere non-ASCII) must be ASCII. */
    require_ascii(fs::absolute(dir), "the mod folder");
    require_ascii(fs::canonical(dir), "the mod folder (resolved)");
    Capabilities caps = load_capabilities();
    Report report;
    Manifest manifest = read_manifest(dir, report, caps);
    /* key -> its first use: what, in which file, where */
    struct Use { std::string what, file; std::pair<int, int> at; };
    std::map<std::string, Use> needed;
    Use advanced;
    auto need = [&](const std::string &key, const Use &use) {
        if (caps.revision.count(key) && !needed.count(key)) needed[key] = use;
    };
    const std::string owner = manifest.fields.count("id") && identifier(manifest.fields["id"]) ? manifest.fields["id"]
                                                                                              : dir.filename().string();
    /* Every other package file is found the way the game finds it: through its index. */
    const Package package(dir, owner);
    if (!package.index) {
        report.add(dir.filename().string(), 0, 0, "error", "the game's package index refuses this folder (the \"p4pc: mods:\" "
                   "line above names the entry and why: a link or special file, an executable, a name with \\ : or ; "
                   "or control characters, or two names that differ only in case). Fix that entry");
    }
    /* main.luau runs once the game is up; data.luau while the game builds its tables, with a
       smaller p4 (docs/modding/systems/data-and-edits.md, DATA_WORLD). */
    for (const char *name : {"main.luau", "data.luau"}) {
        const bool data_world = !std::strcmp(name, "data.luau");
        const std::optional<std::string> actual = package.find(name);
        const std::optional<std::string> contents =
            read_indexed(package, name, data_world ? p4mod_data_luau_max : UINT64_MAX, report);
        if (!contents) continue;
        const std::string shown = *actual;
        const std::string &source = *contents;
        for (const TypedUse &use : analyze(shown, source, data_world, report))
            need(use.key, {use.what, shown, {(int)use.at.begin.line + 1, (int)use.at.begin.column + 1}});
        Luau::Allocator allocator;
        Luau::AstNameTable names(allocator);
        Luau::ParseResult parsed = Luau::Parser::parse(source.data(), source.size(), names, allocator);
        if (!parsed.root) continue;
        Usage usage;
        parsed.root->visit(&usage);
        std::set<std::string> missing;
        for (const auto &[path, location] : usage.used) {
            const std::pair<int, int> at = {(int)location.begin.line + 1, (int)location.begin.column + 1};
            if (data_world) {
                /* event:/operation: uses come from p4.events.on/p4.authorize.on, which data.luau lacks. */
                if (path.rfind("p4.", 0) != 0) continue;
                const std::string lacking = outside_data_world(path);
                if (!lacking.empty()) {
                    if (missing.insert(lacking).second)
                        report.add(shown, at.first, at.second, "note",
                                   lacking + " does not exist in data.luau: it runs while the game builds its tables, before "
                                   "events, hooks, memory and UI exist, and the game refuses the mod when this code runs. "
                                   "Move it to main.luau");
                    continue;
                }
            } else if (path == "p4.data.edit" && missing.insert(path).second) {
                /* lua_data.c: in main.luau, p4.data.edit raises an error naming data.luau. */
                report.add(shown, at.first, at.second, "error",
                           "p4.data.edit runs only in data.luau, while the game builds its tables; in main.luau the "
                           "game raises an error when it is called. Move the edits to data.luau (or use "
                           "p4.data.session.edit for changes during play)");
            }
            auto key = caps.provider(path);
            if (!key || needed.count(*key)) continue;
            needed[*key] = {path, shown, at};
            if (*key == "advanced" && advanced.what.empty()) advanced = needed[*key];
        }
        if (data_world)
            report.add(shown, 0, 0, "note", "the rows and values data.luau edits are checked when the game runs it, "
                                            "against your disc's tables: `p4mod test` does that");
    }
    /* data/edits.xml through the game's own decoder and field rules (p4mod_data_edits.h). */
    if (const std::optional<std::string> xml = read_indexed(package, "data/edits.xml", p4mod_data_edits_xml_max, report)) {
        const std::string shown = *package.find("data/edits.xml");
        char reason[1024] = "";
        P4ModDataEdits edits;
        if (p4mod_data_edits_xml_check((const uint8_t *)xml->data(), xml->size(), &edits, reason, sizeof reason)) {
            int line = 0;
            std::string text = reason;
            if (std::sscanf(reason, "line %d: ", &line) == 1) text = text.substr(text.find(": ") + 2);
            report.add(shown, line, 1, "error", text + ". The game refuses the mod. Fix the record");
        } else if (edits.unchecked) {
            report.add(shown, 0, 0, "note", std::to_string(edits.unchecked) + " rule(s) need your disc's tables and were "
                       "not checked here (first: " + edits.first_unchecked + "); `p4mod test` runs the game, which checks "
                       "them");
        }
        if (edits.item_names) need("data.item_names", {"item renames", shown, {0, 0}});
        if (edits.shop) need("data.shop", {"item shop fields", shown, {0, 0}});
        if (edits.dungeons) need("data.dungeons", {"dungeon rows", shown, {0, 0}});
        /* game_data_edit.c invariants(): run on the final rows after this mod's edits.xml and
           data.luau, over every row, with the disc's values for fields no layer wrote. */
        if (edits.skill_hits)
            report.add(shown, 0, 0, "note", "not checked here: every skill's hits.min <= hits.max, which the game checks "
                                            "on the final rows after data.luau too; `p4mod test` does that");
        if (edits.enemy_drops)
            report.add(shown, 0, 0, "note", "not checked here: each enemy's drop chances add up to at most 100%, which the "
                                            "game checks on the final rows after data.luau too; `p4mod test` does that");
    }
    std::set<std::string> table_keys;
    /* Every extension field, as the game hands a mod's manifest to its [setting] declarations. */
    std::vector<P4ModManifestField> every;
    for (const Section &section : manifest.sections)
        for (const Field &field : section.fields)
            every.push_back({section.name.c_str(), section.argument.c_str(), field.key.c_str(), field.value.c_str(),
                             (unsigned)field.line});
    for (const Section &section : manifest.sections) {
        auto key = caps.provider("section:" + section.name);
        if (key && !needed.count(*key)) needed[*key] = {"[" + section.name + "]", "mod.ini", {section.line, 1}};
        /* Boot-section companion gates (mod_setting_gate.h): `companion` and `companion-setting`. */
        for (const Field &field : section.fields) {
            if (field.key != "companion" && field.key != "companion-setting") continue;
            auto gate = caps.provider("section:" + section.name + "." + field.key);
            if (gate && !needed.count(*gate))
                needed[*gate] = {"[" + section.name + "] " + field.key + "=", "mod.ini", {field.line, 1}};
        }
        if (section.name != "table-source") continue;
        std::vector<P4ModTableField> fields;
        for (const Field &field : section.fields)
            fields.push_back({field.key.c_str(), field.value.c_str(), (unsigned)field.line});
        char reason[512] = "";
        /* game_table_boot.c: the `when` gate first (mod_setting_gate.c), then definition() in its
           order: metadata, key, file, decode. A gate whose toggle is off by default takes no part
           until the player turns it on: what it would load is still checked, as warnings. */
        const char *gate = nullptr;
        int default_on = 1;
        const int gated = p4mod_section_gate(fields.data(), fields.size(), every.data(), every.size(), &gate, &default_on,
                                             reason, sizeof reason);
        if (gated < 0) {
            report.add("mod.ini", section.line, 1, "error", "[table-source " + section.argument + "] " + reason +
                       ". The game refuses the mod. Fix the when= field or its [setting]");
            continue;
        }
        const bool off = gated == 1 && !default_on;
        const char *level = off ? "warning" : "error";
        const std::string when = off ? "with " + std::string(gate) + " on (off by default): " : "";
        P4ModTableSource source;
        if (p4mod_table_source_metadata(section.argument.c_str(), fields.data(), fields.size(), &source, reason,
                                        sizeof reason)) {
            report.add("mod.ini", section.line, 1, level, when + reason + ". The game refuses the mod. Fix the section");
            continue;
        }
        if (!off && !table_keys.insert(section.argument).second) {
            report.add("mod.ini", section.line, 1, "error", "Duplicate table-source key across providers. The game refuses "
                                                            "the mod. Give each [table-source] its own key");
            continue;
        }
        bool directory = false;
        const std::optional<std::string> actual = package.find(source.file, &directory);
        if (!actual || directory) {
            if (package.index)
                report.add("mod.ini", section.line, 1, level, when + "[table-source " + section.argument + "] file=" +
                           source.file + ": Table source must be an authorized contained indexed regular file within8MiB. "
                           "The game refuses the mod. Fix the path");
            continue;
        }
        const std::optional<std::string> xml = read_indexed(package, source.file, p4mod_table_source_max, report);
        if (!xml) continue;
        std::vector<TableFeature> features;
        if (p4mod_table_source_check(&source, (const uint8_t *)xml->data(), xml->size(), table_feature, &features, reason,
                                     sizeof reason)) {
            int line = 0;
            const char *at = std::strstr(reason, "(line");
            if (at) std::sscanf(at, "(line%d)", &line);
            report.add(*actual, line, 1, level, when + reason + ". The game refuses the mod. Fix the record");
        } else {
            report.add(*actual, 0, 0, "note", "rows, attributes and values decode as the game reads them; the IDs and keys "
                                              "they claim and the families they join are checked when the game builds its "
                                              "tables: `p4mod test` does that");
        }
        for (const TableFeature &feature : features) {
            auto provider = caps.provider("section:table-source." + feature.name);
            if (provider && !needed.count(*provider)) needed[*provider] = {feature.what, *actual, {feature.line, 1}};
        }
    }
    /* Package files a capability covers (file:<path>, e.g. data/edits.xml): present means used. */
    for (const auto &[surface, key] : caps.surfaces) {
        if (surface.rfind("file:", 0) != 0 || needed.count(key)) continue;
        bool directory = false;
        const std::optional<std::string> actual = package.find(surface.substr(5), &directory);
        if (actual && !directory) needed[key] = {*actual, *actual, {0, 0}};
    }
    std::set<std::string> declared;
    if (manifest.fields.count("requires"))
        for (const std::string &entry : split(manifest.fields["requires"], ',')) {
            std::string key;
            unsigned rev;
            if (parse_requirement(entry, key, rev)) declared.insert(key);
        }
    static const std::set<std::string> base = {"core", "advanced", "capabilities"};
    struct Finding { std::string file; std::pair<int, int> at; const char *level; std::string text; };
    std::vector<Finding> findings;
    for (const auto &[key, use] : needed) {
        if (declared.count(key) || base.count(key)) continue;
        const std::string revision = key + "@" + std::to_string(caps.revision[key]);
        findings.push_back({use.file, use.at, "warning",
                            "uses " + use.what + " (capability " + revision + "), which requires= does not list. An older build "
                            "would load the mod and fail later. Add " + revision + " to requires="});
    }
    if (!advanced.what.empty())
        findings.push_back({advanced.file, advanced.at, "note",
                            "uses " + advanced.what + " (capability advanced@1): build-specific addresses that may change with port "
                            "updates. Prefer a semantic capability (docs/modding/capabilities.md)"});
    std::sort(findings.begin(), findings.end(), [](const Finding &a, const Finding &b) {
        return std::tie(a.file, a.at) < std::tie(b.file, b.at);
    });
    for (const Finding &finding : findings)
        report.add(finding.file, finding.at.first, finding.at.second, finding.level, finding.text);
    if (!quiet_summary || report.errors)
        std::cout << "p4mod check: " << (manifest.fields.count("id") ? manifest.fields["id"] : dir.filename().string()) << ": "
                  << report.errors << " error(s), " << report.warnings << " warning(s)\n";
    return report.errors ? 1 : 0;
}

static std::string template_ini(const std::string &id, bool script)
{
    return "[mod]\n"
           "id = " + id + "\n"
           "name = " + id + "\n"
           "version = 0.1\n"
           "api = 1." + std::to_string(P4MOD_API_MINOR) + "\n"
           "priority = 0\n"
           "order = 0\n" +
           (script ? "requires = events.battle_start@1, ui.status@1\n" : "");
}
static int command_new(const std::string &id, bool script, const fs::path &parent)
{
    if (!identifier(id))
        throw Failure("\"" + id + "\" is not a mod id: use 1-63 lowercase letters, digits, '.', '_' or '-', not starting or "
                      "ending with '.', e.g. yourname.better-agi");
    const fs::path dir = parent / id;
    fs::create_directories(parent);
    if (!fs::create_directory(dir)) throw Failure(dir.string() + " already exists; choose another id or folder");
    /* The folder is new and ours: a failure part-way removes it, and nothing that existed before. */
    try {
    write_new_file(dir / "mod.ini", template_ini(id, script));
    if (script) {
        write_new_file(dir / "main.luau",
                   "--!strict\n"
                   "-- " + id + ": runs when the game starts. Check it with `p4mod check`, run it with `p4mod test`.\n"
                   "p4.events.on(\"battle_start\", function(event: P4BattleStartEvent)\n"
                   "    p4.ui.status(\"" + id + " is active\", \"hud\")\n"
                   "    p4.log(\"" + id + " saw a battle start\")\n"
                   "end)\n"
                   "p4.log(\"" + id + " ready\")\n");
    } else {
        if (!fs::create_directory(dir / "files") || !fs::create_directory(dir / "files" / "DATA"))
            throw Failure("cannot create new files/DATA in " + dir.string());
    }
    write_new_file(dir / "README.md",
               "# " + id + "\n\n"
               "A Persona4-PC mod. Install it by copying this folder into the game's `mods` folder and enabling it\n"
               "in MODS.\n\n"
               "- `p4mod check` validates it the way the game loads it.\n"
               "- `p4mod test` runs the game quietly with only this mod.\n"
               "- `p4mod pack` makes the zip to publish.\n" +
               std::string(script ? "\n`main.luau` is the script; `p4.d.luau` beside p4mod gives your editor the types.\n"
                                  : "\nPut replacement files under `files/`, with the disc's uppercase paths (files/DATA/...).\n"));
    } catch (...) {
        std::error_code ignored;
        fs::remove_all(dir, ignored);
        throw;
    }
    std::cout << "p4mod new: created " << dir.string() << " (" << (script ? "Luau" : "files") << " template)\n";
    return 0;
}

static fs::path settings_file()
{
#ifdef _WIN32
    /* Read wide: the ANSI environment could best-fit a non-ASCII name onto another path. */
    fs::path path;
    if (const wchar_t *config = _wgetenv(L"P4PC_CONFIG")) path = config;
    else if (const wchar_t *base = _wgetenv(L"APPDATA")) path = fs::path(base) / "p4pc" / "Persona4" / "p4pc.ini";
    else throw Failure("APPDATA is not set; set P4PC_CONFIG to the game's p4pc.ini");
    require_ascii(path, "the settings file");
    return path;
#else
    if (const char *config = std::getenv("P4PC_CONFIG")) return config;
#endif
#ifdef _WIN32
#elif defined(__APPLE__)
    const char *home = std::getenv("HOME");
    if (!home) throw Failure("HOME is not set; set P4PC_CONFIG to the game's p4pc.ini");
    return fs::path(home) / "Library" / "Application Support" / "p4pc" / "Persona4" / "p4pc.ini";
#else
    if (const char *data = std::getenv("XDG_DATA_HOME"); data && *data) return fs::path(data) / "p4pc" / "Persona4" / "p4pc.ini";
    const char *home = std::getenv("HOME");
    if (!home) throw Failure("HOME is not set; set P4PC_CONFIG to the game's p4pc.ini");
    return fs::path(home) / ".local" / "share" / "p4pc" / "Persona4" / "p4pc.ini";
#endif
}
/* The game's settings reader matches sections and keys ASCII case-insensitively (p4cfg.c). */
static bool same_ascii(const std::string &a, const std::string &b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower((unsigned char)x) == std::tolower((unsigned char)y);
           });
}
/* Sets key = value in [section] of an INI text, keeping every other line and the line ends. */
static std::string ini_set(const std::string &text, const std::string &section, const std::string &key, const std::string &value)
{
    const char *end = text.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    std::vector<std::string> lines = text.empty() ? std::vector<std::string>{} : split(text, '\n');
    if (!lines.empty() && lines.back().empty()) lines.pop_back();
    for (std::string &line : lines)
        if (!line.empty() && line.back() == '\r') line.pop_back();
    std::string current;
    long last = -1, found = -1; /* the section's last non-blank line; the key's line */
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string s = trim(lines[i]);
        if (!s.empty() && s[0] == '[' && s.back() == ']') {
            current = trim(s.substr(1, s.size() - 2));
            if (same_ascii(current, section) && last < 0) last = (long)i;
            continue;
        }
        if (same_ascii(current, section) && !s.empty()) last = (long)i;
        size_t eq = s.find('=');
        if (same_ascii(current, section) && eq != std::string::npos && same_ascii(trim(s.substr(0, eq)), key)) found = (long)i;
    }
    const std::string row = key + "=" + value;
    if (found >= 0) lines[(size_t)found] = row;
    else if (last >= 0) lines.insert(lines.begin() + last + 1, row);
    else { lines.push_back("[" + section + "]"); lines.push_back(row); }
    std::string out;
    for (const std::string &line : lines) out += line + end;
    return out;
}
static std::string ini_get(const std::string &text, const std::string &section, const std::string &key)
{
    std::string current;
    for (std::string line : split(text, '\n')) {
        std::string s = trim(line);
        if (!s.empty() && s[0] == '[' && s.back() == ']') { current = trim(s.substr(1, s.size() - 2)); continue; }
        size_t eq = s.find('=');
        if (same_ascii(current, section) && eq != std::string::npos && same_ascii(trim(s.substr(0, eq)), key))
            return trim(s.substr(eq + 1));
    }
    return "";
}
/* The game's settings save (p4cfg.c p4cfg_set_batch): the whole new text into a temporary
 * file beside it, then a rename over it. The old file stays whole until the rename, and a
 * failure removes the temporary file and leaves the old one as it was. */
static void replace_file(const fs::path &path, const std::string &text)
{
#ifdef _WIN32
    const unsigned long pid = GetCurrentProcessId();
#else
    const unsigned long pid = (unsigned long)getpid();
#endif
    const fs::path temporary = path.parent_path() / (path.filename().string() + ".p4mod-" + std::to_string(pid) + ".tmp");
    write_new_file(temporary, text);
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD error = GetLastError();
        DeleteFileW(temporary.c_str());
        throw Failure("cannot replace " + path.string() + " (Windows error " + std::to_string(error) + "); it is unchanged");
    }
#else
    if (std::rename(temporary.c_str(), path.c_str())) {
        const int error = errno;
        unlink(temporary.c_str());
        throw Failure("cannot replace " + path.string() + ": " + std::strerror(error) + "; it is unchanged");
    }
#endif
}
/* The disc's SLUS_217.82, found in the ISO 9660 root directory, must be the retail executable
 * the port is built from (config/retail_fnptr_words.txt): that names the disc exactly. Returns its bytes. */
static std::string disc_elf(const fs::path &disc)
{
    if (!fs::is_regular_file(disc)) throw Failure(disc.string() + " is not a file; pass your Persona 4 (USA) disc image");
    std::ifstream input(disc, std::ios::binary);
    auto le32 = [](const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; };
    unsigned char volume[2048] = {0};
    input.seekg(16 * 2048);
    if (!input.read((char *)volume, sizeof volume) || std::memcmp(volume + 1, "CD001", 5))
        throw Failure(disc.string() + " is not an ISO 9660 disc image (no volume descriptor at sector 16)");
    const uint32_t root = le32(volume + 156 + 2), root_size = le32(volume + 156 + 10);
    std::vector<unsigned char> directory(std::min<uint32_t>(root_size, 1u << 20));
    input.seekg((std::streamoff)root * 2048);
    if (!input.read((char *)directory.data(), (std::streamsize)directory.size()))
        throw Failure(disc.string() + " is truncated: its root directory lies past the end of the file");
    uint32_t extent = 0, size = 0;
    for (size_t at = 0; at < directory.size();) {
        const unsigned length = directory[at];
        if (!length || at + length > directory.size()) { at = (at / 2048 + 1) * 2048; continue; }
        const unsigned name_length = directory[at + 32];
        if (33 + name_length <= length && std::string((const char *)&directory[at + 33], name_length).rfind("SLUS_217.82", 0) == 0) {
            extent = le32(&directory[at + 2]);
            size = le32(&directory[at + 10]);
            break;
        }
        at += length;
    }
    if (!size) throw Failure(disc.string() + " is not the Persona 4 (USA) disc: it has no SLUS_217.82 at its root");
    if (size > (64u << 20)) throw Failure(disc.string() + " has a SLUS_217.82 that is not the retail one this port is built for (too large)");
    std::string elf(size, '\0');
    input.seekg((std::streamoff)extent * 2048);
    if (!input.read(elf.data(), (std::streamsize)size)) throw Failure(disc.string() + " is truncated inside SLUS_217.82");
    Sha256 sha;
    sha.update(elf.data(), elf.size());
    if (const std::string digest = sha.hex(); digest != P4MOD_ELF_SHA256)
        throw Failure(disc.string() + " has a SLUS_217.82 that is not the retail one this port is built for (SHA-256 " + digest +
                      "); use an unmodified dump of the Persona 4 (USA) disc");
    return elf;
}
static int command_setup(const fs::path &disc)
{
    if (!fs::is_regular_file(disc)) throw Failure(disc.string() + " is not a file; pass your Persona 4 (USA) disc image");
    const fs::path absolute = fs::absolute(disc);
    require_ascii(absolute, "the disc image");
    /* The value must read back exactly as the game's settings reader (p4cfg.c) parses a line:
       no line breaks or control bytes (a second line would be a new key or section), no
       whitespace at either end (trimmed), and no ';' or '#' after whitespace (a comment). */
    const std::string recorded = absolute.string();
    for (size_t i = 0; i < recorded.size(); ++i) {
        const unsigned char c = (unsigned char)recorded[i];
        if (c < 0x20 || c == 0x7F || ((c == ';' || c == '#') && i && std::isspace((unsigned char)recorded[i - 1])))
            throw Failure(disc.string() + ": this path cannot be recorded in p4pc.ini exactly (a control character, or "
                          "';' or '#' after a space); move or rename the disc image");
    }
    if (recorded != trim(recorded))
        throw Failure(disc.string() + ": this path starts or ends with a space, which p4pc.ini would drop; rename it");
    disc_elf(absolute);
    const fs::path settings = settings_file();
    std::string text = fs::is_regular_file(settings) ? read_file(settings) : "";
    text = ini_set(text, "game", "iso", recorded);
    fs::create_directories(settings.parent_path());
    replace_file(settings, text);
    std::cout << "p4mod setup: recorded " << absolute.string() << " in " << settings.string() << "\n";
    return 0;
}

/* The environment p4mod inherited, as NAME/VALUE pairs. */
static std::vector<std::pair<std::string, std::string>> inherited_environment()
{
    std::vector<std::pair<std::string, std::string>> entries;
    auto add = [&](const std::string &entry) {
        const size_t eq = entry.find('=', 1);
        if (eq != std::string::npos) entries.emplace_back(entry.substr(0, eq), entry.substr(eq + 1));
    };
#ifdef _WIN32
    LPCH strings = GetEnvironmentStringsA();
    for (LPCH p = strings; p && *p; p += std::strlen(p) + 1) add(p);
    if (strings) FreeEnvironmentStringsA(strings);
#else
    for (char **p = environ; p && *p; ++p) add(*p);
#endif
    return entries;
}
/* A game, SDL or mod-test setting (any letter case, as Windows treats names). */
static bool game_variable(const std::string &name)
{
    std::string upper = name;
    for (char &c : upper) c = (char)std::toupper((unsigned char)c);
    return upper.rfind("P4PC_", 0) == 0 || upper.rfind("SDL_", 0) == 0;
}
/* Runs a program with arguments and extra environment variables, its output into a log file;
 * the exit code. `isolated` first drops every inherited P4PC_* and SDL_* variable, so only
 * `env` configures the game. */
static int run_game(const fs::path &exe, const std::vector<std::string> &args, const std::map<std::string, std::string> &env,
                    const fs::path &log, const fs::path &cwd, bool isolated = false)
{
#ifdef _WIN32
    std::string block;
    std::map<std::string, std::string> merged;
    for (const auto &[key, value] : inherited_environment())
        if (!isolated || !game_variable(key)) merged[key] = value;
    for (const auto &[key, value] : env) merged[key] = value;
    for (const auto &[key, value] : merged) block += key + "=" + value + '\0';
    block += '\0';
    /* The quoting CommandLineToArgvW and the C runtime undo: backslashes double only before a quote. */
    auto quote = [](const std::string &arg) {
        std::string out = "\"";
        size_t slashes = 0;
        for (char c : arg) {
            if (c == '\\') { ++slashes; continue; }
            out.append(c == '"' ? slashes * 2 + 1 : slashes, '\\');
            slashes = 0;
            out += c;
        }
        out.append(slashes * 2, '\\');
        return out + "\"";
    };
    std::string command = quote(exe.string());
    for (const std::string &arg : args) command += " " + quote(arg);
    SECURITY_ATTRIBUTES inherit = {sizeof inherit, nullptr, TRUE};
    HANDLE output = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &inherit, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (output == INVALID_HANDLE_VALUE) throw Failure("cannot write " + log.string());
    STARTUPINFOA startup = {};
    startup.cb = sizeof startup;
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = output;
    startup.hStdError = output;
    PROCESS_INFORMATION process = {};
    std::vector<char> line(command.begin(), command.end());
    line.push_back('\0');
    if (!CreateProcessA(nullptr, line.data(), nullptr, nullptr, TRUE, IDLE_PRIORITY_CLASS | CREATE_NO_WINDOW, block.data(),
                        cwd.string().c_str(), &startup, &process)) {
        CloseHandle(output);
        throw Failure("cannot start " + exe.string());
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
    CloseHandle(output);
    return (int)code;
#else
    std::vector<std::string> dropped;
    if (isolated)
        for (const auto &[key, value] : inherited_environment())
            if (game_variable(key)) dropped.push_back(key);
    std::string program = exe.string();
    std::vector<std::string> words = args;
    std::vector<char *> argv = {program.data()};
    for (std::string &word : words) argv.push_back(word.data());
    argv.push_back(nullptr);
    int output = open(log.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (output < 0) throw Failure("cannot create " + log.string() + ": " + std::strerror(errno));
    pid_t child = fork();
    if (child < 0) { close(output); throw Failure("cannot start " + exe.string()); }
    if (!child) {
        if (chdir(cwd.c_str())) _exit(127);
        dup2(output, 1);
        dup2(output, 2);
        close(output);
        for (const std::string &key : dropped) unsetenv(key.c_str());
        for (const auto &[key, value] : env) setenv(key.c_str(), value.c_str(), 1);
        execv(program.c_str(), argv.data());
        _exit(127);
    }
    close(output);
    int status = 0;
    while (waitpid(child, &status, 0) < 0)
        if (errno != EINTR) throw Failure("cannot wait for " + exe.string() + ": " + std::strerror(errno));
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
#endif
}
/* Claim a fresh directory with one atomic create, private to this user (POSIX 0700; on
 * Windows a protected ACL granting only its owner and SYSTEM); collisions are never
 * opened or removed. The folder and its evidence are kept after the run. */
static fs::path make_test_dir(const std::string &id)
{
    const fs::path temp = fs::temp_directory_path();
#ifdef _WIN32
    const auto pid = GetCurrentProcessId();
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;OICI;FA;;;OW)(A;OICI;FA;;;SY)", SDDL_REVISION_1,
                                                             &descriptor, nullptr))
        throw Failure("cannot build a private folder ACL (Windows error " + std::to_string(GetLastError()) + ")");
    SECURITY_ATTRIBUTES attributes = {sizeof attributes, descriptor, FALSE};
#else
    const auto pid = getpid();
#endif
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 10000; ++attempt) {
        fs::path candidate = temp / ("p4mod-test-" + id + "-" + std::to_string(pid) + "-" +
                                     std::to_string(now) + "-" + std::to_string(attempt));
#ifdef _WIN32
        if (CreateDirectoryW(candidate.c_str(), &attributes)) { LocalFree(descriptor); return candidate; }
        const DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            LocalFree(descriptor);
            throw Failure("cannot create " + candidate.string() + " (Windows error " + std::to_string(error) + ")");
        }
#else
        if (!mkdir(candidate.c_str(), 0700)) return candidate;
        if (errno != EEXIST) throw Failure("cannot create " + candidate.string() + ": " + std::strerror(errno));
#endif
    }
#ifdef _WIN32
    LocalFree(descriptor);
#endif
    throw Failure("cannot allocate a unique test directory in " + temp.string());
}
/* The engine's gate result (newgame_gate.c write_gate_result): "p4pc-gate-result 1", then
 * "key integer" lines. Anything else, or no file, is no result. */
static std::map<std::string, long long> read_gate_result(const fs::path &path)
{
    std::map<std::string, long long> result;
    std::error_code error;
    if (!fs::is_regular_file(fs::symlink_status(path, error)) || error || fs::file_size(path, error) > 4096 || error)
        return result;
    const std::vector<std::string> lines = split(read_file(path), '\n');
    if (lines.empty() || lines[0] != "p4pc-gate-result 1") return result;
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].empty() && i + 1 == lines.size()) break;
        const size_t space = lines[i].find(' ');
        const std::string key = lines[i].substr(0, space), text = space == std::string::npos ? "" : lines[i].substr(space + 1);
        char *end = nullptr;
        const long long number = text.empty() ? 0 : std::strtoll(text.c_str(), &end, 10);
        if (key.empty() || text.empty() || !end || *end || result.count(key)) return {};
        result[key] = number;
    }
    return result;
}
/* What every scenario needs: a mod that checks clean, the recorded disc and the game, on paths the
 * game's Windows file layer can open, and a mod folder with only ordinary files and folders. */
struct TestTarget {
    std::string id, version, iso;
    fs::path game;
};
static std::optional<TestTarget> test_target(const fs::path &dir, std::optional<fs::path> game)
{
    if (command_check(dir, true)) {
        std::cout << "p4mod test: FAIL: fix the errors `p4mod check` reports first\n";
        return std::nullopt;
    }
    Capabilities caps = load_capabilities();
    Report report;
    Manifest manifest = read_manifest(dir, report, caps);
    TestTarget target;
    target.id = manifest.fields["id"];
    target.version = manifest.fields["version"];
    const fs::path settings = settings_file();
    const std::string saved = fs::is_regular_file(settings) ? read_file(settings) : "";
    target.iso = ini_get(saved, "game", "iso");
    if (target.iso.empty() || !fs::is_regular_file(target.iso)) throw Failure("no disc is set up; run `p4mod setup --disc <your disc image>` first");
    if (!game) {
        if (data_dir.empty()) throw Failure("cannot find p4mod's own folder from the system; pass --game <the executable>");
#ifdef _WIN32
        game = data_dir / "p4pc64w.exe";
#else
        game = data_dir / (fs::is_regular_file(data_dir / "p4pc") ? "p4pc" : "p4pc64"); /* AppImage, build tree */
#endif
    }
    if (!fs::is_regular_file(*game)) throw Failure("no game at " + game->string() + "; pass --game <the Persona4-PC executable>");
    target.game = fs::absolute(*game);
    require_ascii(target.game, "the game");
    require_ascii(fs::path(target.iso), "the disc image");
    require_ascii(fs::temp_directory_path(), "the temporary folder");
    /* The copy is the package the game sees: refuse links and special entries first (as pack does),
       so nothing outside the mod folder is copied through a link. */
    package_kind(dir);
    for (auto entry = fs::recursive_directory_iterator(dir); entry != fs::recursive_directory_iterator(); ++entry)
        package_kind(entry->path());
    return target;
}
/* An ordinary scripted, hidden, silent launch with a private card, settings and mod folder in
 * `work`; the engine writes its gate outcome to `gate_result` (newgame_gate.c P4PC_GATE_RESULT). */
static std::map<std::string, std::string> launch_environment(const fs::path &work, const std::string &iso, const fs::path &gate_result)
{
    return {
        {"P4PC_CONFIG", (work / "p4pc.ini").string()}, {"P4PC_MOD_DIR", (work / "mods").string()},
        {"P4PC_ISO", iso}, {"P4PC_SAVE_DIR", (work / "card").string()}, {"P4PC_MODS", "1"}, {"P4PC_PACKS", "0"},
        {"P4PC_INPUT", "script"}, {"P4PC_MOD_WITNESS", "1"}, {"P4PC_SCENE_ARTIFACTS", work.string()},
        {"P4PC_HIDDEN_WINDOW", "1"}, {"SDL_AUDIODRIVER", "dummy"}, {"P4PC_TRAP_CONTINUE", "1"}, {"P4PC_TURBO", "2"},
        {"P4PC_VIRTUAL_CLOCK", "1"}, {"P4PC_GATE_RESULT", gate_result.string()}};
}
/* Echoed log lines may hold anything a mod printed: control bytes are shown as '?' so
   they cannot rewrite this terminal's earlier output. */
static void print_mod_lines(const std::string &text, const std::string &id, bool battle)
{
    for (const std::string &line : split(text, '\n'))
        if (line.find("p4pc: mod " + id + ":") != std::string::npos || line.find("p4pc: mods:") != std::string::npos ||
            line.find("p4pc: data edits refused: ") != std::string::npos ||
            (battle && line.find("p4pc: battle-sim") != std::string::npos)) {
            std::string shown = trim(line);
            for (char &c : shown)
                if ((unsigned char)c < 0x20 || c == 0x7F) c = '?';
            std::cout << "  " << shown << "\n";
        }
}
static int command_test(const fs::path &dir, const std::string &scenario, std::optional<fs::path> game, unsigned frames)
{
    const std::optional<TestTarget> target = test_target(dir, game);
    if (!target) return 1;
    const std::string &id = target->id, &iso = target->iso;
    const fs::path work = make_test_dir(id);
    const fs::path log = work / "game.log";
    std::cout << "p4mod test: " << id << " (" << scenario << "), log " << log.string() << "\n";
    if (!fs::create_directory(work / "card") || !fs::create_directory(work / "mods"))
        throw Failure("cannot create private test folders in " + work.string());
    fs::copy(dir, work / "mods" / id, fs::copy_options::recursive);
    write_new_file(work / "p4pc.ini", "[game]\niso=" + iso + "\n[mods]\nenabled=" + id + "\n");
    /* The gate's outcome comes from a file the engine writes at the gate (newgame_gate.c
       P4PC_GATE_RESULT), in this run's private folder: not from the log, which mixes engine
       lines with whatever the mod prints. */
    const fs::path result_file = work / "gate-result.txt";
    std::map<std::string, std::string> env = launch_environment(work, iso, result_file);
    if (scenario == "boot") {
        env["P4PC_PACE_TRACE"] = "1"; /* title-ready is emitted only when the title accepts input */
        env["P4PC_NEWGAME_GATE"] = std::to_string(frames ? frames : 900) + ",-1:-1";
    } else {
        env["P4PC_AUTOPRESS_FRAMES"] = "1";
        env["P4PC_AUTOPRESS_NEWGAME"] = "1560";
        /* The established New Game driver reaches the Dojima living room (7:2)
         * at flip 30500. A frame count alone does not prove story progress. */
        env["P4PC_AUTOPRESS"] = "cross@2300:2/150,start@6500:2/300";
        env["P4PC_NEWGAME_GATE"] = std::to_string(frames ? frames : 30500) + ",7:2";
    }
    const int code = run_game(target->game, {}, env, log, work);
    const std::string text = read_file(log);
    const std::map<std::string, long long> gate = read_gate_result(result_file);
    auto value = [&](const char *key) { auto found = gate.find(key); return found == gate.end() ? -2LL : found->second; };
    const bool reached =
        code == 0 && value("ok") == 1 && value("flip") >= 0 &&
        (scenario == "boot"
             ? (value("title_ready_state") == 7 || value("title_ready_state") == 9) &&
                   value("title_ready_flip") >= 0 && value("title_ready_flip") <= value("flip")
             : value("title_confirmed") == 1 && value("field") == 7 && value("room") == 2);
    std::string problem;
    /* Log lines can only add a failure: names, IDs and values in data.luau and data/edits.xml
       resolve only against the disc's tables (game_data_edit.c), and a refused mod never runs
       its code, so it cannot print the enabled line itself. */
    if (text.find("p4pc: data edits refused: ") != std::string::npos)
        problem = "the game refused the mod's data edits; the data edits line above says why";
    else if (text.find("p4pc: mods: enabled " + id + " ") == std::string::npos)
        problem = "the game did not enable the mod; the mods: lines above say why";
    else if (text.find("package recovery") != std::string::npos || text.find("native mod message") != std::string::npos)
        problem = "the mod failed while running; its error is the last mod line above";
    else if (gate.empty())
        problem = "the game wrote no gate result, so it did not reach the " + scenario + " scenario (exit " +
                  std::to_string(code) + "); see the log";
    else if (!reached)
        problem = "the game did not reach the " + scenario + " scenario (exit " + std::to_string(code) + "); see the log";
    print_mod_lines(text, id, false);
    if (problem.empty()) {
        std::cout << "p4mod test: PASS\n";
        return 0;
    }
    std::cout << "p4mod test: FAIL: " << problem << "\n";
    return 1;
}

/* ---- the battle scenario: the game's battle simulator, this mod its declared mod set -------- */

static void put16(std::string &out, unsigned value) { out += (char)(value & 255); out += (char)(value >> 8 & 255); }
static void put32(std::string &out, uint32_t value) { put16(out, value & 0xFFFF); put16(out, value >> 16); }

/* The native save records in order, as the game's battle entry check reads them (p4_battle_save_valid). */
static const struct SaveSection {
    uint32_t tag, bytes;
} SAVE_SECTIONS[] = {
#define P4LS_SAVE_SECTION(tag, address, bytes) {tag, bytes},
#include "../include/p4pc/lockstep_save_sections.inc"
#undef P4LS_SAVE_SECTION
};
static const uint32_t SAVE_IMAGE_MAX = 0x2a000; /* datSaveBuildImage's allocation */
static std::string hex32(uint32_t value)
{
    char text[16];
    std::snprintf(text, sizeof text, "0x%x", value);
    return text;
}
/* Why bytes are not a complete native save image, or "" when they are: p4_battle_save_valid's checks
 * (src/platform/battle_sim/entry.c), so a bad save is named here instead of inside the game. */
static std::string save_image_problem(const std::string &bytes)
{
    const unsigned char *image = (const unsigned char *)bytes.data();
    const size_t size = bytes.size();
    auto word = [&](size_t at) {
        return (uint32_t)image[at] | (uint32_t)image[at + 1] << 8 | (uint32_t)image[at + 2] << 16 | (uint32_t)image[at + 3] << 24;
    };
    if (size < 56 || size > SAVE_IMAGE_MAX)
        return std::to_string(size) + " bytes, where a save image has 56 to " + std::to_string(SAVE_IMAGE_MAX);
    if (word(0)) return "its version word is " + hex32(word(0)) + ", not 0";
    unsigned summary = 0;
    for (size_t i = 4; i < 54; i++) summary = (summary + image[i]) & 255u;
    if (image[54] != summary || image[55]) return "its summary checksum does not match";
    size_t offset = 56;
    unsigned checksum = 0;
    for (size_t row = 0; row < sizeof SAVE_SECTIONS / sizeof *SAVE_SECTIONS; row++) {
        if (size - offset < 8) return "it ends inside its records, at byte " + std::to_string(offset);
        const uint32_t tag = word(offset), length = word(offset + 4);
        /* The native compact image omits all three field records, never a subset. */
        if (SAVE_SECTIONS[row].tag == 0x25 && tag == 0x28) row += 3;
        if (tag != SAVE_SECTIONS[row].tag || length != SAVE_SECTIONS[row].bytes || length > size - offset - 8)
            return "at byte " + std::to_string(offset) + " it has record " + hex32(tag) + " of " + std::to_string(length) +
                   " bytes where record " + hex32(SAVE_SECTIONS[row].tag) + " of " + std::to_string(SAVE_SECTIONS[row].bytes) +
                   " bytes belongs";
        for (size_t i = 0; i < (size_t)length + 8; i++) checksum = (checksum + image[offset + i]) & 255u;
        offset += (size_t)length + 8;
    }
    if (size - offset < 13 || word(offset) != 0x2000 || word(offset + 4) != 1 || word(offset + 9) != UINT32_MAX)
        return "its end record is missing";
    if (image[offset + 8] != checksum) return "its record checksum does not match";
    return "";
}

/* The explicit save: a card slot folder (BASLUS-21782Save<N>/) or its data file of the same name,
 * which is the native image itself, version word first. Read once, never opened for writing. */
struct SaveInput {
    fs::path file;
    std::string image;
    bool mod_data = false; /* the slot holds .p4mods save blocks, which a battle entry does not carry */
};
static SaveInput read_save(fs::path given)
{
    if (given.filename().empty() && given != given.root_path()) given = given.parent_path();
    auto kind = [](const fs::path &path) {
        std::error_code error;
        if (!fs::exists(fs::symlink_status(path, error)))
            throw Failure(path.string() + " does not exist; pass a card slot folder (BASLUS-21782Save<N>) or its data file");
        try {
            return package_kind(path);
        } catch (const Failure &) {
            throw Failure(path.string() + ": links, special and unreadable files are not accepted as a save; pass the save itself");
        }
    };
    SaveInput save;
    save.file = kind(given) == PackageKind::directory ? given / given.filename() : given;
    if (kind(save.file) != PackageKind::file) throw Failure(save.file.string() + " is a folder, not a save data file");
    std::error_code error;
    const uintmax_t size = fs::file_size(save.file, error);
    if (error) throw Failure("cannot read " + save.file.string() + ": " + error.message());
    if (size > SAVE_IMAGE_MAX)
        throw Failure(save.file.string() + " is not a Persona 4 save image: " + std::to_string(size) + " bytes, more than " +
                      std::to_string(SAVE_IMAGE_MAX));
    save.image = read_file(save.file);
    if (const std::string problem = save_image_problem(save.image); !problem.empty())
        throw Failure(save.file.string() + " is not a Persona 4 save image the battle can start from: " + problem +
                      ". Pass a card slot folder (BASLUS-21782Save<N>) or its data file, as the game writes it or "
                      "`p4pc --import-save` imports it");
    for (const auto &entry : fs::directory_iterator(save.file.parent_path()))
        if (entry.path().filename().string().rfind(".p4mods", 0) == 0) save.mod_data = true;
    return save;
}

/* A row a battle names by catalog name or key: a quoted name in a plan, or an encounter key. The
 * game's own catalog (p4.data, data.catalog@1) answers; p4mod never guesses an ID. */
struct CatalogRef {
    std::string kind, query; /* the p4.data kind: skill, item, enemy or formation */
    bool answered = false;
    uint32_t id = 0;
    std::string key, error;
};
static size_t catalog_ref(std::vector<CatalogRef> &refs, const std::string &kind, const std::string &query)
{
    for (size_t i = 0; i < refs.size(); i++)
        if (refs[i].kind == kind && refs[i].query == query) return i;
    refs.push_back({kind, query});
    return refs.size() - 1;
}
/* The walk policy's battle plan: battle:, battlerule: and battleturn: lines (include/p4pc/walk_script.h),
 * printable ASCII, at most 16384 bytes once resolved. Where the plan takes a native skill, item or enemy
 * ID it may instead hold a quoted name or key, which the catalog resolves to that ID. */
struct BattlePlan {
    fs::path file;
    std::string text;
    struct Use { int line, column; size_t begin, end, ref; };
    std::vector<Use> uses;
};
static BattlePlan read_battle_plan(const fs::path &path, std::vector<CatalogRef> &refs)
{
    std::error_code error;
    if (!fs::exists(fs::symlink_status(path, error)) || package_kind(path) != PackageKind::file)
        throw Failure(path.string() + " is not a plan file");
    if (fs::file_size(path) > 4 * 16384) throw Failure(path.string() + ": a battle plan has at most 16384 bytes");
    BattlePlan plan;
    plan.file = path;
    for (const std::string &raw : split(read_file(path), '\n'))
        plan.text += (!raw.empty() && raw.back() == '\r' ? raw.substr(0, raw.size() - 1) : raw) + "\n";
    while (plan.text.size() > 1 && plan.text[plan.text.size() - 1] == '\n' && plan.text[plan.text.size() - 2] == '\n')
        plan.text.pop_back();
    int number = 0, commands = 0;
    size_t start = 0;
    for (const std::string &line : split(plan.text, '\n')) {
        ++number;
        auto where = [&](size_t column) { return path.string() + ":" + std::to_string(number) + ":" + std::to_string(column + 1); };
        for (size_t at = 0; at < line.size(); at++)
            if (line[at] < 0x20 || line[at] > 0x7e) throw Failure(where(at) + ": a battle plan is printable ASCII only");
        if (!line.empty()) {
            if (!(line.size() > 7 && line.rfind("battle:", 0) == 0) && !(line.size() > 11 && line.rfind("battlerule:", 0) == 0) &&
                !(line.size() > 11 && line.rfind("battleturn:", 0) == 0))
                throw Failure(where(0) + ": expected a battle:, battlerule: or battleturn: line");
            ++commands;
        }
        for (size_t at = 0; at < line.size(); at++) {
            if (line[at] != '"') continue;
            const size_t close = line.find('"', at + 1);
            if (close == std::string::npos) throw Failure(where(at) + ": the quoted name has no closing quote");
            const std::string query = line.substr(at + 1, close - at - 1);
            if (trim(query).empty()) throw Failure(where(at) + ": the quotes hold no name");
            const std::string before = line.substr(0, at);
            auto ends = [&](const std::string &suffix) {
                return before.size() >= suffix.size() && before.compare(before.size() - suffix.size(), suffix.size(), suffix) == 0;
            };
            std::string kind;
            if (ends("skill:")) kind = "skill";
            else if (ends("item:")) kind = "item";
            else if (ends("enemy:") || ends("charge:") || ends("guard=")) kind = "enemy";
            else if (before.size() >= 8 && before.compare(before.size() - 8, 6, "target") == 0 &&
                     before[before.size() - 2] >= '1' && before[before.size() - 2] <= '8' && before.back() == '=')
                kind = "enemy";
            else if (ends("ally:") || ends("dead:"))
                throw Failure(where(at) + ": party members are native character IDs 1 to 8 (1 = Yu, 2 = Yosuke); "
                                          "the catalog names skills, items and enemies, not party members");
            else
                throw Failure(where(at) + ": a quoted name is resolved only after skill:, item:, enemy:, charge:, guard= "
                                          "or target1= to target8=");
            plan.uses.push_back({number, (int)at + 1, start + at, start + close + 1, catalog_ref(refs, kind, query)});
            at = close;
        }
        start += line.size() + 1;
    }
    if (!commands) throw Failure(path.string() + ": the battle plan has no battle: line");
    return plan;
}
/* A Luau string literal holding exactly `text`: everything but plain characters as \ddd. */
static std::string luau_string(const std::string &text)
{
    std::string out = "\"";
    char escape[8];
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == ' ' || c == '-' || c == '.' || c == ':' || c == '#' || c == '_') {
            out += (char)c;
        } else {
            std::snprintf(escape, sizeof escape, "\\%03u", (unsigned)c);
            out += escape;
        }
    }
    return out + "\"";
}
static const std::string RESOLVER_ID = "p4mod.resolver";
/* The catalog lives in the game: it is captured from the user's disc while the game boots, with the
 * loaded tables' renamed and added rows. So the game answers. A private resolver package, enabled
 * beside the mod under test (whose added and renamed rows are then the ones the battle sees), looks
 * every reference up through p4.data at its first frame, after every main.luau ran, and writes the
 * row's ID and key or the catalog's own error (unknown name with the nearest names, a shared name
 * with the candidates' keys) to its own artifact file. That file is the only channel trusted: the
 * engine writes p4.artifacts only into mod-artifacts/<the calling mod's id>/ beside this launch's
 * private p4pc.ini (mod_artifacts.c), each mod runs in its own VM with no filesystem library, and
 * p4.log lines (which any mod can shape, newlines included) are never read as answers. A boot
 * launch to frame 900, as the engine's own gate result reports, carries it. Returns "" or why
 * nothing was answered; each reference holds its answer. */
static std::map<std::string, std::string> package_digests(const fs::path &root);
static std::string resolve_names(const TestTarget &target, const fs::path &dir, const fs::path &work, std::vector<CatalogRef> &refs,
                                 const std::map<std::string, std::string> &files)
{
    const fs::path root = work / "resolve", resolver = root / "mods" / RESOLVER_ID;
    if (!fs::create_directory(root) || !fs::create_directory(root / "card") || !fs::create_directory(root / "mods") ||
        !fs::create_directory(resolver))
        throw Failure("cannot create private resolver folders in " + root.string());
    fs::copy(dir, root / "mods" / target.id, fs::copy_options::recursive);
    if (package_digests(root / "mods" / target.id) != files)
        throw Failure("the mod folder changed while it was copied to " + (root / "mods" / target.id).string() + "; run the test again");
    write_new_file(resolver / "mod.ini", "[mod]\nid = " + RESOLVER_ID + "\nname = p4mod name resolver\nversion = 1\napi = 1." +
                                             std::to_string(P4MOD_API_MINOR) + "\npriority = 0\norder = 0\n"
                                             "requires = data.catalog@1, events.frame_start@1, artifacts@1\n");
    std::string script = "-- p4mod test: the names a battle uses, answered by the game's own catalog.\nlocal queries = {\n";
    for (const CatalogRef &ref : refs) script += "    {" + luau_string(ref.kind) + ", " + luau_string(ref.query) + "},\n";
    script += "}\n"
              "local answered = false\n"
              "p4.events.on(\"frame_start\", function()\n"
              "    if answered then return end\n"
              "    answered = true\n"
              "    local lines = {\"p4mod-catalog 1\"}\n"
              "    for i, query in queries do\n"
              "        local ok, row = pcall(p4.data[query[1]], query[2])\n"
              "        if ok then\n"
              "            table.insert(lines, i .. \" ok \" .. row.id .. \" \" .. row.key)\n"
              "        else\n"
              "            table.insert(lines, i .. \" error \" .. (string.gsub(tostring(row), \"%s+\", \" \")))\n"
              "        end\n"
              "    end\n"
              "    table.insert(lines, \"done\")\n"
              "    local written, why = p4.artifacts.write(\"answers.txt\", table.concat(lines, \"\\n\") .. \"\\n\")\n"
              "    if not written then p4.log(\"cannot write its answers: \" .. tostring(why)) end\n"
              "end)\n";
    write_new_file(resolver / "main.luau", script);
    write_new_file(root / "p4pc.ini", "[game]\niso=" + target.iso + "\n[mods]\nenabled=" + target.id + "," + RESOLVER_ID + "\n");
    const fs::path gate_file = root / "gate-result.txt";
    std::map<std::string, std::string> env = launch_environment(root, target.iso, gate_file);
    env["P4PC_NEWGAME_GATE"] = "900,-1:-1";
    const fs::path log = root / "game.log";
    const int code = run_game(target.game, {}, env, log, root, true);
    const std::string text = read_file(log);
    print_mod_lines(text, target.id, false);
    print_mod_lines(text, RESOLVER_ID, false);
    if (text.find("p4pc: data edits refused: ") != std::string::npos)
        return "the game refused the mod's data edits; the data edits line above says why";
    /* The launch itself must have run to its gate, as the engine (not the log) reports. */
    const std::map<std::string, long long> gate = read_gate_result(gate_file);
    if (code != 0 || !gate.count("ok") || gate.at("ok") != 1)
        return "the resolver's launch did not reach its frame-900 gate (exit " + std::to_string(code) + "); its log is " + log.string();
    const fs::path answers = root / "mod-artifacts" / RESOLVER_ID / "answers.txt";
    std::error_code error;
    if (!fs::is_regular_file(fs::symlink_status(answers, error)) || error || fs::file_size(answers, error) > (1u << 20) || error)
        return "the game's catalog did not answer (no " + answers.string() + "); the resolver's launch log is " + log.string();
    /* Strict: the header, each reference answered exactly once, then "done" and nothing after. */
    const std::vector<std::string> lines = split(read_file(answers), '\n');
    std::vector<int> seen(refs.size(), 0);
    bool done = false, malformed = lines.empty() || lines[0] != "p4mod-catalog 1";
    for (size_t n = 1; !malformed && n < lines.size(); n++) {
        const std::string &line = lines[n];
        if (done) { malformed = !line.empty() || n + 1 != lines.size(); continue; }
        if (line == "done") { done = true; continue; }
        std::istringstream words(line);
        size_t index = 0;
        std::string verdict;
        if (!(words >> index >> verdict) || !index || index > refs.size() || seen[index - 1]++) { malformed = true; continue; }
        CatalogRef &ref = refs[index - 1];
        if (verdict == "ok") {
            double id = -1;
            std::string key, extra;
            if (!(words >> id >> key) || words >> extra || id < 0 || id > 4294967295.0 || id != (double)(uint32_t)id) {
                malformed = true;
                continue;
            }
            ref.answered = true;
            ref.id = (uint32_t)id;
            ref.key = key;
        } else if (verdict == "error") {
            std::string message;
            std::getline(words, message);
            ref.answered = true;
            ref.error = trim(message);
        } else {
            malformed = true;
        }
    }
    if (malformed || !done) return "the resolver's answer file " + answers.string() + " is not a complete answer";
    for (CatalogRef &ref : refs)
        if (!ref.answered) return "the resolver's answer file " + answers.string() + " is not a complete answer";
    return "";
}

/* JSON text as Python's json.dumps writes it (ensure_ascii), so the mod-set identity is the one
 * tools/battle_sim.py computes for the same package. */
static std::string json_string(const std::string &text)
{
    std::string out = "\"";
    char escape[16];
    for (size_t i = 0; i < text.size();) {
        const unsigned char lead = (unsigned char)text[i];
        uint32_t point = lead;
        size_t length = 1;
        if (lead >= 0x80) {
            length = lead >= 0xF8 ? 0 : lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 0;
            if (!length || i + length > text.size()) throw Failure("\"" + text + "\" is not UTF-8");
            point = lead & (0x7Fu >> length);
            for (size_t k = 1; k < length; k++) {
                const unsigned char next = (unsigned char)text[i + k];
                if ((next & 0xC0) != 0x80) throw Failure("\"" + text + "\" is not UTF-8");
                point = point << 6 | (next & 0x3Fu);
            }
        }
        i += length;
        if (point == '"') out += "\\\"";
        else if (point == '\\') out += "\\\\";
        else if (point == '\n') out += "\\n";
        else if (point == '\r') out += "\\r";
        else if (point == '\t') out += "\\t";
        else if (point == '\b') out += "\\b";
        else if (point == '\f') out += "\\f";
        else if (point >= 0x20 && point <= 0x7e) out += (char)point;
        else {
            if (point > 0xFFFF) {
                point -= 0x10000;
                std::snprintf(escape, sizeof escape, "\\u%04x\\u%04x", 0xD800 + (point >> 10), 0xDC00 + (point & 0x3FF));
            } else {
                std::snprintf(escape, sizeof escape, "\\u%04x", point);
            }
            out += escape;
        }
    }
    return out + "\"";
}
/* {relative path: SHA-256} of every regular file of a package; links, reparse points and special files refused. */
static std::map<std::string, std::string> package_digests(const fs::path &root)
{
    if (package_kind(root) != PackageKind::directory) throw Failure(root.string() + ": package root must be a real directory");
    std::map<std::string, std::string> files;
    for (auto entry = fs::recursive_directory_iterator(root); entry != fs::recursive_directory_iterator(); ++entry) {
        if (package_kind(entry->path()) != PackageKind::file) continue;
        const std::string data = read_package_file(entry->path());
        Sha256 sha;
        sha.update(data.data(), data.size());
        const auto name = entry->path().lexically_relative(root).generic_u8string();
        files[std::string(name.begin(), name.end())] = sha.hex();
    }
    return files;
}
/* The declared mod set's content identity (P4PC_BATTLE_SIM_MODS): SHA-256 of the canonical JSON
 * {packages: [{files, id, version}], settings: {}}, as tools/battle_sim.py names a cohort. */
static std::string mod_set_identity(const std::string &id, const std::string &version, const std::map<std::string, std::string> &files)
{
    std::string text = "{\"packages\":[{\"files\":{";
    for (auto file = files.begin(); file != files.end(); ++file)
        text += (file == files.begin() ? "" : ",") + json_string(file->first) + ":" + json_string(file->second);
    text += "},\"id\":" + json_string(id) + ",\"version\":" + json_string(version) + "}],\"settings\":{}}";
    Sha256 sha;
    sha.update(text.data(), text.size());
    return sha.hex();
}
/* tools/battle_sim.py's seed for one RNG stream (1 additive, 2 MT, 4 effect) of run `run`, master seed 0. */
static uint32_t battle_seed(uint64_t run, uint32_t stream)
{
    std::string prefix("P4PC-BATTLE-SEEDS\0", 18);
    prefix.append(16, '\0');
    for (int i = 0; i < 8; i++) prefix += (char)(run >> (8 * i) & 255);
    for (int i = 0; i < 4; i++) prefix += (char)(stream >> (8 * i) & 255);
    Sha256 sha;
    sha.update(prefix.data(), prefix.size());
    const std::string digest = sha.hex();
    uint32_t value = 0;
    for (int i = 3; i >= 0; i--) value = value << 8 | (uint32_t)std::stoul(digest.substr((size_t)i * 2, 2), nullptr, 16);
    return value;
}

/* Enough JSON to read the worker's result: objects, arrays, strings, numbers, true, false, null. */
struct Json {
    enum Kind { null_value, boolean, number, string, array, object } kind = null_value;
    bool truth = false;
    double value = 0;
    std::string text;
    std::vector<Json> items;
    std::vector<std::pair<std::string, Json>> fields;
    const Json *get(const std::string &key) const
    {
        if (kind != object) return nullptr;
        for (const auto &[name, field] : fields)
            if (name == key) return &field;
        return nullptr;
    }
};
class JsonReader {
public:
    explicit JsonReader(const std::string &source) : s(source) {}
    Json document()
    {
        Json result = value(0);
        space();
        if (at != s.size()) fail();
        return result;
    }

private:
    const std::string &s;
    size_t at = 0;
    [[noreturn]] void fail() { throw Failure("malformed JSON at byte " + std::to_string(at)); }
    void space()
    {
        while (at < s.size() && (s[at] == ' ' || s[at] == '\n' || s[at] == '\r' || s[at] == '\t')) at++;
    }
    bool take(char c)
    {
        space();
        if (at < s.size() && s[at] == c) { at++; return true; }
        return false;
    }
    uint32_t hex4()
    {
        if (s.size() - at < 4) fail();
        uint32_t point = 0;
        for (int i = 0; i < 4; i++, at++) {
            const char c = s[at];
            if (!std::isxdigit((unsigned char)c)) fail();
            point = point << 4 | (uint32_t)(std::isdigit((unsigned char)c) ? c - '0' : std::tolower((unsigned char)c) - 'a' + 10);
        }
        return point;
    }
    std::string text()
    {
        if (!take('"')) fail();
        std::string out;
        for (;;) {
            if (at >= s.size()) fail();
            const char c = s[at++];
            if (c == '"') return out;
            if ((unsigned char)c < 0x20) fail();
            if (c != '\\') { out += c; continue; }
            if (at >= s.size()) fail();
            const char e = s[at++];
            if (e == '"' || e == '\\' || e == '/') out += e;
            else if (e == 'b') out += '\b';
            else if (e == 'f') out += '\f';
            else if (e == 'n') out += '\n';
            else if (e == 'r') out += '\r';
            else if (e == 't') out += '\t';
            else if (e == 'u') {
                uint32_t point = hex4();
                if (point >= 0xD800 && point < 0xDC00) {
                    if (s.compare(at, 2, "\\u")) fail();
                    at += 2;
                    const uint32_t low = hex4();
                    if (low < 0xDC00 || low > 0xDFFF) fail();
                    point = 0x10000 + ((point - 0xD800) << 10) + (low - 0xDC00);
                }
                if (point < 0x80) out += (char)point;
                else if (point < 0x800) { out += (char)(0xC0 | point >> 6); out += (char)(0x80 | (point & 0x3F)); }
                else if (point < 0x10000) {
                    out += (char)(0xE0 | point >> 12); out += (char)(0x80 | (point >> 6 & 0x3F)); out += (char)(0x80 | (point & 0x3F));
                } else {
                    out += (char)(0xF0 | point >> 18); out += (char)(0x80 | (point >> 12 & 0x3F));
                    out += (char)(0x80 | (point >> 6 & 0x3F)); out += (char)(0x80 | (point & 0x3F));
                }
            } else fail();
        }
    }
    Json value(unsigned depth)
    {
        if (depth > 64) fail();
        space();
        if (at >= s.size()) fail();
        Json result;
        if (take('{')) {
            result.kind = Json::object;
            if (take('}')) return result;
            do {
                std::string key = text();
                if (!take(':')) fail();
                result.fields.emplace_back(std::move(key), value(depth + 1));
            } while (take(','));
            if (!take('}')) fail();
        } else if (take('[')) {
            result.kind = Json::array;
            if (take(']')) return result;
            do result.items.push_back(value(depth + 1));
            while (take(','));
            if (!take(']')) fail();
        } else if (s[at] == '"') {
            result.kind = Json::string;
            result.text = text();
        } else if (s.compare(at, 4, "true") == 0 || s.compare(at, 5, "false") == 0) {
            result.kind = Json::boolean;
            result.truth = s[at] == 't';
            at += result.truth ? 4 : 5;
        } else if (s.compare(at, 4, "null") == 0) {
            at += 4;
        } else {
            const size_t start = at;
            while (at < s.size() && (std::isdigit((unsigned char)s[at]) || std::strchr("+-.eE", s[at]))) at++;
            const std::string number = s.substr(start, at - start);
            char *end = nullptr;
            result.kind = Json::number;
            result.value = std::strtod(number.c_str(), &end);
            if (number.empty() || *end) fail();
        }
        return result;
    }
};

/* A decimal or 0x-hexadecimal whole number up to `maximum`. */
static uint64_t parse_count(const std::string &text, uint64_t maximum, const std::string &option)
{
    const bool hex = text.size() > 2 && (text.compare(0, 2, "0x") == 0 || text.compare(0, 2, "0X") == 0);
    const std::string digits = hex ? text.substr(2) : text;
    const uint64_t base = hex ? 16 : 10;
    if (digits.empty()) throw std::invalid_argument(option + " is a number from 0 to " + std::to_string(maximum));
    uint64_t value = 0;
    for (char c : digits) {
        const int digit = std::isdigit((unsigned char)c) ? c - '0'
                          : hex && std::isxdigit((unsigned char)c) ? std::tolower((unsigned char)c) - 'a' + 10 : -1;
        if (digit < 0 || value > (maximum - (uint64_t)digit) / base)
            throw std::invalid_argument(option + " is a number from 0 to " + std::to_string(maximum));
        value = value * base + (uint64_t)digit;
    }
    return value;
}

static const std::map<std::string, uint32_t> BATTLE_POLICIES = {{"greedy", 1}, {"cautious", 2}, {"typical", 3}, {"attentive", 5}};
static const uint32_t BATTLE_WALK_POLICY = 4, BATTLE_UPDATES = 100000, BATTLE_BOOTSTRAP = 100000, BATTLE_ACTIONS = 10000;
struct BattleRequest {
    fs::path save;
    std::string encounter; /* a native ID (decimal or 0x hex) or a formation key */
    std::string policy = "typical";
    std::optional<fs::path> plan;
    uint64_t seed = 0;
};
/* One native battle (docs/platform/battle_sim.md): the save's party against the encounter, played by a
 * native party policy or a battle plan, with this mod as the run's declared mod set. The game resolves
 * everything, names included; this only builds the entry, runs the worker and reads its result. */
static int command_test_battle(const fs::path &dir, const BattleRequest &battle, std::optional<fs::path> game, unsigned frames)
{
    /* The declared mod set is the one mod-witness simulator run: an inherited marker is refused, never reset. */
    for (const auto &[key, value] : inherited_environment()) {
        std::string upper = key;
        for (char &c : upper) c = (char)std::toupper((unsigned char)c);
        if (upper == "P4PC_MOD_WITNESS")
            throw Failure("P4PC_MOD_WITNESS is set in this environment; the battle test sets it itself and refuses an "
                          "inherited one, even an empty one. Unset it and run again");
    }
    const std::optional<TestTarget> target = test_target(dir, game);
    if (!target) return 1;
    const std::string &id = target->id;
    if (id == RESOLVER_ID) throw Failure("the mod id " + RESOLVER_ID + " is p4mod's own name resolver; choose another id");
    const SaveInput save = read_save(battle.save);
    /* Encounters are formations, which the catalog keys (p4:formation:#17, or the key a mod gives an
     * added one) but does not name: a key goes to the catalog, a number is the native ID. */
    std::vector<CatalogRef> refs;
    std::optional<size_t> encounter_ref;
    uint32_t encounter = 0;
    if (!battle.encounter.empty() && std::isdigit((unsigned char)battle.encounter[0]))
        encounter = (uint32_t)parse_count(battle.encounter, 65535, "--encounter");
    else if (battle.encounter.find(':') != std::string::npos)
        encounter_ref = catalog_ref(refs, "formation", battle.encounter);
    else
        throw Failure("--encounter \"" + battle.encounter + "\": encounters are formations, and the game's catalog gives "
                      "formations keys but no names. Give the native ID or the formation's key (p4:formation:#<id>, or "
                      "the key a mod gives a formation it adds)");
    const BattlePlan plan = battle.plan ? read_battle_plan(*battle.plan, refs) : BattlePlan{};
    const uint32_t policy = battle.plan ? BATTLE_WALK_POLICY : BATTLE_POLICIES.at(battle.policy);
    const std::string elf = disc_elf(target->iso);
    const std::map<std::string, std::string> files = package_digests(dir);
    const uint32_t updates = frames ? frames : BATTLE_UPDATES;

    const fs::path work = make_test_dir(id);
    const fs::path log = work / "game.log", result = work / "result.json";
    std::cout << "p4mod test: " << id << " (battle), log " << log.string() << "\n";
    /* tools/mod_witness.py's marker: everything in this folder is mod-witness output, never a retail reference. */
    write_new_file(work / "p4pc-run-mode.json",
                   "{\n  \"mod_witness\": true,\n  \"reference_eligible\": false,\n  \"run_mode\": \"mod-witness\",\n  \"schema\": 1\n}\n");

    std::string plan_text = plan.text;
    if (!refs.empty()) {
        std::cout << "p4mod test: resolving " << refs.size() << " name(s) and key(s) with the game's catalog\n";
        if (const std::string problem = resolve_names(*target, dir, work, refs, files); !problem.empty()) {
            std::cout << "p4mod test: FAIL: " << problem << "\n";
            return 1;
        }
        std::vector<std::string> errors;
        if (encounter_ref) {
            const CatalogRef &ref = refs[*encounter_ref];
            if (!ref.error.empty()) errors.push_back("--encounter: error: " + ref.error);
            else if (ref.id > 65535) errors.push_back("--encounter: error: " + ref.key + " is formation " + std::to_string(ref.id) +
                                                      ", past the battle simulator's encounter IDs 0 to 65535");
            else encounter = ref.id;
        }
        plan_text.clear();
        size_t copied = 0;
        for (const BattlePlan::Use &use : plan.uses) {
            const CatalogRef &ref = refs[use.ref];
            const std::string where = plan.file.string() + ":" + std::to_string(use.line) + ":" + std::to_string(use.column);
            if (!ref.error.empty()) errors.push_back(where + ": error: " + ref.error);
            else if (ref.kind == "item" && (ref.id < 0x301 || ref.id > 0x3FF))
                errors.push_back(where + ": error: \"" + ref.query + "\" is item " + std::to_string(ref.id) + " (" + ref.key +
                                 "), not a battle item the Item list holds (0x301 to 0x3FF)");
            plan_text += plan.text.substr(copied, use.begin - copied) + std::to_string(ref.id);
            copied = use.end;
        }
        plan_text += plan.text.substr(copied);
        for (const CatalogRef &ref : refs)
            if (ref.error.empty()) std::cout << "  " << ref.kind << " \"" << ref.query << "\" = " << ref.id << " (" << ref.key << ")\n";
        if (!errors.empty()) {
            for (const std::string &error : errors) std::cout << error << "\n";
            std::cout << "p4mod test: FAIL: the game's catalog did not resolve every name above to the row the battle needs\n";
            return 1;
        }
    }
    if (plan_text.size() > 16384) throw Failure(plan.file.string() + ": the resolved battle plan has more than 16384 bytes");

    if (!fs::create_directory(work / "card") || !fs::create_directory(work / "mods") || !fs::create_directory(work / "tmp"))
        throw Failure("cannot create private test folders in " + work.string());
    const fs::path staged = work / "mods" / id;
    fs::copy(dir, staged, fs::copy_options::recursive);
    if (package_digests(staged) != files)
        throw Failure("the mod folder changed while it was copied to " + staged.string() + "; run the test again");
    const std::string identity = mod_set_identity(id, target->version, files);
    write_new_file(work / "SLUS_217.82", elf);
    if (battle.plan) write_new_file(work / "plan.txt", plan_text);
    /* P4BSIM01: renderless, seeded, the save image, then the plan (walk policy) or nothing. */
    std::string entry = "P4BSIM01";
    const uint32_t header[] = {64, 1, encounter, battle_seed(battle.seed, 1), battle_seed(battle.seed, 2), updates,
                               (uint32_t)plan_text.size(), (uint32_t)save.image.size(), 0, BATTLE_BOOTSTRAP, policy, 1,
                               BATTLE_ACTIONS, battle_seed(battle.seed, 4)};
    for (uint32_t word : header) put32(entry, word);
    entry += save.image;
    entry += plan_text;
    write_new_file(work / "entry.bin", entry);
    write_new_file(work / "p4pc.ini", "[game]\niso=" + target->iso + "\n[mods]\nenabled=" + id + "\n");
    const std::string tmp = (work / "tmp").string();
    const std::map<std::string, std::string> env = {
        {"P4PC_CONFIG", (work / "p4pc.ini").string()}, {"P4PC_SAVE_DIR", (work / "card").string()}, {"P4PC_ISO", target->iso},
        {"P4PC_MODS", "1"}, {"P4PC_PACKS", "0"}, {"P4PC_MOD_DIR", (work / "mods").string()}, {"P4PC_MOD_WITNESS", "1"},
        {"P4PC_BATTLE_SIM_MODS", identity}, {"P4PC_INPUT", "script"}, {"P4PC_HIDDEN_WINDOW", "1"},
        {"SDL_AUDIODRIVER", "dummy"}, {"P4PC_VIRTUAL_CLOCK", "1"}, {"P4PC_TURBO", "1"}, {"P4PC_ASPECT", "4:3"},
        {"P4PC_RESOLUTION", "640x448"}, {"P4PC_WINDOW_W", "640"}, {"P4PC_WINDOW_H", "448"}, {"P4PC_FULLSCREEN", "0"},
        {"P4PC_VSYNC", "off"}, {"P4PC_PRESENT", "direct"}, {"P4PC_PRESENT_FIT", "4:3"},
        {"TMPDIR", tmp}, {"TMP", tmp}, {"TEMP", tmp}};
    const int code = run_game(target->game,
                              {(work / "SLUS_217.82").string(), "--battle-sim", (work / "entry.bin").string(), "--battle-result",
                               result.string(), "--battle-trace", (work / "trace.jsonl").string()},
                              env, log, work, true);
    const std::string text = read_file(log);
    print_mod_lines(text, id, true);
    if (save.mod_data)
        std::cout << "  note: " << save.file.parent_path().string() << " holds mod save data (.p4mods); a battle starts "
                  << "from the save image alone, so save providers load nil\n";

    std::string problem;
    Json value;
    if (!fs::is_regular_file(result)) {
        problem = "the battle worker published no result (exit " + std::to_string(code) + "); the battle-sim lines above say why";
    } else {
        try {
            value = JsonReader(read_file(result)).document();
        } catch (const Failure &error) {
            problem = std::string("the battle worker's result is unreadable: ") + error.what();
        }
    }
    if (problem.empty()) {
        auto field = [](const Json *object, const char *key) { return object ? object->get(key) : nullptr; };
        auto text_of = [](const Json *v) { return v && v->kind == Json::string ? v->text : std::string(); };
        auto number_of = [](const Json *v) { return v && v->kind == Json::number ? v->value : -1.0; };
        static const std::set<std::string> ended = {"win", "defeat", "escape", "scripted_conclusion"},
                                           censored = {"update_limit", "action_limit"}, errors = {"engine_error", "policy_error"};
        const std::string outcome = text_of(field(&value, "outcome")), cause = text_of(field(&value, "terminal_cause"));
        const Json *mods = field(&value, "mods"), *party = field(&value, "party_policy"), *active = field(mods, "active");
        std::vector<std::pair<std::string, std::string>> running;
        if (active && active->kind == Json::array)
            for (const Json &item : active->items) running.emplace_back(text_of(field(&item, "id")), text_of(field(&item, "version")));
        Sha256 plan_sha;
        plan_sha.update(plan_text.data(), plan_text.size());
        if (number_of(field(&value, "schema")) != 1 || (!ended.count(outcome) && !censored.count(outcome) && !errors.count(outcome))) {
            problem = "the battle worker's result has no known outcome";
        } else {
            std::cout << "p4mod test: battle " << outcome << " (" << cause << ") after "
                      << (uint64_t)std::max(0.0, number_of(field(&value, "updates"))) << " native updates; encounter "
                      << encounter << ", " << text_of(field(party, "name")) << ", seed " << battle.seed << "\n";
            for (const auto &[package, version] : running) std::cout << "  active mod: " << package << " " << version << "\n";
            if (number_of(field(&value, "encounter_id")) != (double)encounter)
                problem = "the battle worker's result is for another encounter";
            else if (!mods || mods->kind != Json::object)
                problem = "this game ran the battle without a declared mod set (its result names none); use a game "
                          "build whose battle simulator admits mod sets";
            else if (text_of(field(mods, "identity")) != identity)
                problem = "the battle worker's result names another mod set";
            else if (number_of(field(party, "id")) != (double)policy ||
                     (battle.plan && text_of(field(party, "plan_sha256")) != plan_sha.hex()))
                problem = "the battle worker's result names another party policy or plan";
            else if (text.find("p4pc: data edits refused: ") != std::string::npos)
                problem = "the game refused the mod's data edits; the data edits line above says why";
            else if (text.find("p4pc: mods: enabled " + id + " ") == std::string::npos)
                problem = "the game did not enable the mod; the mods: lines above say why";
            else if (text.find("package recovery") != std::string::npos || text.find("native mod message") != std::string::npos)
                problem = "the mod failed while running; its error is the last mod line above";
            else if (errors.count(outcome))
                problem = "the battle could not run: " + outcome + " (" + cause + ")";
            else if (running != std::vector<std::pair<std::string, std::string>>{{id, target->version}})
                problem = "the game did not keep exactly this mod active through the battle";
            else if (censored.count(outcome))
                problem = "the battle did not end within " + std::to_string(updates) + " native updates (" + outcome +
                          "); raise --frames, or choose another --policy or --plan";
            else if (code != 0)
                problem = "the game exited with " + std::to_string(code) + " after the battle ended; see the log";
        }
    }
    if (problem.empty()) {
        std::cout << "p4mod test: PASS (evidence in " << work.string() << ")\n";
        return 0;
    }
    std::cout << "p4mod test: FAIL: " << problem << "\n";
    return 1;
}

/* A zip (stored entries for small files, deflate otherwise) written with miniz's
 * raw deflate and CRC-32: the archive format alone, no external tool. */
static int command_pack(const fs::path &dir, std::optional<fs::path> out)
{
    fs::path root = dir;
    if (root.filename().empty() && root != root.root_path()) root = root.parent_path();
    if (package_kind(root) != PackageKind::directory)
        throw Failure(root.string() + ": package root must be a real directory");
    if (command_check(root)) {
        std::cout << "p4mod pack: not packed: fix the errors above first\n";
        return 1;
    }
    Capabilities caps = load_capabilities();
    Report report;
    Manifest manifest = read_manifest(root, report, caps);
    const std::string id = manifest.fields["id"], version = manifest.fields["version"];
    /* The canonical folder ("." and ".." and a trailing separator resolved): the zip goes beside it. */
    const fs::path folder = fs::canonical(root);
    require_ascii(folder, "the mod folder (resolved)");
    if (!out) {
        const bool plain = !version.empty() && version[0] != '.' && version.find("..") == std::string::npos &&
            std::all_of(version.begin(), version.end(), [](unsigned char c) {
                return std::isalnum(c) || c == '.' || c == '_' || c == '-' || c == '+';
            });
        if (!plain)
            throw Failure("version \"" + version + "\" cannot name the zip (letters, digits, '.', '_', '-' or '+', not "
                          "starting with '.'); pass --out FILE.zip");
        out = folder.parent_path() / (id + "-" + version + ".zip");
    }
    const std::string out_name = out->filename().string();
    if (out_name.empty() || out_name == "." || out_name == ".." || out_name.find(':') != std::string::npos)
        throw Failure(out->string() + ": not a file name for the zip (no ':' streams, no folder); pass --out FILE.zip");
    /* Every entry the game's index accepts (no links, special or executable entries, names with
       \ : ; or control characters, or case collisions), less hidden entries and zips. */
    const Package package(folder, id);
    if (!package.index) throw Failure(folder.string() + ": the game's package index refuses this folder; see above");
    std::vector<std::string> files;
    for (size_t i = 0, count = p4mod_package_count(package.index); i < count; ++i) {
        const char *relative = nullptr;
        int is_directory = 0;
        if (p4mod_package_entry(package.index, i, &relative, &is_directory)) break;
        const std::string name = relative;
        if (is_directory || fs::path(name).extension() == ".zip") continue;
        bool hidden = false;
        for (const std::string &part : split(name, '/')) {
            if (part.empty() || part == "." || part == ".." || part.find_first_of("\\:") != std::string::npos)
                throw Failure(name + ": not a package entry name the game accepts");
            hidden = hidden || part[0] == '.';
        }
        if (!hidden) files.push_back(name);
    }
    if (files.size() >= UINT16_MAX) throw Failure("p4mod pack: too many files for ZIP32");
    std::string archive, directory;
    P4ModZip32 layout = {};
    for (const std::string &relative : files) {
        const fs::path file = folder / fs::u8path(relative);
        const std::string name = id + "/" + relative;
        P4ModZip32 trial = layout;
        if (!p4mod_zip32_add(&trial, name.size(), 0, 0))
            throw Failure(file.string() + ": package exceeds ZIP32 name, count or archive limits");
        const std::string data = read_package_file(file);
        const uint32_t crc = (uint32_t)mz_crc32(MZ_CRC32_INIT, (const unsigned char *)data.data(), data.size());
        size_t packed_size = 0;
        void *packed = data.size() > 64 ? tdefl_compress_mem_to_heap(data.data(), data.size(), &packed_size, 128) : nullptr;
        const bool deflated = packed && packed_size < data.size();
        if (!p4mod_zip32_add(&layout, name.size(), data.size(), deflated ? packed_size : data.size())) {
            std::free(packed);
            throw Failure(file.string() + ": package exceeds ZIP32 size or archive limits");
        }
        const std::string body = deflated ? std::string((const char *)packed, packed_size) : data;
        std::free(packed);
        const uint32_t offset = (uint32_t)archive.size();
        auto header = [&](std::string &to, bool central) {
            put32(to, central ? 0x02014b50 : 0x04034b50);
            if (central) put16(to, 20);
            put16(to, 20); put16(to, 0x0800); put16(to, deflated ? 8 : 0); put16(to, 0); put16(to, 0x21);
            put32(to, crc); put32(to, (uint32_t)body.size()); put32(to, (uint32_t)data.size());
            put16(to, (unsigned)name.size()); put16(to, 0);
            if (central) { put16(to, 0); put16(to, 0); put16(to, 0); put32(to, 0); put32(to, offset); }
            to += name;
        };
        header(archive, false);
        archive += body;
        header(directory, true);
    }
    const uint32_t start = (uint32_t)archive.size();
    archive += directory;
    put32(archive, 0x06054b50); put16(archive, 0); put16(archive, 0);
    put16(archive, (unsigned)files.size()); put16(archive, (unsigned)files.size());
    put32(archive, (uint32_t)directory.size()); put32(archive, start); put16(archive, 0);
    write_new_file(*out, archive);
    std::cout << "p4mod pack: wrote " << out->string() << " (" << files.size() << " files)\n";
    return 0;
}

/* ---- argument handling ------------------------------------------------------------ */

/* The folder of this executable as the OS knows it, never argv[0]: a bare `p4mod` run
 * through PATH must not read data or start a game from the current folder. AppImage:
 * /proc/self/exe is usr/bin/p4mod inside the mounted image, beside its data files. */
static std::optional<fs::path> executable_dir()
{
#ifdef _WIN32
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
        if (!length) return std::nullopt;
        if (length < buffer.size()) return fs::path(std::wstring(buffer.data(), length)).parent_path();
        if (buffer.size() >= 32768) return std::nullopt;
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1);
    if (_NSGetExecutablePath(buffer.data(), &size)) return std::nullopt;
    std::error_code error;
    const fs::path path = fs::canonical(fs::path(buffer.data()), error);
    return error ? std::nullopt : std::optional<fs::path>(path.parent_path());
#else
    std::error_code error;
    const fs::path path = fs::read_symlink("/proc/self/exe", error);
    return error ? std::nullopt : std::optional<fs::path>(path.parent_path());
#endif
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    try {
#ifdef _WIN32
        /* The CRT hands main() arguments converted to the ANSI code page, which can best-fit a
           non-ASCII character onto another ASCII path. Look at the UTF-16 command line first and
           refuse anything beyond ASCII (this build's Windows file layer is ASCII-only). */
        for (const wchar_t *c = GetCommandLineW(); c && *c; ++c)
            if (*c > 0x7F)
                throw Failure("the command line has non-ASCII characters, which this build's Windows file layer does "
                              "not support; use names and folders whose paths are plain ASCII");
#endif
        data_dir = executable_dir().value_or(fs::path()); /* empty: data_file and test ask for --data/--game */
        for (size_t i = 0; i + 1 < args.size(); ++i)
            if (args[i] == "--data") { data_dir = args[i + 1]; args.erase(args.begin() + (long)i, args.begin() + (long)i + 2); break; }
        if (args.empty() || args[0] == "--help" || args[0] == "-h" || args[0] == "help") {
            if (args.size() > 1 && HELP.count(args[1])) std::cout << HELP.at(args[1]);
            else std::cout << USAGE;
            return 0;
        }
        const std::string command = args[0];
        if (!HELP.count(command)) {
            std::cerr << "p4mod: unknown command \"" << command << "\"\n\n" << USAGE;
            return 2;
        }
        std::vector<std::string> rest(args.begin() + 1, args.end());
        if (std::find(rest.begin(), rest.end(), "--help") != rest.end() || std::find(rest.begin(), rest.end(), "-h") != rest.end()) {
            std::cout << HELP.at(command);
            return 0;
        }
        std::map<std::string, std::string> options;
        std::vector<std::string> positional;
        static const std::set<std::string> flags = {"--luau", "--files"};
        static const std::map<std::string, std::set<std::string>> allowed = {
            {"setup", {"--disc"}}, {"new", {"--luau", "--files", "--dir"}}, {"check", {}},
            {"test", {"--scenario", "--game", "--frames", "--save", "--encounter", "--policy", "--plan", "--seed"}},
            {"pack", {"--out"}}};
        for (size_t i = 0; i < rest.size(); ++i) {
            const std::string &arg = rest[i];
            if (arg.rfind("--", 0) == 0) {
                if (!allowed.at(command).count(arg)) throw std::invalid_argument("unknown option " + arg + " for `" + command + "`");
                if (flags.count(arg)) { options[arg] = "1"; continue; }
                if (i + 1 >= rest.size()) throw std::invalid_argument(arg + " needs a value");
                options[arg] = rest[++i];
            } else {
                positional.push_back(arg);
            }
        }
        auto one_dir = [&]() {
            if (positional.size() > 1) throw std::invalid_argument("`" + command + "` takes at most one folder");
            return fs::path(positional.empty() ? "." : positional[0]);
        };
        if (command == "check") return command_check(one_dir());
        if (command == "pack") return command_pack(one_dir(), options.count("--out") ? std::optional<fs::path>(options["--out"]) : std::nullopt);
        if (command == "test") {
            const std::string scenario = options.count("--scenario") ? options["--scenario"] : "boot";
            if (scenario != "boot" && scenario != "newgame" && scenario != "battle")
                throw std::invalid_argument("--scenario is boot, newgame or battle");
            unsigned frames = 0;
            if (options.count("--frames")) {
                /* Digits only, 1..1000000 presented frames: no sign, no wrap of "-1" to a near-endless run. */
                const std::string &text = options["--frames"];
                if (text.empty() || text.size() > 7 || !std::all_of(text.begin(), text.end(), [](unsigned char c) {
                        return std::isdigit(c);
                    }) || std::stoul(text) < 1 || std::stoul(text) > 1000000)
                    throw std::invalid_argument("--frames is a whole number of presented frames from 1 to 1000000");
                frames = (unsigned)std::stoul(text);
            }
            const std::optional<fs::path> game = options.count("--game") ? std::optional<fs::path>(options["--game"]) : std::nullopt;
            if (scenario != "battle") {
                for (const char *option : {"--save", "--encounter", "--policy", "--plan", "--seed"})
                    if (options.count(option)) throw std::invalid_argument(std::string(option) + " is only for --scenario battle");
                return command_test(one_dir(), scenario, game, frames);
            }
            if (!options.count("--save") || !options.count("--encounter"))
                throw std::invalid_argument("--scenario battle needs --save <save slot folder or data file> and --encounter <id or key>");
            if (options.count("--policy") && options.count("--plan")) throw std::invalid_argument("choose --policy or --plan, not both");
            BattleRequest battle;
            battle.save = options["--save"];
            battle.encounter = options["--encounter"];
            if (!battle.encounter.empty() && std::isdigit((unsigned char)battle.encounter[0]))
                parse_count(battle.encounter, 65535, "--encounter");
            if (options.count("--policy")) {
                battle.policy = options["--policy"];
                if (!BATTLE_POLICIES.count(battle.policy))
                    throw std::invalid_argument("--policy is typical, greedy, cautious or attentive");
            }
            if (options.count("--plan")) battle.plan = fs::path(options["--plan"]);
            if (options.count("--seed")) battle.seed = parse_count(options["--seed"], UINT64_MAX, "--seed");
            return command_test_battle(one_dir(), battle, game, frames);
        }
        if (command == "new") {
            if (positional.size() != 1) throw std::invalid_argument("`new` takes exactly one mod id");
            if (options.count("--luau") && options.count("--files")) throw std::invalid_argument("choose --luau or --files, not both");
            return command_new(positional[0], !options.count("--files"), options.count("--dir") ? fs::path(options["--dir"]) : fs::path("."));
        }
        if (!options.count("--disc") || !positional.empty()) throw std::invalid_argument("`setup` needs --disc <your disc image>");
        return command_setup(options["--disc"]);
    } catch (const std::invalid_argument &error) {
        std::cerr << "p4mod: " << error.what() << "\n\n" << (args.empty() || !HELP.count(args[0]) ? USAGE : HELP.at(args[0]));
        return 2;
    } catch (const std::exception &error) {
        std::cerr << "p4mod: " << error.what() << "\n";
        return 2;
    }
}
