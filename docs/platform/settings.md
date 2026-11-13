# Player settings

[Platform index](../platform.md) · [Input](input.md) · [Video](video_presentation.md)

## File and precedence

`src/platform/config/p4cfg.c` reads `p4pc.ini` in
`SDL_GetPrefPath("p4pc", "Persona4")`:

- Windows: `%APPDATA%\p4pc\Persona4\`;
- Linux: `~/.local/share/p4pc/Persona4/`;
- macOS: `~/Library/Application Support/p4pc/Persona4/`;
- Android/iOS: the app's files directory.

`P4PC_CONFIG=<path>` selects another file; `P4PC_CONFIG=none` disables file access and
refuses writes. Precedence is **built-in default < INI < environment**. Sections and key
names together identify a setting. A player first run writes the commented template
with player defaults; no test/harness/capture/headless run creates one.
`p4cfg_player_run` excludes explicit config selection and automation environment variables,
including `SDL_AUDIODRIVER`. CTest and boot/scene/Android runners force `P4PC_CONFIG=none`.
Unknown keys are not alternate feature switches.

The generated desktop player template selects a 1280x960 window, `fit=4:3` and
`present=blit`; these are file values, distinct from the no-file gate defaults of
640x448, stretch and auto. Deleting a key restores its built-in default.

[Video settings](video_presentation.md#settings) lists every display key, default and env
mapping. Other player keys are:

- `[game] iso`: validated disc path; `P4PC_ISO` overrides it. See [startup](playing.md).
- `[golden] install`: the player's own Persona 4 Golden (Steam) folder for the
  Golden mechanics mod; `P4PC_GOLDEN` overrides it. When neither is set, the
  standard Steam libraries are searched. See [Golden mechanics](../modding/golden.md).
- `audio.volume` / `P4PC_VOLUME`: 0..100, default 100, applied to the device stream via
  `p4snd_set_user_volume`. Offline test renders do not observe device volume.
- `input.deadzone` / `P4PC_STICK_DEADZONE`: 0..50 percent, default 0.
- `input.kb.<button>`: two SDL key-name slots, comma separated, or `none`.
- `input.pad.<button>`: one SDL gamepad button name, or `none`.
- `input.touch_scroll`: mobile menu swiping, 0/1, default 1.

Input defaults and axis invariants are in [input](input.md#player-controls).

The `[lockstep]` template uses `hotkey=F9` and `interval=60`; its directory is supplied
by the lockstep launcher (private retail-comparison tooling, not part of the public repository), not
normal player startup.

## Optional-package recovery

Manifest/API/conflict, script-compilation and mandatory CPU decoding failures
refuse the affected package before any replacement files or mod code are exposed.
The native message explains the refusal; startup continues without that package.
Failures after exposure never pretend to roll back game state or unload native
work. An attested package gets **Restart without X / Quit**; its actual selector
entry and choice are saved atomically. No owner-string guess identifies a mod.

An unattributed failure instead offers **Restart with all mods disabled / Quit**.
Safe mode atomically empties both `mods.enabled` and `packs.enabled`, records the
cause/choice and a next-boot notice, then requests a genuine fresh process. That
boot explains why all package selectors were disabled. Quit without an attested
owner leaves the selections unchanged. Unsupported automatic restart gives clear
manual-relaunch instructions after saving the chosen recovery.

With packages requested, startup records a pending marker before loading the game
image or preparing optional packages. The real interactive `titleDrawProc` state9
is its stable milestone, independent of scripted input or elapsed time. Genuine
normal game returns/code0 exits and explicit window-close intent also clear it.
Engine automation completions (menu, battle and new-game gates, lockstep and
battle-sim results) and restart requests leave through `p4pc_exit`
(`include/p4pc/crt.h`), so their code 0 is that same normal exit and a
harness that bypasses the title is not counted as a crash.
Fatal/nonzero exits and actual process death do not. Two consecutive unclean
pre-stable startups offer the same safe mode before another optional preparation;
quitting that offer retains the threshold. An empty/refused optional batch clears
its marker. Ordinary no-package startup creates no health settings artifact.
If startup cannot persist crash protection, optional providers are refused for
that launch rather than preventing the retail game from starting.

A real OS directory lease owns pending startup history through stability/normal
exit (process death releases it). Native aliases share the actual directory
identity; no lock file is created. After acquiring it, startup refreshes the same
existing INI before reading crash metadata, so a cached pending value cannot
mislabel the previous owner's healthy exit. A concurrent owner receives an
explicit **Continue with mods off / Quit** message. Continuing disables optional
preparation for that instance only: stored selectors and the other startup's
history remain unchanged. Selecting a different INI alone does not suppress
native error/recovery UI; the shared actual-automation fence controls that.


`mod_recovery_config` exercises real INI transactions and actual process deaths
around the health state machine, including safe-mode notice and selector
preservation. It is not a native-dialog, actual title-frame, restart, gameplay or
retail bit-identity receipt. MOD_WITNESS custody and fidelity refusals remain
independent of this startup health metadata.
The opt-in MOD_WITNESS `mod_loader_recovery` fixture registers the production
observer and leaves through `p4pc_exit`: three consecutive code-0 pre-title
exits keep the marker clear, while a nonzero exit leaves it pending.



[Showing lines 1-42 of 117. Use :43 to continue]

## In-game settings interface

**In-game Config.** PC options extend the game's own System → Config list, not a
second presenter canvas. The six retail options retain their original behavior;
display, volume and deadzone rows follow them, then a Controls sub-list,
registered extensions and the stock confirmation row. The title screen's Config
entry opens the same screen. Native row bars, right-aligned labels, ON/OFF pills,
value arrows, cursor wrap, scrollbar, help/footer glyphs, COMSE sounds and
open/close transitions are driven by the game's original SDK/menu routines.
PC labels use the same uppercase style. Value boxes stay inside the retail
right margin; the scrollbar sits outside the bars. CONFIRM is always last.
Their sprites and font metrics load through the normal VFS/resource path from
the user's ISO; no retail assets, fallback font or alternate renderer are bundled.
PC labels and choice values use the retail font's own measured widths and
kerning, fitting inside their bars/pills without clipping. Retail labels and
help rendering remain unchanged. Pad shoulder/stick values use the game's
legible L1/R1/L3/R3 names; stored SDL binding strings remain unchanged.
The layout is the native 640x448 UI, transformed by the active presenter/aspect
policy, independently of a pending restart-only aspect choice.

Esc or F1 requests Config; a gamepad can hold Select + Start for 0.6 seconds.
In the field this follows the real Camp → System → Config task path and respects
the game's own event, battle and flag-0x37 gates. A refused request is discarded,
not forced or replayed later. Before the menu system is ready there is no fallback
overlay. On Android the touch pad's labelled **CONFIG** pill (right side bar, below
R2) sends F1; it is part of the Java pad overlay ([Android](../android.md)), not
drawn into the game image, so it never covers the retail HUD. Tap a row to select
it and tap again to activate/change it; native arrows and optional vertical swipes
scroll the list.
Touch Back or Esc leaves a sub-list; from a shortcut-opened field root it resumes
the field. Ordinary game menu navigation keeps its stock Cross/Circle/Triangle
behavior (default keyboard Z/X/C, plus arrows); there are no host-only tabs.
The D-pad, left stick and mapped keyboard directions navigate the same native
list, with stock directional repeat; left/right change values. On desktop the
OS pointer is shown when Config opens. Gamepad input hides it, and mouse motion
shows it again and moves focus over rows. The presenter's fitted 640x448
coordinates determine hover and clicks in windowed or fullscreen output.
Closing Config restores the previous cursor visibility and relative-mouse mode.

Gameplay pauses through the native Camp/title menu lifecycle. Guest UI tasks,
transitions and the normal menu audio continue; there is no stopped guest loop or
private UI audio mixer. Opening/capture/closing input is fenced, and held input
must return to neutral before it can reach gameplay again. Fullscreen, window
dimensions, Resolution, fit, presentation and vsync apply live where supported.
Video adjustments execute in order at the safe end-of-present boundary, not
halfway through guest drawing; merely queueing one does not report success.
An unavailable live transition is marked “Applies on restart”; a transactional
Resolution failure retains its existing valid target and does not commit a new
preference. Mobile fullscreen/window dimensions remain controlled by the OS.
Volume and deadzone apply without restarting.

Controls provides two individually editable keyboard slots and one pad slot per
action. Activate a slot and press a key/button; a conflict asks before removing
the assignment from another action. Esc cancels, Delete or the value's left
arrow clears a slot, and Reset all bindings restores every shipped mapping.

## Persistence invariants

Each edit writes a sibling temporary file, checks writes/flush/close, then
atomically renames it over `p4pc.ini`. Binding conflict resolution and reset
write all affected keys in one transaction. Unknown sections/keys, comments
(including inline comments), and untouched bytes are retained. Failed writes
are not applied to any live binding.
`P4PC_CONFIG=none` refuses writes, and environment overrides retain their
precedence. Automated menu tests must explicitly opt into `P4PC_SETTINGS_TEST`
in addition to `P4PC_INPUT=script`; ordinary gates never accept menu input.

## Extension registration

**Registering additional settings.** Include `p4pc/settings.h` and call
`p4_settings_register(&entry)` on the game thread (SDL need not be initialized).
Registration may also append rows while Config is open. It copies the descriptor and returns
0 on success, -1 on an invalid descriptor/full extension list. Its `label`,
optional `description`, callbacks and `userdata` must remain valid while registered. The model stays
independent of the native drawing backend: `format(value, capacity, userdata)`
supplies a value, `adjust(direction, userdata)` receives -1/+1 for changes and
0 for activation, and `restart=1` marks restart-only options. An action such as
Import save acts only on 0. Up to 256 registered rows append to the native root list
(mobile reserves one for Touch scrolling); labels are bounded to 191 bytes,
the optional `key` replaces a matching built-in row in place instead
(including `video.resolution` at slot 3), without shifting binding indices.
Unknown keys append normally. Do not hardcode an extension's position:
`p4_settings_entry_count`/`p4_settings_entry` expose labels, values and restart
flags independently of row placement or the Controls sub-list.
The optional `flags` field declares availability rather than making the menu
special-case a provider: zero means general; `P4_SETTINGS_DESKTOP`/
`P4_SETTINGS_MOBILE` restrict platforms, and `P4_SETTINGS_KEYBOARD`/
`P4_SETTINGS_TOUCH` require that input capability. A keyed replacement with zero
flags inherits its built-in availability. `P4_SETTINGS_TOGGLE` selects the
retail ON/OFF widget; its formatter supplies `On` or `Off`.
`P4_SETTINGS_ACTION` keeps the stock OK widget even when `format` returns a
status; the formatter still runs for asynchronous result consumption.
The optional `description` supplies native help; omit it to derive help from the
label/widget. It is bounded to 2047 bytes and measured/paged with the stock help
font, not clipped or replaced by a generic message. It is model metadata, not a
drawing-toolkit callback. A keyed replacement can override built-in help.
Android/iOS omit desktop window/fullscreen/presentation controls and keyboard
slots without a keyboard, but retain pad bindings and Touch scrolling. VSync
remains available through the real mobile SDL/EGL swap-interval path, with the
same best-effort driver semantics as desktop.
Desktop retains its full setting set. `p4_settings_entry_flags` and
`p4_settings_entry_available` expose this metadata without changing stable model
indices. Registration labels
remain unchanged for callers; only the native presentation uppercases them.
Android watches nonvirtual keyboard devices only while Config is open, including
standalone numeric keypads but not controller-only button devices. Synthesized
virtual-pad keys do not expose keyboard slots. Script/virtual tests skip that
OS-device inventory entirely. A changed device list keeps surviving selections
stable and cancels an in-progress capture before remapping its cursor.

Callbacks own persistence (`p4cfg_set_str`), live application and feedback
(`p4_settings_message`). Keep feedback short for the single native help line;
full diagnostics are logged, with a concise log notice for longer messages.
Built-in video adjustments run at the present boundary; other actions run on the
native menu's game thread while gameplay is paused. An asynchronous system-dialog
callback must queue its result, never modify game state from the dialog thread.
Its formatter can consume that result only while `p4_settings_active()` is true;
after closing, defer it until the user returns to the menu/entry.

### Registered native pages

`p4pc/settings_pages.h` is the C interface for nested screens, virtual catalogs,
choice editors and long details. It has no scripting/package dependency. A root
action registered through `p4_settings_register` calls
`p4_settings_page_open("namespace.page")`; registering a page alone adds no root
row. The native Config still owns drawing, its seven row bars, toggle/OK widgets,
cursor, transitions, sounds, help font and button-glyph footer. CONFIRM remains
last on the root, and the original retail work block remains exactly 0x4b0 bytes.

Limits are explicit: **1024 screens**, **1,048,576 virtual rows per screen**,
**16 total view frames including Root/Controls**, and **256 root extension rows**
in addition to the 58 built-in model entries. The stable bounded registry stores
descriptors, not a catalog-sized array. Only seven visible rows and their toggle
timers are copied. Keys/targets are at most 127 bytes; titles/labels 191; values
127; help 2047; detail bodies 32767. Bounds include the terminating NUL in the
header constants. Invalid/oversized fields fail visibly, never silently truncate.

A list `P4SettingsPage` supplies:

- `count(userdata, &revision)`: nonnegative count and immutable snapshot revision.
- `row(userdata, index, &row)`: one visible row with a stable nonempty key,
  label/value/help, kind and flags. INFO is read-only; TOGGLE uses `checked`;
  VALUE accepts left/right; ACTION receives activation; LINK names another page.
  DISABLED refuses activation, RESTART marks help, and CURRENT marks the selected
  choice ACTION in the stock value pill.
- `find(userdata, stable_key)`: required direct lookup after a revision/filter
  change, not a native scan of a million-row provider. Return -1 if removed.
- `adjust(userdata, stable_key, direction)`: -1/+1 changes or 0 activates;
  return 0/-1 and report failures with `p4_settings_message`. It is optional for
  wholly read-only lists. A choice editor is an ordinary ACTION list that commits
  its stable key and calls `p4_settings_page_back()`, not a second widget array.

A detail page supplies `detail(userdata, &revision)` instead of list callbacks.
The full bounded body is copied, measured with the actual loaded ISO font and
word-wrapped into scrollable native rows. No text/row is lost at a page boundary.
Native glyph codes stay intact. Titles fit the native title area using the same
font metrics; long help has measured pages and a page counter.
Read-only rows keep the stock right-edge strip and cap. Its asset record has a
7-pixel base quad plus a 240-pixel `rightBorder`; the border is temporarily
widened by 133 pixels to fill the 380-pixel bar, then restored. Prose uses the
360-pixel padded text area. Q12 scaling does not widen the native border strip.

All display text uses the shared `native_glyphs.h` **RAW game-font encoding**:
printable ASCII or validated high-first native glyph pairs, not UTF-8, MSG
alternate-style codes or BMD commands. A command-looking trail byte such as the
`f6` in native space `83 f6` remains part of its glyph. Captions are single-line.
Only help/detail accept LF separators: the page layer validates and measures each
raw line separately, never passing LF to `frFontExCreateText`. Other control bytes
are refused, not folded. Clients must explicitly convert/validate external
metadata; no substitute font or silent byte reinterpretation is provided.

Navigation uses stock keyboard/pad bindings and touch row selection/activation,
with **Page Up/Down** or **L1/R1** for seven-row pages, **Home/End** for catalog
ends, and touch scrollbar rail jumps/swipes. **L2/R2**, or tapping the help strip,
pages long help. Back/Esc restores the parent cursor and scroll; revisions retain
the stable selected key or clamp to the nearest surviving row.

Registration copies descriptors but borrows their immutable key/title, callback
functions and userdata until successful unregister. Every returned row/detail
string may be reused at the very next provider callback: native code copies it
before then. `opened` runs once before querying an accepted view; a successful
view gets exactly one `closed` when popped/unregistered/closed, not when suspended
under a child. A failed `opened` is not paired with `closed`. Provider errors
refuse/close the affected view and never report action success.

`page_open/back/close` return 0 for an accepted asynchronous request, not proof
of visibility; `page_active()` reports the current registered page or NULL.
Field/title requests still respect the original menu gates and expire when
refused. Unregister is refused inside callbacks. Otherwise it synchronously
closes the affected stack and descendants (children first), cancels pending
references and drains callbacks, so userdata may be released on return. A stock
fade may still draw native-owned snapshots; it cannot dereference that userdata.
All API/provider calls are game-thread only.

**Hosted flows.** A game menu other than System or the title can present one
registered page flow with the same native Config task, not a second renderer.
Its override creates the real child with `cmpConfigCreate(task, 1)`, then calls
`p4_settings_native_host(handle, key)` (`p4pc/settings_native.h`) before the
child's first update. Title mode 1 is required: it draws Config's own opaque
backdrop and drives its own nLine transitions. Camp mode 0 is refused, because
outside Camp its close waits on an inactive nLine forever (observed in the
Velvet Room). The page is the top view: Back, Esc, touch Back, `page_back()` or
`page_close()` there close Config, and unregistering it does too. The Config
root, Controls and CONFIRM are never shown and the retail flags are not
synchronized. LINKs inside the flow behave normally. The parent reads no pad
edges until `cmpConfigIsStepDone`, then destroys the child; `cmpConfigDestroy`
closes the page stack, children first, then turns nLine off as
`cmpMainTaskDestroy` does, so the parent must not itself own nLine (Camp and the
title use the ordinary routes). A host request is refused while Config is open
or another request is pending, and in retail-only qualification. The parent
hides and restores its own header/footer sprites; Config does not know them.
`settings_host_velvet` runs this from the Velvet Room fusion menu's three-row
root (`tests/settings_host_velvet_gate.sh`, the level-99 Apollo save,
mod-witness lane): mode 0 refused, then list, detail, back and close, nLine off
again, a capture of the returned root compared with the root before hosting, and
ordinary root input.

The page change's mirror-census review is scoped to two Config units at decomp
pin `363eab8a`: `cmpConfig.c` changes its draw hunk count from 26 to 29 for native
page titles, read-only/disabled rows, value pills, measured help and the stock
Back-only footer; its toggle lookup hunks are unchanged. The `0035e820` unit
changes from nine to eight hunks: the input extension grows from 129 to 176
lines and transitions change from 16 to 15; a stale fallthrough-comment hunk
is removed and the destructor comment uses its now-canonical Config role.
Original cleanup statements are unchanged. The prototype pass still inserts
the same 16 declarations: only its whole-unit input hash changes. No other unit
or stage is refreshed, and normal mirror generation retains strict validation.
Provider descriptors stay outside `settings_native.h`, so game units never see
their host pointer fields. The abandoned include exposed five `PageRow` pointers
and ten `Page` pointers (seven callbacks plus key/title/userdata); the rewriter's
foreign `FieldDecl` guard counted all 15 without rewriting them. Removing that
unnecessary dependency avoids updating the seven affected port64 skip censuses.
The native row carries only an explicitly mapped disabled bit, not the provider
descriptor flags.
The INFO helper consumes the opaque SDK sample pointer already handed to the
caller, without storing host pointers in the sidecar or adding a pointer-to-
integer conversion. No port64 rule baseline is relaxed.


## One MODS hub

The C-owned **MODS** root row opens metadata-only catalogs of the configured
player-mod and engine-pack directories, including inactive and invalid
installations. Each uses its own configured-root resolver, never a second
UI-specific path policy. Absent default `mods` or `packs` directories are
empty catalogs; explicitly configured missing or invalid roots remain errors.
Opening Config or formatting its root row does not scan packages, prepare
payloads, publish a VFS generation or create a VM.
Opening the hub explicitly performs discovery. Metadata is not a claim that a
package's payload is usable. Package information remains in the game's own font;
non-ASCII/control metadata bytes are shown losslessly as `\xNN`, not truncated
or rendered through a fallback canvas.
Pack rows follow mod rows and use the `PACK` label. Their independent
`packs.enabled` toggle is persisted for the next restart; it never modifies the
mod selector or its load order. Invalid and duplicate pack IDs cannot be
selected. Pack discovery does not qualify or activate native artwork/audio
handlers that the runtime does not provide.

Active player-mod entries expose their attached registered pages. Native
services occupy a separate service namespace in the same hub; a service never
impersonates an installed mod. Keyboard, pad,
touch, history, descriptions and transitions are the existing native page path.
Four C-owned pages (`mods`, `mods/installation`, `mods/details`, `mods/restart`)
count toward the shared page limit; there is exactly one permanent
settings-model root row.

Include `p4pc/mod_settings.h`. All calls are game-thread-only:

```c
int p4_mod_settings_attach_page(const void *owner, P4ModSettingsTarget kind,
    const char *id, const char *page_key, const char *raw_label);
