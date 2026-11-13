# Playthrough coverage: June to the endings

This map lists which automated gate covers each stretch of the calendar, each dungeon
and each boss from June to the endings. The registrations in
`tests/cmake/180-story.cmake` and the walks they name are the executable source; this page
only indexes them. Collection anchors are the private PCSX2 playthrough cards listed in
`<scratch>/saves/playthrough/index.json` and pinned by SHA-256 in
`tests/collection_save_pins.json` (see [testing](testing.md) and
[saves](saves_memcard.md)). Every collection gate starts from an unchanged card, so a gate
covers only its own segment: it never certifies the days, floors or fights between two
anchors.

Gate kinds:

- **save**: `save_play_gate.sh` with `P4PC_GATE_REQUIRE_SAVE=1`; the walk must finish, the
  native save must report `SDKMEMCARD_SAVE OK`, and the field/room/date must match at the
  gate flip.
- **walk**: `completed_walk_gate.sh`; the walk must finish and the field/room/date must
  match. Used inside dungeons and at boss openings, where the game offers no save.

## April to June 4

The New Game chain (`newgame_to_field`, `story_to_0415`, then one `save_play_gate.sh`
leg per day in `120-story-early.cmake` and `180-story.cmake`) covers 04/11 to 06/04:
Yukiko's Castle 1F–8F with the Avenger Knight (`avenger_knight_0423`) and Shadow Yukiko
(`yukiko_rescue_0426`), the Steamy Bathhouse 1F–11F with its Daring Gigas guardian
(`bathhouse_guardian_0526`), and the daily school, holiday and shopping legs. The chain
does not fight Shadow Kanji; the Kanji preparation legs (`collection_kanji_*_0520/0521`)
start from collection card2-slot08 descendants. From the pristine card2-slot08 card at
Hall 11, `collection_kanji_round_0520` covers the boss door, Shadow Kanji's first COMMAND
and one round of Attacks, `collection_kanji_victory_0520` defeats him with a searched
battle tape and stops at the post-battle event, and `collection_kanji_aftermath_0604`
continues from that victory through the rescue story and every day to the 06/04 home save:
school days, free nights, automatic days and 05/22's Sunday out until evening.

## Calendar from June 4

Every day from June 4 to March 20 has a gate. A leg whose next collection card sits days
away plays on to that card's date and saves there: the `*_aftermath_*` gates continue a boss
victory through the rescue story, and `collection_june_0615`, `collection_evening_0726`,
`collection_evening_0830` and `collection_evening_1222` extend a card's own leg. Each such day
is one of four checked macros (school home by the map, evening sleep, the futon, or out until
evening), chosen by the field, room and date index it waits for first.

