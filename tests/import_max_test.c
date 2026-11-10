/* Synthetic MAX fixture generator. Arithmetic encoding follows Okumura's
 * public-domain LZARI.C; see import_max.c and third_party/README.md.
 * A deliberately simple exhaustive LZSS search keeps the test independent of
 * production decoding. No retail saves or other external fixtures are used.
 * Test-only private fixture mode: import_max_test --encode FOLDER OUT.max.
 * It encodes ordinary files from one save directory (no subdirectories),
 * retaining that directory's basename; compressed output is capped at 256KiB. */
#include "../src/platform/memcard/import_max.h"
#include "p4pc/os.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); \
} } while (0)
#define Q1 32768u
#define Q2 65536u
#define Q3 98304u
#define Q4 131072u
#define CHARS 314u
#define CAPACITY 262144u

typedef struct Encoder {
    unsigned char output[CAPACITY];
    unsigned char history[4096];
    size_t used;
    unsigned int byte, mask, pending;
    unsigned int freq[CHARS + 1], cum[CHARS + 1];
    unsigned int to_symbol[CHARS], to_char[CHARS + 1], positions[4097];
    uint32_t low, high;
    size_t literals, matches, rescales;
} Encoder;

typedef struct Fixture {
    unsigned char bytes[CAPACITY];
    size_t size;
} Fixture;

static void put32(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

/* Independent table-based CRC implementation, unlike the bitwise decoder. */
static void checksum(Fixture *f)
{
    uint32_t table[256], crc = UINT32_MAX, c;
    unsigned int i, bit;
    size_t j;
    memset(f->bytes + 12, 0, 4);
    for (i = 0; i < 256; ++i) {
        c = i;
        for (bit = 0; bit < 8; ++bit)
            c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0);
        table[i] = c;
    }
    for (j = 0; j < f->size; ++j)
        crc = table[(crc ^ f->bytes[j]) & 255] ^ (crc >> 8);
    put32(f->bytes + 12, ~crc);
}

static void bit(Encoder *e, unsigned int value)
{
    if (value) e->byte |= e->mask;
    e->mask >>= 1;
    if (!e->mask) {
        CHECK(e->used < sizeof(e->output));
        e->output[e->used++] = (unsigned char)e->byte;
        e->byte = 0; e->mask = 128;
    }
}

static void emit(Encoder *e, unsigned int value)
{
    bit(e, value);
    while (e->pending) { bit(e, !value); --e->pending; }
}

static void interval(Encoder *e, unsigned int upper, unsigned int lower,
                     unsigned int total)
{
    uint32_t range = e->high - e->low;
    e->high = e->low + (uint32_t)((uint64_t)range * upper / total);
    e->low += (uint32_t)((uint64_t)range * lower / total);
    for (;;) {
        if (e->high <= Q2) emit(e, 0);
        else if (e->low >= Q2) {
            emit(e, 1); e->low -= Q2; e->high -= Q2;
        } else if (e->low >= Q1 && e->high <= Q3) {
            ++e->pending; e->low -= Q1; e->high -= Q1;
        } else break;
        e->low *= 2; e->high *= 2;
    }
}

static void character(Encoder *e, unsigned int ch)
{
    unsigned int symbol = e->to_symbol[ch], i, sum = 0, old;
    interval(e, e->cum[symbol - 1], e->cum[symbol], e->cum[0]);
    if (e->cum[0] >= Q1 - 1) {
        ++e->rescales;
        for (i = CHARS; i; --i) {
            e->cum[i] = sum;
            e->freq[i] = (e->freq[i] + 1) / 2;
            sum += e->freq[i];
        }
        e->cum[0] = sum;
    }
    i = symbol;
    while (e->freq[i] == e->freq[i - 1]) --i;
    old = e->to_char[i];
    e->to_char[i] = ch; e->to_char[symbol] = old;
    e->to_symbol[old] = symbol; e->to_symbol[ch] = i;
    ++e->freq[i];
    while (i) ++e->cum[--i];
}