int p4_mod_settings_detach_page(const void *owner, const char *page_key);
int p4_mod_settings_detach_owner(const void *owner);
```

Register the real `P4SettingsPage` first. `attach_page` copies the id, target key
and native RAW caption (64/128/192 bytes including NUL), returns 0/-1 and refuses
duplicate targets. `owner` is a non-NULL identity cookie, never dereferenced and
never used as permanent root userdata. Lua uses `P4_MOD_SETTINGS_MOD` with its
actual mod id; its registered keys use `lua/<id>/<local-key>`. C services use
`P4_MOD_SETTINGS_SERVICE` and a distinct service id, e.g. `content-packs`.
Services cannot attach targets in the reserved `lua/` namespace.

Detach synchronously removes links and advances the hub revision, but does
**not** unregister pages or invoke VM code. `detach_page` returns 0/-1;
`detach_owner` returns the removed count (zero is success), or -1 for NULL.
Before releasing a VM, unlink its owner, successfully unregister every owned
page **outside provider callbacks**, and only then release Lua references/VM.
Page unregister closes affected views and pending requests synchronously.
An unload requested inside a callback must be deferred. Do not put VM pointers,
captions or callbacks into per-mod permanent `p4_settings_register` entries.
`p4_mod_settings_init()` is idempotent registration only; Config initializes it
after the retail-only qualification guard, and attachment initializes it before
linking. Discovery is still deferred to the actual hub's `opened` callback.

## Regression coverage

`settings_native` (`tests/settings_native_gate.sh`) boots the actual game and
opens its Config task through tagged SDL touch/key events and native pad/list
input. It checks a refused shortcut is not replayed, changes Resolution to
`scale:2`, checks the live 1280x896 GLES target, rebinds Cross to K, checks the
actual keyboard-to-pad mapping, cancels a conflicting binding and closes back
to the field. Its private INI must retain comments/unknown keys and contain the
committed resolution/binding, with no temporary file left behind.
It also counts one synthetic save directory beside an unrelated directory,
without needing a private import source. On Windows the counter enumerates a
`\\?\` root, so this guards normalization of the joined child path before the
directory attribute query; a delta-only import check cannot catch a counter
that always returns zero.
Whitespace-separated `;`/`#` inline comments are not part of cached values:
the parser and atomic writer share this rule, while preserving their file bytes.
Captures include the native root, resolution, Controls, conflict and closed
field. It requires the user's ISO/ELF and scripted input with dummy audio;
ordinary gates never opt into settings input.
The gate uses `P4PC_TURBO=2`: game/menu updates and live GL changes still run,
but only the normal 30-flip warmup before each inspected PNG is rasterized.
Capture scheduling respects `P4PC_MENU_STEP` and re-arms when a row/title retry
finishes; it never predicts through an unresolved menu gate. Distinct row
screenshots must differ, so a stale but nonblack framebuffer cannot pass.
The active native Config task follows the same capture windows, not the retired
overlay's forced-redraw path. An in-menu `draw-skipped` assertion after the first
capture checks the actual GLES draw state while Config remains open.
`P4PC_SETTINGS_RETAIL_ONLY=1` is the qualification path for the original seven-row
Config: no sidecar, extra resources or PC rows are allocated, so its retail
input/drawing behavior can be compared in lockstep.
The caller qualification uses `ovdiff --paired --paired-device -R ... -W ... -G`:
the original EE body and the override receive the same recorded entry state,
with identical native callees and the production headless RW raster lock/upload
path. Return values, callback logs and touched retail guest memory are exact;
strict golden replay remains a separate, unchanged oracle. See the
[paired boundary](../../tools/mipsdiff/replay_notes.md#paired-caller-differential).

The native witness also cycles the registered Aspect ratio through 16:9, auto
and 4:3, checks its restart marker and unchanged running target, and verifies
the first touch selects while the second changes a value.
The registered **Movie fit** row cycles bars, zoom and stretch without a restart
marker. Each phase checks the native row, formatted choice, live typed getter
and a reload of the saved INI; closing must retain stretch. Windows and phone
fixtures capture all three choices (`settings_movie_*` / `native_movie_*`).
An absolute second argument to `settings_native_gate.sh` retains those Windows
PNGs, the saved INI and boot log outside the gate's successful scratch cleanup.
`settings_render`
retains the real-context Resolution policy/transaction tests without a UI
canvas: preset/native/scale targets, hardware-limit rejection before persistence,
an actual blocked `.ini.tmp` write, unchanged framebuffer/pixel/bindings/viewport
and config on failure, and ordered, exactly-once deferred provider callbacks.

Title entry is state-driven: the script skips preceding logos/movie with normal
Start only until the game's real three-entry selector is ready. It never forces
an early Config task. `tests/settings_native_mobile.script` is the phone's
repeatable native fixture: platform availability, scale, pad-only Controls,
touch-scroll ON/OFF and two-row swipes, Movie fit, Import, and title/field open/close.
Set its text as `P4PC_MENU_SCRIPT` in an opted-in private Android run; setting
`P4PC_CAPTURE_WINDOW=1` also retains the actual fitted window surface.
Its private INI starts with `[video] movie_fit = bars`; clear `P4PC_MOVIE_FIT`
so the environment cannot mask the live and persisted choice checks.

`import_settings_native` uses a genuine private `P4PC_IMPORT_SOURCE` and the
actual Config row to check cancellation makes no saves, a picker completion
while closed stays deferred, and returning to the native menu imports it.
Its opt-in Android folder phase activates the real row rather than opening a
desktop dialog. All screenshots and imported cards remain private gate artifacts.

`settings_pages` / `settings_pages64` run `tests/settings_pages.script` through
the production Config and a deliberately reusable-buffer C provider. Coverage
includes 37 toggles, the million-row boundary without catalog materialization,
stable-key revision/removal, native detail/help paging, current choice/commit,
the 16-frame limit, callback-unregister refusal, synchronous owning-page
unregister and descendant close order, empty-page navigation without querying a
nonexistent provider row, malformed row/raw-help refusal and closing back to the
field. The detail fixture includes native `83 f6` space glyphs and LF paragraphs.
`P4PC_SETTINGS_PAGES_TEST=1` opts into this fixture; ordinary runs register
nothing. The same script is used for phone captures. The gate isolates its INI
and save directory; its optional output directory retains all 14 PNGs and log.
Capture comparisons use the project's dependency-free Python PNG support, not an
optional `cmp` executable whose absence could be mistaken for different images.
Pixel probes at the INFO band's right edge guard readable white-on-black text
across the complete selected row.
`settings_render[64]` also exercises screen/extension registry capacities,
duplicate/invalid registration and cancellation of an unconsumed open request.

