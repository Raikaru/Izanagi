# Overrides

Address-keyed C replacements for first-party functions that use PS2 inline asm
(VU0 `lqc2`/`sqc2`/`vmul*`/`vftoi*`, MMI `ppach`/`pextlb`, COP2 `qmtc2`/`qmfc2`).
The PC build replaces each asm-bearing decomp function with the file here, so the
port carries no `__asm__` for these functions.

## Convention

- File name is the lowercase function address: `overrides/<addr>.c`, e.g. `004abe80.c`.
- One replacement definition per file, first line `#include "p4pc/vu0.h"`.
- No marker line and no `#pragma` lines. The decomp's `#pragma push/opt_*/pop` scoping is dropped.
- Static helpers that the decomp defines `static inline` live in `overrides/static/<name>.c`.
  They are spliced over the header or TU definition with the same name.
- The helpers in `include/p4pc/vu0.h` model the VU0 pipeline on one shared register
  file (`p4pc_vu0`, defined in `src/vu0.c`). `vf0` is fixed at (0,0,0,1).
- Results are rounded to nearest float, clamped to +-FLT_MAX, NaN becomes 0 and denormals
  flush to 0. This is the only numeric deviation from the VU (at most one ulp per op).

## Equivalence checks

`tests/vu0_helpers_test.c` checks the helpers against scalar references
(mask and broadcast handling, fixup, `vf0` writes, Q/`vi[22]`, `opmula`,
colour pack). Build and run it through CTest after `add_subdirectory(tests)`.

## Out of scope: do not override these

These are not VU0 compute. Do not write overrides for them. They need a separate
decision (syscall or kernel layer).

- The 163 `asm void` syscall stubs (the `asm void` bodies, not VU0 compute).
- COP0 and kernel-only rows:
  - `FldShadow_PollIntcBit13` 001026b0
  - `tmxDmaCopy` 00463ea0
- Promoted `code1_0042.c` rows: 00421c70, 00421d00, 00422460, 00422480, 004224a0, 004224c0.

## Platform overrides

These are not VU0 compute either. They are the port's answer for PS2 kernel and
interrupt code, and each one is a decision, not a translation.

- `00100310.c` (`kwlnHandleTimerInterrupt`, interrupt enable `sync; ei` plus an INTC mask write): no-op returning 0. The port has no interrupt controller.
- `00100220.c` (`kwlnExitThread`, syscall 0x23): traps by name through `p4pc_trap`. The decomp has no caller and the kernel service has no port equivalent.
- `00100230.c` (`kwlnProbeHeapBudget`, boot heap base just below 0x02000000): creates the port heap arena `[0x00938A00, 0x02000000 - 0xA0000)` through `p4pc_heap_init`, then keeps the retail probe (`malloc(base - 16)` and `free`) and returns `base - 16`. The decomp's `&D_A0000` and `&D_938A00` are outside the retail image, so they are literal values here.
- `00100e30.c` (`kwlnEnableInterrupts`, callback that runs `sync; ei` after `func_004ccb38(0)`): the same call without the `sync; ei` pair. The port has no interrupt controller. The inline asm would otherwise exclude the whole kwln unit.
- `00100350.c` (`kwlnInitIop`, IOP reset, module loads and RPC binding): calls `p4pc_platform_init()` (`src/platform/kernel_init.c`), which keeps the EE side (TIMER0 INTC callback, timer registers). The port has no IOP, so the retail retry loops would never end.
- The malloc/free/calloc/realloc family is port-owned in `src/platform/kernel_heap.c` and `ee_heap.c`, hand-translated from retail EE newlib with four-byte guest chunk/bin words and original split/coalesce/reuse behavior. The excluded `code1_0043` definitions therefore need no marker override. RW uses the matched `0044f510` initializer and original checked wrappers, not a plain host heap; see the [CRT boundary and original-ELF heap oracle](../docs/platform/crt_boundary.md#allocator-boundary-and-checked-headers).

A `// FUN_<ADDR>` override replaces the definition that follows the marker. Single-line `extern` declarations between the marker and that definition are kept (`tools/gen_port.py`, `apply_overrides`).
