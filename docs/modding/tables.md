# Native game-table expansion

## Current native contract

`include/p4pc/game_tables.h` is the C boundary for the element/channel registry
and base-affinity side tables. It has no scripting-runtime dependency. A core binding builds a
`P4GameTables`, registers owned rows, freezes it with a nonzero revision, then
owns safe-boundary publication and the lifetime of that immutable snapshot.
Creating/freezing one does **not** activate a mod, install a hook or change game
RAM. There is no global last-action/channel state.

An element has an explicit stable ID, `owner:local-name` key, display/short
labels, family, flags, display order, an unresolved icon owner/key/revision and
revision. Retail IDs 0..18 remain occupied. New IDs start at 19. Element/family/channel IDs and
affinity semantic references use the full **u32** domain, including `UINT32_MAX`;
there is no hidden 16-bit ceiling or sentinel in this registry. This is **not**
a claim that retail packets accept those IDs: the core must qualify native
routes and never truncate a semantic ID into a narrow field. Assigned IDs come
from its namespace/range manifest, not registration order. Strings are copied;
pointers into staging arrays are invalidated by further registration.
Standard channels are `p4:damage` (0), `p4:instant_ko` (1), `p4:status` (2).
Custom channels have owned keys and explicit IDs. Retail descriptors may be
claimed once to change presentation/classification while preserving their ID
and key. Duplicate IDs, keys and affinity-row claims are refused.

An affinity side row is keyed by `(entity kind, entity ID, active profile ID,
element ID, channel ID)`. Enemy identity is **not** replaced by `DatUnit.id2`;
callers resolve affinity-row/phase aliases explicitly. Lookups do not invent
fallbacks between profiles or enemies. Percentage channels use decoded native
u32 flags plus percentage; KO channels use Normal/Vulnerable/Resist/Null plus an
independent native susceptibility scalar. Light/Dark damage and KO can differ
without creating additional visible elements. `p4_affinity_decode` preserves
the exact packed retail decode, including unflagged zero.
The affinity scalar and packed percentage retain their 0..65535 limits; KO
values remain the four declared enum values. Semantic-ID width does not widen
the packed retail-u16 decode, glyph encoding or another table's native fields.
`p4_affinity_project_ko` maps retail repel/drain/null to KO Null rather than
inventing reflected KO/healing.

Freeze validates references and affinity domains before sorting/publishing the
instance revision. No lookup allocates; numeric ID and affinity queries use
binary search, and ordered display enumeration uses a compiled index. The
registry stores **base** affinities only. It does not claim to implement the
native passive/equipment/Guard/temporary-effect precedence or draw an icon.
Resources validates and resolves icon references later; UI products own their
actual token pins. An all-zero reference is absent, not a guessed icon or a
replacement for the original game's existing icon switches.

The CPU registry also freezes explicit action/component bindings, entity-profile
aliases and effect/channel bindings. A tagged action carries validity separately
from its full-width IDs, so `UINT32_MAX` remains a valid semantic ID. This
classification is metadata only: it does not enable native routes or change
retail battle calculations. Authored runtime row consumption requires its
bound exact-view provenance callback; registration, lookup and freeze do not
mark source use.

## Inventory and migration strategy

`config/game_tables.json` records source-backed shapes, pointer **slots** (not
loaded bases), ID fields, distinct retail limits and migration strategies for
persona metadata/growth, skills, items, enemies, encounters, fusion,
compendium, shops, elements/affinities, knowledge and saves. Unknown resource
counts are null, not guessed from a convenient gameplay boundary. In
particular, skill 0x1b8 is an active/passive split, not the total table size;
knowledge's 0x55 words have capacity for 340 eight-bit rows while native enemy
initialization admits IDs below 336. `D_007242A0` is a misleading recovered name:
the actual metadata pointer slot is **0x007642a0** (decomp
`config/symbol_data_addrs.txt`), not 0x007242a0.
Saved-array addresses are separately marked `static_state`: 0x007973a0 is the
native state blob, **not** a relocatable pointer word. Its compendium at
0x007981cc and roster at 0x00797f8c must not be mistaken for table-base slots.

The catalog also records measured physical resource hashes and block extents
from the target image (embedded ELF SHA1 matches `SLUS_217.82`). In the
`BTL.CVM` profile, skill elements/names have 576 rows while active metadata/
presentation have 440; enemy metadata/model/AI/name banks each have 336.
Encounter blocks separately contain 912 formations, 160 selectors, 56 weighted
groups and 75 special groups. Unclassified payloads retain null stride/count,
not an invented division. `UNIT.TBL` contains both enemy and persona affinity
banks; `MODEL.TBL` spans party, enemy and genus2 child visual rows. A filename
does not identify one publication family.
The remaining `PERSONA.TBL` blocks have distinct domains: nine 392-byte EXP
curves (curve IDs 2..10), a 4-byte skill-replacement pair list with 41 physical
slots including direction-dependent terminators, and 256 persona-ID-indexed
4-byte replacement rows. Physical pair slots are not the logical list length;
the forward/reverse scans stop on different pair halves. Persona replacement
also has a signed-halfword result carrier at 0x0036fd00.

Static ELF special-recipe descriptors at 0x00642f00 are five 8-byte pointer/count
pairs, with separate 32-byte row groups of 11/8/6/6/6 entries. Their 37 rows are
not one contiguous span. Results and bonus skills use full u16 IDs, but each
material selector packs a **signed 10-bit** value: persona IDs 512..1023 become
negative, and 1024+ cannot encode. Added recipes need logical full-width
selectors rather than repacking these bits. The four native arcana-chart
aliases have 24-byte row stride; that alone does not establish their row count.
Retail 0x00311bd8/0x00311bdc sign-extends the low 10 selector bits with
`dsll32`/`dsra32` by 22; this is not an unsigned 1024-ID field.

The nearby 0x00641c20/0x00641c40/0x00641e60 data is skill inheritance, not persona
exclusion data or those fusion charts: nine 2-byte tiers, s16 weight rows with
19 columns, and signed skill-ID rates. New skills declare inheritance_group
0..18 independently of full-width element IDs; a new leaf must never become an
out-of-bounds matrix column.

These are physical-file facts, not a receipt for the live winning VFS provider.
`ZZZZZ.BIN` has different files under the same keys. The catalog explicitly
marks predicted `conditional_base` addresses as `[INFERENCE]`: they assume the
original loader reset and `BTL.CVM` cohort at arena 0x007f17b0 with 16-byte payload
alignment. Native publication must use actual observed loader pointers/bytes,
never those predicted addresses.

Generate a machine-readable audit with:

```
python tools/table_consumers.py --out <private-output.json> \
  --mirror <actual-generated-mirror-directory>
```

