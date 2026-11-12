#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* 0035e820..0035e8ac destroys Config's eleven sprites and work block.
 * Release the port's three stock scrollbar nodes before the original cleanup. */
void cmpConfigDestroy(u8 *task)
{
    u8 *base;
    s32 spriteIndex;
    s32 *slot;

    base = ((KwlnTask *)task)->work;
    p4_settings_native_end((u32)base);
    spriteIndex = 0;
    while (spriteIndex < 11) {
        slot = &((DatCommunityOverlayWork *)base)->sprites[spriteIndex];
        if (*slot != 0) {
            sdkSpriteFreeNode((void *)*slot);
            *slot = 0;
        }
        spriteIndex++;
    }
    ((DatCommunityOverlayWork *)base)->unk24 = 0;
    (*D_008873EC)(base);
}
