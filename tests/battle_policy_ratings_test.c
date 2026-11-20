/* Exercise the real affinity policy's roster decision with synthetic guest records. */
#include "guest_test.h"
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "p4pc/battle_sim.h"
/* Keep unrelated policy entrypoints local so PE links need not resolve their
 * dependencies before section GC. This fixture exercises only the ratings. */
#define p4_battle_policy_begin fixture_policy_begin
#define p4_battle_policy_error fixture_policy_error
#define p4_battle_policy_input fixture_policy_input
#define p4_battle_policy_target fixture_policy_target
#define p4_battle_policy_before_update fixture_policy_before_update
static int fixture_policy_begin(uint32_t, uint32_t);
static const char *fixture_policy_error(void);
static int fixture_policy_input(uint32_t);
static void fixture_policy_target(uint32_t);
static void fixture_policy_before_update(void);
#include "../src/platform/battle_sim/policy.c"

static uint32_t roster;
static int available = 1;
int32_t btlTargetCheckCommandAvailability(void *action, int64_t kind)
{
    (void)action;
    return kind == 5 && available;
}
void *datPersonaRosterSlot(int16_t slot) { return P4_G2H(roster + (uint32_t)slot * 8); }
int32_t datPersonaGetLeadSlot(void) { return 0; }
uint16_t datPersonaCountValidMembers(void) { return 2; }

int main(void)
{
    const uint32_t actor = 0x01000000u, unit = actor + 0x1000;
    roster = actor + 0x2000;
    if (p4t_guest_map(actor, 0x3000) || p4t_guest_map(P4_GPADDR(0xb3ac), 4)) return 77;
    P4_G32(actor + 0x30) = unit;
    P4_G16(unit + 0xa4) = 1;
    P4_G16(roster + 2) = 1;
    P4_G16(roster + 10) = 7;
    P4_GPDATA(uint32_t, 0xb3ac) = 0; /* no observed weaknesses */
    /* The first synthetic roster entry has physical/electric coverage; the second has two ice skills. */
    const uint8_t payload[] = {
        'P','4','A','F','F','R','T','1', 2,0,0,0,
        1,0, 1,0,0,1,0,0,0,0,
        7,0, 0,0,2,0,0,0,0,0
    };
    assert(p4_battle_policy_set_ratings(payload, sizeof payload));
    memset(&policy, 0, sizeof policy);
    choose_persona(actor);
    assert(policy.choice.wheel == WHEEL_PERSONA && policy.choice.menu_index == 1);
    /* A tie keeps the current Persona; an unavailable command never switches. */
    ratings.value[1][2] = 1;
    memset(&policy, 0, sizeof policy);
    choose_persona(actor);
    assert(policy.choice.wheel == 0);
    ratings.value[1][2] = 2;
    available = 0;
    choose_persona(actor);
    assert(policy.choice.wheel == 0);
    return 0;
}