Repeat `--mirror` for each ABI being qualified. The report hashes every scanned
input, identifies direct-symbol uses, scoped numeric constants, narrow fields,
fixed storage/copies and untranslated bodies, retaining file/line/function
address evidence and source identity. It deliberately labels every result
**unreviewed candidate**. A source-only report explicitly says no compiled
mirror was scanned. Numeric matches are not automatically table consumers, and
a missing match is not proof of safety: indirect aliases, script/resource IDs,
asm and layout/copy ownership still need semantic review. This tooling is the
reproducible discovery input, not a completed exhaustive consumer audit or an
activation capability certificate.
The catalog's current-pin schema/bounds rebind does not close that consumer
audit or qualify any compiled mirror or runtime path.

Relocation must use actual loader block byte lengths, not aligned spans. Copy
retail rows into guest-addressable allocation and update every declared pointer,
count and parallel resource binding as one transaction. Never enlarge a retail
array in place. Preserve the simulator's original-size loader observation at
0x00193840 when integrating relocation. Expanded mutable state belongs in the
core's authenticated atomic `.p4mods` sidecar, never in retail padding or the
171112-byte save image.

## Expanded-ID strings and native measurement

`include/p4pc/game_text.h` adds `p4_game_tables_text_*`: owned `(table, ID,
name/description role)` strings, per-string and snapshot revisions, explicit
US-native encoding/font profiles, canonical field-wise enumeration and owner
claims. CPU `text_seal(text, revision)` sorts and freezes definitions without
guest allocation/copy or native publication. `text_semantic_revision` becomes
nonzero; existing `text_revision` stays0 until actual guest materialization.
`P4GameTextDefinition`/`definition_find`/`definition_at` expose exact owned
table/ID/role/revision/size/owner/encoded bytes, deliberately no fake stream.
Both declaration mutation and a different seal revision refuse after sealing.

Runtime `text_materialize` uses the real shared `P4GameTableAllocator` for one
immutable guest arena, copies only prevalidated encoded bytes and assigns actual
streams. No sort, grammar reparse, definition change or table publication occurs
there. Failure preserves sealed metadata with native revision0 and no streams;
retry cannot reopen declarations. Empty snapshots request no guest allocation.
`text_freeze` remains the seal+materialize convenience for actual-runtime clients.
Definition bytes borrow staging/arena storage and must be requeried after
materialization, never retained through that transition. Retain the text object
and stable table/ID/role/revision, not transient byte/guest pointers.
The core retains snapshots through native task/font/queue quiescence. Immediate,
paired and primitive-queued MSG adapters use the actual glyph-list owners,
including `itfMesWindowDrawCallback(node, listHandle)`; they supply no unload fence.
Text row IDs likewise remain full-width u32 keys through registration, sorting,
enumeration and lookup. Native text adapters receive the resulting guest stream,
not a narrowed semantic ID; their two-byte glyph codes are a separate domain.

Payloads are already encoded P4 glyph bytes, **not UTF-8 or Shift-JIS**. Printable
ASCII and high-first two-byte glyphs are accepted; names forbid newlines.
MESSAGE descriptions may include the native newline byte and `c080+`
secondary-style glyph codes. RAW (Config/CreateText) strings forbid both;
Config splits detail/help lines before validating each raw line. The standalone
`p4pc/native_glyphs.h` validator is shared with native pages and has no registry
dependency. Malformed/truncated pairs, interior NULs and command leads
`0xf0..0xff` are refused. A valid trail such as space's `83 f6` remains legal.
There is no guessed Unicode mapping, substituted font, token/substitution
interpreter or icon escape.

MESSAGE measurement and draw use the same `frFontBuildTextFull`/native glyph-list
recipe as item entries (0x00279030/0x002791f0), with actual native
font/style/spacing and horizontal scale rather than character-count estimates.
Width is the `itfMesListWidth` result in **sixteenth-pixel units**, unscaled.
In particular, `0x007645a0` is a 128-based horizontal glyph/kerning scale,
**not a font selector**. The legacy measure seam 0x002787d0 temporarily uses
123/128 when that scale is zero; expanded text does not apply that substitution
because ordinary drawing does not. Multiline mode is explicit and uses the
active native line height. Horizontal scale, font mode and script/state flags
are scoped/restored.

The floating argument of draw/queue/pair is **depth/Z**, not glyph shrink:
native00274a20 writes `FrFontTextRun.depth.scale`, which the actual list draw
forwards to the raster's depth argument. Geometry has separate explicit
`text_measure_geometry` / `text_draw_geometry` APIs. They scope the stock
128-based horizontal/vertical geometry words7645a0/7645a4 around the same
MESSAGE builder, preserving native font metrics, kerning and rounding; caller
state is restored. Style1 is the actual stock slot1 recipe and depth remains
the caller's actual row Z. Geometry words0..128 mean collapsed through normal
size: either0 skips list allocation/draw (the native global0 sentinel itself
would instead mean unscaled),128 is normal. Invalid words return-1 rather
than silently drawing at the wrong size. Measure returns native sixteenth-pixel
width, draw returns status and does not perform an unnecessary measurement.

RAW measurement has a distinct shared helper,
`p4_native_glyphs_measure_raw`: exact Config `frFontExCreateText` (0x00274cd0)
with flags=1, explicit font/style and palette, native widthOut pixels *16, then
`frFontListRelease` **without drawing**. Config still draws with that same raw
recipe. MSG and Config spacing/style parsing are not interchangeable. There is
no width cache claiming an unqualified font/layout revision producer. The
normal native builder owns glyph-list allocation/release; the registry performs
no draw-time string copy.

Retail ID paths must keep their original banks. Item/name/description consumer
cutover and new raster/resource ownership remain separate activation gates;
merely registering a string never makes an expanded item gameplay-ready.

## CPU definitions and source ownership

The skill and item compilers seal owned IDs and their exact retained inputs
before any native pointer publication. Skills use SKILL1 metadata, SKILL0
elements, EFFECT0 presentation and MSG1 names; UNIT2 is persona affinity, not
skill presentation. Item IDs remain bounded by their signed native list and
equipment carriers. Added item categories carry explicit stock policy, owned
action and unresolved resource identity. These CPU APIs do not enable the
native loader, menu, battle or script consumers.

The skill bridge supports exact early stage/rollback, validates actual bank
observations, and materializes a private guest snapshot only at shared
completion. Native skill consumers use registered/active predicates, owned
description text and inheritance metadata rather than the fixed retail skill
count; the loader admission and full native gates still must qualify. Skill
rows may reference full-width element leaves; activating such a leaf requires
the element owner's complete participant cohort at battle completion. A synthetic native
lifecycle fixture exercises staging and rollback with real prepared input spans,
not battle, menu, image-lifetime or save qualification.

