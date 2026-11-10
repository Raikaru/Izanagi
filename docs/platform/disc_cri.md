# Disc and CRI files

[Platform index](../platform.md)

## Read-only VFS

`src/platform/files/` owns the host file layer. `P4PC_ISO` names the user's retail disc.
`iso9660.c` indexes `DATA.CVM`, `BTL.CVM`, `BGM.CVM`, `ENV.CVM` and `ZZZZZ.BIN`, each
an ISO 9660 volume at container offset `0x1800`. Search order is DATA, BTL, BGM, ENV,
ZZZZZ, then disc-root files such as `SLUS_217.82`, `SYSTEM.CNF` and `LIB31/*`.
A leading container label restricts lookup. Device prefixes such as `cdrom0:` and `;1`
suffixes are removed; names are matched uppercase.

`P4PC_OVERRIDE_DIR` supplies loose files at the same uppercase relative paths. Overrides
win over disc entries, including in the merged directory tree. `sceCdInit` (`0042eae0`)
mounts the VFS and fails loudly for an unreadable named ISO; with no ISO it mounts only
overrides. [Player disc selection](playing.md) supplies the normal launch path.

Enabled [file/data mods](../modding/files.md) add read-only layers between the disc
and the developer override directory. Sorting, duplicate-file diagnostics and
opt-in configuration are described there. No enabled packages means the existing
disc/loose-file lookup order is unchanged.

`sce_file.c` implements `sceOpen`, `sceRead`, `sceLseek`, `sceLseek64`, `sceClose` and
`sceSync`. Writes are refused. SDK read-only flag 1 is accepted; write modes 2/3 and
CREATE remain refused. `_FILE_OFFSET_BITS=64` is global so disc offsets above 2 GiB
cannot truncate after a forced compatibility include.

Directory handles have a separate table from file descriptors. `sceDopen` (`00428550`),
`sceDread` (`00428780`), `sceDclose` (`00428618`) and `sceGetstat` (`004288d8`) use
`vfs_dir_open/next/close` and `vfs_stat`. Dirents are `0x148` bytes, stat objects 64 bytes;
timestamps are zero. Mode bits `0x1000` (directory) and `0x2000` (file) follow ps2sdk
[INFERENCE], rather than a claim that retail bytes established those values.

### Early immutable source plans

`p4pc/vfs_source.h` supports the demand-only CPU table/source preparation pass
before `sceCdInit`. It uses the same selected `P4PC_ISO`/developer root, actual
ISO index, container canonicalizer, physical reader and exclusive read observer
as the later native VFS. It does not mount the VFS, publish a package override,
consume a native file slot or create a second disc walk. With no early demand,
normal mounting has no additional identity capture or source allocation.

The first `vfs_source_prepare_selected` captures the selected configuration and,
when present, the open disc's actual `P4OSFileIdentity`; subsequent preparation
reuses it. Exact DISC and DEVELOPER plans retain their actual byte span, canonical
key and file identity. A resolver must try developer, the actual selected
package's existing `P4PackageFilePlan`, then disc; MOD is deliberately not a
fallback plan kind. The contained labels are `DATA`, `BTL`, `BGM`, `ENV`, `ZZZZZ`,
not archive filenames or path guesses.

Plan reads check bounds and the retained object's identity before and after
the same physical read used by `vfs_read`. A changed object refuses; an absent
selected disc/source is explicit, not an invented identity. Plans have balanced
game-thread retain/release ownership and do not themselves establish a native
table binding: the table owner must still compare the actual loaded byte slice.
Later `sceCdInit` uses the captured selection and already-built index, verifies
disc identity and mounts the real package/developer layers only at the existing
mount boundary. These CPU plans are not a gameplay/native-consumer certificate.

For small source-backed tables, the consumer must seal a `P4ContentFingerprint`
from the complete successfully read input and retain it with the selected
DISC/DEVELOPER plan or actual package plan/provider. At the native consume
boundary it must compare the entire bounded guest buffer with
`p4_content_fingerprint_matches` before copying or publishing any rows, and
check the packed descriptor against its own sealed spans. Equal content is
attributed to that retained selected plan; the fingerprint does not claim that
the guest bytes came from a particular read. A mismatch or incomplete buffer
refuses admission. The fingerprint is not a lifetime or gameplay certificate.

`p4pc/game_table_inputs.h` prepares ITEMTBL and the eight packed battle sources
from the selected plan, without mounting or allocating guest RAM. It preserves
the actual winning package provider or native plan, bounds the complete read,
and seals only valid aligned packed spans with their owning input identities.
The complete source fingerprint is sealed at the same point. These immutable
CPU inputs do not themselves authenticate a later native copy or publish rows.
`game_table_inputs` exercises developer/package/disc precedence, malformed and
oversize refusal, exact span ownership and whole-buffer matching using synthetic
files; it does not claim a live native binding.

`p4pc/game_table_admission.h` checks the complete live bytes against their exact
selected input and checks packed descriptor slot vectors against the captured
span roles. ITEM uses its exact source label. An immediate skill observer can
consume a stack-local one-use receipt bound to the exact input, descriptor,
path, buffer and size; wrong/replayed receipts fail, and direct calls still
require generic source preparation. At native ITEM loader `0x001117c0`, the
completed request is checked before the original retail copy. At packed
loader `0x00193840`, the selected descriptor and complete request are checked
and handed synchronously to the skill observer before any destination slot or
bank copy; the existing original-extent publication remains after each copy.
Before staging, every selected packed input must fit both the actual disc
file's byte length and its aligned-payload footprint. The latter matters:
equal file sizes can produce different arena consumption when header padding
changes. If the original disc source or its packed layout is unavailable,
staging refuses; with a packed tuple staged, an unstaged known table descriptor
also refuses. ITEM is bounded by its 104464-byte retail destination. With no
staged input, both boundaries retain the original copy path. These checks do
not themselves stage a table, establish the arena's total extent, grant
image-lifetime authority or qualify native gameplay.

