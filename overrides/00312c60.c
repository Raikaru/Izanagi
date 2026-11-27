#include "p4pc/game_mode.h"
#include "p4pc/golden_fusion_pool.h"

s8 fclFuseRollPersona(u16 *persona, u8 *deck, s32 deckCount)
{
    FclFusionSkillPool *poolFields;
    u8 *pool;
    s32 *weights;
    s32 remaining;
    s32 total;
    s8 taken;
    s32 roll;
    u8 n;
    s16 k;
    s16 i;
    u8 *row;
    s16 scan;
    s32 running;
    u16 *cell;
    s32 id;
    s16 j;
    s16 freeSlots;
    s16 pick;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN && deckCount > 6) {
        P4GoldenFusionPool wide;
        s32 wideWeights[P4_GOLDEN_FUSION_POOL_CAPACITY];
        wide.count = 0;
        if (persona == NULL || deck == NULL || deckCount > 12) {
            kAssertFail(D_00642F30, 0x3AF);
            return 0;
        }
        for (i = 0; i < deckCount; ++i) {
            row = deck + i * 0x30;
            for (j = 0; j < 8; ++j) {
                cell = (u16 *)(row + j * 2 + 0xC);
                if (*cell != 0 && datPersonaFindSkillSlot((u8 *)persona, *cell) == -1 &&
                    p4_golden_fusion_pool_add(&wide, *cell) < 0) {
                    kAssertFail(D_00642F30, 0x3AF);
                    return 0;
                }
            }
        }
        remaining = fclSkillPickCount((s16)wide.count);
        freeSlots = 8 - datPersonaCountSkills(persona);
        if (freeSlots == 0) {
            func_00440b68(D_00642F50);
            return 0;
        }
        if (freeSlots < remaining) remaining = freeSlots;
        taken = 0;
        while (remaining != 0) {
            total = 0;
            for (k = 0; k < (s16)wide.count; ++k) {
                wideWeights[k] = fclSkillWeight(persona, wide.ids[k]);
                total += wideWeights[k];
            }
            if (total == 0) break;
            running = 0;
            roll = RpRandom() % total;
            for (pick = 0; pick < (s16)wide.count; ++pick) {
                running += wideWeights[pick];
                if (roll < running) {
                    for (scan = 0; scan < 8; ++scan) {
                        if (persona[scan + 6] == 0) {
                            taken |= (s8)(1 << scan);
                            break;
                        }
                    }
                    datPersonaAddSkill((u8 *)persona, wide.ids[pick]);
                    p4_golden_fusion_pool_remove(&wide, (unsigned)pick);
                    --remaining;
                    break;
                }
            }
        }
        return taken;
    }
    taken = 0;
    if (persona == NULL) {
        kAssertFail(D_00642F30, 0x3AF);
    }
    pool = yListAllocIndexTable();
    poolFields = (FclFusionSkillPool *)pool;
    for (i = 0; i < (s8)deckCount; i++) {
        j = 0;
        row = deck + i * 0x30;
        for (; j < 8; j++) {
            cell = (u16 *)(row + j * 2 + 0xC);
            if (*cell != 0 && datPersonaFindSkillSlot((u8 *)persona, *cell) == -1) {
                fclListAddUnique(pool, (s16)*cell);
            }
        }
    }
    remaining = fclSkillPickCount(((FclFusionSkillPool *)pool)->count);
    freeSlots = 8 - datPersonaCountSkills(persona);
    if (freeSlots == 0) {
        func_00440b68(D_00642F50);
        fclFreeWorkBlock(pool);
        return 0;
    }
    if (freeSlots < remaining) {
        remaining = freeSlots;
    }
    chkmemAllocSiteTag(D_00642F30, 0x3CC);
    weights = (*jtbl_008873E8)(poolFields->count * 4, 0x40000);
    while (remaining != 0) {
        total = 0;
        for (k = 0; k < MEMBER(s16, pool, FclFusionSkillPool, count); k++) {
            id = ((s16 *)pool)[k];
            weights[k] = fclSkillWeight(persona, id);
            total += weights[k];
        }
        if (total == 0) {
            break;
        }
        running = 0;
        roll = RpRandom() % total;
        n = pool[0x60];
        for (pick = 0; pick < n; pick++) {
            running += weights[pick];
            if (roll < running) {
                for (scan = 0; scan < 8; scan++) {
                    if (persona[scan + 6] == 0) {
                        taken |= (s8)(1 << scan);
                        break;
                    }
                }
                datPersonaAddSkill((u8 *)persona, ((s16 *)pool)[pick]);
                fclListRemoveId(pool, ((s16 *)pool)[pick]);
                remaining--;
                break;
            }
        }
    }
    (*jtbl_008873EC)(weights);
    fclFreeWorkBlock(pool);
    return taken;
}
