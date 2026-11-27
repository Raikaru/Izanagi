# RE-3: Golden item categories and actions (R7a)

Revision: 2026-11-12. Companion: [P10 encounter audit](item-categories-p10.md).
Icon/atlas semantics remain in [item-icon-flags.md](item-icon-flags.md).
This is a static specification, not a runtime qualification or permission to
remove R7a. In particular, unknown UI admission rules below need BBV.

## Evidence and interpretation

Labels apply to each paragraph/table entry carrying the cited evidence key:
**OBSERVED-S** = Steam build 10836033 data or ordinary `.shared` code;
**OBSERVED-N** = the owner's Switch patch 131072 NSO; **OBSERVED-P** = PS2
source; **INFERRED** = explicitly stated extrapolation, not observed play.
A Switch observation is not automatically a proof about protected Steam code.
No game was run, no `.arch` code was decoded, and no payload or instruction
listing is included here. All addresses below are RVAs for Steam and full
runtime addresses for Switch (base `0x7100000000`).

| Key | Location and observation |
| --- | --- |
| S1 | English Steam `data_e.cpk::init_free.bin::init/itemtbl.bin`, SHA-256 `1bf1b589cb754d64099774ab158bb3d039806e8e9b41bc645d3ec97b23bcf533`; section 0: 2,560 × 68, section 1: 2,560 × 24. Kind is u32 at +0; flags are u32 at +4; granted skill is u32 at +0x24. Census includes reserved rows. |
| S2 | `data_e.cpk::init_free.bin::init/cmpTable.bin`, named CTD members below. CTD payload begins after its 16-byte header; counts/strides are measured, not inferred from filenames. `cmpItemSortTable.ctd` SHA-256 `d4aa67f3ace7897991dd7c42173967a289f2ab7736d6b8d9db79fe98878dcd3f`; `cmpEquipSortTable.ctd` `5afe83d24cb530bdcb30b13149c9f2554087ce57778e9a24e42f7b541d67ee43`; `cmpItemType.ctd` `68bd3ab25f08776e4e84a938d7371dba8fd6162b9939dfe917bfdc645d0bc868`. |
| S3 | Ordinary Steam `.shared` `0x19EA90` reads the +4 word of a signed16-indexed 68-byte row. `0x19F560` reads the kind word, checks it against 9, then dispatches through the ten-entry RVA table at `0x19F5F4`; its return arms `0x19F59E`–`0x19F5F3` give the same eight menu categories as N2. The shown addresses are not protected targets. |
| N0 | `main.nso`, SHA-256 `854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`, build ID `66b2b6d53549a6364a43f69e282e3882f4d7028e000000000000000000000000`. Source package is the owner's Switch update; it corroborates behavior, not the Steam content source. |
| N1 | `0x7100111DA0` reads kind +0; `0x7100111EA0` reads flags +4; `0x7100112000` reads granted skill +0x24. Row base pointer `0x71009F8A88`, stride 68, signed16 item ID. |
| N2 | `0x7100112460`, lookup at `0x7100631640`: kind-to-menu-category mapping. `0x71001122F0`, counts at `0x710062AEC8`; `0x71001121B0`: category-index-to-item-ID mapping. These functions assert on unsupported categories/kinds, rather than treating a kind as a bit number. |
| N3 | `0x71000F73B0`/`0x71000F73E0`/`0x71000F7420`/`0x71000F7490` read the sort CTD count, exact flag word, named range table and its count. `0x71000EE160` builds the camp item list in that order, requiring flag **equality** and nonzero inventory count. `0x71000F2720` jumps between adjacent runs of equal flag words. |
| N4 | `0x71000F25E0` chooses the ordinary-use or skill-card menu; `0x71000F1B90` drives item/recipient/persona selection and awaits the card task; `0x71000F2950` performs ordinary use or the successful card consumption. |
| N5 | `0x71002AC840` constructs `shdPersonaSkillCardSeq`; callback `0x71002AC960`, 12 relative dispatch entries at `0x710063A0AC`. Its actual branch bodies are at `0x71002ACA00`–`0x71002ACE6C`. The decompiler stops at the indirect jump: the case targets and pivotal instructions were inspected separately. |
| N6 | `0x710007C350` counts nonzero persona skill slots; `0x710007F8E0` inserts into the first zero of eight u16 slots at persona +0x0C; `0x710007FFD0` finds the first slot containing a specified old skill and replaces it. The replacement UI is `0x71002AB260` / callback `0x71002AB3F0`. |
| N7 | `0x710019D7E0` builds the six generic shop sell selectors; `0x710019DAD0` filters owned, permitted items, computes the sale value and emits entries; `0x710019DD50` compares flag word then item ID. `0x710019D270` applies availability-date checks to kinds 3,4,6,8, not to arbitrary flag bits. |
| N8 | `0x7100111C90` reads count; `0x7100111CB0` clamps the byte count to 99 and updates the possession script bit only for kind 4. `0x7100171C60`/`0x7100171CB0` read/set the separate registered-card bitmap. |
| N9 | `0x71002CC910` selects an item description/help class from flag bits; this is distinct from both use and icon selection. |
| P1 | PS2 `src/g_data/g_data.c`: `0x00106850` kind, `0x00106880` flags, `0x001069A0` granted skill, `0x00106600`/`0x00106620` count. PS2 `DATA/INIT/ITEMTBL.BIN`: 1,536 × 68; the measured kind domain is 0–5, each 256 rows. |

