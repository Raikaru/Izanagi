# File and data mods (API 1.6; accepts 1.0 to 1.5 metadata)

Mods are opt-in. Installing a directory changes nothing until its id appears in the
player's `p4pc.ini`:

```ini
[mods]
enabled = example.one, example.two
load_order = example.two, example.one
```

By default the installation root is `mods/` beside that INI, on Windows, Linux,
Android and iOS. `[mods] directory` selects another root; `P4PC_MOD_DIR` overrides
it. An explicit directory is interpreted as a host path, relative to the working
directory when not absolute. `P4PC_MODS=0` overrides all configuration and disables
mods; `1` permits the configured enabled list but does not enable installed mods.
Other values are errors. Ordinary scripted/capture/headless runs with enabled
mods refuse startup unless the kill switch is set. The explicit, isolated
mod-test boundary below is the only exception; it does not enable installed mods.
Synthetic unit fixtures stage their own bounded inputs without player settings.

Mods made for this port ship read-only in the game program's own `mods/`
directory (`SDL_GetBasePath()`: beside the Windows exe, `usr/bin/mods` in the
Linux AppImage, `Contents/Resources/mods` in the macOS app; the Android package
ships none). The MODS catalog lists them beside the player's installations and
the enabled list finds them, but installation never enables one and nothing
writes `p4pc.ini` for them. A valid player installation with the same id hides
the shipped copy, both in the catalog and at startup, and is reported: the
MODS entry says "player copy shadows bundled <id> <version>" and stderr gets
one line. Two copies within one root still refuse as duplicates. Nothing is
ever written into the shipped root (settings go to `mod-settings/` beside
`p4pc.ini`; saves, captures and caches to the user's directories), so a
read-only Program Files folder or app bundle works. `P4PC_BUNDLED_MOD_DIR`
replaces the shipped root (empty: none; every CTest sets it empty unless the
fixture names one). Repository sources live in `mods/<id>/`; the packagers
copy every one of them.

Standalone engine packs use `[packs] enabled` independently of the mod selector.
Their default root is `packs/` beside `p4pc.ini`; `packs.directory` changes it
and `P4PC_PACK_DIR` takes precedence. `P4PC_PACKS=0` disables the pack selector,
while `1` permits the configured list; other values refuse startup. Enabled
packs use `pack.ini` and `[pack]` with the same six metadata keys and optional
`files/` layout as mods. They use the shared parser, ordering, refusal and VFS
publication; engine packs cannot contain `main.luau`. Unsupported native
extension sections are refused rather than pretending their artwork or audio
was activated. Both selectors stage before their one common publish.

The MODS catalog discovers installed packs on demand without enabling them.
An absent implicit `packs/` is an empty catalog, but an explicitly configured
missing, invalid, or unreadable root is an error.

## Explicit mod-test witness (never a retail reference)

`P4PC_MOD_WITNESS=1` is reserved for the separate mod-test lane. It requires
`P4PC_INPUT=script` and permits headless/hidden/capture exercise through the real
player loader with an explicit fixture configuration. It never overrides
`P4PC_MODS=0` or `P4PC_PACKS=0`, permits no physical/virtual-input escape, and is
not a way to relabel a fidelity gate.

Every native harness/fixture start calls the one scalar
`p4_mod_fidelity_check_start()` policy: validate witness mode first and fail if
invalid; valid scripted witness1 skips the disabled-selector assertion; ordinary
mode asserts both selectors disabled. This includes menu/settings fixture state0,
field/battle entry, input, goal gates, dump and digest starts. Valid mod-test
outputs remain tainted and never reference-eligible. Strict reference operations
(native reference-MAD/ovdiff, reference-manifest lookup and retail gate admission)
still refuse witness presence **even with empty/zero selectors**. No harness
copies its own validate/skip/assert conditional. Actual retail CI/CTest is not
exempt or relabelled as a mod-test lane.

Canonical Windows/Linux/Android boot adapters label witness receipts with
`run_mode=mod-witness`, `mod_witness=true` and `reference_eligible=false`. A run
marker taints outputs from before launch; final receipts carry real output
hashes and portable per-file mode sidecars. Capture copies, archival and decoded
derivatives retain that taint. The reference-manifest guard rejects witness
outputs even when their bytes match a pinned hash.

Windows witness fetches use a separate labelled destination. Android witnesses
require an owned host `P4PC_MOD_WITNESS_OUT`; capture/save/viewer outputs remain
inside it, and the host-only directory is not forwarded into the app. Use
fixture/device paths for mod configuration, never a player's saved settings.

## Package

```text
mods/
  example-one/
    mod.ini
    files/
      DATA/
        MOD_EXAMPLE.TXT
```

```ini
[mod]
id = example.one
name = Example file replacement
version = 1.0.0
api = 1.0
priority = 0
order = 0
```

All six keys are required, each exactly once, in one `[mod]` section. Unknown
metadata keys are refused. API1.1 extension sections require a registered native
handler; unknown/unavailable sections are diagnosed, never ignored.
Whole-line `#`/`;` comments and surrounding whitespace are allowed. `id` is 1–63 lowercase ASCII letters,
digits, hyphens, underscores or dots (no leading/trailing dot). Name and version
are nonempty bounded strings (127/63 bytes); priority/order are signed 32-bit
decimal integers. The API requirement is `major.minor`: major must equal the
runtime's major and minor must not exceed its minor. Missing selected ids,
duplicate selected ids/directories, incompatible versions and invalid selected
packages refuse activation before any mod layer is registered. Diagnostics name
the manifest or enabled id. Disabled malformed packages are diagnosed but do not
activate or stop a different selected package.

Only directories without symlink/reparse-point indirection are discovered.
`files/` is optional for code/native-content-only packages; a present payload
must be a real directory. Its replacement paths must already be uppercase ASCII,
use normal nested directories, fit VFS key/path bounds, and contain no devices,
version suffixes, symlinks, reparse points or special files. Depth is bounded to
32. Keep installed contents unchanged during play: activation audits a trusted,
read-only installation, not an adversarial changing filesystem.

## Settings (API 1.2)

A mod declares its persistent Config rows as `[setting <key>]` sections:

```ini
[setting backlog]
kind = toggle
default = off
label = DIALOGUE BACKLOG
help = Keeps the last lines of dialogue for review.
group = UI

[setting description_style]
kind = choice
choices = Simple | Exact
default = Simple
label = DESCRIPTION STYLE
apply = live
```

The key is 1–48 bytes of `[a-z0-9]` followed by `[a-z0-9_.-]`. `kind` is `toggle`
(choices `off`/`on`, default `off`) or `choice` (2–8 distinct `|`-separated names
of at most 31 bytes; default the first). `label` is required; `label`, `group`
(at most 63 bytes) and `help` (at most 511 bytes) are printable ASCII.
`apply = restart` freezes the value read at admission until the next start: the
row shows the restart marker and `p4.settings.get` returns that admitted value;
`live` (the default) reads the stored value every time. Unknown fields, invalid
defaults or more than 256 settings refuse the mod at admission, and the API 1.2
requirement is checked there too. Engine packs cannot declare settings.

The MODS hub draws the declared rows on the mod's package page in manifest
order, below PACKAGE DETAILS, with a heading row whenever `group` changes. The
rows work while the mod is disabled, so a player can configure a mod before
enabling it. Values persist in `mod-settings/<mod id>.ini` beside `p4pc.ini`,
one file per mod, written to a temporary file and atomically replaced; the
shared `p4pc.ini` is never written. A missing or invalid stored value reads as
the declared default. With no settings file (`P4PC_CONFIG=none`) values stay at
their defaults and writes are refused. Native consumers read the admitted value
with `p4_mod_setting_boot()` (`include/p4pc/mod_setting_store.h`); scripts use
[`p4.settings`](lua.md#persisted-settings). A mod's own files (spoiler logs,
caches) live beside them in `mod-artifacts/<mod id>/`, written only through
[`p4.artifacts`](lua.md#artifacts-artifacts1) (`artifacts@1`).

A boot section can follow one of the mod's own toggles: `when = <key>` in a
`[table-source]`, `[table-derived]`, `[resource-image]` or `[element-ui]` section makes it take
part in this start only while that toggle's admitted value is `on`; off, the
handler treats the section as absent (an `[element-ui]` gate covers its
`[element-column]` sections). Such sections are sealed for the whole session,
so `when` must name a declared `toggle` with `apply = restart`; an undeclared
key, a choice setting, a live toggle or a repeated `when` refuses the mod at
admission. The value comes from the same admission snapshot as
`p4_mod_setting_boot()` (`include/p4pc/mod_setting_gate.h`). A mod therefore
ships its content enabled-by-toggle without contributing anything while the
toggle is off:

```ini
[setting split]
kind = toggle
label = PHYSICAL SPLIT
apply = restart

[table-source p4pc.physsplit:split]
when = split
file = data/split.xml
format = p4-tables-xml
schema = 1
revision = 1
```

## Requirements (API 1.6)

Capability `manifest.require@1` ([capabilities](capabilities.md)).

A package that only works on top of another declares it, one section per
required package of the same kind (mods require mods, packs require packs):

```ini
[require p4pc.physsplit]
version = 1.0
setting.split = on
```

`version = <major>.<minor>` is mandatory: the required package's own `version`
must have the same major and at least that minor. Comparison needs a numeric
version (`1`, `1.0` or `1.0.3`, optionally followed by a `-` or `+` suffix);
anything else is refused as not comparable. Each `setting.<key> = <value>` row
names a declared [setting](#settings-api-12) of the required mod and one of its
choices (`on`/`off` for toggles). At most 16 requirements per package and 8
setting rows per requirement; unknown fields, a missing version, a bad id, a
package requiring itself or an api below 1.6 refuse the package.

Admission treats requirements as metadata, before any mod code runs. Every
requirement must be enabled and admitted, satisfy the version and hold each
named setting at admission; otherwise the dependent is refused as a whole, with
the reason on stderr and in MODS, for example `Hard mode needs Physical Split
with PHYSICAL SPLIT on`. Refusals cascade: a mod whose requirement was refused
for any reason is refused too, and its message quotes that reason. Requirements
always load before their dependents. The selector's (or manifest) order is kept
otherwise, each kind keeps its own catalog slots, and the members of a
requirement cycle are refused. `p4_package_refusal()` keeps this process's
admission reason per id, so a later refusal can say why. Loading a save checks
the packages the save requires before its missing blocks: a required package
refused at startup is named with that reason (`Save requires mod p4pc.hardmode
version 1.0; p4pc.hardmode was refused at startup: Hard mode needs
p4pc.physsplit 1.0+, which is not enabled`), then a required package that is
not enabled at that version, and only then a missing required save block.

The MODS hub follows the same rules. Enabling a dependent first writes the
settings it requires, then enables every missing requirement in front of it in
one `p4pc.ini` batch; a requirement that is not installed (or installed twice),
too old, missing a named setting, part of a cycle, or wanted with another value by
an enabled mod refuses the change with that message. While a dependent is
enabled, the requirement's ENABLED row reads `REQUIRED BY <NAME>` and its
required setting rows are locked, and a load-order move that would put a
requirement after its dependent is refused. Disabling the dependent only
unlocks; the requirement stays enabled. At run time `p4.settings.set` on a
required key returns `false, "locked: required by <name>"`, and
[`p4.mods.required`](lua.md#persisted-settings) reports the admitted version and
values. Native code uses `p4_package_requirements()`,
`p4_package_version_satisfies()` and `p4_packages_requirer()` from
`include/p4pc/packages.h`.

### Companions (API 1.6)

Capability `manifest.companion@1`.

A package that works better beside another, but also without it, names it as a
companion instead:

```ini
[companion p4pc.hardmode]
version = 1.0
```

A companion section has only the mandatory `version`; setting rows, and naming
the same id as both a requirement and a companion, are refused. If the companion
is enabled and admitted it loads first and must satisfy the version, otherwise
the declaring package is refused (`Fusion arts needs companion Hard mode 1.0+
(enabled: 0.9)`). An absent, disabled or refused companion is fine: nothing is
enabled, locked or refused for it. A cycle through companions refuses its
members like any requirement cycle. The hub never enables or locks a companion,
but puts a newly enabled one in front of the mod that names it, refuses a
load-order move that would put it after that mod, and refuses enabling either
side when the installed versions do not fit. Scripts read what a companion or
requirement publishes with [`p4.contract.read`](lua.md#published-contracts).

## Shared native registry and Config discovery

`include/p4pc/packages.h` is the common C boundary for player mods and engine
packs; it does not require or create a script VM. A native client selects its
manifest filename/header (`mod.ini`/`[mod]` or `pack.ini`/`[pack]`), kind and
whether it supplies file replacements. All clients use one parser. Manifests
are bounded to 64KiB, lines to 511bytes and extension fields to 2048; embedded
NUL, duplicate section/argument/key fields, malformed numbers and incompatible
required versions are refused. Native handlers receive immutable parsed
section/argument/key/value/line records rather than reopening manifests.

Equal-key fields are ordered by their original source line as well, so duplicate
diagnostics name the first repeated declaration, independently of heapsort tie
order. Field identity remains section/argument/key; a different source line
does not make a duplicate valid.

Startup stages **both** independently configured selectors before calling
`p4_package_registry_publish` once. Without an explicit mod order, selected
packages receive global `(priority, order, id)` ordering before raw conflicts or
native preparation. `[mods] load_order` overrides the relative order of listed
enabled mods; unlisted enabled mods follow in their default relative order.
Engine packs retain their default catalog slots. Later mods win overlapping files.
A package's global ordinal is not a VFS layer rank. `P4PackageKind` distinguishes
player mods from engine packs; it does not assert that content is cosmetic.

A selector may explicitly acknowledge a refusal through `P4PackageClient.refuse`.
The player native backend does so with a clear message: an invalid enabled id,
version, duplicate installation or unavailable capability disables that mod and
its raw files/resources, not valid siblings. Clients without an acknowledgement
remain strict and atomic. No raw layer or CPU/semantic product publishes until
the final surviving candidate has completely validated its early inputs.

Handler `preflight` checks descriptors/capabilities only. Client `preflight`
compiles all selected script chunks without running them; in-game typechecking
is not part of this pass. Actual bounded payload/header/dimension/loop validation
remains in native `prepare`, never a pretend metadata qualification. A concrete
prepare refusal uses `p4_package_provider_refuse`, returns without publication,
and rolls back every staged native product before rebuilding the candidate.

Actual raw-file/script claims are derived by the core. Native handlers report
their real semantic/table/cosmetic claims. A cosmetic-only requirement is checked
after **all early** prepares/finalizers, so a label cannot hide a script, raw replacement
or another handler's gameplay claim. Capability claims do not themselves assert
that a particular save was touched; save provenance has a separate lifetime.

`p4_package_installations_scan` is an explicit on-demand, metadata-only catalog
for native Config/Mods screens, including installed-but-disabled packages.
It uses the same parser and returns parsed metadata/fields or a per-directory
diagnostic and source line. Callbacks borrow their records only until return.
It does not prepare handlers, walk payloads, create VMs, hash assets or activate
anything. A valid catalog row is not decoder/payload qualification. There is
a 4096-directory scan bound; exceeding it is an error, not a truncated success.
Empty-enabled startup still performs no discovery.

Native Config uses `p4_mods_configured_installations_scan` for player mods:
the configuration frontend reuses init's exact environment/INI/beside-INI root
resolver, independently of enabled ids and the kill switch. Explicit-root
`p4_mods_installations_scan` remains usable by C-only callers without linking
configuration/SDL. Neither path activates a provider. `mod_configured_catalog`
defines real-INI/default/env-precedence cases, malformed payload/source that
must remain unvisited, unavailable selected ids under the kill switch, and the
explicit missing-root error. These fixture definitions are not execution receipts.

The native MODS page can enable or disable an installed mod and move an enabled
mod earlier or later in load order. Invalid or duplicate IDs cannot be selected.
The two selector keys are written atomically; changes take effect on restart and
do not retire or reboot live clients. Live transactional selector replacement is
not implemented.

A later live transaction must keep actual read-origin kind/generation/ordinal
unchanged. An old ordinal is not a current-generation rank: eligibility must
explicitly resolve the retained provider/backing lineage into the new catalog
using `p4_package_provider_same_source` or an equally explicit immutable backing
token. Missing/changed lineage is ineligible; id/string equality and relabelling
provenance are not substitutes. This is a later transaction acceptance contract,
not an implemented hot-apply path; changed raw semantics remain restart-required.

Harness/fixture owners use the scalar `p4pc/mod_fidelity.h` boundary and call
`p4_mod_fidelity_check_start()` at each start, before fixture/game work. It is the
single witness-validation/ordinary-assert policy described above. Ordinary test
fragments still force **both** `P4PC_MODS=0` and `P4PC_PACKS=0`: either staged
selector fails before catalog publication, even with an empty published catalog.
Strict retail/reference operations separately retain
`p4_packages_assert_disabled()`, which also refuses any witness-mode presence.
Isolated synthetic provider/VM witnesses explicitly stage bounded inputs.

The older player-selector assertion deliberately checks only player mods:
turning off `P4PC_MODS` must not disable an independently selected engine pack
in an ordinary player session.

The contained provider inventory and all VFS projections share one immutable
file index; Config/texture indexing does not add a second recursive walker.
Native clients open only indexed contained regular files through bounded
`open`/`read_at`/`close` handles. Handles and explicit provider references pin
the actual old identity/kind/order/generation through failure or quiescent
retirement. A read observer reports exact bytes actually read; it never resolves
an old read through a current provider string. Native tasks/queues and script
callbacks must quiesce before global retirement. Existing reads retain their
generation until the last handle/reference releases it.

Admission never mutates an old provider's info/order/generation in place. A
filtered candidate gets new registry/provider snapshots and a fresh generation;
unchanged providers borrow their original immutable manifest/index through
tracked storage pins, without another filesystem walk. Surviving compiled VMs
rebind to the new provider only when `p4_package_provider_same_source` proves
the same backing inventory. That identity query does not authorize fresh work
on a retired provider. Old already-issued reads keep their exact old snapshot;
new candidates and retained old storage are charged together until actual free.

Provider retain/release is a **lifetime** pin, not deferred I/O authority.
After an actual bounded native file has been opened (and its payload validated
by its owner), `p4_package_file_plan_acquire` issues an immutable authorization
for that exact indexed relative file, original bound, provider snapshot and OS
object/revision. It requires a live provider and reads no additional payload or
stream. Repeated acquisition from that handle reuses the same charged plan.
An owner may retain/transfer this already-issued lineage to a queued texture job
or music instance, then use `p4_package_file_plan_open` after candidate retirement.
Each independent stream keeps the original Info/generation/global ordinal and
rechecks contained no-follow path components, file identity/revision and bound;
a replaced/changed file is refused, never silently adopted. A fresh provider
open or plan acquisition after retirement is refused. Clone/release of an issued
plan does not authorize another file or a broader package root.

Fresh plan acquisition is a prepare/game-thread operation. After issuance,
workers may retain the plan, open independent streams, read/close those streams
and release their references, including after provider retirement. This explicit
issued-lineage exception needs no VFS/current-provider lookup or game-thread
file I/O; each stream still has one owner. `P4PackageReadObserver` runs inline on
the actual reading thread, including workers. Its callback must be worker-safe
and must not enter Lua, VFS or game state. Install/remove that exclusive observer
only before readers start or after they have quiesced.

Windows preserves full128-bit `FileIdInfo` identities. If that query is
unsupported, downlevel NTFS/FAT-family volumes use their documented complete
64-bit file indexes plus volume/revision information; ReFS is never accepted
with a truncated identifier, and unqualified unsupported volumes are refused.
See Microsoft's [native handle identity contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle).

The common `p4pc/package_memory.h` policy caps **combined owned metadata** at
32 MiB desktop / 16 MiB mobile by default. A native startup owner may configure
the limit before the first successful nonzero reservation. Manifest blobs,
parsed fields, registry/provider/catalog storage, shared indexes, conflict
scratch and managed read handles use exact-size tracked allocations. Temporary
growth reserves the **full replacement** while the old allocation remains
charged; a final index size below the limit does not authorize a larger peak.
The metadata sorts are in-place and constant-stack, avoiding libc `qsort`'s
potential unreserved full-array temporary allocation.

Native resolver owners reserve before allocation, commit on success, cancel on
failure and release only at actual free. `p4_packages_memory_usage()` reports
committed resident charges, outstanding reservations and peak resident+reserved.
Borrowed inventory is not charged twice. Retirement does not reset accounting:
old provider/file pins and native snapshots/queued users retain their charges.
Payload/decode buffers, VM heaps, opaque CRT/OS allocations and baseline thread
stacks are separate metrics, not hidden claims about whole-process RSS.

Verified guest-byte tracking has an explicit startup demand, not a package-count
or Lua-presence heuristic. Native prepares with actual staged table rows or
usable texture entries call `p4_packages_source_tracking_require()`; music-only
pinned-file reads do not inherently need the guest range/hash index. CPU-qualified
native inputs request demand before early file publication. The canonical source
owner seals once before the first native game file consumers and enables its
existing dispatcher for `(dump requested || sealed demand)`. Late GPU binding or
Lua startup is not a retroactive early qualification. A late request is refused.
These helpers allocate nothing,
hash nothing and install no observer; a default startup with no consumer demand
does not acquire tracking work merely because a mod exists.

`package_registry` is a synthetic native fixture definition covering disabled
catalog diagnostics, no-activation scans, global order/conflicts, parsed fields,
cosmetic claim rejection, both-selector fidelity refusal, shared metadata limits,
pre-code provider refusal, raw-layer removal and unchanged pinned old-generation
reads/plans through mandatory prepare failure/candidate renewal. File-plan cases
also define post-retirement cloning/opening, refusal of fresh authority,
same-sized object replacement, revision growth and metadata-allocation failure.
These are fixture definitions, not platform execution or no-mod parity receipts.

## Native boot: early CPU admission, late real binding

There is one handler registry, not another private table/resource registry.
Static C descriptors are installed explicitly by a generated plain-C table:
their owner fragments call `p4_register_package_boot_provider(symbol)` before
`305-package-boot.cmake` (helper035). No constructors, weak-symbol discovery or
environment preview supplies a provider.

`boot_phase` and declared `dependencies` give deterministic order: semantic
TABLES100, RESOURCES200, then UI300, with dependency/lexical order within phases.
Missing/backwards dependencies and cycles are refused before callbacks. Nonzero
boot providers participate even without selected data, returning explicit NULL
inactivity rather than allocating a default service. `sections=NULL` is valid
for composition only; schema owners must have an early payload prepare.

Early `prepare` fully CPU-decodes and validates every mandatory named image and
table source, all versions/refs/source identities/static capabilities and conflicts,
without guest/RW/GPU state or queued game work. Optional `finalize_prepare` joins
CPU products after every prepare. All early checks and Luau compile-only preflight
finish before any raw replacement is exposed. Early refusal shows a native reason
and removes the bad package/files from a fresh immutable surviving candidate;
reverse dependency rollback owns only unpublished partial staging.

Raw files still publish in the loader, BEFORE guest entry. Moving publication to
the later completion seam would incorrectly load retail early assets/eight battle
files instead of replacements. Early semantic definitions freeze stable image
`(owner,key,revision)` references, never process-local icon tokens or claims of
compiled item/skill/text/source-view readiness.

At the real0010d480 boundary after eight packed copies, `runtime_prepare` performs
only actual GPU upload/receiver binding, native family-view materialization and
UI token/model binding from retained validated inputs (no re-read/decode). Real
loaded image/current GL/RW state3 are checked; mount alone does not prove them.
All runtime prepares then runtime finalizers finish before infallible runtime
publication. The existing native completion coordinator prepares in a finalizer
and commits in publication; no second coordinator or double completion.

`p4_package_boot_provide`/`product` exchange one borrowed product per provider
per pass, only with declared prepared dependencies. Early CPU and late runtime
stores are separate: no active/global/cross-pass fallback. Owners keep actual
input/payload/source pins alive. `p4_package_provider_selected` proves exact
current candidate/published membership, including refusal/retirement, unlike
identity-only info/same_source. Strong save-source authority waits for actual
compiled family views, not just the CPU semantic seal.

Late failures are POST-EXPOSURE even before Lua code: file consumption may already
have queued native mod-derived work. `p4_package_boot_fail(actualProvider,reason)`
attributes the real kind/id or NULL for an unattributable service. Never call
early rollback, retry admission, fake GP restore or free handles/VMs here. Native
Restart-without-X/Quit atomically persists the correct selector and choice; unknown
attribution disables both selectors. The real OS restart schedules a fresh process
and ordinary drained exit; unsupported restart clearly requires manual relaunch.
VMs/resources stay alive until actual quiescence/drain, VMs before aggregate handles.
First Lua init follows late publication.

Actual native source revalidation may fail earlier than0010d480, after raw
exposure (for example ITEMTBL size/bytes/identity drift before its copy).
Call the nonreturning `p4_package_boot_source_failed(actualPinnedProvider,reason)`
from scalar `package_boot.h` immediately, before any unsafe copy. It uses the
same native recovery/drained-exit frontend outside late-callback execution.
NULL represents an actual native/unattributable source, never a fabricated
mod owner. Identify the frozen source in the reason; do not defer an empty
table, pretend rollback, or crash instead of the native error choice.

The scalar completion call returns1 ONLY if that invocation already committed;
returns0 for later native reloads/early-disabled optional startup. The override
compares explicitly, never treats a negative failure as successful completion.
`package_boot` and `mod_recovery_config` define real registry/file/INI unit cases
for order, refusal, selected lineage, retention and both selectors. They are NOT
renderer/RW/real GPU/native-dialog/restart/gameplay qualification receipts.

### Authored image resources

Native packages may declare a named image for UI consumers:

```ini
[resource-image example.mod:icon]
file=art/icon.png
format=png
schema=1
revision=1
```

The lowercase `owner:key` is the semantic identity, and the nonzero decimal
32-bit revision is authored explicitly; consumers require an exact revision.
`file` names a contained, indexed regular file in the declaring provider, not
a path in another provider or a raw VFS replacement. Currently `format=png`
and `schema=1` are the supported format and schema. PNG payloads are bounded,
fully decoded to straight RGBA8, and validated (including chunk CRCs) during
early CPU preparation. Invalid declarations or payloads refuse their actual
provider before raw files publish. When multiple providers declare one key,
the later selected package wins and the override is reported; descriptor owner
and actual winning provider remain distinct.

`game-resources.images` is the native Resources200 boot product. Its early
borrowed view provides exact owner/key/revision lookup but no graphics objects;
the late product is a frozen `P4Resources` generation built from the retained
validated bytes. Direct provider-plan sources use
`P4_CONTENT_RANK_PACKAGE_PLAN`, not an invented VFS layer rank. The core may
end the resource session only after independently draining game tasks, queues
and UI pins; a still-pinned generation refuses retirement.

### Authored model resources

Native packages may replace a disc model's look with a glTF 2.0 model:

```ini
[resource-model example.model:sword]
file=models/test_sword.glb
format=gltf
schema=1
revision=1
replaces=DATA/MODEL/WEAPON/WP004.RMD
```

Declare `requires = manifest.resource-model@1` in `[mod]` for a static
model, `manifest.resource-model@2` for skinned meshes, or
`manifest.resource-model@3` for slot animations. This lets older builds
refuse an unsupported model capability before loading the mod.
`schema=1` takes static meshes. `schema=2` also takes skinned meshes; a mesh
that is skinned under `schema=1` is refused. `schema=3` also replaces the
replaced model's slot animations with the glTF's `L<layer>S<slot>` animations;
under `schema=1` and `2` those animations are cooked but not applied, and the
start log says so. `owner:key` and `revision` follow the image rules above.

- `file` names a contained, indexed `.glb`, or a `.gltf` whose buffers and PNG
  images are contained files beside it (relative URIs, no `..`).
- `replaces` is the disc key of an `.RMD`, or `<pack>.PAC#<member>.RMD` for a
  model inside a pack, such as
  `DATA/MODEL/PACK/BC001.PAC#bc001.RMD`. It must exist on the disc (or in
  the developer tree `P4PC_OVERRIDE_DIR`).
- A `when` gate follows the other sections: a restart toggle only.

Early CPU preparation cooks the glTF, or loads a cached cook (see
[models.md](models.md#runtime-gltf-models)). It checks it against the
replaced model's budget. A refusal names the glTF object and the exceeded
bound, for example `texture blade is 512x512; at most 256 for a weapon`, and
refuses its provider before raw files publish. The runtime phase freezes the
override table that the model loader consults.

Conflicts:

- When two packages declare the same key, or replace the same model, the
  later selected package wins and the override is reported.
- When one package's `files/` raw-replaces the RMD or PAC that another
  package's `[resource-model]` targets, the model section wins for the
  model's geometry, materials and textures. It is the more specific
  declaration. The game still reads the winning file's other chunks: its
  animation slots, attach table and effects. Both providers are reported.

With no section declared, the table is empty, and every model takes the
retail loader path with nothing more than one flag read.

## Existing Mod Compendium packages

An existing Persona 4 PS2 data package can stay in its original directory. The
loader accepts either a direct disc tree (`DATA/`, `BTL/`, `BGM/`, `ENV/`,
`ZZZZZ/`) or Mod Compendium's conventional `Data/` wrapper around those
containers. For example, `Data/BGM/04.ADX` maps to `BGM/04.ADX`, while
`Data/Data/BUSTUP/NAME.BIN` maps to `DATA/BUSTUP/NAME.BIN`. No repacking,
prebuild-script execution or modification of the installed package is needed.

Optional `Mod.xml` supplies `Id`, `Title`, `Version` and `Game`. A nonempty Game
must identify Persona4, not Golden or another game. XML parsing is streaming,
bounded to 64 KiB, with a 4 KiB parser stack and 32 nesting levels; no external
entities are fetched. A nonzero GUID becomes the stable enabled id
`compendium.<lowercase-guid>`; absent/zero GUID uses a deterministic folder id
`compendium.<16-hex-digits>`. Missing title/version use the folder name and
`unversioned`. Legacy priority/order default to 0, and legacy and native packages
share the same `(priority, order, id)` sort and conflict report.

Legacy filenames may retain their actual ASCII casing. One immutable index
maps canonical game keys to original host paths, so Linux/Android do not rely on
Windows-style case-insensitive lookup. Canonical collisions inside one package
are refused. Metadata and unrelated root documentation are audited but are not
mounted as game files. Native `files/` names remain strictly uppercase. All
indexes together are bounded to 32 MiB; temporary conflict records to 8 MiB.
Indexed misses do not allocate, stat or walk directories. Directory enumeration
uses the same index and winner as open/stat, not a second case-folding scan.

Executable/ELF/IRX/native library files, HostFS, `.pnach` and nonempty prebuild
scripts are refused with an explanation that the feature must be rewritten as a
Luau mod. This includes disguised ELF/PE inputs and code outside the mounted
payload. Only an empty, root-level Compendium-generated `prebuild.bat` is
accepted; it is never run. Presence of `mod.ini` always selects native parsing:
an invalid native manifest cannot silently fall back to `Mod.xml`.

`tests/compendium_metadata_test.py`, `tests/package_files_test.py` and the
legacy/native cases in `tests/mod_files_test.py` cover metadata, code refusal,
case mapping, bounded indexing, direct/wrapped layouts and shared precedence
with synthetic data. Real-mod assets remain private test inputs, never source
or distribution content.

The byte-consumption witness `tests/mod_package_read_test.py` also checks binary
and empty files through real VFS open/stat/read/origin. Set
`P4PC_PRIVATE_MOD_FIXTURES` to an external authored-fixture root to additionally
consume every file in the Almighty Loop Fix and Saturated Portraits packages
and compare complete bytes/hashes with their private source files. Unset means
those two private cases are explicitly skipped, not silently reported as passes.

## Precedence and conflicts

Load order is ascending `(priority, order, id)`, independent of directory listing
order and order of ids in `enabled`. Later layers win. The complete order is:

1. Retail disc.
2. Enabled mod `files/` directories in load order.
3. Explicit developer `P4PC_OVERRIDE_DIR`.

Use container-qualified keys, such as `DATA/PATH/FILE.BIN`, to select the same
container as the retail caller. Existing VFS device-prefix, case and `;1`
normalization still applies to game requests. File opens, stat and merged
listings agree on the winning file and size; each child is listed once. Files
unique to lower layers remain visible. The CD model's physical disc lookup stays
retail-only, and open override descriptors retain the original backing extent.

For an unqualified retail open of an existing disc file, each layer also checks
that file's container-qualified replacement before proceeding to a lower layer.

Every pair of enabled mods replacing the same canonical file is reported on
activation with both ids, the key and the later winner. This is an intentional,
deterministic file override, not a refusal. An explicit developer loose override
remains above all mods. Tables, compiled event scripts, text, textures, models
and music use the same byte-replacement path; no retail bytes are bundled here.

`tests/mod_files_test.py` exercises deterministic sorting, priority/order/id ties,
disabled/missing/duplicate/incompatible packages, all-or-nothing activation,
noncanonical/linked paths, developer precedence, size/stat and merged listings
using synthetic text only. It needs no ISO. A successful file-layer test is not
a claim that a replaced game table's structure or expanded counts are valid.

Python fixture registrations forward the executable target's complete
`CROSSCOMPILING_EMULATOR` command prefix before the child executable. The host
Python interpreter stays native; Win32 Wine and other cross-ABI children use
their configured runner rather than an attempted direct host execution.

Link fixtures use the target OS's exposed filesystem semantics. Native POSIX and
native Windows test ordinary file links (Windows requires link creation rights).
For a POSIX launcher running a Windows executable, the fixture uses a directory
link: Wine can hide Unix **file** symlinks from Win32 attributes, as documented in
the [upstream Wine discussion](https://list.winehq.org/hyperkitty/list/wine-gitlab@list.winehq.org/thread/77JC3DCH6AZZDE54L4IMIYHXUIVITL2E/).
The port does not claim to detect links that the target OS API conceals. This
does not remove any package validation or change native Windows behavior.

## Native provider metadata

`include/p4pc/vfs_origin.h` exposes `vfs_origin(fd, &origin)` for the already
selected provider: kind, rank, provider id and canonical container-qualified
key. Disc rank is 0; registered file layers are 1..N; the developer layer is
N+1. Kind is explicit, never inferred from an id named `disc` or `developer`.
Strings remain immutable until the descriptor closes; copy retained metadata.
No host installation path is exposed by this interface.

One native consumer may register a `P4VfsReadObserver`. Re-registering the same
callback/user pair succeeds; a different occupied registration is refused.
Removal must match the owner pair. Successful nonempty reads notify it with
exactly the initialized destination bytes and file/disc offsets. Host overrides
have no physical disc offset. Raw sector reads have a disc offset but no guessed
file key or file-relative offset: one read can span multiple files. Do not mutate
the bytes or re-enter VFS from the callback.

With no observer, reads take only the null branch: no metadata lookup, hashing,
allocation or metadata copying. Canonical disc keys are formed lazily on query.
`tests/vfs_origin_test.py` uses a synthetic five-container disc to cover provider
identity/ranks, canonical aliases, failed-mount rollback, retained backing
extents, exact observed buffers/offsets and exclusive observer ownership.

Luau hooks, UI registration, save provenance and table expansion are separate
phases; this page does not claim those interfaces exist yet.