static void encode(Encoder *e, const unsigned char *raw, size_t size)
{
    size_t offset = 0, k;
    unsigned int i, distance, best, best_distance, run, ring = 4096 - 60;
    memset(e, 0, sizeof(*e));
    e->high = Q4; e->mask = 128;
    for (i = CHARS; i; --i) {
        e->to_symbol[i - 1] = i; e->to_char[i] = i - 1;
        e->freq[i] = 1; e->cum[i - 1] = e->cum[i] + 1;
    }
    for (i = 4096; i; --i)
        e->positions[i - 1] = e->positions[i] + 10000u / (200u + i);
    memset(e->history, ' ', 4096 - 60);
    while (offset < size) {
        best = 0; best_distance = 0;
        for (distance = 1; distance <= 4096; ++distance) {
            for (run = 0; run < 60 && run < size - offset; ++run) {
                unsigned char prior = run >= distance ? raw[offset + run - distance] :
                    e->history[(ring + run - distance) & 4095];
                if (prior != raw[offset + run]) break;
            }
            if (run > best) { best = run; best_distance = distance; }
            if (best == 60) break;
        }
        if (best < 3) {
            best = 1; ++e->literals; character(e, raw[offset]);
        } else {
            ++e->matches; character(e, best + 253);
            interval(e, e->positions[best_distance - 1],
                     e->positions[best_distance], e->positions[0]);
        }
        for (k = 0; k < best; ++k) {
            e->history[ring] = raw[offset++];
            ring = (ring + 1) & 4095;
        }
    }
    ++e->pending;
    emit(e, e->low >= Q1);
    for (i = 0; i < 7; ++i) bit(e, 0);
}

static void fixture(Fixture *f, Encoder *e, const unsigned char *raw,
                    size_t size, unsigned int count)
{
    encode(e, raw, size);
    memset(f, 0, sizeof(*f));
    memcpy(f->bytes, "Ps2PowerSave", 12);
    memcpy(f->bytes + 16, "BASLUS-21782SYNTHETIC", 21);
    memcpy(f->bytes + 48, "Synthetic test title", 20);
    put32(f->bytes + 80, (uint32_t)e->used + 4);
    put32(f->bytes + 84, count);
    put32(f->bytes + 88, (uint32_t)size);
    CHECK(e->used + 92 <= sizeof(f->bytes));
    memcpy(f->bytes + 92, e->output, e->used);
    f->size = e->used + 92;
    checksum(f);
}

static void clear(P4SaveArchive *archive)
{
    size_t i;
    for (i = 0; i < archive->count; ++i) free(archive->files[i].data);
    memset(archive, 0, sizeof(*archive));
}

static void rejected(const unsigned char *bytes, size_t size)
{
    P4SaveArchive archive = {0};
    char error[128] = {0};
    CHECK(!p4save_decode_max(bytes, size, &archive, error, sizeof(error)));
    CHECK(error[0]);
    clear(&archive);
}

static void boundary_tests(Encoder *encoder, Fixture *fixture_data)
{
    P4SaveArchive archive = {0};
    char error[128];
    const size_t limit = 8u * 1024u * 1024u;
    unsigned char *large = (unsigned char *)malloc(limit + 37);
    unsigned char entries[64 * 48] = {0};
    size_t i, offset = 0;
    CHECK(large);
    memset(large, 0, 36);
    memcpy(large + 4, "large", 5);
    memset(large + 36, '@', limit + 1);
    put32(large, (uint32_t)limit);
    fixture(fixture_data, encoder, large, limit + 36, 1);
    CHECK(p4save_decode_max(fixture_data->bytes, fixture_data->size,
                            &archive, error, sizeof(error)));
    CHECK(archive.count == 1 && archive.files[0].size == limit);
    for (i = 0; i < limit; ++i) CHECK(archive.files[0].data[i] == '@');
    clear(&archive);
    /* The decoded framing fits the cap, but actual file payload exceeds it. */
    put32(large, (uint32_t)limit + 1);
    fixture(fixture_data, encoder, large, limit + 37, 1);
    rejected(fixture_data->bytes, fixture_data->size);
    free(large);
    for (i = 0; i < 64; ++i) {
        (void)snprintf((char *)entries + offset + 4, 32, "empty%u", (unsigned int)i);
        offset = ((offset + 36 + 23) & ~(size_t)15) - 8;
    }
    fixture(fixture_data, encoder, entries, offset, 64);
    CHECK(p4save_decode_max(fixture_data->bytes, fixture_data->size,
                            &archive, error, sizeof(error)));
    CHECK(archive.count == 64);
    clear(&archive);
}