The old private materialization cache was used for **discovery**, not a
whole-image closure claim. Named NSO functions, relative dispatch tables and
relevant instructions were read directly; twelve pivotal ARM64 ranges were
independently decoded by LLVM. No claim here that all indirect callers in
both binaries have been enumerated.

## Kind is not a flag mask

**OBSERVED-S, S1:** every one of the 2,560 rows has kind 0–9. Each kind has
exactly 256 rows. There are **zero kinds above 31, zero rows with kind above
31, and zero rows with kind 10–31**. The roadmap's `kind > 31` refusal does
not describe an item occurring in this pinned table. Do not invent kinds
32+, confuse the +4 flag word with +0 kind, or regard accepting kinds ≤31
as sufficient category/action support.

**OBSERVED-S/N/P, S1/S3/N1/N2/P1:**

| Kind | Golden ID interval | Menu category | Meaning / action family | PS2 counterpart |
| ---: | --- | ---: | --- | --- |
| 0 | 0x000–0x0FF | 0 | Weapons; equipment, not a camp consumable | kind 0 |
| 1 | 0x100–0x1FF | 1 | Armor; equipment | kind 1 |
| 2 | 0x200–0x2FF | 2 | Accessories; equipment | kind 2 |
| 3 | 0x300–0x3FF | 4 | Expendables; includes the new insect/fish subcategories | kind 3; no one-to-one flag translation |
| 4 | 0x400–0x4FF | 5 | Valuables, including books distinguished by flags | kind 4; retains possession-bit side effect |
| 5 | 0x500–0x5FF | 6 | Materials; facility sale/exchange, not normal camp casting | kind 5 |
| 6 | 0x600–0x6FF | 7 | Skill cards; persona teaching, not equipped-item granted-skill membership | none |
| 7 | 0x700–0x7FF | 3 | Clothes/costumes; separate equipment category | none as an ITEM kind/category |
| 8 | 0x800–0x8FF | 4 | Expanded expendables, vegetables, food, gems and valuables by flags | existing expendable behavior where a granted skill permits it; otherwise facility/script action, not a new PS2 kind |
| 9 | 0x900–0x9FF | 0 | Additional weapons; same menu category as kind 0 | kind 0 conceptually, not an ID alias |

All four new kinds are necessary, but there are only **two new top-level
categories**, Clothes and Skill Cards. Expanded weapons and expanded
expendables merge into existing categories. A reserved row is still a row;
these counts are not counts of obtainable items. (S1/N2.)

**OBSERVED-N, N8/P1:** inventory count and item type are separate state.
Golden kind-4 count writes set/clear script flag `0x200 + low-byte(itemID)`;
PS2's corresponding writer uses `itemID - 0x200`. Those expressions agree
for kind-4's interval. Neither side grants the referenced skill merely
because an item was acquired. Kind 6 acquisition does not teach a persona.

## Category tabs, bounds and ordering

### Eight logical categories