The item native owner supports source-stage/rollback, privately materialized
retail-prefix copies plus added rows, exact UI resource pins, and expanded
inventory, equipment, list and shop work areas. The synthetic fixture checks
retail and expanded list enumeration, shop cursor/material layouts, item
metadata and side state with guarded work-area bounds. Its loader and graphics
boundaries are isolated probes; this is not a native battle/menu/save proof.
The item consumer cutover preserves the original retail arrays and banks while
using bounded appended Camp, battle and shop rows and side references for
registered new IDs (at most 32765, due to signed native list carriers). Item
loading authenticates the selected source before the original bounded retail
copy. Config retains sole ownership of its Camp constructor; only the appended
item-tail allocation size is coordinated there. Resource/UI publication and
full native gameplay gates still have to qualify before expanded items can be
considered available.
The separate loader-boundary fixture links the actual native item service,
uses a retail-sized synthetic ITEM and guest mapping, and checks scripted
admission ordering, the copy and builtin-base handoff on both pointer models;
it does not run native boot.
The exact UI-detached query checks only the item owner's staged and active
borrowed pointers. The core must independently quiesce tasks and queues before
ending the item session and releasing the UI pins; pointer detachment does not
prove task, graphics or guest-image lifetime.
At UI300, item and category artwork references must match an actual prepared
resource owner, key and revision; missing images fail before publication.
After Resources200 materializes, the UI owner resolves and pins real tokens,
and the native item owner borrows the UI product. Session teardown requires
independent task, queue and graphics quiescence, followed by item detachment
before UI pin release. No provisional resource token is published at CPU seal.

The enemy compiler seals added enemy rows against the five retail banks indexed
by enemy ID: UNIT0 metadata, UNIT1 affinity, MODEL1 battle parameters, AICALC8
AI rows and MSG2 names, each exactly 336 rows from the actual sealed inputs.
Added IDs are dense from 336 up to at most 4095, so no zero row is invented
between retail and added enemies. Each row reuses a retail enemy's model
resource through `model-source` while its own ID selects every table row, and
needs a `P4_TEXT_ENEMY` name of at most 21 bytes including the NUL. Materializing
verifies all five loaded banks against the sealed bytes, then copies the retail
prefix plus added rows into private guest banks; an empty snapshot allocates
nothing.

The persona compiler applies the same contract to the six persona-indexed
banks: PERSONA0 metadata (14 bytes), PERSONA1 growth (70), MODEL2 battle model
parameters (88), PERSONA5 replacement candidates (4), MSG3 names (17) and UNIT2
affinity (32: sixteen words read by `datPersonaRunSkillSlot` and
`datPersonaGetSkillSlot`), each exactly 256 retail rows. Added IDs are dense from 256 up to at most 511, since
fusion recipe material selectors are signed 10-bit and the name and list APIs
signed16. Each row names a retail `model-source` and needs a `P4_TEXT_PERSONA`
name of at most 17 bytes including the NUL. The party-growth, EXP-curve and
shuffle-pair blocks are not indexed by an added persona's ID and stay retail.
The native persona owner follows the enemy owner below exactly: the shared
packed observer records PERSONA, MODEL, MSG and UNIT loads, the `persona` completion
participant verifies, materializes and relocates the six banks with their
extents, `p4_game_personas_native_limit()` answers one past the highest added
ID (256 otherwise) and session end restores the latest loaded bases. The
datPersona bank asserts (name row `0x00109220`, arcana, learnset, skill slots,
stock insertion, growth, level-up, base stats and `datPersonaInit`) compare
against that limit, so names come from the relocated MSG3 bank;
`p4_game_personas_native_model()` gives the model source for every persona
presentation file keyed by ID: the status portrait `camp/persona/pst_%03x.tmx`
(`0x0011D1F0`), the genus-2 battle model packet (`0x0019B550`), the battle
(`ps%03X`) and facility (`fp%03X`, Velvet Room) model names (`0x002308A0`) and
the Shuffle Time card below, while the scene resource ID keeps the persona's
own 10-bit ID.

The compendium caches 256 retail rows of 48 bytes in the save block at
`0x007981CC` (save tag 16). `p4_game_personas_native_compendium()` returns that
row for retail IDs (and for every ID without expansion) and a port-owned row
for an added ID; lookup, registration, resolve/seed, register-all, completion
percentage (numerator and denominator), the compendium list (`yListSlotRefresh`
kind 2, its work allocation, clear and sort scratch) and the New Game+
snapshot (`0x00310700`/`0x00310850`, added rows appended after the claimed-slot
words at 0x3004) all go through it, and New Game's clear empties the added
rows. The REQUIRED save block `p4pc/persona-compendium` stores the added rows
and pins, by ledger reference, the ID of every added persona the save uses
(registered row, active stock slot or the saved staged record): a package that
now assigns another ID
fails the load instead of reinterpreting records, and a save without the block
loads an empty added compendium. Normal fusion's result scans (the
`0x003130E0`/`0x003133B0` overrides) visit the retail IDs below 0xC0, then the
added IDs, so an added persona whose metadata arcana, flags and level qualify
is a fusion result exactly as a retail one; the 23 fusion gates and special
recipes stay retail. Shuffle Time's persona pools (`0x0036EE60`) scan every ID
below the limit with the retail flag and level rules, and a persona card's
image (`0x0036C7E0`) is its model source's card.

`mod_persona_game` (registered only with `P4PC_MOD_WITNESS_TESTS`, run under
`P4PC_MOD_WITNESS=1`) derives a private package from the user's own disc: one
added persona, ID 256, with Omoikane's rows and name, its level one lower and
Omoikane as model source. From the 4/22 Velvet Room checkpoint the existing
walk fuses Izanagi with Apsaras, which now yields the added persona, assigns it
and saves (the save carries `p4pc/persona-compendium` pinning ID 256); the next
checkpoint reloads that save, fights on 4F and saves again. The Linux x64 run
of this change (on `88c6b735`) passed both legs, the fusion presenting persona
256 with model 172. It is a mod-witness run, never a retail comparison or
reference.

The native enemy owner stages a snapshot with added rows before raw table
exposure. The battle packed loader at `0x00193840` has one shared observer for
every relocating family: it authenticates the selected input once, then each
staged family records the banks that load delivered, so MSG.TBL can feed both
skill and enemy names. At the `0x0010d480` completion the `enemy` participant
requires all five banks, materializes them from the actual loaded bytes and
rewrites the five slots in one commit; a later load re-verifies the same bytes
and rebinds without another allocation. `p4_game_enemies_native_limit()` then
answers one past the highest added ID (336 otherwise), and session end restores
the latest loaded bases to a live image. An empty snapshot registers nothing
and leaves every slot in place. The lifecycle fixture exercises staging, partial
loads, byte refusal, relocation, reload and restore in guest memory; it is not a
native battle qualification.

