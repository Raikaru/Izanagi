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
Link perk order, the Social Link Persona skills and bike rides are selectable.
Golden moved several party skills (Yosuke's Dekaja and Diarama, Chie's Bufula
and Ice Boost, Teddie's Samarecarm, Naoto's Mind Charge, Rise's Full Analysis,
Third Eye and Healing Wave) from the learnsets to Social Link ranks, so the
producer applies the Party group only while `rank_skills` is on as well.

## Package and producer

`mods/p4pc.golden/mod.ini` declares the table settings `numbers`, `enemies`,
`party`, `exp_curves` and `encounters`, the script settings `perk_order`,
`rank_skills` and `bike_rides` (`main.luau`), and one `[table-derived golden]`
section with producer `p4pc:golden`, revision 1, over the inputs item, skill,
unit, msg, persona and encount ([derived table sources](tables.md#derived-table-sources)).
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
4. It records two raw files for the script ([`p4.tables.source`](lua.md#derived-table-source-files-tablessource1)):
   `init/cmpTable.bin` from `init_free.bin` (bike-ride skills) and
   `event/cmm.bin` from `init.bin` (Social Link rank skills), each checked
   against the supported build's digest.

The catalog identity, and so every save made with the package, binds to those
fingerprints and the composed inputs.

## Supported source

The Steam release (app 1113000), build 10836033. The reader opens only
`data_e.cpk` and takes the English battle tables from its `init_free.bin`:
`battle/SKILL.TBL`, `UNIT.TBL`, `PERSONA.TBL`, `MSG.TBL`, `ENCOUNT.TBL`,
`init/itemtbl.bin` and `init/cmpTable.bin`, and `event/cmm.bin` from its
`init.bin`. Each must match that build's SHA-256 (the digests, not the
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

The rank skills are not part of that exception: the script reads Golden's own
`cmmRankUpSkillTable.dat` from the install (below).

## Social Link Persona skills (`rank_skills`)

Each party Social Link teaches its member's Persona skills at the ranks of
Golden's `cmmRankUpSkillTable.dat`, which the script reads from the install's
`event/cmm.bin` (`p4.tables.source("p4pc:golden", "event/cmm.bin")`, a Golden
container: a u32 count, then entries of a 32-byte name, a u32 size and that
many bytes). After an 8-byte header its rows are `u16 member, u16 rank, u32
skill`, the high half naming a skill it replaces, ended by a zero row. Skill
ids become PS2 ids as in the table translation. For Steam build 10836033:

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
**DO NOT LEARN** declines it. Bike rides teach through the same path. The save
block (schema 1) keeps taught and waiting ranks, waiting ride skills and
declined ride skills. Declined skills are not offered by either ride talk,
including after reload; a learned skill later forgotten can be remembered.

## Bike rides (`bike_rides`)

Golden's scooter rides teach party Personas skills (decision D13: no scooter,
licence or Okina event; a ride invitation at the Moel gas station instead).
From 8/24 (date index 145), while the player stands near the attendant at the
Moel gas station in Shopping District, South (field 8, room 2), during
afternoon or after school (time slots 3 or 4), has not ridden that day and
nothing holds the time, a HUD row offers a ride after two consecutive
ready frames at the same horizontal position. The offer is withdrawn on
movement; SELECT opens **MODS > p4pc.golden > BIKE RIDE**.
If the position changes while the menu is open, the selected ride is cancelled.

Retail has no single free-time gate: each activity that spends time checks its
own conditions. What they share is the hold, `p4.calendar.hold()`
(`calendar.hold@1`). The time slot's owner is the scheduler: every calendar
step starts the `sdlScheduler` task, which runs the slot's `sdlMM_DD_<slot>`
procedure from the disc's `scheduler/scheduler_MM.bf`. A procedure that hands
the player no scene makes the task request the next calendar step itself, so a
free field in an afternoon exists only because that slot's procedure handed it
over, and the slot is held until the task is gone. Then the story hold: the
scheduler-owned flags on which the optional time-spending activities of the
free fields refuse, including the two in this same room (the capsule machines
and the bus stop's day-care job), Aiya and the shrine. They hold 12/05's
investigation ("You have to gather information") and the last day, 3/20, when
only the farewells remain. The ride is offered only without a hold, and
`p4.calendar.pass()` refuses during one, again at the frame boundary just
before it stages the slot. The ride keeps those retail activities' rule; their
own availability (rain, weekdays, prices) is not the ride's.

Why those two windows are all the story asks of an afternoon (read from the
PS2 disc's scheduler, field and event scripts, not from a played game): once
the ride's pass ends the slot, the game's own calendar step runs every later
procedure, so nothing scheduled is skipped. What a free afternoon could miss
is an obligation that a later procedure tests. From 8/24 the procedures that
run after a free afternoon test only these flags (the others test flags in
their own slot, before any field is handed over, or results that events set:
the exams, `0x3E1`..`0x3E4` from event 474, and the choices of event 376):

- `0x76`, which the 12/05 deduction sets (event 399, run by
  `sdl12_05_PM_D`; without it `K_Cmd_DATE_SCHEDULE` jumps to the 3/21
  ending), inside the `0x60F` hold;
- the 3/20 farewells and the 3/21 ending branches, inside the `0x602` hold
  (3/21 has no free slot: its morning procedures are empty and its afternoon
  runs the ending);
- the rescue deadlines, `0x15` through 10/05, `0x16` through 11/20 and `0x17`
  through 12/24 (a game over when still clear on the last day; `0x27` and
  `0x28` mark the rescue's follow-up event as seen): the player spends days
  against them, and no retail activity refuses on them;
- records of what the player chose: `0xAE3` (homework at the desk), `0x977`
  (set by event 890's flag procedure), and the Social Link and Christmas
  flags on 12/23 and 12/24.

The town fields' event procedures that fire in an afternoon from 8/24 are the
3/20 ones, inside the `0x602` hold; their other dated events are midnight
ones. The PS2 calendar has no January or February: those scheduler months
are empty and 12/25 ends by jumping to 3/20.

The ride page lists recruited
members whose Social Link has started (Rise has no ride skills and is not listed; her All-Out step
bonus belongs to the navigator group of phase 2); each member page has
Golden's two talks:

- **TALK ABOUT THE FUTURE** teaches the member's next skill of Golden's own
  `cmpPsSkillAddTable.ctd` (`init/cmpTable.bin`, read with
  `p4.tables.source`: a 16-byte header, then seven 20-byte rows of five u16
  skill ids, members 2..8 in order) that the Persona neither knows nor has
  waiting;
- **SHARE MEMORIES** teaches back the most recent skill the member's Persona
  learned by level (its learnset in the `PERSONA2` party block,
  `p4.tables.stock`) at or below its level and no longer has. If the bank is
  unavailable or too short, the ride is refused before time is passed.

A ride first ends the time slot through `p4.calendar.pass()` (capability
`calendar.pass@2`, which refuses while anything holds the time), which runs the game's
own calendar step; it also needs `scene@2` for the owner's retained
completion status. A refusal or an
unsuccessful completion gives no skill and does not consume the daily ride.
Once completion is reported as `passed`, the earned ride is saved even if an
event delays the next ready field. It commits its daily use and skill exactly
once on that ready frame (free slot, or it waits on PERSONA SKILLS), or the
talk "went another way" when the member has nothing to learn. A failed skill
replacement remains waiting rather than consuming the grant. Saving is refused
while the calendar request's outcome is still unknown.
The rows, as PS2 skills, for Steam build 10836033:
Yosuke Confuse Boost, Megido, Green Wall, Makara Break, Diarahan; Chie Hama,
Mind Slice, Hamaon, Tetra Break, Bufudyne; Yukiko Valiant Dance, Matarunda,
Red Wall, Spirit Drain, Mind Charge; Kanji Media, Atom Smasher, Blue Wall,
Fast Heal, Mediarama; Teddie Dekunda, Life Drain, Mustard Bomb, White Wall,
Evade Physical; Naoto Bufudyne, Ziodyne, Tetraja, Myriad Arrows, Angelic Grace.
The save block keeps the day of the last ride and how
far each member's future talks went in its row.

## Published groups (`contract@1`)

Packages that name `p4pc.golden` in `[companion]` or `[require]` (hard mode
balances Golden's enemy rows with its own per-boss tuning) read which groups
this run applies with `p4.contract.read("p4pc.golden", "groups")`, version 1:
booleans `numbers`, `enemies`, `party`, `exp_curves`, `encounters`,
`perk_order`, `rank_skills` and `bike_rides`. A group is true only when it is actually
applied, so `party` needs `rank_skills` and `encounters` needs `enemies`, as
the producer rules above. The values are the admitted restart settings.

## Tests

`golden_source_test` exercises every reader and the translation on synthetic
fixtures (no Golden or retail bytes): @UTF columns, constants, masking and
bounds; literal and back-reference CRILAYLA streams with truncation and
capacity refusals; container and span bounds; a CPK written to the build
directory with a plain and a compressed entry; and the group rules above,
including base-layout refusal and the encounter rules (formation placement,
dropped entries and rates, selector 0 and selector refusal, treasure items and
chest kinds).

`mod_golden_station_test.luau` uses synthetic scene, calendar and table data
to exercise the station boundary, slot eligibility, the hold (before the
offer, after the menu opens and while the slot's schedule still runs),
delayed pass outcomes and retention of a grant when replacement fails.
`mod_scene_status` drives the production pass request through each hold, at
the request and at the frame boundary, and `mod_calendar_hold` reads the
native hold from synthetic flags and task lists. None is an in-game witness.

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
PS2 enemies, with every other formation row and selector 0 the disc's. A fourth
leg loads the 6/6 checkpoint, which already has party Social Link ranks, and
requires at least one rank skill taught or left waiting. The rides leg uses
`P4PC_GOLDEN_RIDE_SEED` to select a private copy of the 09/11 home save
(collection card3-slot12); `collection0911_home_to_moel.walk` is a candidate
route via the town map and Moel, not a witnessed route. The leg opens BIKE
RIDE with SELECT once the HUD offer is up and talks about the future with the
first rider: the ride table must be read from the install, the pass queued and
finished `passed`, and the rider's next ride skill taught or left waiting.
Every leg checks that nothing was refused and that both Social Link features
and bike rides installed. It is a mod-witness run, never a retail comparison
or reference. The station boundary, the story hold's refusal on 12/05 and
3/20, the schedule task's absence once a free afternoon's field is ready, and
the resulting native calendar transition still need in-game evidence; the
obligation analysis above is read from the disc's scripts, not observed in
play.
