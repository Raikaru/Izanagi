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
slots (40 pairs and a (0, 0) terminator), and 256 persona-ID-indexed 4-byte
replacement rows. Physical pair slots are not a count any reader uses: the
forward/reverse scans stop on different zero pair halves. Persona replacement
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

Persona records keep raw u16 skill IDs, so a save can hold an added skill in
any of them: the twelve stock slots (`0x00797F8C`), the staged record of
`datStageRecord` (`0x0079B6A8`), the party members' personas (`DatPartySlot`
`0x00796E50` + `0x54`, party IDs 2..10) and the compendium rows (an added
persona's row rides `p4pc/persona-compendium` with the same raw IDs). The
REQUIRED save block `p4pc/expanded-skills` pins, by ledger reference, the ID of
every registered added skill those records hold. A load without the package
fails with the missing provider; a package whose skill at that ID is missing
or has another owner, key, schema or revision fails the reference; a save that
holds no added skill omits the block.
The block carries no other state: the records load with the image. An ID above
575 that no package registers is left unpinned. `game_skills_save` covers each
record kind, both refusals and the round trip over the real mod-save core.

A cast loads its battle effect `skill/%03X.BED` (`btlFormationFormatName`
0x001D69F0) and its sound bank `skill/%03X.se` (0x001F7D90/0x001F7E30) by skill
ID, and the disc has none for an added ID. With `table-source.skill-source@1`,
a skill row's optional `effect-source` (`P4GameSkillDesc.effect_source`, a retail active skill
1..439) names the retail skill whose two files the added skill's casts load;
`p4_game_skill_effect()` returns that ID, and the ID itself for retail IDs and
for any ID no package registers. Casting an added skill without one refuses
the session with the skill's key instead of waiting forever for a missing
file; a passive skill never needs one.

With `table-source.skill-derive@1`, a derived skill row (`derive`, a retail active skill 1..439) has no
`metadata` or `presentation` of its own: when the skill family seals it copies
that anchor's 40-byte SKILL1 row and 4-byte EFFECT0 presentation from the
selected inputs, after any derived producer and table delta, then applies the
fields it names: `power` (+0x12, 1..9999), `cost` (+0x04, 1..999), `accuracy`
(+0x0E, 1..100) and `critical` (+0x25, 0..100). Those four attributes are
accepted only with `derive`. Its element, inheritance and `effect-source` are
its own, as on any added row.
If the anchor has an exposed `max-heal` delta, an active derived copy inherits
its Divine Grace opt-in along with its effect and power, even when the copy
overrides power. This applies only while the native metadata slot still names
the copy's published bank and the anchor is the exact composed source row.
An independently authored row with identical bytes does not inherit it.

With `table-source.skill-delivery@1`, a skill row's optional `delivery` (`physical` or `magic`;
`P4GameSkillDesc.delivery`) separates how a hit lands from what it is made of
(hardmode-design 8.1). Without it the delivery follows the element, so retail
skills and every existing package are unchanged. The resolver marks the damage
frame `P4_ELEMENT_DELIVERY_PHYSICAL` or `P4_ELEMENT_DELIVERY_MAGIC` in its
element flags (registry element flags never use those bits), and
`p4_game_action_frame_physical()` answers three consumers: the crit roll
(0x002411A0), the counter roll after the shield (0x0023E6F0), and the
physical-only damage term (0x00235520). Affinity, Tetrakarn/Makarakarn,
element Boost/Amp and evasion passives keep asking the element. Reflected and
countered hits carry their frame, so they keep the original delivery. An Ice
skill with `delivery="physical"` therefore crits and is countered like a
physical attack, is blocked by Makarakarn, and is resisted, nulled, reflected
or drained as Ice.

Skill IDs 600–647 are reserved for `p4pc.fusion-arts`. This is a convention,
not enforced: a package that registers one of them conflicts with that package.

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
skill-pair blocks are not indexed by an added persona's ID; added rows leave
them retail (a derived source can replace the skill pairs, below).
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

