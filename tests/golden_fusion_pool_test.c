/* Exercise the real guest override with synthetic Persona records. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/game_mode.h"
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32;
typedef struct { s16 indices[48]; s16 count; } FclFusionSkillPool;
#define MEMBER(type, ptr, record, field) (((record *)(ptr))->field)
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static P4GameMode mode = P4_GAME_MODE_GOLDEN;
static int assertions, retail_calls;
static char D_00642F30[1], D_00642F50[1];
P4GameMode p4_game_mode(void) { return mode; }
static void kAssertFail(void *p, int n) { (void)p; (void)n; ++assertions; }
static int datPersonaFindSkillSlot(u8 *p, u16 id) { u16 *v = (u16 *)p; for (int i=0;i<8;i++) if (v[6+i]==id) return i; return -1; }
static int datPersonaAddSkill(u8 *p, u16 id) { u16 *v = (u16 *)p; for (int i=0;i<8;i++) if (!v[6+i]) { v[6+i]=id; return 0; } return -1; }
static int datPersonaCountSkills(u16 *v) { int n=0; for (int i=0;i<8;i++) n += v[6+i]!=0; return n; }
static int fclSkillPickCount(s16 count) { return count ? 8 : 0; }
static int fclSkillWeight(u16 *p, int id) { (void)p; (void)id; return 1; }
static u32 RpRandom(void) { return 0; }
static u8 *yListAllocIndexTable(void) { ++retail_calls; return calloc(1,sizeof(FclFusionSkillPool)); }
static void fclListAddUnique(u8 *p, s16 id) { FclFusionSkillPool *v=(void *)p; for (int i=0;i<v->count;i++) if (v->indices[i]==id) return; v->indices[v->count++]=id; }
static void fclListRemoveId(u8 *p, s16 id) { FclFusionSkillPool *v=(void *)p; for (int i=0;i<v->count;i++) if (v->indices[i]==id) { memmove(&v->indices[i],&v->indices[i+1],(v->count-i-1)*sizeof(s16)); --v->count; return; } }
static void fclFreeWorkBlock(u8 *p) { free(p); }
static void func_00440b68(const void *p) { (void)p; }
static void chkmemAllocSiteTag(const void *p, s32 n) { (void)p; (void)n; }
static void *allocate(u32 size, u32 align) { (void)align; return malloc(size ? size : 1); }
static void deallocate(void *p) { free(p); }
static void *(*jtbl_008873E8[])(u32,u32) = {allocate};
static void (*jtbl_008873EC[])(void *) = {deallocate};
#include "../overrides/00312c60.c"
static u16 slot_records[12][24], result_record[24];
static u8 slot_work[0x400];
static u8 *yListSlotEntry(s8 slot, s16 index) { if (slot == 1 && index == 0) return (u8 *)result_record; return (slot == 0 || slot == 4) && index >= 0 && index < 12 ? (u8 *)slot_records[index] : NULL; }
static u8 *yListSlotWork(s8 slot) { (void)slot; return slot_work; }
static void datPersonaInitRecord(u8 *p, u16 id) { memset(p, 0, 48); ((u16 *)p)[1] = id; }
#include "../overrides/002f9c30.c"
typedef struct { u16 *entries; s32 count; } FclFusionRecipeTable;
typedef struct { s32 count; u8 pad04[4]; } FclFusionRecipeCount;
typedef struct { s32 flag; s16 persona, flags; } FclFusionGate;
static u8 base_table[512 * 14], gate_table[23 * 8];
static u8 *iGpffffb3d4 = base_table;
static u8 *D_006420A0 = gate_table;
static FclFusionRecipeTable D_00642F00[5];
static FclFusionRecipeCount D_00642F04[5];
static int gate_count, gate_id, gate_bits, flag_value;
unsigned p4_golden_fusion_gate_count(void) { return (unsigned)gate_count; }
int p4_golden_fusion_gate_flag(unsigned i) { (void)i; return 1; }
int p4_golden_fusion_gate_persona(unsigned i) { (void)i; return gate_id; }
int p4_golden_fusion_gate_bits(unsigned i) { (void)i; return gate_bits; }
void p4_game_table_admission_golden_fail(const char *reason, int result) { (void)reason; (void)result; abort(); }
static int datGetFlag(int id) { (void)id; return flag_value; }
static int fclCombineRecipeMatchMask(u16 *r, u16 *ids, s32 count, s16 *matched)
{ (void)ids; (void)count; *matched = 2; return *r ? 3 : 0; }
#include "../overrides/00312220.c"
typedef struct { u8 pad[0x11e]; s8 f11E; } FclCombineWork;
typedef struct { u8 *work; } KwlnTask;
typedef struct { u16 flags, id; } DatPersonaRecord;
static int seen_count; static u16 seen_ids[13];
static int fclCombineCheckRules(u8 *task, s8 count, u16 *ids)
{ (void)task; seen_count = count; memcpy(seen_ids, ids, (size_t)(count + 1) * sizeof(u16)); return 1; }
#include "../overrides/00303a20.c"
int main(void)
{
    u16 persona[24] = {0}, deck[12][24] = {{0}};
    for (int i=0;i<12;i++) for (int j=0;j<8;j++) deck[i][6+j]=(u16)(i*8+j+1);
    CHECK((u8)fclFuseRollPersona(persona,(u8 *)deck,12)==255);
    for (int i=0;i<8;i++) CHECK(persona[6+i]==i+1);
    CHECK(!retail_calls && !assertions);
    memset(persona,0,sizeof persona);
    for (int i=0;i<12;i++) for (int j=0;j<8;j++) deck[i][6+j]=(u16)(j+1);
    CHECK((u8)fclFuseRollPersona(persona,(u8 *)deck,12)==255);
    mode = P4_GAME_MODE_GOLDEN;
    memset(persona, 0, sizeof persona); persona[1] = 100;
    for (int i=0;i<12;i++) for (int j=0;j<8;j++) slot_records[i][6+j] = (u16)(i*8+j+1);
    fclCombineFuseSourceRecords(persona,NULL,NULL,NULL,NULL,NULL,NULL,7,0,3);
    CHECK(persona[1] == 100 && slot_work[0x2e4+3] == 255);
    for (int i=0;i<8;i++) CHECK(persona[6+i] == i+1);
    memset(persona, 0, sizeof persona); persona[1] = 101;
    fclCombineFuseSourceRecords(persona,NULL,NULL,NULL,NULL,NULL,NULL,7,1,0);
    CHECK(persona[1] == 101 && slot_work[0x2e4] == 255);
    for (int i=0;i<8;i++) CHECK(persona[6+i] == i+1);
    for (int i=0;i<8;i++) CHECK(persona[6+i]==i+1);
    CHECK(fclFuseRollPersona(persona,(u8 *)deck,13)==0 && assertions==1);
    {
        u8 work[0x300] = {0}; KwlnTask task = {work};
        work[0x1a] = 7; work[0x2f9] = 1; work[0x2fa] = 0;
        for (int i=0;i<12;i++) slot_records[i][1] = (u16)(i+1);
        result_record[1] = 123;
        CHECK(fclCombineCheckIds((u8 *)&task) == 1 && seen_count == 12);
        CHECK(seen_ids[0] == 1 && seen_ids[11] == 12 && seen_ids[12] == 123);
    }
    {
        P4GoldenRecipe recipe = {0};
        u16 ingredients[12], ps2row[16] = {55};
        recipe.result = 100; recipe.material_count = 12;
        for (int i = 0; i < 12; ++i) recipe.materials[i] = (u16)(i + 1);
        recipe.materials[11] = 1;
        for (int i = 0; i < 12; ++i) ingredients[i] = recipe.materials[11 - i];
        base_table[100 * 14 + 2] = 9;
        CHECK(p4_golden_recipes_install(&recipe, 1) == 0);
        gate_count = 1; gate_id = 100; gate_bits = 1; flag_value = 0;
        CHECK(fclFuseRecipeLookup(persona, 5, ingredients, 12) == 0);
        flag_value = 1;
        CHECK(fclFuseRecipeLookup(persona, 5, ingredients, 12) == 9 && persona[1] == 100);
        ingredients[0] = 2;
        CHECK(fclFuseRecipeLookup(persona, 5, ingredients, 12) == 0);
        {
            P4GoldenRecipe two[2] = {recipe, recipe};
            two[1].result = 102;
            for (int i = 0; i < 12; ++i) two[1].materials[i] = (u16)(30 + i);
            base_table[102 * 14 + 2] = 10;
            CHECK(!p4_golden_recipes_install(two, 2));
            CHECK(p4_golden_recipe_twelfth() == 0 && p4_golden_recipe_cycle_twelfth(1) == 1);
            for (int i = 0; i < 12; ++i) ingredients[i] = two[1].materials[11 - i];
            CHECK(fclFuseRecipeLookup(persona, 5, ingredients, 12) == 10 && persona[1] == 102);
            CHECK(p4_golden_recipe_cycle_twelfth(1) == 0 && p4_golden_recipe_cycle_twelfth(-1) == 1);
            two[1] = recipe; two[1].result = 102;
            CHECK(!p4_golden_recipes_install(two, 2) && p4_golden_recipe_cycle_twelfth(1) == 1);
            for (int i = 0; i < 12; ++i) ingredients[i] = recipe.materials[11 - i];
            gate_count = 1; gate_id = 100; flag_value = 1;
            CHECK(fclFuseRecipeLookup(persona, 5, ingredients, 12) == 10 && persona[1] == 102);
        }
        mode = P4_GAME_MODE_PS2;
        D_00642F00[0].entries = ps2row; D_00642F04[0].count = 1;
        base_table[55 * 14 + 2] = 7;
        CHECK(fclFuseRecipeLookup(persona, 0, ingredients, 2) == 7 && persona[1] == 55);
    }
    mode=P4_GAME_MODE_PS2; memset(persona,0,sizeof persona);
    CHECK((u8)fclFuseRollPersona(persona,(u8 *)deck,2)==255 && retail_calls==1);
    puts("golden_fusion_pool: ok"); return 0;
}
