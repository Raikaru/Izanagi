/* Native paletted texture decode (src/render/p4gl_nativetex.c) on the two textures of FIELD/GRMD/DOBJ044.RMD
 * on the retail disc named by P4PC_ISO: a PAL8 256 x 256 and a PAL4 128 x 128 cloud sheet. Exits 77 (CTest
 * skip) when P4PC_ISO is unset. The expected CRC-32s are those of the decoded index and palette bytes; the
 * decode was checked by looking at the decoded images (clouds) before the values were taken. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl_nativetex.h"
#include "vfs.h"

#define SKIP_RETURN 77

static uint32_t crc32(const uint8_t *p, size_t n)
{
    uint32_t c = 0xFFFFFFFFu;

    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) {
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
    }
    return ~c;
}

static uint32_t rd32(const uint8_t *p)
{
    uint32_t v;

    memcpy(&v, p, sizeof v);
    return v;
}

/* RW chunk: type, size, version (12 bytes), then size bytes. */
typedef struct Chunk {
    uint32_t type, size;
    const uint8_t *body;
} Chunk;

static int chunkAt(const uint8_t *file, size_t fileSize, size_t off, Chunk *c)
{
    if (off + 12 > fileSize) {
        return 0;
    }
    c->type = rd32(file + off);
    c->size = rd32(file + off + 4);
    c->body = file + off + 12;
    return off + 12 + c->size <= fileSize;
}

static int failures;

static void check(const char *what, uint32_t got, uint32_t want)
{
    if (got != want) {
        fprintf(stderr, "native_tex_test: FAIL %s: got %08x want %08x\n", what, (unsigned)got, (unsigned)want);
        failures++;
    }
}

int main(void)
{
    const char *iso = getenv("P4PC_ISO");
    static const uint32_t wantIdx[2] = {0x598A3B01u, 0x5CF41033u};
    static const uint32_t wantPal[2] = {0xA6CD2228u, 0x688036F0u};
    long long size = 0;
    int isDir = 0;
    uint8_t *file;
    int fd;
    int found = 0;

    if (iso == NULL || iso[0] == '\0') {
        fprintf(stderr, "native_tex_test: P4PC_ISO unset, skipping\n");
        return SKIP_RETURN;
    }
    if (vfs_mount(iso, NULL) != 0 || vfs_stat("/FIELD/GRMD/DOBJ044.RMD", &isDir, &size) != 0 || size <= 0) {
        fprintf(stderr, "native_tex_test: cannot find DOBJ044.RMD\n");
        return 1;
    }
    file = malloc((size_t)size);
    fd = vfs_open("/FIELD/GRMD/DOBJ044.RMD", 0);
    if (file == NULL || fd < 0 || vfs_read(fd, file, (int)size) != (int)size) {
        fprintf(stderr, "native_tex_test: cannot read DOBJ044.RMD\n");
        return 1;
    }

    /* The file: a 14-byte header, then the texture dictionary (0x16) whose struct is followed by one
     * native texture chunk (0x15) per texture. A native texture: struct, name, mask, then the outer
     * type-1 chunk that holds the 64-byte header chunk and the data chunk. */
    Chunk dict;

    if (!chunkAt(file, (size_t)size, 14, &dict) || dict.type != 0x16) {
        fprintf(stderr, "native_tex_test: no texture dictionary\n");
        return 1;
    }
    for (size_t off = 0; off + 12 <= dict.size && found < 2;) {
        Chunk tex;

        if (!chunkAt(dict.body, dict.size, off, &tex)) {
            break;
        }
        off += 12 + tex.size;
        if (tex.type != 0x15) {
            continue;
        }
        size_t o = 0;
        Chunk c;
        Chunk outer = {0, 0, NULL};
        int idx = 0;

        while (chunkAt(tex.body, tex.size, o, &c)) {
            o += 12 + c.size;
            if (c.type == 1 && ++idx == 2) {
                outer = c; /* the second type-1 chunk: the first is the platform id struct */
                break;
            }
        }
        Chunk hdr, data;

        if (outer.body == NULL || !chunkAt(outer.body, outer.size, 0, &hdr) || hdr.size < 16 ||
            !chunkAt(outer.body, outer.size, 12 + hdr.size, &data)) {
            fprintf(stderr, "native_tex_test: texture %d is not a native texture\n", found);
            failures++;
            found++;
            continue;
        }
        const int32_t w = (int32_t)rd32(hdr.body);
        const int32_t h = (int32_t)rd32(hdr.body + 4);
        const int32_t depth = (int32_t)rd32(hdr.body + 8);
        uint8_t *indices = malloc((size_t)w * (size_t)h);
        uint8_t palette[256 * 4];

        memset(palette, 0, sizeof palette);
        if (indices == NULL || !p4gl_nativetex_decode(data.body, data.size, w, h, depth, indices, palette)) {
            fprintf(stderr, "native_tex_test: FAIL decode of texture %d (%d x %d x %d)\n", found, (int)w, (int)h,
                    (int)depth);
            failures++;
        } else {
            check("indices", crc32(indices, (size_t)w * (size_t)h), wantIdx[found]);
            check("palette", crc32(palette, depth == 8 ? 1024u : 64u), wantPal[found]);
        }
        free(indices);
        found++;
    }
    if (found != 2) {
        fprintf(stderr, "native_tex_test: found %d textures, want 2\n", found);
        failures++;
    }
    free(file);
    if (failures == 0) {
        printf("native_tex_test: ok\n");
    }
    return failures != 0;
}
