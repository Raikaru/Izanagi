# Saves and memory cards

[Platform index](../platform.md) · [Player paths](playing.md)

## libmc2 folder-card contract

`src/platform/memcard/ps2_memcard.c` maps port N to `$P4PC_SAVE_DIR/mc<N>`.
Player launches default to `save/` beside `p4pc.ini`; the shim's standalone default is
`p4pc_save`. Sockets are opened by CreateSocket. Supported entries are Init, CreateSocket,
Format, GetInfo, SearchFile, ReadFile, WriteFile, CreateFile, Mkdir, Chdir, CheckAsync,
Sync and Sync2. Delete, Rename, GetDir and other unsupported entries remain traps.

Host file work completes at submission and parks one result. Accepted asynchronous calls
return a positive command ID, except Format's zero submission result. Sync/Sync2 return
1 once with kind/result, then -1 while idle. Busy calls return `0x81018010` (Format:
`0x81010010`) **before changing files or output buffers**. Present cards report ready
without a synthetic insertion/CARD_CHANGED retry.

Kinds are libmc2's values: GetInfo 2, Format 3, ReadFile 5, WriteFile 6, CreateFile 7,
Mkdir 11, Chdir 12 and SearchFile 14. Failures preserve the `0x8101xxxx` encoding,
including the low-word error that the game's card code switches on, rather than `-errno`.
Names beginning with `/` are absolute within the card, not host-absolute paths.
GetInfo reports formatted status (`status[1] = 1`). SearchFile uses the game's expected
directory-entry layout: size at `+0x10`, attribute at `+0x14`, name at `+0x1e`.
`P4PC_MC_LOG=1` logs operations, result codes, names, offsets and sizes.

Windows normalizes to absolute extended-length paths for reads, metadata, creation,
formatting and real-path custody resolution. `p4_os_list_dir` uses long-path-capable CRT enumeration, not MAX_PATH-limited
MinGW `opendir`. Classification converts UTF-8 to wide paths for `GetFileAttributesW`
and excludes reparse points. Formatting joins children with one separator; POSIX iteration
uses the same OS seam. A long retained artifact path must not make a real card look absent.
Directory ownership rejects reparse points both by path and by the opened handle;
Wine can expose a Unix directory link in path attributes while reporting its target
through the directory handle. Paths whose link status the platform conceals cannot
be identified by these Windows APIs.

The game writes `BASLUS-21782Save<N>/` containing its matching data file (171112 bytes),
`icon.ico` (53542 bytes) and `icon.sys` (964 bytes). Its own save builder and
`datSaveVerifyChecksum` produce/verify the format; the platform does not invent a new save.
The title applies a loaded game when handing over to the field, not immediately when
“Load successful” is displayed.

## Native mod-save blocks

`include/p4pc/mod_save.h` is the C-only provider interface. It does not enlarge
the retail image or make package/table presence a gameplay capability.
Registration copies a size/version-negotiated descriptor and stable owner/key;
the caller keeps callbacks, user data and exact frozen views alive until
successful unregister at a real quiescent boundary.

Providers encode bounded little-endian data, not native struct/pointer blobs.
Return1 before any output to omit empty/retail-default state; registration alone
must not produce metadata. Present blocks carry their own schema and required
flag. Actual package requirements bind kind/id/version from an attested current
provider; native mode/schema identities are explicit component data, never
inferred from a failed package lookup or an owner prefix.

Actual `p4_mod_save_touch` use is process-scoped, not installation: cached mod
code/data/products remain live across load and New Game without another physical
read. It joins each captured save's provenance until restart. New Game clears
loaded OLD-save history, not the current process-use set. This is ordinary mod
history, not the separate test-custody flag.