**OBSERVED-S/N, S2/N2:** `cmpItemType.ctd` has eight 64-byte label records,
in this order: Weapons, Armor, Accessories, Clothes, Expendables, Valuables,
Materials, Skill Cards. These are category indices, not kinds.

| Category index | Full row-domain capacity | Category index to item ID |
| ---: | ---: | --- |
| 0 | 512 | index 0–255 unchanged; index 256–511 plus 0x800 |
| 1 | 256 | index + 0x100 |
| 2 | 256 | index + 0x200 |
| 3 | 256 | index + 0x700 |
| 4 | 512 | index 0–255 plus 0x300; index 256–511 plus 0x700 |
| 5 | 256 | index + 0x400 |
| 6 | 256 | index + 0x500 |
| 7 | 256 | index + 0x600 |

**OBSERVED-N, N2:** the eight capacities sum to 2,560. They bound category
iteration before possession filtering; they are not visible shop stock
counts. In particular, category 0 index 256 is item 0x900, **not** 0x100;
category 4 index 256 is 0x800. Preserve global IDs in inventory and saves.
`0x7100112330` is a *different*, kind-dependent auxiliary index helper:
it subtracts 0x700 for both kinds 7 and 8, and 0x800 for kind 9. Do not
silently replace that helper with a generic low-byte operation. Card
registration uses it for kind 6 only, where the subtraction is 0x600.

### Twelve camp item sort groups and four equipment sort groups

**OBSERVED-S/N, S2/N3:** sort rows are `{u32 exact_flags, char stem[20]}`.
The stored stem is the **complete extensionless member name**, including
the case-sensitive `cmpTypeID_` prefix. Resolve **`stem + ".ctd"`**,
not `"cmpTypeID_" + stem + ".ctd"`. Each resolved CTD contains an ordered
list of inclusive `{u16 first, u16 last}` ranges. A range is not a packed
list of individual IDs and is not itself an availability declaration.
The suffix column below abbreviates the stem for readability; it is not
the literal string stored in the sort row.

#### RE-3 addendum: exact stem and range grammar

**OBSERVED-S/N, S2/N3; correction to the original wording:** the ITEM row
stores `cmpTypeID_ITEM`, and the requested sibling member is therefore
`cmpTypeID_ITEM.ctd`. The same rule applies to all twelve item-sort and all
four equipment-sort rows. `0x71000F7420`/`0x71000F7490` obtain the 20-byte
field from the sort record and format `%s.ctd` directly, then resolve that
member in the loaded `cmpTable.bin` resource. They do not prepend a prefix,
uppercase the field, or interpret the table's abbreviated suffix column.

**OBSERVED-S, fresh pinned-install census:** all 16 stored stems are ASCII,
NUL-terminated within the 20-byte field, length 13–19 excluding NUL, with
zero padding after the first NUL. Each consists of the literal mixed-case
prefix `cmpTypeID_` followed by the uppercase/underscore suffix in the
tables here. An uppercase-only check on the **entire** stored stem would
reject all 16 valid rows. No field contains `.ctd`, a path separator or a
second prefix. All 16 exact `stem + ".ctd"` member lookups exist.

**OBSERVED-S/N, S2/N3:** there are 319 item-sort range records and 15
equipment-sort range records: **334 total** across 16 referenced CTDs.
All endpoints are in 0–2559, every first endpoint is ≤ its last endpoint,
and none is 0xFFFF. These lists are **count-delimited**, not sentinel-
terminated. The weapon range list includes endpoint **0**; zero is an
ordinary inclusive endpoint here, not an end marker or invalid domain.
No descending, out-of-domain or 0xFFFF exception is needed for this build.
Do not apply formation-ID-zero semantics to item-range endpoints.

**OBSERVED-S:** no individual range table visits the same ID twice, but
different groups deliberately cover overlapping ID domains. That is valid:
the exact flag-word filter selects each group's members. Range membership
alone does not imply the item has that group's kind, is obtainable, or is
owned. Preserve range-record order; do not sort the records, reject overlap
between different groups, require contiguous coverage, or require every
range endpoint's item row to have the group's flags.

Synthetic grammar fixtures (proposed, not executed in this lane): a
NUL-padded stored `cmpTypeID_TEST` requests `cmpTypeID_TEST.ctd` exactly
once; it must not request `cmpTypeID_cmpTypeID_TEST.ctd`. An equipment
range `[0,2]` visits 0,1,2 and does not terminate at zero. A count of two
with ranges `[12,12]`, `[9,10]` visits 12,9,10 without a sentinel.

