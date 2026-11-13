#include "p4pc/native_rows.h"

/* 003190d0: fclCombineRowsBusy, retail; rows are read through p4_native_rows_velvet_row so the
 * tenth (side) row of a list with the mod card counts too. */
s32 fclCombineRowsBusy(u8 *task) {
    u8 *work;
    s16 i;

    work = ((struct KwlnTask *)task)->work;
    for (i = 0; i < ((FclCombineWork *)work)->commandCount; i++) {
        if (((FclCombineWork *)work)->commandCursor != i) {
            if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(p4_native_rows_velvet_frame(((FclCombineRow *)p4_native_rows_velvet_row(work, i))->id)))->flags, 1) == 1) {
                return 1;
            }
        }
    }
    return (s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1DC))->flags, 1) == 1;
}
