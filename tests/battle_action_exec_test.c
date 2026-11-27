/* Synthetic sequence witnesses, not a retail battle or a matching-C claim. */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "guest_test.h"
#include "p4pc/battle_action_exec_native.h"

static P4ActionExec last;
static unsigned admissions, commits, terminals, boundaries;
static unsigned markers[P4_EXEC_LOST + 1];
static unsigned effects;
static uint32_t live_packets[P4_EXEC_PACKETS + 8];
static unsigned live_count;
static void observe(unsigned event, const P4ActionExec *e, void *context)
{
    assert(context == &last);
    assert(!p4_action_exec_observe(NULL, NULL));
    if (!e->token) { ++markers[event]; return; }
    last = *e;
    if (event == P4_EXEC_ADMITTED) ++admissions;
    if (event == P4_EXEC_COMMITTED) ++commits;
    if (event == P4_EXEC_TERMINAL) ++terminals;
    if (event == P4_EXEC_BASE_BOUNDARY) ++boundaries;
}
static P4ActionExec snap(uint64_t token)
{
    P4ActionExec out;
    assert(p4_action_exec_snapshot(token, &out));
    return out;
}
static void add(uint32_t address, uint64_t uid, uint32_t type)
{
    live_packets[live_count++] = address;
    p4_action_exec_packet_add(address, uid, type);
}
static void remove_packet(uint32_t address, int completed)
{
    for (unsigned i = 0; i < live_count; ++i)
        if (live_packets[i] == address) {
            live_packets[i] = live_packets[--live_count];
            p4_action_exec_packet_remove(address, completed);
            return;
        }
    assert(0);
}
static void reset(void)
{
    p4_action_exec_release();
    while (live_count) remove_packet(live_packets[0], 0);
    p4_action_exec_battle_begin();
    assert(p4_action_exec_observe(observe, &last));
    admissions = commits = terminals = boundaries = effects = 0;
    memset(markers, 0, sizeof markers);
    memset(&last, 0, sizeof last);
}
static uint64_t admit(uint32_t action, uint64_t uid, unsigned more, unsigned extra)
{
    return p4_action_exec_admit(action, uid, action + 100, 311, 1, 0, more, extra, 0);
}
static void finish(uint32_t action)
{
    uint64_t previous = p4_action_exec_enter_action(action);
    p4_action_exec_end_seen(action, 0);
    p4_action_exec_end_processed(action, 0);
    p4_action_exec_standby(action);
    p4_action_exec_leave(previous);
}
static void identity_and_status(void)
{
    reset();
    uint64_t token = admit(1, UINT64_C(0x100000001), 0, 0);
    assert(token && admissions == 1 && !effects);
    assert(last.base_turn && last.unit_uid == 101 && last.action_uid == UINT64_C(0x100000001));
    for (int i = 0; i < 3; ++i) { /* START -> formation/EVENT -> START; delayed init too */
        uint64_t previous = p4_action_exec_enter_action(1);
        assert(p4_action_exec_current(1) == token);
        ++effects;
        p4_action_exec_leave(previous);
    }
    /* No AI/decider event: a status-directed action reaches READY directly. */
    p4_action_exec_command(1, 1, 106);
    assert(commits == 1 && last.skill == 106 && last.token == token);
    ++effects;
    p4_action_exec_end_seen(1, 0);
    p4_action_exec_end_processed(1, 1); /* active-list continuation, not a new admission */
    p4_action_exec_standby(1);
    p4_action_exec_resume(1);
    assert(p4_action_exec_current(1) == token && !terminals && admissions == 1);
    finish(1);
    assert(terminals == 1 && last.outcome == P4_EXEC_HITLESS);
    uint64_t next = admit(1, UINT64_C(0x100000001), 0, 0);
    assert(next > token); /* same object, native UID and instance; new execution */
    p4_action_exec_remove(1);
}
static void packets_and_home(void)
{
    reset();
    uint64_t token = admit(1, 55, 0, 0);
    uint64_t action_scope = p4_action_exec_enter_action(1);
    add(1000, 55, 0x700);
    add(1001, 0, 0x702); /* untagged authoritative buff clear / summon resource */
    p4_action_exec_leave(action_scope);
    assert(snap(token).pending_packets == 2);
    p4_action_exec_end_seen(1, 0);
    p4_action_exec_end_processed(1, 0);
    remove_packet(1000, 1);
    assert(!terminals); /* END is not completion; ENDHOME may still create work */
    uint64_t scope = p4_action_exec_enter_action(1);
    add(1002, 55, 0x105); /* ENDHOME final rotation before STANDBY */
    p4_action_exec_standby(1);
    p4_action_exec_leave(scope);
    remove_packet(1002, 1);
    assert(!terminals); /* no reliance on flag 0x20 */
    scope = p4_action_exec_enter_packet(1001);
    add(1003, 0, 0x10f); /* descendant from packet update/destroy */
    remove_packet(1001, 1);
    p4_action_exec_leave(scope);
    assert(!terminals && snap(token).pending_packets == 1);
    scope = p4_action_exec_enter_packet(1003);
    remove_packet(1003, 1);
    assert(!terminals); /* callback scope still open; it may enqueue a child */
    p4_action_exec_leave(scope);
    assert(terminals == 1 && last.outcome == P4_EXEC_RESOLVED && last.token == token);
}
static void hitless_direct_and_free(void)
{
    reset();
    uint64_t token = admit(1, 1, 0, 0);
    p4_action_exec_command(1, 5, 0); /* free Persona and Analyze don't commit the execution */
    p4_action_exec_command(1, 10, 0);
    assert(!commits);
    p4_action_exec_command(1, 1, 227);
    finish(1);
    assert(snap(token).outcome == P4_EXEC_HITLESS && last.committed);
    reset();
    token = p4_action_exec_admit(1, 1, 101, 311, 1, 0x8000, 0, 0, 0);
    assert(!last.base_turn);
    finish(1);
    assert(snap(token).outcome == P4_EXEC_DIRECT_END && !last.committed);
    reset();
    token = admit(1, 1, 0, 0);
    p4_action_exec_command(1, 1, 227);
    p4_action_exec_command(1, 1, 228);
    finish(1);
    assert(snap(token).outcome == P4_EXEC_UNPROVEN); /* unexpected replacement after commit */
}
static void cancellations(void)
{
    for (unsigned reason = P4_EXEC_REMOVED; reason <= P4_EXEC_BATTLE_RELEASE; ++reason) {
        reset();
        uint64_t token = admit(1, 1, 0, 0);
        uint64_t scope = p4_action_exec_enter_action(1);
        add(1000, 0, 0x700);
        p4_action_exec_leave(scope);
        if (reason == P4_EXEC_REMOVED) remove_packet(1000, 0); /* no update ran */
        if (reason == P4_EXEC_FLUSHED) p4_action_exec_flush();
        if (reason == P4_EXEC_BATTLE_RELEASE) {
            p4_action_exec_release(); /* precedes action destruction and packet flush */
            p4_action_exec_remove(1);
            p4_action_exec_flush();
        }
        assert(terminals == 1 && snap(token).outcome == reason);
        if (live_count) {
            scope = p4_action_exec_enter_packet(1000);
            add(1001, 0, 0x701); /* destroy callback creates work: cancellation stays terminal */
            remove_packet(1000, 0);
            p4_action_exec_leave(scope);
            remove_packet(1001, 1);
        }
        finish(1);
        assert(terminals == 1);
    }
    reset();
    uint64_t token = admit(1, 1, 0, 0);
    p4_action_exec_remove(1); /* action deletion, even with no packets */
    assert(snap(token).outcome == P4_EXEC_REMOVED);
    reset();
    token = admit(1, 1, 0, 0);
    uint64_t scope = p4_action_exec_enter_action(1);
    add(1000, 1, 0x700);
    p4_action_exec_leave(scope);
    uint64_t next = admit(1, 1, 0, 0);
    assert(next != token && terminals == 1 && last.outcome != P4_EXEC_RESOLVED);
    assert(snap(token).outcome == P4_EXEC_REPLACED);
    scope = p4_action_exec_enter_packet(1000);
    add(1001, 0, 0x702); /* retained old owner, not the new execution of the same action */
    p4_action_exec_leave(scope);
    assert(snap(token).pending_packets == 2 && !snap(next).pending_packets);
    remove_packet(1000, 1); remove_packet(1001, 1);
    finish(1);
}
static void base_turns(void)
{
    reset();
    uint64_t base = admit(1, 1, 0, 0);
    assert(snap(base).base_turn);
    finish(1);
    uint64_t more = admit(1, 1, 1, 0);
    assert(!snap(more).base_turn);
    finish(1);
    uint64_t extra = admit(1, 1, 0, 2);
    assert(!snap(extra).base_turn);
    /* Native END emits this only on the counter-exhaustion/secondary-buff path.
     * The final extra execution closes the base chain, but never starts a base turn. */
    p4_action_exec_base_boundary(1);
    p4_action_exec_base_boundary(1);
    assert(boundaries == 1 && !last.base_turn && last.base_boundary);
    finish(1);
    uint64_t next_base = admit(1, 1, 0, 0);
    assert(snap(next_base).base_turn);
    finish(1);
}
static void fail_closed(void)
{
    reset();
    uint64_t token = admit(1, 1, 0, 0);
    add(1000, 0, 0x702); /* no source callback, no action UID: cannot guess */
    remove_packet(1000, 1);
    finish(1);
    assert(snap(token).outcome == P4_EXEC_UNPROVEN);
    reset();
    p4_action_exec_resume(1); /* enabling midbattle must not mint a fake admission */
    assert(!p4_action_exec_current(1));
    token = admit(1, 1, 0, 0);
    finish(1);
    assert(snap(token).uncertainty & P4_EXEC_ORPHAN_RESUME);
    reset();
    token = admit(1, 1, 0, 0);
    uint64_t scope = p4_action_exec_enter_action(1);
    add(1000, 1, 0x700);
    p4_action_exec_leave(scope);
    finish(1); /* sealed, but old work survives ordinary admission */
    uint64_t next = admit(1, 1, 0, 0);
    assert(next != token && !terminals);
    add(1001, 1, 0x702); /* two live executions share the action UID */
    remove_packet(1001, 1); remove_packet(1000, 1);
    assert(snap(token).outcome == P4_EXEC_UNPROVEN);
    finish(1);
    assert(snap(next).outcome == P4_EXEC_UNPROVEN);
    reset();
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i) assert(admit(i + 1, i + 1, 0, 0));
    assert(!admit(100, 100, 0, 0));
    assert(snap(p4_action_exec_current(1)).uncertainty & P4_EXEC_CAPACITY);
    reset();
    token = admit(1, 1, 0, 0);
    scope = p4_action_exec_enter_action(1);
    for (unsigned i = 0; i <= P4_EXEC_PACKETS; ++i) add(1000 + i, 0, 0x702);
    p4_action_exec_leave(scope);
    assert(snap(token).uncertainty & P4_EXEC_CAPACITY);
}
static void native_adapters(void)
{
    reset();
    uint32_t action = P4_H2G(p4t_galloc(0x458, 16));
    uint32_t unit = P4_H2G(p4t_galloc(0xa70, 16));
    uint32_t packet = P4_H2G(p4t_galloc(0x90, 16));
    uint32_t data = P4_H2G(p4t_galloc(0x200, 16));
    uint32_t battle = P4_H2G(p4t_galloc(0x200, 16));
    assert(!p4t_guest_map(P4_GPADDR(0xb3ac), 4));
    P4_GPDATA(uint32_t, 0xb3ac) = battle;
    P4_G32(battle + 0x174) = action;
    P4_G32(unit + 0xa64) = data;
    P4_G32(action) = 42; P4_G32(action + 4) = 1;
    P4_G32(action + 0x30) = unit;
    P4_G32(unit + 0xa8) = 812;
    P4_G16(unit + 0xa4) = 311; P4_G8(unit + 0xa2) = 1;
    p4_action_exec_native_admit(action);
    uint64_t token = last.token;
    assert(last.action_uid == UINT64_C(0x10000002a) && last.native_id == 311
        && last.unit_uid == 812 && last.unit_data == data);
    P4ActionExecUnit identity;
    assert(p4_action_exec_native_unit(data, &identity));
    assert(identity.action == action && identity.uid == 812 && identity.data == data && identity.id == 311);
    assert(!p4_action_exec_native_unit(data + 1, &identity));
    uint32_t duplicate = P4_H2G(p4t_galloc(0x458, 16));
    P4_G32(action + 0x450) = duplicate;
    P4_G32(duplicate + 0x30) = unit;
    assert(!p4_action_exec_native_unit(data, &identity));
    P4_G32(action + 0x450) = action; /* corrupt/cyclic live list: bounded refusal */
    assert(!p4_action_exec_native_unit(data, &identity));
    P4_G32(action + 0x450) = 0;
    uint64_t scope = p4_action_exec_enter_action(action);
    P4_G16(action + 0x6c) = 1; P4_G16(action + 0x6e) = 227;
    p4_action_exec_native_command(action);
    assert(last.token == token && last.skill == 227 && !effects);
    ++effects; /* synthetic initializer effect, always after the admission/commit */
    P4_G32(packet + 0x40) = 0x700;
    P4_G32(packet + 0x60) = 42; P4_G32(packet + 0x64) = 0xc0000001u;
    p4_action_exec_native_packet_add(packet);
    live_packets[live_count++] = packet;
    P4_G8(packet + 0x47) = 0; /* flag20 is deliberately irrelevant */
    p4_action_exec_native_end(action);
    p4_action_exec_end_processed(action, 0);
    P4_G16(action + 0xc) = 0x22;
    p4_action_exec_native_checkpoint(action);
    assert(!snap(token).sealed);
    P4_G16(action + 0xc) = 1;
    p4_action_exec_native_checkpoint(action);
    p4_action_exec_leave(scope);
    remove_packet(packet, 1);
    assert(terminals == 1 && last.outcome == P4_EXEC_RESOLVED);
    p4_action_exec_release();
    assert(!p4_action_exec_native_unit(data, &identity));
}
/* Consumer witness: persistent charge protection is not recomputed at every
 * skill selection. An unknown/canceled shot must not open a completion window. */