| Item-list order | Exact flags | Range-table suffix | Range records |
| ---: | --- | --- | ---: |
| 0 | 0x00010000 | ITEM | 260 |
| 1 | 0x00800000 | VEGETABLE | 15 |
| 2 | 0x00400000 | FISH | 12 |
| 3 | 0x00200000 | BEETLE | 1 |
| 4 | 0x04000000 | BUTTERFLY | 1 |
| 5 | 0x08000000 | LEAF | 1 |
| 6 | 0x01000000 | FOOD | 2 |
| 7 | 0x00080000 | SKL_CARD | 1 |
| 8 | 0x10000000 | GEM | 2 |
| 9 | 0x00020000 | EV_ITEM | 2 |
| 10 | 0x02000000 | BOOK | 21 |
| 11 | 0x00040000 | MATERIAL | 1 |

**OBSERVED-S/N, S2/N3:** equipment sort order is WEAPON (mask 0x3FFF,
2 range records), ARMOR (0x4000, 1), ACC (0x8000, 1), DRESS
(0x100000, 11). These equipment masks are not the camp item-list equality
predicate. Clothes are a separate choice, not armor with an invented stat
bonus. Detailed equipment rendering remains R7b/M8c, outside this spec.

**OBSERVED-N, N3:** camp iteration walks sort-group order, range-record
order, then increasing ID *within each range*, accepting only an exact
flag-word match and count >0. Thus it is not globally ascending ID order.
For example, the measured FISH and BOOK tables are deliberately reordered.
Preserve these tables in the per-user cache; do not bake their game-data
range payloads into code or flatten them by sorting all IDs. Synthetic
combined flags such as `0x00810000` fail both exact group tests even though
the icon selector can still select an icon.

### Shops

**OBSERVED-N, N7:** generic sell-list selector order has **six** cases:
0 Weapons (logical category 0), 1 Armor (1), 2 Accessories (2),
3 Expendables (filtered sort groups), 4 Materials (6), 5 Clothes (3).
Do not add Skill Cards as a seventh generic sale category. The six cases
are available selector IDs, not a proof that every shop displays six tabs.

**OBSERVED-N, N7:** case 3 preserves the twelve-sort-table order while
admitting only these eight groups: ITEM, VEGETABLE, FISH, BEETLE,
BUTTERFLY, LEAF, FOOD, GEM. It omits SKL_CARD, EV_ITEM, BOOK, MATERIAL.
Case 0 compares flag word then item ID. Other category walks retain their
category enumeration order. Case 4 inserts a leading aggregate sell row
when materials exist; that row is not a newly owned item or a seventh tab.
`0x710019DAD0` requires count >0 and permission helper `0x71001732A0`
to return zero. Sale value is truncated `factor * item.price`, with minimum
1. The factor/permission implementation is not specified by an icon bit.

**OBSERVED-N, N8:** the separate registered-card facility enumerates logical
category 7: `0x71001A2610` builds donation candidates (owned, nonzero skill,
not already registered, permitted, matching the requested skill element),
while `0x71001A2780` builds registered purchase candidates, bounded by
money and the 99-count limit. `0x7100171CB0` sets registration only if the
item has a nonzero granted skill. This bitmap is **not** the persona's
learned-skill list, and teaching in camp is not evidence of registration.
Visible shop-specific labels/counts and registration persistence need the
BBV cards below; do not equate a generic selector count with a shop layout.

## Gameplay interpretation of expanded +4 bits

**OBSERVED-S/N, S1/S2/N3/N4/N7/N9:** all ten bits 19–28 occur; bits 29–31
occur in **zero** rows. Every expanded-bit-bearing row in this build has
one of the exact flag words below. These are classification/dispatch tags,
not ten new independent battle buffs. The +0x30 category/equip mask and
+0x24 granted skill are distinct fields.

