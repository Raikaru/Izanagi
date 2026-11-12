# Palette quantizer (palquant closure): func_003f07d0 accumulates pixels, func_003f17e0 builds
# the palette, func_003f1ce0 remaps. Each retail override is its own translation unit, so the
# file-static helpers that more than one override defines do not collide. The pool allocator
# and the palquant mirror's externs come from palquant_prelude.h. The mask table and bit count
# are mapped at their retail addresses (p4_os_map_fixed).
set(PALQUANT_OVERRIDES 003f07d0 003f0610 003eff30 003f0110 003f0510 003f0c10 003f0de0
  003f02d0 003f03f0 003f1030 003f1240 003f13b0 003f1520 003f1600 003f1700 003f17e0 003f1ce0)
set(PALQUANT_SRCS palquant_test.c "${P4_OS_SRC}")
foreach(addr IN LISTS PALQUANT_OVERRIDES)
  list(APPEND PALQUANT_SRCS "${CMAKE_SOURCE_DIR}/overrides/${addr}.c")
endforeach()
add_executable(palquant_test ${PALQUANT_SRCS})
add_dependencies(palquant_test p4_mirror)
target_include_directories(palquant_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${P4_GEN}/decomp/include")
target_compile_options(palquant_test PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include palquant_prelude.h>")
target_link_libraries(palquant_test m)
p4_ps2_exe(palquant_test)
add_test(NAME palquant COMMAND palquant_test)

# Promoted leaf overrides (PortPromotedA): each override is its own translation unit, run
# against memory mapped at the retail globals it reads (p4_os_map_fixed).
set(PROMOTED_LEAF_OVERRIDES 001f5680 00396e80 003cc250 003d5710 003b5fb0 003b6390 003b6420 003b64c0
  003b4230 003b65d0 003b6540 003d0790 003cbf30 003ce840 0039a910 003a2770 003a5180
  003b4ab0 003b4c10 003b4ff0 003b5650 003b5ac0 003b5bf0 003b6020 003b61e0)
set(PROMOTED_LEAF_SRCS promoted_leaf_test.c "${P4_OS_SRC}")
foreach(addr IN LISTS PROMOTED_LEAF_OVERRIDES)
  list(APPEND PROMOTED_LEAF_SRCS "${CMAKE_SOURCE_DIR}/overrides/${addr}.c")
endforeach()
add_executable(promoted_leaf_test ${PROMOTED_LEAF_SRCS})
add_dependencies(promoted_leaf_test p4_mirror)
target_include_directories(promoted_leaf_test PRIVATE "${P4_GEN}/decomp/include")
target_compile_options(promoted_leaf_test PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include type.h>")
p4_ps2_exe(promoted_leaf_test)
add_test(NAME promoted_leaf COMMAND promoted_leaf_test)
# RxHeap (retail allocator, RenderWare): create, allocate, free with merges, grow, and the
# out-of-memory error. Each retail override is its own translation unit. The memory function
# table sits at its retail address (p4_os_map_fixed), so the table slots are linker symbols too
# (P4_ABSOLUTE_SYMBOL in the test).
set(RXHEAP_OVERRIDES 00415070 00414ef0 00414930 00414820 00414e10 00414a30 00414b50 00414c10)
set(RXHEAP_SRCS rxheap_test.c "${CMAKE_SOURCE_DIR}/src/platform/rx_heap_realloc.c" "${P4_OS_SRC}")
foreach(addr IN LISTS RXHEAP_OVERRIDES)
  list(APPEND RXHEAP_SRCS "${CMAKE_SOURCE_DIR}/overrides/${addr}.c")
endforeach()
add_executable(rxheap_test ${RXHEAP_SRCS})
add_dependencies(rxheap_test p4_mirror)
target_include_directories(rxheap_test PRIVATE "${P4_GEN}/decomp/include")
target_compile_options(rxheap_test PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include stddef.h>" "$<$<COMPILE_LANGUAGE:C>:SHELL:-include type.h>")
target_link_libraries(rxheap_test m)
p4_ps2_exe(rxheap_test)
add_test(NAME rxheap COMMAND rxheap_test)
set_tests_properties(rxheap PROPERTIES SKIP_RETURN_CODE 77)