Load decodes every provider into owned staging, then validates all providers
before any commit. Absent blocks stage default state. Explicit new-game reset
has a NULL/zero image as well as an absent block; normal untagged loads retain
the full image for reference checks. Validation may inspect other staged
providers but cannot change GP slots, game memory, queues or active snapshots.
Failure discards every partial result; successful commit is allocation-free,
infallible and consumes staging. This does not replace the native battle-table
completion coordinator.

Lua mods may register one `p4.save.register` provider with a positive mod schema,
a `max_bytes` limit up to 1 MiB, a snapshot function and a load notification.
Snapshots return nil, booleans, IEEE binary64 numbers, binary strings, or plain
acyclic tables of those values with boolean/number/string keys. Tables with
metatables and native handles, functions, threads and buffers are refused.
The versioned encoding orders raw keys, bounds nesting/nodes/bytes, and records
the selected script provider as required only when non-nil state is emitted.
The mod schema and serializer revision are independent. Integers that require
more than binary64's exact integer range must be split into numeric limbs or
encoded as binary strings; a rounded Lua number cannot recover the original.
A nil snapshot omits the block; registration alone does not tag a vanilla save.

Snapshot functions are bounded protected calls: they cannot dispatch native
game work or mutate guest/native state through the API. Lua globals are not
rolled back if a snapshot fails. All snapshots and codecs finish before save
payload staging; the transport keeps native-owned participant staging until
the final card generation publishes. Failure discards new snapshots; success
swaps native slots without Lua, allocation or I/O in participant commit.
Load builds values under the VM cap before retail restore; after every native
provider commits, an ordinary budgeted load callback receives the value (nil
for absence/New Game). Post-commit callback failures use native recovery,
not a claimed rollback. Missing required script providers refuse; missing
historical-only mods use the existing precommit warning and acknowledgement.

`.p4mods` has a bounded versioned length-delimited envelope bound to the exact
retail size/SHA-256, independently versioned owner/key blocks and separate
historical provenance/current requirements. Unknown required providers, wrong
bindings or malformed/incompatible data refuse; unknown optional blocks
round-trip unchanged rather than disappearing. The `.p4mods-commit` marker
binds one retail/sidecar generation. Hash integrity is not a signature or proof
that locally editable data is trusted. Absence is not an attestation that a save
was never modified.

The real full-image WRITE submission (`00464650`, historically named
`H_Memcard_BeginLoad`) captures providers before the task yields. It receives the
complete version-prefixed retail image; providers never retain that image.
Tagged writes stage the data file, `icon.ico`, `icon.sys`, sidecar and commit
marker in one same-card private generation. Publication happens only after the
final `icon.sys` close and before reporting async success, not after the first
data write. No extension state means the original retail byte/write path;
explicitly overwriting an old tagged slot still stages a clean replacement so
stale sidecar/test markers cannot bind the new image.

Native game writes and imports share `save_transaction.c`. A real root-directory
lock excludes another process without adding a memory-card lockfile. A bounded
checksummed journal records pending staging before payload writes, and publication
intent before directory renames. Journal updates atomically replace the old file
on the same filesystem, including Windows; the old record is never removed first.
The old complete generation stays canonical
until publication. Real restore is attempted if publication fails; failed
restoration preserves stage/backup/journal with a clear error, never a claimed
rollback. Startup recovers recognized interrupted generations or refuses an
ambiguous/corrupt one without deleting it. This is not a power-loss/physical-media
durability qualification.

Only an actual complete data-file read stages providers, under the same root
lock as retail/sidecar/marker reads; 56-byte menu summaries do not. Missing,
corrupt, incompatible or required state is refused before the read succeeds
and before any GP/extension commit. Missing historical mods are listed in a
native warning and require an explicit player acknowledgement when optional
state permits continuation; scripted runs never invent that acknowledgement.

Every missing historical kind/id/version and unknown optional block is listed,
not only the first one. Native Details pages expose the complete bounded list.
Native providers may use `p4_mod_save_warn(context, text)` during real-load
decode/validate to add optional native-source/mode provenance to that same
precommit acknowledgement. Text is copied (4096 bytes per message, 8MiB total);
invalid or unretainable warnings refuse the load even if a callback ignores the
error. Required-source checks remain strict. This API neither invents a package
nor sets witness custody, and is invalid during encode or New Game defaults.