## PC disc contract

There is one PC-speed implementation, not a PS2-duration model or a configurable
fast-load path. Unknown INI keys receive the parser's normal unknown-key handling.
Correct dependency ordering is required; PS2 load duration is not.

The CRI SDK path reaches `sceCdSearchFile`, `sceCdRead`, `sceCdSync`, `sceCdGetError`
and `sceCdBreak` in `sce_cdvd.c`. Search preserves the original LSN and size. Host I/O
may fill bytes immediately, but a successful asynchronous read remains busy until the
**next scheduler frame boundary, after VBLANK**. It cannot finish in its submission
update, even for cached or override data. `sceCdSync(1)` polls busy 1 / complete 0;
`sceCdSync(0)` waits for the boundary. A second read is refused while busy. Break cancels
the read's owned completion alarm.

`sce_adxf.c`, `adxf_partition.c`, `adxf_fsvr.c` and `rofs_host.c` implement CRI file
windows over the same VFS and retain guest callback addresses. ADXF uses the same
next-frame completion fence. CVFS states are 0 idle / 1 complete / 2 transfer / 3 error;
ADXF states are 1 prepared / 2 reading / 3 complete / 4 error. They are not interchangeable.
Close/stop cancels a handle's alarm before releasing storage.

Device completion does not install resources or advance the calendar. The game's own
CRI/filesystem/task steps consume completion and publish dependent work. In particular,
resource resolution must not outrun the calendar latch in the requesting update; this
ordering fence preserves riverbank weather without changing calendar or field-loader code.
It is not a boot-time whole-machine lockstep guarantee.

Native busy-wait safepoints service priorities and device alarms, never directly pump CRI
publication. `H_Cdvd_ReadSync` preserves its condition/update order and polls only while
its request is pending. Its fixture uses an actual timer-woken EE worker. Status getters
remain observers, including in the viewer.

Raw `sceCdRead` and the ADXF/ROFS window reads are the only users of `p4pc_set_next_frame_alarm`. Its seam,
`p4pc_frame_alarm_seam`, stays NULL except in a lockstep battle qualification run with
`P4PC_LOCKSTEP_DISC_REPLAY`. There, after the anchor, it holds each read until the frame
at which retail observed it complete (the in-battle disc replay of lockstep, the private retail-comparison tooling).
The disc implementation files themselves are unchanged.

## Wrapper and size ABI

`H_Cdvd_*` wrappers are not the only disc path: generated CRI `dvci.c` calls the SDK
directly. The H_Cdvd file wrappers use `sceRead`; `H_Cdvd_SlotWrite` reaches the refused
`sceWrite`; `H_Cdvd_ResetStreamState` reaches the media-mode no-op `sceCdMmode`.
Path/archive/cache helpers retain their compiled game implementations. Function ownership
and unresolved address bindings belong to `config/` and `overrides/CLAIMS.md`, not an
independent historical wrapper census in this page.

The host-file chain preserves a full 64-bit result in EE `v0`: `htCiGetFileSize`
(`004e77c0`) and delegate `004e8058` use explicit 64-bit C types, not host `long`.
`htci_filesize_upper` reproduces signed `dsra32`. `004e47f8` selects the real `sceLseek64`
backend (`00429358`) when `00731c70 == 1`; the other branch deliberately sign-extends a
32-bit offset and result. Do not remove this live retail branch as obsolete compatibility.

## Observers and regression coverage

- `files_disc` checks all five containers, root ELF magic/size, raw bytes and override
  precedence. It skips with 77 without `P4PC_ISO`; it is not a CD-native timing test.
- `cri_file_size` and `cri_file_size64` exercise the real size/high-word/narrowing chain
  using the greater-than-4-GB ISO through the override layer. They compare bytes around
  4 GB and require `P4PC_ISO` and `P4PC_ELF`; no disc fixture is committed.
- `vfs_source` checks the actual ISO parser/index, pre-mount refusal of native
  opens, exact developer/disc spans and container names, one observer, retained
  identities, mutation refusal, selected-configuration capture, later mount reuse
  without another walk, package/developer precedence, explicit absence, and
  whole-buffer fingerprint acceptance against the selected developer source
  rather than a lower disc source. It uses generated synthetic fixture bytes.
- Kernel tests cover real/virtual clock alarms, cancellation and next-frame completion.
- `P4PC_FIELD_ENTRY_DUMP` observes ready field visits: `FldRootUpdate` states 12/13/14,
  both pending scene-request bits clear, date/weather/environment, 176 persistent event
  words and 22 live resource lists. A selected capture also emits a `capture:true` snapshot,
  possibly during loading; that neither marks readiness nor suppresses the later entry.
- Read-only retail trace correlation is done with the private PCSX2 trace tooling, which is
  not part of the public repository. It measures evidence; it does not select or implement a port
  timing policy.

Developer field/battle harnesses wait for logo work state 4, when both title-loader children
are complete, before replacing the logo. A minimum sequence-relative delay alone is not
permission to destroy a loading task. Ordinary New Game/checkpoint play uses no such shortcut.
