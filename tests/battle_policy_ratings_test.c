/* Exercise the real affinity policy's roster decision with synthetic guest records. */
#include "guest_test.h"
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include "p4pc/battle_sim.h"
/* Test-only bridge to the same Zig policy implementation used by p4port. */
enum { WHEEL_PERSONA = 5 };
int p4pc_test_policy_set_ratings(const uint8_t *, size_t);
void p4pc_test_policy_reset(void);
void p4pc_test_choose_persona(uint32_t);
uint16_t p4pc_test_policy_wheel(void);
uint16_t p4pc_test_policy_menu_index(void);
void p4pc_test_rating(uint32_t, uint32_t, int8_t);

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
    assert(p4pc_test_policy_set_ratings(payload, sizeof payload));
    p4pc_test_policy_reset();
    p4pc_test_choose_persona(actor);
    assert(p4pc_test_policy_wheel() == WHEEL_PERSONA && p4pc_test_policy_menu_index() == 1);
    /* A tie keeps the current Persona; an unavailable command never switches. */
    p4pc_test_rating(1, 2, 1);
    p4pc_test_policy_reset();
    p4pc_test_choose_persona(actor);
    assert(p4pc_test_policy_wheel() == 0);
    p4pc_test_rating(1, 2, 2);
    available = 0;
    p4pc_test_choose_persona(actor);
    assert(p4pc_test_policy_wheel() == 0);
    return 0;
}
