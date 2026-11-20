#ifndef P4PC_CAPABILITIES_H
#define P4PC_CAPABILITIES_H
#include <stddef.h>
/* The API capability registry (CONTRIBUTING.md "API capabilities"). One declaration
 * fragment per key, include/p4pc/p4.d/<key>.luau, names a key and its revision; the build
 * generates this table from those files, so it lists exactly what this build provides. */
#define P4_CAPABILITY_KEY_MAX 96u
/* Revision this build provides, 0 when the key is absent. */
unsigned p4_capability_revision(const char *key);
size_t p4_capability_count(void);
/* Sorted by key; NULL past the end. */
const char *p4_capability_at(size_t index, unsigned *revision);
/* `key` or `key@revision` (default revision 1) into key[P4_CAPABILITY_KEY_MAX]: 0, or -1. */
int p4_capability_parse(const char *text, size_t size, char *key, unsigned *revision);
/* A mod.ini `requires=` list: comma-separated keys. 0 = well formed. */
int p4_capability_requires_valid(const char *list);
/* 0 when this build provides every key at its revision; otherwise -1 and a player-readable
 * reason naming the first missing key. */
int p4_capability_requires_check(const char *list, char *reason, size_t capacity);
#endif
