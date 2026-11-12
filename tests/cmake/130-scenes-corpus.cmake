# SkyGL-corpus field scenes (tge's PCSX2 savestates: the field id, date index, time slot and camera words read from the
# EE RAM, scenes.json in the notes of docs/renderer_port.md): p4pc_viewer field:F:R with P4PC_FIELD_DATE / SLOT / CAMERA
# against the emulator frame, refs/p4-<scene>.rgb beside the retail image. The limits sit just above the measured MAD: the
# NPCs, dialog windows and some effects of these frames are not reproduced yet, so these gates catch regressions only.
# P4PC_FIELD_WEATHER is the weather value of the day (calendar state +8 in the savestate: 0 clear, 1 rain, 2 cloudy); the frames
# with a different weather than the day table's (date 15) were 21% and 29% off before it.
# One scene gate of the field renderer on the i386 viewer, and its twin on the 64-bit viewer when it is built (tests/scene_mad.sh: the reference frame
# refs/<ref>.rgb beside the retail image, the viewer's mode, the limit in percent, then the viewer's environment).
function(p4_scene_gate name png mode ref limit)
  add_test(NAME ${name} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer> "${CMAKE_BINARY_DIR}/${png}" ${mode} ${ref} ${limit} ${ARGN})
  set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
  if(TARGET p4pc_viewer64)
    add_test(NAME ${name}64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_${png}" ${mode} ${ref} ${limit} ${ARGN})
    set_tests_properties(${name}64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 600)
    # Sky retains all 600 rasterised frames for feedback history, unlike the
    # short-window scenes. Qemu + llvmpipe exceeded its 2400 s limit.
    if(P4_PORT64_ARCH STREQUAL "aarch64" AND name STREQUAL "scene_sky_black_tint")
      set_property(TEST ${name}64 APPEND PROPERTY LABELS slow)
    endif()
  endif()
endfunction()

p4_scene_gate(scene_sky_black_tint scene_sky-black-tint.png field:10:1 p4-sky-black-tint 2.8 P4PC_FIELD_DATE=17 P4PC_FIELD_SLOT=4 "P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0")
p4_scene_gate(scene_save_butterfly_glow scene_save-butterfly-glow.png field:8:2 p4-save-butterfly-glow 3.4 P4PC_DRAW_WINDOW=30 P4PC_FIELD_PLAYER=311.5,0,-2216.1 P4PC_FIELD_DATE=16 P4PC_FIELD_SLOT=3 "P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656")

