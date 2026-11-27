#include "p4pc/game_mode.h"
#include "p4pc/golden_recipe.h"
extern unsigned p4_golden_fusion_gate_count(void);
extern int p4_golden_fusion_gate_flag(unsigned);
extern int p4_golden_fusion_gate_persona(unsigned);
extern int p4_golden_fusion_gate_bits(unsigned);
extern void p4_game_table_admission_golden_fail(const char *, int);

void fclCombineMaterialListBuild(s8 materialCount)
{
    u8 *items;
    s16 row;
    s16 item;
    s16 selected;
    s32 column;
    u8 *entry;
    u8 *table;
    s16 *value;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        int count = p4_golden_recipes_count();
        int selectedGolden = 0;
        if (count < 0) { p4_game_table_admission_golden_fail("fusion recipes unavailable", count); return; }
        for (int r = 0; r < count; ++r) {
            int id, allowed = 1;
            if (p4_golden_recipe_material_count((unsigned)r) != materialCount) continue;
            id = p4_golden_recipe_result((unsigned)r);
            if (!fclCombineMaterialAvailable((u16)id)) continue;
            for (unsigned g = 0; g < p4_golden_fusion_gate_count(); ++g) {
                if (p4_golden_fusion_gate_persona(g) == id) {
                    int bits = p4_golden_fusion_gate_bits(g);
                    if (bits & 1) { if (!datGetFlag(p4_golden_fusion_gate_flag(g))) { allowed = 0; break; } }
                    else if (bits & 2) { allowed = 0; if (datGetFlag(p4_golden_fusion_gate_flag(g)) == 1) { allowed = 1; break; } }
                }
            }
            if (!allowed) continue;
            yListSlotAppend(0, id, 0);
            yListSlotWork(0)[0x14 + selectedGolden] = 1;
            for (int m = 0; m < materialCount; ++m) {
                int material = p4_golden_recipe_material((unsigned)r, (unsigned)m);
                yListSlotAppend((s8)(selectedGolden + 1), material, 1);
                yListSlotWork((s8)(selectedGolden + 1))[0x14 + m] =
                    datPersonaFindRosterRecordById((u16)material) != NULL;
            }
            ++selectedGolden;
        }
        return;
    }
    selected = 0;
    row = 0;
    table = (u8 *)D_00642F00 + materialCount * 8;
    for (; row < 6; row++) {
        entry = ((FclMaterialTableEntry *)table)[-2].recipes + row * 0x20;
        if (((FclMaterialRecipe *)entry)->result != 0 && fclCombineMaterialAvailable(((FclMaterialRecipe *)entry)->result) == 1) {
            yListSlotAppend(0, (s16)((FclMaterialRecipe *)entry)->result, 0);
            items = entry + 4;
            yListSlotWork(0)[0x14 + selected] = 1;
            item = 0;
            column = selected + 1;
            for (; item < materialCount; item++) {
                value = (s16 *)(items + item * 2 + 4);
                yListSlotAppend((s8)column, *value, 1);
                yListSlotWork((s8)column)[0x14 + item] = 0;
                if (datPersonaFindRosterRecordById(*(u16 *)value) != NULL) {
                    yListSlotWork((s8)column)[0x14 + item] = 1;
                }
            }
            selected++;
        }
    }
}