| Dates | Anchor | Gate | Kind | Covers |
|---|---|---|---|---|
| 06/04–06/05 | card2-slot09 descendant | `collection_sunday_0605` | save | Sunday, home save |
| 06/05–06/06 | descendant | `collection_kanji_reunion_0606` | save | Kanji reunion |
| 06/06–06/07 | descendant | `collection_school_0607` | save | school day |
| 06/04–06/15 | card2-slot09 | `collection_june_0615` | save | 06/05 holiday, 06/06 reunion, 06/07 school, 06/08–06/14 school days and free nights, 06/15 evening home |
| 06/15–06/20 | card2-slot10 | `collection_camping_0620` | save | 06/16, free night, 06/17–06/18 camping trip, 06/19, 06/20 school |
| 06/19–06/21 | card2-slot11 | `collection_evening_0621` | save | out and home, 06/20 school, free night, 06/21 school, home |
| 06/21–06/23 | card2-slot12 | `collection_evening_0623` | save | two automatic days to 06/23 evening |
| 06/23–06/24 | card2-slot13 | `collection_junes_0624` | save | 06/24 TV-world scene, Junes meeting, Go back home |
| 06/23–06/25 | card2-slot13 | `collection_rise_search_0625` | save | 06/24 as above, then 06/25's information search on Rise into the TV and an Entrance save |
| 06/25 | card2-slot14 | `collection_marukyu_entry_0625` (+ `_linux`) | walk, digest | Marukyu Striptease seats, stage, 1F and 2F with every encounter escaped, 3F arrival |
| 06/25 | card2-slot15 | `marukyu_allocator` | walk | 11F: Shadow Rise victory, Rise cleanup, Shadow Teddie's first COMMAND |
| 06/25 | card2-slot15 | `collection_teddie_victory_0625` | walk | 11F: Shadow Teddie's defeat with a searched battle tape, to the post-battle event |
| 06/25–06/26 | card2-slot15 | `collection_teddie_aftermath_0626` | save | Shadow Teddie's defeat, the rescue story, 06/26 evening home |
| 06/26–06/27 | card2-slot16 | `collection_school_story_0627` | save | post-rescue school story |
| 06/26–07/08 | card2-slot16 | `collection_evening_0708` | save | 06/27 home, school days and free nights to 07/08 evening |
| 07/08–07/09 | card3-slot01 | `collection_school_0709` | save | school |
| 07/09–07/18 | card3-slot02 | `collection_evening_0718` | save | 07/10–07/17 school days and free nights, 07/18 home |
| 07/18–07/24 | card3-slot03 | `collection_final_exams_0724` | save | exam week |
| 07/18–07/26 | card3-slot03 | `collection_evening_0726` | save | exam week, 07/24 home, 07/25, 07/26 evening home |
| 07/26–08/01 | card3-slot04 | `collection_evening_0801` | save | 07/27 Junes meeting, home, summer days to 08/01 evening |
| 08/01–08/03 | card3-slot05 | `collection_evening_0803` | save | day out, 08/02, 08/03's automatic day, 08/03 evening home |
| 08/03 | card3-slot06 | `collection_voidquest_entry_0803` (+ `_linux`) | walk, digest | Void Quest title, Chapter 1's floor, Chapter 2 arrival |
| 08/03 | card3-slot07 | `collection_voidquest_opening_0803` | walk | Endgame: Shadow Mitsuo's first COMMAND |
| 08/03 | card3-slot07 | `collection_mitsuo_victory_0803` | walk | Endgame: Shadow Mitsuo's defeat with a searched battle tape, to the post-battle event |
| 08/03–08/12 | card3-slot07 | `collection_mitsuo_aftermath_0812` | save | Shadow Mitsuo's defeat, the rescue story, summer days and free nights, 08/12 evening home |
| 08/12–08/14 | card3-slot08 | `collection_summer_break_0814` | save | summer break |
| 08/14–08/22 | card3-slot09 | `collection_summer_festival_0822` | save | summer job, festival |
| 08/14–08/30 | card3-slot09 | `collection_evening_0830` | save | summer job, festival, 08/23–08/29 automatic days and days out until evening, 08/30 evening home |
| 08/30–09/07 | card3-slot10 | `collection_evening_0907` | save | end of summer, 09/02–09/06 school, 09/07 home |
| 09/07–09/11 | card3-slot11 | `collection_school_trip_0911` | save | school trip |
| 09/11–09/15 | card3-slot12 | `collection_evening_0915` | save | day out, school 09/12–09/14, 09/15 evening home |
| 09/15–09/18 | card3-slot13 | `collection_evening_0918` | save | 09/16 Junes, 09/17 school, 09/18 evening home |
| 09/18 | card3-slot14 | `collection_secretlab_entrance_0918` | walk | Secret Laboratory exterior (step 80) |
| 09/18 | card3-slot15 | `collection_shadow_naoto_opening_0918` | walk | B9F: Shadow Naoto's first COMMAND |
| 09/18 | card3-slot15 | `collection_naoto_victory_0918` | walk | B9F: Shadow Naoto's defeat with walker battle rules, to the post-battle event |
| 09/18–10/05 | card3-slot15 | `collection_naoto_aftermath_1005` | save | Shadow Naoto's defeat, the rescue story, 09/19–10/04 school days, holidays and free nights, 10/05 evening home |
| 10/05–10/06 | card3-slot16 | `collection_return_1006` | save | post-rescue return |
| 10/06–10/10 | card4-slot01 | `collection_evening_1010` | save | 10/07, free night, 10/08 school, 10/09, 10/10 evening home |
| 10/10–10/20 | card4-slot02 | `collection_midterms_1020` | save | midterms |
| 10/20–10/21 | card4-slot03 | `collection_school_1021` | save | home, night, 10/21 school |
| 10/21–10/26 | card4-slot04 | `collection_evening_1026` | save | home, 10/22–10/25, 10/26 evening home |
| 10/26–10/28 | card4-slot05 | `collection_evening_1028` | save | 10/27, 10/28 evening home |
| 10/28–10/29 | card4-slot06 | `collection_culture_festival_1029` | save | festival day 1 |
| 10/29–11/01 | card4-slot07 | `collection_culture_festival_1101` | save | festival, trip |
| 10/29–11/04 | card4-slot07 | `collection_school_1104` | save | festival, 11/02–11/03, 11/04 school |
| 11/04–11/06 | card4-slot08 | `collection_heaven_entrance_1106` | walk | abduction, Heaven's gates (step 100) |
| 11/06 | card4-slot09 | `collection_kunino_sagiri_opening_1106` | walk | Heaven 10F: Kunino-sagiri's first COMMAND |
| 11/06 | card4-slot09 | `collection_kunino_sagiri_victory_1106` | walk | Heaven 10F: Kunino-sagiri's defeat with walker battle rules, to the post-battle event |
| 11/06–11/20 | card4-slot09 | `collection_kunino_sagiri_aftermath_1120` | save | Kunino-sagiri's defeat, the rescue story, 11/07–11/19 school days and free nights, 11/20 evening home |
| 11/20–11/21 | card4-slot10 | `collection_return_1121` | save | post-rescue return |
| 11/20–12/02 | card4-slot10 | `collection_evening_1202` | save | 11/22–12/01 school days and automatic days, 12/02 evening home |
| 12/02–12/03 | MAX18657 (12/02) | `collection_hospital_bad_1203` | walk | Namatame pushed in: bad ending to the new-cycle prompt |
| 12/02–12/03 | MAX18657 (12/02) | `collection_hospital_continue_1203` | save | spare Namatame, 12/03 home |
| 12/03–12/04 | card4-slot12 | `collection_deduction_1204` | save | Junes deduction, hospital questions, 12/04 home |
| 12/04–12/05 | card4-slot13 | `collection_culprit_1205` | save | 12/05 information search (every townsperson, both questions), Aiya, naming Adachi, questioning him, 12/05 home |
| 12/05–12/06 | card4-slot14 | `collection_adachi_1206` | save | the search's last answer, Aiya, naming Adachi, 12/06 story, 12/06 home |
| 12/06–12/07 | card4-slot15 | `collection_evening_1207` | save | 12/07 home |
| 12/06–12/22 | card4-slot15 | `collection_evening_1222` | save | 12/07 home, 12/08–12/21 school days and free nights, 12/22 evening home |
| 12/23 | card4-slot16 | `collection_adachi_opening_1223` | walk | Magatsu Inaba top: Adachi's first COMMAND |
| 12/23 | card4-slot16 | `collection_adachi_victory_1223` | walk | Yu's Persona switch, Adachi defeated by battle rules, Ameno-sagiri's first COMMAND |
| 12/23–12/24 | card4-slot16 | `collection_ameno_sagiri_victory_1223` | save | Adachi as above, Ameno-sagiri defeated by a searched battle tape, the story home, 12/24 evening home |
| 12/24–03/20 | card5-slot01 | `collection_departure_calendar_0320` | walk | Christmas, calendar jump, farewell map |
| 03/20 | card5-slot02 | `collection_normal_ending_0320` | save | home, departure, normal ending, credits, new-cycle save |
| 03/20 | card5-slot02 | `collection_true_trigger_0320` | save | true-ending trigger: Junes refusal, food court, Dojima, Velvet Room, attendant, TV Entrance save |
| 03/20 | card5-slot04 | `collection_attendant_0320` | save | the attendant and Izanami's reveal, TV Entrance save |
| 03/20 | card5-slot05 | `collection_yomotsu_entrance_0320` | walk | Search for Izanami: Yomotsu Hirasaka exterior |
| 03/20 | card5-slot06 | `collection_izanami_opening_0320` | walk | Path 9: Izanami's first COMMAND, one round of Attacks |
| 03/20 | card5-slot06 | `collection_izanami_victory_0320` | walk | Izanami and Izanami-no-Okami with a searched battle tape to Thousand Curses, Yu's final battle (Myriad Truths), Yomotsu Hirasaka exterior |
| 03/20 | card5-slot06 | `collection_true_ending_0320` | save | the same victory, the true ending's epilogue and credits, new-cycle save |
| 03/20 | CBS18506 (NG+) | `collection_margaret_0320` | walk | Margaret victory |
| 04/10–04/11 | card5-slot07 | `collection_new_cycle_0411` | save | true-ending clear data: new cycle to 04/11 home |

