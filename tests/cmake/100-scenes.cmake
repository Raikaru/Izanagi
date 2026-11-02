# Renderer regression gate: p4pc_viewer title (frame 400, the settled title screen) against the PCSX2 frame, mean
# absolute difference over RGB, failing above 2.0% (the frame measures 1.27%). The reference is a raw RGB file in
# refs/ beside the retail image (tools/png_to_rgb.py), never committed; skipped (exit 77) without P4PC_ISO,
# P4PC_ELF or the reference.
add_test(NAME title_mad COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/title_mad.png")
set_tests_properties(title_mad PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
# Five fresh title->LOAD GAME boots; require the restored field/date.
add_test(NAME title_load COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/title_load_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(title_load PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
# The title gate on desktop OpenGL 3.3 core, so that path keeps its gate when the default is the ES one.
add_test(NAME title_mad_gl33 COMMAND "${CMAKE_COMMAND}" -E env P4PC_GL_API=gl sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/title_mad_gl33.png")
set_tests_properties(title_mad_gl33 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
# Context loss: the GL context is replaced mid-run (P4PC_TEST_CONTEXT_LOSS=N, src/render/p4gl_context.c context_recover: what Android does when the
# system takes the GPU away) and the frame must still match its reference: every texture uploaded again from its raster's shadow, the programs, buffers
# and framebuffers rebuilt, the state caches reset. The title is 2D (im2d, sprites); the field adds models (atomic cache), im3d and render targets.
add_test(NAME title_mad_ctxloss COMMAND "${CMAKE_COMMAND}" -E env P4PC_TEST_CONTEXT_LOSS=200 sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/title_mad_ctxloss.png")
set_tests_properties(title_mad_ctxloss PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
add_test(NAME field_mad_ctxloss COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/field_mad_ctxloss.png" field:7:2 field 1.6 P4PC_DRAW_WINDOW=4 P4PC_TEST_CONTEXT_LOSS=300)
set_tests_properties(field_mad_ctxloss PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# The Dojima living room through the game's own field task (p4pc_viewer field:7:2, 600 frames) against the PCSX2 frame
# field.png, failing above 1.6% (the frame measures 1.49% on llvmpipe, 1.50% on NVIDIA and the Android emulator). Reference refs/field.rgb, as for title_mad (tests/scene_mad.sh).
add_test(NAME field_mad COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/field_mad.png" field:7:2 field 1.6 P4PC_DRAW_WINDOW=4)
set_tests_properties(field_mad PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
# The battle boot gate retains its established intro-state/nonblank witness.
# Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
add_test(NAME battle_menu COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_gate.sh" $<TARGET_FILE:p4pc> 20 420)
set_tests_properties(battle_menu PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
# The command menu of the Shadow Yosuke fight (encounter 512, the corpus battle-floor-a savestate's stage f222) against the PCSX2 frame: the same
# stage, hero and menu; the boss's pose and the film grain differ. refs/p4-battle-floor-a.rgb beside the retail image. The presses end at flip 1200 so
# the menu stays up; the virtual clock makes the frame repeat.
# (14.24 on i386 and 14.23 on x86_64 on llvmpipe. Until the bool-return pass of tools/gen_port.py the i386 build ran the boss's first action 31 frames early:
# btlFormationHasEntry left false in AL with a pointer above it and a packet's wait condition read it as true.)
add_test(NAME scene_battle_floor_a COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh" $<TARGET_FILE:p4pc> 512 p4-battle-floor-a 1400 14.4 "cross@360:6,cross@420:6,cross@480:6,cross@540:6,cross@600:6,cross@660:6,cross@720:6,cross@780:6,cross@840:6,cross@900:6,cross@960:6,cross@1020:6,cross@1080:6,cross@1140:6,cross@1200:6" P4PC_VIRTUAL_CLOCK=1)
set_tests_properties(scene_battle_floor_a PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
# The first command menu of the fog-dream tutorial battle (encounter 532, field 222; the Start presses skip the introduction's dialogue)
# against the retail GS frame (the PCSX2 software-GS screenshot of the retail dump, stretched to 640x448: refs/p4-fogbattle.rgb).
# The stage floor is a BSP world sector (drawn through the atomic path since the sector draw), the haze and fog passes, the
# feedback noise and the TV outline's Z-only draws (AFAIL = ZB_ONLY) that confine the additive speckle and the menu bar all show
# in it. What differs: the TV outline's animation phase and the COMMAND ring's angle (the port reaches the menu at another frame).
add_test(NAME scene_fog_battle COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh" $<TARGET_FILE:p4pc> 532 p4-fogbattle 1700 3.9 "cross@360:2/150,start@500:2/300" "P4PC_VIRTUAL_CLOCK=1")
set_tests_properties(scene_fog_battle PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300)
# The camp menu (P4PC_START_MENU=7:2, src/platform/menu_harness.c): the field's Triangle trigger opens it and Skill, Item, Equip,
# Persona, Status, S.Link and System each draw a lit frame. Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
add_test(NAME camp_menu COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/menu_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(camp_menu PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
# A lit readback is not capture evidence when its PNG cannot be written.
add_test(NAME camp_menu_write_failure COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/menu_gate.sh" $<TARGET_FILE:p4pc> --expect-write-failure)
set_tests_properties(camp_menu_write_failure PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 600)
# The save and load screens (src/platform/menu_harness.c, tests/save_gate.sh): a save made through the game's own save UI into the memory card
# folder reloads through its load UI to the same game state, and the folder has the retail layout. Skipped (exit 77) without P4PC_ISO and P4PC_ELF.
add_test(NAME save_load COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(save_load PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
# Retail saves (downloaded, converted by tools/ps2save.py, kept beside the disc in <retail dir>/saves and never committed) load through the title's LOAD GAME
# into their fields, and the state the port builds for saving matches the retail file but for play-time counters (tests/retail_save_gate.sh).
add_test(NAME retail_save_load COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/retail_save_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(retail_save_load PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
# Game systems off the story path, from the level-99 Apollo save (saves/card_apollo beside the disc, never committed): the Velvet Room's fusion list and
# compendium and the Marukyu Striptease floor menu, reached through the title's LOAD GAME, Goho-M out of the final dungeon, the TV-world Entrance, its Velvet Room door and Rise's navigator, then Teddie's shops (Daidara, Shiroku)
# (tests/systems_gate.sh, tools/systems/*.txt). refs/systems_<scenario>.rgb beside the retail image (`systems_gate.sh --make-ref`).
# Rooms the story path does not show first (tests/fields_gate.sh): started through the menu harness, each must load (the school roof and the shrine
# have textures the native texture decoder once refused).
add_test(NAME fields_load COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/fields_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(fields_load PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1500)
add_test(NAME systems_velvet_fusion COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> fusion)
set_tests_properties(systems_velvet_fusion PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
add_test(NAME systems_velvet_compendium COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> compendium)
set_tests_properties(systems_velvet_compendium PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
add_test(NAME systems_club_floor_menu COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> club)
set_tests_properties(systems_club_floor_menu PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
# Fixed generated-floor seeds preserve the regression references when asynchronous read ordering changes load duration.
# The minimap reference's GS sampler/colour correction was checked against retail at this same seed (renderer_port.md 11.1).
add_test(NAME systems_minimap COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> minimap 0.7)
set_tests_properties(systems_minimap PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200 ENVIRONMENT "P4PC_BOARD_SEED=0x17de")
# One generated floor per dungeon through the real entry (Rise's navigator): Void Quest and Heaven (the club's is systems_minimap).
add_test(NAME systems_game_1f COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> game1f)
set_tests_properties(systems_game_1f PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200 ENVIRONMENT "P4PC_BOARD_SEED=0x16bc")
add_test(NAME systems_heaven_1f COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> heaven1f)
set_tests_properties(systems_heaven_1f PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200 ENVIRONMENT "P4PC_BOARD_SEED=0x1699")
add_test(NAME systems_shop_daidara COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> daidara)
set_tests_properties(systems_shop_daidara PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
add_test(NAME systems_shop_shiroku COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> shiroku)
set_tests_properties(systems_shop_shiroku PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
# Retail GS audit scenes: material samplers and screen-affine Gouraud alpha on the Entrance haze.
add_test(NAME scene_tv_entrance COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> entrance 3.8)
set_tests_properties(scene_tv_entrance PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
add_test(NAME scene_marukyu_seats COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh" $<TARGET_FILE:p4pc> seats 3.0)
set_tests_properties(scene_marukyu_seats PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
# The real field-68 scripts, near the corridor end; the upper fog region excludes heading-dependent floor/character pixels.
add_test(NAME scene_near_fog COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> near-fog 4600 4.15 card save0411_to_near_fog_scene.walk 0,0,640,200 3.35)
set_tests_properties(scene_near_fog PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900 ENVIRONMENT "P4PC_EVENT_REF=p4-near-fog-corpus")
# Corpus dialogue beats reached from the real 04/15 checkpoint, including camera/procedure/weather validation.
add_test(NAME scene_velvet_windows_blurry COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> velvet-windows-blurry 3630 1.05 story0415 save0415_to_river_scene.walk)
set_tests_properties(scene_velvet_windows_blurry PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
add_test(NAME scene_riverbank_vignette COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> riverbank-vignette 5880 1.30 story0415 save0415_to_river_scene.walk)
set_tests_properties(scene_riverbank_vignette PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
add_test(NAME scene_rain COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> rain 25100 3.0 newgame newgame_to_rain_scene.walk)
set_tests_properties(scene_rain PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)
add_test(NAME scene_arcana_rankup_sparkles COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> arcana-rankup-sparkles 7300 3.65 story0415 save0415_to_street.walk)
set_tests_properties(scene_arcana_rankup_sparkles PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
add_test(NAME scene_tv_world_enter COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> tv-world-enter 75594 1.65 newgame newgame_to_0413.walk)
set_tests_properties(scene_tv_world_enter PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1700)
add_test(NAME scene_shadow_haze COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh" $<TARGET_FILE:p4pc> shadow-haze 110560 1.65 newgame newgame_to_0413.walk)
set_tests_properties(scene_shadow_haze PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1700)

# A real field walk driven through SDL, including short rim travel and 1 kHz cross-axis jitter.
add_test(NAME pad_field_run COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/pad_field_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(pad_field_run PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1500)
# Determinism against retail's recorded values (tests/rng_gate.sh): the boot seeds and draws, and the Marukyu floor's cell grid for the retail seed.
add_test(NAME rng_boot COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/rng_gate.sh" $<TARGET_FILE:p4pc> boot)
set_tests_properties(rng_boot PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 900)
add_test(NAME rng_floor COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/rng_gate.sh" $<TARGET_FILE:p4pc> floor)
set_tests_properties(rng_floor PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1800)

# Native port-frame goldens, not claims of a retail widescreen reference. The
# 4:3 retail gates above keep their original names, bytes and capture phases.
add_test(NAME widescreen_field_16_9 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
  $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/widescreen_field_16_9.png"
  field:7:2 port-wide-field-16-9 0.1 P4PC_ASPECT=16:9 P4PC_DRAW_WINDOW=4 "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")
set_tests_properties(widescreen_field_16_9 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen" TIMEOUT 600)
# Frame 0 is never presented; the state trigger owns the capture time.
# Keep this argument nonempty because registration forwards arguments as a CMake list.
add_test(NAME widescreen_battle_16_9 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh"
  $<TARGET_FILE:p4pc> 532 port-wide-battle-16-9 0 0.1
  "cross@360:2/150,start@500:2/300" P4PC_ASPECT=16:9 "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)"
  P4PC_VIRTUAL_CLOCK=1 P4PC_CAPTURE_STATE=battle-menu,40,1,4000)
set_tests_properties(widescreen_battle_16_9 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen" TIMEOUT 600)
add_test(NAME widescreen_dialogue_16_9 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh"
  $<TARGET_FILE:p4pc> velvet-windows-blurry 3630 0.1 story0415 save0415_to_river_scene.walk)
set_tests_properties(widescreen_dialogue_16_9 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen"
  TIMEOUT 900 ENVIRONMENT "P4PC_ASPECT=16:9;P4PC_EVENT_REF=port-wide-dialogue-16-9;P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")


# Physical-resolution regression goldens. These references contain the exact
# target dimensions, so accidentally rendering1x and merely presenting larger
# fails both the reference-size contract and the image comparison.
add_test(NAME scene_resolution_title1080 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer>
  "${CMAKE_BINARY_DIR}/resolution_title1080.png" title p4-resolution-title1080 0.5
  P4PC_RESOLUTION=1920x1080 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4)
set_tests_properties(scene_resolution_title1080 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
add_test(NAME scene_resolution_title1366_cells COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer>
  "${CMAKE_BINARY_DIR}/resolution_title1366_cells.png" title p4-resolution-title1366-cells 0.5
  P4PC_RESOLUTION=1366x768 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4
  P4PC_WINDOW_W=1277 P4PC_WINDOW_H=954 P4PC_CAPTURE_WINDOW=1)
add_test(NAME scene_resolution_title1277_cells COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer>
  "${CMAKE_BINARY_DIR}/resolution_title1277_cells.png" title p4-resolution-title1277-cells 0.5
  P4PC_RESOLUTION=1277x954 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4
  P4PC_WINDOW_W=1277 P4PC_WINDOW_H=954 P4PC_CAPTURE_WINDOW=1)
set_tests_properties(scene_resolution_title1366_cells scene_resolution_title1277_cells
  PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600 ENVIRONMENT "P4PC_TITLE_EDGES=1")
add_test(NAME scene_resolution_field1366 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer>
  "${CMAKE_BINARY_DIR}/resolution_field1366.png" field:7:2 p4-resolution-field1366 0.5
  P4PC_RESOLUTION=1366x768 P4PC_DRAW_WINDOW=4)
set_tests_properties(scene_resolution_field1366 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
add_test(NAME scene_resolution_butterfly1440 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer>
  "${CMAKE_BINARY_DIR}/resolution_butterfly1440.png" field:8:2 p4-resolution-butterfly1440 0.5
  P4PC_RESOLUTION=2560x1440 P4PC_DRAW_WINDOW=30 P4PC_FIELD_PLAYER=311.5,0,-2216.1 P4PC_FIELD_DATE=16 P4PC_FIELD_SLOT=3
  "P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656")
set_tests_properties(scene_resolution_butterfly1440 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
add_test(NAME scene_resolution_fog4k COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh" $<TARGET_FILE:p4pc>
  532 p4-resolution-fog4k 1700 0.5 "cross@360:2/150,start@500:2/300"
  P4PC_RESOLUTION=3840x2160 P4PC_VIRTUAL_CLOCK=1)
set_tests_properties(scene_resolution_fog4k PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 300 ENVIRONMENT "P4PC_FOG_SEAM=1")
# Full-canvas policies have new immutable port frames, qualified independently
# of the original 4:3 and widescreen field/battle/dialogue references.
foreach(wideRatio IN ITEMS 16-9 21-9)
  string(REPLACE "-" "_" wideName "${wideRatio}")
  if(wideRatio STREQUAL "21-9")
    set(wideMode auto)
    set(wideWidth 1680)
  else()
    set(wideMode "16:9")
    set(wideWidth 1280)
  endif()
  add_test(NAME widescreen_title_cover_${wideName} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
    $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/widescreen_title_cover_${wideName}.png"
    title port-wide-title-cover-${wideRatio}-llvmpipe19 0.1
    P4PC_CONFIG=none P4PC_ASPECT=${wideMode} P4PC_WINDOW_W=${wideWidth} P4PC_WINDOW_H=720
    P4PC_RESOLUTION=scale:1 P4PC_PRESENT=blit P4PC_TITLE_FRAME=400 P4PC_VIRTUAL_CLOCK=1 P4PC_CAPTURE_WINDOW=1
    "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")
  set_tests_properties(widescreen_title_cover_${wideName} PROPERTIES
    SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen" TIMEOUT 600 ENVIRONMENT "P4PC_TITLE_EDGES=1")
  string(REPLACE "-" ":" wideAspect "${wideRatio}")
  foreach(wideScene IN ITEMS allout shuffle)
    set(wideVariant full)
    # Recaptured once the sky VU0 vector multiplies moved the effect-RNG timeline (docs/renderer_port.md).
    set(wideRef port-wide-shuffle-full-vu0-${wideRatio})
    if(wideScene STREQUAL "allout")
      set(wideVariant masked)
      set(wideRef port-wide-allout-masked-${wideRatio})
    endif()
    add_test(NAME widescreen_${wideScene}_${wideVariant}_${wideName} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/widescreen_saved_gate.sh"
      $<TARGET_FILE:p4pc> ${wideScene} ${wideAspect} ${wideRef}-llvmpipe19)
    set_tests_properties(widescreen_${wideScene}_${wideVariant}_${wideName} PROPERTIES
      SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen" TIMEOUT 900
      ENVIRONMENT "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")
  endforeach()
  foreach(wideCutin IN ITEMS allout-intro cast-eyes)
    string(REPLACE "-" "_" wideCutinName "${wideCutin}")
    add_test(NAME widescreen_${wideCutinName}_${wideName} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/widescreen_saved_gate.sh"
      $<TARGET_FILE:p4pc> ${wideCutin} ${wideAspect} port-wide-${wideCutin}-${wideRatio}-llvmpipe19)
    set_tests_properties(widescreen_${wideCutinName}_${wideName} PROPERTIES
      SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen" TIMEOUT 900
      ENVIRONMENT "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")
  endforeach()
endforeach()

