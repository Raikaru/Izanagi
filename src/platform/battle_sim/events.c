/* Native committed events. No Luau, package enablement, allocation or RNG. */
#include "p4pc/battle_events.h"
#include <stddef.h>

uint32_t p4_battle_observer_interest;
static struct {
    uint32_t token;
    P4BattleObserver observer;
} subscribers[8];
static unsigned extent, delivering;
static uint32_t last_token;

static void refresh_interest(void)
{
    uint32_t interest = 0;
    while (extent && !subscribers[extent - 1].token) extent--;
    for (unsigned i = 0; i < extent; i++) {
        if (!subscribers[i].token) continue;
        const P4BattleObserver *o = &subscribers[i].observer;
        if (o->admission) interest |= P4_BATTLE_OBSERVE_ADMISSION;
        if (o->cost) interest |= P4_BATTLE_OBSERVE_COST;
        if (o->hit) interest |= P4_BATTLE_OBSERVE_HIT;
        if (o->command) interest |= P4_BATTLE_OBSERVE_COMMAND;
        if (o->request) interest |= P4_BATTLE_OBSERVE_REQUEST;
    }
    p4_battle_observer_interest = interest;
}

uint32_t p4_battle_observer_add(const P4BattleObserver *observer)
{
    if (!observer || delivering || last_token == UINT32_MAX ||
        (!observer->admission && !observer->cost && !observer->hit && !observer->command && !observer->request)) return 0;
    unsigned slot = 0;
    while (slot < sizeof subscribers / sizeof *subscribers && subscribers[slot].token) slot++;
    if (slot == sizeof subscribers / sizeof *subscribers) return 0;
    subscribers[slot].observer = *observer;
    subscribers[slot].token = ++last_token;
    if (extent <= slot) extent = slot + 1;
    refresh_interest();
    return subscribers[slot].token;
}

int p4_battle_observer_remove(uint32_t token)
{
    if (!token || delivering) return 0;
    for (unsigned i = 0; i < extent; i++) {
        if (subscribers[i].token != token) continue;
        subscribers[i].token = 0;
        subscribers[i].observer = (P4BattleObserver){0};
        refresh_interest();
        return 1;
    }
    return 0;
}

void p4_battle_publish_admission(const P4BattleAdmission *event)
{
    delivering++;
    for (unsigned i = 0; i < extent; i++) {
        const P4BattleObserver *o = &subscribers[i].observer;
        if (subscribers[i].token && o->admission) o->admission(event, o->context);
    }
    delivering--;
}

void p4_battle_publish_cost(const P4BattleCost *event)
{
    delivering++;
    for (unsigned i = 0; i < extent; i++) {
        const P4BattleObserver *o = &subscribers[i].observer;
        if (subscribers[i].token && o->cost) o->cost(event, o->context);
    }
    delivering--;
}

void p4_battle_publish_hit(const P4BattleHit *event)
{
    delivering++;
    for (unsigned i = 0; i < extent; i++) {
        const P4BattleObserver *o = &subscribers[i].observer;
        if (subscribers[i].token && o->hit) o->hit(event, o->context);
    }
    delivering--;
}

void p4_battle_publish_command(const P4BattleCommand *event)
{
    delivering++;
    for (unsigned i = 0; i < extent; i++) {
        const P4BattleObserver *o = &subscribers[i].observer;
        if (subscribers[i].token && o->command) o->command(event, o->context);
    }
    delivering--;
}

void p4_battle_publish_request(const P4BattleRequest *event)
{
    delivering++;
    for (unsigned i = 0; i < extent; i++) {
        const P4BattleObserver *o = &subscribers[i].observer;
        if (subscribers[i].token && o->request) o->request(event, o->context);
    }
    delivering--;
}