typedef struct FolderInput {
    char folder[4096];
    char names[P4SAVE_MAX_FILES][33];
    size_t sizes[P4SAVE_MAX_FILES], count, payload;
    int failed;
} FolderInput;

static int compare_names(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static void collect_file(const char *name, void *user)
{
    FolderInput *input = (FolderInput *)user;
    char path[4096];
    struct stat st;
    int chars;
    size_t length;
    if (input->failed || !strcmp(name, ".") || !strcmp(name, "..")) return;
    chars = snprintf(path, sizeof(path), "%s/%s", input->folder, name);
    if (chars < 0 || (size_t)chars >= sizeof(path) ||
        p4_os_extended_path(path, sizeof(path)) != 0 ||
        stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "Cannot encode non-ordinary or unreadable entry: %s\n", name);
        input->failed = 1;
        return;
    }
    length = strlen(name);
    if (!length || length > 32 || input->count == P4SAVE_MAX_FILES ||
        st.st_size < 0 || (uint64_t)st.st_size > P4SAVE_MAX_BYTES - input->payload) {
        fprintf(stderr, "Fixture name, file count, or payload exceeds MAX limits: %s\n", name);
        input->failed = 1;
        return;
    }
    memcpy(input->names[input->count++], name, length + 1);
    input->payload += (size_t)st.st_size;
}

