/* The test driver inserts the exact Golden recipe cycling mirror splice here. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "p4pc/game_mode.h"
#include "p4pc/golden_recipe.h"
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef struct { s16 flags; } FclCombineRowSprite;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static P4GameMode mode = P4_GAME_MODE_GOLDEN;
P4GameMode p4_game_mode(void) { return mode; }
static u16 D_008C027A[1], D_008C024E[1];
static u16 records[2][12][24];
static int counts[2], stage_calls, sounds, gate = 1, missing, wrong_result;
static FclCombineRowSprite sprite;
static void *fclSpriteEntry(int id) { (void)id; return &sprite; }
static int fclSpriteFlagBit(s16 flags, int bit) { return (flags >> bit) & 1; }
static void yListSlotCreate(int kind, int slot) { (void)kind; counts[slot] = 0; memset(records[slot], 0, sizeof records[slot]); }
static void yListSlotAppend(int slot, int id, int roster)
{
    u16 *r = records[slot][counts[slot]++]; r[1] = (u16)id;
    ((u8 *)r)[4] = roster && id != missing ? 1 : 0;
}
static void *yListSlotEntry(int slot, int i) { return records[slot][i]; }
static int fclFuseRecipeLookup(u16 *output, int kind, u16 *ids, int count)
{
    int index = p4_golden_recipe_twelfth(); (void)output;
    if (!gate || kind != 5 || count != 12 || index < 0) return 0;
    for (int i = 0; i < 12; ++i) if (ids[i] != p4_golden_recipe_material((unsigned)index, (unsigned)i)) return 0;
    output[1] = (u16)(p4_golden_recipe_result((unsigned)index) + wrong_result);
    return 1;
}
static void fclCombineFuseSourceRecords(u16 *out, void *a, void *b, void *c, void *d, void *e, void *f, int n, int list, int row)
{ (void)out; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; if (n == 7 && list == 1 && row == 0) ++stage_calls; }
static void sdkSndStartStream(int a, int b, int c, int d) { (void)a; (void)b; (void)c; (void)d; ++sounds; }
static void cycle(u8 *p, void *task)
{
    u16 buttons; (void)task;
    /*@@RECIPE_CYCLE@@*/
    (void)buttons;
}
int main(void)
{
    P4GoldenRecipe recipes[2] = {{0}}; u8 work[0x300] = {0};
    for (int r = 0; r < 2; ++r) {
        recipes[r].result = (u16)(100 + r); recipes[r].material_count = 12;
        for (int i = 0; i < 12; ++i) recipes[r].materials[i] = (u16)(r * 20 + i + 1);
    }
    CHECK(!p4_golden_recipes_install(recipes, 2));
    D_008C027A[0] = 0x2000; cycle(work, NULL);
    CHECK(p4_golden_recipe_twelfth() == 1 && counts[0] == 12 && counts[1] == 1 && records[1][0][1] == 101);
    CHECK(records[0][0][1] == 21 && records[0][11][1] == 32 && work[0x20] == 0 && stage_calls == 1 && sounds == 1);
    gate = 0; D_008C027A[0] = 0x8000; cycle(work, NULL);
    CHECK(p4_golden_recipe_twelfth() == 0 && records[1][0][1] == 100 && work[0x20] == 1 && stage_calls == 1);
    gate = 1; missing = 21; D_008C027A[0] = 0x2000; cycle(work, NULL);
    CHECK(work[0x20] == 1 && stage_calls == 1);
    recipes[1].materials[11] = recipes[1].materials[0]; missing = 0;
    CHECK(!p4_golden_recipes_install(recipes, 2));
    D_008C027A[0] = 0x2000; cycle(work, NULL);
    CHECK(work[0x20] == 1 && stage_calls == 1);
    recipes[1].materials[11] = 32; wrong_result = 1;
    CHECK(!p4_golden_recipes_install(recipes, 2));
    D_008C027A[0] = 0x2000; cycle(work, NULL);
    CHECK(work[0x20] == 1 && stage_calls == 1);
    mode = P4_GAME_MODE_PS2; D_008C027A[0] = 0x8000; cycle(work, NULL);
    CHECK(records[1][0][1] == 101 && sounds == 5);
    puts("golden_recipe_ui: ok"); return 0;
}