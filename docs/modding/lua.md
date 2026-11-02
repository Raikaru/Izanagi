# Luau hooks, memory and events

The scripting language is the pinned **Luau 0.744** interpreter and source
compiler, vendored with its MIT licence. There is no Lua 5.4 compatibility layer,
JIT, CodeGen or native module loading. An enabled mod may contain `main.luau`;
data-only mods need no script. Source is limited to 1 MiB; precompiled bytecode is
not accepted. Each mod owns its VM, globals, allocator, scratch and subscriptions.
Installation never enables a mod. [File mods](files.md) describes discovery,
API-version checks, global ordering and the restart-required selection boundary.

The bundled [`p4pc.example`](../../examples/mods/p4pc.example) is inert until
copied to the configured mod root and explicitly enabled. It hooks the actual
time-slot setter, reads and writes a small private numeric guest layout, and
logs its counter. It snapshots a plain number into its required save-sidecar
block and restores that number into fresh guest scratch when a save is loaded.

`mod_example_game[64|64w]` is an opt-in, real-executable witness, registered
only with `P4PC_MOD_WITNESS_TESTS=ON`. Supply `P4PC_ISO`, `P4PC_ELF` and a
private-readable `P4PC_EXAMPLE_SEED` containing the story0415 card; run it
headlessly through local study admission with `P4PC_INPUT=script`,
`P4PC_MOD_WITNESS=1`, `P4PC_MODS=1` and `P4PC_PACKS=0`. It copies the seed,
drives the actual time-slot hook and two game saves, reloads in a fresh process,
and checks the authenticated sidecar, required provider and restored counter. Outputs are
tagged non-reference witness artifacts; this is not a retail comparison.

## Sandbox, budgets and lifetime

The sandbox retains Luau's base, coroutine, table, string, math, utf8, bit32 and
buffer facilities, including normal `pcall`, `xpcall`, environments and coroutines.
There is no `os`, `io`, `debug`, `package`, `require`, source/bytecode loader,
filesystem library, `collectgarbage`, vector/integer library or native loader.
`p4.log(message)` emits one namespaced diagnostic, at most 4096 bytes. Public `p4`
namespace tables and native handle metatables are read-only.

VM allocation is capped at 8 MiB per mod. Initialization has 65,536
safepoints and a callback has 4096. Callbacks additionally share 8192 units
per presentation frame per mod: each callback entry and each safepoint consumes
one. These deterministic interpreter budgets are not millisecond CPU deadlines
and cannot preempt a native C builtin.
Budget failure is sticky across script `pcall` and coroutine boundaries.

Callbacks run on the native game thread. Each native coroutine incarnation gets
its own protected Luau callback stack. A native original that schedules another
game coroutine must not cause the second callback to reuse a suspended stack.
Native work is excluded from interpreter CPU accounting, and returning from a
native call restores the correct interpreter segment. Synchronous native service
callbacks prohibit `p4.call` and continuations rather than permitting an unsafe
native yield.

Lua executes under its own nearest-rounding floating-point environment. Typed
native calls restore the caller's game environment; native changes to that
environment survive the eventual return to the game, including error paths.
An escaping init/hook/event error preserves its bounded diagnostic for the native
recovery screen. It never invents a successful game result or silently unloads
queued work; the two loading/failure paths are described below.

A mod admits **128 lifetime hook/event subscriptions**. Removed native handles
are tombstones until shutdown, so removing a subscription does not replenish
that limit. Disable/re-enable existing handles instead of allocating one each
frame. Removal is safe from its own callback; registration during native dispatch
is refused. Store reusable buffers and subscriptions in module globals.

This removes direct host filesystem and native-code facilities. It does **not**
make arbitrary calls into an unsafe C game safe for hostile code. Enable only
trusted mods. Public addresses are guest addresses, never host pointers.

## Typed hooks at retail addresses