Every native enemy-ID assert that guards a bank index (`datUnitInit` at
`0x00231af0`, the 27 `datCalc` sites, including the owned `0x00235520` and
`0x00242990` overrides, and the battle reward at `0x001f2cc0`) compares
against `p4_game_enemies_native_limit()`, which is 336 until expanded banks
commit, so retail sessions assert exactly as before. The saved knowledge bitmap
and collection bits (`0x0010f3d0`, `0x0010f420`, `0x0010f460`) are a fixed
340-entry save-state array: IDs from 336 never set or read it, and expanded
enemies keep knowledge only through the knowledge sidecar keys of their
authored aliases. Presentation resources keyed by a fixed retail enemy domain
use `p4_game_enemies_native_model()`: the enemy model-load packet at
`0x0019b550` (every battle-start, cut-in and reload model load) and the
336-row Analyze view table at `0x00219130` take an added enemy's declared
`model-source`, and that use is attributed to the enemy's owning source. Its
data banks (stats, affinity, battle parameters, AI and name) stay indexed by its
own ID. Regular enemies have no sound bank (`0x001f7650` answers only the
retail boss IDs), so added enemies stay silent there as retail regular enemies
do.

`mod_enemy_game` (registered only with `P4PC_MOD_WITNESS_TESTS`, run under
`P4PC_MOD_WITNESS=1`) derives a private package from the user's own disc in its
work directory: every regular encounter enemy (ID below 0x100) becomes an added
enemy from 336 with its retail rows, name and model source, and ENCOUNT.TBL's
encounters point at the clones. It then plays the castle 5F training walk from
the 4/20 checkpoint, fights the clones, saves (the save requires the package),
and reloads that save into the 4/21 school walk. The Linux x64 run at
`b6721bf0` fought added enemies 338, 411 and 490 (models 5, 87 and 174) and
passed both legs. It is a mod-witness run, never a retail comparison or
reference.

