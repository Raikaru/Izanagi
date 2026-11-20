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

`P4PC_WALK_DIGEST=1` rolls the player's matrix and random state into the walk
digest. Arithmetic checks and same-walk gameplay comparisons do not establish
whole-machine equality. A first divergence does not validate the untested suffix.

## Dungeon floors across targets

`tests/dungeon_digest_gate.sh` holds a completed walk over generated floors to a digest that
the Windows x64 and the Linux x64 builds both recorded (`tests/dungeon_digests/<walk>.txt`).
`P4PC_DUNGEON_DIGEST=1` (`p4walk_dungeon_digest`, `src/platform/pad/walk_script.c`) prints a
row every 100 flips with separate hashes of the field ids, dungeon step and floor seed, the
floor's placed cells, the additive random generator, RpRandom's MT array, the player's
matrix, the floor's units (ids and model matrices) and the battle state, plus a rolling hash
of the player's matrix and the additive generator over every flip. A failure prints the
first differing row and names its differing components. Pointer words are never hashed: the
64-bit port half's heap blocks do not sit at the same guest addresses on the two targets (at
flip 1000 of the Marukyu walk the word at 0x0071669C is 0x02433B70 on Windows and 0x02433C90
on Linux), while the data they lead to is equal. Nor are RwMatrix's three pad words: the game
leaves them uninitialised. `btlUnitUpdateAll` (0019c0d0) builds each battle unit's matrix on
the stack, sets rows, position and flags as retail does (`swc1`/`sw` to sp+0x60..0x98, never
0x7c/0x8c/0x9c) and copies all 64 bytes, so the pads hold each host's stack garbage (the hero's
battle matrix at 0x01153AF0 holds `0 / 3 / 0x00007FF7` in them on Windows, `3 / 0 / 0` on Linux,
with equal rows); other matrices' pads hold host garbage too (0x00940CDC at flip 6000:
0x00007FF6 on Windows, 0x00007F6E on Linux). Compare equivalent builds and inputs
when investigating cross-target differences.

Host code that turns a script into input must not depend on the C library's rounding. In the
EE mode (MXCSR round toward zero, x87 nearest) the Windows CRT's `strtof`/`strtod` truncate
while glibc's round to nearest: 99,316 of 200,000 decimal fractions parsed one ulp apart, none
in round-to-nearest. Host `sinf`/`cosf` differ in the last place for about one input in eighty
in either mode; `hypotf` and `sqrtf` agreed on all 2,000,000 samples. The walker therefore
parses script numbers in round-to-nearest, as do `P4PC_AUTOPRESS` times and the title
driver's origin, and `face:DEG` takes its direction from basic operations only.
