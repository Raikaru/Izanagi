# Host witness factories for Linux x86_64 and Windows x64. References are declared
# in their owning areas; 290-port64-host-cases registers both ABIs after final
# source-test policy. The original i386 targets remain the nightly reference.
if(P4_PORT64_LINK AND P4_PORT64_OBJS)

  # The renderer's reference gates on the 64-bit viewer (same scripts, limits and references as title_mad and field_mad): x86_64 measures the same
  # frames as i386 (title 1.14 %, field 1.775 %).
  if(TARGET p4pc_viewer64)
    add_test(NAME title_mad64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_title_mad.png")
    add_test(NAME field_mad64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_field_mad.png" field:7:2 field 1.9 P4PC_DRAW_WINDOW=4)
    # the variants of the title and field gates: desktop GL 3.3 instead of ES, and a lost and recreated GL context (docs/renderer_port.md)
    add_test(NAME title_mad_gl3364 COMMAND "${CMAKE_COMMAND}" -E env P4PC_GL_API=gl sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_title_mad_gl33.png")
    add_test(NAME title_mad_ctxloss64 COMMAND "${CMAKE_COMMAND}" -E env P4PC_TEST_CONTEXT_LOSS=200 sh "${CMAKE_CURRENT_SOURCE_DIR}/title_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_title_mad_ctxloss.png")
    add_test(NAME field_mad_ctxloss64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_field_mad_ctxloss.png" field:7:2 field 1.6 P4PC_DRAW_WINDOW=4 P4PC_TEST_CONTEXT_LOSS=300)
    set_tests_properties(title_mad_gl3364 title_mad_ctxloss64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 300)
    set_tests_properties(field_mad_ctxloss64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 600)
    set_tests_properties(title_mad64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 300)
    set_tests_properties(field_mad64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 600)
    # Viewer modes are not auto-cloned with the p4pc game gates.
    add_test(NAME widescreen_field_16_964 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
      $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_widescreen_field_16_9.png"
      field:7:2 port-wide-field-16-9 0.1 P4PC_ASPECT=16:9 P4PC_DRAW_WINDOW=4 "P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)")
    set_tests_properties(widescreen_field_16_964 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes;widescreen;port64" TIMEOUT 600)
    # Viewer gates need explicit twins; the registration prelude clones only p4pc.
    add_test(NAME scene_resolution_title108064 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64>
      "${CMAKE_BINARY_DIR}/64_resolution_title1080.png" title p4-resolution-title1080 0.5
      P4PC_RESOLUTION=1920x1080 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4)
    add_test(NAME scene_resolution_field136664 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64>
      "${CMAKE_BINARY_DIR}/64_resolution_field1366.png" field:7:2 p4-resolution-field1366 0.5
      P4PC_RESOLUTION=1366x768 P4PC_DRAW_WINDOW=4)
    add_test(NAME scene_resolution_butterfly144064 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64>
      "${CMAKE_BINARY_DIR}/64_resolution_butterfly1440.png" field:8:2 p4-resolution-butterfly1440 0.5
      P4PC_RESOLUTION=2560x1440 P4PC_DRAW_WINDOW=30 P4PC_FIELD_PLAYER=311.5,0,-2216.1 P4PC_FIELD_DATE=16 P4PC_FIELD_SLOT=3
      "P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656")
    set_tests_properties(scene_resolution_title108064 scene_resolution_field136664 scene_resolution_butterfly144064
      PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 600)
    add_test(NAME scene_resolution_title1366_cells64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64>
      "${CMAKE_BINARY_DIR}/64_resolution_title1366_cells.png" title p4-resolution-title1366-cells 0.5
      P4PC_RESOLUTION=1366x768 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4
      P4PC_WINDOW_W=1277 P4PC_WINDOW_H=954 P4PC_CAPTURE_WINDOW=1)
    add_test(NAME scene_resolution_title1277_cells64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh" $<TARGET_FILE:p4pc_viewer64>
      "${CMAKE_BINARY_DIR}/64_resolution_title1277_cells.png" title p4-resolution-title1277-cells 0.5
      P4PC_RESOLUTION=1277x954 P4PC_TITLE_FRAME=400 P4PC_DRAW_WINDOW=4
      P4PC_WINDOW_W=1277 P4PC_WINDOW_H=954 P4PC_CAPTURE_WINDOW=1)
    set_tests_properties(scene_resolution_title1366_cells64 scene_resolution_title1277_cells64
      PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 600 ENVIRONMENT "P4PC_TITLE_EDGES=1")
    add_test(NAME widescreen_field_16_9_nvidia64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
      $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_widescreen_field_16_9_nvidia.png"
      field:7:2 port-wide-field-16-9-nvidia-rtx4080 0.1 P4PC_ASPECT=16:9 P4PC_DRAW_WINDOW=4
      "P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
    set_tests_properties(widescreen_field_16_9_nvidia64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes64;widescreen;port64" TIMEOUT 600)
    foreach(wideRatio IN ITEMS 16-9 21-9)
      string(REPLACE "-" "_" wideName "${wideRatio}")
      if(wideRatio STREQUAL "21-9")
        set(wideMode auto)
        set(wideWidth 1680)
      else()
        set(wideMode "16:9")
        set(wideWidth 1280)
      endif()
      foreach(wideBackend IN ITEMS llvmpipe19 nvidia-rtx4080)
        if(wideBackend STREQUAL "llvmpipe19")
          set(wideSuffix 64)
          set(wideRenderer "llvmpipe (LLVM 19.1.7, 256 bits)")
          set(wideLabels "gl;scenes;widescreen;port64")
        else()
          set(wideSuffix _nvidia64)
          set(wideRenderer "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
          set(wideLabels "gl;scenes64;widescreen;port64")
        endif()
        add_test(NAME widescreen_title_cover_${wideName}${wideSuffix} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
          $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/64_widescreen_title_cover_${wideName}${wideSuffix}.png"
          title port-wide-title-cover-${wideRatio}-${wideBackend} 0.1
          P4PC_CONFIG=none P4PC_ASPECT=${wideMode} P4PC_WINDOW_W=${wideWidth} P4PC_WINDOW_H=720
          P4PC_RESOLUTION=scale:1 P4PC_PRESENT=blit P4PC_TITLE_FRAME=400 P4PC_VIRTUAL_CLOCK=1 P4PC_CAPTURE_WINDOW=1
          "P4PC_GATE_RENDERER=${wideRenderer}")
        set_tests_properties(widescreen_title_cover_${wideName}${wideSuffix} PROPERTIES
          SKIP_RETURN_CODE 77 LABELS "${wideLabels}" TIMEOUT 600 ENVIRONMENT "P4PC_TITLE_EDGES=1")
      endforeach()
    endforeach()
  endif()
  # Port frames are scoped to their qualified renderer, not compared across
  # software/hardware colour quantization with a looser limit.
  if(TARGET p4pc64)
    add_test(NAME widescreen_battle_16_9_nvidia64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh"
      $<TARGET_FILE:p4pc64> 532 port-wide-battle-16-9-nvidia-rtx4080 0 0.1
      "cross@360:2/150,start@500:2/300" P4PC_ASPECT=16:9
      "P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"
      P4PC_VIRTUAL_CLOCK=1 P4PC_CAPTURE_STATE=battle-menu,40,1,4000)
    set_tests_properties(widescreen_battle_16_9_nvidia64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes64;widescreen;port64" TIMEOUT 600)
    add_test(NAME widescreen_dialogue_16_9_nvidia64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh"
      $<TARGET_FILE:p4pc64> velvet-windows-blurry 3630 0.1 story0415 save0415_to_river_scene.walk)
    set_tests_properties(widescreen_dialogue_16_9_nvidia64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes64;widescreen;port64"
      TIMEOUT 900 ENVIRONMENT "P4PC_ASPECT=16:9;P4PC_EVENT_REF=port-wide-dialogue-16-9-nvidia-rtx4080;P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
    foreach(wideRatio IN ITEMS 16-9 21-9)
      string(REPLACE "-" "_" wideName "${wideRatio}")
      string(REPLACE "-" ":" wideAspect "${wideRatio}")
      foreach(wideScene IN ITEMS allout shuffle)
        set(wideVariant full)
        # Recaptured once the sky VU0 vector multiplies moved the effect-RNG timeline (docs/renderer_port.md).
        set(wideRef port-wide-shuffle-full-vu0-${wideRatio})
        if(wideScene STREQUAL "allout")
          set(wideVariant masked)
          set(wideRef port-wide-allout-masked-${wideRatio})
        endif()
        add_test(NAME widescreen_${wideScene}_${wideVariant}_${wideName}_nvidia64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/widescreen_saved_gate.sh"
          $<TARGET_FILE:p4pc64> ${wideScene} ${wideAspect} ${wideRef}-nvidia-rtx4080)
        set_tests_properties(widescreen_${wideScene}_${wideVariant}_${wideName}_nvidia64 PROPERTIES
          SKIP_RETURN_CODE 77 LABELS "gl;scenes64;widescreen;port64" TIMEOUT 900
          ENVIRONMENT "P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
      endforeach()
      foreach(wideCutin IN ITEMS allout-intro cast-eyes)
        string(REPLACE "-" "_" wideCutinName "${wideCutin}")
        add_test(NAME widescreen_${wideCutinName}_${wideName}_nvidia64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/widescreen_saved_gate.sh"
          $<TARGET_FILE:p4pc64> ${wideCutin} ${wideAspect} port-wide-${wideCutin}-${wideRatio}-nvidia-rtx4080)
        set_tests_properties(widescreen_${wideCutinName}_${wideName}_nvidia64 PROPERTIES
          SKIP_RETURN_CODE 77 LABELS "gl;scenes64;widescreen;port64" TIMEOUT 900
          ENVIRONMENT "P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
      endforeach()
    endforeach()
  endif()

  # Historical retail frames retain their independent bounds. These tighter
  # port witnesses catch regressions hidden by animation/retail residuals,
  # including ATST NEVER + ZB_ONLY silently discarding the fog TV's depth mask.
  # Never compare hardware/software quantization by widening a shared limit.
  function(p4_port_frame_properties name timeout)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/save/${name}")
    set(frameLabels "gl;scenes64;port64")
    if(timeout GREATER 1200)
      list(APPEND frameLabels slow)
    endif()
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77
      LABELS "${frameLabels}" TIMEOUT ${timeout}
      ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1;P4PC_ASPECT=4:3;P4PC_RESOLUTION=scale:1;P4PC_GATE_RENDERER=${portFrameRenderer};P4PC_SAVE_DIR=${CMAKE_BINARY_DIR}/save/${name};${ARGN}")
  endfunction()

  function(p4_port_event_frame name scene capture timeout seed walk)
    set(frameTest scene_port_${name}_${portFrameBackend}64)
    add_test(NAME ${frameTest} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_gate.sh"
      $<TARGET_FILE:p4pc64> ${scene} ${capture} 0.05 ${seed} ${walk} ${ARGN})
    p4_port_frame_properties(${frameTest} ${timeout}
      "P4PC_EVENT_REF=port-scene-${scene}-${portFrameRef}")
  endfunction()

  function(p4_port_viewer_frame name mode reference)
    set(frameTest scene_port_${name}_${portFrameBackend}64)
    add_test(NAME ${frameTest} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/scene_mad.sh"
      $<TARGET_FILE:p4pc_viewer64> "${CMAKE_BINARY_DIR}/${frameTest}.png"
      ${mode} port-scene-${reference}-${portFrameRef} 0.05 ${ARGN})
    p4_port_frame_properties(${frameTest} 600)
  endfunction()

  foreach(portFrameBackend IN ITEMS llvmpipe nvidia)
    if(portFrameBackend STREQUAL "llvmpipe")
      set(portFrameRenderer "llvmpipe (LLVM 19.1.7, 256 bits)")
      set(portFrameRef llvmpipe19)
    else()
      set(portFrameRenderer "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2")
      set(portFrameRef nvidia-rtx4080)
    endif()
    if(TARGET p4pc64)
      set(frameTest scene_port_fog_battle_${portFrameBackend}64)
      add_test(NAME ${frameTest} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh"
        $<TARGET_FILE:p4pc64> 532 port-scene-fog-battle-${portFrameRef} 1700 0.05
        "cross@360:2/150,start@500:2/300")
      p4_port_frame_properties(${frameTest} 300)
      set(frameTest scene_port_tv_entrance_${portFrameBackend}64)
      add_test(NAME ${frameTest} COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/systems_gate.sh"
        $<TARGET_FILE:p4pc64> entrance 0.05 port-scene-tv-entrance-${portFrameRef})
      p4_port_frame_properties(${frameTest} 1200)
      p4_port_event_frame(near_fog near-fog 4600 900 card save0411_to_near_fog_scene.walk
        0,0,640,200 0.05)
      p4_port_event_frame(velvet_windows_blurry velvet-windows-blurry 3630 900 story0415 save0415_to_river_scene.walk)
      p4_port_event_frame(riverbank_vignette riverbank-vignette 5880 900 story0415 save0415_to_river_scene.walk)
      p4_port_event_frame(rain rain 25100 1200 newgame newgame_to_rain_scene.walk)
      p4_port_event_frame(arcana_rankup_sparkles arcana-rankup-sparkles 7300 900 story0415 save0415_to_street.walk)
      p4_port_event_frame(tv_world_enter tv-world-enter 75594 1700 newgame newgame_to_0413.walk)
      p4_port_event_frame(shadow_haze shadow-haze 110560 1700 newgame newgame_to_0413.walk)
    endif()
    if(TARGET p4pc_viewer64)
      p4_port_viewer_frame(field field:7:2 field P4PC_DRAW_WINDOW=4)
      p4_port_viewer_frame(field_ctxloss field:7:2 field P4PC_DRAW_WINDOW=4 P4PC_TEST_CONTEXT_LOSS=300)
      p4_port_viewer_frame(sky_black_tint field:10:1 sky-black-tint
        P4PC_FIELD_DATE=17 P4PC_FIELD_SLOT=4
        "P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0")
      p4_port_viewer_frame(save_butterfly_glow field:8:2 save-butterfly-glow
        P4PC_DRAW_WINDOW=30 P4PC_FIELD_PLAYER=311.5,0,-2216.1 P4PC_FIELD_DATE=16 P4PC_FIELD_SLOT=3
        "P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656")
    endif()
  endforeach()

  # ovdiff (retail interpreter against the native, docs/override_diff.md) on the 64-bit objects: the same verdicts as i386 are the M4 criterion.
  # Not on macOS: it is a Linux harness (personality(), /proc, the ucontext register file, GNU ld's --wrap), which macOS has none of.
  if(NOT P4_APPLE)
    add_executable(ovdiff64 "${CMAKE_SOURCE_DIR}/tools/mipsdiff/ovdiff.c" "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" "${P4_TABLES64}" ${P4_OBJS64})
    target_include_directories(ovdiff64 PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
    target_compile_options(ovdiff64 PRIVATE ${P4_X64_FLAGS} -fno-builtin -Wno-unused)
    target_compile_definitions(ovdiff64 PRIVATE ${P4_PORT_DEFINES})
    target_link_options(ovdiff64 PRIVATE ${P4_X64_LINK_FLAGS} LINKER:--wrap=func_0043f810 LINKER:--wrap=func_0043f9c8)
    target_link_libraries(ovdiff64 m ${P4_SDL3_LIB64} ${P4_FFMPEG_LIBS64})
    add_dependencies(ovdiff64 p4decomp64)
    add_test(NAME ovdiff_subset64 COMMAND ovdiff64 -n 40 env "@${CMAKE_SOURCE_DIR}/tests/ovdiff_subset.txt")
    set_tests_properties(ovdiff_subset64 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" LABELS port64
      ENVIRONMENT "P4_MIPSDIFF_DIR=${CMAKE_SOURCE_DIR}/tools/mipsdiff")
  endif()
  # ---- The host tests that are not the game: a 64-bit twin of every one of them.
  # A host test is an executable built from port sources (and, for some, the rewritten mirror), run by one test or several (arguments, environment). Its twin
  # is the same sources compiled for the selected ABI, with the same arguments,
  # environment and limits. Guest-addressed storage belongs to the guest block.
  # Generated tables and mirror objects use the selected ABI; other object
  # libraries are rebuilt with their original compiler launcher.
  # A narrow-only compiler launcher must never be cloned into a 64-bit caller
  # witness: run the real full rewriter with the selected ABI before compiling.
  foreach(printf_suffix IN ITEMS 64 64w)
    if(printf_suffix STREQUAL "64w")
      if(NOT TARGET p4decomp64w)
        continue()
      endif()
      set(printf_flags ${P4W_FLAGS})
    else()
      set(printf_flags ${P4_X64_FLAGS})
    endif()
    set(printf_source "${CMAKE_CURRENT_SOURCE_DIR}/data/ee_printf_abi_calls.c")
    set(printf_generated "${CMAKE_CURRENT_BINARY_DIR}/ee_printf_abi_calls${printf_suffix}.i")
    add_custom_command(OUTPUT "${printf_generated}"
      COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/printf_rewrite.py"
        --full --clang "${CMAKE_C_COMPILER}" --root "${CMAKE_CURRENT_SOURCE_DIR}/data"
        --out "${printf_generated}" "${printf_source}" -- ${printf_flags}
      DEPENDS "${printf_source}" "${CMAKE_SOURCE_DIR}/tools/port64/printf_rewrite.py"
        "${CMAKE_SOURCE_DIR}/tools/port64/gptr_rewrite.py"
      VERBATIM COMMAND_EXPAND_LISTS)
    set_source_files_properties("${printf_generated}" PROPERTIES GENERATED TRUE LANGUAGE C)
    add_library(printf_abi_calls${printf_suffix} OBJECT "${printf_generated}")
    target_compile_options(printf_abi_calls${printf_suffix} PRIVATE ${printf_flags})
  endforeach()

  function(p4_host64_exe exe suffix)
    if(suffix STREQUAL "64w")
      set(flags ${P4W_FLAGS})
      set(linkflags ${P4W_LINK_FLAGS} -Wl,--stack=8388608)
      set(tables "${P4W_TABLES}")
      set(objects ${P4W_OBJS})
      set(decomp p4decomp64w)
      set(os_src "${CMAKE_SOURCE_DIR}/src/platform/os/os_win32.c")
      set(sdl "${P4W_SDL3}")
      set(avcodec "${P4W_AVCODEC}")
      set(avutil "${P4W_AVUTIL}")
      set(swscale "${P4W_SWSCALE}")
      set(exes_property P4_HOST64W_EXES)
    else()
      set(flags ${P4_X64_FLAGS})
      set(linkflags ${P4_X64_LINK_FLAGS})
      set(tables "${P4_TABLES64}")
      set(objects ${P4_OBJS64})
      set(decomp p4decomp64)
      set(os_src "${P4_OS_SRC}")
      set(sdl "${P4_SDL3_LIB64}")
      set(avcodec "${P4_AVCODEC_LIB64}")
      set(avutil "${P4_AVUTIL_LIB64}")
      set(swscale "${P4_SWSCALE_LIB64}")
      set(exes_property P4_HOST64_EXES)
    endif()
    if(TARGET ${exe}${suffix})
      set_property(GLOBAL APPEND PROPERTY ${exes_property} ${exe}${suffix})
      return()
    endif()
    get_target_property(srcs ${exe} SOURCES)
    get_target_property(incs ${exe} INCLUDE_DIRECTORIES)
    get_target_property(defs ${exe} COMPILE_DEFINITIONS)
    get_target_property(opts ${exe} COMPILE_OPTIONS)
    get_target_property(linkopts ${exe} LINK_OPTIONS)
    get_target_property(libs ${exe} LINK_LIBRARIES)
    if(P4_APPLE AND linkopts)
      # ld64's spelling of section garbage collection (GNU ld's --wrap has none: p4_host64_tests does not clone those tests on macOS)
      list(TRANSFORM linkopts REPLACE "^(LINKER:|-Wl,)--gc-sections$" "-Wl,-dead_strip")
    endif()
    set(mirror FALSE)
    set(src64)
    foreach(s IN LISTS srcs)
      if(s MATCHES "p4pc_tables" OR s MATCHES "TARGET_OBJECTS:p4(decomp|port|portee)>")
        set(mirror TRUE)
      elseif(s MATCHES "^[$]<TARGET_OBJECTS:([^>]+)>$")
        set(obj "${CMAKE_MATCH_1}")
        if(NOT TARGET ${obj}${suffix})
          get_target_property(msrcs ${obj} SOURCES)
          get_target_property(mincs ${obj} INCLUDE_DIRECTORIES)
          get_target_property(mdefs ${obj} COMPILE_DEFINITIONS)
          get_target_property(mopts ${obj} COMPILE_OPTIONS)
          get_target_property(launcher ${obj} C_COMPILER_LAUNCHER)
          get_target_property(mdir ${obj} SOURCE_DIR)
          set(object_srcs)
          foreach(ms IN LISTS msrcs)
            if(NOT IS_ABSOLUTE "${ms}")
              set(ms "${mdir}/${ms}")
            endif()
            if(suffix STREQUAL "64w")
              string(REGEX REPLACE "os/os_posix\\.c$" "os/os_win32.c" ms "${ms}")
            endif()
            list(APPEND object_srcs "${ms}")
          endforeach()
          add_library(${obj}${suffix} OBJECT ${object_srcs})
          if(mopts)
            target_compile_options(${obj}${suffix} PRIVATE ${mopts})
          endif()
          target_compile_options(${obj}${suffix} PRIVATE ${flags})
          target_compile_definitions(${obj}${suffix} PRIVATE ${P4_PORT_DEFINES})
          if(mdefs)
            target_compile_definitions(${obj}${suffix} PRIVATE ${mdefs})
          endif()
          if(mincs)
            if(suffix STREQUAL "64w")
              list(FILTER mincs EXCLUDE REGEX "^/usr/")
            endif()
            target_include_directories(${obj}${suffix} PRIVATE ${mincs})
          endif()
          if(suffix STREQUAL "64w")
            target_include_directories(${obj}${suffix} SYSTEM PRIVATE "${P4_WIN64_PREFIX}/include")
          endif()
          if(launcher)
            set_target_properties(${obj}${suffix} PROPERTIES C_COMPILER_LAUNCHER "${launcher}")
          endif()
        endif()
        list(APPEND src64 $<TARGET_OBJECTS:${obj}${suffix}>)
      elseif(s MATCHES "TARGET_OBJECTS")
        message(FATAL_ERROR "tests: ${exe} has an unrecognised object source ${s}")
      else()
        if(NOT IS_ABSOLUTE "${s}")
          get_target_property(sdir ${exe} SOURCE_DIR)
          set(s "${sdir}/${s}")
        endif()
        if(suffix STREQUAL "64w")
          string(REGEX REPLACE "os/os_posix\\.c$" "os/os_win32.c" s "${s}")
        endif()
        list(APPEND src64 "${s}")
      endif()
    endforeach()
    if(NOT mirror)
      foreach(extra "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${os_src}")
        if(NOT extra IN_LIST src64)
          list(APPEND src64 "${extra}")
        endif()
      endforeach()
    endif()
    if(mirror)
      add_executable(${exe}${suffix} ${src64} "${tables}" ${objects})
      target_compile_options(${exe}${suffix} PRIVATE -fno-builtin)
      add_dependencies(${exe}${suffix} ${decomp})
    else()
      add_executable(${exe}${suffix} ${src64})
    endif()
    if(opts)
      target_compile_options(${exe}${suffix} PRIVATE ${opts})
    endif()
    target_compile_options(${exe}${suffix} PRIVATE ${flags}
      -Werror=pointer-to-int-cast -Werror=int-to-pointer-cast -Werror=int-conversion
      -Werror=void-pointer-to-int-cast -Werror=int-to-void-pointer-cast)
    if(incs)
      if(suffix STREQUAL "64w")
        list(FILTER incs EXCLUDE REGEX "^/usr/")
      endif()
      target_include_directories(${exe}${suffix} PRIVATE ${incs})
    endif()
    target_compile_definitions(${exe}${suffix} PRIVATE ${P4_PORT_DEFINES})
    if(defs)
      target_compile_definitions(${exe}${suffix} PRIVATE ${defs})
    endif()
    target_link_options(${exe}${suffix} PRIVATE ${linkflags})
    if(linkopts)
      target_link_options(${exe}${suffix} PRIVATE ${linkopts})
    endif()
    set(libs64 m)
    foreach(l IN LISTS libs)
      if(l STREQUAL "${P4_SDL3_LIB}")
        list(APPEND libs64 ${sdl})
      elseif(l STREQUAL "${P4_AVCODEC_LIB}")
        list(APPEND libs64 ${avcodec})
      elseif(l STREQUAL "${P4_SWSCALE_LIB}")
        list(APPEND libs64 ${swscale})
      elseif(l STREQUAL "${P4_AVUTIL_LIB}")
        list(APPEND libs64 ${avutil})
      elseif(suffix STREQUAL "64w" AND l MATCHES "^(pthread|dl|rt)$")
        # Win32 code uses kernel32 and winpthread, not the POSIX import libraries.
      elseif(NOT l STREQUAL "m" AND NOT l MATCHES "NOTFOUND")
        list(APPEND libs64 ${l})
      endif()
    endforeach()
    if(mirror)
      list(APPEND libs64 ${sdl} ${avcodec} ${swscale} ${avutil})
    endif()
    list(REMOVE_DUPLICATES libs64)
    target_link_libraries(${exe}${suffix} ${libs64})
    if(suffix STREQUAL "64w")
      target_include_directories(${exe}${suffix} SYSTEM PRIVATE "${P4_WIN64_PREFIX}/include")
      target_link_libraries(${exe}${suffix} -Wl,-Bstatic winpthread -Wl,-Bdynamic kernel32 user32)
      set_target_properties(${exe}${suffix} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${P4W_OUT}" SUFFIX ".exe")
      add_dependencies(${exe}${suffix} p4pc64w_dlls)
    endif()
    set_property(GLOBAL APPEND PROPERTY ${exes_property} ${exe}${suffix})
  endfunction()

  # The retail override tests: the override sources are compiled by the test as they are, against stubs and against memory (symbols or absolute symbols at the retail
  # globals) the test defines. On a 64-bit host the shipped override is rewritten (unit64.py build --src: guest-memory globals, 32-bit guest pointers, the register ABI),
  # so the twin rewrites the same source and links the object with the same test, whose guest-reachable data sits in guest memory (tests/guest_test.h).
  #   p4_ovr_test64(<exe> <test source> OVERRIDES <addr>... [FLAGS <flag>...] [SOURCES <source>...])    builds <exe>64 and <exe>64w
  function(p4_ovr_test64 exe testsrc)
    cmake_parse_arguments(O "" "" "OVERRIDES;FLAGS;SOURCES" ${ARGN})
    if(NOT TARGET ${exe})
      return()
    endif()
    set(flagargs --cflag=-include "--cflag=${CMAKE_CURRENT_SOURCE_DIR}/override_test_types.h")
    foreach(f IN LISTS O_FLAGS)
      list(APPEND flagargs "--cflag=${f}")
    endforeach()
    foreach(suffix IN ITEMS 64 64w)
      if(suffix STREQUAL "64w")
        if(NOT TARGET p4pc64w)
          continue()
        endif()
        set(flags ${P4W_FLAGS})
        set(linkflags ${P4W_LINK_FLAGS})
        set(targetflags --target=${P4W_TRIPLE} -m64)
        set(opt "${P4_PORT64W_OPT}")
        set(launcher ${P4_PORT64W_LAUNCHER})
        set(exes_property P4_HOST64W_EXES)
        set(guestsrcs ${P4_GUEST_TEST_SRC})
        list(TRANSFORM guestsrcs REPLACE "os/os_posix\\.c$" "os/os_win32.c")
      else()
        set(flags ${P4_X64_FLAGS})
        set(linkflags ${P4_X64_LINK_FLAGS})
        set(targetflags ${P4_X64_TARGET_FLAGS})
        set(opt "${P4_PORT64_OPT}")
        set(launcher ${P4_PORT64_LAUNCHER})
        set(exes_property P4_HOST64_EXES)
        set(guestsrcs ${P4_GUEST_TEST_SRC})
      endif()
      string(JOIN " " target_string ${targetflags})
      set(objs)
      foreach(addr IN LISTS O_OVERRIDES)
        set(o "${CMAKE_CURRENT_BINARY_DIR}/ovr${suffix}/${exe}/${addr}.o")
        add_custom_command(OUTPUT "${o}"
          COMMAND "${CMAKE_COMMAND}" -E env "P4_PORT64_TARGET=${target_string}"
                  "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" build
                  --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}" --src "${CMAKE_SOURCE_DIR}/overrides/${addr}.c"
                  --out "${o}" --clang "${CMAKE_C_COMPILER}" --opt "${opt}" --check ${launcher} ${flagargs}
          DEPFILE "${o}.d"
          DEPENDS "${CMAKE_SOURCE_DIR}/overrides/${addr}.c" "${CMAKE_CURRENT_SOURCE_DIR}/override_test_types.h"
                  ${P4_PORT64_TOOLS} JOB_POOL port64 VERBATIM)
        list(APPEND objs "${o}")
      endforeach()
      set(srcs "${CMAKE_CURRENT_SOURCE_DIR}/${testsrc}" ${O_SOURCES})
      if(P4_EE_FLOAT AND launcher)
        list(APPEND srcs "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c")
      endif()
      add_executable(${exe}${suffix} ${srcs} ${objs} ${guestsrcs})
      target_compile_options(${exe}${suffix} PRIVATE ${flags}
        -Werror=pointer-to-int-cast -Werror=int-to-pointer-cast -Werror=int-conversion
        -Werror=void-pointer-to-int-cast -Werror=int-to-void-pointer-cast)
      target_compile_definitions(${exe}${suffix} PRIVATE ${P4_PORT_DEFINES})
      target_include_directories(${exe}${suffix} PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
      target_link_options(${exe}${suffix} PRIVATE ${linkflags})
      target_link_libraries(${exe}${suffix} m)
      if(suffix STREQUAL "64w")
        target_link_libraries(${exe}${suffix} kernel32 user32)
        set_target_properties(${exe}${suffix} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${P4W_OUT}" SUFFIX ".exe")
        add_dependencies(${exe}${suffix} p4pc64w_dlls)
      endif()
      set_property(GLOBAL APPEND PROPERTY ${exes_property} ${exe}${suffix})
    endforeach()
  endfunction()
  p4_ovr_test64(qsort_test qsort_test.c OVERRIDES 00440bb8
    SOURCES "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c")
  p4_ovr_test64(spline_test spline_test.c OVERRIDES 003b9a40)
  p4_ovr_test64(lockstep_restore_witness_test lockstep_restore_witness_test.c OVERRIDES 0010e710
    FLAGS -include "${CMAKE_CURRENT_SOURCE_DIR}/lockstep_restore_witness_prelude.h"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lockstep_align_guest_header_test.c"
      "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c")
  p4_ovr_test64(bmp_test bmp_test.c OVERRIDES 003d8c00)
  p4_ovr_test64(promoted_leaf_test promoted_leaf_test.c OVERRIDES 001f5680 00396e80 003cc250 003d5710 003b5fb0 003b6390 003b6420 003b64c0
    003b4230 003b65d0 003b6540 003d0790 003cbf30 003ce840 0039a910 003a2770 003a5180
    003b4ab0 003b4c10 003b4ff0 003b5650 003b5ac0 003b5bf0 003b6020 003b61e0)
  p4_ovr_test64(palquant_test palquant_test.c OVERRIDES 003f07d0 003f0610 003eff30 003f0110 003f0510 003f0c10 003f0de0
    003f02d0 003f03f0 003f1030 003f1240 003f13b0 003f1520 003f1600 003f1700 003f17e0 003f1ce0
    FLAGS -I "${CMAKE_SOURCE_DIR}/decomp/include" -I "${CMAKE_CURRENT_SOURCE_DIR}" -include "${CMAKE_CURRENT_SOURCE_DIR}/palquant_prelude.h")
  p4_ovr_test64(rtanim_test rtanim_test.c OVERRIDES 00415ad0 004165a0 00415bc0 00417310 00416e10 003d5840 003d59d0 003d5bc0 003dcc70
    SOURCES "${CMAKE_SOURCE_DIR}/src/platform/rw_rtanim_record.c"
      "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c"
      "${CMAKE_SOURCE_DIR}/src/platform/ee_libm.c" "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c")
  p4_ovr_test64(matrix_override_test matrix_override_test.c OVERRIDES 003df9f0 003e0e20 003e0680
    FLAGS -include "${CMAKE_CURRENT_SOURCE_DIR}/matrix_override_prelude.h")
  p4_ovr_test64(rxheap_test rxheap_test.c OVERRIDES 00415070 00414ef0 00414930 00414820 00414e10 00414a30 00414b50 00414c10
    SOURCES "${CMAKE_SOURCE_DIR}/src/platform/rx_heap_realloc.c")

  set(P4_HOST64_SKIP "")
  if(P4_PORT64_ARCH STREQUAL "aarch64")
    list(APPEND P4_HOST64_SKIP fpu_state)       # (checks the x86 MXCSR/x87 state restore of state_dump.c: SSE intrinsics)
  endif()
  # Called after every reference fixture and final source-test policy exists,
  # including late tool and lockstep tests. Both architectures use the same cases.
  function(p4_host64_tests suffix)
    set(skip ${P4_HOST64_SKIP})
    if(suffix STREQUAL "64w")
      list(REMOVE_ITEM skip fpu_state)
    endif()
    get_property(P4_CMD_TESTS GLOBAL PROPERTY P4_CMD_TESTS)
    foreach(tn IN LISTS P4_CMD_TESTS)
      if(tn IN_LIST skip OR tn MATCHES "64(w)?$" OR tn MATCHES "^port64_")
        continue()
      endif()
      get_property(cmd GLOBAL PROPERTY P4_CMD_${tn})
      set(exe)
      foreach(w IN LISTS cmd)
        if(TARGET "${w}")
          set(exe "${w}")
          break()
        elseif(w MATCHES "^[$]<TARGET_FILE:([A-Za-z0-9_]+)>$")
          set(exe "${CMAKE_MATCH_1}")
          break()
        endif()
      endforeach()
      if(NOT exe OR exe MATCHES "64(w)?$" OR exe STREQUAL "p4pc" OR exe STREQUAL "p4pc_viewer")
        continue()
      endif()
      # ovdiff has no Win32 variant: its interpreter harness is POSIX-only.
      if(suffix STREQUAL "64w" AND exe STREQUAL "ovdiff")
        continue()
      endif()
      # macOS: ld64 has no --wrap. The tests that intercept calls with it (ovdiff, the CRI file-system and SFD callback adapters) check
      # port code that is the same on every target; Linux and Windows run them.
      if(P4_APPLE)
        get_target_property(exe_linkopts ${exe} LINK_OPTIONS)
        if(exe_linkopts MATCHES "--wrap=")
          continue()
        endif()
      endif()
      get_target_property(etype ${exe} TYPE)
      if(NOT etype STREQUAL "EXECUTABLE")
        continue()
      endif()
      p4_host64_exe(${exe} ${suffix})
      if(TEST ${tn}${suffix})
        continue()
      endif()
      if(suffix STREQUAL "64w" AND NOT P4_WINE64)
        continue()
      endif()
      set(cmd64)
      if(suffix STREQUAL "64w")
        list(APPEND cmd64 "${CMAKE_COMMAND}" -E env WINEDEBUG=-all
          "WINEPREFIX=${CMAKE_CURRENT_BINARY_DIR}/wine64prefix")
      endif()
      foreach(w IN LISTS cmd)
        if(w STREQUAL "${exe}" OR w STREQUAL "$<TARGET_FILE:${exe}>")
          if(suffix STREQUAL "64w")
            list(APPEND cmd64 "${P4_WINE64}" "$<TARGET_FILE:${exe}${suffix}>")
          elseif(w STREQUAL "${exe}")
            list(APPEND cmd64 ${exe}${suffix})
          else()
            list(APPEND cmd64 "$<TARGET_FILE:${exe}${suffix}>")
          endif()
        elseif(w MATCHES "^[$]<TARGET_FILE:([A-Za-z0-9_]+)>$")
          set(other "${CMAKE_MATCH_1}")
          if(TARGET "${other}")
            get_target_property(other_type "${other}" TYPE)
          else()
            set(other_type "")
          endif()
          if(other_type STREQUAL "EXECUTABLE")
            p4_host64_exe(${other} ${suffix})
            add_dependencies(${exe}${suffix} ${other}${suffix})
            if(suffix STREQUAL "64w")
              list(APPEND cmd64 "${P4_WINE64}" "$<TARGET_FILE:${other}${suffix}>")
            else()
              list(APPEND cmd64 "$<TARGET_FILE:${other}${suffix}>")
            endif()
          else()
            list(APPEND cmd64 "${w}")
          endif()
        elseif(w MATCHES "^${CMAKE_BINARY_DIR}/${suffix}_")
          list(APPEND cmd64 "${w}")
        elseif(w MATCHES "^${CMAKE_BINARY_DIR}/([^/]+)$")
          list(APPEND cmd64 "${CMAKE_BINARY_DIR}/${suffix}_${CMAKE_MATCH_1}")
        else()
          list(APPEND cmd64 "${w}")
        endif()
      endforeach()
      add_test(NAME ${tn}${suffix} COMMAND ${cmd64})
      if(suffix STREQUAL "64w")
        set(lab port64win)
        set_tests_properties(${tn}${suffix} PROPERTIES FIXTURES_REQUIRED wine64_ready)
      else()
        set(lab port64)
      endif()
      get_test_property(${tn} LABELS olab)
      if(olab MATCHES "(^|;)gl(;|$)")
        list(APPEND lab gl)
      endif()
      if(olab MATCHES "(^|;)slow(;|$)" OR tn MATCHES "^(ovdiff_dblcores|movie_task_opening|movie_task_p4ct001)$")
        list(APPEND lab slow)
      endif()
      set(work "${CMAKE_CURRENT_BINARY_DIR}/host${suffix}/${tn}")
      file(MAKE_DIRECTORY "${work}")
      set_tests_properties(${tn}${suffix} PROPERTIES LABELS "${lab}" WORKING_DIRECTORY "${work}")
      foreach(prop SKIP_RETURN_CODE TIMEOUT WORKING_DIRECTORY RUN_SERIAL RESOURCE_LOCK)
        get_test_property(${tn} ${prop} pv)
        if(pv)
          set_tests_properties(${tn}${suffix} PROPERTIES ${prop} "${pv}")
        endif()
      endforeach()
      get_test_property(${tn} ENVIRONMENT oenv)
      set(env64)
      if(oenv)
        foreach(e IN LISTS oenv)
          if(NOT e MATCHES "^P4PC_SAVE_DIR=")
            list(APPEND env64 "${e}")
          endif()
        endforeach()
      endif()
      file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/save/${tn}${suffix}")
      list(APPEND env64 "P4PC_SAVE_DIR=${CMAKE_BINARY_DIR}/save/${tn}${suffix}")
      set_tests_properties(${tn}${suffix} PROPERTIES ENVIRONMENT "${env64}")
    endforeach()
  endfunction()

endif()