static void gigas_sequence(void)
{
    reset();
    unsigned guarded = 0;
    const uint16_t skills[] = {227, 237, 197, 37}; /* identities are synthetic inputs, not an AI-script witness */
    for (unsigned i = 0; i < 4; ++i) {
        admit(1, 1, 0, 0);
        p4_action_exec_command(1, 1, skills[i]);
        if (!i) guarded = 1;
        uint64_t scope = p4_action_exec_enter_action(1);
        if (i == 3) { add(1000, 0, 0x700); add(1001, 0, 0x702); }
        p4_action_exec_leave(scope);
        finish(1);
        assert(guarded);
        if (i == 3) {
            remove_packet(1000, 1);
            assert(guarded && last.outcome == P4_EXEC_RUNNING);
            remove_packet(1001, 1);
            assert(last.outcome == P4_EXEC_RESOLVED);
            guarded = 0;
        }
    }
    assert(!guarded);
}
static void instance_and_summon_ownership(void)
{
    reset();
    uint64_t parent = admit(1, 71, 0, 0);
    uint64_t scope = p4_action_exec_enter_action(1);
    add(1000, 0, 0x110); /* summon resource packet has no summoner UID */
    p4_action_exec_leave(scope);
    finish(1);
    uint64_t child = admit(2, 72, 0, 0); /* queued summon later acts independently */
    assert(parent != child && snap(parent).native_id == snap(child).native_id);
    assert(snap(parent).unit_uid != snap(child).unit_uid);
    assert(snap(parent).pending_packets == 1 && !snap(child).pending_packets);
    scope = p4_action_exec_enter_packet(1000);
    add(1001, 0, 0x702);
    p4_action_exec_leave(scope);
    remove_packet(1000, 1); remove_packet(1001, 1);
    assert(snap(parent).outcome == P4_EXEC_HITLESS && snap(child).outcome == P4_EXEC_RUNNING);
    finish(2);

    reset();
    add(1000, 0, 0x700); /* work already outstanding at a new admission */
    uint64_t token = admit(1, 1, 0, 0);
    assert(snap(token).uncertainty & P4_EXEC_UNKNOWN_WORK);
    remove_packet(1000, 1);
    finish(1);
    assert(snap(token).outcome == P4_EXEC_UNPROVEN);

    reset();
    token = admit(1, 1, 0, 0);
    add(1000, 0, 0x700);
    assert(last.token == token && last.uncertainty); /* synchronous uncertainty notification */
    remove_packet(1000, 1);
    finish(1);
    assert(last.outcome == P4_EXEC_UNPROVEN);

    reset();
    token = admit(1, 1, 0, 0);
    p4_action_exec_end_seen(1, 1); /* skip acquired after initial admission */
    p4_action_exec_end_processed(1, 0);
    p4_action_exec_standby(1);
    assert(snap(token).outcome == P4_EXEC_DIRECT_END);
}
static void lifetime_and_idle_loss(void)
{
    reset();
    uint64_t epoch = p4_action_exec_battle_epoch;
    p4_action_exec_battle_begin();
    assert(p4_action_exec_battle_active && p4_action_exec_battle_epoch == epoch + 1);
    assert(markers[P4_EXEC_BATTLE_BEGIN] == 1 && !admissions);
    add(1000, 0, 0x700); /* no running execution can carry UNCERTAIN */
    assert(markers[P4_EXEC_LOST] == 1 && !admissions && !terminals);
    remove_packet(1000, 1);
    p4_action_exec_flush();
    assert(markers[P4_EXEC_LOST] == 2);
    p4_action_exec_release();
    assert(!p4_action_exec_battle_active && markers[P4_EXEC_BATTLE_END] == 1);
    p4_action_exec_battle_begin();
    assert(p4_action_exec_battle_epoch == epoch + 2 && markers[P4_EXEC_BATTLE_BEGIN] == 2);
}
int main(void)
{
    if (p4t_guest_init()) return 77;
    assert(!p4_action_exec_enabled);
    uint64_t epoch = p4_action_exec_battle_epoch;
    p4_action_exec_battle_begin(); /* lifetime facts exist even before tracking */
    assert(p4_action_exec_battle_active && p4_action_exec_battle_epoch == epoch + 1);
    p4_action_exec_release();
    assert(!p4_action_exec_battle_active);
    assert(!admit(1, 1, 0, 0));
    identity_and_status(); packets_and_home(); hitless_direct_and_free();
    cancellations(); base_turns(); fail_closed(); native_adapters(); gigas_sequence();
    instance_and_summon_ownership(); lifetime_and_idle_loss();
    reset();
    assert(p4_action_exec_observe(NULL, NULL));
    puts("battle_action_exec: synthetic identity, completion/cancellation, base-turn and Gigas consumer sequences passed");
    return 0;
}
