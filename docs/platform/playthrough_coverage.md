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
(`bathhouse_guardian_0526`), and the daily school, holiday and shopping legs. Shadow
Kanji is not qualified; the Kanji preparation legs (`collection_kanji_*_0520/0521`) start
from collection card2-slot08 descendants.

## Calendar from June 4

| Dates | Anchor | Gate | Kind | Covers |
|---|---|---|---|---|
| 06/04–06/05 | card2-slot09 descendant | `collection_sunday_0605` | save | Sunday, home save |
| 06/05–06/06 | descendant | `collection_kanji_reunion_0606` | save | Kanji reunion |
| 06/06–06/07 | descendant | `collection_school_0607` | save | school day |
| 06/08–06/14 | none | — | — | no anchor |
| 06/15–06/20 | card2-slot10 | `collection_camping_0620` | save | 06/16, free night, 06/17–06/18 camping trip, 06/19, 06/20 school |
| 06/19–06/21 | card2-slot11 | `collection_evening_0621` | save | out and home, 06/20 school, free night, 06/21 school, home |
| 06/21–06/23 | card2-slot12 | `collection_evening_0623` | save | two automatic days to 06/23 evening |
| 06/23–06/24 | card2-slot13 | `collection_junes_0624` | save | 06/24 TV-world scene, Junes meeting, Go back home |
| 06/23–06/25 | card2-slot13 | `collection_rise_search_0625` | save | 06/24 as above, then 06/25's information search on Rise into the TV and an Entrance save |
| 06/25 | card2-slot14 | `collection_marukyu_entry_0625` | walk | Marukyu Striptease seats, stage, 1F arrival |
| 06/25 | card2-slot15 | `marukyu_allocator` | walk | 11F: Shadow Rise victory, Rise cleanup, Shadow Teddie's first COMMAND |
| 06/26–06/27 | card2-slot16 | `collection_school_story_0627` | save | post-rescue school story |
| 06/26–07/08 | card2-slot16 | `collection_evening_0708` | save | 06/27 home, school days and free nights to 07/08 evening |
| 07/08–07/09 | card3-slot01 | `collection_school_0709` | save | school |
| 07/09–07/18 | card3-slot02 | `collection_evening_0718` | save | 07/10–07/17 school days and free nights, 07/18 home |
| 07/18–07/24 | card3-slot03 | `collection_final_exams_0724` | save | exam week |
| 07/25 | none | — | — | no anchor |
| 07/26–08/01 | card3-slot04 | `collection_evening_0801` | save | 07/27 Junes meeting, home, summer days to 08/01 evening |
| 08/01–08/03 | card3-slot05 | `collection_evening_0803` | save | day out, 08/02, 08/03's automatic day, 08/03 evening home |
| 08/03 | card3-slot06 | `collection_voidquest_entry_0803` | walk | Void Quest title, Chapter 1 arrival |
| 08/03 | card3-slot07 | `collection_voidquest_opening_0803` | walk | Endgame: Shadow Mitsuo's first COMMAND |
| 08/04–08/11 | none | — | — | no anchor |
| 08/12–08/14 | card3-slot08 | `collection_summer_break_0814` | save | summer break |
| 08/14–08/22 | card3-slot09 | `collection_summer_festival_0822` | save | summer job, festival |
| 08/23–08/29 | none | — | — | no anchor |
| 08/30–09/07 | card3-slot10 | `collection_evening_0907` | save | end of summer, 09/02–09/06 school, 09/07 home |
| 09/07–09/11 | card3-slot11 | `collection_school_trip_0911` | save | school trip |
| 09/11–09/15 | card3-slot12 | `collection_evening_0915` | save | day out, school 09/12–09/14, 09/15 evening home |
| 09/15–09/18 | card3-slot13 | `collection_evening_0918` | save | 09/16 Junes, 09/17 school, 09/18 evening home |
| 09/18 | card3-slot14 | `collection_secretlab_entrance_0918` | walk | Secret Laboratory exterior (step 80) |
| 09/18 | card3-slot15 | `collection_shadow_naoto_opening_0918` | walk | B9F: Shadow Naoto's first COMMAND |
| 09/19–10/04 | none | — | — | no anchor |
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
| 11/07–11/19 | none | — | — | no anchor |
| 11/20–11/21 | card4-slot10 | `collection_return_1121` | save | post-rescue return |
| 11/20–12/02 | card4-slot10 | `collection_evening_1202` | save | 11/22–12/01 school days and automatic days, 12/02 evening home |
| 12/02–12/03 | MAX18657 (12/02) | `collection_hospital_bad_1203` | walk | Namatame pushed in: bad ending to the new-cycle prompt |
| 12/02–12/03 | MAX18657 (12/02) | `collection_hospital_continue_1203` | save | spare Namatame, 12/03 home |
| 12/03–12/04 | card4-slot12 | `collection_deduction_1204` | save | Junes deduction, hospital questions, 12/04 home |
| 12/05 | card4-slot13/-slot14 | — | — | see "12/05 clue search" below |
| 12/06–12/07 | card4-slot15 | `collection_evening_1207` | save | 12/07 home |
| 12/08–12/22 | none | — | — | no anchor |
| 12/23 | card4-slot16 | `collection_adachi_opening_1223` | walk | Magatsu Inaba top: Adachi's first COMMAND |
| 12/24–03/20 | card5-slot01 | `collection_departure_calendar_0320` | walk | Christmas, calendar jump, farewell map |
| 03/20 | card5-slot02 | `collection_normal_ending_0320` | save | home, departure, normal ending, credits, new-cycle save |
| 03/20 | card5-slot05 | `collection_yomotsu_entrance_0320` | walk | Search for Izanami: Yomotsu Hirasaka exterior |
| 03/20 | card5-slot06 | `collection_izanami_opening_0320` | walk | Path 9: Izanami's first COMMAND |
| 03/20 | CBS18506 (NG+) | `collection_margaret_0320` | walk | Margaret victory |
| 04/10–04/11 | card5-slot07 | `collection_new_cycle_0411` | save | true-ending clear data: new cycle to 04/11 home |

