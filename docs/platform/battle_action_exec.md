# Internal battle execution contract

`include/p4pc/battle_action_exec.h` is the synchronous C observation contract; `battle.execution@1` is its additive Luau view. Neither replaces the combat resolver nor claims a retail battle witness. The binding installs one native sink and fans out to ordered, owner-scoped subscriptions. No observer means no execution tracking, but battle-active/epoch facts still advance so late subscription cannot invent an admission.

## Identity and committed choice

The ordinary branch of `btlActionUpdateQueue` (`001b15b0`) mints a monotonic 64-bit token **before** `btlActionSetState(action, 2)` invokes START's synchronous initializer. The active-list branch resumes its existing token and never mints. Formation/EVENT/END reentry, delayed initializers and status-directed READY do not allocate identities. The token is not the native action UID: `btlActionCreate` (`001b0930`) assigns that UID once per object. Snapshots copy unit UID, native ID, side and DatUnit address, so two enemies with the same ID remain distinct. Tokens do not reset between battles. Missing/expired snapshots are not success; terminal slots may be reused after all retained work is gone.

The READY owner (`001a4800`) publishes command/skill immediately before resolution or execution initialization, after any decision replacement. This also covers status-directed actions that bypass AI. Free Persona/Analyze operations do not commit the execution. An unexpected second committed command marks uncertainty rather than silently replacing a proven choice.

## Completion and cancellation

END entry (`001afa50`) only sets `end_seen`. END update (`001afb50`) must pass its readiness waits and finish its effects. Its `restart=1` preserves the token across the active-list continuation. Otherwise it marks end processing complete; ENDHOME must still reach STANDBY before sealing, because `btlActionEndHomeUpdate` can enqueue a final rotation before entering state 1.

Success requires **all three**: sealed action, zero retained packets, and no open callback scopes. Packet tracking ignores flag `0x20`, including packets the engine's `00193bf0` lookup excludes. Enqueue (`00194590`) records ownership before packet init. All action init/update and packet init/update/condition/destroy callbacks have nested save/restore scopes. Untagged packets created by those callbacks inherit execution ownership; descendants retain their parent's token even if the original action has since been readmitted. Summon's untagged resource-load packet is covered this way. The newly created summon is an independent actor when it later receives its own ordinary admission, not another execution of the summoner.

