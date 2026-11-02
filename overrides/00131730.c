/* Native equipment rows append after the whole Camp work, not inside its
 * fixed submenu. Preserve retail eligibility and equipped-count ordering. */
#include "p4pc/game_items_native.h"
void cmpEquipBuildItemList(s16 *workArea)
{
    s16 i;
    s16 count = 0;
    s16 id;
    s32 type;
    s64 equipmentType;
    s32 found;
    s32 filter;
    s16 value;
    u32 total = p4_game_tables_items_native_gear_scan_count();
    P4GameItemEquipmentRow *rows = p4_game_tables_items_native_equipment_rows(workArea);
    id = workArea[workArea[20] + 28];
    type = ((CmpEquipWork *)workArea)->selection[1];
    equipmentType = type;
    for (u32 ordinal = 0; ordinal < total; ++ordinal) {
        i = (s16)p4_game_tables_items_native_gear_scan_id(ordinal);
        if (i < 0) continue;
        found = i == datGetEquippedItemByUnit(id, equipmentType);
        if (!found) {
            filter = (u8)datItemCount(i) != 0;
            if (filter) filter = datEntryHasFlag(i, id) != 0;
            if (filter) filter = (s32)type == datIdBand(i);
            if (!filter) continue;
        }
        value = found + (u8)datItemCount(i);
        if (value > 99) value = 99;
        rows[count].id = i;
        rows[count].quantity = value;
        ++count;
    }
    ((CmpEquipWork *)workArea)->itemCount = count;
    if ((u32)count > total) kAssertFail(D_005E9FA0, 0x299);
}