`mod_skill_game` (the same registration and mode) adds skill 576 from the
user's disc: Agi's rows as "Test Agi", Agi as its effect source, inheriting
nothing. It also adds persona 256 as above, whose first innate skill is 576 and
whose agility is 99 so the protagonist acts before the AI companions end the
fight, and binds 576's damage to the Physical Split witness's Pierce leaf with
persona 256's alias and side rows. The 4/22 fusion walk yields persona 256 and
saves; the battle leg's driven 4F fight opens with the protagonist casting 576
from the native Skill list (`skill:576`), the element route reports `element
leaf 21 resolves skill 576`, and the walk saves; both saves carry
`p4pc/expanded-skills`. Loading the save with mods disabled is refused for the
missing Physical Split package, and loading with only that package enabled is
refused for the missing skill package. The Linux x64 run on `a464376e` passed
all three original legs (the cast loaded `skill/001.BED`). It is a mod-witness
run, never a retail comparison or reference.

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
route prints one `element leaf N resolves weapon|skill|enemy attack I` line per
authored leaf and action source a battle uses (the first 64 such pairs of a
run). A third, menu-harness leg captures the hero's Camp skill
list and persona panel from the new-game state into the work directory. The
first Linux x64 run passed both walk legs with Slash (Yu's weapon 39) and
Strike (Chie's weapon 112); its persona-panel capture shows the nine columns
(three authored icons, then fire..dark) matching the retail seven-column
values. The witness does not capture Analyze or the battle skill row; the
manual battle-harness captures of those are listed under Element presentation.

`mod_item_game` (same registration and mode) is the added-item witness. From
the user's own disc it derives a package with three added items cloned from
retail ITEMTBL rows: sword 1536 (weapon 0x004's row and model, attack 1), armor
1537 (0x106) and accessory 1538 (0x204), each with its own name, description
and icon, priced low without material gates. It composes with the Physical
Split package: the sword, every retail enemy attack and the Ice and Elec spells
deal Slash, and the accessory resists Slash through an EQUIPPED_ITEM effect row.
From the Apollo save the shop leg buys all three plus three more accessories at
Daidara through the game's own Yes/No choices, equips them in Camp (Yu all
three; Yosuke, Chie and Yukiko an accessory) and saves; the save requires the
package. The battle leg reloads it on Marukyu 1F, guards while the Shadows act,
strikes once with the sword and escapes, so it never depends on a victory or
Shuffle Time; it must print `element leaf 19 resolves weapon 1536` and `affinity
effect of equipped item 1538 resolves element leaf 19` (the effect route prints
one line per effect row a hit resolves). The last leg reloads that save with
mods off and must be refused because the item package is not enabled. The
Linux x64 run on `8aed581b` passed all three legs:
the Shadows' Mazio (skill 40) resolved Slash against the wards, Yu's sword
struck, and every refused load named the missing provider. It is a
mod-witness run, never a retail comparison or reference.

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

### Session bank overlays

`p4pc/game_table_overlay.h` is the table service's per-run overlay of whole
loaded banks, bound for Luau as `p4.tables` (`tables.overlay@1`). Bank keys are the file
labels and block indexes of the actual `0x005f66c0` descriptors (`SKILL0`..1,
`UNIT0`..2, `MSG0`..4, `PERSONA0`..5, `ENCOUNT0`..4, `EFFECT0`, `MODEL0`..2,
`AICALC0`..10); every base and byte extent comes from the extent registry, so a
bank is usable only once all eight files loaded and its slot still holds the
noted base. Four static banks name fixed ELF data spans instead: `FUSION0`..3
are the arcana charts at `0x00642147`, `0x00642387`, `0x00642727` and
`0x00642967` that `fclFusePair` (`0x003124a0`) and `fclFuseTripleArcana`
(`0x00311ea0`) read as `[low * 24 + high]`; each bank is the 526 bytes from cell
(1,1) to cell (22,22), and every overlaid byte must be 0 (no fusion) or an
arcana 1..22. Nothing else derives from these charts (the added-persona fusion
overrides start from the arcana the chart gave). The static `ITEMTBL` bank
(`tables.overlay@2`) is the retail item table as the game reads it: the init copy at
`0x0079bf10` (after the 16-byte header at `D_0079bf00`), 1536 rows of 68 bytes.
It is loaded only while the item pointer `0x007642a0` names that copy and the
catalog identity is retail (an item catalog publishes its own base), checked
again inside every apply and before every restore. It is field-restricted: only
each row's unlock-gate words at `+0x3c` and `+0x40` (`datItemMoonGateFirst` and
`datItemMoonGateSecond`, which the shop lists and Daidara's unlocks test) may
differ from stock; any other differing byte refuses the layer. Item UI, text and
list owners act only on an active catalog, so with the retail table every reader
reads these rows live. The init read step (`001117c0`) re-publishes the copy
through `p4_game_tables_items_native_use_builtin`, which notifies
`p4_game_tables_overlay_static_published`: the fresh copy becomes stock and the
layer is re-applied, or dropped with a diagnostic when it no longer differs only
in its gates. `p4_game_tables_overlay_apply`
replaces one owner's layer all or none: the allowlist is `PERSONA0`/`1`,
`UNIT0`/`1`/`2`, `ENCOUNT0`..`4`, `FUSION0`..`3` and `ITEMTBL`; sizes must equal
the extent; a bank has one owner; it refuses while
`p4_game_table_catalog_identity` reports package sources (read on every apply
and observer call, never cached: an overlay applied before a relocating
publication would be baked into the expanded bank), while the battle state
pointer (gp `0xb3ac`) is set, and before its session restore is registered. The
first overlay of a bank captures its live bytes as stock; writes go through the
canonical guest byte copy.

With no table source published nothing derives sealed products from these
banks: the element route, knowledge gate, persona compendium/fusion/model owners
and AI-record bridge stay disabled and every retail reader reads them live, so
an overlay is what the game uses. While any overlay exists the service registers
the extent observer (`p4_game_table_extent_observe`): a loader re-copy or a
relocation of an overlaid slot re-captures stock and re-applies the layer, and a
changed extent or catalog drops it instead of copying a mismatched layer. The
session ends at every load and New Game reset through an optional native save
provider (`p4pc/table-overlay-session`, never written): its commit clears the
observer and restores every bank whose slot still names it, before any mod's
load notification. VM teardown restores that mod's banks. Without an overlay
nothing is registered and no game byte changes. `game_table_overlay` exercises
the allowlist, extents, owners, refusals, observer, the static charts, the
gate-restricted item table (pointer guard, catalog guard, field window,
re-publication) and session restore over synthetic guest banks; it is not a
gameplay witness.

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
metadata fields without I/O or provider inference. The current startup handler
authorizes the contained file through its selected provider plan; the metadata
parser by itself neither opens files nor admits a mod. See
[Source boot lifecycle](#source-boot-lifecycle) for the runtime path.

`game_table_xml.h` decodes one `<p4-tables schema="1">` root with flat,
non-executable records. Numeric attributes are checked u32 decimal or `0x`
hexadecimal values; the declaration revision supplies each authored revision.
Each row binds to the explicit source handle, and a failed candidate is discarded.

| Record | Required attributes |
| --- | --- |
| `element` | `id`, `flags`, `family`, `order`, `key`, `display-name`, `short-name`; optional all-or-none `icon-owner`, `icon-key`, `icon-revision` (skill art) and `header-icon-owner`, `header-icon-key`, `header-icon-revision` (column-header art) |
| `channel` | `id`, `kind`, `key`, `display-name` |
| `affinity` | `kind`, `entity`, `profile`, `element`, `channel`, and either `value` and `scalar` or `retail-row` and `retail-element` |
| `action` | `source-kind`, `source-id`, `component`, `element`, `channel`, `key` |
| `alias` | `kind`, `entity`, `native-profile`, `profile`, `knowledge`, `key` |
| `effect` | `source-kind`, `source-id`, `element`, `channel`, `flags`, `key` |
| `offensive-effect` | `source-kind`, `source-id`, `element`, `flags`, `key` ([Offensive effects](#offensive-effects)) |
| `damage-cover` | `source-kind`, `source-id`, `element`, `channel`, `percent`, `family`, `key` ([Damage cover](#damage-cover)) |
| `status-ailment` | `element`, `flags`, `cure-mask`, `key`; optional `default-from`, `crit-bonus`, `transfer` ([Side ailments](#side-ailments)) |
| `status-rider` | `skill`, `element`, `rate`, `key` ([Riders and battle readers](#riders-and-battle-readers)) |
| `skill` | `id`, `active`, `element`, `element-aux`, `inheritance-group`, signed `inheritance-rate`, `key`, and either exact 40-byte `metadata` and 4-byte `presentation` hex or `derive` (retail active anchor 1..439) with optional `power`, `cost`, `accuracy`, `critical`; optional `effect-source` (retail active skill 1..439) and `delivery` (`physical` or `magic`) |
| `item` | `id`, `key`, either an authored icon reference or `icon-source="anchor"` on a derived weapon, and either exact 68-byte `metadata` hex or `derive` with optional overrides ([Derived items](#derived-items)); optional requirement ID/quantity pairs and model IDs |
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

### Derived items

With `table-source.item-derive@1`, an `item` row with `derive="<retail item id>"` instead of `metadata` copies
that anchor's 68-byte row from the selected ITEM input (the disc's ITEMTBL, or
a selected replacement of it) when the item family seals, then replaces only
the fields it names. A forged weapon can then take its anchor's attack, hit,
critical byte, equip mask and model from the user's own data while the mod
ships its name, property, element and price:

```xml
<item id="1600" key="fusion:forged-blade" derive="0x004" value="0x24"
    element="2" price="1200" icon-owner="art" icon-key="art:blade" icon-revision="1"/>
```

| Override | `P4ItemRow` field |
| --- | --- |
| `primary-value`, `secondary-value` | +08, +0A (u16; a weapon's attack and hit) |
| `property` | +0E (u8) |
| `stat-bonus` | +14, five signed i16 as `s,e,m,a,l` |
| `granted-skill` | +24 (u32) |
| `value` | +28 (u16, the `datItemValue` property word) |
| `accessory-percent`, `element` | +2A, +2C (u16) |
| `category-mask`, `price` | +30, +34 (u32; the mask is equip compatibility) |
| `available` | +38, `month,day` (signed i8) |
| `moon-gates` | +3C, +40, `first,second` (u32) |

`kind` and `flags` always stay the anchor's. The anchor must be a retail item
(below 1536); a row has `metadata` or `derive`, never both. A derived weapon
without `right-model` binds the anchor's own native type-7 keys (right =
anchor, left = anchor + 1000 for unit 2), so it looks like its anchor; named
models are checked against the resolved metadata at seal, and models on a
non-weapon, a missing unit-2 left key, a unit-3 key outside 0x70..0x95 or a
weapon anchor above 255 without models refuse the seal. Derived rows request
the ITEM and MODEL inputs; names and descriptions remain `string` rows. As with
UNIT-derived affinity rows, the source owns the row and the copied values'
origin is the sealed ITEM input, so no retail number is shipped.

`table-source.item-icon-anchor@1` permits `icon-source="anchor"` instead of an
authored `icon-owner`, `icon-key` and `icon-revision` triple. It is explicit,
exclusive, and valid only for a derived weapon whose selected anchor has a
native weapon-family flag. It retains that anchor's identity from the sealed
ITEM input. Inventory flag icons and the weapon shop use their original native
sprite packs and category selection; the Camp inventory row retains its native
flag-selected sprite. Actual use records both the new row and selected ITEM
input. No PNG resource is created or pinned for such a row; authored references
still require an exact prepared and pinned resource. These native paths and
expanded-item gameplay require game and renderer qualification.

## Authored image resource service

`P4Resources` owns authored raster generations and transient numeric tokens;
the actual package/source producer supplies decoded RGBA and its already
selected source metadata. Resource names and diagnostic provider strings do
not authorize a package or substitute for a retained provider/file plan.
Freeze exposes a generation atomically. UI products resolve stable
owner/key/revision references later and pin actual tokens for their lifetimes;
table semantics never depend on a resource token or stage an image in schema0.

In an explicitly no-device battle-simulator session only, image preparation
validates the selected source, PNG header and bounded dimensions without decoding
the compressed payload. Runtime publishes metadata-only tokens (no raster or
pixels), retains normal ownership and pin/retirement rules, and suppresses
device drawing. This mode cannot validate PNG payload integrity or rendering;
ordinary play still fully decodes, validates and materializes each image.

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

## Derived table sources

A derived source replaces existing rows of the selected retail inputs with
rows a native producer builds on the player's machine, for example from
another install the player owns. Producers are native code that core code
registers with `p4_game_tables_derived_register()` (`include/p4pc/game_table_derived.h`)
before package boot. A package selects at most one. Declare
`requires = manifest.table-derived@2` in `[mod]` when using these derived
sources, including relocated pair or encounter inputs:

```ini
[table-derived golden]
producer = p4pc:golden
revision = 1
inputs = skill unit msg persona item
```

`producer` must be registered in this build and `revision` must equal the
producer's own; `inputs` names the inputs the producer reads or replaces (item,
skill, unit, msg, persona, encount, effect, model, aicalc). A section `when`
gate works as it does for `table-source`. The declared inputs are prepared
like any requested input; once one packed input is declared, all eight are
captured, as above.

At seal, after the inputs are prepared and before any family reads a span,
each producer runs once. It can read the uncomposed span of any declared input
with `p4_table_derived_base()` and pushes:

- same-size rows (`p4_table_derived_sink_rows`): ascending existing row ids in
  one span, at that span's native stride. Strides are admitted only where a
  family compiler already validates the layout: ITEM 0 (68 bytes after the
  16-byte header), SKILL 0/1 (2/40), UNIT 0/1/2 (60/32/32), MSG 1/2/3 (19/21/17),
  PERSONA 0/1/5 (14/70/4), EFFECT 0 (4), MODEL 1/2 (232/88) and AICALC 8 (164);
- whole spans (`p4_table_derived_sink_span`) of the same length. A length
  change refuses, except for the skill-pair list and the four ENCOUNT blocks
  below;
- retail item names (`p4_table_derived_sink_item_names`): ascending item IDs
  below 1536, each name 1 to 31 printable ASCII characters. Retail item names
  live in the executable (`datGetItemNameId` `0x001067F0`), not in ITEMTBL. One
  package owns them; a second producer naming items refuses both. `p4.data`
  edit layers may rename after it. The names are copied to the guest when the
  battle tables complete, and the override returns them from then on;
- at least one fingerprint (`p4_table_derived_sink_fingerprint`): a label and
  SHA-256 of what it read;
- optionally, recorded sources (`p4_table_derived_sink_source`): a copy of a
  raw file it loaded (1 byte to 4 MiB, a short printable name unique per
  producer) for the selecting package's script to parse itself. Copies are
  charged to package memory and freed when the session ends.

PERSONA 4, the skill-replacement pair list at `0x007644DC`, may take another
length. Its readers are Shuffle Time's skill change (`0x0036F880`, forward until
an original of 0 or backward until a replacement of 0), roster fusion's skill
change (`0x003096D0` and `0x00304580`) and the level-up pick (`0x0011E490`,
behind a chance that is always zero in retail); none uses a count. A
replacement list holds 1 to 4096 pairs: the last is (0, 0), no other pair has a
zero half, no original skill appears twice (the level-up pick keeps at most
eight candidates, one per known skill), and every skill is a retail one (below
576) or one a package declares, checked once every family seals. The
loader still loads, authenticates and copies the selected original block. At
battle-table completion the `table-derived` participant checks that the loaded
block is that original, copies the new list to the guest heap followed by three
zero pairs (roster fusion's pick reads up to three pairs past the terminator,
which in retail land in the loader's alignment padding), and points the slot at
the copy with its extent; a reload's completion points it at the same copy
again. Session end with a live image puts the loaded original back.

ENCOUNT 0, 1, 3 and 4 may grow, in the PS2 row layout (`include/p4pc/game_encounters.h`):
formations (24 bytes, `0x00764504`), selectors (10, `0x0076450C`), weighted
groups (248, `0x00764508`) and record groups (348, `0x00764514`); the 200-byte
block 2 has no reader and keeps its length. Every reader indexes these rows by
an ID another row, a field step or a script names, never by a count: the
`Encount_*` pickers (`0x00161630` to `0x00161C80`), the field and battle
formation readers (`0x00174680`, `0x00163990`, `0x001932F0`, `0x001B1D70`,
`0x001B2380`, `0x001B3A00`, `0x001DA130`, `0x001EA420`, `0x001EF7E0`,
`0x001EF8C0`, `0x001F0620`, `0x001F56D0`, `0x001F73D0`, `0x0021DE90`,
`0x0022BD20`, `0x0022FDC0`) and `datUnitCreateEncounterGroup` (`0x00231630`);
the only literal is boss formation 533 at `0x001F0620`. A grown block holds
whole rows, no fewer than the selected block (so every ID a script or field
step names stays in range) and at most 4096. At compose every selector must
name an existing weighted and record group and every weighted entry an existing
formation; once every family seals, each formation enemy must be a retail enemy
(below 336) or a declared one, and each record item a retail item (below 1536)
or a declared one. Golden's weighted groups are 252 bytes with a third rate
column; a producer translates them to the 248-byte layout. The loader keeps
loading the original blocks into the shared battle arena, and the
`table-derived` participant moves each grown block's slot to a heap copy as it
does the skill pairs. No save tag holds these indices: the dungeon boards (tag
41) keep treasure placements, each treasure's kind (record flag bit 0) and
shadow counts. Restoring a board (`0x0015AC60`) re-picks each treasure from its
step's record group in the current tables, counting only records of that kind
(`Encount_FindRecord` mode 0 or 1); a kind with no weight there divides by zero.
So at compose every step the selected selectors have must keep exactly the
treasure kinds (records with an item and weight) its selected record group
yields, and a board saved under either table set restores under the other. In
Golden, groups 4, 14, 38, 43, 44 and 46 have no locked (bit 0 set) treasure
where PS2's have one; a Golden producer keeps PS2's group for those steps.

One package owns a (input, span) family: a second producer naming it refuses
both packages. Any refusal, a missing fingerprint or an absent source refuses
the selecting package before raw exposure, and nothing is composed. The input
keeps the fingerprint of the selected original, so admission still
authenticates the loader's actual buffer; it then overwrites that buffer with
the composed bytes before the native copy, so the native banks and every CPU
family see the same rows. The catalog identity appends the selections, their
fingerprints, recorded sources (name and SHA-256), the composed inputs' SHA-256
and each relocated list's SHA-256; it is unchanged when no producer is selected
and no delta declared.

`p4_table_derived_source(producer, name)` answers with a recorded source only
once the table boot has published the catalog it was composed into: not while
producers run, not after a refusal and not after the session ends. The view is
read-only; script access copies it ([`p4.tables.source`](lua.md#derived-table-source-files-tablessource1),
capability `tables.source@1`).

The derived-source fixture runs a test producer over real package boot and a
synthetic disc: base spans, row and span composition, every refusal above, a
recorded source hidden during production and exposed after publication, the
admission rewrite of a buffer that still holds the original, and a 108-pair
list and four grown ENCOUNT blocks relocated over their synthetic originals
through two loads and a session end in guest memory (and, without one, slots
that never move). It does not prove a real producer, the actual loader or
gameplay.

### Table deltas

With capability `manifest.table-delta@4`, a package that only changes a few
numbers of existing rows, on top of whatever the producers made of them
(Golden's rows, for example), declares named-field deltas instead of shipping
rows:

```ini
[table-delta rebalance]
file = data/rebalance.xml
revision = 1
when = rebalance
```

```xml
<p4-table-deltas schema="1">
<skill-delta id="0x02" power="80"/>
<skill-delta id="0x05" power="51" cost="12"/>
<skill-delta id="0x89" power="100" min-hits="1" max-hits="4" split="1"/>
<skill-delta id="0xC0" max-heal="25"/>
<learn-delta persona="0xC0" skill="0x0F" level="23"/>
<learn-delta persona="0xC2" skill="0x7A" replace="0x240"/>
<learn-delta persona="0xC6" add="0x9D" level="0"/>
</p4-table-deltas>
```

`file` (at most 256 KiB, at most 2048 deltas) and a nonzero decimal
`revision` are required; `when` gates the section as for `table-source`.
Values are decimal or `0x` hexadecimal. Only these fields exist:

| Record | Field | Row bytes | Range |
| --- | --- | --- | --- |
| `skill-delta id=` (retail active skill 1..439, SKILL 1) | `power` | +0x12 s16 | 1..9999 |
| | `cost` | +0x04 u16 (SP, or the HP percentage of an HP-cost skill) | 1..999 |
| | `accuracy` | +0x0E u8 | 1..100 |
| | `critical` | +0x25 u8 | 0..100 |
| | `min-hits`, `max-hits` | +0x0F, +0x10 u8: the fewest and most hits the skill rolls | 1..8 |
| | `split` | +0x11 HP effect kind: 1 sets 14 (split), 0 sets 1 (formula damage) | 0..1 |
| | `ailment-rate` | +0x19 u8: the base chance of the row's rolled ailment | 1..99 |
| | `max-heal` | +0x11 HP effect kind 11 and +0x12 s16 percentage; opts into Divine Grace | 1..100 |
| `learn-delta persona=` (party Persona 0xC0..0xD7, PERSONA 2) | `skill` + `level` | the learn event of that skill moves to the level (0..99) | |
| | `skill` + `replace` | that event teaches the other skill at the same level | |
| | `add` + `level` | the first free event learns the skill at the level | |

`max-heal` requires the composed row's HP effect kind 2 (formula healing)
and owns both the effect and power fields: another `max-heal`, `power` or
`split` delta for that skill conflicts. It converts the row to native kind 11,
records the opt-in with the exposed catalog and removes it at teardown. On the
HP channel, the amount equation floors max HP times the percentage and keeps
native kind 11's minimum of one; for a source with Divine Grace it then
truncates the 1.5 multiplier, except for the native `options & 1` estimate.
No formula-heal variance or formula-only recovery-stage modifier is added.
Unmarked kind 11 rows, SP effects, full heals and all other metadata are
unchanged.

PERSONA 2 is the party block at `0x007644D4`: 24 rows of 622 bytes, row
`persona - 0xC0`, with 32 learn events `{u8 level, s8 kind, u16 skill}` from
+4 (kind 1 learn, 2 forget, 0 empty). The level-up scan
(`datPersonaPreviewLevelUp`, `0x0010BE60`) stops at the first empty event and
reads the run of events between the old and new level as one contiguous block,
and `datPersonaLearnFixedSkills` (`0x0010D150`) teaches the level-0 events when
the Persona is created. So a learn delta names an event by the skill it teaches
(a learn, kind 1), never by its index, which differs between PS2 and Golden
rows; after a row's deltas its used events are sorted by level (stably, so a
learn and a forget at one level keep their order) and stay 622 bytes. An
event the row lacks, a skill the row already learns, or a full row refuses the
package. A skill a learn delta teaches must be retail (below 576) or one a
package declares, checked once every family seals.

Revision 2 added the hit fields. `datCalcRollSkillValue` (`0x00243650`) rolls
a skill's hits per target, uniformly from `min-hits` to `max-hits`, after
`datCalcHitResult` has decided that target's outcome (hit, critical, miss)
once; the per-target loop in `001f14f0` then resolves each hit as one full
formula hit with its own 95..105% variance. The range stops at 8, the most any
retail row rolls (Hassou Tobi): each hit takes a 0x20-byte result row in the
action, whose last rows the endurance state reuses. `split="1"` sets HP effect
kind 14, which divides each hit's formula damage by the hits rolled against
that target, so a skill's total equals one hit of its power however many hits
it rolls; each hit deals at least 1, the loop still stops at a hit that
defeats the target, and the final-damage stage sees each divided hit. The PS2
executable passes `datCalcSkillAmount` a hit count of 1 there, so kind 14 never
divided, and no PS2 or Golden SKILL row uses it; the port passes the rolled
count (`config/mirror_fixes.txt`, 001f14f0), which only kind 14 reads. AI
damage estimates still ask for one hit, which for a split skill is its
expected total. The hit fields are checked against the composed row, after the
producers and every package's deltas for it: the skill must roll its own hits
(a weapon skill, `flags00` bit 1, takes the wielder's), `split` needs formula
HP damage (effect kind 1 or 14), and `min-hits` must not exceed `max-hits`.

Revision 3 added `ailment-rate`, the byte `datCalcInflictStatus`
(`0x00238940`) reads as the row's base ailment rate: at 100 or more the ailment
lands outright, otherwise the native chance formula for the skill rolls it.
Retail and Golden riders keep their published rates there (Golden raises
Skull Cracker 20 to 35). The formula, the affinity, Fear and passive boost
multipliers, the status chance stage, the 0..99 clamp and the single roll are
unchanged, as are the Guard, immunity and null exits before them; enemies and
the party read the same row. The field is checked against the composed row:
its status mode (+0x18) must be 1 (inflict) or 3 (inflict one of the mask) and
its status mask (+0x1C, low 24 bits) nonzero without the instant-KO bit
0x80000, whose chance is a separate channel. The field stops at 99, so an
authored rate never bypasses the formula, the stage or the roll.

Deltas own no family: they patch the composed rows inside the derived compose
stage, after every producer and in package boot order, before any input byte
changes are written and before the composed digests. Deltas alone, with no
producer, compose the same way. A target (a skill row's field, or a party
Persona's skill) one declaration names twice refuses it; one two packages name
refuses both. The admission rewrite and the catalog identity cover deltas like
producer rows: the identity appends each declaration (package, key, revision,
file SHA-256) and each delta, and is unchanged when none is declared.

The derived-source fixture also composes deltas after the test producer (a
skill row both change, a row only the delta changes, a producer row given a hit
range, split and an ailment rate, moved, replaced and added learn events
re-sorted, and the admission rewrite), deltas without a producer, and refuses a
conflict between two packages, a target named twice, an out-of-range value, an
unknown field, a missing learn event, a full row, an undeclared added skill, an
out-of-range hit count, a minimum above the maximum, split on a non-damage row,
hits on a producer's weapon row, an out-of-range ailment rate and an ailment
rate on a row without a rolled ailment.

The bundled hard-mode rebalance deltas are selected by the restart-scoped setting at table boot, not by
the armed state of the current hard-mode run. They can therefore affect an
Off-mode run while the setting is on. Safe run-aware publication, including
new-game, load, checkpoint and legacy-save choices, remains unresolved; do not
treat this option as a finished hard-mode activation boundary.

The bundled hard-mode **Skill Rebalance** option applies the `rebalance` deltas
to both sides after retail or Golden producers. The table section is gated at
package boot by the restart-scoped `rebalance` toggle, which defaults on,
**not** by the difficulty choice or an armed save. Thus an enabled hard-mode
package with difficulty Off can still apply these rows; the difficulty setting's
“OFF: the original game” wording does not describe that table behavior.
Run-state isolation is not implemented here.

#### Run-isolation investigation (not implemented)

The run's saved `mode`, feature flags, tuning revision and epoch are managed by
`p4pc.hardmode/main.luau`, not by table-delta admission. Its reset notification
clears the run and marks a New Game pending only for Hard or Harder. A subsequent
load cancels that pending state: a hard-mode block restores its saved run even
when the current mode differs; a plain save stays plain unless the one-shot
adoption option and a non-Off mode are selected. Otherwise the first field or
battle event arms a pending New Game. Neither `runFromSettings` nor the saved
run block records `rebalance`. Consequently a boot-time mode gate would both
miss saved runs loaded under Off and apply changes before a plain load has
settled. It cannot provide the requested isolation.

The following is a required design, **not current behavior**:

| Transition | Required rebalance state |
| --- | --- |
| Boot, reset, or a plain load without adoption | Unarmed: composed baseline without this delta, regardless of mode. |
| New Game armed in Hard/Harder, or explicit adoption of a plain save | Atomically select the rebalance choice captured for that run; Off mode cannot arm it. |
| Load an armed hard-mode save | Select that save's recorded choice, even if the current mode or restart setting differs. |
| Switch between armed and plain saves, reset, or fail a load | Do not leak the previous run's rows or leave a partially selected layer; a failed transaction must not publish a new run's layer. |

The table service needs an owned, validated **session-selectable named-delta**
seam, not a Luau byte patch. It must retain the composed baseline after all
other producers/deltas and a validated rebalance variant, preserving ownership,
conflict checks, source provenance and the catalog/save identity contract.
Both variants must be available even when the current restart toggle is off,
because an earlier save may require the rebalance. Selection and restoration
must publish the complete variant and its side state together, at a proven
transaction boundary before any skill consumer reads the new session. A
re-copy, relocation, expanded-skill publication, teardown and refused load
must not reintroduce the wrong variant. The setting controls newly armed runs,
not whether a saved run's variant can be prepared. The run snapshot must record
the choice and an appropriate delta revision/identity, and authorization must
refuse an unavailable or incompatible saved variant rather than silently use
the current setting.

This seam is absent. `game_table_boot.c` admits the `when`-gated declaration
once at boot; `game_table_derived.c` validates and writes the deltas into the
composed SKILL input and hashes that result. `game_skills_native.c` can then
materialize and publish a separate expanded metadata bank. `SKILL1` is not in
the `p4.tables` overlay allowlist, and that overlay refuses package table
catalogs; it cannot select between these composed products. The existing
session overlay restores before mod load notifications, but does not provide a
transactional semantic skill-delta selection. `battle_start` is emitted after
battle initialization, and the current overlay also refuses during battle, so
using that fallback event is not proof of selection before all readers. The
missing seam must explicitly establish New Game and load ordering, including
failed authorization, before it is used.

The saved Lua value is staged and authorized before commit, but the native
save provider's staged object is a private Lua snapshot; no table selector
receives its decoded choice during validation. Notifications happen only after
commit. Reset begins before `datInitDefaultGameState` initializes units and
roster, and commits afterward (`overrides/001113b0.c`); identifying the first
safe publication and every potential reader during that interval is still
necessary. A new selector cannot simply patch the live slot: retail
`p4_game_skills_definition_find` returns the retained composed input to action
and AI readers, while native expanded skills use a materialized bank.
`p4_game_skills_seal` also derives added skill rows from the then-composed
anchor. A generic implementation must make those views and derived rows
consistent with the selected variant, or reject affected compositions before
publishing; neither is currently implemented.

The load boundary is more specific than the Lua notification: a complete card
read calls `p4_mod_save_transport_loaded` (`ps2_memcard.c`), which stages and
validates every provider and obtains any warning acknowledgement before
reporting success. `mod_save.c` discards staged values on decode/validation
failure or abort; it commits every provider before notifying any. At restore,
`overrides/0010e710.c` first checks that the payload matches the staged image,
then calls `datSaveRestoreSection` for the saved sections, and only afterward
commits and notifies. Thus authorization precedes those restore writes, but
the existing Lua `load` callback follows them. This does not prove that no
restore-section code reads SKILL data, nor that a prior default-state reset
left the previous session intact after a refused read. `field_enter` is also
an after-hook on the field task once its ready-state checks pass
(`events.c`), not a demonstrated pre-reader New Game boundary.

The selector must additionally bridge a private Lua staged value into native
validation without committing it early, and define the reset/read failure
semantics across the prior default reset and the later card transaction. The
source above does not establish that bridge or a pre-reader New Game boundary;
the present notes and fixtures are prerequisites, not an implementation of
transactional publication.

The entire XML declaration must move together: the seven all-target rider
rows (Blight, Virus Wave, Mind Slice, Crazy Chain, Seal Bomb, Mustard Bomb and
Navas Nebula) keep their approved power/cost changes and composed rider
metadata; the single-target riders and split multi-hit rows keep their approved
changes too. All six healing rows are included. Four `max-heal` rows also set
`derived.max_heal`, read by `datCalcSkillAmount` for Divine Grace, while
Diarahan and Mediarahan change cost only. Selecting just row bytes, just the
healing marker, or only the damage fields would split the approved behavior.
Battle damage, hit and status calculations, healing and its estimate path,
skill costs and menu/AI readers of SKILL1, plus expanded-skill materialization,
are affected consumers and need qualification against the same selected state.

Existing hard-mode save blocks contain no rebalance flag: they cannot reveal
whether the old restart toggle was on when saved. A migration must obtain an
authoritative old choice or explicitly refuse/resolve this ambiguity; inferring
it from the current mode, toggle, or the presence of a hard-mode block would
silently alter some runs. The derived-source fixture composes the actual seven
all-target rider and six healing records together over synthetic original and
producer-replaced rider rows, on and off. It checks whole rows, preparation
visibility and exposed percent-heal markers. Consumer regressions seal those
inputs into the skill-definition service and exercise a synthetic expanded
bank's marked and unmarked derived rows, an independently authored byte copy,
slot replacement, packed-bank reload/republish and teardown. The amount-equation fixture separately exercises
marked and unmarked expanded IDs with a synthetic marker provider. Both table
variants are selected at package boot; this does not compose every XML record
or switch a live session. The headless hard-mode lifecycle fixture exercises
both New Game arming events, Off, plain-load cancellation, saved runs loaded under Off and
malformed-block authorization, and demonstrates that two otherwise identical
snapshots made with opposite rebalance settings are indistinguishable. Its
settings are synthetic and do not select a real catalog. Neither fixture proves
run-isolation or gameplay. The missing integration
coverage still includes whole-declaration switching in both directions,
restart-setting changes, authoritative legacy resolution or refusal, aborted
and refused loads across resets, and re-copied/expanded banks across run selection.

This is a partial implementation, not the complete approved physical or
P3-identity family. Alongside the existing elemental tier deltas, it changes
these rows (power / cost; physical costs are the row's HP percentage and retain
the native fixed cost component):

- Bash 50/6, Cleave 50/6, Assault Dive 50/6, Sonic Punch 45/6, Skewer 55/6,
  Single Shot 40/6, Power Slash 120/11, Black Spot 95/11, Gigantic Fist 190/16,
  Vile Assault 300/16, Rainy Death 170/16, Brave Blade 200/16, God's Hand
  320/20 and Primal Force 285/20. Their other fields, including Vile Assault's
  Down bonus and the severe skills' crit rates, stay native.
- Fatal End 95/11, 92 accuracy, 20 crit; Mighty Swing 80/11, 95/30; Heat Wave
  70/15, 90/10; Deathbound 65/15, 88/20; Heaven's Blade 245/20, 95/40;
  Vorpal Blade 200/28, 92/13; Gale Slash 70/15, 95/5; Herculean Strike 85/15,
  85/5; Vicious Strike 145/22, 90/8; Cruel Attack 100/11, 99/5. Fatal End has
  no Fear rider and Cruel Attack keeps its native Down bonus.
- Ragnarok and Panta Rhei 325/30 SP, preserving their native accuracy and
  one-hit target; Life Drain 35 fixed HP/3 SP, limited by the target's current
  HP as before. Megido 55/18 SP, Megidola 110/27, Megidolaon 165/40, Black
  Viper 255/40 and Morning Star 204/45.
- Multi-hit physicals, all `split` (one tier hit divided across the hits rolled
  against each target), with their native hit ranges, accuracy, crit and
  riders: Double Fangs, Kill Rush and Twin Shot 55/6; Hysterical Slap, Arm
  Chopper and Brain Shake 45/6; Torrent Shot 115/11; Rampage (the 75-accuracy
  row 0xAB, not the 300-power row 0xB0) 55/8; Swift Strike 50/8; Atom Smasher
  55/15; Blade of Fury 75/15; Aeon Rain 220/22; Agneyastra and Hassou Tobi
  330/28. The P3-identity rows also set accuracy/crit: Tempest Slash 100/11,
  95/10, 1..4 hits; Myriad Arrows 160/22, 87/5, 2..5 hits; Arrow Rain 195/22,
  80/5; Akasha Arts 160/22, 85/10.
- Single-target physical ailment riders (family 5, F5-D1) take a 45% base
  rate: Poison Skewer, Skull Cracker and Muzzle Shot 45/6; Cell Breaker 85/11;
  Poison Arrow 75/11, 95 accuracy, 5 crit (its P3-identity row); and the
  multi-hit Hysterical Slap, Arm Chopper and Brain Shake above. Golden's rate
  for each is 35 (Poison Arrow's already 45).
- Single-hit all-target ailment physicals retain the composed row's ailment
  and rate, accuracy, critical, hit count and delivery: Mind Slice, Crazy Chain,
  Seal Bomb, Mustard Bomb and Navas Nebula 70/15; Blight 140/22 and Virus Wave
  125/22. The multi-hit all-target Atom Smasher above also retains its native
  rider. Golden rates remain Golden when its producer is enabled; without it,
  these changes do not overwrite the retail rates.
- Family 7 (`skill-rebalance-design.md` §10, F7-D1/D2): Dia 25%/3 SP,
  Diarama 50%/6, Media 22%/12 and Mediarama 42%/24 use `max-heal`;
  Diarahan (0xC2) and Mediarahan (0xC5) keep their composed 100%-of-maximum
  effects and change only cost from 18 to 12 and 30 to 36 SP respectively.
  Targeting and unrelated metadata remain unchanged. The
  [Golden skill listing](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Skills)
  identifies these skill IDs. Synthetic table fixtures use the actual XML
  records, check on/off composition and conflicts, while the synthetic native
  amount fixture covers the percentage, rounding, Divine Grace, maximum-HP
  boundaries, estimate, SP and recovery-stage exclusions. These fixtures are
  not in-game or balance qualification.

The synthetic drain fixture composes the shipped delta on and off over a
synthetic retail-layout row, then passes that composed row to the production
amount equation. It checks the 30/35 HP amount and the target-HP limit for
either side; it does not exercise the transfer, menu cost, battle, save or
run activation.

Niflheim and Thunder Reign remain unchanged until their 60% Freeze/Shock
riders can share the optional status setting, immunities, boss windows, decay
and Guard rules: neither row carries a status payload, and Freeze and Shock are
side ailments (leaves 24 and 25) whose rider path at 0x00238940 is not part of
the landed side-ailment service. Pralaya's KO rider also stays unchanged. Those
omissions are intentional, not damage-only approximations. Family 5's SMT V
wear-off and repeat-affliction decay are separate and not implemented here.

`mod_hardmode_game --scenario split` (witness lane, `P4PC_MOD_WITNESS=1`)
checks the live rows: with Skill Rebalance on, each single-target physical rider
above holds base rate 45 with its native status mode and mask, and the
single-hit ones their power, HP cost, accuracy and crit. It also compares the
seven single-hit all-target riders on and off, requiring only power and cost
to change and the original rolled ailment to remain. These added comparisons
have not yet been witnessed; the test does not witness a landed ailment or its
frequency. For the multi-hit rows, from
`story0420_5f` with Skill Rebalance on, every multi-hit
row above is live in guest RAM with its power, HP cost, effect kind 14 and the
named accuracy, crit and hits, the others native, and with it off every one is
native kind 1. In the battle harness the protagonist's Myriad Arrows lands 2..5
hits on its lone target, and a witness-only delta that gives the 0xB0 Rampage
row power 20, accuracy 100, crit 0 and exactly 3 hits shows the same seeded hits
without and with `split`: each split hit is a third (2.5..3.5x) of its whole
one.

## Native preparation and shared completion

`P4GameTablePrepareFn` is a native, nonyielding preparation callback over the
actual loader label and borrowed managed bytes, not gameplay Lua. The core
checks actual winning-byte provenance/claims and finalizes staged bindings
before publication. A path alone is not provenance. Missing preparation is
`P4_TABLE_PREPARE_REQUIRED`; conflicting callback/user pairs are refused.

Shared battle data completion at0x0010d480 has ordered families `item`, `skill`,
`persona`, `enemy`, `table-source`, `table-derived`, `element`, `element-route`,
`element-ai`, `element-knowledge` and `element-ui`. Expanding owners register one
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
missing pair. Damage and KO channels are distinct keys. A [side
ailment](#side-ailments) with `default-from` is the one pair that exists
without a row: every enemy alias without its (leaf, `p4:status`) row has that
defaulted key. Retail enemies (below 336) keep store bits for what has no key:
their natural status pairs (retail leaves 8..15 on the status channel) that
have no side row get one bit per enemy and slot, because the retail Analyze
bitmap ends at leaf 7, and with no alias for their native profile each
defaulted side ailment gets one bit per enemy and side-ailment index. Defaulted
pairs exist whether or not a run enables side ailments.

The `game-tables.enemy-knowledge` package handler is inert unless the tables
request expanded elements. Its CPU pass registers the REQUIRED
`p4pc/enemy-knowledge` save block over Root's sealed source ledger; its runtime
pass registers the `element-knowledge` completion participant, which requires
`element`. Prepare compiles the unpublished store (a reload of the same
snapshot keeps its observations) and commit publishes it. The block (schema 3)
records each known key with its canonical alias, element/channel descriptors
and side row (its side ailment for a defaulted key) through the ledger
references, then each retail enemy's nonzero status bits by enemy id, then each
retail side bit as its enemy id and side-ailment ledger reference. A schema 2
block loads with no side bit known and a schema 1 block (side keys only) also
with no status known. Nothing known omits the block and an absent block clears
to the default. A changed alias, descriptor or source refuses the whole load.

The expanded route of 0x001f14f0 learns through
`p4_game_knowledge_native_observe(defender, frame)` at the existing
eligible-outcome gate (enemy defender, outcome not a miss), once for each
frame whose affinity was actually resolved against that defender, and never
writes the retail bitmap itself. The call follows the native affinity
resolver's route: no element or a leaf without affinity learns nothing; an
authored alias/side pair, defaulted ones included, learns its key; an
unmodified retail component without one, which the original resolver answered,
sets the original retail bit for leaves below 8 (enemies below 336) and the
store's status bit for a status-channel leaf 8..15; a defaulted side ailment's
status pair of a retail enemy without an alias sets its side bit. An ailment
landing on an enemy (0x00238940: a routed status payload that passed immunity
and its roll; a KO payload is learned at 0x001f14f0) observes the same way with
its status slot on the status channel. Anything else refuses through the
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
a retail-only enemy keeps its retail bits, and its store status and side bits
count no more than a retail ailment does.

The Analyze menu (0x00219790, element-ui) reads one displayed pair through
`p4_game_knowledge_native_analyze(defender, element, known)`: the same route
with no owned binding (a display resolves no action), so no affinity is known,
a persona is invalid and an original-route pair reads its retail bit below
leaf 8, its status bit for a status leaf 8..15 or its side bit for a defaulted
side ailment. Only after the pair resolves
does 0x001f0950's encounter override apply: without enemy metadata flags 0x4004
and with story flag 0x1403, flag 0x1408 reveals and 0x1409 hides. AI reads
never take that override (and an original-route AI read still reads exactly the
retail bitmap). The Analyze ailment row reads
`p4_game_knowledge_native_ailment(defender, slot, state, observed)`: observed
is that Analyze answer for the slot on the status channel, override included,
and state classifies the effective (wrapper) affinity word: NULL/REPEL/DRAIN
immune, RESIST resist, WEAK weak, otherwise normal; a slot the enemy has no
pair for is NOT_FOUND. Which rows to draw and any reveal policy on top (for
example a party member's Full Analysis) belong to the presenter. The
knowledge fixture covers keys, status and side bits, defaulted pairs, the
observation, AI, Analyze and ailment read routes, the participant, the handler
and the save block over the real package core; it makes no battle, menu or
save-image parity claim.

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

Every decode the retail game makes through 0x001de640 still passes through that
hookable entry: a BF setter's decode carries its explicit caller work and
command argument on the actor's carrier for that one synchronous call, so a
mod's hook on 0x001de640 sees script-chosen actions (party AI, boss scripts) as
well as table-chosen ones. A mod changes the decoded command only through
`p4_game_elements_ai_native_substitute(action, skill)`, called from that
entry's before-hook for the same action. The skill must be active and must
name no individual target: its row targets the actor itself (filter bit
`0x01` at `+0x0A`) or every unit its sides collect (nonzero class at `+0x08`),
so the game's own collector supplies the targets. With the expanded carrier
the call authorizes exactly the next decode of that command for that action.
That decode selects it as an authored command that keeps the decision of the
original weighted draw, outside the group's and the script's permits and its
argument tag; the end-of-script check still requires the action to hold that
frame. The decode, a new decision root or the end of the routed call discards
an unused authorization. A command changed without one must still be one the
drawn group and the script permit; otherwise the session fails as before.

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

Header icons and their native-size art require `manifest.element-ui@2`.

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

With `manifest.element-header@1`, any selected provider, not only the layout
owner, may give a leaf without a header icon of its own (a retail leaf, such as
the ailment leaves `p4:status8`..`p4:status15`) a header image:

```ini
[element-header p4:status8]
owner = p4pc.hardmode
key = p4pc.hardmode:panic-header
revision = 1
```

`owner` is the declaring package and the image its own `[resource-image]`;
`when` gates the section. The image is planned, checked and pinned like a
header icon, and only `p4_game_elements_ui_header_art_find` /
`p4_game_elements_ui_native_header_icon` return it: affinity columns keep their
own art. A second declaration for a leaf, an unregistered key, a leaf whose
descriptor has a `header_icon`, more than 32 such headers, or a declaration
without an expanded element cohort refuses the provider.

`[element-prompt analyze-ailments]` and `[element-prompt analyze-affinities]`
(`manifest.element-prompt@1`) take the same three fields and rules: the Analyze
page-flip prompt images, one
package each, planned and pinned like a header image and returned by
`p4_game_elements_ui_prompt_find` /
`p4_game_elements_ui_native_prompt(P4_ELEMENT_UI_PROMPT_*, &handle)`; without a
declaration there is no prompt image.

**Analyze ailment page.** In battle Analyze (btlActionAnalyzeUpdate, 001A24B0),
Square flips the affinity row to the target's ailment page and back, with
the cursor cue; every Analyze entry and target switch opens on the affinity
page. Columns follow Panic, Fear, Rage, Silence, Poison, Exhaustion and
Enervation (leaves 8, 10, 11, 15, 9, 13, 14), then the authored Freeze and
Shock leaves 24 and 25. A leaf is a column only when the target has a pair for
it (`p4_game_knowledge_native_ailment`) and a package gives it header art
(`[element-header]`, or `header-icon-*` on a visible authored leaf). A
declared side ailment is shown only while its battle gate is active. The
header uses the affinity row's known/unknown tints, and each cell uses the native
Analyze sprite for its state: immune (null, repel or drain) shows the Null
cell, resist Str, weak Wk, otherwise the plain cell, and "?" while unobserved.
While the target has an ailment page, the declared `analyze-ailments` prompt
draws at the right end above the affinity row, and `analyze-affinities` above
the ailment row; an undeclared prompt draws nothing.
Once Rise navigates (story flag 0x38) and her Persona holds Full Analysis
(skill 0x10B), every ailment column is shown as known; this is a presentation
policy, and retail Analyze reads only its own bitmap. Without any column, Square
does nothing and Analyze is unchanged.

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
236-pixel span, horizontally scaling native glyphs. Authored art draws at its
own pixel size with the transform the native glyph at that site gets, never
in a fixed box: column headers take the row's horizontal fit (and Analyze's
canvas factor and scale), skill icons are 1:1 in the battle row and
descriptor lists, and the Camp list applies its row animation's vertical
scale. A descriptor's optional `header_icon` is a second image for its
affinity-column headers; the prepared column exposes it as
`header_token`/`header_width`/`header_height`, falling back to the skill icon
when no header icon is declared, and skill rows keep the skill icon. Supply art
at the disc's cell sizes: headers 30x23, skill glyphs 34..38x27. Both images
are declared `[resource-image]` art, checked and pinned with the layout. Every
VISIBLE leaf's header art is pinned, column or not, so a header row outside the
prepared columns (an Analyze page of ailment leaves) finds it with
`p4_game_elements_ui_header_art_find` or, natively,
`p4_game_elements_ui_native_header_icon(leaf, &handle)` and draws it with
`p4_game_elements_ui_native_draw_icon` at 1.0; neither falls back to the skill
icon.
Authored art receives exactly the colour
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

`p4pc/game_damage_stage.h` is the one place a mod changes ordinary HP damage;
Luau mods reach it through `p4.damage.on` (capability `stage.damage@1`, [lua.md](lua.md#final-damage-modifiers)).
Hard mode and challenge rules register modifiers there; nothing hooks
`datCalcSkillAmount` (0x00235520) directly. The stage runs once per eligible
hit at the end of that function, after the whole retail equation (formula,
attack/defense/status/passive terms, critical, guard, the 1..31000 work clamp
and the 95..105% variance) and after the outcome: only a hit that delivers HP
damage is eligible. Fixed, percentage and scripted (hit kind 16) amounts, SP,
misses, nulls, drains, healing and item actions (`options` bit 0) never reach it.
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

The same bracket names the command being resolved (`origin`): `item` for an
item action (that call's `options & 1`), `attack` for the weapon-property
skills (Attack and its follow-ups, `datCalcUsesWeaponProperties`), otherwise
`skill`; an unbracketed estimate is `unknown`. A party source also reports its
active persona (`source_persona`, from `datPersonaGetRecordId` 0x00109360) and
that persona's level (`source_persona_level`, `datPersonaGetLevelByPcId`
0x00109400) for a skill or an unknown origin, so an estimate and the resolved
hit agree; both are 0 for an enemy source, an item or an attack. The persona
readers run only while a modifier is registered.

Order is fixed: declared priority ascending, then multiplicative before
additive within a priority, then the owning mod's load order, then
registration order. Declared priorities in use: hard mode's boss bursts 50
and Technical 100, then [damage cover](#damage-cover) at
`P4_DAMAGE_COVER_PRIORITY` (1000). Values compose in double precision and
convert once at the end with the retail truncation, clamped to the equation's
own range (1..32550). A non-finite value or negative factor is ignored for
that hit and reported once; registration while a hit is composing is refused.
`P4PC_DAMAGE_TRACE=1` prints every hit, each applied step and the result to
stderr. The 001f14f0 per-target persona scale still applies after the stage,
as retail applies it after this function. `game_damage_stage` covers the order,
single conversion, clamp and refusals. `game_elements_amount` runs the real
equation: every effect, outcome, channel, hit kind and option is unchanged with
a modifier that is consulted but leaves the hit alone, and a challenge plus
hard-mode pair composes as declared, including a repelled hit, without
drawing RNG.

The recovery, skill cost and status chance stages below share this stage's
registry (`game_stage.c`): the same order, single composition in double
precision, refusals and trace switch per stage, each with its own modifiers
and conversion. None runs while it has no modifier. Luau mods reach them
through `p4.recovery.on`, `p4.cost.on` and `p4.chance.on` (capabilities
`stage.recovery@1`, `stage.cost@1` and `stage.chance@1`,
[lua.md](lua.md#recovery-skill-cost-and-chance-modifiers)).

### Recovery stage

`p4pc/game_recovery_stage.h` changes formula HP heals. It runs at the end of
`datCalcSkillAmount` on the HP channel for the formula heal (effect kind 2,
the Dia and Media lines) after Divine Grace and the 95..105% variance, when
the outcome delivers it. Fixed, percentage and full heals, revives, drains,
items (`options & 1`) and SP never reach it. A heal reports the skill, source
and target side and ID, the 001f14f0 bracket's `resolved` and `origin`, and
the source persona and level as above. The result is truncated once and
clamped to 1..32550. `P4PC_RECOVERY_TRACE=1` traces it. `game_elements_amount`
runs the real equation over every effect, channel, outcome, hit kind and
option: only those heals consult a modifier, a declining modifier changes
nothing, and a factor composes once without drawing RNG.

### Skill cost stage

`p4pc/game_cost_stage.h` changes skill costs. It runs at the end of
`datCalcSkillCost` (0x0023D9B0) for a nonzero cost, after the percentage or
flat cost, Spell Master/Arms Master halving and the minimum of 1. Every cost
reader calls that function, so all of them agree: the battle skill list
(00205170), availability (0023ddc0), the deduction in 001f14f0, AI
(001e8d80) and the Camp/Persona cost rows (0010f600 through 00113520). A
cost reports the skill, the unit's side and ID, the resource (the skill row's
kind: HP 1, SP 2), the stock cost and the persona it is priced for: the unit's
active persona record, or, inside a 00113520 row, the record that row shows,
which may be a stocked persona. An enemy reports none. The result is rounded
half up once and clamped to 1..65535, so a nonzero cost never becomes free.
`P4PC_COST_TRACE=1` traces it. `game_amount_stages` covers its order,
rounding, clamps, the row bracket and refusals.

### Status chance stage

`p4pc/game_chance_stage.h` changes the chance of a rolled ailment or instant
KO. It runs in `datCalcInflictStatus` (0x00238940) after the native chance
(the ratio-of-products formula times the affinity, Fear and passive boost
multipliers, truncated) and before the native 0..99 clamp and the single
`RpRandom() % 100` compare. Guard, immunity and null exits, skills whose base
rate is 100 or more and the scripted unconditional KO never reach it. An
attempt reports the skill, source and target, the component (`ailment`, or
`instant_ko` for the 0x80000 mask), the attempted status mask, whether it is
an item action, the stock chance, the source persona and level (0 for an
enemy or an item) and the target's [afflictions](#affliction-count) this
battle. The result is truncated once; the native clamp then
applies. `P4PC_CHANCE_TRACE=1` traces it. `game_elements_hit` runs the real
function: an identity modifier changes nothing, the RNG draw count never
changes, and a guaranteed rate never consults the stage.

### Ailment wear-off stage

`p4pc/game_wearoff_stage.h` changes the chance that a unit shakes off an
ailment on its own turn, for party members and enemies alike. It runs in
`btlSoundSelectCureableStatuses` (0x001F6930), the decision each turn makes
about the ailments the unit carries. Eligible rolls: confusion, fear, rage and
exhaustion (cured without a roll on the third turn) and enervation and silence
(sixth; skill 0x22F halves the limit, 0x230 makes it one), from the unit's
second turn with the ailment until the turn before its limit. A roll reports
the unit's side and ID, the ailment, the unit's level and Luck, its turns with
the ailment so far, the limit, the native chance (`func_00244f60`) and the
unit's [afflictions](#affliction-count) this battle, the current one included.
The result is truncated once and clamped to 0..100; the native
`datCalcRandomBelow(100)` compare then draws exactly as retail, so the RNG
draw count never changes. `P4PC_WEAROFF_TRACE=1` traces it.

### Affliction count

`p4pc/game_afflictions_native.h` counts the ailments that land on each unit in
a battle, for prior-affliction rules such as SMT V's decay. A landing is a
`datCalcSetBadStatus` (0x00232680) call that writes rage, fear, silence,
confusion, poison, exhaustion or enervation (Dizzy and KO are not
afflictions); in battle that is a hit's status commit. The battle constructor
(`btlTaskCreate`, 0x00192E90, its `P4_BATTLE_TASK_BEGIN` request) clears
every count. Units are keyed by their DatUnit record: a record reused within
one battle keeps its count. Counting runs only while a chance or wear-off
modifier is registered; the status word changes exactly as retail.

### Buff stage

`p4pc/game_buff_stage.h` changes the multiplier a unit's buff rank gives one
stat. It runs at the end of `datCalcBuffMultiplier` (0x00233BC0), after the
native value: the side's rank percentage (rank -1, 0 or 1), Charge or
Concentrate (x2.5) on the attack or magic an attack consumes, and Lead boost.
Its six callers are the damage equation (the source's attack or magic, the
target's defense) and the hit chance (the source's accuracy, the target's
evasion), estimates included. A query reports the unit's side and ID, the
stat, the rank, whether this stat's Charge is up and the native multiplier.
The result is clamped to 0..100 and converted once to the native single
precision. `P4PC_BUFF_TRACE=1` traces it. A second rank is not modelled: the
game keeps a rank of -1, 0 or 1.

### Reward stage

`p4pc/game_reward_stage.h` changes what a won battle pays. EXP passes it per
recipient in `btlXpPrepareAwards` (0x0021E110), after the native award (the
level-difference scale, Growth 1-3 for stock Personas, Lead boost and the EXP
accessories) and before the level-up preview, so the results screen shows
what is applied: the hero (ID 1, his own level), each battle party member and
character 5 while flag 0x38 is set (character ID, their level), and each of the
protagonist's Personas (Persona ID, its level, whether it is the one he
carries; stock Personas without Growth are asked with stock 0). The team's
money passes it after the native double/one-yen flags in both
`btlXpBaseAward` (0x0021E050, which the results screen asks every frame it
draws) and `btlXpApplyAwards` (0x0021E9A0, the payment), so a modifier must
answer the same query the same way every time. The
result is truncated once and clamped to 0..999999; 0 pays nothing.
`P4PC_REWARD_TRACE=1` traces it.

### Unit stage

`p4pc/game_unit_stage.h` changes the values the game derives for a unit, on
both sides. Max HP and max SP pass it at the end of `datCalcMaxHp`
(0x00231F80) and `datCalcMaxSp` (0x00232290), after the table value, the
HP/SP skills and accessories and the party's 999 cap. An enemy starts a battle
with its table HP and SP (`datUnitInit`, 0x00231AF0), which pass it under the
same stats, so a scaled maximum starts full. The five battle stats pass it in
`datCalcGetBattleStat` (0x00232C70), the reader of every damage, hit and
ailment formula, before Enervation halves them; status screens read the
Persona's own stats and are unchanged. A query reports the unit's side, ID and
level, the value and the native value. The result is truncated once and
clamped: max HP 1.., max SP 0.., up to 999 for the party and 65535 for an
enemy; a stat to 1..99. Max HP and SP are game-wide readers, outside battle
too, so a modifier must be a cheap pure function. `P4PC_UNIT_TRACE=1` traces
it.

### Hit stage

`p4pc/game_hit_stage.h` changes the chance that an attack hits and the chance
that it lands a critical. Accuracy passes it in `datCalcResolveSkillHit`
(0x0023E6F0) after the native chance (the hit-rate formula, the evasion
skills and equipment) and its 50..99 clamp, before the `RpRandom() % 100`
compare; the critical chance passes it in `datCalcHitResult` (0x002411A0)
after the native chance (the scaled-ratio formula, the target's halving and
the source's doubling skills, critical equipment and buffs) and its 0..100
clamp. Automatic hits and the guaranteed-critical draw never reach it. A
query reports which roll, the skill, source and target and the native chance;
the result is truncated once and clamped to 0..100, so 0 never succeeds and
100 always does. The RNG draw count never changes. `P4PC_HIT_TRACE=1` traces
it. `game_elements_hit` runs both real functions: an identity modifier changes
nothing.

### Passive grants

`p4pc/game_passive_grant.h` is a native table of (scope, key, skill) tuples,
each held by one or more owners (`p4.passive`, capability `passive.grant@1`,
[lua.md "Passive grants"](lua.md#passive-grants)). It is not a stage: no Luau
runs while the game checks a passive. A unit has a granted passive exactly
where the game's own checks would find it in its slots, so every check asks
the table after the unit's slots and the accessory's granted skill. A party
member sees its member ID, its current Persona (`datPersonaGetRecordId`,
0x00109360) and its three equipped items; an enemy its enemy ID (the kind its
UNIT0 row, and so its slot list, belongs to). The protagonist's roster checks
see the roster Persona and the protagonist's own member and equipment grants.
The tuples stay sorted, so queries are binary searches and the listing never
depends on the order of grants; the Persona is looked up only while a Persona
grant of that skill exists, the equipment only while an item grant does. With
nothing granted `p4_game_passive_live` is zero and each check is one load and
the original miss: no call, allocation, RNG or Luau.
The table is not saved and is not reset by a load or New Game: an owner's
tuples end when it revokes them or its script stops (`p4_game_passive_revoke_owner`
at VM teardown), never by another owner. A full table (256 tuples) refuses only
a new tuple. Once the semantic catalog is loaded, `p4.passive.grant` admits
only Personas, items and enemies that are rows of it.

Every reader of a unit's passives, audited against the decomp:

| Check | Native seam | Passives |
| --- | --- | --- |
| Learned-skill search (`datCalcHasRequestedSkill`, static in datCalc.c) | `overrides/static/datCalcHasRequestedSkill.c` | every datCalc.c caller and `datCalcHasSkill` 0x00232730: damage/heal boosts and Divine Grace (0x00235520), ailment and Hama/Mudo boosts (0x00238940), resist/null/repel/drain affinity passives (0x0023A6B0), max HP/SP (0x00231F80, 0x00232290), regeneration mask (0x00232D80), ailment recovery (0x001F6930), battle-start auto-buffs (0x001B2380), victory recovery (0x001B5970), the AI's own-skill test (0x001DAF40 and its inlined copies) |
| Fixed-slot search (`datCalcHasFixedSkill`, static) | `overrides/static/datCalcHasFixedSkill.c` | evasion, Firm Stance, counters (0x0023E6F0), critical skills (0x002411A0) |
| HP/SP cost halving, hand-inlined | `config/mirror_fixes.txt`, 0x0023D9B0 | 0x20B, 0x20A |
| Ailment boost, hand-inlined | `config/mirror_fixes.txt`, 0x00242360 | 0x21C |
| Endure and Light/Dark survival, hand-inlined | `overrides/00242990.c` | 0x231, 0x232, 0x211, 0x221, 0x212, 0x222, 0x210, 0x20F |
| AI physical-target test | `config/mirror_fixes.txt` 0x001DB360 and `game_elements_ai_predicate.inc` | Counter 0x1F6..0x1F8 (an enemy's datUnitInit flag 0x100 or a grant; a party member's slots or a grant) |
| Growth on stocked Personas | `overrides/0021e110.c` | 0x1FF, 0x200, 0x201, each once with the slots |
| Camp duplicate-skill choice (`cmpSkillListContains`) | `overrides/static/cmpSkillListContains.c`, 0x00138490 | 0x20A |
| Field encounter check `FldEvent_PartyHasSkill` | `config/mirror_fixes.txt`, 0x00172BA0 | 0x205: the leader's slots and every active member's accessory, so the protagonist's member, Persona and equipment grants and every active member's own and equipment grants (the retail disc's `INIT/ITEMTBL.BIN` row 0x270, the item the check names, has granted skill 0x205 at +0x24, the only row that does) |
| Added affinity effects and damage cover | `p4_game_elements_unit_sources` | any skill with effect or cover rows, after the slots, items and accessory, each once |
| Walk-script battle planner | `P4WalkHost.passive_granted` | an enemy kind's affinity passives |

These readers see the slots only, by design, and never a grant: the AI's
command scans (0x001DB160, 0x001DA930, 0x001DD570, 0x001DD920, 0x001DE000,
0x001E9350, 0x001E9770, 0x001EBC00, `game_knowledge_ai_body.inc`) take active
skills below the passive range; Analyze (0x00219790), skill menus, Persona
status, fusion inheritance, learning, Shuffle cards and the special-item pick
(0x0023DFF0) show or edit the eight-slot list. Navigator skills (0x10C..0x10F,
0x114, read with `datPersonaFindSkillSlot`) are active-range IDs and cannot be
granted. Each check reads the table when the game makes it: auto-buffs at
battle start, regeneration at each turn start, Counter's AI avoidance at each
decision. A revoked HP/SP passive lowers the maximum at once and current HP
stays until the game's next clamp, as with `p4.unit.on`. `game_passive_grant`
runs the table and the real static bodies and Growth code; `game_elements_hit`
the real 0x00242990 over every substitution, `game_knowledge_ai` the AI
predicate, `game_elements_affinity` sources through damage cover,
`mod_luau` the Luau binding with two owners, catalog validation and a full
table, and `mod_luau_save` save-bound grants dropped at a real load and a reset.

## Offensive effects

With `table-source.offense@1`, an `offensive-effect` row supplies the
Boost and/or Amp tiers for one offensive damage leaf to a unit carrying its
source. This example uses a
fixture-only skill ID, not an acquisition or UI declaration:

```xml
<offensive-effect source-kind="0" source-id="450" element="6"
    flags="1" key="example:light-boost"/>
```

`source-kind` 0 means a skill, 1 an equipped item; `element` is an existing
offensive leaf. `flags` is 1 for Boost, 2 for Amp, or 3 for both. Without an
active policy these mean 1.25 and 1.5 respectively. One owner claims each source's offensive coverage set, with one row per
source/leaf. The same source scan as damage cover includes the unit's skill
list, three equipped items and an accessory's granted skill. Duplicate tiers
are boolean: a declared Boost and a native Boost do not multiply twice.

The calculation joins these tiers to the existing early elemental factor in
`0x00235520`, including its accessory multiplier and default 1.875 cap, before the
remaining damage multipliers and variance. It applies only to formula HP
damage, not fixed amounts, SP, items or the scripted 500 result. An estimate
uses the same factor as a resolved calculation; neither row lookup nor the
resolved-calculation bracket asserts that an action was committed. Each
selected row's actual semantic use is authorized through its frozen source
ledger, including estimates. A pure registry preview does not record use.

`stage.damage@3` adds `p4.damage.offense_policy({boost = 1.2, amp = 1.3,
cap = 1.55})`. This start-up declaration returns a disabled subscription.
Enable acquires exclusive ownership of the three finite, positive coefficients;
if another policy is active, enable raises and leaves that policy unchanged.
Disable, remove, or VM teardown relinquishes ownership, restoring retail when
no other policy is active. There is no priority or silent last-writer-wins
composition. The policy applies to native and registered Boost/Amp tiers at
the early HP factor seam, including estimates; accessory multiplication precedes
the same cap, and duplicate Boost or Amp sources still count once. It does
not change status chances or the final-damage stage.

Bundled hard mode enables this policy only while a hard-mode run is armed,
including a restored or explicitly adopted run, and disables it during reset
and on a plain load. Both Hard and Harder use Boost 1.20, Amp 1.30 and cap
1.55. Changing settings alone does not alter an already saved run; with no
armed run the default coefficients remain retail. A competing active owner
is a failure, not an implicit fallback to retail.

Offensive coverage has its own ledger domain and does not register an affinity
effect, demand defensive side rows, or activate expanded affinity routing on
its own. Existing Hama/Mudo Boost status-chance behavior is unchanged; those
legacy passives do not thereby become HP-damage boosts. No new skill IDs,
acquisition or presentation are supplied by this record. The amount, affinity
and XML fixtures exercise synthetic facts only; real gameplay and the broader
API milestone remain unqualified.

## Damage cover

A `damage-cover` row lowers the HP damage one leaf delivers to a unit that
carries its source, without touching affinity. Hard mode's linings are the
first users: a Slash lining takes a share off Slash hits while a Slash
weakness stays WEAK and still knocks the wearer Down.

```xml
<damage-cover source-kind="1" source-id="200" element="19" channel="0"
    percent="15" family="overhaul:lining" key="overhaul:slash-lining-2"/>
```

- `source-kind` 0 is a skill and 1 an equipped item (`P4AffinityEffectSource`).
  Sources match exactly as the added affinity effects do, through
  `p4_game_elements_unit_sources`: the unit's eight `datCalcSkillList`
  entries (a party member's persona skills, an enemy's metadata list), then a
  party member's three equipped items, each accessory followed by its granted
  skill at u16 +0x24. Enemies wear nothing.
- `element` is a declared leaf with affinity; `channel` must be `p4:damage` (0);
  `percent` is 1..99.
- `family` is a key of the row's owner. Of one family, only the highest
  matching percent applies (two linings never stack); families multiply, so a
  unit with a 15% lining and a 50% guard skill takes 0.85 x 0.5 of the hit.
- A source covers a leaf at most once, and one owner claims a source's whole
  cover set, as with affinity effects. Rows join the catalog identity under
  the `P4_TABLE_SOURCE_COVER` domain, and like effect rows they request the
  native element route.

At table-source runtime publication each package that owns cover rows gets one
`P4_DAMAGE_MULTIPLY` modifier in the final damage stage, at priority
`P4_DAMAGE_COVER_PRIORITY` (1000, after hard mode's Technical 100 and boss
bursts 50) and that package's load order; session end unregisters them.
Without cover rows nothing registers and the stage keeps its retail
behavior. The modifier applies only to hits with a resolved leaf, to
estimates (AI target choice) exactly as to resolved hits, and records the
use of each row it applies; it draws no RNG and writes no game state. Since
the stage composes after the equation and its outcome, weakness, Down, One
More and every affinity query are unchanged. `game_tables` covers the row
rules, `game_table_xml` the record, and `game_elements_affinity` the source
matching, families and the stage composition, including a one-leaf ward made
from an equipped-item affinity effect.

## Physical Split mod

`mods/p4pc.physsplit` is the bundled Physical Split mod. It ships disabled; once
enabled, its restart toggle PHYSICAL SPLIT (default off) gates every boot section
(`when = split`): with the toggle off the mod contributes nothing and a save
does not require it. On, Slash, Strike and Pierce (leaves 19..21, family 0,
physical/offensive/affinity/visible) replace the Phys column of the
nine-column `[element-ui]` layout, with original white icons
(`tools/physsplit.py` draws them). The package carries authored data only;
`tools/physsplit.py generate` writes `data/split.xml` from `tools/physsplit/`,
and the `physsplit_package` test refuses a stale package.

Other mods may rely on a stable contract. The setting `split` is a toggle with
apply = restart and default off. The leaves are Slash 19, Strike 20 and
Pierce 21, with keys `p4pc.physsplit:slash|strike|pierce`, flags 15, family 0
and the damage channel 0. A dependent mod's own rows can name these leaves
(its weapon actions, `effect` wards and `damage-cover` linings) only in
sections that are gated on `split` being on. Physical Split itself declares no
`effect` or `damage-cover` rows, so those sources stay free for other owners.

- Actions: every Phys weapon by its owner (Yu, Yosuke, Teddie Slash; Chie,
  Kanji Strike; Yukiko, Naoto Pierce; the shared item 0 Strike), the 59
  fixed-Phys active skills by the design table (unlisted ones Strike), and
  every enemy's basic attack: its P3 FES counterpart's attack type where the
  shadow is the same, Strike otherwise. Attack and the follow-ups
  0x104..0x10A stay weapon-derived, so they take the actor's weapon or enemy
  attack type; non-Phys skills and weapons keep their element.
- Aliases: every retail enemy (initial id2 = id), Margaret's eight AI-scripted
  phases (id2 320..327, the only literal `AI_FUNCTION_0091` arguments in the
  enemy script), and every persona 1..255.
- Affinities: each alias has three rows. A persona row is the persona's own
  retail Phys (`retail-row`); an enemy row is its retail Phys unless its
  record in `tools/physsplit/enemies.json` authors a type. Those records
  follow the design's rules: null/repel/drain Phys stays locked on all three;
  an enemy that is the same Shadow as a P3 FES one (same name, FES's spelling,
  or FES's only sub-boss entry of it) takes that Shadow's three facts
  (PennyEve's FES guide), as does one whose every FES Shadow of the same
  family and Arcana (UNIT0 metadata +2, less one) agrees on them and on the
  basic attack. Facts are adapted to P4's balance gates (a normal-Phys
  enemy ends all normal or with exactly one Weak and one Resist, pairing a
  missing half Slash->Strike->Pierce; a resist-Phys enemy stays resistant on
  at least two types and gains at most one Null; nonstandard percentages stay).
  A mandatory boss met before the party has every type gains no Null:
  Avenger Knight (310), whom the Castle rescue party fights with Slash and
  Strike basic attacks only, keeps his retail Phys on all three types
  (`NO_PHYS_IMMUNITY` in `tools/physsplit.py`, 2026-11-10).
  Other enemies (122 of 294 in use) keep their retail Phys on all three and
  are marked pending review; blank slots are marked unused.
  `tools/physsplit.py report` prints the per-enemy matrix with the user's
  retail values and checks every gate.

Enemy knowledge is per type: Analyze learns only the types that actually hit,
and the save's `p4pc/enemy-knowledge` block keeps them.

`mod_physsplit_game` (witness lane, `P4PC_MOD_WITNESS=1`) plays the castle 4/20
training walk with `P4PC_BATTLE_TRACE=1`. With the mod enabled and the toggle
off, the save image and every traced command and hit equal a run without the
mod. With it on, Yu's weapon resolves Slash, Chie's Skewer Pierce, the save
requires the package and records only the types each enemy was hit by, the
next day's walk reloads it, and the battle Analyze panel (captured) shows
those types known and the others `?`. Test-only copies of the package that
make every enemy weak to one type show each type's hits WEAK, knocking a
survivor Down, and its attacker acting again or leading the All-Out Attack
(One More), while other types' hits are not WEAK. The Bathhouse 5/20 walk
runs the shipped package with Yukiko's Pierce fans and party AI support, and
a persona copy weak to Strike checks that only Strike-typed enemy attacks hit
WEAK. With the bundled QoL mod's FULL DESCRIPTIONS on, its generator reads the
registry (`p4.elements`) and the Camp skill help is captured. Retail P4 enemy
AI never queries a Physical weakness (its weak queries name elements 1..4, or
any element), so these runs check typed attacks and party AI, not a
physical weakness-seeking enemy script. These are mod-witness runs, never
retail references.

Those walks never inflict an ailment, so the per-turn ailment recovery check
with routing on (`btlActionBadUpdate` → 001F6930's no-skill status calc) is
exercised by a rendered `tools/battle_sim.py` replay of the Shadow Yukiko
fight with the package on and every party member attacking (a synthetic_lab
preparation from the story0426 save, run index 2, where Yu is Panicked at
update 4302). The battle reaches its native terminal (defeat, update 5985)
with no refusal, and the same replay with mods off is state-hash identical
across the builds with and without that calc's route (5981 updates).
## Side ailments

With `table-source.status@1`, a `status-ailment` row turns one of its owner's status leaves into a side
ailment: an ailment held per battle unit outside `DatUnit.bad`, so retail's
ailment bits are never widened. `table-source.status@2` adds `status-rider`
rows, the `transfer` attribute and the `P4_SIDE_BREAK_ON_HIT` flag;
`table-source.status@3` adds the `enemy-scale` attribute. Hard mode's
Freeze and Shock are the first users (leaves 24 and 25 are reserved for
`p4pc.hardmode`; their affinity-only rows use separate `[element-header]` art).

```xml
<element id="24" flags="4" family="24" order="24" key="p4pc.hardmode:freeze"
    display-name="Freeze" short-name="Frz"/>
<status-ailment element="24" flags="7" cure-mask="0x16" default-from="2"
    crit-bonus="100" key="p4pc.hardmode:freeze-ailment"/>
```

- `element` is a leaf above the retail 0..18 that the row's owner declares with
  `P4_ELEMENT_AFFINITY` and neither `OFFENSIVE` nor `PHYSICAL`. One row per leaf,
  at most `P4_SIDE_AILMENT_LIMIT` (16) rows.
- `flags` are `P4_SIDE_LOSE_TURN` (1: the unit loses its next own turn and
  recovers at that turn's end), `P4_SIDE_SINGLE_SLOT` (2: excludes and is
  excluded by every retail ailment, Dizzy included, the `bad & 0xFFFFF` test
  of `datCalcStatusEffectResult` 0x002397D0), `P4_SIDE_SMTV_DECAY` (4: each
  prior landing this battle lowers the next chance) and `P4_SIDE_BREAK_ON_HIT`
  (8: a later damaging hit clears it, as SMT V's Sleep and P3 Reload's Freeze;
  never in the packet that landed it).
- `cure-mask` (nonzero) is cleared by a retail mode-2 cure whose mask covers
  it: `(cure & cure-mask) == cure-mask`, so 0x16 clears with Patra (0x16) and
  Amrita/Salvation (0xFF) but not with a single-ailment cure.
- `crit-bonus` (0..100, default 0) is a percent added to the critical chance
  against an ailing unit after the native no-crit gates.
- `transfer` (0..100, default 0) is P5's Shock contact: a damaging hit of
  physical delivery (the Phys leaf or a Physical Split leaf, never the
  holder's own reflected hit) moves the ailment to the attacker with chance
  `transfer` × the attacker's susceptibility, when the attacker could take it
  (alive, unprotected, no side ailment, and for a single-slot leaf no retail
  one). 0 draws nothing.
- `enemy-scale` (1..100, default unscaled; 0 is refused) is the percent of
  every rider's base chance when the rolling hit's source is an enemy and its
  target a party member: it multiplies the same formula as the game's
  enemy-source scale. With it a base of 100 is rolled like any other (an enemy
  rider of 100 does not land unrolled), and enemy-on-enemy and party hits are
  unscaled. Hard mode halves its Freeze and Shock this way (`enemy-scale="50"`).
- `default-from` (optional) is a damage leaf (affinity and offensive). A unit
  with no row for (leaf, `p4:status`) is immune if all eight retail ailment
  leaves 8..15 are (null, repel, drain or the 0x20 no-effect flag); otherwise
  its susceptibility follows that leaf's effective damage affinity: weak 125%,
  resist 50%, null/repel/drain immune, otherwise 100%. An authored status row
  replaces the default, and added effect rows on the leaf apply to either.
  Pairs with a default are never required from every alias. Without
  `default-from`, an absent row is an error as for any other added pair.

Susceptibility is therefore read like any status leaf, through
`p4_game_elements_native_affinity` with a (leaf, `p4:status`) frame. The
battle state is the side-ailment service (`game_side_ailment.h`). Units are
guest `DatUnit` addresses:

- `p4_side_ailment_set(unit, leaf, reason)` lands an ailment (reason RIDER or
  MOD) and counts a landing; `p4_side_ailment_landings` returns that count.
  It refuses outside a battle, an unknown leaf, a dead unit, a unit that
  already holds a side ailment and, for a single-slot leaf, a unit holding a
  retail ailment; Down is allowed.
- `p4_side_ailment_clear(unit, reason)` clears it (CURE, MOD, RECOVERED, DEFEAT,
  BATTLE_END or RESET) and returns the cleared leaf.
- `p4_side_ailment_query` returns the held definition;
  `p4_side_ailment_turn_lost` reports the lost turn of a lose-turn ailment.
- `p4_side_ailment_mark_landed`, `p4_side_ailment_landed` and
  `p4_side_ailment_action_end` carry one action's landings to its later hits
  before the commit at 0x001ACB10.
- One observer receives LANDED, CLEARED and TURN_LOST with their reason
  (RIDER, MOD, TRANSFER; CURE, MOD, RECOVERED, DEFEAT, BATTLE_END, RESET,
  BROKEN, TRANSFER); set, clear and mark refuse during delivery.
- `p4_side_ailment_set_enabled` is the per-run gate. It is off by default and
  again at table-session end, and refused inside a battle. **A declaring
  package must enable it for each run that uses its side ailments** (hard
  mode does so when a run adopts Freeze/Shock); while it is off every reader
  is inert, `p4_side_ailment_active` is 0 and set/mark refuse NOT_READY, while
  the definitions stay visible.
- `p4_side_ailment_protect(unit, on)` is a lock-guard window for one unit in
  this battle: riders skip it before any draw and set/mark refuse it INVALID,
  like a dead unit. It is refused outside a battle or with the gate off and
  lifts at unit exit, battle begin/end and session end.

State begins with the battle (`btlInit`, 0x00192B90; every encounter wave of it,
so a next wave keeps the party's), ends for a unit when its action exits (0x001B05D0) and for all at
`btlRelease` (0x00192CD0) or table-session end (RESET). Nothing is saved.
Without rows nothing publishes, every call returns at once and no unit is read.
Rows join the catalog identity under `P4_TABLE_SOURCE_SIDE_AILMENT` and
request the native element route.

### Riders and battle readers

A `status-rider` row lets a landed hit of a skill inflict its owner's side
ailment (the Freeze and Shock of hard mode's Ice and Elec skills):

```xml
<status-rider skill="0x1A" element="24" rate="10" key="p4pc.hardmode:freeze-bufu"/>
```

`rate` (1..100) is the base chance; one row per (skill, leaf), at most
`P4_SIDE_RIDER_LIMIT` (256); the leaf must be a side ailment of the same
owner. Rows join the catalog identity under `P4_TABLE_SOURCE_SIDE_RIDER`.

The readers (`game_side_ailment_battle.c`, declared in
`game_side_ailment_native.h`) make a side ailment act in battle. Each returns
before reading a unit unless a battle runs with published rows, so without
them retail flow, RNG draws and results are unchanged.

| Site | Reader |
| --- | --- |
| `btlTargetResolveSelectedSkill` 0x001F14F0, after each hit's retail statuses | a landed hit (hit-check outcome 1, whatever its weak/critical class; reflected hits against the unit that takes them) rolls the skill's riders with the original 0x00238940 arithmetic, the rider's rate in place of the skill's +0x19 byte: rate × status percent × (100 + source Luck) / (100 + target Luck), × the game's enemy-source scale `fGpffff811c` (0.8), × the status's `enemy-scale` percent when an enemy hits a party member, truncated, then × status percent again, ×2 against buff 7, ×1.25 with Ailment Boost 0x21E, truncated and clamped to 0..99, rolled on `RpRandom() % 100`; a rate of 100 lands without a roll (unless `enemy-scale` applies). An occupied slot or Guard refuse every rider without a draw; a single-slot rider yields to a held retail ailment or one the same hit inflicts, and a null/repel/drain/immune status word refuses that rider, without a draw. `P4_SIDE_SMTV_DECAY` lowers the percent R to R − n·(2R + 100)/10 after n landings. A success marks the landing and, on an enemy with native routing on, teaches Analyze its (leaf, status) pair as an ailment skill's landing would (`p4_game_elements_native_learn_leaf`). Only the real resolution rolls, never 0x002397D0's evaluation callers (0x00239F50, 0x0023A1E0). |
| `datCalcStatusEffectResult` 0x002397D0 | a held or marked side ailment refuses a retail ailment (and a weapon's) like a held retail one |
| `datCalcRollSkillChance` 0x00242360 | no Dizzy while a side ailment holds or is marked |
| `btlTargetApplyHpSpChange` 0x001F3010 | for an ailment held before the packet and a damaging hit: breaks it (`P4_SIDE_BREAK_ON_HIT`, reason BROKEN) or rolls its `transfer` (reason TRANSFER); then commits the hit's marked landing, clears a held ailment whose cure mask the removed statuses cover, and clears it on defeat (also `btlTargetSetDefeatState` 0x001F2EB0) |
| `datCalcResolveSkillHit` 0x0023E6F0 | a unit with a lose-turn side ailment cannot evade, as retail Dizzy |
| `datCalcHitResult` 0x002411A0 | adds the held ailment's `crit-bonus` to the physical critical chance after every native gate, unless the hit is resisted |
| `btlSoundRollStatusCure` 0x001F6770, `btlActionBadUpdate` 0x001A31E0 | a lose-turn ailment sends the turn to the bad state, which spends it before Down recovery (a Down unit stays down and stands next turn, so one turn is lost, never two) and recovers the ailment at its end |
| `btlActionStartInit` 0x001A06D0 | each turn starts with no landing marked by an earlier action |
| `btlActionCanFollowUpAfterEnemiesDown` 0x0019FF60 | a unit that loses its turn to a side ailment neither starts nor joins an All-out Attack |
| Social Link helpers 0x001EF9C0, 0x001EFF50, 0x001EFD50 | a party member that loses its turn to a side ailment does not help, follow up or shield |
| AI queries 0x001D9B60 (MYBAD, FRBAD, ENBAD), 0x001D9CA0, 0x001DBBA0, 0x001E2550, 0x001E26C0, 0x001E9770, 0x001E9F20, 0x001EA050 | a status mask that asks for every retail ailment (all of 0xFF) also matches a side ailment; a mask for particular retail ailments does not |
| AI picks 0x001E7950, 0x001E7C20 | a side-ailing unit is not unafflicted |

`game_tables` covers the row rules, `game_table_xml` the records,
`game_elements_affinity` the default, `game_side_ailment` the state,
refusals, events and lifecycle, and `game_side_ailment_battle` the readers.

## Qualification and availability

The native registry has isolated synthetic tests for retail packed decode,
owned new leaves/channels, independent enemy/profile/damage/KO rows, conflict
refusal and failed-freeze nonpublication. These are **in-memory metadata tests**,
not battle, menu, save/load or platform parity claims. The inventory parser has
synthetic direct-access/narrowing/storage/asm/mirror-identity tests.

No Luau expansion capability is advertised by this phase; expanded elements are
reached through table sources. Their native battle, AI, knowledge, UI, resource
and sidecar participants are qualified in real play by `mod_physsplit_game`
(Physical Split: every leg on Linux x64, the castle legs also on Windows x64)
and `mod_element_game`; other platforms and viewports have not been run with
them. Physical Split and damaging
Light/Dark share the one action/effect-component adapter
(`p4_game_elements_runtime_affinity_effective`): a binding chooses the
element and channel, a side row keyed by that pair supplies the base, and the
retail resolvers at 0x0023a6b0 and 0x00242800 answer only unmodified natural
pairs, while the hit route at 0x0023e6f0 asks the owning frame. Neither mod
replaces those three functions.
