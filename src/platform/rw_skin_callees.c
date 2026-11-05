/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* RenderWare skin helpers at retail 0x003b7510 (SkinFindMaxWeights), 0x003b7590
 * (SkinFindNumUsedBones) and 0x003b7680 (func_003b7680).
 *
 * The decomp declares these in src/rw/rpskin.c but never defines them: the retail code
 * is asm with no FUN_ marker, so an overrides/ file has nothing to splice into. These
 * port-owned bodies follow the retail asm. Field offsets are byte offsets into the skin
 * object, as the asm uses them: +0 row count, +4 lane count, +8 lane storage, +12 aligned
 * row storage, +16 largest used-lane count, +20 and +24 the per-entry arrays, +68 the raw
 * allocation. The allocator is the retail table entry at 0x008873E8 (D_008873E8[0]).
 */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"

/* The allocator slot 0x008873E8 (D_008873E8[0]): a guest code address. */
typedef uint8_t *(*P4SkinAllocFn)(int32_t arg0, int32_t arg1);

#define SKIN_U32(obj, off) (*(uint32_t *)((obj) + (off)))
/* A pointer field of a skin record is a guest address: read it as a host pointer. */
#define SKIN_PTR(obj, off) ((uint8_t *)P4_G2H(SKIN_U32(obj, off)))

/* 0x003b7510: obj+16 becomes the largest number of used weight lanes (1..4) across the
 * `count` rows of 16 bytes each. */
void SkinFindMaxWeights(uint8_t *obj, uint32_t *rows, uint32_t count)
{
    uint32_t i;

    SKIN_U32(obj, 16) = 1;
    if (count == 0) {
        return;
    }
    for (i = 0; i < count; i++) {
        uint32_t *row = rows + 4 * i;

        while (SKIN_U32(obj, 16) < 4 && row[SKIN_U32(obj, 16)] != 0) {
            SKIN_U32(obj, 16)++;
            if (SKIN_U32(obj, 16) == 4) {
                return;
            }
        }
    }
}

/* 0x003b7590: *cnt gets the distinct bone indices in use. For each of the `n` entries, the
 * first obj+16 lanes whose weight word is nonzero contribute one byte from lanes[i]. */
void SkinFindNumUsedBones(uint8_t *obj, uint32_t *lanes, uint32_t *rec, uint8_t *list,
                          uint32_t *cnt, uint32_t n)
{
    uint32_t i;

    *cnt = 0;
    for (i = 0; i < n; i++) {
        uint32_t rowcnt = SKIN_U32(obj, 16);
        uint32_t j;

        for (j = 0; j < rowcnt; j++) {
            uint32_t lane;
            uint32_t k;
            int found = 0;

            if (rec[4 * i + j] == 0) {
                continue;
            }
            lane = (lanes[i] >> (8 * j)) & 0xff;
            for (k = 0; k < *cnt; k++) {
                if (list[k] == lane) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                list[*cnt] = (uint8_t)lane;
                (*cnt)++;
            }
        }
    }
}

/* 0x003b7680: allocates the skin object's storage (cnt + 20*n + 64*rows + 15 bytes) and
 * fills its fields. Copies, in order: the lane bytes into +8, the 64-byte row records into
 * +12 (from the last row down), src20 (4*n bytes) into +20 and src24 (16*n bytes) into +24.
 * Returns 1 on success, 0 when the allocation fails. */
