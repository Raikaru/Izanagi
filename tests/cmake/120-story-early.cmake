# New Game to the first controllable field, scripted (tests/newgame_gate.sh, src/platform/newgame_gate.c): P4PC_TURBO=2 plays the
# difficulty prompt, the name entry and the opening events in about two minutes and requires field 7 room 2 at flip 30500 and the
# frame before it within 5.0% of refs/newgame_field.rgb (a port frame made with `newgame_gate.sh --make-ref`; never committed).
add_test(NAME newgame_to_field COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/newgame_gate.sh" $<TARGET_FILE:p4pc> 5.0 30500)
set_tests_properties(newgame_to_field PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
add_test(NAME story_to_0415 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/story_gate.sh" $<TARGET_FILE:p4pc> 140000 7:2,4/15)
set_tests_properties(story_to_0415 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1800)
# The first outdoor free roam, from the port-made 04/15 evening save (story_gate.sh --make-save; saves/story0415 beside the disc, never committed): 04/15 night,
# 04/16, 04/17 up to Daidara and the shop, then the shopping district (field 8 room 2) on 04/17. Skipped (77) without the save.
add_test(NAME street_0417 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 8:2,4/17 story0415 save0415_to_street.walk)
set_tests_properties(street_0417 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
# The same checkpoint on to Junes, the TV and the gate of Yukiko's Castle (field 23 room 1) on 04/17.
add_test(NAME castle_gate_0417 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 60000 23:1,4/17 story0415 save0415_to_castle.walk)
set_tests_properties(castle_gate_0417 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
# Through Yukiko's Castle 1F (a generated floor: the planner routes it, the battle driver wins its three shadow fights and plays Shuffle Time) and
# up to the 2F landing (field 23 room 3), where Teddie's scene ends and the player is free.
add_test(NAME castle_2f_0417 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 90000 23:3,4/17 story0415 save0415_to_castle_2f.walk)
set_tests_properties(castle_2f_0417 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1500)
# The same walk on through the retreat (the 2F landing's middle area) to the Dojima living room on the evening of 04/17, and a save at the calendar. Run with
# P4PC_SAVE_OUT=<retail dir>/saves/story0417 to keep that card as the checkpoint of the following days (never committed).
add_test(NAME home_0417 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 110000 7:2,4/17 story0415 save0415_to_home_0417.walk)
set_tests_properties(home_0417 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1800)
add_test(NAME save_play COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_play_gate.sh" $<TARGET_FILE:p4pc> 14000 68:1)
set_tests_properties(save_play PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)

# P4PC_DRAW_WINDOW=w (src/viewer/p4pc_viewer.c) steps the scene for all its frames but draws only the last w: the rasteriser is nearly
# all of these gates' time on llvmpipe. w is the smallest warm-up that reproduces the full-render frame (measured by running every
# w and comparing the PNGs): field_mad 2 (0 differing bytes; 4 is used);
# save_butterfly_glow 20 (6 to 12 pixels one level off, the same MAD to 4 decimals; 30 is used; no w up to 200 is byte-exact).
# The remaining viewer field-10 scene (sky_black_tint) keeps every frame: a frame-feedback pass leaves
# +-1 noise over the whole frame that only the full history reproduces (w=300 still differs by 1400 to 1600 pixels and moves the
# MAD in the 4th decimal; exact only from w=560).
# save_butterfly_glow reads 3.3083 on llvmpipe, which is what this limit judges; a GPU reads higher (3.425 on the RTX 4080).

# Cross-target determinism: the 04/11 save, the fog-dream walk and its tutorial battle (tests/walk_digest_gate.sh): a digest of the player's matrix and the random
# state every 500 flips and the battle's state changes, equal to the recorded run, which Linux and Windows print alike.
add_test(NAME walk_digest COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/walk_digest_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(walk_digest PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

