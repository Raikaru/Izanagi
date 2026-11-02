add_library(p4decomp OBJECT ${P4_UNIT_SRCS})
# m2c types some pointer slots as s32 (e.g. a heap pointer stored in a word) and some callback
# slots with a pointer parameter where the callee takes an int. On this 32-bit target the widths
# match, so these conversions are accepted for the mirror only. A bare `return;` in a non-void
# function (the value stays in the return register, as under MWCC) is accepted likewise.
target_compile_options(p4decomp PRIVATE -malign-double -Wno-int-conversion -Wno-incompatible-function-pointer-types
  -Wno-return-mismatch)
# The libc names the decomp calls are renamed to their retail functions for decomp units only
# (include/p4pc/libc_rename.h). Host and port code keep glibc.
target_compile_options(p4decomp PRIVATE -DP4_DECOMP_UNIT=1)
# 128-bit copies (`s128`, `u_long128`: the EE's lq/sq pair) are 16-byte aligned types, so at -O2 clang copies
# them with movaps. The retail frames and arrays are 16-aligned (MWCC aligned them), but the m2c-typed locals here
# (`f32 spE0[16]` copied through an `s128 *`) are only 4-aligned: movaps then faults (read of address
# 0xffffffff on Windows) at -O2 and was hidden at -O0, where the copies were memcpy calls. No implicit SIMD:
# aggregate copies use integer moves, which have no alignment requirement and give the same bytes.
target_compile_options(p4decomp PRIVATE -mno-implicit-float)

