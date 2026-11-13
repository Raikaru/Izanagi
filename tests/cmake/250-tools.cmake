# Tests that run no game or port executable (python and shell checks of tools and sources: the add_test wrapper in the registration prelude records them) have no
# i386 or 64-bit build to belong to, so they take the label port64 and run with the x64 build.
# Reference metadata/checker regression tests; no retail files are needed.
add_test(NAME doc_links COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/check_doc_links.py")
add_test(NAME ref_guard COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/ref_guard_test.py")
add_test(NAME scale_diff COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/scale_diff_test.py")
add_test(NAME title_edges COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/title_edges_test.py")
add_test(NAME cutin_edges COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/cutin_edges_test.py")
add_test(NAME disc_contract COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/disc_contract_test.py")
# Auto-init must instrument the custom rewritten mirror compiler.
add_test(NAME unit64_auto_init COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/unit64_auto_init_test.py")
add_test(NAME field_entry_witness COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/field_entry_test.py")
# Real-play captures follow procedure timers or camera/player state, not a fragile absolute flip.
add_test(NAME event_scene_capture COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_capture_test.py")
add_test(NAME package_runtime_provenance COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/package_runtime_provenance_test.py")
# PC-side shell code calls only tools that Git for Windows ships (no grep, flock, pgrep ...).
add_test(NAME windows_tool_lint COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/windows_tool_lint_test.py")
# Inspect the actual Windows executables without launching them: a console
# subsystem can create a visible Windows Terminal even when SDL is hidden.
if(P4_WIN32)
  _add_test(NAME windows_game_subsystem COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/windows_game_subsystem_test.py"
    $<TARGET_FILE:p4pc> $<TARGET_FILE:p4pc_viewer>)
  # exe-data: the test reads the PE headers of its .exe arguments and never runs them (tools/windows_test_lint.py).
  set_tests_properties(windows_game_subsystem PROPERTIES LABELS "tools;exe-data" TIMEOUT 60)
endif()
if(TARGET p4pc64w)
  _add_test(NAME windows_game_subsystem64w COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/windows_game_subsystem_test.py"
    $<TARGET_FILE:p4pc64w> $<TARGET_FILE:p4pc_viewer64w>)
  set_tests_properties(windows_game_subsystem64w PROPERTIES LABELS "tools;port64win;exe-data" TIMEOUT 60)
endif()
set_tests_properties(windows_tool_lint PROPERTIES LABELS "tools;port64" TIMEOUT 60)
# The Windows GL host tests' Wine commands as native ones (tools/wingl.py: land.yml's wingl job and the PC's local_gates.py --gl).
add_test(NAME wingl COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/wingl_test.py")
set_tests_properties(wingl PROPERTIES LABELS "tools;port64" TIMEOUT 60)
if(CMAKE_CXX_COMPILER_LOADED AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
  set(P4_ADAPTER_GRAPH_COFF_ARGS)
  if(TARGET p4pc64w)
    list(APPEND P4_ADAPTER_GRAPH_COFF_ARGS --coff)
  endif()
  add_test(NAME host_adapter_graph COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/host_adapter_graph_test.py"
    --cmake "${CMAKE_COMMAND}" --cc "${CMAKE_C_COMPILER}" ${P4_ADAPTER_GRAPH_COFF_ARGS})
  set_tests_properties(host_adapter_graph PROPERTIES LABELS "tools;port64" TIMEOUT 180 ENVIRONMENT "P4PC_INPUT=script")
endif()
add_test(NAME profile_report COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/profile_report_test.py")
add_test(NAME crt_guard COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/crt_guard_test.py"
  --clang "${CMAKE_C_COMPILER}" --nm "${P4_CRT_NM}" --objdump "${P4_CRT_OBJDUMP}")
add_test(NAME printf_rewrite COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/printf_rewrite_test.py")
add_test(NAME fn_word_rewrite COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/fn_word_rewrite_test.py")
add_test(NAME null_offset_rewrite COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/null_offset_rewrite_test.py")
add_test(NAME variadic_call_rewrite COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/variadic_call_rewrite_test.py")

# EE 64-bit integer results live entirely in $v0, never a $v0:$v1 pair.
if(TARGET ovdiff)
  add_test(NAME ovdiff_ee_abi COMMAND $<TARGET_FILE:ovdiff> --selftest-abi)
  set_tests_properties(ovdiff_ee_abi PROPERTIES ENVIRONMENT "P4_MIPSDIFF_DIR=${CMAKE_SOURCE_DIR}/tools/mipsdiff")
  add_test(NAME ovdiff_ee_lanes COMMAND $<TARGET_FILE:ovdiff> --selftest-lanes)
  set_tests_properties(ovdiff_ee_lanes PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME ovdiff_synthetic_words COMMAND $<TARGET_FILE:ovdiff> --selftest-synthetic-words)
  add_test(NAME mipsdiff_replay_oracle COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mipsdiff_replay_test.py" $<TARGET_FILE:ovdiff>)
  set_tests_properties(mipsdiff_replay_oracle PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME mipsdiff_windows COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mipsdiff_window_test.py" $<TARGET_FILE:ovdiff>)
  set_tests_properties(mipsdiff_windows PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(TARGET ovdiff64)
  add_test(NAME ovdiff_ee_abi64 COMMAND $<TARGET_FILE:ovdiff64> --selftest-abi)
  set_tests_properties(ovdiff_ee_abi64 PROPERTIES SKIP_RETURN_CODE 77 LABELS port64 ENVIRONMENT "P4_MIPSDIFF_DIR=${CMAKE_SOURCE_DIR}/tools/mipsdiff")
  add_test(NAME ovdiff_ee_lanes64 COMMAND $<TARGET_FILE:ovdiff64> --selftest-lanes)
  set_tests_properties(ovdiff_ee_lanes64 PROPERTIES SKIP_RETURN_CODE 77 LABELS port64)
  add_test(NAME ovdiff_synthetic_words64 COMMAND $<TARGET_FILE:ovdiff64> --selftest-synthetic-words)
  set_tests_properties(ovdiff_synthetic_words64 PROPERTIES LABELS port64)
  add_test(NAME mipsdiff_replay_oracle64 COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mipsdiff_replay_test.py" $<TARGET_FILE:ovdiff64>)
  set_tests_properties(mipsdiff_replay_oracle64 PROPERTIES SKIP_RETURN_CODE 77 LABELS port64)
  add_test(NAME mipsdiff_windows64 COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mipsdiff_window_test.py" $<TARGET_FILE:ovdiff64>)
  set_tests_properties(mipsdiff_windows64 PROPERTIES SKIP_RETURN_CODE 77 LABELS port64)
endif()
add_test(NAME asm_worklist COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/asm_worklist_test.py")
set_tests_properties(asm_worklist PROPERTIES LABELS port64)
add_test(NAME mipsdiff_sigs COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mipsdiff_sigs_test.py")
set_tests_properties(mipsdiff_sigs PROPERTIES LABELS port64)
add_test(NAME models_export COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/models_export_test.py")
set_tests_properties(models_export PROPERTIES LABELS port64)
# The runtime glTF model cooker (src/platform/models) behind [resource-model]: models_roundtrip cooks synthetic
# models and, with the disc (P4PC_ISO), exported retail weapons and props through this driver and compares
# the cooked model with the RMD it came from; it also checks the refusals and the content-hash cache.
add_executable(model_cook_cli model_cook_cli.c "${CMAKE_SOURCE_DIR}/src/platform/models/p4model_cook.c"
  "${CMAKE_SOURCE_DIR}/src/platform/models/p4model_cache.c" "${CMAKE_SOURCE_DIR}/src/platform/models/p4model_stream.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/third_party/lodepng/lodepng.c" "${P4_OS_SRC}")
target_include_directories(model_cook_cli PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(model_cook_cli PRIVATE ${P4_PORT_DEFINES} LODEPNG_NO_COMPILE_DISK LODEPNG_NO_COMPILE_ENCODER
  LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS LODEPNG_NO_COMPILE_ALLOCATORS)
target_link_libraries(model_cook_cli m)
# The port64 scope builds p4_x64, not the default all target. Use its
# architecture-specific cooker and include it in that aggregate.
set(_p4_model_target model_cook_cli)
if(P4_PORT64_LINK AND P4_PORT64_OBJS)
  p4_host64_exe(model_cook_cli 64)
  set(_p4_model_target model_cook_cli64)
endif()
get_target_property(_p4_model_runner ${_p4_model_target} CROSSCOMPILING_EMULATOR)
if(NOT _p4_model_runner)
  unset(_p4_model_runner)
endif()
add_test(NAME models_roundtrip COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/models_roundtrip_test.py"
  --runner ${_p4_model_runner} "$<TARGET_FILE:${_p4_model_target}>" --)
unset(_p4_model_runner)
unset(_p4_model_target)
set_tests_properties(models_roundtrip PROPERTIES LABELS port64)

get_property(P4_PLAIN_TESTS GLOBAL PROPERTY P4_PLAIN_TESTS)
foreach(t IN LISTS P4_PLAIN_TESTS)
  set_property(TEST ${t} APPEND PROPERTY LABELS port64)
endforeach()