| Bit / word | Rows (reserved included) | Gameplay classification and use routing | PS2 counterpart |
| --- | ---: | --- | --- |
| 19 / 0x00080000 | 256 | Skill cards; camp persona-teach branch via logical category 7; excluded from generic sale; separate registration facility | no card action/category |
| 20 / 0x00100000 | 256 | Clothes; logical category 3, fourth equipment sort group, generic sale selector 5 | no separate costume ITEM category |
| 21 / 0x00200000 | 32 | BEETLE item-list group; sale-eligible expendable group; ordinary camp use still depends on granted skill | rows retain kind 3; old broad expendable flag 0x10000 where same-ID rows exist |
| 22 / 0x00400000 | 12 | FISH group; explicit custom order, generic sale admitted; camp use is granted-skill-driven | same broad kind 3/expendable class, not an icon-mode conversion |
| 23 / 0x00800000 | 26 | VEGETABLE group in kind 8; sale admitted; a field-usable granted skill takes the ordinary effect route | no kind-8 bank; ordinary effect mechanism exists |
| 24 / 0x01000000 | 21 | FOOD group in kind 8; same camp-use gate and sale inclusion | no kind-8 bank; ordinary effect mechanism exists |
| 25 / 0x02000000 | 55 | BOOK group, kind 4; separate list grouping, omitted from generic sale; kind-4 possession-bit update remains | same-ID PS2 flag 0x20000, 55 rows; not an automatic read-book action |
| 26 / 0x04000000 | 4 | BUTTERFLY group in kind 3; separate list and sale inclusion; no automatic camp spell from the flag | old broad expendable flag 0x10000 in the four same-ID rows |
| 27 / 0x08000000 | 4 | LEAF group in kind 3; separate list and sale inclusion; no automatic camp spell from the flag | old broad expendable flag 0x10000 in the four same-ID rows |
| 28 / 0x10000000 | 30 | GEM group in kind 8; listed separately and sale-admitted; no automatic teach/equip behavior | no kind-8 bank; facility exchange is a separate action |

**OBSERVED-S, S1 and same-ID PS2 comparison:** BEETLE's 32 and FISH's 12
rows also replace PS2 0x10000 exactly. This comparison identifies old
broad classifications, not a rule to rewrite arbitrary flags. New kind-8
rows include 159 rows with ordinary 0x10000 and 20 with 0x20000 besides
the three expanded flags above; kind 8 is not synonymous with vegetables.

**OBSERVED-N, N9:** expanded bits also select help classes: card 0x1C,
clothes 0x1E, beetle 0x2E, fish 0x1D, vegetable 0x2D, food 0x2C,
book 0x1F, butterfly 7, leaf 8, gem 10. These indices are neither skill
IDs nor icon indices. N9 tests bits in priority order, whereas N3/N7 use
exact equality. This distinction matters for synthetic combined flags.
The cited paths establish real menu/sale/help consequences beyond icons;
they do **not** prove absence of additional indirect/script readers.

## Use actions (six explicitly distinguished routes)

**OBSERVED-S/N/P, S1/N2–N8/P1:** categories and actions are not bijective.
The six routes below cover the observed category-level dispatch; contextual
facility/script effects are intentionally distinguished from camp use.

| Route | Applies to | Rule and evidence |
| --- | --- | --- |
| A1 equipment | categories 0,1,2,3 | Weapons/armor/accessories/clothes are equipment choices, not immediate use of +0x24 as a spell. New kinds 7/9 must reach categories 3/0. N2, S2 equipment-sort tables. |
| A2 ordinary field skill | expendables, including the vegetable/fish/insect/food subgroups when their skill permits | N4 reads granted skill then `0x71000725D0`: active ID <472, skill flag bit 1 clear, SKILL1 +4 bit 0 set. Target kind 0 opens recipient selection; target kinds 1/2 open the multi-recipient route. N4/N1; PS2 has granted-skill field P1. |
| A3 escape/return special case | ordinary item with granted skill 0xF6 | `0x71000F2950` uses `0x71000F6D80` to reject unsupported field state without decrement. Success decrements once; a later `0x71000F6D90` flag-0xC5C check can refund the decrement on its message path. Item-ID-specific side effects remain in N4; do not infer them from category bits. |
| A4 teach | category 7 / kind 6 | Nonzero skill opens persona selection, then N5 and N6; consume only after the teaching task reports success. Full flow below. |
| A5 facility/script use | books, valuables, materials, gem exchange and insect/fishing/food contextual interactions | List/sale membership alone does not execute reading, fishing, feeding, gardening or crafting. N3/N4/N7 specify camp listing and generic sales; contextual effects and their consumption schedules are not established here (U3). |
| A6 unavailable/cancel | any route failing its gate or cancelled before success | N4 ordinary target eligibility failure returns without count change; multi-target use consumes once if at least one recipient succeeds, not once per recipient. Blank/zero-skill card does not enter teach. Card-task cancellation reports false to its caller, which does not call the decrement routine. |

