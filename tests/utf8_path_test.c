/* Host paths that are not ASCII. The port's paths are UTF-8 (the settings folder from SDL_GetPrefPath, the ISO the player picked, saves, mods, caches);
 * the Windows C library takes the ANSI codepage, so a player whose profile folder is named C:\Users\Zoe-with-an-accent used to get no settings file, no
 * mods and no save folder, and every launch ran on defaults. This runs the real code paths in a directory named with 2-, 3- and 4-byte UTF-8:
 * the host-path layer (include/p4pc/hostpath.h and the p4_os_* file calls), the settings file (p4cfg_load / p4cfg_set_str through P4PC_CONFIG) and the
 * loose-file layer of the virtual file system (P4PC_OVERRIDE_DIR). The memory-card shim has its own run (memcard_test --utf8). On Windows every
 * result is also checked through the wide API, independent of the helper under test, so ANSI mojibake cannot pass; on POSIX the same names pass through
 * unchanged. Usage: utf8_path_test <scratch-root> (ASCII). */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <wchar.h>
#else
#include <unistd.h>
#endif

#include "../src/platform/config/p4cfg.h"
#include "../src/platform/files/vfs.h"
#include "p4pc/hostpath.h"
#include "p4pc/os.h"
#include "p4pc/vfs_source.h"

static int failures;

#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s (errno %d)\n", __FILE__, __LINE__, #c, errno); \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* e-acute, n-tilde, two CJK characters, a four-byte character (surrogate pair in UTF-16). */
#define ROOT_UTF8 "-\xc3\xa9\xc3\xb1-\xe6\x97\xa5\xe6\x9c\xac-\xf0\x9f\x8e\xb4"
#define ROOT_WIDE L"-\x00e9\x00f1-\x65e5\x672c-\xd83c\xdfb4"
#define NAME_A_UTF8 "/f-\xc3\xbc.bin"
#define NAME_A_WIDE L"\\f-\x00fc.bin"

static char gAscii[400]; /* the ASCII scratch root */
static char gRoot[800];  /* gAscii + ROOT_UTF8 */

/* Does the object exist under this exact name, asked of the OS directly (the wide API on Windows, the narrow one elsewhere)? */
static int rawExists(const char *utf8Tail, const wchar_t *wideTail)
{
#ifdef _WIN32
    wchar_t wide[1200];
    size_t n = 0;
    for (const char *c = gAscii; *c; c++) wide[n++] = (wchar_t)(unsigned char)(*c == '/' ? '\\' : *c);
    wcscpy(wide + n, ROOT_WIDE);
    n = wcslen(wide);
    wcscpy(wide + n, wideTail);
    (void)utf8Tail;
    return _waccess(wide, 0) == 0;
#else
    char path[1200];
    struct stat st;
    (void)wideTail;
    snprintf(path, sizeof path, "%s%s%s", gAscii, ROOT_UTF8, utf8Tail);
    return stat(path, &st) == 0;
#endif
}

static void path(char *out, size_t cap, const char *tail)
{
    CHECK(snprintf(out, cap, "%s%s", gRoot, tail) < (int)cap);
}

static void makeDir(const char *directory)
{
    CHECK(!p4_host_mkdir(directory) || errno == EEXIST);
}

static void writeText(const char *file, const char *text)
{
    FILE *f = p4_host_fopen(file, "wb");
    CHECK(f != NULL);
    if (f) {
        CHECK(fputs(text, f) >= 0);
        CHECK(fclose(f) == 0);
    }
}

static char *readAll(const char *file)
{
    static char text[4096];
    FILE *f = p4_host_fopen(file, "rb");
    size_t n;
    if (!f) return NULL;
    n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    return text;
}

static int contentIs(const char *file, const char *expected)
{
    const char *text = readAll(file);
    return text && !strcmp(text, expected);
}

typedef struct { int sawFile, sawOther; } Seen;
static void seeName(const char *name, void *user)
{
    Seen *seen = user;
    if (!strcmp(name, "f-\xc3\xbc.bin")) seen->sawFile = 1;
    else if (strcmp(name, ".") && strcmp(name, "..")) seen->sawOther++;
}

