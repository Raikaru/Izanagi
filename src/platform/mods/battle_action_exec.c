#include "p4pc/battle_action_exec.h"
#include <stddef.h>
#include <string.h>

/* Fixed side records: no guest padding, pointer-width assumptions or engine
 * ownership changes. Terminal records remain queryable until slot reuse. */
typedef struct Record { P4ActionExec value; unsigned scopes; } Record;
typedef struct Packet { uint32_t address; uint64_t owner; } Packet;
static Record records[P4_EXEC_RECORDS];
static Packet packets[P4_EXEC_PACKETS];
static unsigned packet_count, unowned, notifying, releasing, battle_uncertainty;
static uint64_t serial, current;
static P4ActionExecObserver sink;
static void *sink_context;
unsigned p4_action_exec_enabled;
unsigned p4_action_exec_battle_active;
uint64_t p4_action_exec_battle_epoch;

static Record *by_token(uint64_t token)
{
    if (token)
        for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
            if (records[i].value.token == token) return &records[i];
    return NULL;
}
static Record *by_action(uint32_t action)
{
    Record *found = NULL;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i) {
        Record *r = &records[i];
        if (r->value.token && r->value.action == action &&
            r->value.outcome == P4_EXEC_RUNNING && (!found || r->value.token > found->value.token))
            found = r;
    }
    return found;
}
static void emit(unsigned event, Record *r)
{
    ++notifying;
    sink(event, &r->value, sink_context);
    --notifying;
}
static void marker(unsigned event, unsigned reason)
{
    Record value = {.value = {.uncertainty = reason}};
    emit(event, &value);
}
static void cancel(Record *r, unsigned outcome)
{
    if (r && r->value.token && r->value.outcome == P4_EXEC_RUNNING) {
        r->value.outcome = (uint8_t)outcome;
        emit(P4_EXEC_TERMINAL, r);
    }
}
static void mark_uncertain(Record *r, unsigned reason)
{
    if (r->value.outcome == P4_EXEC_RUNNING && (r->value.uncertainty & reason) != reason) {
        r->value.uncertainty |= reason;
        emit(P4_EXEC_UNCERTAIN, r);
    }
}
static void taint(unsigned reason)
{
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
        if (records[i].value.outcome == P4_EXEC_RUNNING && records[i].value.token)
            mark_uncertain(&records[i], reason);
    marker(P4_EXEC_LOST, reason);
}
static void settle(Record *r)
{
    if (!r || r->value.outcome != P4_EXEC_RUNNING || !r->value.sealed ||
        r->value.pending_packets || r->scopes) return;
    r->value.outcome = r->value.uncertainty ? P4_EXEC_UNPROVEN :
        r->value.direct_end ? P4_EXEC_DIRECT_END :
        r->value.result_packets ? P4_EXEC_RESOLVED : P4_EXEC_HITLESS;
    emit(P4_EXEC_TERMINAL, r);
}
int p4_action_exec_observe(P4ActionExecObserver observer, void *context)
{
    if (notifying || current || packet_count) return 0;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
        if (records[i].scopes || (records[i].value.token && records[i].value.outcome == P4_EXEC_RUNNING))
            return 0;
    memset(records, 0, sizeof records);
    sink = observer;
    sink_context = context;
    p4_action_exec_enabled = observer != NULL;
    unowned = releasing = battle_uncertainty = 0;
    return 1;
}
int p4_action_exec_snapshot(uint64_t token, P4ActionExec *out)
{
    Record *r = by_token(token);
    if (!r || !out) return 0;
    *out = r->value;
    return 1;
}
uint64_t p4_action_exec_current(uint32_t action)
{
    Record *r = by_action(action);
    return r ? r->value.token : 0;
}
uint64_t p4_action_exec_admit(uint32_t action, uint64_t uid, uint32_t unit_uid,
                            uint16_t id, uint8_t side, uint16_t flags,
                            uint8_t one_more, uint8_t extra, uint32_t unit_data)
{
    if (!p4_action_exec_enabled) return 0;
    Record *old = by_action(action);
    if (old && !old->value.sealed) cancel(old, P4_EXEC_REPLACED);
    Record *r = NULL;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
        if (!records[i].value.token || (records[i].value.outcome != P4_EXEC_RUNNING &&
            !records[i].value.pending_packets && !records[i].scopes)) { r = &records[i]; break; }
    if (!r || serial == UINT64_MAX || releasing) {
        battle_uncertainty |= P4_EXEC_CAPACITY;
        taint(P4_EXEC_CAPACITY);
        return 0;
    }
    *r = (Record){ .value = {
        .token = ++serial, .action_uid = uid & UINT64_C(0x3fffffffffffffff),
        .action = action, .unit_uid = unit_uid, .unit_data = unit_data, .native_id = id, .side = side,
        .base_turn = !one_more && !extra && !(flags & 0x8004),
        .direct_end = (flags & 0x8000) != 0,
        .uncertainty = battle_uncertainty | (unowned ? P4_EXEC_UNKNOWN_WORK : 0)
    }};
    emit(P4_EXEC_ADMITTED, r);
    return r->value.token;
}
void p4_action_exec_resume(uint32_t action)
{
    Record *r = by_action(action);
    if (p4_action_exec_enabled && (!r || r->value.sealed)) {
        battle_uncertainty |= P4_EXEC_ORPHAN_RESUME;
        taint(P4_EXEC_ORPHAN_RESUME);
    }
}
void p4_action_exec_command(uint32_t action, uint16_t command, uint16_t skill)
{
    Record *r = by_action(action);
    if (!r) return;
    /* Persona/Analyze are free subcommands, not the final execution choice. */
    if (command == 5 || command == 10) return;
    if (r->value.committed) mark_uncertain(r, P4_EXEC_CONFLICT);
    r->value.command = command;
    r->value.skill = skill;
    r->value.committed = 1;
    emit(P4_EXEC_COMMITTED, r);
}
void p4_action_exec_end_seen(uint32_t action, int direct_end)
{
    Record *r = by_action(action);
    if (r) {
        r->value.end_seen = 1;
        r->value.direct_end |= direct_end != 0;
    }
}
void p4_action_exec_end_processed(uint32_t action, int restart)
{
    Record *r = by_action(action);
    if (!r || restart) return;
    if (!r->value.end_seen) mark_uncertain(r, P4_EXEC_CONFLICT);
    r->value.end_processed = 1;
}
void p4_action_exec_standby(uint32_t action)
{
    Record *r = by_action(action);
    if (!r || !r->value.end_processed) return;
    r->value.sealed = 1;
    settle(r);
}
void p4_action_exec_base_boundary(uint32_t action)
{
    Record *r = by_action(action);
    if (r && !r->value.base_boundary && !r->value.direct_end) {
        r->value.base_boundary = 1;
        emit(P4_EXEC_BASE_BOUNDARY, r);
    }
}
void p4_action_exec_remove(uint32_t action)
{
    if (!p4_action_exec_enabled) return;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
        if (records[i].value.action == action)
            cancel(&records[i], releasing ? P4_EXEC_BATTLE_RELEASE : P4_EXEC_REMOVED);
}
void p4_action_exec_flush(void)
{
    if (!p4_action_exec_enabled) return;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i)
        cancel(&records[i], releasing ? P4_EXEC_BATTLE_RELEASE : P4_EXEC_FLUSHED);
    if (!releasing) marker(P4_EXEC_LOST, P4_EXEC_UNKNOWN_WORK);
}
void p4_action_exec_release(void)
{
    p4_action_exec_battle_active = 0;
    if (!p4_action_exec_enabled) return;
    releasing = 1;
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i) cancel(&records[i], P4_EXEC_BATTLE_RELEASE);
    marker(P4_EXEC_BATTLE_END, 0);
}
void p4_action_exec_battle_begin(void)
{
    p4_action_exec_battle_active = 1;
    ++p4_action_exec_battle_epoch;
    if (!p4_action_exec_enabled) return;
    if (packet_count || current) {
        battle_uncertainty |= P4_EXEC_CONFLICT;
        taint(P4_EXEC_CONFLICT);
        return;
    }
    for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i) cancel(&records[i], P4_EXEC_UNPROVEN);
    memset(records, 0, sizeof records);
    unowned = releasing = battle_uncertainty = 0;
    marker(P4_EXEC_BATTLE_BEGIN, 0);
}
static uint64_t enter(uint64_t token)
{
    uint64_t previous = current;
    current = token;
    Record *r = by_token(current);
    if (r) ++r->scopes;
    return previous;
}
uint64_t p4_action_exec_enter_action(uint32_t action)
{
    if (!p4_action_exec_enabled) return 0;
    /* A target's nested initializer still belongs to its source execution. */
    return enter(current ? current : p4_action_exec_current(action));
}
uint64_t p4_action_exec_enter_packet(uint32_t packet)
{
    if (!p4_action_exec_enabled) return 0;
    for (unsigned i = 0; i < packet_count; ++i)
        if (packets[i].address == packet) return enter(packets[i].owner);
    taint(P4_EXEC_UNKNOWN_WORK);
    return enter(0);
}
void p4_action_exec_leave(uint64_t previous)
{
    if (!p4_action_exec_enabled) return;
    Record *r = by_token(current);
    if (r && r->scopes) --r->scopes;
    current = previous;
    settle(r);
}
void p4_action_exec_packet_add(uint32_t packet, uint64_t uid, uint32_t type)
{
    if (!p4_action_exec_enabled) return;
    if (packet_count == P4_EXEC_PACKETS) {
        battle_uncertainty |= P4_EXEC_CAPACITY;
        taint(P4_EXEC_CAPACITY);
        return;
    }
    for (unsigned i = 0; i < packet_count; ++i)
        if (packets[i].address == packet) {
            battle_uncertainty |= P4_EXEC_CONFLICT;
            taint(P4_EXEC_CONFLICT);
            return;
        }
    Record *owner = by_token(current);
    uid &= UINT64_C(0x3fffffffffffffff);
    if (!owner && uid) {
        for (unsigned i = 0; i < P4_EXEC_RECORDS; ++i) {
            Record *r = &records[i];
            if (!r->value.token || r->value.action_uid != uid || r->value.outcome != P4_EXEC_RUNNING) continue;
            if (owner) { owner = NULL; break; } /* same UID, different executions: ambiguous */
            owner = r;
        }
    }
    if (owner && uid && uid != owner->value.action_uid) {
        mark_uncertain(owner, P4_EXEC_CONFLICT);
        owner = NULL;
    }
    packets[packet_count++] = (Packet){packet, owner ? owner->value.token : 0};
    if (owner) {
        ++owner->value.pending_packets;
        if (type == 0x700) ++owner->value.result_packets;
    } else {
        ++unowned;
        taint(P4_EXEC_UNKNOWN_WORK);
    }
}
void p4_action_exec_packet_remove(uint32_t packet, int completed)
{
    if (!p4_action_exec_enabled) return;
    for (unsigned i = 0; i < packet_count; ++i) {
        if (packets[i].address != packet) continue;
        Record *r = by_token(packets[i].owner);
        if (r) {
            if (!completed) cancel(r, releasing ? P4_EXEC_BATTLE_RELEASE : P4_EXEC_REMOVED);
            --r->value.pending_packets;
        } else if (!packets[i].owner) --unowned;
        packets[i] = packets[--packet_count];
        settle(r);
        return;
    }
    taint(P4_EXEC_UNKNOWN_WORK);
}
