#include "p4pc/game_mode.h"
#include "p4pc/golden_recipe.h"
extern unsigned p4_golden_fusion_gate_count(void);
extern int p4_golden_fusion_gate_flag(unsigned);
extern int p4_golden_fusion_gate_persona(unsigned);
extern int p4_golden_fusion_gate_bits(unsigned);
extern void p4_game_table_admission_golden_fail(const char *, int);

s32 fclFuseRecipeLookup(u16 *output, s32 recipeKind, u16 *ingredients, s32 ingredientCount)
{
    s32 count;
    s16 matched;
    s32 row;
    s32 threshold;
    u16 *entry;
    s32 key;
    s32 allowed;
    s32 result;
    s32 gate;
    u32 flagIndex;
    s32 skillIndex;
    u8 *flag;
    s16 bits;
    u16 *skill;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        int n = p4_golden_recipes_count();
        int gates;
        int wanted = recipeKind == 5 ? 12 : recipeKind + 2;
        if (!output || !ingredients || recipeKind < 0 || recipeKind > 5 || ingredientCount != wanted) return 0;
        if (n < 0) { p4_game_table_admission_golden_fail("fusion recipes unavailable", n); return 0; }
        gates = (int)p4_golden_fusion_gate_count();
        int preferred = recipeKind == 5 ? p4_golden_recipe_twelfth() : -1;
        for (int scan = 0; scan < n; ++scan) {
            int r = preferred < 0 ? scan : scan == 0 ? preferred : scan <= preferred ? scan - 1 : scan;
            unsigned mask = 0;
            int i;
            if (p4_golden_recipe_material_count((unsigned)r) != wanted) continue;
            for (i = 0; i < wanted; ++i) {
                int material = p4_golden_recipe_material((unsigned)r, (unsigned)i);
                int j;
                for (j = 0; j < ingredientCount; ++j) {
                    if (!(mask & (1u << j)) && ingredients[j] == material) {
                        mask |= 1u << j;
                        break;
                    }
                }
                if (j == ingredientCount) break;
            }
            if (i != wanted) continue;
            key = p4_golden_recipe_result((unsigned)r);
            allowed = 1;
            for (int g = 0; g < gates; ++g) {
                if (p4_golden_fusion_gate_persona(g) == key) {
                    int gbits = p4_golden_fusion_gate_bits(g);
                    if (gbits & 1) {
                        if (!datGetFlag(p4_golden_fusion_gate_flag(g))) { allowed = 0; break; }
                    } else if (gbits & 2) {
                        allowed = 0;
                        if (datGetFlag(p4_golden_fusion_gate_flag(g)) == 1) { allowed = 1; break; }
                    }
                }
            }
            if (allowed) {
                datPersonaInitRecord((u8 *)output, (u16)key);
                return iGpffffb3d4[output[1] * 14 + 2];
            }
        }
        return 0;
    }
    if (output == NULL || ingredients == NULL || recipeKind >= 5)
        kAssertFail(D_00642F30, 0x17B);
    entry = D_00642F00[recipeKind].entries;
    count = D_00642F04[recipeKind].count;
    row = 0;
    threshold = (s16)(recipeKind + 2);
    while (row < count) {
        matched = 0;
        result = fclCombineRecipeMatchMask(entry, ingredients, ingredientCount, &matched);
        result = matched >= threshold ? result : 0;
        if (result != 0) {
            key = *entry;
            allowed = 1;
            for (flagIndex = 0; flagIndex < 0x17U; flagIndex++) {
                flag = D_006420A0 + flagIndex * 8;
                if (key == ((FclFusionGate *)flag)->persona) {
                    bits = ((FclFusionGate *)flag)->flags;
                    if (bits & 1) {
                        if (datGetFlag(((FclFusionGate *)flag)->flag) == 0) {
                            gate = 0;
                            goto flag_done;
                        }
                    } else if (bits & 2) {
                        allowed = 0;
                        if (datGetFlag(((FclFusionGate *)flag)->flag) == 1) {
                            gate = 1;
                            goto flag_done;
                        }
                    }
                }
            }
            gate = allowed;
flag_done:
            if (gate) {
                datPersonaInitRecord((u8 *)output, *entry);
                for (skillIndex = 0; skillIndex < 4; skillIndex++) {
                    skill = (u16 *)((u8 *)&entry[skillIndex] + 24);
                    if (*skill != 0 && datPersonaFindSkillSlot((u8 *)output, *skill) == -1 &&
                        datPersonaAddSkill((u8 *)output, *skill) == 0) break;
                }
                return iGpffffb3d4[output[1] * 14 + 2];
            }
        }
        entry += 16;
        row++;
    }
    return 0;
}