## Dungeons

| Dungeon | Entry | Floors | Boss |
|---|---|---|---|
| Yukiko's Castle | New Game chain | 1F–8F | Shadow Yukiko: `yukiko_rescue_0426` |
| Steamy Bathhouse | New Game chain | 1F–11F | Shadow Kanji: `collection_kanji_victory_0520` |
| Marukyu Striptease | `collection_marukyu_entry_0625` | 1F–2F, 3F arrival; 4F–10F uncovered | Shadow Rise: `marukyu_allocator`; Shadow Teddie: `collection_teddie_victory_0625` |
| Void Quest | `collection_voidquest_entry_0803` | Chapter 1, Chapter 2 arrival; later chapters uncovered | Shadow Mitsuo: `collection_mitsuo_victory_0803` |
| Secret Laboratory | `collection_secretlab_entrance_0918` | exterior, B9F; B1F–B8F uncovered | Shadow Naoto: `collection_naoto_victory_0918` |
| Heaven | `collection_heaven_entrance_1106` | gates, 10F; 1F–9F uncovered | Kunino-sagiri: `collection_kunino_sagiri_victory_1106` |
| Magatsu Inaba | none | final field only | Adachi: `collection_adachi_victory_1223`; Ameno-sagiri: `collection_ameno_sagiri_victory_1223` |
| Yomotsu Hirasaka | `collection_yomotsu_entrance_0320` | exterior, Path 9; Paths 1–8 uncovered | Izanami: `collection_izanami_victory_0320` |

