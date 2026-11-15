/* Weapon fusion transaction (include/p4pc/mod_forge.h) over a fake game state:
 * whole validation before queueing, the weapon_fusion authorizer's commit stage
 * spending nothing on denial, one all-or-nothing commit at the safe point and
 * waiting out a battle. The game accessors are the stubs below. */
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_forge.h"
#include "p4pc/mod_persona.h"
#include "guest_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
uint32_t p4_mod_safe_point;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_forge: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)

/* Fake game: items 1..3 Yu swords, 0x700 an expanded Yu sword, 0x710 armor. */
static int32_t counts[0x800];
static uint32_t money;
static int battle;
static int joined[9];
static int failed;
static P4ModStockEntry stock[3];
static int stock_count;
static uint8_t rows[0x800][68];

_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    fprintf(stderr, "mod_forge: failure owner %zu: %s\n", owner, reason);
    if (failed && !failures) puts("mod_forge: ok");
    exit(failed && !failures ? 0 : 1);
}
int32_t datItemCount(int16_t item) { return counts[item]; }
void datItemSetCount(int16_t item, int32_t count) { counts[item] = count; }
int32_t datMemberIsUnlocked(int64_t member) { return joined[member]; }
int32_t datIdBand(int16_t item) { return item == 0x710 ? 1 : 0; }
uint32_t datGetMoney(void) { return money; }
void datSetMoney(uint32_t amount) { money = amount; }
int32_t datPersonaRemoveRosterMemberById(int32_t persona)
{
    for (int i = 0; i < stock_count; ++i)
        if (stock[i].persona == persona) {
            memmove(&stock[i], &stock[i + 1], (size_t)(stock_count - i - 1) * sizeof stock[0]);
            --stock_count;
            return 1;
        }
    return 0;
}
void *kwlnTaskGetTaskByName(const void *name) { (void)name; return battle ? (void *)&battle : NULL; }
int p4_mod_persona_stock(P4ModStockEntry *out, int capacity)
{
    int n = stock_count < capacity ? stock_count : capacity;
    memcpy(out, stock, (size_t)n * sizeof *out);
    return n;
}
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id)
{
    return id && id < 0x800 && rows[id][0x30] ? rows[id] : NULL;
}

static void reset(void)
{
    memset(counts, 0, sizeof counts);
    memset(rows, 0, sizeof rows);
    for (int id = 1; id <= 8; ++id) joined[id] = 1;
    for (int id = 1; id <= 3; ++id) rows[id][0x30] = 1u << 5; /* Yu: bit characterId + 4 */
    rows[0x700][0x30] = 1u << 5;
    rows[0x710][0x30] = 1u << 5;
    counts[2] = 1;
    money = 5000;
    stock_count = 3;
    memset(stock, 0, sizeof stock);
    stock[0] = (P4ModStockEntry){.slot = 0, .lead = 1, .persona = 1};
    stock[1] = (P4ModStockEntry){.slot = 1, .persona = 7};
    stock[2] = (P4ModStockEntry){.slot = 2, .persona = 61};
}

static const char *denial = NULL;
static int challenge(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)context; (void)user;
    if (!denial) return 1;
    snprintf(reason, capacity, "%s", denial);
    return 0;
}

