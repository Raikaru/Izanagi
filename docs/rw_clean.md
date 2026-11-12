# RenderWare clean layer

The game calls RenderWare Graphics 3.7 (Rw*/Rp*/Rt*). Today the port compiles Criterion's SDK source as the decomp
carries it (`decomp/src/renderware`, headers in `decomp/include/rw`), the decompiled RenderWare in `decomp/src/rw` and
the RenderWare span of `decomp/src/promoted` (retail 0x0038F990..0x00417510), plus port translations of retail RenderWare
functions (`overrides/` in that span, `src/platform/rw_*.c`). `src/rwclean` replaces all of it, subsystem by subsystem,
with original code (MIT) that keeps the API, the retail data layout and the retail arithmetic.

## Switch

CMake `P4_RW_CLEAN` lists the subsystems to compile from `src/rwclean` (comma-separated ids of `config/rw_clean.json`).
The default is empty: the build is exactly the SDK build. A lane check selects subsystems with
`P4PC_CHECK_ENV="P4PC_RW_CLEAN=math"`; the environment variable wins over the cache at every configure.

For each selected subsystem `tools/gen_port.py`:

- drops its `units` from the compiled unit list (the mirror still holds them, so the reviewed mirror passes and
  `config/mirror_contracts.json` see the default tree);
- copies `src/rwclean` into the mirror after those passes and compiles the subsystem's `sources` as mirror units,
  with the same flags as the code they replace: the EE float rewrite, the 64-bit guest-pointer rewrite, the census;
- strips, from every other compiled unit, the definition after each `// FUN_xxxxxxxx` marker a clean source carries,
  so the clean function is the only one bound to that retail address (fn-pointer words, engine slots, mod hooks and
  ovdiff all reach it);
- leaves its `port_sources` out of `p4port`/`p4portee` (`cmake/sources/150-renderware.cmake`, `290-cri-ee.cmake`).

`gen/rw_clean_units.txt` tells the census check (`tools/port64/unit64.py --rw-clean-units`) which units the switch
governs: an inactive one is not compared, an active clean unit is compared once its baseline row exists. The mirror
lists the other units the stripping changed (`src/rwclean/stripped_units.txt` in the mirror, `--rw-clean-stripped`):
their rule counts follow the switch and are not compared while it is on. A subsystem's rows join the baselines when it
becomes the default; refresh baselines only from a default build.

A subsystem becomes the default when its gates hold (below), and one land later its replaced units, overrides,
CLAIMS rows and port transcriptions leave the tree or the compiled set.

## Clean-room rules

Allowed inputs: function names and signatures, struct sizes and offsets, constants and data addresses (interface facts,
recorded as numbers); the game's and the port's call sites; the retail program run as a black box in the R5900
interpreter (`tools/mipsdiff/r5900.c`, `ovdiff`) and behaviour recorded from the SDK build; public format and API
documentation. Not used while writing clean code: the text of `decomp/src/renderware`, `decomp/include/rw` beyond the
interface facts, `decomp/src/rw`, RenderWare-span bodies in `decomp/src/promoted` and `overrides/`, the
`src/platform/rw_*.c` transcriptions, `tools/rw_draft.sh` output for RenderWare addresses, and librw (MIT, not used:
its host-pointer layouts and float order cannot serve this port; decided with the user).

Each behaviour below was established by running the retail function on generated inputs and comparing its writes and
result with the clean native, byte for byte (a differential harness of the lane's study; the committed check is ovdiff).
Functions whose retail code needs the started engine (module open/close, RwMatrixCreate/Destroy, the transforms that
call through the vector slots) are not in the ovdiff list: the study's i386 harness runs them on the retail engine's
memory after RwEngineStart, with the native's free-list slots bridged to the retail free-list code, so both sides share
one heap.

## Gates per subsystem (switch on)

1. ovdiff exact on every clean function with a retail address; recorded-behaviour tests for internals without one.
2. Walk digest, scenes/scenes64, dungeon digests, battle-sim traces, lockstep (collision 900, the Teddie route).
3. Linux x64, Windows x64, Windows i386, Android arm64 and macOS builds.

## Subsystems

### math (phase 1)

Replaces `bavector.c`, `bamatrix.c`, `babinmtx.c`, `bacolor.c`, `babbox.c`, the RtQuat functions the port compiled
from `overrides/` (0x003dc490, 0x003dc510, 0x003dc590, 0x003dc610, 0x003dc740, 0x003dcb40) and retires
`rw_normalize2.c`, `rw_normalize3.c`, `rw_skytran.c`. Sources: `rwc_vector.c`, `rwc_matrix.c`, `rwc_vu0.c`,
`rwc_bbox.c`, `rwc_color.c`, `rwc_quat.c`.

Recorded behaviour:

- Module globals: vector module (offset at 0x00764890) = two zero words, then the point, points, vector and vectors
  providers; matrix module (offset at 0x00764858) = the free list (descriptor at 0x008871C0: 64-byte entries, 256 per
  block, 16-aligned, one block preallocated, hint 0x4000d), a word set to 0x20000, the multiply provider, and the
  normal, orthogonal and identity tolerances (0.01). The colour module only counts instances (0x0076484C).
- The driver's VU0 providers (0x00402390..0x00402570): each lane accumulates right * x, up * y, at * z (+ pos for a
  point) in VU0 arithmetic. VU0 operands: a zero exponent reads as a signed zero, an all-ones exponent as the signed
  largest float (no infinities or NaNs).
