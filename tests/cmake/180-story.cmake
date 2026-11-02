# E176 starts after going upstairs on 04/17: reaching Class 2-2 on the next morning
# proves the voice wait ended. school_0418 continues through its dialogue choice.
add_test(NAME voiced_event_0417 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 25000 6:6,4/18 story0417 repro_0417_night.walk)
set_tests_properties(voiced_event_0417 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)

# Continue from home_0417's card: the morning contact-info choice, classroom tutorials,
# and after-school free control. The walk saves at the classroom butterfly; retain the
# card as story0418 with P4PC_SAVE_OUT for the next short story gate.
add_test(NAME school_0418 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,4/18 story0417 save0417_to_school_0418.walk)
set_tests_properties(school_0418 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# From the classroom checkpoint, use school quick travel and the food-court
# meeting's third answer to enter the TV, then save at the entrance butterfly.
add_test(NAME tv_0418 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 10000 20:1,4/18 story0418 school_to_tv.walk)
set_tests_properties(tv_0418 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Traverse the 2F story corridor and assert step 8 (Castle 3F, not merely 40:0),
# use Goho-M and save at the Entrance. Retain the card as story0418_3f.
add_test(NAME castle_3f_0418 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 25000 20:1,4/18 story0418tv save0418tv_to_castle_3f.walk)
set_tests_properties(castle_3f_0418 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Leave the TV after the 3F trip, sleep through 04/18 evening and save at school
# after the next day's events. The classroom card is story0419.
add_test(NAME school_0419 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 12000 6:6,4/19 story0418_3f entrance_sleep_to_school.walk)
set_tests_properties(school_0419 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Restocked from Shiroku (three Goho-Ms), return to the TV and retain story0419tv.
add_test(NAME tv_0419 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,4/19 story0419 save0419_to_tv_0419.walk)
set_tests_properties(tv_0419 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Resume 3F, win the encounter on the stair route, reach step 9, escape and save.
add_test(NAME castle_4f_0419 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 20:1,4/19 story0419tv save0419tv_to_castle_4f.walk)
set_tests_properties(castle_4f_0419 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Rest after the 4F trip and finish the next school day's events.
add_test(NAME school_0420 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 12000 6:6,4/20 story0419_4f entrance_sleep_to_school.walk)
set_tests_properties(school_0420 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Return to the TV from the rested classroom checkpoint; retain story0420tv.
add_test(NAME tv_0420 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 10000 20:1,4/20 story0420 school_to_tv.walk)
set_tests_properties(tv_0420 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Win the fourth-floor encounters and assert the fixed fifth floor (60:1, step 10).
add_test(NAME castle_5f_0420 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 23000 20:1,4/20 story0420tv save0420tv_to_castle_5f.walk)
set_tests_properties(castle_5f_0420 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Train the underlevelled party through four normal fights, heal with Dia and save.
add_test(NAME castle_training_0420 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 38000 20:1,4/20 story0420_5f save0420_5f_to_training.walk)
set_tests_properties(castle_training_0420 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Rest after training and finish 04/21's events before resupplying.
add_test(NAME school_0421 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 13000 6:6,4/21 story0420trained entrance_sleep_to_school.walk)
set_tests_properties(school_0421 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Restock escape items and replace all three characters' starting armor.
add_test(NAME tv_0421_armored COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 35000 20:1,4/21 story0421 save0421_to_tv_0421.walk)
set_tests_properties(tv_0421_armored PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Eight ordinary encounters, with Dia between pairs and a live 4F assertion each time.
add_test(NAME castle_training_0421 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 20:1,4/21 story0421tv save0421tv_to_training.walk)
set_tests_properties(castle_training_0421 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Review the trained party before resting and completing the next school day's events.
add_test(NAME school_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,4/22 story0421trained save0421training_to_school_0422.walk)
set_tests_properties(school_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Return the newly rested party to the TV-world Entrance.
add_test(NAME tv_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 11000 20:1,4/22 story0422 school_to_tv.walk)
set_tests_properties(tv_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Replace the starting Persona with fire-resistant Orobas and preserve Zio/Patra.
add_test(NAME orobas_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 16000 20:1,4/22 story0422tv save0422tv_to_orobas.walk)
set_tests_properties(orobas_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Collect early Personas for the mid-boss's defensive fusion options, then save.
add_test(NAME castle_personas_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 33000 20:1,4/22 story0422orobas save0422orobas_to_personas.walk)
set_tests_properties(castle_personas_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Omoikane's native Tarunda/Sukunda provide the original-game mid-boss counter.
add_test(NAME omoikane_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 20:1,4/22 story0422personas save0422personas_to_omoikane.walk)
set_tests_properties(omoikane_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Persist Heal/Support before the boss's opening, then rest in the following gate.
add_test(NAME tactics_0422 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 20:1,4/22 story0422omoikane save0422omoikane_to_tactics.walk)
set_tests_properties(tactics_0422 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Rest the configured party and finish Saturday 04/23's school events.
add_test(NAME school_0423 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 15000 6:6,4/23 story0422tactics entrance_sleep_to_school.walk)
set_tests_properties(school_0423 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Restock five escapes and two 50-HP party heals, retaining the prepared AI tactics.
add_test(NAME tv_0423_supplied COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 20:1,4/23 story0423 save0423_to_tv_0423.walk)
set_tests_properties(tv_0423_supplied PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Tarunda/Sukunda, Zio, then a no-SP weapon finisher; require the live 5F return and save.
add_test(NAME avenger_knight_0423 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 42000 20:1,4/23 story0423tv save0423tv_to_knight.walk)
set_tests_properties(avenger_knight_0423 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Sunday has no school: take the room's town exit, choose Junes and save the rested party.
add_test(NAME tv_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,4/24 story0423knight save0423knight_to_tv_0424.walk)
set_tests_properties(tv_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# The defeated knight unlocks the north stair room; preserve the 6F label during callback patching.
add_test(NAME castle_6f_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,4/24 story0424tv save0424tv_to_castle_6f.walk)
set_tests_properties(castle_6f_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# The seeded 6F encounter is two Burning Beetles: obtain Bufu instead of repeating Zio.
add_test(NAME forneus_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,4/24 story0424_6f save0424_6f_to_forneus.walk)
set_tests_properties(forneus_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Fill the saved party's HP before traversing 6F; assert 7F before escaping and saving.
add_test(NAME castle_7f_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 20:1,4/24 story0424forneus save0424forneus_to_castle_7f.walk)
set_tests_properties(castle_7f_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Two ordinary 7F encounters, then the fixed top floor (23:2 / step 13).
add_test(NAME castle_8f_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 32000 20:1,4/24 story0424_7f save0424_7f_to_castle_8f.walk)
set_tests_properties(castle_8f_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Fire-null Senri inherits Bufu, while the original Omoikane remains available for Tarunda.
add_test(NAME senri_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 20000 20:1,4/24 story0424_8f save0424_8f_to_senri.walk)
set_tests_properties(senri_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# The compendium fee is paid normally; preserve fire-null Senri and Tarunda Omoikane.
add_test(NAME dekaja_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,4/24 story0424senri save0424senri_to_dekaja.walk)
set_tests_properties(dekaja_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# First encounter: Fire-weak/Ice-absorbing tables. Second: four-element-null shadows.
add_test(NAME castle_training_0424 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 35000 20:1,4/24 story0424dekaja save0424dekaja_to_training.walk)
set_tests_properties(castle_training_0424 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Rest after the upper-floor training, then finish Monday's school events.
add_test(NAME school_0425 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 6:6,4/25 story0424trained entrance_sleep_to_school.walk)
set_tests_properties(school_0425 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0425 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 12000 20:1,4/25 story0425 school_to_tv.walk)
set_tests_properties(tv_0425 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Ordinary upper-floor experience with Senri, saved at the real 8F butterfly (no Goho-M remains).
add_test(NAME castle_training1_0425 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 23000 23:2,4/25 story0425tv entrance_to_senri_training.walk)
set_tests_properties(castle_training1_0425 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME castle_training2_0425 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 23:2,4/25 story0425trained1 save0425trained1_to_training2.walk)
set_tests_properties(castle_training2_0425 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 6:6,4/26 story0425trained2 save0425trained2_to_school0426.walk)
set_tests_properties(school_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 12000 20:1,4/26 story0426 school_to_tv.walk)
set_tests_properties(tv_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME castle_training3_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 23:2,4/26 story0426tv entrance_to_senri_training.walk)
set_tests_properties(castle_training3_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME castle_training4_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 23:2,4/26 story0426trained3 save8f_to_senri_training.walk)
set_tests_properties(castle_training4_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME castle_training5_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 23:2,4/26 story0426trained4 save8f_to_senri_training.walk)
set_tests_properties(castle_training5_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME yukiko_rescue_0426 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 55000 6:6,4/27 story0426trained5 save0426trained5_to_yukiko_rescue.walk)
set_tests_properties(yukiko_rescue_0426 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0428 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 20000 6:6,4/28 story0427rescued school_sleep_to_school.walk)
set_tests_properties(school_0428 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME holiday_0429 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 25000 7:2,4/29 story0428 school_sleep_to_holiday.walk)
set_tests_properties(holiday_0429 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME yukiko_returns_0430 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 7:2,4/30 story0429 home0429_to_yukiko_returns0430.walk)
set_tests_properties(yukiko_returns_0430 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME holiday_0501 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 7:2,5/1 story0430 night_sleep_to_holiday.walk)
set_tests_properties(holiday_0501 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0502 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 6:6,5/2 story0501 home_day_sleep_to_school.walk)
set_tests_properties(school_0502 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME holiday_0503 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 7:2,5/3 story0502 school0502_to_holiday0503.walk)
set_tests_properties(holiday_0503 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME holiday_0504 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 7:2,5/4 story0503 night_sleep_to_holiday.walk)
set_tests_properties(holiday_0504 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0504 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 25000 20:1,5/4 story0504 home_day_to_tv.walk)
set_tests_properties(tv_0504 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME fox_0505 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 7:2,5/5 story0504tv tv0504_to_fox0505.walk)
set_tests_properties(fox_0505 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0506 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 6:6,5/6 story0505 night_sleep_to_school.walk)
set_tests_properties(school_0506 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0507 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 6:6,5/7 story0506 school_sleep_to_school.walk)
set_tests_properties(school_0507 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME sunday_0508 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 7:2,5/8 story0507 school0507_to_sunday0508.walk)
set_tests_properties(sunday_0508 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME midterms_0512 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 6:6,5/12 story0508 night_sleep_to_school.walk)
set_tests_properties(midterms_0512 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0513 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 6:6,5/13 story0512 school_sleep_to_school.walk)
set_tests_properties(school_0513 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME weekend_0515 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 7:2,5/15 story0513 school0513_to_weekend0515.walk)
set_tests_properties(weekend_0515 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME story_0516 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 7:2,5/16 story0515 night0515_to_story0516.walk)
set_tests_properties(story_0516 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME priestess_0517 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 7:2,5/17 story0516 night0516_to_priestess0517.walk)
set_tests_properties(priestess_0517 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME kanji_clue_0518 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,5/18 story0517 night0517_to_kanji_clue0518.walk)
set_tests_properties(kanji_clue_0518 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0519 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 28000 6:6,5/19 story0518clue night_sleep_to_school.walk)
set_tests_properties(school_0519 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME kanji_tv_0519 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 20:1,5/19 story0519 school0519_to_kanji_tv.walk)
set_tests_properties(kanji_tv_0519 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_discovered_0519 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 16000 20:1,5/19 story0519tv save0519tv_to_bathhouse.walk)
set_tests_properties(bathhouse_discovered_0519 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 16000 6:6,5/20 story0519bathhouse entrance_sleep_to_school.walk)
set_tests_properties(school_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0520_supplied COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 20:1,5/20 story0520 save0520_to_tv_0520.walk)
set_tests_properties(tv_0520_supplied PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_1f_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 24000 20:1,5/20 story0520tv save0520tv_to_bathhouse_1f.walk)
set_tests_properties(bathhouse_1f_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_2f_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 32000 20:1,5/20 story0520bathhouse1f save0520bathhouse1f_to_bathhouse_2f.walk)
set_tests_properties(bathhouse_2f_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_3f_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 32000 20:1,5/20 story0520bathhouse2f save0520bathhouse2f_to_bathhouse_3f.walk)
set_tests_properties(bathhouse_3f_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME wind_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 20:1,5/20 story0520bathhouse3f save0520bathhouse3f_to_wind.walk)
set_tests_properties(wind_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME kanji_glimpse_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 16000 20:1,5/20 story0520wind save0520wind_to_kanji_glimpse.walk)
set_tests_properties(kanji_glimpse_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_training_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 20:1,5/20 story0520glimpse save0520glimpse_to_training.walk)
set_tests_properties(bathhouse_training_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 16000 6:6,5/21 story0520trained entrance_sleep_to_school.walk)
set_tests_properties(school_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME supplied_tv_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 32000 20:1,5/21 story0521 save0521_to_tv_0521.walk)
set_tests_properties(supplied_tv_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_training_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 20:1,5/21 story0521tv save0521tv_to_gigas_training.walk)
set_tests_properties(bathhouse_training_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME recovered_tv_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 14000 20:1,5/21 story0521trained save0521trained_to_ready.walk)
set_tests_properties(recovered_tv_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_training2_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 20:1,5/21 story0521ready save0521ready_to_training2.walk)
set_tests_properties(bathhouse_training2_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME sunday_0522 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 7:2,5/22 story0521trained2 save0521trained2_to_sunday0522.walk)
set_tests_properties(sunday_0522 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0522 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 20:1,5/22 story0522 save0522_to_tv_0522.walk)
set_tests_properties(tv_0522 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME guard_party_0522 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 20:1,5/22 story0522tv save0522tv_to_direct.walk)
set_tests_properties(guard_party_0522 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_4f_0522 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 48000 20:1,5/22 story0522direct save0522direct_to_bathhouse_4f.walk)
set_tests_properties(bathhouse_4f_0522 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0523 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/23 story0522bathhouse4f entrance_sleep_to_school.walk)
set_tests_properties(school_0523 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME supplied_tv_0523 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 42000 20:1,5/23 story0523 save0523_to_tv_0523.walk)
set_tests_properties(supplied_tv_0523 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_5f_0523 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 20:1,5/23 story0523tv save0523tv_to_bathhouse_5f.walk)
set_tests_properties(bathhouse_5f_0523 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0524 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/24 story0523bathhouse5f entrance_sleep_to_school.walk)
set_tests_properties(school_0524 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0524 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,5/24 story0524 save0524_to_tv_0524.walk)
set_tests_properties(tv_0524 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_6f_0524 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 65000 20:1,5/24 story0524tv save0524tv_to_bathhouse_6f.walk)
set_tests_properties(bathhouse_6f_0524 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0525 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/25 story0524bathhouse6f entrance_sleep_to_school.walk)
set_tests_properties(school_0525 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME supplied_tv_0525 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 42000 20:1,5/25 story0525 save0525_to_tv_0525.walk)
set_tests_properties(supplied_tv_0525 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME bathhouse_7f_0525 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 20:1,5/25 story0525tv save0525tv_to_bathhouse_7f.walk)
set_tests_properties(bathhouse_7f_0525 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0526 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/26 story0525bathhouse7f entrance_sleep_to_school.walk)
set_tests_properties(school_0526 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0526 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,5/26 story0526 save0526_to_tv_0526.walk)
set_tests_properties(tv_0526 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME obariyon_0526 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 26000 20:1,5/26 story0526tv save0526tv_to_obariyon.walk)
set_tests_properties(obariyon_0526 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME bathhouse_guardian_0526 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 75000 20:1,5/26 story0526obariyon save0526obariyon_to_bathhouse_8f.walk)
set_tests_properties(bathhouse_guardian_0526 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0527 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/27 story0526bathhouse8f entrance_sleep_to_school.walk)
set_tests_properties(school_0527 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME tv_0527 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,5/27 story0527 save0527_to_tv_0527.walk)
set_tests_properties(tv_0527 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0528 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/28 story0527tv entrance_sleep_to_school.walk)
set_tests_properties(school_0528 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME supplied_tv_0528 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,5/28 story0528 save0528_to_tv_0528.walk)
set_tests_properties(supplied_tv_0528 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME sunday_0529 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 22000 7:2,5/29 story0528tv save0528tv_to_sunday0529.walk)
set_tests_properties(sunday_0529 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME tv_0529 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,5/29 story0529 save0529_to_tv_0529.walk)
set_tests_properties(tv_0529 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

add_test(NAME school_0530 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/30 story0529tv entrance_sleep_to_school.walk)
set_tests_properties(school_0530 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME supplied_tv_0530 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,5/30 story0530 save0530_to_tv_0530.walk)
set_tests_properties(supplied_tv_0530 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME bathhouse_9f_0530 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 65000 20:1,5/30 story0530tv save0530tv_to_bathhouse_9f.walk)
set_tests_properties(bathhouse_9f_0530 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME bathhouse_10f_0530 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 65000 20:1,5/30 story0530bathhouse9f save0530bathhouse9f_to_bathhouse_10f.walk)
set_tests_properties(bathhouse_10f_0530 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0531 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,5/31 story0530bathhouse10f entrance_sleep_to_school.walk)
set_tests_properties(school_0531 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME tv_0531 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,5/31 story0531 save0531_to_tv_0531.walk)
set_tests_properties(tv_0531 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0601 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,6/1 story0531tv entrance_sleep_to_school.walk)
set_tests_properties(school_0601 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME supplied_tv_0601 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,6/1 story0601 save0601_to_tv_0601.walk)
set_tests_properties(supplied_tv_0601 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME trained_tv_0601 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,6/1 story0601tv save0601tv_to_training.walk)
set_tests_properties(trained_tv_0601 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0602 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,6/2 story0601trained entrance_sleep_to_school.walk)
set_tests_properties(school_0602 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME supplied_tv_0602 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,6/2 story0602 save0602_to_tv_0602.walk)
set_tests_properties(supplied_tv_0602 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME bathhouse_11f_0602 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 65000 20:1,6/2 story0602tv save0602tv_to_bathhouse_11f.walk)
set_tests_properties(bathhouse_11f_0602 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0603 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 6:6,6/3 story0602bathhouse11f entrance_sleep_to_school.walk)
set_tests_properties(school_0603 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME tv_0603 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 18000 20:1,6/3 story0603 save0603_to_tv_0603.walk)
set_tests_properties(tv_0603 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME trained_tv_0603 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 120000 20:1,6/3 story0603tv save0603tv_to_training.walk)
set_tests_properties(trained_tv_0603 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME school_0604 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 20000 6:6,6/4 story0603trained entrance_sleep_to_school.walk)
set_tests_properties(school_0604 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME supplied_tv_0604 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 36000 20:1,6/4 story0604 save0604_to_tv_0604.walk)
set_tests_properties(supplied_tv_0604 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME healer_tv_0604 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 48000 20:1,6/4 story0604 save0604_to_healer_tv_0604.walk)
set_tests_properties(healer_tv_0604 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card2-slot08 anchor and input-produced descendants.
# These prep gates do not claim that the earlier New Game chain rescued Kanji.
add_test(NAME collection_kanji_andras_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 90000 20:1,5/20 playthrough0520kanji collection0520_to_andras_support.walk)
set_tests_properties(collection_kanji_andras_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_kanji_support111_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 90000 20:1,5/20 playthrough0520andra collection0520andras_to_support111.walk)
set_tests_properties(collection_kanji_support111_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_kanji_eligor_0520 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 35000 20:1,5/20 playthrough0520phoenix collection0520support111_to_eligor.walk)
set_tests_properties(collection_kanji_eligor_0520 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_kanji_supplies_0521 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 20:1,5/21 playthrough0520eligor collection0520eligor_to_funded_tv0521.walk)
set_tests_properties(collection_kanji_supplies_0521 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card2-slot09 begins after the unqualified Kanji fight.
# Its descendants cover the reunion and following school day, not the rescue.
add_test(NAME collection_sunday_0605 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,6/5 playthrough0604postkanji night_sleep_to_holiday.walk)
set_tests_properties(collection_sunday_0605 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_kanji_reunion_0606 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,6/6 playthrough0605postkanji collection0605_to_kanji_reunion0606.walk)
set_tests_properties(collection_kanji_reunion_0606 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_school_0607 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 6:6,6/7 playthrough0606postkanji night_sleep_to_school.walk)
set_tests_properties(collection_school_0607 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent June collection cards card2-slot10/-slot11/-slot12/-slot13: each sleeps
# (or goes out) from its anchor and follows the automatic story, free nights and school
# days to a native save: 06/20 (through the 06/17-06/18 camping trip), 06/21 (the next
# anchor's date), 06/23, 06/24.
add_test(NAME collection_camping_0620 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 6:6,6/20 playthrough0615evening collection0615_to_school0620.walk)
set_tests_properties(collection_camping_0620 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_0621 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,6/21 playthrough0619home collection0619_to_home0621.walk)
set_tests_properties(collection_evening_0621 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_0623 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 7:2,6/23 playthrough0621evening collection0621_to_home0623.walk)
set_tests_properties(collection_evening_0623 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_junes_0624 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,6/24 playthrough0623evening collection0623_to_home0624.walk)
set_tests_properties(collection_junes_0624 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
# The same anchor through 06/25's information search on Rise (grandmother, Chie, the
# photographer, the Risette fan, the photographer again) into the TV and a save at the
# Entrance.
add_test(NAME collection_rise_search_0625 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 32000 20:1,6/25 playthrough0623evening collection0623_to_rise_tv0625.walk)
set_tests_properties(collection_rise_search_0625 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card2-slot14, June 25 at the TV-world Entrance: travel to Marukyu
# Striptease and enter from the stage to 1F's arrival (step 41). Not its floors, Shadow
# Rise or a save.
add_test(NAME collection_marukyu_entry_0625 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 10000 42:0,6/25 playthrough0625tv collection0625_tv_to_marukyu1f.walk)
set_tests_properties(collection_marukyu_entry_0625 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Pristine Marukyu 11F collection anchor: Rise cleanup and its following event
# must survive the retail-layout allocator's post-release model writes, then
# reach Teddie's first fresh protagonist COMMAND. This is not Teddie defeat.
add_test(NAME marukyu_allocator COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/marukyu_allocator_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(marukyu_allocator PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)

# Independent post-Marukyu collection card2-slot16; no rescue claim.
add_test(NAME collection_school_story_0627 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,6/27 playthrough0626postmarukyu collection0626_to_school0627.walk)
set_tests_properties(collection_school_story_0627 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent post-Marukyu card2-slot16 again: 06/27 home, then the school days, free nights
# and evenings of 06/28-07/07 to the 07/08 home save (the next card's date).
add_test(NAME collection_evening_0708 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 80000 7:2,7/8 playthrough0626postmarukyu collection0626_to_home0708.walk)
set_tests_properties(collection_evening_0708 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot01, before the July school story.
add_test(NAME collection_school_0709 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 6:6,7/9 playthrough0708evening collection0708_to_school0709.walk)
set_tests_properties(collection_school_0709 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot02: July 9 evening through 07/11 and the school days,
# free nights and evenings of 07/12-07/17 to the 07/18 home save (the next card's date).
add_test(NAME collection_evening_0718 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 90000 7:2,7/18 playthrough0709evening collection0709_to_home0718.walk)
set_tests_properties(collection_evening_0718 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot03, July final-exam week through the home save.
add_test(NAME collection_final_exams_0724 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,7/24 playthrough0718exameve collection0718_to_exams0719.walk)
set_tests_properties(collection_final_exams_0724 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent summer-break cards card3-slot04 (07/26 evening: 07/27 Junes, home, then the
# automatic days to the 08/01 home save, the next card's date) and card3-slot05 (08/01
# daytime: out, nights, 08/02 and 08/03's days, home), each with a native save.
add_test(NAME collection_evening_0801 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 40000 7:2,8/1 playthrough0726evening collection0726_to_home0801.walk)
set_tests_properties(collection_evening_0801 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_0803 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,8/3 playthrough0801home collection0801_to_home0803.walk)
set_tests_properties(collection_evening_0803 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card3-slot06, August 3 at the TV-world Entrance: Rise's Chase the boy,
# the Void Quest title screen and Chapter 1's arrival (step 61). Not its floors or Mitsuo.
add_test(NAME collection_voidquest_entry_0803 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 8000 43:0,8/3 playthrough0803tv collection0803_tv_to_voidquest1f.walk)
set_tests_properties(collection_voidquest_entry_0803 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Pristine card3-slot07: Mitsuo introduction and fresh protagonist command only.
add_test(NAME collection_voidquest_opening_0803 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/voidquest_opening_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(collection_voidquest_opening_0803 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent post-Void Quest card3-slot08; this does not certify Mitsuo's defeat.
add_test(NAME collection_summer_break_0814 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,8/14 playthrough0812summer night_sleep_to_holiday.walk)
set_tests_properties(collection_summer_break_0814 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card3-slot09 follows the summer-job/festival week to its home save.
add_test(NAME collection_summer_festival_0822 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 40000 7:2,8/22 playthrough0814festival collection0814_to_festival0822.walk)
set_tests_properties(collection_summer_festival_0822 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot10: August 30 evening through 09/01 and the 09/02-09/06
# school days to the 09/07 home save (the next card's date).
add_test(NAME collection_evening_0907 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 7:2,9/7 playthrough0830evening collection0830_to_home0907.walk)
set_tests_properties(collection_evening_0907 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot11: school trip through September 11 home save.
add_test(NAME collection_school_trip_0911 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 7:2,9/11 playthrough0907trip collection0907_to_schooltrip0911.walk)
set_tests_properties(collection_school_trip_0911 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent September cards card3-slot12 (09/11 daytime: school days 09/12-09/14,
# 09/15 evening, the next anchor's date) and card3-slot13 (09/15 evening: 09/16 Junes,
# 09/17 school, 09/18 evening); native home saves, no Naoto claim.
add_test(NAME collection_evening_0915 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 40000 7:2,9/15 playthrough0911home collection0911_to_home0915.walk)
set_tests_properties(collection_evening_0915 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_0918 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,9/18 playthrough0915evening collection0915_to_home0918.walk)
set_tests_properties(collection_evening_0918 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card3-slot14: real Rise travel to Secret Laboratory exterior.
add_test(NAME collection_secretlab_entrance_0918 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/secretlab_entrance_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(collection_secretlab_entrance_0918 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent card3-slot15: B9F interior and Shadow Naoto opening, not victory.
add_test(NAME collection_shadow_naoto_opening_0918 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/naoto_opening_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(collection_shadow_naoto_opening_0918 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent post-rescue collection card3-slot16, not a Naoto victory witness.
add_test(NAME collection_return_1006 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,10/6 playthrough1005evening collection1005_to_return1006.walk)
set_tests_properties(collection_return_1006 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card4-slot01: October 6 evening through 10/07's free night,
# 10/08 school and 10/09 to the 10/10 home save (the next anchor's date).
add_test(NAME collection_evening_1010 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,10/10 playthrough1006evening collection1006_to_home1010.walk)
set_tests_properties(collection_evening_1010 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card4-slot02: both exam blocks through classroom native save.
add_test(NAME collection_midterms_1020 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 6:6,10/20 playthrough1010midterms collection1010_to_midterms1020.walk)
set_tests_properties(collection_midterms_1020 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent October cards card4-slot03 (after school 10/20: 10/21's classroom save),
# card4-slot04 (after school 10/21: 10/22-10/25 to the 10/26 home save) and card4-slot05
# (10/26 evening: 10/27, 10/28 home save); each later leg reaches the next card's date.
add_test(NAME collection_school_1021 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 6:6,10/21 playthrough1020school collection1020_to_school1021.walk)
set_tests_properties(collection_school_1021 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_1026 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 7:2,10/26 playthrough1021school collection1021_to_home1026.walk)
set_tests_properties(collection_evening_1026 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")
add_test(NAME collection_evening_1028 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,10/28 playthrough1026evening collection1026_to_home1028.walk)
set_tests_properties(collection_evening_1028 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card4-slot06: first school festival day and home save.
add_test(NAME collection_culture_festival_1029 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,10/29 playthrough1028festival collection1028_to_festival1029.walk)
set_tests_properties(collection_culture_festival_1029 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent collection card4-slot07: remaining festival story through classroom save.
add_test(NAME collection_culture_festival_1101 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 6:6,11/1 playthrough1029festival collection1029_to_festival1101.walk)
set_tests_properties(collection_culture_festival_1101 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card4-slot07 again: the remaining festival story, home, 11/02-11/03 and
# 11/04's after-school classroom save (the next card's date).
add_test(NAME collection_school_1104 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 70000 6:6,11/4 playthrough1029festival collection1029_to_school1104.walk)
set_tests_properties(collection_school_1104 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card4-slot08: abduction investigation, Fool9 and live Heaven entrance.
add_test(NAME collection_heaven_entrance_1106 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 60000 28:1,11/6 playthrough1104school collection1104_to_heaven1106.walk)
set_tests_properties(collection_heaven_entrance_1106 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent card4-slot09: real Heaven10 door/story, not Kunino Sagiri victory.
add_test(NAME collection_kunino_sagiri_opening_1106 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/kunino_sagiri_opening_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(collection_kunino_sagiri_opening_1106 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent post-rescue card4-slot10, not a Kunino Sagiri victory witness.
add_test(NAME collection_return_1121 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 30000 7:2,11/21 playthrough1120evening collection1120_to_return1121.walk)
set_tests_properties(collection_return_1121 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent post-rescue card4-slot10 again: 11/21 home, then 11/22-12/01's school days,
# free nights and automatic days to the 12/02 home save (the next card's date).
add_test(NAME collection_evening_1202 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 7:2,12/2 playthrough1120evening collection1120_to_home1202.walk)
set_tests_properties(collection_evening_1202 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Both hospital branches use the same requested MAX18657, never a later substitute.
# Completed bad ending has no field; GameClearDraw resets the new-cycle date to index9.
add_test(NAME collection_hospital_bad_1203 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 45000 "-1:-1,4/10" playthrough1202max18657 collection1202_max18657_to_hospital_bad.walk)
set_tests_properties(collection_hospital_bad_1203 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# All six observed correct answers, then native home save in the continuing story.
add_test(NAME collection_hospital_continue_1203 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 7:2,12/3 playthrough1202max18657 collection1202_max18657_to_hospital_continue.walk)
set_tests_properties(collection_hospital_continue_1203 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card4-slot12, December 3 evening after sparing Namatame: December 4's
# Junes deduction and hospital questions to the home save. Not the 12/05 clue search.
add_test(NAME collection_deduction_1204 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 45000 7:2,12/4 playthrough1203evening collection1203_to_home1204.walk)
set_tests_properties(collection_deduction_1204 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card4-slot15: December 6 evening through 12/07's home save;
# no Magatsu Inaba traversal claim.
add_test(NAME collection_evening_1207 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 35000 7:2,12/7 playthrough1206evening collection1206_to_home1207.walk)
set_tests_properties(collection_evening_1207 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent card4-slot16: final Magatsu field and first Adachi COMMAND, not victory.
add_test(NAME collection_adachi_opening_1223 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 30000 246:3,12/23 playthrough1223magatsu collection1223_magatsu_to_adachi.walk)
set_tests_properties(collection_adachi_opening_1223 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent post-Magatsu card5-slot01: Christmas through the farewell world map.
add_test(NAME collection_departure_calendar_0320 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 30000 1:1,3/20 playthrough1224evening collection1224_to_departure0320.walk)
set_tests_properties(collection_departure_calendar_0320 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent card5-slot02, March 20 in the shopping district: go home through the farewell
# map, play the departure, the normal ending and its credits, and save the new-cycle data
# over the card (native Save OK; the cleared-game state has no field, calendar index 9).
add_test(NAME collection_normal_ending_0320 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 "-1:-1,4/10" playthrough0320farewell collection0320_to_normal_ending.walk)
set_tests_properties(collection_normal_ending_0320 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Independent last-day card5-slot05: Rise's destination and live exterior control.
add_test(NAME collection_yomotsu_entrance_0320 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 12000 30:2,3/20 playthrough0320tv collection0320_tv_to_yomotsu_entrance.walk)
set_tests_properties(collection_yomotsu_entrance_0320 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent card5-slot06: Path 9 portal and first-phase Izanami COMMAND, not victory.
add_test(NAME collection_izanami_opening_0320 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 15000 247:2,3/20 playthrough0320yomotsu9f collection0320_yomotsu9f_to_izanami.walk)
set_tests_properties(collection_izanami_opening_0320 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Requested untouched CBS18506: native Margaret defeat, post-event Heaven10 control.
add_test(NAME collection_margaret_0320 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/completed_walk_gate.sh" $<TARGET_FILE:p4pc> 15000 28:2,3/20 playthrough0320margaret18506 collection0320_cbs18506_margaret_victory.walk)
set_tests_properties(collection_margaret_0320 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Independent true-ending clear data card5-slot07: LOAD GAME starts the new cycle
# (difficulty, name entry, opening) to 04/11's living room and its native save.
add_test(NAME collection_new_cycle_0411 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 50000 7:2,4/11 playthrough0410newcycle collection0410_newcycle_to_home0411.walk)
set_tests_properties(collection_new_cycle_0411 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_GATE_REQUIRE_SAVE=1")

# Rakukaja's ally-target group extends through byte +0x3C. Exercise that path
# rather than only Zio's enemy target, with the native dream-state input recipe.
add_test(NAME battle_rakukaja COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_gate.sh" $<TARGET_FILE:p4pc> 532 1800)
set_tests_properties(battle_rakukaja PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300
  ENVIRONMENT "P4PC_WALK_SCRIPT=${CMAKE_SOURCE_DIR}/tests/walks/dream_rakukaja.walk")

