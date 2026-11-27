#ifndef P4PC_GOLDEN_ATLAS_H
#define P4PC_GOLDEN_ATLAS_H
#include "p4pc/golden_texture.h"
typedef struct P4GoldenSprite {
    uint32_t texture_index;
    int32_t x0,y0,x1,y1,offset_x,offset_y,width,height;
    uint8_t flip; /* SPR +0x18 bits 0/1; lookup returns physical source corners */
} P4GoldenSprite;
typedef struct P4GoldenAtlas {
    size_t texture_count, sprite_count;
    P4GoldenTexture *textures;
    P4GoldenSprite *sprites;
} P4GoldenAtlas;
/* CPU-owned decoded atlas; no pointers into caller bytes survive success. */
int p4_golden_atlas_open(const uint8_t *bytes, size_t size, P4GoldenAtlas *out);
void p4_golden_atlas_free(P4GoldenAtlas *atlas);
int p4_golden_atlas_sprite(const P4GoldenAtlas *atlas, int index, P4GoldenSprite *out);
int p4_golden_item_sprite(uint32_t flags);
/* special_state is explicitly supplied; this API does not alias platform flags. */
int p4_golden_arcana_sprite(unsigned arcana, int special_state);
/* Packed natural-source crop, applying flip bits, but not destination size or
 * position metadata. Caller owns width*height*4 output. Used by cache/resource
 * adapters, which retain the full sprite metadata separately. */
int p4_golden_atlas_crop(const P4GoldenAtlas *atlas, int index, uint8_t *rgba, size_t size);
#endif