- RwMatrixMultiply (VU0 arithmetic, flags = a & b). The default provider MatrixMultiply uses the EE FPU.
- VU0 registers are game state: code that runs later stores lanes it never set, so the clean routines leave the port's
  register file (`p4pc_vu0`) as retail's do. A product (the provider and RwMatrixMultiply, so also the pre/post forms of
  the rotations and RwMatrixTransform) loads the left rows into vf1-vf4 and the right rows into vf5-vf8 (all four
  words), leaves the result rows in vf9-vf12 (x, y, z) and the position row's sum before the right position in ACC
  (x, y, z), and stores whole rows: the destination's padding words get the w words vf10-vf12 held before. The provider
  also loads the destination's first row into vf13 and keeps its flags word (vf9.w). A transform loads the rows into
  vf1-vf4 (vf1-vf3 for vectors) and computes a single input in vf5 (vf4) loaded with w = 0, ACC holding the sum before
  the last term; the array forms leave the last result in vf6 (x, y, z) and the input after the last one in vf5 (they
  load ahead, so they read one vector past the array). The switch-off port did this; ignoring it changed the w words
  of frame LTMs and the player matrix's padding in a collision 900 replay.
- Lengths use sqrt.s on y*y + x*x (+ z*z); a normalise scales by the reciprocal length, writes zeros for a zero vector,
  and RwV3dNormalize/RwV2dNormalize report core error 0x19 for it; 0x003e4030 returns the reciprocal and reports nothing.
- RwMatrixOptimize: an error equal to its tolerance passes; the identity is tested only for an orthonormal matrix. The
  orthogonal error adds its last two squares first.
- RwMatrixOrthoNormalize keeps the first axis of the most nearly perpendicular cyclic pair of unit axes (ties: the
  first pair, then at-right; a zero axis makes the axis after it the kept one) and rebuilds the other two by unit cross
  products.
- MatrixInvertGeneric: adjugate (negated minors at odd positions) times the reciprocal of the determinant expanded down
  the first column; only a determinant of +0 leaves it unscaled (retail tests the word: -0 scales by 1/-0 = -FLT_MAX);
  flags cleared. RwMatrixInvert copies an identity whole and transposes an orthonormal matrix (flags 3). dst must not
  be src (retail computes in place in an unspecified order).
- Rotations: diagonal 1 - (1 - cos)(1 - a_k^2), off-diagonal (1 - cos)(a_i a_j) +- sin a_k; RwMatrixRotate normalises the
  axis and converts degrees with one multiply by 0.0174532924. Retail's replaced matrix leaves its last padding word as
  stack garbage; the clean one clears it.
- RwMatrixCreate writes only the three diagonal ones and the flags (0x20003); the rest is what the free list returns.
- RtQuatConvertFromMatrix: trace (right.x + up.y) + at.z > 0 gives real = s/2 with s = sqrt(trace + 1) and each
  imaginary part a difference of opposite elements times 0.5/s; otherwise the largest diagonal element (a later axis
  wins a tie) selects one of three retail helpers, each sqrt((m_ii - (m_jj + m_kk)) + 1). It returns TRUE, or FALSE
  with nothing written for a NULL quaternion or matrix.
- RtQuatTransformVectors: (w^2 - u.u) v + (2 (u.v) u + 2w (u x v)), so a quaternion that is not unit also scales.
- RtQuatRotate: half angle = angle * pi/360 (one multiply), sin and cos of it, the normalised axis times the sine;
  preconcatenation is q * r, postconcatenation r * q. A NULL quaternion or axis reports error 0x80000016, then
  another combine value error 0x80000003 (plugin 0x195 for both); each returns NULL before any work.
- The lanes of a combined quaternion's imaginary part, of a quaternion-transformed vector and of the default vector
  providers' results are never -0 (a zero is +0); every other function keeps the IEEE sign of a zero.

Every behaviour above was also checked with a quarter of the input words +-0 (the study's zero-sign stress).

Known interpreter limits (not differences of the clean code): retail sinf/cosf in the interpreter differ from the
port's PCSX2-exact libm, so RwMatrixRotate and RtQuatRotate are checked through their parts (the native gets the
interpreter's sine and cosine and must pass them the same argument). The interpreter's VU0 sums are exact since this
subsystem's check found them rounding operands far apart in exponent (docs/override_diff.md, "VU0 sums").