Unknown optional blocks survive the next native save unchanged. `0010e710`
proves the exact version/payload digest before retail GP restoration and commits
extensions only after that restoration. `001113b0` stages provider defaults
before the actual New Game reset and commits them afterward. The sole shared
override depends only on this registered-provider transaction, not a particular
table expansion. Each actual provider stages its own empty/default state and
clears its owned extension state in infallible commit; registration alone does
not imply that such a producer or its reset implementation exists.
The actual READ submission `00465570 H_Memcard_BeginSave` (again historically
misnamed) supersedes the previous logical MC intent before native searches.
Only that boundary, a new save, or New Game cancels owned staging. An unrelated
low-level read during a live save receives busy without touching its buffers
or cancelling the save; later icon writes cannot falsely acknowledge it.

Test custody is separate from a player's ordinary mod history. Only explicit
mod-witness inputs/runs use header flags at byte68 mask`0x1` and the canonical
`p4pc-run-mode.json`/portable`.p4pc-mode.json` markers. Even an otherwise-empty
witness save carries an intrinsic bound envelope. Gameplay refuses those test
artifacts outside the explicit `P4PC_MOD_WITNESS=1`, `P4PC_INPUT=script` lane.
Ordinary modded saves have that flag OFF: turning mods off uses the ordinary
history warning/acknowledgement and required-block rules, never a witness-only
refusal. New Game cannot declassify an already witnessed process. Custody checks
inspect named and resolved ancestors, including aliases; native-folder copying
preserves and labels that provenance rather than silently stripping it.
On POSIX, marker inspection uses no-follow metadata rather than listing each
ancestor, so a traversable execute-only directory is supported. Symlink or
non-file markers, inaccessible paths and missing or non-directory parents
still fail closed.

The codec/transport fixtures use synthetic checksum-valid bytes, the real
package registry, MC async entrypoints, provider staging and filesystem. They
define hash vectors, exact ownership/binding, defaults, warning decisions,
optional retention, native stale-tag stripping, alias custody and genuine
process death before publication. These are not native SDL/restart/gameplay or
i386/x64 retail bit-identity receipts. Actual save/restore, imported/collection
round trips and the separate mod-test lane require their real game gates.


## Importing PS2 saves

```sh
p4pc --import-save BASLUS-217825361766530.PSV
p4pc --import-save backup.ps2
p4pc --import-save BASLUS-21782Save0 --slot 3
```

`--import-save FILE_OR_FOLDER [--slot N]` needs neither a game boot nor a disc.
Destination is `save/mc0/` beside the settings file, or `$P4PC_SAVE_DIR/mc0/`.
Supported sources:

- PS3/Apollo PS2 `.psv`, uLaunchELF/EMS `.psu`, Action Replay MAX `.max`;
- CodeBreaker `.cbs`, SharkPort/X-Port `.sps` / `.xps`;
- 8 MiB PCSX2 `.ps2` images, 512-byte pages with or without 16-byte ECC spare;
- PCSX2 folder cards or individual `BASLUS-21782SaveN` folders.

Unzip downloads first. Only NTSC-U Persona 4 saves are accepted, not unrelated games
or PS1 PSV. Slots are zero-based 0..15 (LOAD GAME displays 1..16). Normal import preserves
a free source slot, otherwise uses the first empty slot; it never overwrites.
**`--slot N` explicitly authorizes replacement** and requires exactly one save.
Multiple saves require enough empty slots; parsing/integrity validation finishes before
any write. Each save is staged completely through the same native journaled
transaction; atomicity is per save, not all16 as a batch. Replacement retains
the previous directory until complete publication, with actual restoration or
explicit retained recovery state on failure. Linked/non-directory slot paths
are never replacement targets, even with `--slot`.

