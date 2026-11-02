# Numeric and cross-target determinism

[Platform index](../platform.md) · [Port compilation](../porting.md)

## Floating-point state and library calls

The same source, input and virtual-clock policy must produce the same guest arithmetic
across targets. `p4pc_fpu_check` restores EE MXCSR round-toward-zero, flush-to-zero and
DAZ, with x87 control `0x037f`, after initial GL calls and around present/swap. Drivers
may change that state. The first eight changes log `float state changed by <where>`;
`P4PC_FPU_LOG=1` prints boot/initial-flip/thread-entry state. `fpu_state` deliberately
spoils it and requires restoration on Linux and Wine.

Windows reference fibers share the host thread's float state; `FIBER_FLAG_FLOAT_SWITCH`
must not install a fresh default state on worker startup. Host input quantization
explicitly uses nearest rounding separately from guest arithmetic.

Game-facing libc/libm calls bind through `include/p4pc/libc_rename.h` and the
canonical retail entries, including native CRI callers. Float and raw soft-double
cores live in `src/platform/ee_libm.c` and `ee_dlibm.c`; host CRT is not a blanket
permission for port code. The [CRT boundary](crt_boundary.md) documents every
binding family, object/caller inventories, reviewed host-adapter pairs, the strict
allowlist and the build guard. See also [renderer arithmetic](../renderer_port.md).
`ee_libm` hashes fixed sweeps in EE mode against recorded values. Do not normalize
RNG/matrices/battle words to make cross-target differences disappear.

Decimal input uses `src/platform/ee_strtod.c` (`_strtod_r`, `00443068`) and
`src/platform/ee_scanf.c` (`sscanf`, `_sscanf_r`, `__svfscanf_r`, `00442100`,
`00442188`, `00446108`). The source reference is newlib 1.10.0's Gay/BSD
implementation, with the retail IEEE/Pack_32 layout, 64-bit `long`, and
`FLT_ROUNDS=0` (no nearest-rounding shortcut; `+0.5` ulp-adjustment compensation).
The overlapping 1.9.0 source does not establish a unique release identifier.
The initial approximation and bigint/ulp correction use raw EE double helpers,
not host floating-point arithmetic. Power tables and the bigint freelist/powers
of five cache are shared with dtoa at the original guest addresses.
Native scanner entry points accept native destination pointers in `va_list`;
the scanner core accepts eight-byte guest argument slots containing four-byte
guest destination addresses. `%l` writes eight bytes on both Windows and Linux.
The game's two inventoried input formats are `%lx` and `%lx %lx %lx`;
destination locals retain the retail 64-bit width and narrow only at original uses.
`ee_scanf_retail_oracle` loads the unchanged user-supplied ELF and compares
return bits, destination bytes/canaries, end/consumed counts and guest errno;
it never substitutes native calls inside the retail execution.

## fp-bit ABI

`src/platform/ps2_fpbit.c` implements `__pack_f` (`0044db98`), `__unpack_f` (`0044dc50`),
`__unpack_d` (`0044ddf0`) and `__fpcmp_parts_d` (`0044e508`). They take `P4FpF` / `P4FpD`
struct pointers from `ps2_fpbit.h`, not 64-bit GPR pairs; `__pack_f` returns a float.
The contract is retail, not generic IEEE soft-float:

- Unpack exponent zero is class zero even for nonzero fraction (denormals flush).
- Exponent `0x7ff` with zero fraction is infinity.
- Packing below normal range flushes to zero; INF/NaN packs exponent 255, mantissa `0x7fffff`.
- Double-parts comparison orders class/sign/exponent/fraction, returning 1 if either is NaN.

`fpbit` checks input/output pairs derived from retail instructions. Other 64-bit helper
names in that address range must not inherit this pointer ABI by association.

## State diagnosis and retained oracles

`P4PC_DUMP_RAM=<prefix>` with `P4PC_DUMP_RAM_AT=<flip>,...` captures guest RAM.
`tools/ram_diff.py` finds the first differing non-host-pointer word; a watchpoint then
identifies its writer. Compare hexadecimal matrices, not rounded position text:
`P4PC_WALK_LOG=1 P4PC_WALK_LOG_EVERY=1 P4PC_WALK_LOG_M=1` exposes the actual bits.

`P4PC_WALK_DIGEST=1` rolls the player's matrix and random state into the walk digest.
`walk_digest` (`tests/walk_digest_gate.sh`) and `tools/win_digest.sh <lane>` use the same
22-row golden through flip 6500 and stop at 6600. The floor RNG witness retains goal
flip 7000, seed `0x19c3` and grid checksum `4034924771 2512`. Inputs, limits and these
oracles are not retimed or replaced to accommodate another disc model.
The golden was last re-recorded when RenderWare's vector-multiply slots began running the
sky driver's VU0 routines (`src/platform/rw_skytran.c`). Those routines are bit-exact
with PCSX2 recordings, and they round differently from the generic C providers they
replaced. The 45-degree rotation words around 0x009419AC already differ by one or two
ulps (-0.5 is exact instead of -0.50000006). At flip 3629, the post-concatenated step that
`FldFrameTask_Update` passes to `RwMatrixTranslate` moves the player's x and z by one
ulp. From the 4000 row on, the digests differ. The MT array and every battle-state row
are unchanged.

Arithmetic checks and independent same-walk gameplay are not whole-machine equality.
Use save-anchored lockstep (the private retail-comparison tooling) with exact retained input and
full state for fidelity comparisons. An expected first divergence does not qualify its unverified suffix.