static int encode_folder(const char *folder, const char *output_path,
                         Encoder *encoder, Fixture *result)
{
    FolderInput input = {{0}, {{0}}, {0}, 0, 0, 0};
    unsigned char *raw = NULL;
    char path[4096], output[4096], error[128], dirname[33];
    P4SaveArchive archive = {0};
    const char *base;
    FILE *file;
    size_t i, length, capacity, offset = 0, data_size, consumed = 0;
    long recorded_size;
    int chars, ok = 0, write_ok;
    length = strlen(folder);
    if (!length || length >= sizeof(input.folder)) goto invalid_path;
    memcpy(input.folder, folder, length + 1);
    while (length && (input.folder[length - 1] == '/' || input.folder[length - 1] == '\\'))
        input.folder[--length] = '\0';
    base = input.folder;
    for (i = 0; i < length; ++i)
        if (input.folder[i] == '/' || input.folder[i] == '\\') base = input.folder + i + 1;
    if (!*base || strlen(base) > 32 || !strcmp(base, ".") || !strcmp(base, ".."))
        goto invalid_path;
    memcpy(dirname, base, strlen(base) + 1);
    if (p4_os_extended_path(input.folder, sizeof(input.folder)) != 0 ||
        p4_os_list_dir(input.folder, collect_file, &input) != 0 ||
        input.failed || !input.count) {
        fprintf(stderr, "Cannot read a nonempty, flat save folder: %s\n", folder);
        return 1;
    }
    qsort(input.names, input.count, sizeof(input.names[0]), compare_names);
    capacity = input.payload + input.count * 51;
    raw = (unsigned char *)calloc(1, capacity);
    if (!raw) { fprintf(stderr, "Cannot allocate fixture input\n"); return 1; }
    for (i = 0; i < input.count; ++i) {
        chars = snprintf(path, sizeof(path), "%s/%s", input.folder, input.names[i]);
        if (chars < 0 || (size_t)chars >= sizeof(path) ||
            p4_os_extended_path(path, sizeof(path)) != 0 || !(file = fopen(path, "rb")))
            goto read_failed;
        if (fseek(file, 0, SEEK_END) != 0 || (recorded_size = ftell(file)) < 0 ||
            (uint64_t)recorded_size > input.payload - consumed ||
            fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            goto read_failed;
        }
        data_size = (size_t)recorded_size;
        input.sizes[i] = data_size;
        put32(raw + offset, (uint32_t)data_size);
        memcpy(raw + offset + 4, input.names[i], strlen(input.names[i]));
        offset += 36;
        if (fread(raw + offset, 1, data_size, file) != data_size ||
            fgetc(file) != EOF || ferror(file)) {
            fclose(file);
            goto read_failed;
        }
        if (fclose(file) != 0) goto read_failed;
        consumed += data_size;
        offset = ((offset + data_size + 23) & ~(size_t)15) - 8;
    }
    fixture(result, encoder, raw, offset, (unsigned int)input.count);
    memset(result->bytes + 16, 0, 32);
    memcpy(result->bytes + 16, dirname, strlen(dirname));
    checksum(result);
    /* Never write a private fixture until its decode exactly matches input. */
    if (!p4save_decode_max(result->bytes, result->size, &archive, error, sizeof(error))) {
        fprintf(stderr, "Generated MAX did not decode: %s\n", error);
        goto done;
    }
    CHECK(archive.count == input.count && !strcmp(archive.name, dirname));
    offset = 0;
    for (i = 0; i < input.count; ++i) {
        CHECK(!strcmp(archive.files[i].name, input.names[i]));
        CHECK(archive.files[i].size == input.sizes[i]);
        CHECK(!memcmp(archive.files[i].data, raw + offset + 36, input.sizes[i]));
        offset = ((offset + 36 + input.sizes[i] + 23) & ~(size_t)15) - 8;
    }
    length = strlen(output_path);
    if (!length || length >= sizeof(output)) goto invalid_output;
    memcpy(output, output_path, length + 1);
    if (p4_os_extended_path(output, sizeof(output)) != 0 ||
        !(file = fopen(output, "wb"))) goto invalid_output;
    write_ok = fwrite(result->bytes, 1, result->size, file) == result->size;
    if (fclose(file) != 0) write_ok = 0;
    if (!write_ok) {
        (void)remove(output);
        goto invalid_output;
    }
    printf("Encoded %s: %zu files, %zu payload bytes, %zu MAX bytes\n",
           dirname, input.count, consumed, result->size);
    ok = 1;
    goto done;
read_failed:
    fprintf(stderr, "Cannot read fixture input (or folder changed): %s\n", path);
    goto done;
invalid_output:
    fprintf(stderr, "Cannot write MAX fixture: %s\n", output_path);
done:
    clear(&archive);
    free(raw);
    return ok ? 0 : 1;
invalid_path:
    fprintf(stderr, "Folder must have a 1..32-byte save directory basename\n");
    return 1;
}