int32_t func_003b7680(uint8_t *obj, int32_t rows, uint32_t cnt, uint32_t n, uint8_t *lanes,
                      uint8_t *src24, uint8_t *src20, uint8_t *srcRows)
{
    uint32_t size = cnt + 20 * n + 64 * (uint32_t)rows + 15;
    uint8_t *p = ((P4SkinAllocFn)p4_fn_g2h_sig(P4_G32(0x008873E8u), P4_SIG(2, 1, 0, 0, 0, 0, 0, 0)))((int32_t)size, 0x30116);
    uint32_t pg = P4_H2G(p);

    SKIN_U32(obj, 68) = pg;
    if (p == NULL) {
        return 0;
    }
    memset(p, 0, size);
    SKIN_U32(obj, 0) = (uint32_t)rows;
    SKIN_U32(obj, 4) = cnt;
    SKIN_U32(obj, 8) = pg;
    SKIN_U32(obj, 12) = (pg + cnt + 15) & ~15u;
    SKIN_U32(obj, 20) = SKIN_U32(obj, 12) + 64 * (uint32_t)rows;
    SKIN_U32(obj, 24) = SKIN_U32(obj, 20) + 4 * n;

    if (lanes != NULL && cnt != 0) {
        memcpy(SKIN_PTR(obj, 8), lanes, cnt);
    }
    if (srcRows != NULL && rows != 0) {
        uint32_t k;

        for (k = (uint32_t)rows; k-- > 0;) {
            memcpy(SKIN_PTR(obj, 12) + 64 * k, srcRows + 64 * k, 64);
        }
    }
    if (src20 != NULL) {
        memcpy(SKIN_PTR(obj, 20), src20, 4 * n);
    }
    if (src24 != NULL) {
        memcpy(SKIN_PTR(obj, 24), src24, 16 * n);
    }
    return 1;
}

/* Retail 0x003b85e0 (skinskycommon.c, an excluded unit, so port-owned): prepares a skinned geometry for the
 * sky skin pipeline. For a geometry without the native flag (+8 & 0x1000000) it rewrites the skin's per-vertex
 * weights: the low ten bits of each float weight of the (up to four) weights of a vertex become the VU1
 * matrix-cache offset of that weight's bone, (bone + 1) * 4, where the bone is the corresponding byte of the
 * vertex's bone index word, mapped through the skin's remap table (skin + 0x38) when skin + 0x30 is set. A weight
 * of zero is cleared whole instead; weight 0 always takes an offset. The skin's counters at +0x1C .. +0x28 count
 * the vertices by the number of weights after the first (0 to 3). Returns the geometry.
 *
 * skin = func_003b83f0(geometry) (RpSkinGeometryGetSkin): +0x14 the vertex bone index words, +0x18 the vertex
 * weights (four floats per vertex); the geometry's vertex count is at +0x14. CPU skinning reads the weights
 * without those ten bits. */
extern uint32_t func_003b83f0(uint32_t geometry);   /* decomp: the skin's guest address for the geometry's */

static uint32_t skinOffset(const uint8_t *skin, uint32_t bone)
{
    if (SKIN_U32(skin, 0x30) != 0) {
        bone = *(SKIN_PTR(skin, 0x38) + bone);
    }
    return (bone + 1u) * 4u;
}

uint8_t *func_003b85e0(uint8_t *geometry)
{
    uint8_t *skin;
    uint32_t *idx;
    uint32_t *wt;
    uint32_t count;
    uint32_t i;

    if ((SKIN_U32(geometry, 8) & 0x1000000u) != 0) {
        return geometry;
    }
    skin = (uint8_t *)P4_G2H(func_003b83f0(P4_H2G(geometry)));
    idx = (uint32_t *)SKIN_PTR(skin, 0x14);
    wt = (uint32_t *)SKIN_PTR(skin, 0x18);
    count = SKIN_U32(geometry, 0x14);
    SKIN_U32(skin, 0x1C) = 0;
    SKIN_U32(skin, 0x20) = 0;
    SKIN_U32(skin, 0x24) = 0;
    SKIN_U32(skin, 0x28) = 0;
    for (i = 0; i < count; i++, wt += 4, idx++) {
        uint32_t extra = 0;
        uint32_t k;

        wt[0] = (wt[0] & 0xFFFFFC00u) | skinOffset(skin, *idx & 0xFFu);
        for (k = 1; k < 4; k++) {
            float w;

            memcpy(&w, &wt[k], sizeof w);
            if (w != 0.0f) {
                wt[k] = (wt[k] & 0xFFFFFC00u) | skinOffset(skin, (*idx >> (8u * k)) & 0xFFu);
                extra++;
            } else {
                wt[k] = 0;
            }
        }
        SKIN_U32(skin, 0x1C + 4u * extra) += 1;
    }
    return geometry;
}

/* Retail 0x003b88c0 (skinskycommon.c): `jr ra` with a0 left as the return value, called with no arguments and its
 * result ignored (rpskin.c func_003b8410, when a skin's hierarchy is replaced). Nothing to undo in the port. */
uint64_t func_003b88c0(void)
{
    return 0;
}