```luau
-- Choose a researched address from this build's generated catalog.
local info = assert(p4.describe(address))
p4.log(info.signature)
local observer = p4.hook(info.address, info.signature, "after", function(ctx)
    p4.log(string.format("returned from %x", ctx.address))
end)
observer:disable()
observer:enable()
observer:remove()
```

`p4.describe(address)` returns nil for an unsupported address. The generated
`gen/mod_hooks.tsv` records supported signatures and exclusions; do not infer a
function name or ABI from an address spelling or stale decomp alias. Signatures
have a result letter followed by at most 12 parameter letters:

| Letter | Type |
| --- | --- |
| `v` | void result |
| `b`, `B` | signed/unsigned 8-bit |
| `h`, `H` | signed/unsigned 16-bit |
| `i`, `I` | signed/unsigned 32-bit |
| `q`, `Q` | signed/unsigned 64-bit, represented by an eight-byte LE buffer |
| `f`, `d` | float32/float64 |
| `p` | 32-bit guest data address; zero is NULL |

Variadics, by-value aggregates, unknown aliases, function-pointer declarators and
target-dependent scalar widths such as `size_t` and `long` are not guessed.
An incompatible signature fails before registration or native
invocation. Before hooks execute in global package order; after hooks execute in
reverse. A second active replacement is a reported conflict, not last-writer-wins.

```luau
local replacement = p4.hook(address, signature, "replace", function(ctx)
    ctx.args[1] = adjusted_argument
    ctx.next() -- original at most once; updates ctx.result
    ctx.result = adjusted_result
end)
local value = p4.call(address, signature, argument) -- original, bypasses root hooks
```

`ctx.next()` is valid only inside the replacement's current native coroutine and
callback. Retaining a context does not retain a live native stack frame; stale,
cross-coroutine and repeated continuations are refused. Recursive calls made by
the original still observe hooks. `p4.call` does not establish that a selected
operation is pure, legal at this lifecycle point, or free of RNG/cost/turn effects.
Never invoke a damage/accuracy calculator merely to draw a preview.

The generator instruments final mirrored definitions **after** overrides and ABI
corrections. Named calls, aliases and guest function pointers reach that same
entry, so a hook wraps a hand-translated override. Inactive entries perform only
a predictable enabled load/branch, without allocation, lookup, VM entry or argument
marshalling. Active typed bridges are separate. No executable patching, JIT or
platform-specific detour is involved.
Bridge invokers honor the same forced CRT aliases as the mirrored unit: a
retail function such as EE `kill` reaches its `func_00421fb8` body, not a host
process API with the same spelling.
The table generator inspects the compiled bridge as a project archive, not a
host library; system-library names remain subject to the existing host guard.
The standalone i386/x64 task comparison links the real bridge and dispatch
objects, retaining only its reachable task hooks with linker section GC.

## Guest memory, function words and declared layouts

```luau
local scratch = p4.mem.alloc(256)
local a = scratch.address
p4.mem.write("u32", a, 123)
assert(p4.mem.read("u32", a) == 123)
local bits = p4.u64(0x89abcdef, 0x01234567)
p4.mem.write("u64", a + 8, bits)
local low, high = p4.split64(p4.mem.read("u64", a + 8))
```

Integer helpers are explicitly little-endian and permit unaligned accesses.
32-bit and narrower values are range-checked Luau numbers; 64-bit values are
exact eight-byte buffers, **not** doubles, signed host integers or decimal strings.
Use Luau's `buffer` operations for binary codecs. `f32/f64`, bounded strings and
byte buffers are also supported. String writes reject embedded NUL and require
capacity for the terminator. All stores validate the complete payload before
changing any bytes.

[Memory policy](memory.md) describes the source-backed write allowlist, executable
and readonly exclusions, real stack/debug-header protection and function-word
classification. Ordinary writable spans require actual image/heap evidence; a
numeric address inside the old RAM window is not authorization. Function cells
accept only nil or a `p4.fn(address)` reference to a known native dispatch entry,
not arbitrary integers. Declared function cells cannot be partially overwritten
through a scalar/string/byte store.