## Bosses

| Boss | Date | Card | Gate | Status |
|---|---|---|---|---|
| Avenger Knight | 04/23 | New Game chain | `avenger_knight_0423` | defeated |
| Shadow Yukiko | 04/26 | New Game chain | `yukiko_rescue_0426` | defeated |
| Shadow Kanji | 05/20 | card2-slot08 | `collection_kanji_victory_0520` (first round: `collection_kanji_round_0520`) | defeated by a searched `battleturn:` tape |
| Daring Gigas | 05/26 | New Game chain | `bathhouse_guardian_0526` | defeated |
| Shadow Rise | 06/25 | card2-slot15 | `marukyu_allocator` | defeated |
| Shadow Teddie | 06/25 | card2-slot15 | `collection_teddie_victory_0625` | defeated by a searched `battleturn:` tape |
| Shadow Mitsuo | 08/03 | card3-slot07 | `collection_mitsuo_victory_0803` (opening: `collection_voidquest_opening_0803`) | defeated by a searched `battleturn:` tape |
| Shadow Naoto | 09/18 | card3-slot15 | `collection_naoto_victory_0918` | defeated by walker battle rules |
| Kunino-sagiri | 11/06 | card4-slot09 | `collection_kunino_sagiri_victory_1106` | defeated by walker battle rules |
| Adachi | 12/23 | card4-slot16 | `collection_adachi_victory_1223` | defeated by walker battle rules |
| Ameno-sagiri | 12/23 | card4-slot16 | `collection_ameno_sagiri_victory_1223` (opening: `collection_adachi_victory_1223`) | defeated by a searched `battleturn:` tape |
| Izanami | 03/20 | card5-slot06 | `collection_izanami_victory_0320` (opening and first round: `collection_izanami_opening_0320`) | defeated by a searched `battleturn:` tape to Thousand Curses' scripted defeat, then Yu's Myriad Truths |
| Margaret (optional) | 03/20 | CBS18506 (NG+) | `collection_margaret_0320` | defeated |

