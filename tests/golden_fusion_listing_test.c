/* Exercise the actual listing and ownership overrides with synthetic list slots. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "p4pc/game_mode.h"
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef int64_t s64;
typedef struct { u8 *recipes; s32 unk04; } FclMaterialTableEntry;
typedef struct { u16 result; u8 pad02[6]; s16 materials[8]; u8 pad18[8]; } FclMaterialRecipe;
typedef struct { u16 flags, personaId; } FclCombinePersonaHeader;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static P4GameMode mode = P4_GAME_MODE_GOLDEN;
P4GameMode p4_game_mode(void) { return mode; }
static u8 works[15][1024], records[15][12][48], roster[512];
static int counts[15], gate_count, gate_persona, gate_flags, gate_value;
static u32 D_00642F00[16], D_00642EF0[16];
static u8 *yListSlotWork(s8 slot) { return works[(int)slot]; }
static u8 *yListSlotEntry(s8 slot, s32 index) { return records[(int)slot][index]; }
static void *datPersonaFindRosterRecordById(u16 id) { return id < 512 && roster[id] ? &roster[id] : NULL; }
static void yListSlotAppend(s8 slot, s32 id, s8 useRoster)
{ u8 *r=records[(int)slot][counts[(int)slot]++]; ((u16 *)r)[1]=(u16)id; r[4]=(u8)(useRoster && roster[id]); }
static int fclCombineMaterialAvailable(u16 id) { (void)id; return 1; }
static int datGetFlag(s32 id) { (void)id; return gate_value; }
unsigned p4_golden_fusion_gate_count(void) { return gate_count; }
int p4_golden_fusion_gate_flag(unsigned i) { (void)i; return 1; }
int p4_golden_fusion_gate_persona(unsigned i) { (void)i; return gate_persona; }
int p4_golden_fusion_gate_bits(unsigned i) { (void)i; return gate_flags; }
void p4_game_table_admission_golden_fail(const char *why, int result) { (void)why; (void)result; }
static int operandOrderNotEqual(int a, int b) { return a != b; }
#include "../overrides/00313800.c"
#include "../overrides/003139d0.c"
#include "../overrides/00313a80.c"
#include "../overrides/00313ae0.c"
int main(void)
{
    P4GoldenRecipe recipes[2] = {{0}};
    recipes[0].result=100; recipes[0].material_count=4;
    recipes[1].result=101; recipes[1].material_count=4;
    for (int i=0;i<4;i++) { recipes[0].materials[i]=(u16)(i+1); recipes[1].materials[i]=(u16)(i+5); roster[i+1]=roster[i+5]=1; }
    CHECK(!p4_golden_recipes_install(recipes,2));
    gate_count=1; gate_persona=101; gate_flags=1; gate_value=0;
    fclCombineMaterialListBuild(4);
    CHECK(counts[0]==1 && counts[1]==4 && ((u16 *)records[0][0])[1]==100);
    CHECK(fclCombineMaterialsHeld(4,0)==1 && fclCombineItemSlot(4,100)==0);
    roster[100]=1; CHECK(fclCombineSlotOwned(4,0)==1);
    roster[3]=0; CHECK(fclCombineMaterialsHeld(4,0)==0); roster[3]=1;
    ((u16 *)records[1][1])[1]=1; CHECK(fclCombineMaterialsHeld(4,0)==0);
    mode=P4_GAME_MODE_PS2; CHECK(fclCombineMaterialsHeld(4,0)==1);
    mode=P4_GAME_MODE_GOLDEN; memset(counts,0,sizeof counts); gate_value=1;
    fclCombineMaterialListBuild(4);
    CHECK(counts[0]==2 && counts[1]==4 && counts[2]==4 && fclCombineItemSlot(4,101)==1);
    puts("golden_fusion_listing: ok"); return 0;
}
