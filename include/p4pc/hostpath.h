/* The one place a UTF-8 host path becomes something the C library can open.
 *
 * Every path the port builds is UTF-8: the settings folder from SDL_GetPrefPath, the ISO the player picked, the saves, mods, caches and
 * Golden folders under them, the log. The Windows C library's fopen/stat/remove/mkdir/getenv take the ANSI codepage, so a player whose
 * profile folder has a non-ASCII name (C:\Users\Zoë\AppData\...) silently read and wrote nothing: p4pc.ini was never found and every launch
 * ran on defaults. Port code therefore opens, removes, stats and creates through the p4_host_* functions below, and reads path-bearing
 * environment variables with p4_host_getenv; on Windows they convert to the wide CRT calls, elsewhere they are the plain POSIX calls.
 * Renames, directory listing and removal, atomic replace and locks are in include/p4pc/os.h (p4_os_rename, p4_os_list_dir, ...), which
 * is UTF-8 on Windows as well.
 *
 * Header-only on purpose: the converters and tests that open files link no OS shim, and this needs no <windows.h>.
 * tests/os_path_audit.cmake fails the build when port code calls the raw fopen/stat/remove/mkdir/opendir/getenv-of-a-path again. */
#ifndef P4PC_HOSTPATH_H
#define P4PC_HOSTPATH_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct { uint64_t size; int64_t ctime, mtime; int is_dir, is_file; } P4HostStat;

#ifdef _WIN32
#include <direct.h>
#include <wchar.h>

#define P4_HOST_WIDE_CAP 4096 /* UTF-16 units, including the terminator */

/* UTF-8 -> UTF-16, strictly (no overlong forms, surrogates or values past U+10FFFF). 0, or -1 with errno EINVAL / ENAMETOOLONG. */
static inline int p4_host_wide(const char *text, wchar_t *out, size_t capacity)
{
    const unsigned char *s = (const unsigned char *)text;
    size_t n = 0;
    if (!text || !out || !capacity) { errno = EINVAL; return -1; }
    while (*s) {
        uint32_t c = *s++;
        int extra = c < 0x80 ? 0 : c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc2 ? 1 : -1;
        if (extra < 0 || c > 0xf4) { errno = EINVAL; return -1; }
        if (extra) c &= 0x3fu >> extra;
        for (int i = 0; i < extra; i++, s++) {
            if ((*s & 0xc0) != 0x80) { errno = EINVAL; return -1; }
            c = c << 6 | (*s & 0x3f);
        }
        if ((extra == 2 && c < 0x800) || (extra == 3 && (c < 0x10000 || c > 0x10ffff)) || (c >= 0xd800 && c <= 0xdfff)) {
            errno = EINVAL; return -1;
        }
        if (n + (c >= 0x10000 ? 2 : 1) >= capacity) { errno = ENAMETOOLONG; return -1; }
        if (c >= 0x10000) { c -= 0x10000; out[n++] = (wchar_t)(0xd800 + (c >> 10)); out[n++] = (wchar_t)(0xdc00 + (c & 0x3ff)); }
        else out[n++] = (wchar_t)c;
    }
    out[n] = 0;
    return 0;
}

/* UTF-16 -> UTF-8. 0, or -1 with errno. */
static inline int p4_host_narrow(const wchar_t *text, char *out, size_t capacity)
{
    size_t n = 0;
    if (!text || !out || !capacity) { errno = EINVAL; return -1; }
    while (*text) {
        uint32_t c = (uint16_t)*text++;
        if (c >= 0xd800 && c <= 0xdbff && (uint16_t)*text >= 0xdc00 && (uint16_t)*text <= 0xdfff)
            c = 0x10000 + ((c - 0xd800) << 10) + ((uint16_t)*text++ - 0xdc00);
        else if (c >= 0xd800 && c <= 0xdfff) { errno = EINVAL; return -1; }
        size_t bytes = c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
        if (n + bytes >= capacity) { errno = ENAMETOOLONG; return -1; }
        if (bytes == 1) out[n++] = (char)c;
        else {
            for (size_t i = bytes; i-- > 1;) { out[n + i] = (char)(0x80 | (c & 0x3f)); c >>= 6; }
            out[n] = (char)(bytes == 2 ? 0xc0 | c : bytes == 3 ? 0xe0 | c : 0xf0 | c);
            n += bytes;
        }
    }
    out[n] = 0;
    return 0;
}