Scratch allocations are zeroed, 1 byte..1 MiB each and at most 4 MiB per mod.
On translated guest models they use the real guarded guest arena; exhaustion is
an error, never a host-pointer fallback. `scratch:release()` revokes that handle.
A declared view holds its own lifetime pin, whereas a naked numeric address does
not retain memory. Other mods cannot use private scratch through memory helpers.

```luau
local Record = p4.mem.layout<<{flags: number, id: number, skills: P4GuestArray<number>}>>({
    name = "ResearchedRecord",
    size = 0x30,
    fields = {
        flags = {offset = 0, type = "u16"},
        id = {offset = 2, type = "u16"},
        skills = {offset = 0x0c, type = "u16", count = 8},
    },
})
local record = Record:view(guest_address, true) -- read-only
p4.log(tostring(record.skills[1]))
```

In strict Analysis, supply the view type explicitly as above: the layout
specification describes runtime fields but cannot infer the generic view type.

Layouts validate bounded field extents, fixed array counts, nested layouts and
`type="string", capacity=N` declarations. Read-only restrictions propagate into
arrays and nested views. Out-of-range array reads return nil; writes and unknown
fields are refused. Layouts pin authorized memory but do not invent native-object
lifetime or relocate fixed retail tables. Internal pin/read/write closures are
not public memory methods.

## Winning VFS reads and delivered input

`p4.files.read(path[, maximum])` returns a Luau **buffer** containing the winning
read-only VFS bytes, bounded by at most 4 MiB. It neither reads host paths nor
bypasses existing developer/mod ordering. The VFS must be mounted. Resource
formats belong in mods using bounded buffer codecs, not private feature-specific
native parsers.

`p4.input.action(name)` returns `down, pressed, released` from the final delivered
game pad packet, after keyboard/gamepad/touch mapping and input fences. Names are
`confirm` (Cross), `cancel` (Circle), `menu` (Triangle), `help` (Square), directions,
`start/select/l1/r1/l2/r2/l3/r3` and the four face-button names.
`p4.input.sticks()` returns left x/y, right x/y, normalized about neutral 128.
Sampling activates on demand; getters never pump host events or resample devices.

## Native Config pages

The game installs the real `p4.ui` provider before source admission. It registers
pages with the existing native Config backend and attaches them under the owning
mod in the **single MODS hub**. It does not add per-mod permanent root rows, draw
an overlay/toolkit, impersonate native SERVICE pages or force a field/title menu.
Core-only VM fixtures intentionally have no UI provider; they never stub a native
settings implementation to make an API test pass.

`p4.ui.page(spec)` requires `key`, `title`, `count()`, `row(index)` and `find(key)`.
Lua row indexes are **one-based**; `find` returns nil when a stable key was
removed. Counts are bounded by the native backend's1048576-row limit.
Rows provide `key`, `label`, `kind` (`info/toggle/value/action/link`) and optional
`value/help/checked/disabled/restart/current`. A link supplies the registered
target's global page key. Toggle/value/action rows require `adjust(key,direction)`
with direction -1/+1 for changes or0 for activation; info/link-only pages may
omit it. Call `page:refresh()` after observable row membership/order/value changes
to advance the native snapshot revision, rather than acting on a stale index.
Optional `opened()`/`closed()` callbacks follow actual native view lifetime.

`p4.ui.detail{key,title,detail=function() return body end}` uses the same native
font's full-body wrapping/paging. It cannot mix row/adjust callbacks. Both
constructors return a readonly page handle with its globally namespaced `key`,
`:open()` and `:refresh()`. Local keys use1–59 ASCII letters/digits/underscore/
hyphen/dot; the global identity is `lua/<real-mod-id>/<local-key>`.
`:open()` returns whether the asynchronous request was accepted, **not**
visibility. `p4.ui.active()` returns the visible registered page key or nil;
`p4.ui.back()`/`close()` return accepted/refused status through the existing menu
gates. A refresh during shutdown returns false.

