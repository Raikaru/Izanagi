/* Tests the C broker only; these synthetic packets are not combat witnesses. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include "p4pc/battle_events.h"

typedef struct Seen {
    unsigned admissions, costs, hits, commands;
    uint32_t token;
    P4BattleAdmission admission;
    P4BattleCost cost;
    P4BattleHit hit;
    P4BattleCommand command;
    unsigned requests;
    P4BattleRequest request;
} Seen;

static void admission(const P4BattleAdmission *event, void *context)
{
    Seen *seen = context;
    seen->admissions++;
    seen->admission = *event;
}
static void cost(const P4BattleCost *event, void *context)
{
    Seen *seen = context;
    seen->costs++;
    seen->cost = *event;
}
static void command(const P4BattleCommand *event, void *context)
{
    Seen *seen = context;
    seen->commands++;
    seen->command = *event;
}
static void hit(const P4BattleHit *event, void *context)
{
    Seen *seen = context;
    seen->hits++;
    seen->hit = *event;
    const P4BattleObserver attempted = {context, admission, cost, hit, command, 0};
    assert(!p4_battle_observer_add(&attempted));
    assert(!p4_battle_observer_remove(seen->token));
}
static void request(const P4BattleRequest *event, void *context)
{
    Seen *seen = context;
    seen->requests++;
    seen->request = *event;
    assert(!p4_battle_observer_remove(seen->token));
}


int main(void)
{
    Seen first = {0}, second = {0};
    const P4BattleObserver all = {&first, admission, cost, hit, command, 0};
    const P4BattleObserver hits = {&second, 0, 0, hit, 0, 0};
    const P4BattleObserver empty = {0};
    assert(!p4_battle_observer_interest);
    assert(!p4_battle_observer_add(0));
    assert(!p4_battle_observer_add(&empty));
    first.token = p4_battle_observer_add(&all);
    second.token = p4_battle_observer_add(&hits);
    assert(first.token && second.token && first.token != second.token);
    assert(p4_battle_observer_interest == 15);

    P4BattleAdmission a = {0};
    a.actor.action_uid = UINT64_C(0x12345678fedcba98);
    a.actor.unit_uid = 123;
    a.actor.native_id = 1;
    a.actor.present = 1;
    a.native_action_count = 4;
    a.next_state = 5;
    a.queue_kind = P4_BATTLE_QUEUE_ORDINARY;
    p4_battle_publish_admission(&a);
    assert(first.admissions == 1 && !second.admissions);
    assert(first.admission.actor.action_uid == a.actor.action_uid);
    assert(first.admission.native_action_count == 4);
    assert(first.admission.queue_kind == P4_BATTLE_QUEUE_ORDINARY);

    P4BattleCost c = {0};
    c.actor = a.actor;
    c.kind = P4_BATTLE_COST_SP;
    c.requested_cost = 4;
    c.skill_id = 10;
    c.hp_before = c.hp_after = 999;
    c.sp_before = 999;
    c.sp_after = 128;
    p4_battle_publish_cost(&c);
    assert(first.costs == 1 && !second.costs);
    assert(first.cost.requested_cost == 4);
    assert(first.cost.sp_before == 999 && first.cost.sp_after == 128);
    assert(first.cost.requested_cost != first.cost.sp_before - first.cost.sp_after);

    P4BattleHit h = {0};
    h.source = h.target = a.actor;
    h.hp_before = h.sp_before = 999;
    h.hp_after = 207;
    h.sp_after = 128;
    h.result_flags = 0x80;
    h.resources_applied = 1;
    p4_battle_publish_hit(&h);
    assert(first.hits == 1 && second.hits == 1);
    assert(first.hit.target.action_uid == a.actor.action_uid);
    assert(!first.hit.requested_hp_delta && !first.hit.requested_sp_delta);
    assert(first.hit.hp_before == 999 && first.hit.hp_after == 207);
    assert(second.hit.sp_before == 999 && second.hit.sp_after == 128);
    assert(first.hit.resources_applied && first.hit.result_flags == 0x80);

    P4BattleCommand command_event = {0};
    command_event.actor = a.actor;
    command_event.action_kind = 7;
    command_event.target_count = 1;
    command_event.native_state = 0x19;
    p4_battle_publish_command(&command_event);
    assert(first.commands == 1 && !second.commands);
    assert(first.command.actor.action_uid == a.actor.action_uid);
    assert(first.command.action_kind == 7 && !first.command.free_action);
    command_event.action_kind = 5;
    command_event.free_action = 1;
    p4_battle_publish_command(&command_event);
    assert(first.commands == 2 && first.command.free_action);

    uint32_t tokens[6];
    for (unsigned i = 0; i < 6; i++) {
        tokens[i] = p4_battle_observer_add(&all);
        assert(tokens[i]);
    }
    assert(!p4_battle_observer_add(&all));
    uint32_t old = first.token;
    assert(p4_battle_observer_remove(old));
    assert(!p4_battle_observer_remove(old));
    first.token = p4_battle_observer_add(&all);
    assert(first.token && first.token != old);
    assert(!p4_battle_observer_remove(old));
    assert(p4_battle_observer_remove(first.token));
    for (unsigned i = 0; i < 6; i++) assert(p4_battle_observer_remove(tokens[i]));
    assert(p4_battle_observer_interest == P4_BATTLE_OBSERVE_HIT);
    assert(p4_battle_observer_remove(second.token));
    assert(!p4_battle_observer_interest);
    assert(!p4_battle_observer_remove(0));
    const P4BattleObserver requests = {&first, 0, 0, 0, 0, request};
    first.token = p4_battle_observer_add(&requests);
    assert(first.token && p4_battle_observer_interest == P4_BATTLE_OBSERVE_REQUEST);
    P4BattleRequest event = {P4_BATTLE_SCRIPT_REQUEST_BEGIN, 0, 0, 0};
    p4_battle_publish_request(&event);
    assert(first.requests == 1 && first.request.stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN);
    event.stage = P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED;
    event.encounter_id = 0xfedc;
    p4_battle_publish_request(&event);
    assert(first.requests == 2 && first.request.encounter_id == 0xfedc);
    event.stage = P4_BATTLE_FIELD_REQUEST_BEGIN;
    event.encounter_id = 0x1234fedc;
    event.guest_object = 0x007abc00;
    event.value = 0xabcd1234;
    p4_battle_publish_request(&event);
    assert(first.requests == 3 && first.request.encounter_id == 0x1234fedc);
    assert(first.request.guest_object == 0x007abc00 && first.request.value == 0xabcd1234);
    event.stage = P4_BATTLE_TASK_CREATED;
    event.guest_object = 0x007abd00;
    event.value = 0x007abe00;
    p4_battle_publish_request(&event);
    assert(first.requests == 4 && first.request.stage == P4_BATTLE_TASK_CREATED);
    assert(first.request.guest_object == 0x007abd00 && first.request.value == 0x007abe00);
    event.stage = P4_BATTLE_FIELD_GROUP_CREATED;
    event.guest_object = 0x007abc00;
    event.value = 0x007abf00;
    p4_battle_publish_request(&event);
    assert(first.requests == 5 && first.request.stage == P4_BATTLE_FIELD_GROUP_CREATED);
    assert(first.request.guest_object == 0x007abc00 && first.request.value == 0x007abf00);
    event.stage = P4_BATTLE_SCRIPT_GROUP_CREATED;
    event.guest_object = 0x007ac000;
    event.value = 0;
    p4_battle_publish_request(&event);
    assert(first.requests == 6 && first.request.stage == P4_BATTLE_SCRIPT_GROUP_CREATED);
    assert(first.request.guest_object == 0x007ac000 && !first.request.value);
    event.stage = P4_BATTLE_COLLISION_READY;
    event.encounter_id = 123;
    event.guest_object = 0x007abc00;
    event.value = 0x007abd00;
    p4_battle_publish_request(&event);
    assert(first.requests == 7 && first.request.stage == P4_BATTLE_COLLISION_READY);
    assert(first.request.encounter_id == 123 && first.request.guest_object == 0x007abc00 &&
           first.request.value == 0x007abd00);
    assert(p4_battle_observer_remove(first.token));
    assert(!p4_battle_observer_interest);
    return 0;
}
