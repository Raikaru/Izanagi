# Testing and harnesses

[Platform index](../platform.md) · [Contributing](../../CONTRIBUTING.md)

## Headless execution

Build and run the tests in the `p4pc-ci` container ([BUILDING.md](../../BUILDING.md));
`sh tools/ci_tests.sh BUILD` runs the suite the way CI does. Unattended runs use hidden
windows, dummy audio and script input; Linux uses SDL offscreen or a hidden Xvfb context.
Do not show windows, play audio or sample physical controllers on an unattended desktop.

Every automated game run requires `P4PC_INPUT=script`; runners force it after caller
and CTest overrides. Use `P4PC_CONFIG=none`, private writable saves and a bounded execution
deadline. `P4PC_HIDDEN_WINDOW=1` gives Windows a hidden GL context.
`P4PC_OFFSCREEN=1` omits swaps but can change edge pixels; retain the gate's own draw policy.
Missing private data produces an explicit skip (77) only where the test declares one.
A missing required file during an opted-in check is not automatically a pass.

`ctest --test-dir BUILD --output-on-failure` runs the configured tests. Host-only CTests
and scene gates remain distinct. i386 and 64-bit twin tests must keep identical guest
contracts, not reinterpret host pointers as guest words.

## Viewer and renderer smoke

Both run the renderer's default GLES 3.0 subset or `P4PC_GL_API=gl` desktop GL 3.3.
`render_smoke` links real port/decomp objects, boots the retail image, loads a TMX through
VFS, checks raster lock/unlock bytes, draws an Im2D textured quad into a readback FBO and
checks sampled pixels. It requires `P4PC_ISO`, `P4PC_ELF` and a context, and writes
`render_smoke.png`. It also covers undrawn submission and resolution invariants.

`p4pc_viewer SLUS_217.82 field/rmd/m001_002.RMD [out.png]` asynchronously loads a game
model, pumps pending disc requests, fits the camera to clump bounding spheres, renders
through the game's atomic path, reads RGBA8 and writes PNG. `pack.bin#name.RMD` selects a
packed model (for example `title/title.bin#tc01.RMD`); `ptank` builds the retail PTank
create chain. Field mode uses `field:F:R`.

Streamed title/bare-RMD modes initialize real CRI devices, volumes and the load worker
through `kwlnInitSubsystems`. Field mode already performs that setup and must not create
a second worker. Packed-memory/procedural modes retain lightweight initialization.
Status getters only observe; the viewer must not make them pump CRI.

## Captures and diagnostics

- `P4PC_CAPTURE_FRAMES=a,b,c` with `P4PC_CAPTURE_PNG=path_%d.png` captures immediately
  before those presented flips swap. `P4PC_CAPTURE_RGB` writes private raw RGB references;
  `P4PC_CAPTURE_WINDOW=1` also writes the fitted drawable as `.win.png`.
- `P4PC_DUMP_TASKS_FRAMES=a,b,c` invokes the game's task-tree dump at those flips.
- `P4PC_DUMP_RAM=<prefix>` and `P4PC_DUMP_RAM_AT=a,b,c` retain guest memory for
  `tools/ram_diff.py`. See [numeric diagnosis](numeric_determinism.md).