Text is the native font's **RAW encoding**, not UTF-8, MSG/BMD commands or host
HTML. English ASCII captions follow the existing native settings convention.
Bounds include NUL: page/global row keys128, labels/titles192, values128,
help2048 and detail32768 bytes. Embedded NUL and oversized values are errors;
captions are single-line and help/detail may use the page layer's LF separators.
Native validation visibly refuses malformed/unrenderable fields.

All provider callbacks are synchronous game-thread protected invocations with
the ordinary 4096-safepoint and per-frame limits. Native calls/continuations
that could yield are prohibited, as is registering a new screen from a
synchronous provider callback. Create screens during initialization or an
ordinary game event instead. Errors enter the same controlled native recovery
as hook/event errors, not an invented successful widget result.

The VM privately roots page userdata, callback snapshots and the last returned
row/detail, even if a script drops its public handle. Native code copies borrowed
strings before another provider call. On a proven native teardown boundary the
binding unlinks the owner's hub entries first, successfully unregisters every
owned page outside provider callbacks, then releases Lua roots before destroying
the VM. Failure to establish that lifetime enters controlled recovery, never a
fake unregister or a GC-owned native view. Selection changes remain restart-only;
there is no script-side module-unload shortcut.

The authored [`native_ui` fixture](../../tests/mod_fixtures/native_ui/main.luau)
uses the public API for toggles, stable-key actions, links and a long detail page.
For an admitted mod-test game run, select `witness.native-ui` from the
`tests/mod_fixtures` root with the explicit witness configuration and navigate the
real hub using scripted native input. Its `UI_READY/OPEN/ADJUST/ACTION/CLOSE`
markers are callback evidence only; actual native rendering/input/scroll/paging
still require labelled platform receipts. The Analysis CLI checks this same
fixture source. Source/fixture definitions are not passing game/visual receipts.

The UI flags are presentation metadata, not automatic persistence:
`restart` does not write configuration or request a restart. This page provider
does not expose general host INI/file writes or rich in-game message commands.

The fixture's explicit `Fail callback witness` action throws `UI_CALLBACK_BAD`
inside the real adjust provider. Use it only in the isolated mod-test lane to
exercise native recovery with a live UI callback; the scripted recovery choice
and resulting persisted selector change require their own actual receipts.

## Semantic events

`p4.events.on(name, callback)` returns an enable/disable/remove subscription.
Payloads contain copied scalars; guest addresses do not retain native objects.
Routes use typed native hooks and remain inactive without subscribers. A missing
required native seam refuses activation rather than fabricating an event.
The pinned Analysis checker resolves literal event-name overloads; branch on a
union of names before registering so each callback receives its specific payload.

| Name | Native seam | Payload |
| --- | --- | --- |
| `frame_start` | completed scheduler VSync/device boundary, before guest updates resume | frame |
| `field_leave` | ready field becomes pending/not-ready or root destruction: 0x155360 before/after, 0x155bf0 before | task, field, room, date, period |
| `field_enter` | 0x155360 returns ready state 12..14, pending clear, requested ids match; once per visit | same |
| `battle_start` | 0x192e90 returns initialized task | task, setup, battle |
| `battle_end` | 0x192cd0 before release | battle, outcome (0 unset/cancelled, 1 victory, 2 defeat, 3 no party) |
| `menu_open/close` | native Camp/Config root creation/destruction: 0x12d000/0x12cc50, 0x35c690/0x35e820 | task, kind (0 Camp, 1 Config) |
| `calendar_advance` | completed date/time setters 0x106100/0x1062f0, actual changes only | old_date, old_period, date, period |
| `save` | 0x10d7c0 returns finalized checksummed image | version, data, size |
| `load` | 0x10e710 finishes applying accepted sections, before field handover | version, data, size |

The save event does not prove disk publication. Post-image notifications do not
authorize save provenance or pre-commit restrictions such as Ironman.

## Save provenance