/* "r", "w" or "a" then any of "+bx", widened; anything else is refused rather than handed to the C library. */
static inline int p4_host_wide_mode(const char *mode, wchar_t out[8])
{
    size_t i = 0;
    if (!mode || (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a')) { errno = EINVAL; return -1; }
    for (; mode[i]; i++) {
        if (i >= 7 || (i && !strchr("+bx", mode[i]))) { errno = EINVAL; return -1; }
        out[i] = (wchar_t)mode[i];
    }
    out[i] = 0;
    return 0;
}

static inline FILE *p4_host_fopen(const char *path, const char *mode)
{
    wchar_t wide[P4_HOST_WIDE_CAP], wmode[8];
    return p4_host_wide_mode(mode, wmode) || p4_host_wide(path, wide, P4_HOST_WIDE_CAP) ? NULL : _wfopen(wide, wmode);
}
static inline FILE *p4_host_freopen(const char *path, const char *mode, FILE *stream)
{
    wchar_t wide[P4_HOST_WIDE_CAP], wmode[8];
    return !stream || p4_host_wide_mode(mode, wmode) || p4_host_wide(path, wide, P4_HOST_WIDE_CAP) ? NULL : _wfreopen(wide, wmode, stream);
}
static inline int p4_host_remove(const char *path)
{
    wchar_t wide[P4_HOST_WIDE_CAP];
    return p4_host_wide(path, wide, P4_HOST_WIDE_CAP) ? -1 : _wremove(wide);
}
static inline int p4_host_mkdir(const char *path)
{
    wchar_t wide[P4_HOST_WIDE_CAP];
    return p4_host_wide(path, wide, P4_HOST_WIDE_CAP) ? -1 : _wmkdir(wide);
}
/* Facts about a file, following links; 0, or -1 with errno (ENOENT when absent). ctime is the creation time on Windows. */
static inline int p4_host_stat(const char *path, P4HostStat *out)
{
    wchar_t wide[P4_HOST_WIDE_CAP];
    struct _stat64 st;
    if (!out) { errno = EINVAL; return -1; }
    if (p4_host_wide(path, wide, P4_HOST_WIDE_CAP) || _wstat64(wide, &st)) return -1;
    out->size = (uint64_t)st.st_size; out->ctime = (int64_t)st.st_ctime; out->mtime = (int64_t)st.st_mtime;
    out->is_dir = (st.st_mode & _S_IFMT) == _S_IFDIR; out->is_file = (st.st_mode & _S_IFMT) == _S_IFREG;
    return 0;
}
/* getenv for a value that may be a path, read from the wide environment (the ANSI one turns a non-ASCII user name into '?').
 * The string stays valid for the life of the process; a changed variable returns a new one. Slots are per translation unit
 * and the table is not locked: call it from one thread (startup and configuration reads, as getenv is used). */
static inline const char *p4_host_getenv(const char *name)
{
    static struct { char name[64]; char *value; } table[16];
    wchar_t wname[64], *wvalue;
    char narrow[3 * P4_HOST_WIDE_CAP];
    if (!name || p4_host_wide(name, wname, 64) || !(wvalue = _wgetenv(wname)) || !*wvalue || p4_host_narrow(wvalue, narrow, sizeof narrow)) return NULL;
    for (size_t i = 0; i < sizeof table / sizeof table[0]; i++) {
        if (table[i].value && strcmp(table[i].name, name)) continue;
        if (!table[i].value) { if (strlen(name) >= sizeof table[i].name) return NULL; strcpy(table[i].name, name); }
        if (!table[i].value || strcmp(table[i].value, narrow)) {
            char *copy = (char *)malloc(strlen(narrow) + 1);
            if (!copy) return NULL;
            strcpy(copy, narrow);
            table[i].value = copy; /* the previous value is left allocated: a caller may still hold it */
        }
        return table[i].value;
    }
    return NULL;
}
#else
static inline FILE *p4_host_fopen(const char *path, const char *mode)
{
    if (!path || !mode || (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') || strlen(mode) > 7) { errno = EINVAL; return NULL; }
    for (size_t i = 1; mode[i]; i++) if (!strchr("+bx", mode[i])) { errno = EINVAL; return NULL; }
    return fopen(path, mode);
}
static inline FILE *p4_host_freopen(const char *path, const char *mode, FILE *stream)
{
    if (!path || !mode || !stream || (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') || strlen(mode) > 7) { errno = EINVAL; return NULL; }
    for (size_t i = 1; mode[i]; i++) if (!strchr("+bx", mode[i])) { errno = EINVAL; return NULL; }
    return freopen(path, mode, stream);
}
static inline int p4_host_remove(const char *path) { if (!path) { errno = EINVAL; return -1; } return remove(path); }
static inline int p4_host_mkdir(const char *path) { if (!path) { errno = EINVAL; return -1; } return mkdir(path, 0777); }
static inline int p4_host_stat(const char *path, P4HostStat *out)
{
    struct stat st;
    if (!path || !out) { errno = EINVAL; return -1; }
    if (stat(path, &st)) return -1;
    out->size = (uint64_t)st.st_size; out->ctime = (int64_t)st.st_ctime; out->mtime = (int64_t)st.st_mtime;
    out->is_dir = S_ISDIR(st.st_mode) != 0; out->is_file = S_ISREG(st.st_mode) != 0;
    return 0;
}
static inline const char *p4_host_getenv(const char *name) { return name ? getenv(name) : NULL; }
#endif

#endif