- `P4PC_FIELD_ENTRY_DUMP` observes field-entry state; selected capture rows are not
  substitutes for readiness. See [disc observers](disc_cri.md#observers-and-regression-coverage).
- `P4PC_HEAP_CHECK` enables guarded guest allocation; see [heap](threads_kernel.md#startup-and-memory).
- `P4PC_PROFILE`, `P4PC_PROFILE_EVERY`, `P4PC_PROFILE_HANG`, `P4PC_GLSTATS` and draw
  windows are specified in [performance](performance.md).

The remote boot runner accepts GDB controls through `--env`:

- `P4PC_GDB_TRACE=f1,f2`: calls, four argument words and a three-frame backtrace;
- `P4PC_GDB_RET=f1,f2`: calls with two argument words and return values;
- `P4PC_GDB_WATCH=addr`: writes to one word;
- `P4PC_GDB_WATCHRET=f:N`: stop when the Nth call's return-address slot changes;
- `P4PC_GDB_X=addr[:words],...`: memory after a signal, also no-space GDB expressions
  evaluated in frame 0, such as `*(int*)($ebp+8):24`;
- `P4PC_GDB_FPU=loc,loc`: x87/MXCSR at code locations;
- `P4PC_GDB_SNAPSHOT=seconds`: interrupt a hang and print every thread;
- `P4PC_GDB_SYM=0xaddr`: resolve a reported allocation/call-site PC.

Failure artifacts are immutable per attempt. Keep complete logs, invocation metadata,
RAM, produced frames and save/trace evidence. A later retry cannot replace an earlier
failure. Do not commit private references or captures. Reference filename/hash pairs in
`tests/refs.sha256` are immutable; see CONTRIBUTING for new-reference cutovers.

## Title and battle drivers

The native boot sequence is memory-card check (30), data boot chain (1), then gameSequence
(27): logos, opening movie, title. The sequence task handle is at `0x00764298`; callback
records are at `0x005dcc64`, stride `0x50`. Table debug labels do not reliably name behavior.
Movie task skip/end rules are in [movies](movies.md).

`P4PC_AUTOPRESS_NEWGAME=T` drives Start through opening/title, waits for the real title
state (PRESS ANY BUTTON state 7, menu state 9), moves to NEW GAME and confirms. Its
confirm sets the AUTOPRESS script clock to T, matching retail `--script-after-newgame T`.
`P4PC_AUTOPRESS_LOAD=T` selects LOAD GAME the same way. State-driven selection avoids
assuming title loading takes a fixed number of flips. The retained New Game schedule is:

```text
start@600:2,start@1100:2,down@1500:2,cross@1560:2,cross@2300:2/150,start@6500:2/300
```

`P4PC_NEWGAME_GATE=flip[,field:room[,month/day]]` exits at the selected flip only if the
actual location/date and optional `P4PC_REF_RGB` / `P4PC_REF_MAX` image requirement pass.
`P4PC_GATE_REQUIRE_SAVE=1` also requires completed walk and Save OK where registered.
A date/TV-entrance endpoint after defeat is not evidence that an asserted dungeon floor
was reached. Preserve actual live floor-step/resource waits in the canonical walk.

`P4PC_BATTLE_DRIVER=button,...` supplies a repeated command plan; `P4PC_BATTLE_FIRST`
selects first-command input and `P4PC_BATTLE_SKIP` covers initial tutorial text.
`P4PC_BATTLE_LOG=1` logs state/menu/pulses. Plans arm only for a party action in native
COMMAND state 5, battle context state 8, with ACTION_RUNNING clear, not AI/STANDBY state 1.
The standalone driver spaces plan pulses by 14 flips; noncommand Help/result input uses
its separate cadence. An armed macro may continue through TARGET state 6 only for the
same action pointer. Actor handoff retires it; running actions never accept it.

## Menu and battle harnesses

`P4PC_START_MENU=field[:room]` starts FIELD RECORD sequence 9 with new-game state after
boot data and logo work are ready. `P4PC_MENU_SCRIPT` is comma-separated:
`open` retries Triangle until camp is active, button names hold 6 flips then settle for
`P4PC_MENU_STEP` (default 45), `waitN`, `shot:name`, and `state`.
Without `P4PC_START_MENU`, `P4PC_MENU_SCRIPT` observes natural ISO startup instead
of forcing a field. `settings-title-ready` drives the real logo/movie/title
selector and waits for readiness; package smokes use this path before exiting.
`P4PC_MENU_EXIT=1` requires camp opened or native settings checks passed, with
every shot lit and every PNG successfully written.
Android relative captures resolve inside app-writable internal storage without changing
cwd. `camp_menu_write_failure` requires nonzero exit for a failed real PNG write.

Save tokens are `save`, `load`, `mod`, `reset`, `keep:name`, `check:saved:reset:loaded`,
`dump:name`, `vsfile:path` and `totitle`. Keep/check compare the game's built save image,
not a second serialization. See [card coverage](saves_memcard.md#tests-and-private-fixtures).

`P4PC_START_BATTLE=encounter` creates party/enemy units and requests sequence 6 using the
same encounter-start body as the game script. `P4PC_START_BATTLE_AT` is a minimum delay
(default 10), not permission to destroy pending logo work. Unless
`P4PC_START_BATTLE_KEEPSAVE` is set, it initializes new-game state.
`P4PC_BATTLE_HERO_LEVEL` sets real level and fills HP/SP; `P4PC_BATTLE_HERO_HP` changes
only current HP, still clamped by the game's maximum. Logs expose state/HP/SP, packet lists,
camera, lights and fades. `P4PC_BATTLE_GATE` tests the registered state/frame conditions;
a harness encounter is not a complete story-combat witness.

### Offline battle batch coordinator

`tools/battle_sim.py` coordinates the actual native battle worker; it contains no
combat resolver. Use `batch --scenario SCENARIO.json --worker EXE ELF --out OUT`
or `replay --scenario SCENARIO.json --worker EXE ELF --index RUN_INDEX --out REPLAY`.
Each output directory must be private and initially empty. `--resume` accepts only
the identical scenario, coordinator source, and worker/resource file fingerprints;
completed indices, including recorded errors, are retained. Interrupted attempt directories are
preserved separately. Replay is a balance-seeded rerun, not captured-state fidelity
replay; the original run index and master seed derive the same stream seed patterns.
The native trace is retained for every attempt, including full per-turn events on
replay. `--rendered` is an explicitly GPU-admitted replay reference.

The JSON schema is version 1. Required fields:

```json
{
  "schema": 1,
  "entry": {
    "source": {
      "kind": "save",
      "id": "immutable-private-source-id",
      "image": "native-save-image.bin",
      "sha256": "REPLACE_WITH_ACTUAL_LOWERCASE_SHA256"
    }
  },
  "encounter_id": 1,
  "policy": {
    "kind": "tape",
    "version": 1,
    "tape": [{"update": 3, "buttons": 16384, "hold": 2}]
  },
  "master_seed": "000102030405060708090a0b0c0d0e0f",
  "seed_start": 0,
  "n": 20,
  "options": {
    "update_limit": 100000,
    "bootstrap_limit": 100000,
    "action_limit": 10000,
    "latency_updates": 1,
    "fill_resources": false,
    "timeout_seconds": 120
  }
}
```

This is a format illustration, **not a qualified encounter fixture or action
tape**. Encounter/source/timing must come from an actual supported native entry.
The source image is the complete native save image including its version, not a
card container or summary; native code validates its layout and checksum.
Image paths resolve relative to the scenario file. Source `kind: "walker"` also
requires `parent_id` and `walk_sha256`. A source `kind: "declarative"` carries
the structured native preparation definition below. The coordinator encodes
`P4BPRE01`, invokes the actual native preparation worker once, verifies its
bound receipt, and freezes the resulting complete image before seed execution.
It never edits save records or implements native growth/serialization in Python.
The native preparation implementation still requires its own exercised receipt;
fake-worker protocol tests are not proof of native preparation qualification.
Other policy kinds are `greedy`, `cautious`, and `typical`, version 1, without a
tape; unsupported native capabilities must return an error, never a fallback.
Optional entry `context` and `rules` objects are retained and hashed as provenance,
not interpreted as memory edits by Python. Limits and explicit resource filling
are worker options; no implicit healing or preparation is performed.
Resource cohorts explicitly use `native_inherit` when `fill_resources` is false,
or `declared_fill_living_hp_sp` when true. Native initial-party snapshots retain
their reported boundary: phase 1 uses `after_restore_before_constructor`, not
an effective first-COMMAND/turn-start snapshot. Source HP/SP of 999/999 alone is
not evidence of an illegal cohort: the ordinary engine may inherit that value
at first COMMAND and later clamp through its real zero-delta turn-start packet.
The coordinator never forces such a clamp, repairs the source, or silently
interprets a preconstruction snapshot as the resources for the first action.
Both rendering modes pin the private process configuration to 4:3, 640x448,
windowed, direct presentation, with vsync off. Inherited desktop-aspect settings
cannot change reference versus no-device camera/culling work.
Inherited `P4PC_*` and `SDL_*` variables are removed before those fixed overrides
are installed. Input is script-only, content packs/mods are off, audio is dummy,
and the native clock/turbo policy is explicit. To supply a disc, declare optional
`runtime.iso` with `path` (relative to the scenario or absolute) and lowercase
`sha256`; the disc is stream-hashed once before the batch, not once per seed.
The separate `runtime.elf` object has the same `path`/`sha256` fields and must
match the explicit native worker ELF argument. Reports label the actual native
port binary as `identity.port_executable`; retail ELF/ISO fingerprints live
under `identity.runtime.elf`/`iso`. These are different byte artifacts, never
interchangeable build pins. All explicit worker file arguments are also hashed.
Optional `runtime.video_driver` explicitly selects `SDL_VIDEODRIVER`. Arbitrary
environment maps are rejected: inherited walkers, board/RNG seeds, captures,
lockstep sessions, and player settings cannot become hidden batch inputs.

Stream tags 1/2/3/4 denote additive/MT/policy/effect. Schema-1 derivation is
SHA-256 of `P4PC-BATTLE-SEEDS`, NUL, the master seed's 16 bytes, LE64 index,
and LE32 tag, taking digest bytes 0..3 as LE32. Jobs/order never enter seed
derivation. MT's `seed|1` collisions are reported separately.
Policy tag 3 is retained but deliberately unused by deterministic version-1
policies (lapses off); it is never substituted for an engine RNG stream.
Native `rng.draws` / trace `rng_draws` count only post-seeding additive-stream
draws, not MT/effect/private calls. The phase-1 worker labels its entry
`harness-script6` and its trace `phase1-scoped-state-rng`; scoped trace hashes
are not full authoritative-state fidelity proof. Reports retain those native
capability labels and separate RNG state/bank evidence without broadening claims.

Default concurrency is one; `--jobs` is an explicit maximum, not a measurement
of safe admitted host capacity. Each fresh process (including the single retry
after a crash, timeout, or invalid/missing publication) independently acquires
`Machine(priority="study").case(wait, cpu=True)`. Lands have priority between
cases; the worker deadline starts only after admission. Production admission
currently follows the host support of `local_gates`; game-free tests inject
admission and run on Windows/Linux. No unmanaged native launch is substituted
on a host lacking that admission implementation.

`report.json` retains sorted per-index results, both attempt receipts/logs,
fingerprints, actual stream seeds, native RNG observations, win rate/Wilson 95%
interval, resolved-only rate, unresolved best/worst bounds, turn/resource
percentiles/histograms, terminal wipe causes and ranked replay indices.
Completed means a non-error/non-limit native outcome; escape/scripted outcomes
remain in the completed denominator. Resolved-only means win/defeat. Limits
are censored, never defeats; no seed is silently replaced. Missing native
measurements are unavailable, never synthesized from updates or HP polls.
All-requested worst/best win bounds vary only error/censored outcomes; observed
escape/scripted conclusions remain known non-wins, not hypothetical victories.
Measurements live only under `measurement_cohorts`, each containing its native
`contract`, sorted `run_indices`, and separate `turns`/`resources` statistics for
`all`, `win`, and `defeat`. No global pooled turn/resource distribution is emitted:
different capability versions, observer attachment, counter definitions, resource
boundaries, or recovery scopes form separate cohorts. Missing historical labels
remain `unreported`, never retroactively assigned phase-2 meanings. Replay ranking
omits counter/resource comparisons when resolved runs have different contracts.
Within one comparable native contract, replay extremes rank by win, fewer
reported actor turns, more total reported final-party HP, then lower index.
Missing counters/party snapshots are omitted, never replaced with zero health.
Native `party_policy` identity, latency, control and knowledge metadata and entry
observation boundary labels also participate in the measurement contract.
`entry_observations.first_protagonist_command` retains the separate
`native_input_ready` observation; when absent, update and party must be null.
Whether that observation occurred is not used to split otherwise identical
counter cohorts, and missing observations never become zero-resource snapshots.

Definition strings are metadata, not counters. The source-only phase-2 contract
defines `actor_turns` as `ordinary_queue_admissions_including_skips` and `actions`
as `ready_to_execution_attempts_excluding_persona_analyze`; those are not uniform
rounds, completed hits, or guaranteed paid actions. The native spend definition
uses computed party skill costs and actual item decrements. Requested recovery/
drain and effective HP/SP adjustments remain separate metrics, so a native
999-to-maximum adjustment is not invented spend. `rounds: null` stays unavailable.
These parser/cohort tests do not qualify an unexercised native metrics build.
`summary.md` is the short human-readable companion. The fake-worker contract
suite is `python tests/test_battle_sim.py` / CTest `battle_sim_coordinator`;
it needs no game assets. A fake batch is not native acceptance evidence.

#### Native declared-party preparation

For a declarative source, `entry.source` contains only `kind`, `id`, and
`definition`. The following definition illustrates source-attested selection;
the baseline filename must refer to the actual complete private payload whose
hash is supplied. This is not a native qualification fixture.

```json
{
  "schema": 1,
  "classification": "source_attested",
  "baseline": {
    "kind": "save",
    "id": "immutable-source-id",
    "image": "native-save-image.bin",
    "sha256": "8da917dffc4e70151d84a47b4a8923af7fb72df9c8592fdfd0b58a6d13d59d56"
  },
  "preparation": {
    "master_seed": "fedcba98765432100123456789abcdef",
    "resource_mode": "inherit",
    "clear_temporary_statuses": false,
    "bootstrap_limit": 10000,
    "field_resume_limit": 100000,
    "timeout_seconds": 120
  },
  "context": {},
  "party": {
    "hero": {"mode": "preserve"},
    "roster_mode": "preserve",
    "selected_slot": "preserve",
    "actors": [{
      "id": 1,
      "equipment": {"weapon": "preserve", "armor": "preserve", "accessory": "preserve"},
      "resources": {"mode": "preserve"},
      "tactic": "preserve"
    }],
    "personas": []
  },
  "inventory": {"mode": "preserve"}
}
```

`classification` is `source_attested` or `synthetic_lab`. Source-attested
selection/preservation binds records and stock to the source; it is **not**
proof that an edited collection save or arbitrary source was legally attained.
Synthetic level/stat/skill/gear/item grants remain synthetic, never relabeled as
obtained in play. Native validation owns recruitment, ID/catalog legality,
consistent EXP, ownership, equipment transactions and actual progression.
No general fusion/acquisition solver is implied or required.

- `baseline` accepts the same save/walker reference and immutable hashes as a
  direct source. `context` contains optional native numeric assertions `date`,
  `time_slot`, `field`, `room`, `step`; it does not set story flags.
- Preparation uses its own declared master seed, schema-1 index zero and native
  additive/MT/effect seed owners. Its output is frozen once, never grown again
  for each battle seed. Bootstrap/field-resume limits are 1..10,000,000 updates.
- `resource_mode` is `inherit` or `fill_living_hp_sp`. Actor resources are
  `{"mode":"preserve"}` or `{"mode":"explicit","hp":N,"sp":N}`, with native
  0..999 wire bounds and native death/resource validation. Global fill plus any
  explicit actor resource request is rejected as `competing_resource_policies`;
  there is no invented precedence. Fill does not silently revive. The battle's
  separate `options.fill_resources` profile applies when restoring the frozen
  entry; the preparation receipt records the baseline-to-frozen resource policy.
- Actor order declares formation: 1..4 unique native combatants, protagonist 1
  first, IDs drawn from 1,2,3,4,6,7,8 (navigator 5 is not a combatant). Equipment
  IDs or `"preserve"` and tactic 0..4 or `"preserve"` are explicit.
- `hero.mode` is `preserve`, `assert_source` with `level`/`exp`, or
  `synthetic_exact` with consistent native `level`/`exp`. `roster_mode` is a
  native mode 0..3 or `"preserve"`; `selected_slot` is 0..11 or `"preserve"`.
- A persona record has `owner` (native 1..10), `slot` (hero 0..11, companions 0),
  and `mode`. Any hero records define the **complete** carried roster in
  contiguous slot order 0..n-1, not overlays; no hero records preserve it.
  Companion records overlay their actual owner; absent owners preserve.
  An original selected slot must still be valid or preparation fails.
- `preserve_source` requires `source_slot` (hero 0..11, companions 0), with
  optional `persona_id` and `assertions`. An omitted ID resolves from the actual
  source record. Assertions may contain paired `level`/`exp`, eight `skills`,
  five native-order stat bytes, and paired five-byte `bonus_a`/`bonus_b`.
- `synthetic_exact` requires `persona_id` and `record` containing `level`, `exp`,
  eight `skills`, and five `stats`. Optional paired bonus arrays must be zero.
  Real native initialization/setters own construction; it is not a save patch.
- `native_growth` requires `persona_id` and `growth` containing `origin`
  (`source_record` or `native_init`), target `level` and `exp`. Source origin
  requires `source_slot`; native init omits it. Optional eight `starting_skills`
  explicitly replace synthetic starting skills. Optional `assertions` check
  final skills/stats/bonus arrays; they never assign predicted growth.
  Ordered `decisions` contain `level`, `learned_skill`, `replacement_slot`
  (0..7 or `"auto"`), and `removed_skill` (zero for empty).
- Skills have exactly eight slots; empty is **0**, not 0xFFFF, and nonempty skill
  IDs are 1..4095 (including the 576..4095 expansion). Leaf/channel registry IDs
  are a separate full-width concept, not this packed native skill domain.
  Persona arrays have at most 21 records and 4,096 total skill decisions.
  Source-attested persona requests preserve records; new exact/grown records
  are synthetic.
- `inventory.mode` is `preserve` or `exact_battle_consumables`, the latter with
  unique `{id,count}` entries (at most 1,536; counts 0..99). Unrelated gear/key/
  story stock survives. Native kind/stock bounds and attested ownership are
  checked independently; a table size is not permission to expand inventory.

Native invocation is `EXE ELF --battle-prepare REQUEST --battle-prepared-image
EFFECTIVE_SAVE --battle-prepare-result RECEIPT`. Every preparation attempt is a
fresh CPU-admitted, isolated process with the deadline beginning after admission.
The encoder emits the native-owned 160-byte header and exact 64/112/8/24-byte
records, including zero reserved fields and derived contiguous decision spans.
It does not clone the native serializer or section/checksum validator.

Successful acceptance requires exit zero, `schema:1`, `operation:battle_prepare`,
`outcome:prepared`, terminal cause `native_save_build_image_complete`, the exact
classification and definition/request/source hashes, actual effective-image
hash/length, and literal `effective_image_native_valid:true`. That attestation
comes from the native canonical validator before publication; every battle
ENTRY independently runs the same native validator again. Exit zero or an
unbound image hash alone is insufficient.

`preparation_error` is a recorded native failure: retain its receipt and stop
before any seed job, without retrying generation or falling back to a source
party. Crashes/timeouts/missing or invalid publications get one infrastructure
retry with both attempts retained. Resume reuses `preparation/frozen.json` and
the verified image, or recovers a complete atomic native publication. Changed
frozen artifacts fail instead of regenerating a different party.

Declarative replay requires `--prepared-from ORIGINAL_BATCH_OUT`; it reuses the
original frozen image and never repeats growth. Reports retain the original
definition/source classification, native preparation receipt and effective-image
identity separately. Preparation remains native-backend qualification work
until exercised; game-free fake receipts never become native evidence.

## Shared walker invariants

`P4PC_WALK_SCRIPT` uses the same controller source as retail. Walk files live in
`tests/walks/`; `P4PC_WALK_LOG` and `P4PC_WALK_LOG_EVERY` expose progress.
Do not add timing-dependent alternative plans, retime oracles, normalize RNG/state, or
replace canonical cards as a workaround for PC-speed loading. Input readiness follows
native tasks/menus, not estimated disc duration.

- Field navigation is available only while `FieldSceneState.sceneTask` (`+0x1c`) is
  published. A surviving player script releases held input and pauses across unload;
  it makes no collision queries or NAV_VOID cache entries for absent geometry.
  Title/card input without a player continues. `nav_probe` covers these boundaries.
- `skiptask:NAME[:off][:BUTTON]` retries the chosen button (Cross default, Triangle for
  camp) every 24 frames until the task condition holds, then releases it.
  Passive `waittask:NAME` observes readiness; `:off` waits for disappearance.
- `waitchoice` advances only actual nonchoice text and completes with neutral input
  at a ready native selection. It releases input while any choice is preparing,
  even if another caption is live. Field BF text requires event input ownership
  outside a battle; battle Help retains its strict text predicate. `choose:N`
  still commits the selected answer.
- `waitbtl:menu[:ID]` requires 40 fresh COMMAND-ready frames; optional ID is character
  1..8, not party-chain position. It advances actual nonchoice Help text, not command,
  target or live/preparing choices, and declines earlier All-Out while waiting.
- `battle:first1` through `first8` override first plans by character. Full directives are
  retained in 511-byte command storage, including delay and cancellation keys.
  `plan1` through `plan8` override only later repeats for the same native character IDs;
  first-plan precedence stays unchanged and charge Guard has priority over both.
  Unlisted IDs still use the shared `plan`. For example, `first3=circle,cross,cross`
  with `plan3=circle,cross,cross` restores Chie's weapon after Guard while other
  actors reopen Garu or Media. TARGET continuation and target bindings still apply.
  `persona=modal` gives the native Persona-status handoff its own Cross input instead
  of moving a still-live Shuffle grid. Omission keeps the existing ordinary cadence.
- `waitstate16:ADDR==V` / `!=V` reads either aligned halfword, with odd/out-of-range
  rejection; `waitstate` retains its 32-bit predicate. Avoid comparing adjacent flags
  as part of a Persona ID or inventory byte.
- `walk:goal` uses the live upstairs quad; `walk:entry` uses downstairs `0x37fe`, never
  an upstairs fallback. Both are scoped to the initial field and dungeon step and retire
  on transition. An optional tolerance (for example `walk:goal,40`) is an input choice,
  not a change to collision or event polygons.
- `push:X,Z,N` expires after N frames even if an automatically triggered event
  removes player control. It emits no movement while control is absent and does
  not hold the next dialogue command hostage to the event returning control.
- `waitbtl:dead:ID` requires matching native enemy records with zero u16 HP, including
  all duplicate IDs. An absent/unlinked record is not proof of a kill during defeat cleanup.
- Explicit `battle:...;guard=ID` observes the living enemy's Power/Mind Charge flags
  (`DatUnit +0x14`, mask `0xc0000`) at a fresh command and sends real Circle/Up/Cross.
  Its choice stays fixed during the macro, and Guard does not consume first plans.
  Repeat plans must reopen their intended wheel command after Guard changes it.
  `guard=ID:PC,PC` limits that override to listed native character IDs 1..8; bare
  `guard=ID` retains all-party behavior, independently of which companions are alive.
- `target1=ID` through `target8=ID` bind that character's TARGET confirmations to a
  living native enemy ID. Fresh Right input follows the native target list before
  confirming; an all-target skill can confirm when that enemy is included. Dead,
  absent or unreachable IDs fail loudly, with no fallback or game-state write.
  Remove or rebind the option before ally-targeted healing or after the enemy dies;
  charge Guard bypasses target bindings. Omitted bindings preserve existing input.

Save-anchored story legs start from the nearest pristine collection checkpoint before
the content, or a private input-produced descendant with recorded provenance. Keep
these legs distinct from the New Game chain. Fusion steps check the actual born
Persona ID and required inherited skills before native saving; a preview alone is not
success. Shop and equipment preparation use the card's real stock and balance.
Predicates only read game state: the walker never normalizes inventory, HP, results or RNG.
If a boss remains unqualified, restarting after it establishes only that later leg:
a subsequent native save or date goal does not certify the missing fight or a
continuous playthrough.
Summer and later collection legs can begin after an unqualified boss. Preserve the
pristine collection identity in each registration; do not replace it with an unrelated
post-fight card while claiming combat continuity.
The Secret Laboratory entrance gate requires Rise's native destination selection,
live exterior step 80 and restored player control. It does not cover the laboratory
floors or Shadow Naoto.
The independent B9F collection gate covers its live step 89, interior door and
Shadow Naoto's first protagonist COMMAND. It does not join the exterior anchor
into a continuous traversal of floors B1F–B8F or certify the boss fight.

October collection legs restart from post-rescue cards; their automatic school
story and native saves do not turn the unqualified Naoto fight into a victory
witness. The exam gate crosses both exam blocks and their intervening Sunday
evening; it does not require a particular exam grade. The two festival legs use
separate pristine evening anchors: the first festival day's home save and the
remaining automatic story's classroom save. Each leg requires completed input
and native Save OK, not just a matching date.

The November4 school anchor covers the abduction investigation, observed
deductions, the actual Fool9 overlay and Enter the TV selection. It finishes at
Heaven's Pearly Gates with live step 100, no event and player control. This is
an entrance witness, not traversal of Heaven or a rescue victory.

The independent Heaven tenth-floor gate checks its live step, closed door and
Namatame's information topics before the first fresh party COMMAND. It does not
certify Heaven's earlier floors or a Kunino Sagiri victory.

November's return leg likewise uses an independent post-rescue collection card.
Its automatic story and native home save do not establish traversal of Heaven's
preceding floors or a Kunino Sagiri victory.

Both hospital branches start from the same requested unchanged MAX18657
December2 card. The continue branch gives all six observed correct answers
and requires completed input plus native December3 home Save OK; it does not
solve the later culprit question. The bad branch explicitly throws Namatame
into the TV and confirms that decision, then completes the bad ending through
the native new-cycle save prompt without confirming or writing it. Its
`GameClearDraw` and `shdWNDProc` task predicates are the meaningful endpoint:
there is no field, and the game has reset its staged new-cycle calendar to
index 9. The registration's `-1:-1,4/10` is that non-field cleared-game state,
not an April10 story-coverage claim.

Magatsu's independent final-field card covers the collision-routed approach to
Adachi and his first party COMMAND, not the preceding floors, his defeat or
Ameno-sagiri. A separate post-Magatsu evening card covers Christmas and the
PS2 calendar's automatic jump to March20. That leg stops on the native world
map (`L map` and its symbol task, no active event); it does not select a
farewell destination or establish an ending.

The last-day TV collection leg selects Rise's actual Search for Izanami answer
and requires Ashihara-Nakatsu's exterior step 140, no event and restored player
control. The separate Path 9 card covers its step 149 portal and the first
Izanami phase's fresh party COMMAND. Neither gate establishes traversal of the
intervening paths, either boss phase's defeat or the true ending. These
non-saving checkpoints use `completed_walk_gate.sh`, which requires both the
native field/date goal and completed canonical input; a matching date alone
must not pass a stalled tape.

Margaret's independent victory gate uses the requested unchanged CBS18506 card,
not a replacement collection save. Its open Heaven10 approach starts the real
event and reaches a fresh party COMMAND in arena 244:3. Normal Tactics input
changes Naoto from Act Freely to Direct Commands; Megidolaon,
God's Hand and Vorpal Blade skills attack while Yukiko selects Salvation and
the native charge flags trigger Guard. The gate requires enemy 274's actual
zero u16 HP before cleanup, the subsequent victory event, and restored Heaven10
control at field 28:2/step 110 on March20 with completed canonical input. It
does not claim preceding Heaven traversal or write a save.

**Castle training and retail heap reuse.** `castle_training_0421` retains the
`story0421tv` card, eight ordinary encounters, Dia healing between pairs, a live
4F assertion after every encounter, and Goho-M followed by native Save OK.
Its canonical `save0421tv_to_training.walk` explicitly reopens Attack rather than
repeating a saved Skill-wheel selection. The input plan, not the game or heap,
is re-planned for the retail-allocator cutover.

There is a known retail-dependent variant here: `004968a0`
(`ThunderOrbitBoltUpdateStrip`) reads `bolt.color` before initializing it when
`age < -1`. Prior heap contents choose its respawn/geometry branch, changing
effect RNG consumption and eventually camera/RpRandom consumption. The old
64-bit dlmalloc left native `fd`/`bk` pointers in guest-backed freed chunks;
those bytes are not a canonical PS2 heap reference. The retail allocator chain
instead retains guest-word metadata. Do not clear the bolt, normalize RNG, or
restore the old allocator's stale bytes to preserve an old walk outcome.
See the [CRT boundary receipt](crt_boundary.md) for the allocator decision.

The paired retail study also records a separate **unverified** Castle 4F
generation/path gap: resource identities and floor-save words already differ
at that load, after a failed initial anchor, and retail does not reach the later
battle COMMAND boundary on the same recorded input. No guest patches are used.
Passing this native route is not certification of that PS2 suffix or of
byte-identical heap history; the independent gap remains with lockstep.

CTest `marukyu_allocator` and its automatic `marukyu_allocator64` twin run
`tests/marukyu_allocator_gate.sh` against the real game executable. They reproduce
the pristine card2-slot15 Rise-cleanup/event transition using the canonical
`collection0625_rise_to_teddie_menu.walk`. The private seed
`saves/playthrough0625marukyu11f` must contain the unchanged normalized collection
card; missing private data retains the declared skip 77. Success requires tape
completion at the following encounter's first protagonist COMMAND menu and the
unchanged 60000-flip, arena 242:3, June 25 goal, not just a pre-battle field/date.
This covers the real allocator-layout crash; it does not certify Teddie defeat,
a native save, or a continuous earlier playthrough. Historical pre-fix binaries
are negative controls, not a fabricated new use-after-free mutation.

Sparse `walk_choice` / `walk_tasks` fixtures cover these input-lifetime contracts; they
are not a claim that an independently timed retail route used the same pad stream.

## Fidelity and coverage ownership

Exact-input comparisons from qualified pristine saves use lockstep (private retail-comparison tooling).
Story registrations and canonical walks are the executable source of current scenario
names, limits and goals. Milestone run logs, incident timelines and historical HP/seed
ledgers belong in private evidence/git history, not a second mutable contract here.
Existing ordinary schedules, checkpoints, image limits, digest/floor-RNG goals and
references stay unchanged by this documentation split.

`port64_task` and `port64_task_retail` compare the same 227-line task scenario in i386,
rewritten x64 and the retail R5900 interpreter, with checked guest conversion. Full target
qualification and census rules live in [port64](../port64.md) and CONTRIBUTING.

Run `python tools/check_doc_links.py` to check repository-local Markdown paths and heading
fragments in `docs/`, README and CONTRIBUTING. External websites/private evidence paths
are not fetched; code spans are not treated as links.