### Skill-card teaching transaction

**OBSERVED-S/N, S1/N4–N6:** of 256 kind-6 rows, **253** have a nonzero
granted skill, all different, in the Golden skill domain (minimum 1,
maximum 592); three have zero. The item ID is not the granted skill ID.
Do not shift a Golden passive skill ID again during teaching.

**OBSERVED-N, N4/N5/N6:**

1. The selected owned list entry carries an item ID/count. N4 admits a
   skill-card item if its granted skill's low halfword is nonzero, bypassing
   the ordinary field-cast gate. The constructor passes this granted skill
   into `shdPersonaSkillCardSeq`, not the item ID or an equipment slot.
2. The item menu lets the player select a protagonist roster persona and
   view its skill pane (`0x71000F1B90`, states 11–16). The card callback
   asserts skill >0 and <624. Missing persona pointers are assertion cases,
   not a fallback party member or a successful no-op.
3. N5 state 2 counts the persona's eight skill slots. With fewer than eight,
   N6 inserts the skill in the first zero slot and updates presentation.
   With eight, it constructs `shdPersonaSkillSelectSeq` with the new skill
   and waits for its result. N5 state 4 forwards that task's success/result;
   rejecting/cancelling the replacement is not immediate consumption.
4. In the replacement callback `0x71002AB3F0`, the selected old skill and
   proposed new skill are passed to `0x710007FFD0`; that helper replaces
   the **first matching old-skill slot**, returns its slot index, or -1 if
   absent. It does not sort the persistent skill list. The separate
   presentation helper is updated when a slot was found.
5. Completion getter `0x71002ACEB0` tests task state 10; success getter
   `0x71002ACED0` tests result +0x10 !=0. Only completed **and successful**
   tasks cause N4 to call `0x71000F2950`. Its card branch bypasses ordinary
   target-effect application and decrements inventory exactly once via
   `0x7100111C90`/`0x7100111CB0`. A last-copy decrement rebuilds the item
   list and adjusts cursor/scroll bounds. Earlier cancel paths do not.
6. The low-level insertion helper does not reject a duplicate skill. The
   complete upstream duplicate/locked-skill admission rule is **unknown**
   (U1); do not invent a duplicate check, promise duplicate acceptance, or
   describe an unreachable helper call as retail behavior. Likewise,
   replacement failure UI wording and which slots cannot be selected need
   U1. Structural invalid-ID/missing-persona asserts are not ordinary
   player-facing failure messages.

There is no PS2 skill-card action to alias. Reuse of the eight-slot persona
storage is justified by N6, not evidence that PS2's item-use UI already
implements this transaction. Native Golden presentation/assets remain M8/M9;
script/facility feature integration remains M11 where explicitly unresolved.

## Unknowns and proposed blind BBV cards

Every card uses the owner's unmodified Steam build, own saves, normal input
and local capture only (roadmap D2/D13). No memory editing, injection or
game execution by the spec agent. Freeze a blind transcript before comparing
with this spec. Deterministic cases should repeat three times; stochastic
contextual behavior needs the roadmap's preregistered N≥30 test.