Compilation, subscription/page registration and writes to mod-owned scratch do
not tag saves. Actual hook/event/native UI callback execution, permitted writes
to game-owned guest memory and actual native calls record the VM's pinned,
selected provider kind/id/version. This ordinary semantic-use history is
process-sticky across load/New Game; installed or retired metadata is not source
authority. The generic file-use consumer uses the one actual VFS dispatcher;
CPU preparation and validated cosmetic-only reads do not become semantic uses.

Normal player modded saves do not acquire MOD_WITNESS custody. With those mods
later disabled, the native loader lists missing versions and optional blocks
and permits explicit continuation when safe; required bindings remain strict.
Only actual witness custody refuses playback outside the separate witness lane.

Each mod may register one `p4.save.register{schema, max_bytes, snapshot, load}`
provider. `schema` is a positive 32-bit version and `max_bytes` is 9 through
1,048,576, including the value encoding. `snapshot()` returns nil to omit the
block, or a plain number, string, boolean or nested table with boolean, number
or string keys. Tables must not have metatables or cycles; functions, userdata
and handles are not serializable. Snapshots run without native calls or guest
mutation, and encoding or budget errors refuse the save before publication.

An emitted block requires the exact selected provider/version. Loading stages
and validates all blocks before publication; the commit is an infallible native
swap, with no Lua execution. After commit, `load(value)` receives the decoded
value, or nil when no block is present, including a vanilla save or New Game.
Use `snapshot` to read mod-owned Lua state and `load` to rebuild any fresh
scratch. A post-commit callback failure enters normal mod failure recovery; it
does not retroactively undo the accepted game state.


## Native lifetime and shutdown

Loading has two passes. **Before any mod code runs**, the shared admission
transaction checks manifests, API versions, native capabilities/conflicts and
compiles/loads every enabled source into a retained function. It does not run
in-game Analysis: typechecking belongs to the modder CLI/luau-lsp, and runtime
type errors follow the runtime path below. Refusal displays a clear native
message, quarantines that mod's raw files and typed resources, and continues
with valid siblings. Native prepares roll back before a filtered generation is
published; surviving compiled chunks are retained, not compiled again.

The complete CPU decode/validation of mandatory resources/table sources also
precedes raw exposure. Raw replacements publish early; only genuine runtime
GPU/guest-bank/UI binding remains at the real0010d480 completion boundary.
The first mod chunk executes only AFTER that late transaction publishes. A
late failure before Lua still uses post-exposure recovery, not early rollback.


**After exposure**, init/callback errors and budget failures cannot invent a
game return value or pretend queued work was rolled back. The native recovery
owner shows `Mod X failed: … The game must restart`, with `Restart without X`
and `Quit`. One atomic INI write removes X from `mods.enabled` and records
`mods.recovery_mod`/`mods.recovery_choice`, preserving other keys/comments.
Unattributed failures never guess a package/native owner: **Restart with all mods
disabled / Quit** explicitly offers safe mode. Safe mode atomically clears BOTH
mod and pack selectors and records its cause/next-boot notice; Quit leaves those
selections unchanged. Two unclean pre-stable startups offer the same recovery.
An actual other settings-directory owner offers explicit instance-only mods-off
continuation without changing stored selectors/history.
An unwritable INI refuses automatic restart and explains manual repair; a
failed platform restart displays manual-relaunch instructions before controlled
exit. Successful restart goes through `p4_os_request_restart` and ordinary
drained exit, never same-process SDL/game/VM reentry.

A real native task/queue quiescence boundary may own safe teardown. The current
restart-required path keeps VM/provider pins until process shutdown when that
proof is unavailable; no automatic hot-unload/exit cleanup invents one.
The separate `P4PC_MOD_WITNESS=1` lane never reads physical modal input:
`P4PC_MOD_RECOVERY_CHOICE=restart|quit` supplies its scripted choice (default
Quit), and labelled error/message receipts replace interactive acknowledgement.
These noninteractive policy witnesses are not native-dialog rendering receipts.

