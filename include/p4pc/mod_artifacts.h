#ifndef P4PC_MOD_ARTIFACTS_H
#define P4PC_MOD_ARTIFACTS_H
/* Mod-owned artifact files (API 1.9, docs/modding/lua.md "Artifacts"):
 * mod-artifacts/<mod id>/<path> beside p4pc.ini, written by temp file plus
 * atomic replace. Paths are 1..8 '/'-separated segments of [A-Za-z0-9_.-]
 * (no leading dot, no trailing dot, no Windows device names, no segment that
 * differs only by case from an existing entry), at most 128 bytes. Quotas are
 * checked before writing: 4 MiB per file, 256 files and 64 MiB per mod. No
 * link or reparse point is created or followed. Without a settings file
 * (P4PC_CONFIG=none) writes are refused and reads find nothing. */
#include <stddef.h>
#include <stdint.h>

#define P4_MOD_ARTIFACT_PATH_MAX 128u
#define P4_MOD_ARTIFACT_SEGMENTS 8u
#define P4_MOD_ARTIFACT_FILE_MAX (4u * 1024u * 1024u)
#define P4_MOD_ARTIFACT_FILES_MAX 256u
#define P4_MOD_ARTIFACT_TOTAL_MAX (64u * 1024u * 1024u)

/* The settings directory; NULL disables persistence. 0 or -1. */
int p4_mod_artifact_root(const char *directory);
int p4_mod_artifact_path_valid(const char *path);
/* 0, or -1 with a player-readable reason and nothing changed. */
int p4_mod_artifact_write(const char *mod_id, const char *path, const void *bytes, size_t size,
                          char *reason, size_t reason_capacity);
/* The file's size with its bytes copied (only the size when out is NULL),
 * -1 when it is missing, -2 (with reason) when it cannot be read or is larger
 * than capacity. */
int64_t p4_mod_artifact_read(const char *mod_id, const char *path, void *out, size_t capacity,
                             char *reason, size_t reason_capacity);
/* The mod's artifact folder for display: 0, or -1 without a settings directory. */
int p4_mod_artifact_folder(const char *mod_id, char *out, size_t capacity);
#endif
