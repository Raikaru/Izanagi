-- Production hardmode policy on synthetic immutable execution facts/native
-- status results. Real VM/reducer delivery is tested by mod_luau_execution.
-- No game, no retail bytes, no substitute RNG or guessed damage callbacks.
local F = FIXTURE
local save = F.save
F.rows = {
    [310] = {affinities = {}}, [311] = {affinities = {}},
    [258] = {affinities = {}}, [259] = {affinities = {}},
}
F.settings.battle_retry = true
save.load(nil, "reset")
save.load(nil, "load")
assert((save.snapshot() :: any).technical)
local A = table.freeze({uid = 0x60000001, id = 311, side = "enemy", data = 0x4000, action = 0x5000})
local B = table.freeze({uid = 0x60000002, id = 311, side = "enemy", data = 0x6000, action = 0x7000})
local epoch, serial = 0, 0
local before = F.hookPhases[0x00238940].before
local after = F.hookPhases[0x00238940].after
assert(before and after and #F.executionHandlers == 1)
local SKILLS = 0x10000
F.memMap[0x7644a8] = SKILLS -- gp b3b8, the native 0x28-byte skill table
F.memMap[SKILLS + 20 * 0x28 + 0x18] = 1 -- inflicted status, not a cure
F.memMap[SKILLS + 21 * 0x28 + 0x18] = 2 -- removal-mask path, guaranteed by stock
local function begin()
    epoch += 1
    F.executionUnits = {[A.data] = A, [B.data] = B}
    F.memMap[A.data], F.memMap[B.data] = 4, 4
    F.memMap[A.data + 2], F.memMap[B.data + 2] = 311, 311
    F.memMap[A.data + 8], F.memMap[B.data + 8] = 100, 100
    F.memMap[A.data + 0xC], F.memMap[B.data + 0xC] = 0, 0
    F.fireExecution(table.freeze({kind = "battle_start", epoch = string.format("%016x", epoch), battle_active = true, reason = 0}))
end
local function deliver(kind: string, x: any?, reason: number?)
    F.fireExecution(table.freeze({kind = kind, epoch = string.format("%016x", epoch), battle_active = true,
        reason = reason or 0, execution = if x then table.freeze(table.clone(x)) else nil}))
end
local function admit(actor: any, base: boolean?): any
    serial += 1
    local x = {token = string.format("%016x", serial), action_uid = "0000000000000099", actor = actor,
        base_turn = base ~= false, command = 0, skill = 0, committed = false, pending_packets = 0,
        result_packets = 0, uncertainty = 0, direct_end = false, end_seen = false, sealed = false,
        base_boundary = false, outcome = "running"}
    deliver("admitted", x)
    return x
end
local function commit(x: any, skill: number, command: number?)
    x.command, x.skill, x.committed = command or 3, skill, true
    deliver("committed", x)
end
local function finish(x: any, outcome: string?)
    x.outcome = outcome or "resolved"
    x.sealed, x.end_seen, x.pending_packets = true, true, 0
    x.result_packets = if x.outcome == "resolved" then 1 else 0
    deliver("terminal", x)
end
local function attempt(actor: any, rate: number, blocked: boolean)
    local calls = #F.callLog
    local context = {args = {20, 0x9000, actor.data, 0}, result = nil :: any}
    before(context)
    if rate < 100 then
        local factor = F.chanceSpec.fn({skill = 20, target_side = "enemy", target_id = 311,
            component = "ailment", statuses = 4, stock = rate})
        assert(factor == (if blocked then 0 else 30 / rate), "wrong rolled Fear rate")
    end -- Stock >=100 never invokes the chance stage.
    context.result = 0x05 -- A synthetic successful native result, Fear + another leaf.
    after(context)
    assert(context.result == (if blocked then 0x01 else 0x05), "Fear veto/baseline or unrelated leaf changed")
    assert(#F.callLog == calls, "window policy must not add native calls/RNG")
end
local function both(actor: any, blocked: boolean)
    attempt(actor, 40, blocked)
    attempt(actor, 100, blocked)
end
local function charge(actor: any)
    local x = admit(actor)
    both(actor, true) -- closed before synchronous initializer/committed choice
    commit(x, 227)
    finish(x, "hitless") -- self buff; not a shot, must not discharge the guard
end
local function shot(actor: any)
    local x = admit(actor)
    commit(x, 149)
    both(actor, true)
    finish(x)
end
local function fact(name: string)
    F.fire("frame_start", {}) -- drain the existing bounded snapshot diagnostics
    emit("CASE " .. name .. " PASS")
end

begin()
both(A, false) -- no invented initial immunity/extra two-base-turn bound
charge(A)
both(A, true)
both(B, false) -- same enemy ID, different authoritative UID
-- Intermediate supports (the guard is skill-independent), extra/One More,
-- and arbitrarily many base turns cannot time out the charge span.
for i = 1, 6 do
    local x = admit(A, i > 2)
    commit(x, 218) -- source-backed Tarukaja; all non-shot supports preserve it
    finish(x)
    both(A, true)
end
shot(A)
both(A, false)
fact("charge_support_extra_shot")
fact("same_enemy_distinct_uid")

charge(A)
local alias = admit(A); commit(alias, 149, 8); finish(alias)
both(A, true) -- an unrelated command's numeric field is not a Shot
fact("non_skill_alias_keeps_guard")
local hitlessShot = admit(A); commit(hitlessShot, 149); finish(hitlessShot, "hitless")
both(A, false) -- a clean completed execution needs no landed-hit assertion
fact("hitless_shot_completion")

F.memMap[SKILLS + 20 * 0x28 + 0x18] = 3
both(A, false); charge(A); both(A, true); shot(A); both(A, false)
F.memMap[SKILLS + 20 * 0x28 + 0x18] = 1
fact("kind3_infliction")
for _, guarded in {false, true} do
    if guarded then charge(A) end
    local cure = {args = {21, 0x9000, A.data, 0}, result = 4}
    before(cure); after(cure); assert(cure.result == 4, "guard prolonged Fear by blocking a cure")
    F.executionUnits[A.data] = nil -- cures do not need attack attribution
    before(cure); after(cure); assert(cure.result == 4, "unattributed cure must keep baseline")
    F.executionUnits[A.data] = A
    both(A, guarded)
    if guarded then shot(A) end
end
fact("guarded_unguarded_cures_unchanged")

-- The generic landing/recovery/one-per-phase rules remain exactly the existing
-- windowTurn rules. Extra/One More do not age Fear, ordinary base admissions do.
F.memMap[B.data + 0xC] = 4
local b1 = admit(B); commit(b1, 218); finish(b1)
assert(F.memMap[B.data + 0xC] == 4)
local extra = admit(B, false); commit(extra, 218); finish(extra)
assert(F.memMap[B.data + 0xC] == 4, "extra action aged the ailment")
local b2 = admit(B); commit(b2, 218); finish(b2)
assert(F.memMap[B.data + 0xC] == 0, "second base admission failed generic recovery")
both(B, true) -- same phase was used; no new wear-off roll or immunity counter
both(A, false)
fact("generic_base_turn_phase_cap")

-- Every terminal cancellation fails to baseline, clears the unreliable shot,
-- and cannot open on an unrelated later completion. Only a NEW proven shot can.
for _, outcome in {"direct_end", "removed", "flushed", "battle_release", "replaced", "unproven"} do
    begin(); charge(A)
    local x = admit(A); commit(x, 149)
    if outcome == "direct_end" then x.direct_end = true end
    if outcome == "unproven" then x.uncertainty = 1 end
    finish(x, outcome)
    both(A, true)
    local unrelated = admit(A); commit(unrelated, 218); finish(unrelated)
    both(A, true)
    if outcome ~= "battle_release" then shot(A); both(A, false) end
    fact("baseline_" .. outcome)
end

for _, field in {"sealed", "end_seen", "committed", "pending_packets"} do
    begin(); charge(A)
    local incomplete = admit(A); commit(incomplete, 149)
    incomplete.outcome, incomplete.sealed, incomplete.end_seen = "hitless", true, true
    incomplete[field] = if field == "pending_packets" then 1 else false
    deliver("terminal", incomplete)
    both(A, true)
    incomplete[field] = if field == "pending_packets" then 0 else true
    deliver("terminal", incomplete) -- no resurrection of the abandoned sequence
    both(A, true)
    shot(A); both(A, false)
end
fact("baseline_incomplete_completion")


begin(); charge(A)
local dead = admit(A, false)
dead.direct_end = true
F.memMap[A.data + 8], F.memMap[A.data + 0xC] = 0, 0x80000
finish(dead, "direct_end")
both(A, true)
fact("death_baseline")
for _, loss in {{"unknown_work", 1}, {"capacity", 2}, {"orphan_resume", 4}, {"conflict", 8}} do
    begin(); both(A, false)
    deliver("lost", nil, loss[2] :: number)
    both(A, true) -- loss with NO running execution must close the idle window too
    both(B, true) -- previously unseen instance inherits the fail-baseline latch
    begin(); both(A, false) -- clean native lifetime, not a guessed timeout
    fact("baseline_" .. (loss[1] :: string))
end

begin(); charge(A)
local uncertain = admit(A); commit(uncertain, 149)
uncertain.uncertainty = 8; deliver("uncertain", uncertain)
both(A, true)
finish(uncertain)
both(A, true) -- stale uncertain shot cannot release the latch
shot(A); both(A, false)
fact("baseline_uncertain_command")

begin(); both(A, false)
F.executionUnits[A.data] = nil
both(A, true)
F.executionUnits[A.data] = A
both(A, true)
shot(A); both(A, false)
fact("baseline_missing_identity")

begin(); both(A, false)
assert(F.chanceSpec.fn({skill = 20, target_side = "enemy", target_id = 311,
    component = "ailment", statuses = 4, stock = 40}) == 0)
both(A, true)
fact("baseline_missing_context")

begin(); both(A, false)
after({args = {20, 0x9000, A.data, 0}, result = 4})
both(A, true)
fact("baseline_broken_pairing")

begin(); both(A, false)
epoch += 1 -- missing native lifetime event is not a new, trusted battle
local stale = admit(A); commit(stale, 218); finish(stale)
both(A, true)
fact("baseline_epoch")

begin(); both(A, false)
local recycled = table.freeze({uid = 0x60000003, id = 311, side = "enemy", data = A.data, action = A.action})
F.executionUnits[A.data] = recycled
both(recycled, true)
fact("baseline_recycled_data")

begin(); both(A, false)
local moved = table.freeze({uid = A.uid, id = 311, side = "enemy", data = 0xa000, action = A.action})
F.executionUnits[moved.data] = moved
F.memMap[moved.data], F.memMap[moved.data + 2] = 4, 311
both(moved, true)
fact("baseline_changed_data")

begin(); both(A, false)
admit(table.freeze({uid = 0x60000004, id = 311, side = "enemy", data = 0, action = A.action}))
both(A, true)
fact("baseline_missing_actor_data")

-- A paired/nested non-Gigas status query cannot supply an instance for Gigas
-- or have its own result changed. The outer authoritative target is restored.
begin(); both(A, false)
local outer = {args = {20, 0x9000, A.data, 0}, result = 4}
before(outer)
local inner = {args = {21, 0x9000, 0x8000, 0}, result = 5}
F.memMap[0x8000], F.memMap[0x8002] = 4, 310
before(inner); after(inner); assert(inner.result == 5)
after(outer); assert(outer.result == 4)
fact("nested_unrelated_unchanged")

begin(); both(A, false)
local ca = {args = {20, 0x9000, A.data, 0}, result = 4}
local cb = {args = {20, 0x9000, B.data, 0}, result = 4}
before(ca); before(cb); after(ca); after(cb)
assert(ca.result == 0 and cb.result == 0)
both(A, true); both(B, true)
fact("baseline_out_of_order")

begin(); both(A, false)
local deep = {args = {20, 0x9000, A.data, 0}, result = 4}
for _ = 1, 65 do before(deep) end
for _ = 1, 65 do deep.result = 4; after(deep); assert(deep.result == 0) end
both(A, true)
fact("baseline_status_capacity")

-- The existing native retry answer clears fight-local policy even when the
-- restored enemy group/DatUnit address is reused. No old charge/shot survives.
begin(); charge(A)
F.memMap[0x764504] = 0x3000
F.memMap[0x3000 + 812 * 0x18] = 2
local defeat = {name = "defeat", encounter = 812, source = "field", polls = 0, allowed = true,
    reason = "", party = {}}
assert(F.defeat(defeat) == "hold")
F.input.confirm = true; defeat.polls = 32
assert(F.defeat(defeat) == "retry")
F.input.confirm = nil
both(A, false)
fact("retry_resets_span")

begin(); charge(A)
F.fireExecution(table.freeze({kind = "battle_end", epoch = string.format("%016x", epoch), battle_active = false, reason = 0}))
begin(); both(A, false)
fact("battle_end_resets_span")

-- Outside a run: both rolled and guaranteed Fear keep baseline immunity.
save.load(nil, "reset")
F.settings.mode, F.settings.adopt_on_load = "Off", false
save.load(nil, "load")
begin(); both(A, true)
fact("off_baseline")
local abandons = 0
for _, line in F.logs do if string.find(line, "HARDMODE_GIGAS abandon", 1, true) then abandons += 1 end end
assert(abandons >= 16, "fail-baseline paths need witness diagnostics")
emit("mod_hardmode_gigas: PASS")