static void checkHostLayer(void)
{
    char a[900], b[900], dir[900];
    P4HostStat st;
    FILE *f;
    Seen seen = {0, 0};

    /* The folder itself, and a file in it, were made under exactly the UTF-8 names. */
    CHECK(rawExists("", L""));
    path(a, sizeof a, NAME_A_UTF8);
    writeText(a, "one");
    CHECK(rawExists(NAME_A_UTF8, NAME_A_WIDE));
    CHECK(contentIs(a, "one"));

    /* Modes: append, update, exclusive creation, and the refusals. */
    f = p4_host_fopen(a, "ab"); CHECK(f && fputs("+two", f) >= 0 && !fclose(f));
    CHECK(contentIs(a, "one+two"));
    f = p4_host_fopen(a, "r+b"); CHECK(f && fputc('O', f) == 'O' && !fclose(f));
    CHECK(contentIs(a, "One+two"));
    errno = 0; CHECK(!p4_host_fopen(a, "wbx") && errno == EEXIST);
    errno = 0; CHECK(!p4_host_fopen(a, "invalid") && errno == EINVAL);
    errno = 0; CHECK(!p4_host_fopen(a, "rz") && errno == EINVAL);
#ifdef _WIN32
    /* Windows cannot name bytes that are not UTF-8 (POSIX paths are bytes, so there it is only a missing file). */
    errno = 0; CHECK(!p4_host_fopen("\xff\xfe-not-utf8", "rb") && errno == EINVAL);
#else
    CHECK(!p4_host_fopen("\xff\xfe-not-utf8", "rb"));
#endif
    errno = 0; CHECK(!p4_host_fopen(NULL, "rb") && errno == EINVAL);

    /* stat: size, kind, absence. */
    CHECK(!p4_host_stat(a, &st) && st.is_file && !st.is_dir && st.size == 7 && st.mtime > 1000000000);
    CHECK(!p4_host_stat(gRoot, &st) && st.is_dir && !st.is_file);
    path(b, sizeof b, "/absent-\xc3\xa9");
    errno = 0; CHECK(p4_host_stat(b, &st) && errno == ENOENT);
    f = p4_os_fopen_rb(a);
    CHECK(f != NULL);
    if (f) fclose(f);

    /* mkdir (the host call and the OS shim's), directory listing, rename, replace, remove, rmdir. */
    path(dir, sizeof dir, "/sub-\xe6\x97\xa5");
    makeDir(dir);
    errno = 0; CHECK(p4_host_mkdir(dir) && errno == EEXIST);
    CHECK(p4_os_is_dir_nofollow(dir));
    path(b, sizeof b, "/sub-\xe6\x97\xa5/g-\xc3\xb1.bin");
    CHECK(!p4_os_rename(a, b));
    CHECK(!p4_host_stat(b, &st) && st.size == 7 && p4_host_stat(a, &st));
    writeText(a, "new");
    CHECK(!p4_os_replace_file(a, b));
    CHECK(contentIs(b, "new"));
    CHECK(p4_os_is_file_nofollow(b));
    CHECK(!p4_os_list_dir(dir, seeName, &seen) && !seen.sawFile && seen.sawOther == 1);
    seen.sawOther = 0;
    writeText(a, "x");
    CHECK(!p4_os_list_dir(gRoot, seeName, &seen) && seen.sawFile);
    CHECK(!p4_host_remove(a) && p4_host_stat(a, &st));
    errno = 0; CHECK(p4_host_remove(a) && errno == ENOENT);
    errno = 0; CHECK(p4_os_rmdir(dir) && errno != 0); /* not empty */
    CHECK(!p4_host_remove(b) && !p4_os_rmdir(dir) && p4_host_stat(dir, &st));
    errno = 0; CHECK(p4_os_list_dir(dir, seeName, &seen) && errno == ENOENT);

    /* The environment carries a UTF-8 path both ways. */
    CHECK(!p4_os_setenv("P4PC_UTF8_PATH_TEST", gRoot, 1));
    CHECK(p4_host_getenv("P4PC_UTF8_PATH_TEST") && !strcmp(p4_host_getenv("P4PC_UTF8_PATH_TEST"), gRoot));
    CHECK(!p4_host_getenv("P4PC_UTF8_PATH_TEST_UNSET"));
}