## Dungeons

| Dungeon | Entry | Floors | Boss |
|---|---|---|---|
| Yukiko's Castle | New Game chain | 1F–8F | Shadow Yukiko: `yukiko_rescue_0426` |
| Steamy Bathhouse | New Game chain | 1F–11F | Shadow Kanji: not qualified |
| Marukyu Striptease | `collection_marukyu_entry_0625` | 1F arrival; 1F–10F uncovered | Shadow Rise: `marukyu_allocator`; Shadow Teddie: opening only |
| Void Quest | `collection_voidquest_entry_0803` | Chapter 1 arrival; chapters uncovered | Shadow Mitsuo: opening only |
| Secret Laboratory | `collection_secretlab_entrance_0918` | exterior, B9F; B1F–B8F uncovered | Shadow Naoto: opening only |
| Heaven | `collection_heaven_entrance_1106` | gates, 10F; 1F–9F uncovered | Kunino-sagiri: opening only |
| Magatsu Inaba | none | final field only | Adachi: opening only; Ameno-sagiri: uncovered |
| Yomotsu Hirasaka | `collection_yomotsu_entrance_0320` | exterior, Path 9; Paths 1–8 uncovered | Izanami: opening only |

## Endings

- **Bad ending:** `collection_hospital_bad_1203` (to the native new-cycle save prompt, no write).
- **Normal ending:** `collection_normal_ending_0320`, through the credits and a native
  new-cycle save.
- **True-ending route:** `collection_yomotsu_entrance_0320` and
  `collection_izanami_opening_0320`; the clear data it produces is exercised by
  `collection_new_cycle_0411`. Izanami's defeat and the true ending itself are uncovered.

## Uncovered and why

- **Days with no anchor** (rows marked "no anchor" above): no collection card sits inside
  them or at their start, and chained private descendants are kept out of the collection
  gates. Every leg that could be played through to the next card's date was.
- **Dungeon floors between anchors** (Marukyu 1F–10F, Void Quest's chapters, Secret
  Laboratory B1F–B8F, Heaven 1F–9F, Magatsu Inaba, Yomotsu Paths 1–8): no anchor inside
  them, and generated floors are not yet stable across targets. From the same Marukyu card
  and input, the Windows x64 build wins one 1F battle and reaches 3F, while the Linux x64
  build's route parts from it about 21 units into 1F, meets three encounters and loses the
  third (the earlier one-digit differences in the walk logs are printf rounding, not state).
  The dungeon legs therefore stop at the first floor's arrival. With an attack-only plan,
  Void Quest Chapter 2 also lost an ordinary encounter on Windows.
- **Boss victories** (Shadow Kanji, Teddie, Mitsuo, Naoto, Kunino-sagiri, Adachi,
  Ameno-sagiri, Izanami): only openings are gated. Earlier scripted-combat studies of
  Mitsuo, Naoto and Izanami lost or did not finish, and an attack-only plan from the 11F
  card loses to Shadow Teddie (`dng_decease`); a victory needs a qualified battle plan per
  boss.
- **12/05 clue search:** after the 12/04 deduction, 12/05 starts at Junes; the elevator
  answers "You have to gather information... You decide to ask around." and going home
  answers "There doesn't seem to be enough information... You should go to town and gather
  more clues." In the shopping district the townspeople offer an "About those murders..."
  topic and party members (Yukiko) wait with hints; which conversations complete the search
  and lead to naming the culprit is not mapped yet, so card4-slot13/-slot14 have no gate.
- **03/20 true-ending trigger:** cards card5-slot03 and -slot04 are later farewell states in
  the shopping district (their save point is the area near (437, -2106)); Junes refuses
  them ("There's no need to be here..."). The step from them to the Entrance save
  (card5-slot05) is not mapped yet.
- **First-cycle hospital card card4-slot11 (12/02):** the same hospital segment as the
  gated MAX18657 card; Cross-only input plays it into the bad ending.
