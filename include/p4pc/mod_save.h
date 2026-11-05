/* Native mod-save blocks. Retail image bytes and layout are never extended. */
#ifndef P4PC_MOD_SAVE_H
#define P4PC_MOD_SAVE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define P4_MOD_SAVE_ABI_VERSION 1u
#define P4_MOD_SAVE_REQUIRED 1u

typedef struct P4ModSaveWriter P4ModSaveWriter;
typedef struct P4ModSaveReader P4ModSaveReader;
typedef struct P4ModSaveLoad P4ModSaveLoad;
typedef struct P4ModSaveRegistration P4ModSaveRegistration;
struct P4PackageProvider;
/* Implemented by actual source preparation, not by this save codec. The context
 * pins/attests the exact frozen view and explicit package/native-mode origin.
 * NULL writer means load validation only; 0=authorized, negative=failure.
 * No positive omit status is defined here; unknown origins fail, never fallback. */
typedef int (*P4ModSaveRequireOwnerFn)(const void *frozen_view,
    P4ModSaveWriter *nullable_writer, const char *owner,
    void *immutable_prepared_context);

typedef struct P4ModSaveImage {
    const unsigned char *data; /* NULL/size0 only for explicit new-game reset. */
    size_t size;
    const char *slot; /* Diagnostic only; NULL is valid during capture. */
} P4ModSaveImage;

typedef struct P4ModSaveContext {
    P4ModSaveImage image;
    const P4ModSaveLoad *load; /* Cross-provider access only during validation. */
    char *error;
    size_t error_capacity;
} P4ModSaveContext;

typedef struct P4ModSaveProvider {
    size_t struct_size;
    uint32_t abi_version;
    const char *owner, *key; /* Copied stable namespace/block identity. */
    uint32_t schema_version, flags;
    uint64_t max_bytes;
    void *user;
    /* 0=emitted, 1=omit before writing, otherwise fail the entire capture. */
    int (*encode)(const P4ModSaveContext *, P4ModSaveWriter *, void *user);
    /* Absent block: schema0/readerNULL; stage empty/default state explicitly.
     * Partial staged ownership is discarded even when decode returns failure. */
    int (*decode)(const P4ModSaveContext *, uint32_t stored_schema,
                  P4ModSaveReader *nullable, void **staged, void *user);
    /* Optional cross-provider check after EVERY decode, before any commit. */
    int (*validate)(const P4ModSaveContext *, void *staged, void *user);
    /* Infallible/allocation-free/non-yielding; consumes staged ownership. */
    void (*commit)(void *staged, void *user);
    /* Tolerates NULL. Never called on an already committed result. */
    void (*discard)(void *staged, void *user);
    /* Optional capture precondition. Every registered provider's authorize runs
     * before ANY provider encodes; nonzero refuses the whole save before a byte
     * of the image/sidecar is written. Context image is the borrowed capture. */
    int (*authorize)(const P4ModSaveContext *, void *user);
} P4ModSaveProvider;

/* Game thread only. Registration copies metadata, not callback/user ownership.
 * Both operations refuse active transactions. Successful unregister is required
 * before releasing callbacks/user; it never proves native queue quiescence. */
int p4_mod_save_register(const P4ModSaveProvider *, P4ModSaveRegistration **out);
int p4_mod_save_unregister(P4ModSaveRegistration *);
int p4_mod_save_write(P4ModSaveWriter *, const void *bytes, size_t size);
int p4_mod_save_read(P4ModSaveReader *, void *bytes, size_t size);
uint64_t p4_mod_save_remaining(const P4ModSaveReader *);
/* Canonical little-endian integer codecs; no native struct blobs. */
int p4_mod_save_write_u32(P4ModSaveWriter *, uint32_t);
int p4_mod_save_write_u64(P4ModSaveWriter *, uint64_t);
int p4_mod_save_read_u32(P4ModSaveReader *, uint32_t *);
int p4_mod_save_read_u64(P4ModSaveReader *, uint64_t *);
/* Validate phase only: 0=registered decoded provider (possibly NULL staged),
 * 1=absent provider, negative=bad phase/arguments. Borrowed, never active state. */
int p4_mod_save_staged(const P4ModSaveContext *, const char *owner,
                       const char *key, const void **out);
/* Real-load decode/validate only, never encode/New Game. Copies a nonempty
 * message (at most 4096 bytes; all warnings bounded to 8MiB) into the native
 * precommit warning/acknowledgement path; 0/-1. Failure poisons that load even
 * if ignored. This cannot bypass required refs or set witness custody.
 * Registration/omitted encode alone never creates save metadata. */
int p4_mod_save_warn(const P4ModSaveContext *, const char *message);
/* Records the actual selected package kind/id/version. NULL version resolves
 * its current immutable catalog version; missing/ambiguous owners fail capture.
 * An explicit version must match the selected provider, never a schema number.
 * Historical provenance and mandatory current requirements remain separate. */
int p4_mod_save_require_mod(P4ModSaveWriter *, const char *id,
                           const char *version, int required);
/* Strong source-preparation path: records the actual current immutable provider,
 * preserving kind/id/version even when two selector kinds share an id. */
int p4_mod_save_require_provider(P4ModSaveWriter *,
    const struct P4PackageProvider *, int required);
/* Actual semantic use, not installation. Native/Lua owners pass their real
 * current provider. This process-use set survives load/New Game: cached mod
 * code/data/products remain live. Loaded old-save history is separate and
 * resets on New Game. Changing the active package set requires restart. */
int p4_mod_save_touch(const struct P4PackageProvider *);

#ifdef __cplusplus
}
#endif
#endif