int main(int argc, char **argv)
{
    static const size_t sizes[] = {0, 1, 12, 13, 8000, 40000};
    static unsigned char raw[CAPACITY], payload[48026];
    static Encoder encoder;
    static Fixture good, bad;
    P4SaveArchive archive = {0};
    char error[128], name[33];
    size_t i, j, offset = 0, data_offset = 0, ends[6];
    uint32_t random = 0x12345678u;
    if (argc == 4 && !strcmp(argv[1], "--encode"))
        return encode_folder(argv[2], argv[3], &encoder, &good);
    if (argc != 1) {
        fprintf(stderr, "usage: %s [--encode FOLDER OUT.max]\n", argv[0]);
        return 2;
    }
    for (i = 0; i < sizeof(payload); ++i) {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        payload[i] = i < 8026 ? (unsigned char)(i % 7) : (unsigned char)random;
    }
    for (i = 0; i < 6; ++i) {
        put32(raw + offset, (uint32_t)sizes[i]);
        if (i == 1) memset(raw + offset + 4, 'N', 32); /* Full-width name. */
        else {
            (void)snprintf(name, sizeof(name), "file%u", (unsigned int)i);
            memcpy(raw + offset + 4, name, strlen(name));
        }
        offset += 36;
        memcpy(raw + offset, payload + data_offset, sizes[i]);
        data_offset += sizes[i]; offset += sizes[i]; ends[i] = offset;
        offset = ((offset + 23) & ~(size_t)15) - 8;
    }
    fixture(&good, &encoder, raw, offset, 6);
    CHECK(encoder.matches > 0 && encoder.literals > 32768 && encoder.rescales > 0);
    CHECK(p4save_decode_max(good.bytes, good.size, &archive, error, sizeof(error)));
    CHECK(archive.count == 6 && !strcmp(archive.name, "BASLUS-21782SYNTHETIC"));
    data_offset = 0;
    for (i = 0; i < 6; ++i) {
        CHECK(archive.files[i].size == sizes[i]);
        CHECK(!memcmp(archive.files[i].data, payload + data_offset, sizes[i]));
        if (i == 1) {
            CHECK(strlen(archive.files[i].name) == 32);
            for (j = 0; j < 32; ++j) CHECK(archive.files[i].name[j] == 'N');
        } else {
            (void)snprintf(name, sizeof(name), "file%u", (unsigned int)i);
            CHECK(!strcmp(archive.files[i].name, name));
        }
        for (j = 0; j < 8; ++j)
            CHECK(!archive.files[i].created[j] && !archive.files[i].modified[j]);
        data_offset += sizes[i];
    }
    clear(&archive);
    /* Header truncation, compressed truncation, CRC corruption. */
    for (i = 0; i < 92; ++i) rejected(good.bytes, i);
    rejected(good.bytes, good.size - 1);
    rejected(good.bytes, good.size / 2);
    bad = good; bad.bytes[good.size - 10] ^= 0x80; rejected(bad.bytes, bad.size);
    bad = good; bad.bytes[12] ^= 1; rejected(bad.bytes, bad.size);
    bad = good; memset(bad.bytes + 12, 0, 4); rejected(bad.bytes, bad.size);
    bad = good; bad.bytes[0] = 'X'; rejected(bad.bytes, bad.size);
    bad = good; put32(bad.bytes + 84, 65); checksum(&bad); rejected(bad.bytes, bad.size);
    bad = good; put32(bad.bytes + 88, 8u * 1024u * 1024u + 6u * 51u + 1u);
    checksum(&bad); rejected(bad.bytes, bad.size);
    bad = good; put32(bad.bytes + 88, UINT32_MAX); checksum(&bad);
    rejected(bad.bytes, bad.size);
    bad = good; put32(bad.bytes + 80, 1); checksum(&bad); rejected(bad.bytes, bad.size);
    /* Even with a repaired CRC/size, missing arithmetic input must fail. */
    bad = good; bad.size -= 64; put32(bad.bytes + 80, (uint32_t)bad.size - 88);
    checksum(&bad); rejected(bad.bytes, bad.size);
    /* Both documented compressed-length conventions round-trip. */
    bad = good; put32(bad.bytes + 80, (uint32_t)bad.size - 92); checksum(&bad);
    CHECK(p4save_decode_max(bad.bytes, bad.size, &archive, error, sizeof(error)));
    clear(&archive);
    bad = good; put32(bad.bytes + 80, (uint32_t)offset); checksum(&bad);
    CHECK(p4save_decode_max(bad.bytes, bad.size, &archive, error, sizeof(error)));
    clear(&archive);
    /* Missing final padding is legal; padding between entries is not. */
    fixture(&bad, &encoder, raw, ends[5], 6);
    CHECK(p4save_decode_max(bad.bytes, bad.size, &archive, error, sizeof(error)));
    clear(&archive);
    fixture(&bad, &encoder, raw, ends[0], 2); rejected(bad.bytes, bad.size);
    /* Valid arithmetic, invalid entry bounds; caller frees partial results. */
    put32(raw, UINT32_MAX);
    fixture(&bad, &encoder, raw, offset, 6); rejected(bad.bytes, bad.size);
    put32(raw, 0);
    put32(raw + 40, UINT32_MAX);
    fixture(&bad, &encoder, raw, offset, 6); rejected(bad.bytes, bad.size);
    boundary_tests(&encoder, &bad);
    rejected(NULL, 0);
    CHECK(!p4save_decode_max(good.bytes, good.size, NULL, NULL, 0));
    puts("MAX synthetic roundtrip and rejection tests passed");
    return 0;
}