The native hook/event idle queries describe their actual callback stacks only.
Bulk owner removal and aggregate clear fail closed during dispatch; they never
silently return while a caller frees callback userdata. VM teardown also refuses
active native-context invocation frames. It first tombstones an owner's
callbacks, then destroys its VM while those handles remain valid for userdata
finalizers. Aggregate event/hook clear comes **after all VMs/handle clients**,
followed by memory-policy teardown. Package retirement comes after consumers'
real quiescence/pin release, not after a mask/count heuristic.

Native death witnesses invoke aggregate clear and owner removal from actual
hook/event callbacks and require the explicit refusal. These are unit-witness
definitions, not passing platform/gameplay receipts.

`P4PC_MOD_WITNESS_TESTS=ON` explicitly registers the separate
`mod_loader_recovery[64|64w]` CTests (label `mod-witness`); the option is OFF for
default retail gates. Enable the actual native link profile for every selected
ABI and run this label only through an admitted, isolated mod-test lane.
The driver uses real provider reads, retained compiled functions, the real VM,
native recovery backend, atomic INI writes and the OS fresh-image restart API,
with authored synthetic mods. It checks compile/version/capability/duplicate
refusals before any script-side subscription exists, unchanged survivor source
read counts, removal of the bad raw VFS layer, init/runtime-type/budget errors,
event/hook callback errors, both persisted choices and a fresh entry with the
bad mod absent. Its scripted acknowledgement is explicitly labelled non-retail;
it does not qualify physical dialog rendering or game task/queue unload safety.

## Types and qualification

[`include/p4pc/p4.d.luau`](../../include/p4pc/p4.d.luau) is the shipped public type
surface. The pinned Analysis CLI loads it after builtin definitions and checks
both accepted API usage and rejected unavailable globals. Script `--!nocheck`
can opt out; static checking is developer assistance, not sandbox enforcement.

The VM witness uses real shared manifests/provider pins, the interpreter and the
cooperative scheduler, with explicit synthetic retail descriptors and explicit
unavailable-image/heap inputs. Unregistered raw metadata cannot create a mod VM.
It is not a retail gameplay witness. Required integration qualification separately
covers no-mod state/RNG/walk identity on i386/x64 and unhooked frame overhead on
`story_to_0415`, bathhouse and haze: paired turbo wall time and paced frame CPU
must remain within measured run-to-run noise. The inactive byte/branch cost and
hooked-call dispatch budget are explicit in [budgets](budgets.md#hook-cost-acceptance),
not described as zero cost or an unobserved timing result.
Source implementation or a configured target is not a passing platform receipt.

The real no-loaded-mod identity driver is `mod_identity.py`, a private study tool that is not
part of this repository. It builds
separately from game launches and owns actual admission per build/native case:
Windows uses CPU/native `Machine` cases; POSIX runs on the HOST through
`--container-command` with the accepted `p4pc-run` and identical absolute bind
paths, one container invocation per build or game. See the
[admission example](budgets.md#real-frame-windows-and-paired-guard-study).
There is no claimed `P4PC_SCOPE=mods-no-load` route or acceptance from source alone.
It builds entry guards and Luau together OFF/ON on **both i386 and x64**,
with fixed zero initialization and both selectors forced off. The shared
package/VFS plumbing remains in both builds. It runs the existing real
`logic64_init.py` checkpoint/walk cohort with private seed-card copies, not
synthetic game state.

Its report binds each actual game binary and CMake cache by SHA-256. Per case it
compares every emitted native lockstep state-word/RNG/walk stream byte, the
independent walk digests, all 624 MT state words and cursor, and the existing
pointer-free endpoint semantics. A missing/empty native stream or a failed
native walk fails the check. It retains the real logs, snapshots and stream
files under its caller-owned build root. Run it only through land/study
admission; `P4PC_MOD_WITNESS` is always refused, including with empty selectors.
The separate frame-overhead/platform qualification above remains required.
The real counter and paired `mod_guard_timing.py` study contract (private tooling) are
documented in [budgets](budgets.md#real-frame-windows-and-paired-guard-study);
their source/fixtures are not unobserved timing receipts.