int main(void)
{
    if (p4t_guest_init()) return 1;
    char reason[192];
    P4ModForge forge = {.character = 1, .persona = 7, .vessel = 2, .result = 0x700, .fee = 1200};
    reset();
    /* The vessel read is the commit's own vessel check: a weapon row the fighter may
     * equip, its unequipped count, no display name involved; 0 for anything else. */
    counts[0x710] = 4;
    CHECK(p4_mod_forge_vessel_count(1, 2) == 1 && p4_mod_forge_vessel_count(1, 3) == 0);
    CHECK(p4_mod_forge_vessel_count(2, 2) == 0 && p4_mod_forge_vessel_count(5, 2) == 0);
    CHECK(p4_mod_forge_vessel_count(0, 2) == 0 && p4_mod_forge_vessel_count(9, 2) == 0);
    CHECK(p4_mod_forge_vessel_count(1, 0x710) == 0 && p4_mod_forge_vessel_count(1, 0x7ff) == 0);
    CHECK(p4_mod_forge_vessel_count(1, 0) == 0 && p4_mod_forge_vessel_count(1, 0x7ffe) == 0);
    counts[0x700] = -1;
    CHECK(p4_mod_forge_vessel_count(1, 0x700) == 0);
    counts[0x700] = 2;
    CHECK(p4_mod_forge_vessel_count(1, 0x700) == 2);
    CHECK(!(p4_mod_safe_point & P4_MOD_SAFE_FORGE) && !p4_mod_forge_pending(9));
    /* A compatible held weapon does not authorize an unrecruited fighter. */
    rows[2][0x30] |= 1u << 6;
    rows[0x700][0x30] |= 1u << 6;
    joined[2] = 0;
    CHECK(p4_mod_forge_vessel_count(2, 2) == 0);
    P4ModForge unjoined = forge; unjoined.character = 2;
    CHECK(p4_mod_forge_queue(9, &unjoined, reason, sizeof reason) && strstr(reason, "has not joined"));
    CHECK(!p4_mod_forge_pending(9) && stock_count == 3 && counts[2] == 1 && counts[0x700] == 2 && money == 5000);
    joined[2] = 1;
    CHECK(p4_mod_forge_vessel_count(2, 2) == 1);
    CHECK(!p4_mod_forge_queue(9, &unjoined, reason, sizeof reason));
    joined[2] = 0;
    CHECK(p4_mod_forge_apply_pending() == 0);
    P4ModForge refused;
    CHECK(p4_mod_forge_take_result(9, &refused, reason, sizeof reason) == P4_MOD_FORGE_REFUSED);
    CHECK(refused.character == 2 && refused.result == 0x700 && strstr(reason, "has not joined"));
    CHECK(stock_count == 3 && counts[2] == 1 && counts[0x700] == 2 && money == 5000);
    p4_mod_safe_point &= ~P4_MOD_SAFE_FORGE;
    reset();
    /* Whole validation: every bad field refuses and queues nothing. */
    P4ModForge bad = forge; bad.persona = 1;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "equipped"));
    bad = forge; bad.persona = 99;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "not carried"));
    bad = forge; bad.vessel = 3;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "unequipped"));
    bad = forge; bad.result = 0x710;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "not a weapon"));
    bad = forge; bad.character = 2;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "not a weapon"));
    bad = forge; bad.character = 5;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "does not fight"));
    bad = forge; bad.fee = 6000;
    CHECK(p4_mod_forge_queue(9, &bad, reason, sizeof reason) && strstr(reason, "money"));
    CHECK(!(p4_mod_safe_point & P4_MOD_SAFE_FORGE) && !p4_mod_forge_pending(9));
    battle = 1;
    CHECK(p4_mod_forge_queue(9, &forge, reason, sizeof reason) && strstr(reason, "battle"));
    battle = 0;

    /* A challenge denial at commit spends nothing and reports its reason. */
    P4ModAuthorizer *rule = p4_mod_authorizer_add(3, "weapon_fusion", challenge, NULL);
    CHECK(rule);
    denial = "NO WEAPON FUSION challenge rule";
    CHECK(!p4_mod_forge_queue(9, &forge, reason, sizeof reason));
    CHECK(p4_mod_forge_pending(9) && (p4_mod_safe_point & P4_MOD_SAFE_FORGE));
    CHECK(p4_mod_forge_queue(10, &forge, reason, sizeof reason) && strstr(reason, "pending"));
    CHECK(p4_mod_forge_take_result(9, NULL, reason, sizeof reason) == P4_MOD_FORGE_NONE);
    CHECK(p4_mod_forge_apply_pending() == 0);
    p4_mod_safe_point &= ~P4_MOD_SAFE_FORGE;
    CHECK(stock_count == 3 && counts[2] == 1 && counts[0x700] == 0 && money == 5000);
    P4ModForge seen;
    CHECK(p4_mod_forge_take_result(9, &seen, reason, sizeof reason) == P4_MOD_FORGE_REFUSED);
    CHECK(!strcmp(reason, "NO WEAPON FUSION challenge rule") && seen.persona == 7);
    CHECK(p4_mod_forge_take_result(9, NULL, NULL, 0) == P4_MOD_FORGE_NONE);

    /* The rule can change after queueing: the safe point asks again, before
     * revalidation as well as before any spending, even if the vessel vanished. */
    denial = NULL;
    CHECK(!p4_mod_forge_queue(9, &forge, reason, sizeof reason));
    counts[2] = 0;
    denial = "NO WEAPON FUSION challenge rule";
    CHECK(p4_mod_forge_apply_pending() == 0);
    CHECK(stock_count == 3 && counts[2] == 0 && counts[0x700] == 0 && money == 5000);
    CHECK(p4_mod_forge_take_result(9, &seen, reason, sizeof reason) == P4_MOD_FORGE_REFUSED);
    CHECK(!strcmp(reason, "NO WEAPON FUSION challenge rule"));
    counts[2] = 1;

    /* Allowed: one complete commit, after waiting out a battle. */
    denial = NULL;
    CHECK(!p4_mod_forge_queue(9, &forge, reason, sizeof reason));
    battle = 1;
    CHECK(p4_mod_forge_apply_pending() == 1);
    CHECK(stock_count == 3 && money == 5000);
    battle = 0;
    CHECK(p4_mod_forge_apply_pending() == 0);
    CHECK(stock_count == 2 && stock[1].persona == 61);
    CHECK(counts[2] == 0 && counts[0x700] == 1 && money == 3800);
    CHECK(p4_mod_forge_take_result(9, &seen, reason, sizeof reason) == P4_MOD_FORGE_FORGED && reason[0] == 0);
    CHECK(seen.result == 0x700 && seen.fee == 1200);

    /* The owner's teardown drops a queued request. */
    reset();
    CHECK(!p4_mod_forge_queue(9, &forge, reason, sizeof reason));
    p4_mod_forge_clear_owner(9);
    CHECK(!p4_mod_forge_pending(9) && p4_mod_forge_apply_pending() == 0);
    CHECK(stock_count == 3 && money == 5000);

    /* State that changed after queueing is the owner's failure, never partial. */
    p4_mod_authorizers_clear();
    CHECK(!p4_mod_forge_queue(9, &forge, reason, sizeof reason));
    counts[2] = 0;
    if (failures) return 1;
    failed = 1;
    p4_mod_forge_apply_pending();
    fprintf(stderr, "mod_forge: a misfit committed instead of failing\n");
    return 1;
}
