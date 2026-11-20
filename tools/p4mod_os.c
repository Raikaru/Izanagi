/* The four include/p4pc/os.h calls the game's package file index and code-file
 * check (src/platform/mods/file_index.c, compendium.c) make, for p4mod. They
 * are the game's own bodies from src/platform/os/os_win32.c and os_posix.c;
 * the rest of that layer (guest RAM, coroutines, threads) has no place in a
 * host tool. Keep these in step with those files. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#define _FILE_OFFSET_BITS 64
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "p4pc/os.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
int p4_os_is_dir_nofollow(const char *path) {
    WCHAR wide[4096];
    DWORD a;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0]))
        return 0;
    a = GetFileAttributesW(wide);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) && !(a & FILE_ATTRIBUTE_REPARSE_POINT);
}
int p4_os_is_file_nofollow(const char *path) {
    WCHAR wide[4096];
    DWORD a;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0]))
        return 0;
    a = GetFileAttributesW(wide);
    return a != INVALID_FILE_ATTRIBUTES && !(a & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE));
}
int p4_os_list_dir(const char *path, void (*visit)(const char *name, void *user), void *user) {
    char pattern[4096];
    struct _finddata_t entry;
    const size_t length = strlen(path);
    const char *separator;
    intptr_t handle;
    int error;
    if (length == 0) {
        errno = ENOENT;
        return -1;
    }
    separator = path[length - 1] == '/' || path[length - 1] == '\\' ? "" : "\\";
    if (snprintf(pattern, sizeof pattern, "%s%s*", path, separator) >= (int)sizeof pattern) {
        errno = ENAMETOOLONG;
        return -1;
    }
    handle = _findfirst(pattern, &entry);
    if (handle == -1) return -1;
    do {
        visit(entry.name, user);
    } while (_findnext(handle, &entry) == 0);
    error = errno;
    _findclose(handle);
    if (error != ENOENT) {
        errno = error;
        return -1;
    }
    return 0;
}
FILE *p4_os_fopen_rb(const char *path) { return fopen(path, "rb"); }
#else
int p4_os_is_dir_nofollow(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
int p4_os_is_file_nofollow(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISREG(st.st_mode);
}
int p4_os_list_dir(const char *path, void (*visit)(const char *name, void *user), void *user) {
    DIR *directory = opendir(path);
    struct dirent *entry;
    if (directory == NULL) return -1;
    for (;;) {
        int error;
        errno = 0;
        entry = readdir(directory);
        if (entry != NULL) {
            visit(entry->d_name, user);
            continue;
        }
        error = errno;
        closedir(directory);
        if (error != 0) {
            errno = error;
            return -1;
        }
        return 0;
    }
}
/* No "fd:" paths here: p4mod hands the index real paths only. */
FILE *p4_os_fopen_rb(const char *path) { return fopen(path, "rb"); }
#endif
