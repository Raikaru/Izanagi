/* Native block loader unchanged, plus read-only host metadata recording the
 * ORIGINAL block size. Never infer rows from padding or a RAM-wide bound. */
#include "p4pc/guest.h"
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_table_extents.h"

s32 btlUnitLoadBlocksFromFile(u8 *loadTable, s32 synchronous)
{
    u8 *block;
    s32 blockIndex;
    u32 blockSize;
    u32 offset;
    u8 **destination;

    if (iGpffffb434 == 0) {
        func_00440b68(&iGpffffa0a0, D_005F6BE0, 0x4B1);
        iGpffffb434 = H_Cdvd_AcquireDirectory(*(s32 *)loadTable, 0);
    }
    if (synchronous != 0) {
        H_Cdvd_ReadSync(iGpffffb434);
        goto block_6;
    }
    if (H_Cdvd_IsFileLoaded(iGpffffb434) == 0) {
        return 0;
    }
block_6:
    block = (u8 *)((HCdvdRequest *)(u8 *)iGpffffb434)->data;
    p4_game_table_admission_packed(loadTable, (const char *)P4_G2H(*(u32 *)loadTable),
        block, ((HCdvdRequest *)(u8 *)iGpffffb434)->size, p4_game_tables_observe_packed);
    blockIndex = 0;
    goto loop_8_check;
loop_8_body:
    blockSize = *(u32 *)block;
    offset = iGpffffb430;
    *destination = (u8 *)P4_G2H(P4_H2G(D_007F17B0) + offset);
    iGpffffb430 +=
        (((blockSize >> 4) + ((blockSize & 0xF) != 0)) * 0x10);
    memcpy(*destination, block + 4, blockSize);
    p4_game_table_extent_note(P4_H2G(destination), P4_H2G(*destination), blockSize);
    offset = blockSize + 4;
    block +=
        (((offset >> 4) + ((offset & 0xF) != 0)) * 0x10);
    blockIndex += 1;
loop_8_check:
    destination = ((BtlUnitLoadBlock *)loadTable)[blockIndex].destination;
    if (destination != NULL) {
        goto loop_8_body;
    }
    H_Cdvd_Destroy(iGpffffb434);
    iGpffffb434 = 0;
    return 1;
}
