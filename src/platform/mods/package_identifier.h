/* The package identifier rule: mod.ini id=, [section] names and handler names.
 * One definition for the game (package_registry.c) and p4mod (tools/p4mod.cpp),
 * so an id p4mod accepts for new/check/pack is exactly one the game loads. */
#ifndef P4PC_PACKAGE_IDENTIFIER_H
#define P4PC_PACKAGE_IDENTIFIER_H
#include <string.h>

/* Bytes in P4PackageInfo.id, its NUL included. */
#define P4_PACKAGE_ID_SIZE 64

/* 1..63 bytes of a-z, 0-9, '-', '_' or '.', neither starting nor ending with
 * '.': so never ".", ".." or a path component that climbs. */
static inline int p4_package_identifier(const char *s)
{
    size_t size = strlen(s);
    if (!size || size >= P4_PACKAGE_ID_SIZE) return 0;
    for (size_t i = 0; i < size; ++i)
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') ||
              s[i] == '-' || s[i] == '_' || s[i] == '.')) return 0;
    return s[0] != '.' && s[size - 1] != '.';
}
#endif