static void checkSettingsFile(void)
{
    char folder[900], ini[900];
    const char *text;

    path(folder, sizeof folder, "/profile-\xc3\xa9");
    makeDir(folder);
    CHECK(snprintf(ini, sizeof ini, "%s/p4pc.ini", folder) < (int)sizeof ini);
    writeText(ini, "[video]\nwidth = 1111\n[game]\niso = /games/p4-\xc3\xa9.iso\n");
    /* p4cfg_load with the settings file at a non-ASCII path: the case that ran on defaults. */
    CHECK(!p4_os_setenv("P4PC_CONFIG", ini, 1));
    p4cfg_load();
    CHECK(p4cfg_path() && !strcmp(p4cfg_path(), ini));
    CHECK(p4cfg_int("video.width", 0) == 1111);
    CHECK(!strcmp(p4cfg_str("game.iso", ""), "/games/p4-\xc3\xa9.iso"));
    /* A write (temporary file, rename) and a re-read of the startup settings. */
    CHECK(!p4cfg_set_str("video.height", "777"));
    text = readAll(ini);
    CHECK(text && strstr(text, "height = 777") && strstr(text, "width = 1111"));
    writeText(ini, "[video]\nwidth = 2222\n");
    CHECK(!p4cfg_refresh_startup());
    CHECK(p4cfg_int("video.width", 0) == 2222);
    CHECK(rawExists("/profile-\xc3\xa9/p4pc.ini", L"\\profile-\x00e9\\p4pc.ini"));
    {   /* no stale temporary left beside it */
        char tmp[920];
        P4HostStat st;
        CHECK(snprintf(tmp, sizeof tmp, "%s.tmp", ini) < (int)sizeof tmp);
        CHECK(p4_host_stat(tmp, &st) != 0);
    }
    CHECK(!p4_host_remove(ini) && !p4_os_rmdir(folder));
}

static void checkLooseFiles(void)
{
    char dir[900], file[900];
    unsigned char buffer[8];
    int fd, handle, is_dir = -1, found = 0;
    long long size = -1;
    char name[64];

    path(dir, sizeof dir, "/override-\xc3\xb6");
    makeDir(dir);
    CHECK(snprintf(file, sizeof file, "%s/HELLO.TXT", dir) < (int)sizeof file);
    writeText(file, "hello");
    /* The loose-file layer, selected by P4PC_OVERRIDE_DIR as a developer or the mod loader does, with no disc. */
    CHECK(!p4_os_setenv("P4PC_OVERRIDE_DIR", dir, 1));
    CHECK(!vfs_source_prepare_selected());
    CHECK(!vfs_source_mount_selected());
    CHECK(!vfs_stat("HELLO.TXT", &is_dir, &size) && !is_dir && size == 5);
    fd = vfs_open("HELLO.TXT", 0);
    CHECK(fd >= 0 && vfs_read(fd, buffer, sizeof buffer) == 5 && !memcmp(buffer, "hello", 5));
    if (fd >= 0) vfs_close(fd);
    handle = vfs_dir_open("");
    CHECK(handle >= 0);
    while (handle >= 0 && vfs_dir_next(handle, name, sizeof name, &is_dir, &size) > 0)
        if (!strcmp(name, "HELLO.TXT") && !is_dir && size == 5) found = 1;
    if (handle >= 0) vfs_dir_close(handle);
    CHECK(found);
    CHECK(!p4_host_remove(file) && !p4_os_rmdir(dir));
}

int main(int argc, char **argv)
{
    P4HostStat st;
    if (argc < 2 || strlen(argv[1]) > sizeof gAscii - 1) {
        fprintf(stderr, "usage: utf8_path_test <ascii scratch root>\n");
        return 2;
    }
    snprintf(gAscii, sizeof gAscii, "%s", argv[1]);
    snprintf(gRoot, sizeof gRoot, "%s%s", gAscii, ROOT_UTF8);
    p4_host_mkdir(gAscii);
    if (p4_host_mkdir(gRoot) && errno != EEXIST) { fprintf(stderr, "cannot create %s\n", gRoot); return 1; }
    CHECK(!p4_host_stat(gRoot, &st) && st.is_dir);

    checkHostLayer();
    checkSettingsFile();
    checkLooseFiles();
    CHECK(!p4_os_rmdir(gRoot));

    if (failures) {
        fprintf(stderr, "utf8_path_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("utf8_path_test: ok\n");
    return 0;
}