A win tape is the boss walk's own prefix, a `battle:` line and one `battleturn:` line per
fresh party command (see [testing](testing.md)), found by the battle simulator's fork
search on that walk's real state; the gate then runs it unchanged in an ordinary walk.

### Scripted battle inventory

The player's own disc can provide a preliminary inventory of selected scripted
battle starts, party commands and battle-unit lookups:

```sh
python tools/campaign_battles.py --iso <p4.iso> --elf <SLUS_217.82> --out <private.json> --summary
```

The tool scans non-media files for candidate flowscripts and linearly approximates
their stacks using the ELF's command parameter counts. It lists selected
`btlScriptStartEncounter`, `CALL_BATTLE`, `Cmd_PickEncounter`, `PARTY_IN`,
`PARTY_OUT`, `GetPartyMember` and battle-unit command calls with their inferred
arguments and, where available, encounter enemies and escape bit. This is not
control-flow analysis: inferred arguments and reachability may be wrong, and
dynamic arguments, excluded media, unrecognized blocks or commands, field symbol
encounters and native tutorial, advantage and results paths leave gaps. It is not
a complete battle list or proof that a solo-member rule is qualified for a fight.

The ISO and ELF are read only. Output contains retail-derived data: choose a new
private path outside this checkout and the configured retail area; existing files
are never overwritten. The tool requests owner-only file permissions where
supported, but the user must ensure the destination, stdout, summary and any
redirected output are private and never committed. Never redirect stdout onto an
input or other evidence: the shell may truncate it before the tool starts.

## Endings

- **Bad ending:** `collection_hospital_bad_1203` (to the native new-cycle save prompt, no write).
- **Normal ending:** `collection_normal_ending_0320`, through the credits and a native
  new-cycle save.
- **True ending:** `collection_true_trigger_0320` (farewell to the Entrance),
  `collection_yomotsu_entrance_0320`, then `collection_true_ending_0320` from the Path 9
  card: Izanami's defeat, the epilogue, the staff roll and the new-cycle save. The clear data
  it produces is exercised by `collection_new_cycle_0411`.

## Uncovered and why

- **Dungeon floors between anchors** (Marukyu 4F–10F, Void Quest after Chapter 2's
  arrival, Secret Laboratory B1F–B8F, Heaven 1F–9F, Magatsu Inaba, Yomotsu Paths 1–8): no
  anchor inside them. Generated floors are stable across targets: the Marukyu and Void
  Quest gates are cross-target digest pairs (`tests/dungeon_digest_gate.sh`), and the
  2026-11-02 Marukyu split between a Windows and a Linux run was two different sources
  (before and after the VU0 multiplies of 06c24b38), not two targets
  ([numeric determinism](numeric_determinism.md#dungeon-floors-across-targets)). Under that
  arithmetic an attack-only Yu loses the third 1F encounter, so the Marukyu walk escapes
  every encounter. With an attack-only plan, Void Quest Chapter 2 also lost an ordinary
  encounter on Windows.
- **First-cycle hospital card card4-slot11 (12/02):** the same hospital segment as the
  gated MAX18657 card; Cross-only input plays it into the bad ending.
