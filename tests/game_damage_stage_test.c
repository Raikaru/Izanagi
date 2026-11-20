/* Final-damage stage order, single conversion, clamp and refusals. The amount
 * equation itself is exercised by game_elements_amount_test. */
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_damage_stage_native.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static char order[64];
static size_t order_length;
typedef struct Step { char tag; double value; int decline; } Step;
static int step(void *user, const P4GameDamageHit *hit, double *value)
{
    const Step *s = user;
    (void)hit;
    assert(order_length + 1u < sizeof order);
    order[order_length++] = s->tag;
    if (s->decline) return 0;
    *value = s->value;
    return 1;
}
static uint32_t add(char tag, int32_t priority, uint32_t load, P4GameDamageKind kind, Step *s, double value)
{
    s->tag = tag; s->value = value; s->decline = 0;
    const P4GameDamageModifierDesc desc = {"test.mod", priority, load, kind, step, s};
    uint32_t handle = 0;
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_OK && handle);
    return handle;
}
static P4GameDamageHit hit(int32_t stock)
{
    return (P4GameDamageHit){.skill_id = 10, .source_side = P4_DAMAGE_SIDE_PARTY, .target_side = P4_DAMAGE_SIDE_ENEMY,
        .origin_side = P4_DAMAGE_SIDE_ENEMY, .hit_kind = P4_DAMAGE_HIT_NORMAL, .resolved = 1, .stock = stock};
}
static int32_t run(int32_t stock)
{
    order_length = 0; memset(order, 0, sizeof order);
    const P4GameDamageHit h = hit(stock);
    return p4_game_damage_apply(&h);
}
static void clear(const uint32_t *handles, size_t n)
{
    for (size_t i = 0; i < n; ++i) assert(p4_game_damage_unregister(handles[i]) == P4_DAMAGE_OK);
    assert(!p4_game_damage_count() && !p4_game_damage_native_active());
}

