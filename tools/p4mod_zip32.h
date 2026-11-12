#ifndef P4MOD_ZIP32_H
#define P4MOD_ZIP32_H

#include <stdint.h>

/* ZIP64 sentinel values are deliberately refused. The total also stays below
 * UINT32_MAX, so both directory offsets and all local offsets fit ZIP32. */
typedef struct P4ModZip32 {
    uint64_t local_bytes;
    uint64_t directory_bytes;
    uint64_t entries;
} P4ModZip32;

static inline int p4mod_zip32_size_ok(uint64_t size)
{
    return size < UINT32_MAX;
}

/* On failure leave the layout unchanged. Names and both file sizes are bytes. */
static inline int p4mod_zip32_add(P4ModZip32 *zip, uint64_t name, uint64_t raw, uint64_t packed)
{
    if (zip->entries >= UINT16_MAX - 1u || name > UINT16_MAX ||
        !p4mod_zip32_size_ok(raw) || !p4mod_zip32_size_ok(packed) ||
        !p4mod_zip32_size_ok(zip->local_bytes) || !p4mod_zip32_size_ok(zip->directory_bytes))
        return 0;
    uint64_t local = zip->local_bytes + 30u + name + packed;
    uint64_t directory = zip->directory_bytes + 46u + name;
    if (!p4mod_zip32_size_ok(local) || !p4mod_zip32_size_ok(directory) ||
        !p4mod_zip32_size_ok(local + directory + 22u))
        return 0;
    zip->local_bytes = local;
    zip->directory_bytes = directory;
    ++zip->entries;
    return 1;
}

#endif
