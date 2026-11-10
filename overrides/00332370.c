/* Keep the actual message/window owner and retail category copy recipe.
 * Expanded call sites supply logical dependencies, never repacked row bits. */
#include "p4pc/game_items_native.h"
static void p4_material_show_00332370(u8 *task, s32 itemId, s16 materialId, s32 empty);
void wshopMaterialTalkShowRequirement(u8 *task, s32 itemId, s32 materialInfo)
{
    p4_material_show_00332370(task, itemId, (s16)((materialInfo & 0xFFF00) >> 8), materialInfo == 0);
}
static void p4_material_show_00332370(u8 *task, s32 itemId, s16 materialId, s32 empty)
{
    u8 categoryName[0x20];
    s32 category;
    u8 *work = ((KwlnTask *)task)->work;
    if (empty) {
        *(work + 1) = nmMesWindowAcquire(D_00645240);
        nmMesWindowSetSlotString(*(work + 1), 0, datGetItemNameId(materialId));
        nmMesWindowShowSlotRecord(*(work + 1), 0x1E);
        return;
    }
    *(work + 1) = nmMesWindowAcquire(D_00645240);
    nmMesWindowSetSlotString(*(work + 1), 0, datGetItemNameId(materialId));
    category = datEntryEvaluateFlags((s16)itemId);
    u32 registeredName = p4_game_tables_items_native_category_name((u32)itemId);
    if (registeredName) {
        nmMesWindowSetSlotString(*(work + 1), 1, P4_G2H(registeredName));
    } else if (category == 0xE) {
        frFontExCopyString((void *)D_0064A07C[0], categoryName, 0x20);
        nmMesWindowSetSlotString(*(work + 1), 1, categoryName);
    } else if (category == 0xF) {
        frFontExCopyString((void *)D_0064A080[0], categoryName, 0x20);
        nmMesWindowSetSlotString(*(work + 1), 1, categoryName);
    } else {
        frFontExCopyString((void *)D_0064A078[0], categoryName, 0x20);
        nmMesWindowSetSlotString(*(work + 1), 1, categoryName);
    }
    nmMesWindowShowSlotRecord(*(work + 1), 0x1F);
}
static void p4_material_gate_00332370(u8 *task, s32 itemId, u32 gate, u32 packed)
{
    p4_material_show_00332370(task, itemId,
        (s16)p4_game_tables_items_native_requirement_item((u32)itemId, gate, packed),
        p4_game_tables_items_native_requirement_empty((u32)itemId, gate, packed));
}