`mod_element_game` (same registration and mode) is the expanded-element
end-to-end witness. From the user's own disc it derives the Physical Split
development baseline: Slash, Strike and Pierce leaves 19..21 with synthetic
icon art, the nine-column `[element-ui]` layout, an alias plus three side rows
copying the decoded retail Phys word for every retail enemy and persona
record, damage bindings for every single-character weapon (Yu, Yosuke and
Teddie Slash; Chie and Kanji Strike; Yukiko and Naoto Pierce) and the design's
skill samples (Cleave Slash, Bash Strike, Skewer and Black Spot Pierce). It
plays the castle 5F training walk from the 4/20 checkpoint, saves (the save
requires the package) and reloads into the 4/21 school walk; the element action
route prints one `element leaf N resolves weapon|skill I` line per authored leaf
a battle action uses. A third, menu-harness leg captures the hero's Camp skill
list and persona panel from the new-game state into the work directory. The
first Linux x64 run passed both walk legs with Slash (Yu's weapon 39) and
Strike (Chie's weapon 112); its persona-panel capture shows the nine columns
(three authored icons, then fire..dark) matching the retail seven-column
values. The witness does not capture Analyze or the battle skill row; the
manual battle-harness captures of those are listed under Element presentation.

The table service owns the battle-bank extent registry
(`p4pc/game_table_extents.h`), built and active in every session. Every bank
slot writer publishes the slot's exact byte extent in the same step as its
pointer store: the native copy at `0x00193840` notes each original block, and
the skill and enemy relocating commits and their session-end restores use
`p4_game_table_slot_publish`. Readers such as the battle simulator get whole rows
only when all eight files loaded, the slot still holds the recorded base, the
extent lies in RAM and the stride divides it; a writer that skipped publication
fails closed. `p4_game_table_catalog_identity` names the catalog those banks were
built from: a stable retail SHA-256 until a table-source completion commits, then
a SHA-256 over every package source with the SHA-256 of its decoded bytes, every
native declaration origin and every sealed original input's SHA-256.

The native affinity/effective-action owner resolves an explicit tagged
element/channel and per-action component, then combines the selected entity's
base side row with the original `datCalc` passive, equipment, Guard and
temporary-effect precedence body. Full-width IDs never become legacy bit masks
or inheritance-matrix indices. Additional affinity-effect rows are keyed by
their exact skill/weapon and channel; runtime use follows the owned source.
Expanded routing remains refused unless source, full-width action route,
AI-carrier, knowledge and UI participants are present. Element staging itself
registers the `element-route` participant for this build's compiled native
call-site cutover; it prepares only after the element owner's prepared tuple
exists and its commit is what turns routing on. The prepared element tuple
keeps its sorted battle requirement (action bindings, affinity effects and
active skill frames); `p4_game_elements_prepared_required_pair` answers it
for later participants, and `p4_game_elements_profile_natural_pair` names the
retail pairs (leaves 0..18, including Phys0) that keep the original affinity
without a side row. A display column is answerable only if it is one or the
other; displays never widen the battle requirement. When that requirement is
non-empty, prepare also refuses unless every enemy row the staged enemy
snapshot declares (retail 0..335 plus added enemies) has an alias for its
initial selector (datUnitInit `0x00231AF0` sets id2 = id) and every persona
record 1..255 (datPersonaInitAllRecords `0x0010FFA0`) plus every added persona
the staged persona snapshot declares has a persona alias, so
a missing alias can only surface mid-battle for an undeclared AI-scripted
enemy id2 form, which is still refused at runtime. Enemy status metadata
(`p4_game_elements_enemy_metadata_flags`) is read for any ID below
`p4_game_enemies_native_limit()`, so Analyze and enemy contexts see an added
enemy once its banks are published; prepare-time callers stay retail-only. Synthetic tests
exercise original query ordering, independent channel effects and explicit
action frames, but do not qualify native battle or menu behavior.
The action calculation cutover carries the owning frame and optional
target-affinity query into the original amount equation and script formulas;
it does not use a process-global last action or substitute an unrelated skill.
When no expanded element route is active, the legacy calculation entry follows
the original path. The equation fixture calls these actual overrides with
scripted engine facts and checks state and RNG order; it is not a native battle
or AI qualification.
The hit resolver at `0x0023e6f0` uses that owning frame for full-width
physical classification, exact-channel affinity and the original hit-rate
formula; Light/Dark KO queries retain their distinct KO channel. Its direct
synthetic witness compares retail/tagged state, query order and RNG, including
expanded skill and wide-leaf cases. Critical/weak classification at
`0x002411a0` likewise uses the tagged affinity and physical classification
without folding leaf IDs into legacy masks. Applicability at `0x00241bc0`
retains its native early eligibility checks and classifies the full-width frame;
an absent source retains the retail skill-element fallback rather than forcing
weapon resolution. Status infliction at `0x00238940` selects the status or
instant-KO channel only after the original payload choice and early exits, then
uses the skill's tagged family for the existing ailment-boost predicates.
Light retains its retail status-element rule. The endurance resolver
`0x00242990` receives the actual attacker and resolved KO status from its
`0x001f14f0` caller: Light/Dark KO-specific survival effects do not trigger on
ordinary damage, and explicit KO bindings retain their full-width family.
Direct fixture probes cover these branches; they are not native battle
qualification. Presentation-only consumers — skill-update formation, target
cue and hit-reaction motion at `0x001a7720`, opening motion at `0x00199d00` and
the hit-display category at `0x00201b00` — ask the same owning frame when
routing is enabled. A retail leaf keeps its exact value; a new leaf presents as
its declared retail family (or the switch's default branch) and contributes
only its ailment class, never a narrowed alias or the asserting legacy bit.
Follow-up attacks resolve the follow-up skill's owning frame: the availability
check at `0x001a3df0` repeats `0x001d94d0`'s side-2, KO-excluded scan with the
frame's physical/magic barrier class and native null/repel/drain query, and
the target filter at `0x001a43a0` uses the same frame affinity.
The enemy-knowledge learning gate in `0x001f14f0` observes, through the
knowledge owner, the row's primary frame and, when the uncured KO payload was
resolved, its instant-KO frame; it never narrows the leaf or writes the retail
bitmap itself, and the retail gate is unchanged when routing is off.

`P4GameTableSources` binds each exact frozen registry, skill, item, enemy or text view
and authored entry to an explicit native source or retained selected package
provider. The shared `P4ModSaveRequireOwnerFn` validates that origin and, when
given a save writer, records the actual package requirement. Missing, duplicate,
changed or unknown ownership is an error; owner strings alone never confer
authority. Native semantic-use callbacks touch only the actual selected source
when a row is used, not when it is registered, sealed or inspected.

The expanded-item save provider serializes owned item quantities, availability
and equipment references through the mod sidecar. It stages and validates the
whole transaction before commit; retail inventory and save bytes are untouched.
Native developer-input use records the already-sealed SHA-256 in that same
sidecar, while unused sources emit nothing. Synthetic fixtures exercise source
ownership, item sidecar round trips and the full-width skill definition path;
they do not claim an end-to-end native save or gameplay route.

## Declarative table source schema

The pinned declaration format is:

```ini
[table-source overhaul:combat]
file=data/tables/combat.xml
format=p4-tables-xml
schema=1
revision=7
```

The qualified owner:key, explicit contained file, format/schema and independent
nonzero u32 revision are required. `game_table_manifest.h` validates these
metadata fields without I/O or provider inference. The future C-only startup
handler must authorize the actual contained file through its selected provider
plan; this metadata parser alone neither opens files nor admits a mod.

`game_table_xml.h` decodes one `<p4-tables schema="1">` root with flat,
non-executable records. Numeric attributes are checked u32 decimal or `0x`
hexadecimal values; the declaration revision supplies each authored revision.
Each row binds to the explicit source handle, and a failed candidate is discarded.

| Record | Required attributes |
| --- | --- |
| `element` | `id`, `flags`, `family`, `order`, `key`, `display-name`, `short-name`; optional all-or-none `icon-owner`, `icon-key`, `icon-revision` |
| `channel` | `id`, `kind`, `key`, `display-name` |
| `affinity` | `kind`, `entity`, `profile`, `element`, `channel`, and either `value` and `scalar` or `retail-row` and `retail-element` |
| `action` | `source-kind`, `source-id`, `component`, `element`, `channel`, `key` |
| `alias` | `kind`, `entity`, `native-profile`, `profile`, `knowledge`, `key` |
| `effect` | `source-kind`, `source-id`, `element`, `channel`, `flags`, `key` |
| `skill` | `id`, `active`, `element`, `element-aux`, `inheritance-group`, signed `inheritance-rate`, `key`, exact 40-byte `metadata` and 4-byte `presentation` hex |
| `item` | `id`, `key`, exact 68-byte `metadata` hex and icon reference; optional requirement ID/quantity pairs and model IDs |
| `category` | `id`, `key`, `filter-a`, `filter-b`, `stock-policy`, icon reference; optional all-or-none action owner/key/revision |
| `enemy` | `id`, `model-source`, `key`, exact 60-byte `metadata`, 32-byte `affinity`, 232-byte `model` and 164-byte `ai` hex |
| `persona` | `id`, `model-source`, `key`, exact 14-byte `metadata`, 70-byte `growth`, 88-byte `model`, 4-byte `candidates` and 32-byte `affinity` hex |
| `string` | `table`, `id`, `role`, `bytes` (game-encoded hex including final NUL) |

An `affinity` row with `retail-row` and `retail-element` (0..15) instead of
`value`/`scalar` takes the entity's own entry from the selected UNIT input
(the disc's UNIT.TBL, or a selected replacement of it): row `retail-row`,
packed column `retail-element` of the enemy (UNIT1) or persona (UNIT2)
affinity bank of the row's `kind`, read when the builder seals. A percentage
channel stores the exact `p4_affinity_decode` word with its low 16 bits as
scalar; a KO channel stores the `p4_affinity_project_ko` projection. Such a
row requests the UNIT input and has an authored row's key, conflict and
ownership rules; the source that declares it owns the row, while its value's
origin is that sealed input, whose SHA-256 is part of
`p4_game_table_catalog_identity`. Freeze refuses while a row is unresolved. A
row outside the actual bank, or a word with no form for the row's channel,
refuses the source with a diagnostic naming the kind, row and element. A
percentage row needs at most one `P4_AFF_FLAGS` flag; a word carrying retail's
ailment no-effect flag (0x20 in the packed high byte, read only by
`datCalcInflictStatus` 0x00238940's 0x27000000 mask) refuses on any channel,
since no channel models it and a KO projection would invert it. A mod can
therefore declare "this entity's retail Phys, for Slash" without shipping any
retail value.

The builder seals exact family inputs and ownership together without guest
allocation. It is not by itself a boot handler, resource resolver, loaded-image
attestation or native gameplay capability.

## Authored image resource service

`P4Resources` owns authored raster generations and transient numeric tokens;
the actual package/source producer supplies decoded RGBA and its already
selected source metadata. Resource names and diagnostic provider strings do
not authorize a package or substitute for a retained provider/file plan.
Freeze exposes a generation atomically. UI products resolve stable
owner/key/revision references later and pin actual tokens for their lifetimes;
table semantics never depend on a resource token or stage an image in schema0.

Allocation charges the padded CPU shadow and GPU backing against the supplied
budget. Failure publishes no token. Retirement prevents new pins and destroys
the real raster only after existing pins drain. An immediate authored draw
restores both the modified render-state values and their written/default bits.
The renderer creates each authored raster through real RW and privately adopts
only that fresh raster's guest shadow, resizing it for transparent padded rows.
Ordinary game raster creation and the deferred-only public retagging guard are
unchanged; no global or thread-local creation marker is used.
The native bridge requires a completed image load, the game's current context
and initialized RW engine; this load flag does not attest image survival after
teardown. The core must quiesce resource users before releasing those owners.
After that independently proven quiescence, `p4_resources_retire_unpinned`
refuses without changing the generation while any pin remains; success
synchronously destroys it. This checks resource pins only, not task or queue
quiescence, and invalidates the generation pointer on success.
The SDL current-context query gates only host graphics/resource operations; its
result is never published as guest state or used as a content-authority proof.

The token fixture exercises synthetic raster-backend responses, quotas, failure,
pins and retirement; the separate native renderer fixture uses real RW/GL.
Neither fixture alone establishes item-menu or gameplay qualification.

## Source boot lifecycle

At package boot phase 100, the table-source handler validates each selected
provider's declaration, refuses duplicate source keys, opens its authorized
contained file, and pins the actual file plan while parsing its schema. The
combined semantic and source-owner views are sealed before later boot phases
observe them. Before raw exposure the handler stages the selected input admission
and native family owners; a failure releases only the stages it actually owns.
The runtime phase materializes the owned text and stages provenance and element
consumers against that same sealed product. This path uses selected registry
providers and file plans, not a detached package-selection preview.
When any packed input is needed, native boot captures all eight packed
descriptors sharing the retail battle arena before sealing; admission can then
authenticate each actual buffer and compare every aligned contribution to its
retail counterpart. An ITEM-only transaction does not capture packed inputs.

The source-boot fixture exercises real synthetic provider plans, XML validation,
CPU seal, and refusal before raw exposure when a native source owner is absent.
It does not prove native loader integration, gameplay or rendering.

## Native preparation and shared completion

`P4GameTablePrepareFn` is a native, nonyielding preparation callback over the
actual loader label and borrowed managed bytes, not gameplay Lua. The core
checks actual winning-byte provenance/claims and finalizes staged bindings
before publication. A path alone is not provenance. Missing preparation is
`P4_TABLE_PREPARE_REQUIRED`; conflicting callback/user pairs are refused.

Shared battle data completion at0x0010d480 has ordered families `item`, `skill`,
`persona`, `enemy`, `table-source`, `element`, `element-route`, `element-ai`,
`element-knowledge` and `element-ui`. Expanding owners register one
prepare/commit pair and can declare required participants before the graph locks.
Every prepare must succeed before any commit runs. Preparation may freeze
owned allocations but cannot publish guest bases, counts or active snapshots.
Commit only publishes prevalidated state; it cannot fail or yield.
Unpublished allocations remain caller-owned after failure for explicit
refused-boot teardown, not invented rollback.

Package startup joins this same coordinator through
`p4_game_tables_battle_prepare()` before any native completion commit and
`p4_game_tables_battle_commit()` only after every enclosing prepare succeeds.
Prepare alone publishes nothing; duplicate/reentrant prepare or clear while
prepared is refused. Commit cannot fail and misuse refuses rather than silently
publishing an unprepared tuple. `p4_game_tables_battle_abort()` may discard only
an unpublished prepared/failed transaction; it neither frees owner staging nor
undoes an earlier commit. It never unlocks registration. After raw VFS exposure,
a runtime failure requires controlled restart without the failed owner, not
candidate retry or fictional aggregate rollback. The existing `battle_complete`
is the prepare+commit convenience for actual later native loads.

Registration locks at first completion. Every real later load revalidates and
rebinds its owners, so a later native-loader pointer write cannot discard an
expanded binding. A failure remains sticky until explicit core-owned abort of
that unpublished transaction or quiesced teardown clears the coordinator. No participants
request no allocation or guest writes. Translated completion code imports only
the scalar `game_tables_native.h`, not host registry descriptors.

The completion fixture exercises ordering, reentrancy, prepare-only
nonpublication, abort/commit boundaries and repeated-load protocol without a game process. The text
fixture uses native-recipe stubs to check byte grammar, ownership, exact draw/
measurement arguments and the actual callback argument/release convention.
Neither fixture claims battle/menu/save/render qualification.

## Enemy knowledge

Expanded Analyze/affinity knowledge is keyed by an authored enemy alias's
knowledge and profile ids plus the element and channel
(`p4pc/game_knowledge.h`). Every key comes from an actual enemy alias and its
side affinity row; aliases deliberately sharing a knowledge id share
observations within one profile, but sharing never qualifies another alias's
missing pair. Damage and KO channels are distinct keys.

The `game-tables.enemy-knowledge` package handler is inert unless the tables
request expanded elements. Its CPU pass registers the REQUIRED
`p4pc/enemy-knowledge` save block over Root's sealed source ledger; its runtime
pass registers the `element-knowledge` completion participant, which requires
`element`. Prepare compiles the unpublished store (a reload of the same
snapshot keeps its observations) and commit publishes it. The block records
each known key with its canonical alias, element/channel descriptors and side
row through the ledger references; no known key omits the block and an absent
block clears to the default. A changed alias, descriptor or source refuses the
whole load.

The expanded route of 0x001f14f0 learns through
`p4_game_knowledge_native_observe(defender, frame)` at the existing
eligible-outcome gate (enemy defender, outcome not a miss), once for each
frame whose affinity was actually resolved against that defender, and never
writes the retail bitmap itself. The call follows the native affinity
resolver's route: no element or a leaf without affinity learns nothing; an
authored alias/side pair learns its key; an unmodified retail component
without one, which the original resolver answered, sets the original retail
bit for leaves below 8 (enemies below 336). Anything else refuses through the
caller's controlled runtime failure. With expanded elements off the original
body runs unchanged.

AI reads take the same route. `p4_game_knowledge_native_ai_query(defender,
frame, query)` makes a known affinity ACTUAL and an unknown enemy affinity
PROVIDED neutral 100, so a hidden row never ranks a candidate; persona
defenders and frames without affinity are known, and an original-route
component reads exactly the retail `datTestBitmapBit(enemy, leaf)`.
`p4_game_knowledge_native_any(defender, known)` is 0x001db740's Analyze
any-known: the alias's side keys plus the original bits of natural retail pairs
(leaves 0..5 damage, Light/Dark KO) that have no side row or added effect pair;
a retail-only enemy keeps its retail bits.

The Analyze menu (0x00219790, element-ui) reads one displayed pair through
`p4_game_knowledge_native_analyze(defender, element, known)`: the same route
with no owned binding (a display resolves no action), so no affinity is known,
a persona is invalid and an original-route pair reads its retail bit below
leaf 8. Only after the pair resolves does 0x001f0950's encounter override
apply: without enemy metadata flags 0x4004 and with story flag 0x1403, flag
0x1408 reveals and 0x1409 hides. AI reads never take that override. The
knowledge fixture covers keys, the observation, AI and Analyze read routes, the
participant, the handler and the save block over the real package core; it
makes no battle, menu or save-image parity claim.

## Source-qualified AI carriers

`game_elements_ai.h` borrows the actual retained AICALC rows and player/enemy
BF spans from the winning input plan. Their three sealed identities are
distinct. CPU preparation parses immutable BF/CFG/command contracts and checks
all eight condition-action bytes, weighted command references and enemy
procedure references against the real sealed skill/BF definitions. Opaque
`0x1000`/`0x2000` and declared `0x800x` actions are not invented skill/item rows.
Neither preparation nor binding records package use; actual row/BF consumption
uses the input owner's semantic-use callback.

The TABLES100 handler `game-tables.enemy-ai` depends on `game-tables.sources`
and is inert unless the tables request expanded elements. Its CPU prepare
consumes that pass's exact Root product, finds AICALC by input kind in the
dense input array and borrows issued spans 8 (rows), 9 (player BF) and 10
(enemy BF). Source authority is the pre-copy table admission of all eleven
AICALC slots (`game_table_admission.c`). At the real 0x0010d480 runtime pass
the handler requires the same Root views, inputs and owners, binds the actual
GP roots used by 0x001ded30 (`b3d0`, `b428`, `b42c`) by exact length and bytes,
and registers the `element-ai` completion participant (requiring `element`
and `table-source`). That late bind proves the mapping only. Early rollback
releases only the unused CPU product. Knowledge has its own handler. Core
quiesces script/action users, calls `p4_game_elements_ai_boot_session_end`,
clears the shared completion graph and only then releases Root's products.

The participant requires the exact prepared element table and skill views,
actual bound banks and the original three-slot VM pools. Expanded actions keep
one original allocation with an explicitly sized scalar extension; the
original `0x458` record is never repurposed as padding. VM metadata belongs to
the actual pool slot/generation. Command/element-valued locals own full
frame/target snapshots rather than aliasing a global or the actor's last result.

Script values carry a boolean validity per candidate source frame. `OR`/`AND`
keep their meaning (a result either operand establishes stays valid), and a
branch permits only the frames whose prediction matches the path the script
actually took. Validity is never about a defender: as in the original game,
the script may ask about unit B and then act on C (a fallback pick, a new
target after One More, a heal on another ally), so an answer about B neither
drops C from a selector or scoring pass nor binds the targets the native
command finally uses. When the script stops, the carrier checks only that the
action still holds the frame it committed. MY stays fixed-self and FR/EN keep
their original eligible-side ANY domain and flags. Control scopes use bounded
prepared VM storage, never per-action allocation, a pointer-keyed map or
native-command/RNG replay; exhaustion is an error, not a fallback. Malformed
carrier/query state uses Core's controlled runtime failure, never a host CRT
`abort`. Scoring compares all eleven scalar frame members explicitly, without
host `memcmp` in a translated unit.

The canonical 0x001d94d0 side search keeps separate inactive and metadata
runners over the same body. Expanded searches retain per-frame ANY results over
the original eligible side; the option2 barrier gate precedes knowledge,
option4 rejects an unknown frame and option8 accepts it before the dead/status
checks without reading its real affinity. FR/EN script query roots pass their
VM explicitly and keep source-scoped numeric vectors without replaying a
command, RNG draw or callback. The source snapshot holds at most 24 entries
(basic, eight equipped skills and the fifteen commands of the three weighted
groups), built with the pure canonical preview; spawn, refresh and unused
commands never call usability gates or source-use hooks. Each consumed frame
crosses the real runtime constructor before affinity/HP evaluation.

The 0x001de800 table root keeps all three condition vectors, intersects
permissions per frame and preserves the original positive-condition precedence
`7,3,5,6,1,2,4,0` including the default action byte. Weighted selection keeps
five entries, the original u8 weights/u16 sums, `random <= cumulative` and one
draw; a zero-total group still returns the `0x763398` basic-attack sentinel
without a draw. The 0x001dbba0 candidate selector carries the exact candidate
frame and the original per-script knowledge policy; HP scoring passes each
target's own prediction to the canonical `p4_game_elements_native_amount`. The
retail context accessor still returns a guest word; AI adapters translate it
through `P4_G2H` before host metadata calls. The selector's s32 query value
keeps the host-ABI cast on its retail fallback.

Encoded command setters consume the immutable tag of their actual argument.
A proven basic-attack guard derives its source role from actual native
basic/encoded-command consumers and control flow, never a name, hash or PC
allowlist, and asks about the actor's real basic attack; an unqualified literal
Phys0 query refuses. BASE MYHANSYA/KYUSYU/MUKOU/WEAK and wrapper normal/not-normal
keep their distinct original knowledge/buff ordering, and knowledge reads take
the route above. With expanded elements off every carrier site runs its
original body. Expanded routing still requires the `element-route` and
`element-ui` participants; the isolated role/decision/collector witnesses are
not battle parity evidence.
## Element presentation

`p4pc/game_elements_ui.h` holds one owner's ordered layout of at most nine
affinity columns over the exact frozen registry. A column names a registered
leaf, its actual effect component and affinity channel, and optionally an
independent same-leaf instant-KO marker with its own channel and knowledge.
Order and omission are explicit; registering physical leaves never hides
retail Phys0 or changes any descriptor flag. Every visible new affinity leaf
needs a column, and every visible new leaf needs a descriptor icon reference
(its skill-list art). Translated units see only the scalar
`game_elements_ui_native.h`.

The UI300 package handler `game-elements.ui` (dependencies
`game-tables.sources` and `game-resources.images`) reads at most one
provider's layout:

```ini
[element-ui]
schema = 1
revision = 1
order = phys.split:slash, phys.split:strike, phys.split:pierce, p4:fire, p4:ice, p4:electric, p4:wind, p4:light, p4:dark

[element-column phys.split:slash]
component = damage
channel = p4:damage

[element-column p4:fire]
component = damage
channel = p4:damage
legacy-primary = fire

[element-column p4:light]
component = instant-ko
channel = p4:instant_ko
legacy-primary = light-ko
```

Every ordered key needs its `[element-column]` (only one of each kind is shown
above). Components are `damage`, `instant-ko` or `status`; channels are exact
registered keys. A marker needs both `marker-component = instant-ko` and
`marker-channel`. `legacy-primary`/`legacy-marker` (`phys`, `fire`, `ice`,
`elec`, `wind`, `light-ko`, `dark-ko`) let the static persona panel read the
actual seven-word native snapshot for exactly that selector's native pair (the
persona's native profile 0, whatever its semantic profile ID); an authored side
row always wins. A damaging-Light layout declares
its damage primary plus a KO marker, and only the marker may use `light-ko`.
Unknown fields, a second layout owner, or a layout without an expanded
element cohort refuse that provider before raw exposure. An expanded cohort
with no authored layout uses the retail seven columns; a retail session
registers nothing and every caller keeps its original code.

The early pass plans against the CPU products only: descriptors, pairs, and
every declared alias answering every displayed pair (side row, the shared
`p4_game_elements_profile_natural_pair` route, or an authorized persona
snapshot word). Authored art is the descriptor's stable icon reference and
must be an exact declared image; retail leaves keep their real affinity-bank
headers (0x2E..0x34), and no token is ever treated as an atlas index. The
runtime pass resolves and pins real tokens, allocates the command-free ASCII
`KO:N/W/S/X/?` marker stream and one immediate native sprite sample, and
stages the `element-ui` completion participant (requiring `element` and
`element-knowledge`). Its prepare accepts a displayed pair only if it is in the
element owner's prepared required set (which holds every retail enemy and
persona alias to it) or is a legacy-authorized natural pair (answered by the
original route in Analyze and the native snapshot word in the persona panel).
It also refuses a retail enemy alias (below 336) without loaded metadata; an
added enemy's metadata row is verified by the enemy owner in the same
transaction and published by its commit. Commit publishes only after `element`
commits.

Consumers, all scalar and fail-closed:

| Site | Behaviour with an expanded cohort |
| --- | --- |
| 0x00116820 persona/compendium/fusion panel | columns from the actual display snapshot and persona side rows |
| 0x00219790 Analyze | `p4_game_knowledge_native_analyze` (0x001f0950 policy), then `p4_game_elements_native_profile_base_affinity` (0x0023a6b0 role) for known pairs |
| 0x001138c0, 0x00205170 skill icons | skill-element applicability frame (no actor in lists; the battle row's actual DatUnit) |
| 0x00138bf0 Camp skill list | the same frame before the legacy label lookup |

Seven columns keep the retail 51/37 spacing; eight or nine fit the same
236-pixel span, horizontally scaling native glyphs, with authored headers in
26x20 and skill icons in 24x24 boxes. Authored art receives exactly the colour
the native glyph at that site would: the persona panel's header tint, each skill
row's tone, and Analyze's known (0x24,0x4C,0x00) or unknown (0x70,0x70,0x70)
header tint. The colour multiplies the image, so draw white glyph art on a
transparent background, as the retail atlases do; coloured art comes out dark
on Analyze. Markers use the shared MESSAGE geometry
API at half size. Retail-leaf skills keep the original 0x00114cb0/0x0013ac30
art unless their claimed descriptor declares authored art. Retail draws
`datCalcSkillElement(NULL, skill)`; lists without an actor keep that skill's
own element, while the battle row's real unit lets a weapon-derived skill show
its equipped weapon's element. Draws authorize the exact issued element/channel (and persona
alias/row) through the Root source transport; planning and getters do not.

Teardown is core-owned: after task, queue and graphics quiescence call
`p4_game_elements_ui_native_session_end()`, then
`p4_game_elements_ui_boot_session_end()` (which refuses while the snapshot is
still borrowed and releases every pin), before the resource and table session
ends. Tokens and draw handles are never saved.

The `game_elements_ui` fixture covers layout validation, CPU alias coverage,
the art set, exact image checks, real token pins/release and the marker
stream over the real registry and token service (mocked raster backend).
Manual Linux x64 captures with the Physical Split development package, through
the menu and battle harnesses, match this section at all five sites: the persona
panel, the Camp list and its selected-row descriptor, the battle skill row, and
Analyze before and after a Slash hit teaches the first column. Only the Camp
list and persona panel are automated in `mod_element_game`. These are
mod-witness evidence, not retail references. No other platform or viewport has
been captured.

## Final damage stage

`p4pc/game_damage_stage.h` is the one place a mod changes ordinary HP damage.
Hard mode and challenge rules register modifiers there; nothing hooks
`datCalcSkillAmount` (0x00235520) directly. The stage runs once per eligible
hit at the end of that function, after the whole retail equation (formula,
attack/defense/status/passive terms, critical, guard, the 1..31000 work clamp
and the 95..105% variance) and after the outcome: only a hit that delivers HP
damage is eligible. Fixed, percentage and scripted (hit kind 16) amounts, SP,
misses, nulls, drains, healing and `options & 1` estimates never reach it.
With no modifier registered the function is retail and the stage never runs.

Each modifier is a pure function of the hit: no RNG, no game-state writes, no
re-entry. It receives the skill, source and target side (party/enemy),
DatUnit ID and `DatUnit.bad` ailment word, the unit the hit was aimed at, the
`datCalcHitResult` kind (normal 1, critical 2, weak 4), the resolved element
leaf when one exists, whether a repel delivered the hit back to its source,
and whether 001f14f0 is resolving the hit (`resolved`) or the call is an
estimate (AI target choice). The ailment words are the ones the equation read:
001f14f0 computes every hit of an action before its packet commit applies any
ailment, so they exclude this action's own ailments and stay exact per unit
when a formation repeats an enemy ID. 001f14f0 brackets each resolved hit's HP
amount call, so a repelled hit keeps the side it was aimed at and is composed
exactly once. Multi-hit skills compose each resolved hit once.

Order is fixed: declared priority ascending, then multiplicative before
additive within a priority, then the owning mod's load order, then
registration order. Values compose in double precision and convert once at
the end with the retail truncation, clamped to the equation's own range
(1..32550). A non-finite value or negative factor is ignored for that hit and
reported once; registration while a hit is composing is refused.
`P4PC_DAMAGE_TRACE=1` prints every hit, each applied step and the result to
stderr. The 001f14f0 per-target persona scale still applies after the stage,
as retail applies it after this function. `game_damage_stage` covers the order,
single conversion, clamp and refusals. `game_elements_amount` runs the real
equation: every effect, outcome, channel, hit kind and option is unchanged with
a modifier that is consulted but leaves the hit alone, and a challenge plus
hard-mode pair composes as declared, including a repelled hit, without
drawing RNG.

## Qualification and availability

The native registry has isolated synthetic tests for retail packed decode,
owned new leaves/channels, independent enemy/profile/damage/KO rows, conflict
refusal and failed-freeze nonpublication. These are **in-memory metadata tests**,
not battle, menu, save/load or platform parity claims. The inventory parser has
synthetic direct-access/narrowing/storage/asm/mirror-identity tests.

No Luau expansion capability is advertised by this phase. Native battle/AI/UI
adapters, item/skill/persona/enemy consumer cutovers, resource registration and
sidecar persistence must qualify before their corresponding expanded IDs become
reachable from gameplay. Physical Split and damaging Light/Dark must share one
explicit action/effect-component affinity adapter, not competing replacements
at 0x0023a6b0, 0x00242800 and 0x0023e6f0.