static void ordering(void)
{
    Step s[6];
    /* Registered out of order: priority, then multiply before add, then load
     * order, then registration order decide. */
    uint32_t handles[6];
    handles[0] = add('f', 5, 3, P4_DAMAGE_ADD, &s[0], 1.0);
    handles[1] = add('d', 5, 9, P4_DAMAGE_MULTIPLY, &s[1], 1.0);
    handles[2] = add('a', -3, 7, P4_DAMAGE_ADD, &s[2], 1.0);
    handles[3] = add('b', 5, 2, P4_DAMAGE_MULTIPLY, &s[3], 1.0);
    handles[4] = add('c', 5, 2, P4_DAMAGE_MULTIPLY, &s[4], 1.0);
    handles[5] = add('e', 5, 1, P4_DAMAGE_ADD, &s[5], 1.0);
    assert(p4_game_damage_count() == 6 && p4_game_damage_native_active());
    (void)run(100);
    assert(!strcmp(order, "abcdef"));
    clear(handles, 6);
}
static void composition(void)
{
    Step s[3];
    /* One conversion at the end: 10 * 0.75 * 1.25 = 9.375 -> 9. Rounding per
     * modifier would give 7 then 8. */
    uint32_t handles[3] = {add('m', 0, 0, P4_DAMAGE_MULTIPLY, &s[0], 0.75), add('n', 0, 1, P4_DAMAGE_MULTIPLY, &s[1], 1.25)};
    assert(run(10) == 9);
    /* Additive after multiplicative in one priority: 10 * 0.75 * 1.25 + 0.7 = 10.075. */
    handles[2] = add('p', 0, 0, P4_DAMAGE_ADD, &s[2], 0.7);
    assert(run(10) == 10 && !strcmp(order, "mnp"));
    /* A declining modifier leaves the value alone but is still consulted. */
    s[0].decline = 1;
    assert(run(10) == 13 && !strcmp(order, "mnp")); /* 10 * 1.25 + 0.7 = 13.2 */
    s[0].decline = 0;
    clear(handles, 3);
}
static void clamp(void)
{
    Step s;
    uint32_t handle = add('z', 0, 0, P4_DAMAGE_MULTIPLY, &s, 0.0);
    assert(run(500) == P4_DAMAGE_MIN);
    s.value = 1e300; /* overflow stays finite and saturates at the retail maximum */
    assert(run(32550) == P4_DAMAGE_MAX);
    s.value = 1.0;
    assert(run(P4_DAMAGE_MAX) == P4_DAMAGE_MAX && run(P4_DAMAGE_MIN) == P4_DAMAGE_MIN);
    clear(&handle, 1);
    handle = add('y', 0, 0, P4_DAMAGE_ADD, &s, -100.0);
    assert(run(40) == P4_DAMAGE_MIN);
    clear(&handle, 1);
}
static void invalid_steps(void)
{
    Step s[2];
    uint32_t handles[2] = {add('i', 0, 0, P4_DAMAGE_MULTIPLY, &s[0], -2.0), add('j', 0, 0, P4_DAMAGE_ADD, &s[1], 5.0)};
    assert(run(20) == 25); /* the negative factor is ignored for that hit */
    s[0].value = NAN;
    assert(run(20) == 25);
    s[1].value = INFINITY;
    assert(run(20) == 20);
    clear(handles, 2);
}
static uint32_t inner_handle;
static int32_t nested_result;
static int reenter(void *user, const P4GameDamageHit *h, double *value)
{
    (void)user;
    const P4GameDamageModifierDesc desc = {"test.inner", 0, 0, P4_DAMAGE_ADD, step, NULL};
    assert(p4_game_damage_register(&desc, &inner_handle) == P4_DAMAGE_BUSY);
    assert(p4_game_damage_unregister(1u) == P4_DAMAGE_BUSY);
    const P4GameDamageHit inner = hit(77);
    nested_result = p4_game_damage_apply(&inner);
    (void)h;
    *value = 2.0;
    return 1;
}
static void refusals(void)
{
    uint32_t handle = 0;
    P4GameDamageModifierDesc desc = {"test.mod", 0, 0, P4_DAMAGE_MULTIPLY, NULL, NULL};
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_INVALID);
    desc.modifier = step;
    desc.owner = "";
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_INVALID);
    desc.owner = "has space";
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_INVALID);
    char long_owner[P4_DAMAGE_OWNER_SIZE + 1];
    memset(long_owner, 'a', sizeof long_owner - 1u); long_owner[sizeof long_owner - 1u] = 0;
    desc.owner = long_owner;
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_INVALID);
    desc.owner = "test.mod"; desc.kind = (P4GameDamageKind)7;
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_INVALID);
    assert(p4_game_damage_register(&desc, NULL) == P4_DAMAGE_INVALID);
    assert(p4_game_damage_unregister(12345u) == P4_DAMAGE_NOT_FOUND);
    assert(!p4_game_damage_count());
    desc.kind = P4_DAMAGE_MULTIPLY; desc.modifier = reenter;
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_OK);
    assert(run(30) == 60 && nested_result == 77);
    clear(&handle, 1);
}
static P4GameDamageHit seen;
static int capture(void *user, const P4GameDamageHit *h, double *value)
{
    (void)user; (void)value;
    seen = *h;
    return 0;
}
static void native_bracket(void)
{
    const P4GameDamageModifierDesc desc = {"test.capture", 0, 0, P4_DAMAGE_MULTIPLY, capture, NULL};
    uint32_t handle;
    assert(p4_game_damage_register(&desc, &handle) == P4_DAMAGE_OK);
    /* An estimate: no bracket, origin is the target; ailment words pass as given.
     * Its command is unknown and a party source still reports its persona. */
    assert(p4_game_damage_native_amount(10, 0, 3, 0x22u, 1, 40, 0x100040u, P4_DAMAGE_HIT_WEAK, 1, 19, 55, 0x2a, 31) == 55);
    assert(!seen.resolved && !seen.reflected && seen.origin_side == P4_DAMAGE_SIDE_ENEMY && seen.origin_id == 40 &&
        seen.source_id == 3 && seen.source_status == 0x22u && seen.target_side == P4_DAMAGE_SIDE_ENEMY &&
        seen.target_status == 0x100040u && seen.hit_kind == P4_DAMAGE_HIT_WEAK &&
        seen.element_valid && seen.element_id == 19 && seen.stock == 55 &&
        seen.origin == P4_DAMAGE_ORIGIN_UNKNOWN && seen.source_persona == 0x2a && seen.source_persona_level == 31);
    /* An enemy source never reports a persona. */
    (void)p4_game_damage_native_amount(10, 1, 40, 0, 0, 3, 0, P4_DAMAGE_HIT_NORMAL, 0, 0, 12, 0x2a, 31);
    assert(seen.source_side == P4_DAMAGE_SIDE_ENEMY && !seen.source_persona && !seen.source_persona_level);
    /* A repelled resolved skill hit: delivered to its party source, aimed at enemy 40. */
    p4_game_damage_native_hit_begin(10, 1, 40, 1, 0, 0);
    assert(p4_game_damage_native_amount(10, 0, 3, 0, 0, 3, 0, P4_DAMAGE_HIT_NORMAL, 0, 0, 12, 0x2a, 31) == 12);
    p4_game_damage_native_hit_end();
    assert(seen.resolved && seen.reflected && seen.origin_side == P4_DAMAGE_SIDE_ENEMY && seen.origin_id == 40 &&
        seen.target_side == P4_DAMAGE_SIDE_PARTY && seen.target_id == 3 && !seen.element_valid &&
        seen.origin == P4_DAMAGE_ORIGIN_SKILL && seen.source_persona == 0x2a && seen.source_persona_level == 31);
    /* An item action and a weapon-property attack report their command and no persona. */
    p4_game_damage_native_hit_begin(10, 1, 40, 0, 1, 0);
    (void)p4_game_damage_native_amount(10, 0, 3, 0, 1, 40, 0, P4_DAMAGE_HIT_NORMAL, 0, 0, 12, 0x2a, 31);
    p4_game_damage_native_hit_end();
    assert(seen.resolved && seen.origin == P4_DAMAGE_ORIGIN_ITEM && !seen.source_persona && !seen.source_persona_level);
    p4_game_damage_native_hit_begin(10, 1, 40, 0, 0, 1);
    (void)p4_game_damage_native_amount(10, 0, 3, 0, 1, 40, 0, P4_DAMAGE_HIT_NORMAL, 0, 0, 12, 0x2a, 31);
    p4_game_damage_native_hit_end();
    assert(seen.resolved && seen.origin == P4_DAMAGE_ORIGIN_ATTACK && !seen.source_persona);
    /* A bracket for another skill never marks this call resolved. */
    p4_game_damage_native_hit_begin(11, 1, 40, 1, 1, 0);
    (void)p4_game_damage_native_amount(10, 0, 3, 0, 0, 3, 0, P4_DAMAGE_HIT_NORMAL, 0, 0, 12, 0x2a, 31);
    p4_game_damage_native_hit_end();
    assert(!seen.resolved && !seen.reflected && seen.origin_side == P4_DAMAGE_SIDE_PARTY &&
        seen.origin == P4_DAMAGE_ORIGIN_UNKNOWN && seen.source_persona == 0x2a);
    clear(&handle, 1);
}

int main(void)
{
    assert(!p4_game_damage_native_active());
    ordering();
    composition();
    clamp();
    invalid_steps();
    refusals();
    native_bracket();
    puts("game_damage_stage: order, single conversion, clamp, refusals and the 001f14f0 bracket passed");
    return 0;
}