| ID | Unknown / scope limit | Proposed BBV and invariant |
| --- | --- | --- |
| U1 | Duplicate skill, locked replacement slots, and exact player-facing refusal/cancel behavior on Steam; N5/N6 establish mutation/transaction but not every UI eligibility writer | Capture cards for (a) persona with a free slot, (b) eight slots then accept a different old skill, (c) cancel, (d) already knows skill with a free slot, (e) already knows skill with eight slots, (f) locked/special skill where obtainable. Record item count, selected persona, all eight before/after skill IDs from visible names, dialogs and chosen slot. Successful new teaching changes only the selected persona and consumes one; cancellation must not. Pre-register duplicate behavior as an open question, not a guessed expected rejection. |
| U2 | Per-facility visible shop tab count/order, unavailable-tab hiding, and permission/factor helpers behind N7 | Owner visits each shop on a fixed date with one of each obtainable class. Record visible tabs left-to-right and sell inclusion, including costume/card/book/material distinctions; compare against the six selector IDs without assuming six visible tabs. Record sale quote and stock bounds; preserve counts after cancelled sale. |
| U3 | Contextual non-camp actions/consumption: beetle, butterfly, leaf, fish, vegetable, food, gem, book, quest valuables and material crafting/exchange | For each class, record the actual facility/field action, eligibility prerequisite, item count before/after accept/cancel/failure, skill/effect if visible, and time advance. No category-wide rule is inferred from an icon. Use fixed-save action/cancel pairs; distinguish ordinary camp heal from contextual feeding/trading. These feature rules cannot be marked implemented from this document alone. |
| U4 | Steam equivalence of NSO teaching, ordering and registration state, including persistence | Capture append/overwrite/cancel, separate card donation and repurchase, then save/load. Distinguish persona learned skill, owned card count and registered stock. Teaching alone must not be reported as registration without evidence. Compare ordered lists with unsorted-ID FISH/BOOK examples from locally owned stock. |
| U5 | Additional indirect item-flag consumers and garment-specific gameplay effects; absence has not been proved | Compare clothes-only changes at fixed party/equipment/skill state; record displayed stats, skills and action availability. Exercise all ten expanded flag classes in camp/help/shop/context. A difference opens a new semantic rule, not a guessed buff from the flag. A static whole-consumer closure remains preferable to claiming absence from BBV. |

P10 has one separate unresolved retry/liveness question and one unexercised
runtime receipt/check in the companion; neither is silently folded into
an item action. Its ordinary Steam total formula is statically corroborated.

## Synthetic acceptance fixtures for the implementer

These are proposed fixtures, **not tests executed in this spec-writing lane**.

- Category 0 indices 255/256/511 map to 0x0FF/0x900/0x9FF; index 512
  refuses. Category 4 indices 255/256/511 map to 0x3FF/0x800/0x8FF.
  The eight category walks visit all 2,560 synthetic IDs exactly once.
- Two ordered synthetic ranges `[12,12]`, `[9,10]`, with matching flag and
  counts, emit 12,9,10. A combined flag does not pass exact equality.
- Teach synthetic skill 601 into a persona with seven occupied slots:
  fill the eighth, leave other personas unchanged, decrement card once.
  Full slots: replacement selects an old skill; cancel leaves both stores
  unchanged. Skill zero cannot start teaching; skill 624 hits the task's
  structural invalid-ID boundary. Duplicate admission stays U1, not a
  source-text or guessed-behavior test.
- Multi-target synthetic ordinary item: all targets fail → no decrement;
  one or several succeed → one decrement. Equipment category selection
  alone never executes its granted skill as a camp consumable.
- The ten expanded flags each resolve the list/help/sale route above;
  synthetic bits 29–31 have no declared behavior. Preserve flag words rather
  than narrowing them, translating sprite indices, or permitting new kinds
  merely because they fit a five-bit field.

## Coverage receipt and admission boundary

**OBSERVED-S/N/P:** 2,560/2,560 Golden rows classified; 10/10 observed
kinds (4 Golden-only); 0 kinds >31; 8/8 logical categories (2 new), 12/12
camp sort groups, 4/4 equipment sort groups, 6/6 generic sell selectors;
6 explicit category-level action routes; 10/10 expanded flag bits;
256/256 skill-card rows, 253 nonzero skill references within 0–623.
P10: 68/68 groups, including all five positive-rate/empty-weight groups,
covered by the separate static flag matrix.

**INFERRED implementation boundary:** this defines data/category routing and
observed transactions, not a completed M7d adapter. U1–U5 and P10's E2
retry/liveness question remain explicit unknowns; E1 is an unexercised
runtime receipt, not an unresolved Steam-total formula. Do not lift R7a
from row counts or P10 from a synthetic all-zero fallback alone. No code,
boot registry, save format or test threshold is changed by this revision.

Import note: re-imported by the Golden M7d step from the committed RE-3 specification; the M6c intake step had left it out of the specification tree.
