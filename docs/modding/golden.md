# Golden mechanics (`p4pc.golden`)

Persona 4 Golden's battle-side mechanics on the PS2 port, derived at run time
from the player's own Golden install. Nothing from Golden is shipped, cached or
written: the port reads the player's copy, checks it, translates the battle
tables to the PS2 layouts and hands the differing rows to the table service.
No Golden story, Hollow Forest or winter content is involved.

The package is installed but, like every mod, not enabled until the player
enables it in MODS. Each group is a restart-applied setting on its MODS page,
on by default once the mod is enabled. Phase 1 covers the groups below; Rise's
support, dual follow-ups, Golden Shuffle Time, Golden enemy AI, third
awakenings and the new Jester/Aeon Personas come later.

**Status.** Every phase-1 table group, Golden's random encounters, the Social
Link perk order and the Social Link Persona skills are selectable. Golden moved
several party skills (Yosuke's Dekaja and Diarama, Chie's Bufula and Ice
Boost, Teddie's Samarecarm, Naoto's Mind Charge, Rise's Full Analysis, Third
Eye and Healing Wave) from the learnsets to Social Link ranks, so the producer
applies the Party group only while `rank_skills` is on as well. Bike rides
follow.

## Package and producer

`mods/p4pc.golden/mod.ini` declares the table settings `numbers`, `enemies`,
`party`, `exp_curves` and `encounters`, the script settings `perk_order` and
`rank_skills` (`main.luau`), and one `[table-derived golden]` section with
producer `p4pc:golden`, revision 1, over the inputs item, skill, unit, msg,
persona and encount ([derived table sources](tables.md#derived-table-sources)).
Core boot registers the producer (`include/p4pc/golden_producer.h`) before
package admission; it runs once at table seal when the package is enabled:

1. It reads the admitted group settings with `p4_mod_setting_boot`, drops
   `party` unless `rank_skills` is on and `encounters` unless `enemies` is on.
2. It finds the install from `P4PC_GOLDEN`, else `golden.install` in
   `p4pc.ini` ([settings](../platform/settings.md)), else the Steam libraries.
   No install refuses the package with a reason naming the setting.
3. It loads and checks the tables, reports each table's SHA-256 as a
   fingerprint, builds the selected groups over the base spans and pushes the
   rows. PERSONA's party block and EXP curves and the ENCOUNT blocks are
   pushed as whole spans: the base span with the Golden rows laid over it,
   extended when formations follow the base rows.

The catalog identity, and so every save made with the package, binds to those
fingerprints and the composed inputs.

## Supported source

The Steam release (app 1113000), build 10836033. The reader opens only
`data_e.cpk` and takes the English battle tables from its `init_free.bin`:
`battle/SKILL.TBL`, `UNIT.TBL`, `PERSONA.TBL`, `MSG.TBL`, `ENCOUNT.TBL` and
`init/itemtbl.bin`. Each must match that build's SHA-256 (the digests, not the
contents, are in `src/platform/golden/golden_source.c`); any other build is
refused until its tables are measured. `p4_golden_locate` takes an explicit
folder, else searches every standard Steam `libraryfolders.vdf`.

Formats (`include/p4pc/golden_source.h`): CRI CPK with @UTF tables (masked
packets are unmasked), CRILAYLA compression, Golden's sequential `.bin`
container (252-byte names, 64-byte alignment) and the packed-bank TBL spans the
PS2 tables also use.

## Translation to the PS2 layouts

`include/p4pc/golden_tables.h` builds row replacements over the selected base
input (disc, developer or package), so the package owns only the rows Golden
actually changes. A base span that is not the retail layout is refused.

- **Skill ids.** Actives keep their id below 0x1B8; Golden's 0x1B8..0x1D7 are
  Golden-only; Golden passives are the PS2 id plus 0x20. A Golden skill maps
  only to a PS2 slot that has a name: Golden fills several unnamed PS2 slots
  (Vigor Song in 0x11F keeps PS2's unused row bytes), and those stay out.
- **Skill rows.** Golden's 0x2C-byte row widens the flag byte at 0 to a word
  and pads after bytes 1..3; only Golden-only skills use the widened bits.
  Byte 1 (changed only on Healing Wave, with unmeasured meaning) stays PS2's.

| Group (setting) | Rows | Rules |
| --- | --- | --- |
| Numbers | SKILL rows/elements of named PS2 skills; compendium learnsets (0x01..0xBF); item rows 0x000..0x3FF | Items PS2 prices at 1 (dummy rows) stay PS2's; PS2 keeps an item's flags and availability window; a changed effect or grant must name a PS2 skill (Golden's new SP-regen Marks stay PS2's Invigorate grants, same 3/5/7 SP) |
| Enemies | UNIT0 and UNIT1 rows of named PS2 enemies | Golden HP, stats, affinities, EXP, yen, drops and weapon; **PS2 skill lists**, because the PS2 AI scripts name their skills; drops of items PS2 lacks (Golden banks, PS2 dummy rows) are removed |
| Party (`party`; only with `rank_skills`) | PERSONA0, the PERSONA2 party block and UNIT2 for 0xC0..0xCD | Learn events are remapped; a Golden-only learn and a forget at the same level (Golden's upgrade pairs, e.g. Rise's Vigor Song) are dropped together |
| EXP curves | PERSONA3 | Golden's party curves |
| Encounters (`encounters`; only with `enemies`) | ENCOUNT 0, 1, 3 and 4 | See [random encounters](#random-encounters-encounters) |

## Random encounters (`encounters`)

Golden renumbered its random formations, so its encounter set is swapped as a
whole and never mixed with PS2's formation IDs. Selectors are indexed by the
field step, and Golden keeps PS2's numbering for PS2's floors (every one of the
160 selectors names the same treasure group in both), so PS2's dungeon data
reaches Golden's rows. Layouts and readers are in
`include/p4pc/game_encounters.h` and [tables](tables.md#derived-table-sources).

- **Selectors** 1..159 are Golden's: weighted group, shadow counts
  (`Encount_GetEntryFlags`/`RateOrOverride`) and treasure count
  (`Encount_GetEntryRate`, at most the 8 field-object slots of
  `FldUnit_UpdateEnemySlot` 0x001662D0). Selector 0 is the game's "none"
  (`Encount_FindRecord` returns no treasure for it) and stays PS2's. A
  selector naming a group past PS2's 56 weighted or 75 treasure groups, or
  more than 8 treasures, refuses the package. Golden's selectors 160..179 are
  its own floors and stay out.
- **Weighted groups** 0..55 are Golden's, repacked from 252 to 248 bytes:
  Golden's header has three columns per rate (normal, special, rare); PS2's
  picker (`Encount_PickEncounter` 0x00161630) reads two (the day kinds), so the
  third is dropped. Each list keeps its 30 entries (normal 0..19, special
  20..24, rare 25..29). An entry naming a formation that is dropped is
  cleared, and a special or rare category that lost every entry that way gets
  rate 0, so the picker never draws an empty category (Golden's Happiness
  Hand, a PS2 blank enemy slot, is the rare draw on five early floors; those
  floors then have no rare draw). The normal rate stays: with every rate 0 the
  picker asserts, while an empty normal category returns "none", as PS2's
  own empty group 55 does.
- **Formations.** Every formation a translated group names is placed once. A
  formation naming an enemy PS2 lacks (an ID from 336, or a PS2 blank slot) is
  dropped. Otherwise it keeps its own ID where PS2's row is the same or names
  no enemy; PS2 rows a script may name (story, boss and every other row with
  enemies) are never overwritten, so a Golden formation whose ID PS2 uses for
  another row gets a new ID after PS2's 912, and the group names that ID.
  With build 10836033 the 56 groups name 617 formations: 7 identical, 152 in
  empty PS2 rows, 450 new (1362 rows) and 8 dropped.
- **Treasure groups** 0..74 are Golden's unless Golden's group names an item
  PS2 lacks (not a retail ID, or a dummy row PS2's same group does not hold)
  or yields other chest kinds than PS2's (byte 7 bit 0: normal or locked
  chests, counted where a record has an item and weight). A dungeon board in a
  save keeps each chest's kind and re-picks its record from the current group
  (`fldStepRestoreBoard` 0x0015AC60), and `Encount_FindRecord` asserts on an
  empty kind, in either direction (a board saved with the package and
  restored without it); the table service refuses such a set
  ([tables](tables.md#derived-table-sources)), so the group stays PS2's whole.
  With build 10836033, 62 groups are Golden's; 4, 14, 38, 43, 44 and 46 stay
  PS2's (Golden has no locked chests there) and 74 (no PS2 floor's) for its
  items. Golden's groups 75..83 are its own floors.

Golden's field shadow movement (`field/table/enemy.bin`, speed and sight
values) is not part of this group.

## Social Link perk order (`perk_order`)

PS2 and Golden give each party Social Link the same five battle perks in a
different order. `mods/p4pc.golden/main.luau` keeps the PS2 perk code and
changes only the rank each PS2 test sees:

| Perk | PS2 rank | Golden rank | PS2 test |
| --- | --- | --- | --- |
| Take a mortal blow for the protagonist | 1 | 9 | `0x001EFD50`, rank > 0 |
| Follow-up attack | 3 | 3 | `0x001EFF50`, rank >= 3 (unchanged) |
| Help a downed ally up | 5 | 1 | `0x001EF9C0`, rank >= 5 |
| Cure severe ailments | 7 | 5 | `0x001EF9C0`, rank >= 7 |
| Endure a mortal blow | 9 | 7 | `0x00242990`, rank >= 9 |

While one of those functions runs, a replacement of `cmmCommunityRecordValue`
(`0x00107AC0`) returns the PS2 rank that grants the same perk at the player's
real rank; outside them that hook is disabled. The rank-up announcement
(`cmmRankUpSelectDisplayIndex`, `0x00255ED0`) is remapped the same way, so the
message names the perk the new rank grants; Rise's navigator announcements
keep their ranks. The Golden order is a published game-rule fact (see
[the one exception](#published-facts-the-one-exception)).

### Published facts: the one exception

Everything else is derived from the player's install at run time. The perk
ranks above are the one Golden fact the port carries for good: five rank
numbers, no Golden bytes. Golden keeps them in code; its Steam `P4G.exe` is
protected and is only hashed, never unpacked or decrypted. The order is the
published one (the Megami Tensei Wiki Social Link pages for each member, for
example <https://megamitensei.fandom.com/wiki/Social_Link/Yosuke_Hanamura>,
Persona 4 Golden tab) and matches the comparisons in the player's separate,
unprotected 2020 PC executable (assist 1 and cure 5 in the status picker,
follow-up 3, endure 7, protect 9).

The rank-skill rows below are, for now, also carried in `main.luau`: they are
the rows of Golden's own `cmmRankUpSkillTable.dat` in the install
(`data_e.cpk` `init.bin`, `event/cmm.bin`) as PS2 skill ids, and the script
will read that table from the install instead once `p4.tables.source` is
available.

## Social Link Persona skills (`rank_skills`)

Each party Social Link teaches its member's Persona skills at the ranks of
Golden's `cmmRankUpSkillTable.dat` (rows `u16 member, u16 rank, u32 skill`,
the high half naming a skill it replaces), as PS2 skill ids:

| Member | Rank: skill |
| --- | --- |
| Yosuke | 2 Trafuri, 4 Dekaja, 6 Auto-Sukukaja, 8 Diarama, 10 Evade Elec |
| Chie | 2 Rebellion, 4 Ice Boost, 6 Revolution, 8 Bufula, 10 Evade Fire |
| Yukiko | 2 Mudo, 4 Divine Grace, 6 Amrita, 8 Mudoon, 10 Evade Ice |
| Rise | 2 Weakness Scan, 3 Full Analysis (replacing Analysis), 4 Third Eye, 5 Healing Wave |
| Kanji | 2 Dizzy Boost, 4 Masukunda, 6 Power Charge, 8 Regenerate 3, 10 Evade Wind |
| Naoto | 2 Invigorate 1, 4 Mind Charge, 6 Invigorate 2, 8 Heat Riser, 10 Invigorate 3 |
| Teddie | 0 Traesto (once he has his Persona), 4 Recarm, 6 Marakunda, 8 Samarecarm, 10 Evade Elec |

Left out: Rise's rank 8 Stamina Song (replacing Healing Wave), a Golden-only
skill with no PS2 counterpart, and every member's rank 11 row, the third
awakening skills. Rise's other Lovers perks (All-Out cheer, in-battle aid,
protection, Rise's Revival) are navigator support for phase 2.

A skill is taught when the player next controls an idle field after
`cmmScriptRankUp` (`0x0024B380`) finishes, so after a MAX rank's awakening,
and once after loading a save that already has the ranks. A row that replaces
a skill the Persona knows goes through the game's own `datPersonaReplaceSkill`
(`0x0010CD70`); otherwise `datPersonaAddSkill` (`0x0010CC20`) puts it in the
first free slot of the member's Persona (`datPersonaGetRecord`, `0x0010A900`).
A HUD status row announces it for six seconds. A skill the Persona already
knows is only marked as taught. When all eight slots are taken the skill waits on
**MODS > p4pc.golden > PERSONA SKILLS**: each member's earliest waiting skill
links to a page of the eight current skills, and confirming one replaces it
through `datPersonaReplaceSkill` (`0x0010CD70`) at the next frame boundary;
**DO NOT LEARN** declines it. The save block (schema 1) keeps the taught and
waiting ranks, so a skill the player later replaces or declines is never
taught again.

## Tests

`golden_source_test` exercises every reader and the translation on synthetic
fixtures (no Golden or retail bytes): @UTF columns, constants, masking and
bounds; literal and back-reference CRILAYLA streams with truncation and
capacity refusals; container and span bounds; a CPK written to the build
directory with a plain and a compressed entry; and the group rules above,
including base-layout refusal and the encounter rules (formation placement,
dropped entries and rates, selector 0 and selector refusal, treasure items and
chest kinds).

`mod_golden_game` (registered only with `P4PC_MOD_WITNESS_TESTS`, run under
`P4PC_MOD_WITNESS=1`, skipped unless `P4PC_GOLDEN` names an install) enables
the repository's package over the user's own disc and Golden install. With
`encounters = off` (the training walk is written for PS2's 4F encounters) it
plays the castle 5F training walk from the 4/20 checkpoint and saves, then
reloads that save into the 4/21 school walk; these legs check every table
group but encounters (0xF) and nine row sets, that the save requires the
package, and (reload, guest RAM at flip 7000) that exactly the Numbers group's
54 skill records differ from the disc's. With every default group (0x1F,
thirteen row sets) a fresh 4/20 checkpoint restores its castle board against
Golden's treasure groups and plays the training walk through its first
encounter, a formation Golden's 4F group picks; guest RAM after it must hold
Golden's weighted groups naming formations up to row 1361, each naming only
PS2 enemies, with every other formation row and selector 0 the disc's. A last
leg loads the 6/6 checkpoint, which already has party Social Link ranks, and
requires at least one rank skill taught or left waiting. Every leg checks
that nothing was refused and that both Social Link features installed. It is
a mod-witness run, never a retail comparison or reference.