Windows rename/empty-directory removal retries only transient access-denied or sharing
violations with exponential waits bounded to 2040 ms. Retries are logged; persistent
failure preserves its real error and the backup/rollback protection. Other errors fail
immediately.

Game bytes/icons are preserved; changing slots renames the directory and matching data
filename. PS2 timestamps are retained losslessly in `.p4save-meta`, and host modification
times are set when available. PCSX2 `_pcsx2_index` creation/modification timestamps are
retained, including `$ROOT` UTC and older `%ROOT` Japan-time forms, block or flow syntax.
Folder cards without an index use host timestamps. PCSX2 private metadata is not copied
as game files. MAX has no original file timestamps.

Ordinary imported PS2/native saves remain untagged and their retail bytes stay
unchanged. An explicit replacement removes stale old-generation mod metadata.
Native folders can also carry separately bounded `.p4mods`/`.p4mods-commit`
metadata: both must be present and match the exact full retail image and one
another. A valid pair is preserved, including slot remap (filename only).
Offline import does not pretend that required runtime providers are installed;
normal gameplay load performs that check. Incomplete/malformed pairs refuse.
Native extension pairs are not PS2 container entries and have no silent
strip/migration/export path. Witness inputs retain canonical non-reference
custody, even when copied outside a witness process.

Validation covers sizes, names, extents, MAX CRC32, CBS zlib Adler-32, SharkPort checksum,
card ECC, Persona 4 summary checksum and tagged-record checksum (`0010e880`). PSV's PS3
signature is intentionally not verified: the payload is plaintext PS2 data, and no console
signing keys are needed or shipped.

The settings Import save action offers desktop file/folder picking. SDL callbacks copy
results to a queue; import happens only while the settings interface is active. Cancel
changes nothing. A result arriving after close waits until the interface reopens.
Android imports supported containers/folders from `Android/media/org.p4pc.game/files/import`
(the first activation creates it); Android supplies the actual volume path. No storage
permission or Python is required. Sources are unchanged; repeated imports use empty slots.
Return to LOAD GAME afterward: importing never replaces the running party or warps a field.

## Tests and private fixtures

`tests/save_import_fixtures.py --source PATH/TO/PORT_SAVE --out SCRATCH
--max-encoder PATH/TO/import_max_test --driver PATH/TO/save_import_test` creates private
fixtures and checks byte-identical import, collision/replacement, truncation, traversal
and checksum rejection. Without `--source` it uses checksum-valid synthetic data.
`tools/ps2save.py` retains developer card/PSU writing and inspection, not a second player
import policy.

`save_load` drives the game's real save/load dialogs, resets state and compares built
images. `retail_save_load` loads private retail saves and permits at most 16 differing
volatile bytes; this is a save round-trip tolerance, not general state normalization.
`save_import_load` is opt-in with `P4PC_IMPORT_SOURCE`, disc and ELF: it runs the actual
CLI then LOAD GAME, checks the field/state and compares the full save image.
Settings import tests exercise cancellation and closed-menu deferral with private input,
never an unattended OS dialog on the user's desktop.

Harness `P4PC_SAVE_SEED=DIR` copies pristine input into card 0 under an isolated
`P4PC_SAVE_DIR`; `P4PC_SAVE_OUT` retains an output checkpoint. Retail saves, captures,
card images and canonical checkpoint bytes remain private and are never committed.

The project's private gate runner (not part of this repository) provisions the collection
checkpoints from an index of private save folders. It checks the pinned payload digest,
copies each slot to an isolated per-attempt slot 0, and checks both source and
copy again after the game exits. The two import witnesses similarly use a
digest-pinned private PSV under the retail save tree. Missing or changed
inputs fail the provisioned gate instead of being counted as a skip, and
native result reuse keys these inputs. Neither the index nor the retail files
are modified; standalone CTest import runs remain opt-in.