`RESOLVED` means normal END and all tracked work retired normally, with at least one type-`0x700` result packet submitted. It does **not** mean damage landed. `HITLESS` is the same completed lifecycle with no type-`0x700` submission. A miss, no targets, a non-damaging command and an unavailable target must not be inferred from that counter alone. `DIRECT_END` (START's `0x8000` skip) is distinct and must not consume a charged-hit guard as a successful shot.

| Case | Status and classification | Source seam |
| --- | --- | --- |
| Ordinary/status-directed admission before effects | Closed: new token; synchronous admission | `001b15b0`, READY `001a4800`; status route `btlAction_inferred_001a1450.c:1372-1396` |
| START reentry / active-list resumption | Closed: same token, no base-turn increment | `btlAction.c:141-148`; active branch `001b15b0`; END restart `001afb50` |
| Normal END with retained/unflagged packets | Closed for scoped/uniquely tagged work: `RESOLVED`, only after home/packet/scoped callbacks finish | `001afa50`, `001afb50`, `001b0260`, `00194590`, `00194670` |
| Direct END | Closed: `DIRECT_END`, never a resolved charged hit | START `001a06d0` flag `0x8000` |
| Hitless | Closed: `HITLESS`; no invented hit callback | END/ENDHOME plus zero result submissions |
| Forced packet removal without update | Closed: `REMOVED`, never normal completion | forced-free branch `00194670` |
| Action removal/replacement | Closed: `REMOVED` / `REPLACED`; old packets keep old token | both frees in `001b0a60` / `001b0bf0`; ordinary readmission |
| Flush | Closed as cancellation: `FLUSHED` before any destroy callback | `00194a30` |
| Battle release | Closed as cancellation: `BATTLE_RELEASE` before action destruction, not merely at later flush | `00192cd0` |
| Untagged authoritative packet from action/packet callback, including summon resource load | Closed by callback inheritance, independently of UID/flag20 | summon `001abbb0`; scoped dispatch/enqueue |
| Untagged work outside every scoped callback | **Fail-closed**: `UNKNOWN_WORK`, terminal `UNPROVEN`; missing authoritative producer/causal edge, no guessing by queue head | no global attribution assumption |
| Multiple live executions sharing a UID, packet enqueued outside their scopes | **Fail-closed**: ambiguous ownership taints live executions | native UID is object-lifetime, not execution-lifetime |
| Orphan resumption / late installation / capacity exhaustion / conflicting command | **Fail-closed**: uncertainty or missing token, never fabricated admission or success | 64 execution records, 2048 simultaneous packet records; install before battle creation |
| Work escaping packet/action callbacks through a separate asynchronous subsystem | **Fail-closed for use as a general proof**: no exhaustive authority/lifetime inventory was established; this contract does not certify arbitrary task/resource completion | a future producer needs an explicit causal/lifetime edge before its guard may open |

Cancellation is terminal exactly once and cannot be resurrected by destroy callbacks or a later END. Remaining retained work is still accounted until released. Uncertainty is emitted synchronously as `P4_EXEC_UNCERTAIN`, remains sticky for the affected execution, and cannot be cleared by draining an unattributed packet. Unknown outstanding work also taints newly admitted executions. Capacity/orphan faults remain sticky until a clean battle boundary. Do not enable a window merely because a snapshot is missing or an execution was canceled.

## Base turns

An ordinary admission starts a base turn only when the **pre-initializer** One More byte `+0x28` and extra-action byte `+0x29` are both zero, and neither followup flag `4` nor skip flag `0x8000` is set. Ordinary queue admission by itself is not a base turn. This mirrors START's own ordinary-turn gates (`001a06d0`) without counting START reentries; status-impaired base turns remain admitted turns, not successful attacks.

`P4_EXEC_BASE_BOUNDARY` is a separate native END fact, emitted immediately before `datCalcAdvanceSecondaryBuffs` on the non-special, non-skipped counter-exhaustion/death branch (`001afb50`). It closes the base **chain**, possibly on its last One More/extra execution; it is not an extra base-turn admission or proof that asynchronous effects are finished. Use `base_turn` at admission to count starts, this event to observe the native chain boundary, and terminal outcome for completion. Do not count command events or the battle's `actionCount` as boss turns.

## Luau delivery and hard-mode consumer

`p4.battle.execution.on(callback)` registers outside battle only. Delivery is synchronous, before initializer effects, in subscription order. Native game calls, yielding and subscription changes are forbidden during delivery; Lua policy state and guest memory remain available. Events, execution copies and actors are frozen. `snapshot(token)` and `current(action)` return copies or nil, never fabricated success. Tokens, action UIDs and epochs are 16-digit lowercase hexadecimal strings, not lossy Lua numbers; 32-bit unit UIDs remain exact numbers.

An uncaught listener error or exhausted budget enters the existing nonreturning native mod recovery path, exactly like the synchronous formula stages. It never silently disables a listener and resumes combat on stale policy. Attribution uncertainty is instead a delivered fact that a functioning policy handles by restoring baseline immunity.

`unit(DatUnit)` is a pure live-list identity query, not an engine call. It refuses absent, duplicate, cyclic/overlong and inactive battle lists; it is unavailable in snapshot callbacks. This lets an ordinary native hook bind its actual target to a unit UID before the chance stage (whose snapshot contains only an enemy ID). Disabling/removing a subscription cannot fabricate missed admissions; re-enabling is outside battle only. VM teardown removes only that owner's listeners.

Battle start/end and global `lost` markers have no execution. Loss is delivered even with **no running execution**, so an idle vulnerability window cannot silently remain open. Per-execution uncertainty is sticky. A queued semantic event is not a substitute for this pre-effect service.

Hard mode uses the existing generic Fear/Ice window rules for Daring Gigas (311), unchanged from the other window bosses: the shipped chance, one held base turn then recovery, one window per phase, and the existing Analyze status rows. One More/extra actions do not age the ailment. There is no added two-base-turn window timeout, wear-off roll, immune-turn counter or new fight cap. Only its charge guard is new: admission is provisionally closed before effects, committed Power Charge retains the guard through all supports/followups, and only the exact committed Single Shot (skill 149, not an action-list index) reaching sealed, packet-drained, untainted `RESOLVED` **or `HITLESS`** clears it.

Missing identity/context, uncertainty, direct END and canceled/removed/replaced/flushed/released work **abandon to baseline Fear immunity**, clear unreliable sequence state and log `HARDMODE_GIGAS abandon`. They never open the window or start an arbitrary expiry. A new proven shot can release a per-instance latch; sticky native capacity/orphan uncertainty may prevent such a proof until battle end. A clean hitless Shot is a completed execution, not failed attribution: the guard does not require damage landing or charge consumption. Indeed type-`0x700` counts also include START self-regeneration (`btlAction_inferred_001a0670.c:263`, `btlTarget.c:893`), so they cannot prove an outgoing shot. Retry, load/reset and native battle lifetime boundaries prevent old sequences crossing fights.

`datCalcInflictStatus` (`00238940`) now has a canonical declaration preserving its K&R-promoted s32 ABI and original u16 narrowing. Its complete no-hook i386 unit object was identical before/after on epyc (SHA-256 `2a73a27e93d2276b30a7d4efe6ddb6c1987cf58bbbc35ff4c4d87f01199fd55f`, 71816 bytes; production compile flags except debug-only `-g0`). The standard generator can therefore expose a typed hook. Paired before/after hooks correlate each real target and veto only guarded/baseline Fear in the returned mask before native commit, preserving other leaves. This is essential because stock guaranteed-rate attempts bypass the chance stage. Unguarded guaranteed attempts keep the native result; no custom RNG is introduced.

## Tests

`battle_action_exec` compiles the production reducer and native scalar adapters over synthetic action/packet sequences. It covers identity/reentry/status, packet descendants and home completion, direct/hitless END, all cancellations, base/extra/One More, ambiguous/unowned work, capacity refusal, observer lifetime, guest offsets and the persistent Gigas-style guard. The normal host-test factory supplies 64-bit twins. These tests contain no retail bytes and launch neither a game nor a renderer.

`mod_luau_execution` runs the production binding and reducer on the real VM with synthetic guest actors: capability availability, both namespace-composition orders, collision refusal, immutable facts, exact tokens, pre-effect ordering, synchronous/native-call refusal, snapshot live-read refusal, listener ownership/lifetime and idle loss. `mod_hardmode_gigas` runs the production mod on synthetic execution/status sequences: per-UID separation, charge/support/extra/shot, generic base aging/phase cap, baseline abandonment for each cancellation/loss, guaranteed and normal Fear in guarded/unguarded states, nested unrelated status calls, retry and battle end. These are CPU policy/transport witnesses, not retail AI, balance or gameplay receipts.

`mod_luau_execution_failure` injects an actual listener exception and requires its own child process to reach the registered native recovery owner, never return to combat or silently disable. The Gigas fixture enforces snapshot restrictions (including no `p4.log`/guest reads/native calls); abandon diagnostics reuse the existing bounded technical-note queue and are emitted on the next frame.

The veto is limited to native **infliction kinds 1/3**, following `datCalcStatusEffectResult` (`decomp/src/Main/Battle/Data/datCalc.c:2463,2537`). The same `00238940` function also returns a **removal mask** for kind-2 cures (`:2561-2567,2593-2602`); changing that mask would prolong Fear instead of restoring baseline. Guarded, unguarded and unattributed cures therefore retain their native result, with a dedicated policy witness. Normal/guaranteed infliction and kind-3 infliction still pass the guard.
