/* Force-included into the p4decomp translation units only (CMakeLists.txt).
 *
 * The decomp calls these libc names, and retail binds each one to a guest
 * function (the addresses are the retail ELF's). A host libc pointer must never
 * reach the guest heap, and guest state (strtok's save pointer) must stay in
 * the guest, so each name is renamed to its retail target. Native game-facing
 * callers use the same cores. Only reviewed host-adapter caller/symbol pairs
 * retain host libc; tools/crt_guard.py rejects every other host CRT reference.
 *
 * The heap, string, comparison, conversion, vsprintf and qsort aliases below
 * name their retail entry points. Allocation uses the literal retail EE
 * newlib heap behind the original EE-width boundary. */
#ifndef P4PC_LIBC_RENAME_H
#define P4PC_LIBC_RENAME_H

#define malloc  func_0043ece8
#define free    func_0043ed08
#define realloc func_00441558
#define calloc  func_0043c710
/* Host-equivalent byte operations still enter through EE-width functions.
 * A native size_t callback is not the game's four-byte size_t contract. */
#define memcpy  func_0043f810
#define memmove func_0043f8c0
#define memset  func_0043f9c8
#define strlen  func_00442948
#define strncpy func_00442de8
#define strncat func_00442a80
#define strstr  func_00443010
#define strtok  func_00443f18
#define vsprintf func_00446ed8
#define sprintf func_00442088
#define printf func_00440b68
#define vprintf func_00446e88
#define sscanf func_00442100
/* These are matched EE SDK functions, not POSIX struct-stat/process APIs.
 * Keep their game bodies out of the host symbol namespace as well. */
#define fstat func_00421f98
#define kill func_00421fb8

/* The EE conversions use 64-bit accumulation; atoi keeps the low 32 bits.
 * Comparisons return the exact unsigned-byte difference, not only its sign. */
#define atoi func_0043c6b0
#define memcmp func_0043f778
#define strcmp func_004426e8
#define strncmp func_00442c30

/* Equal-key order affects render-time effect RNG consumption. Use retail's
 * partition/insertion order, not the platform CRT's permitted permutation. */
#define qsort func_00440bb8

/* The float libm: retail's cores (src/platform/ee_libm.c, translated from the asm) and not the host's. glibc's sinf under the EE's round-toward-zero MXCSR
 * returns 0.99999994 for sin(pi/2): the "Living Room" caption of the field (FldPanel2dUpdate: `if (!(sinf(x) < 1.0f))`) never opened. */
/* glibc's math.h pastes function names into __DECL_SIMD_<name> after expanding them: give the renamed names the empty declaration it expects. */
#define __DECL_SIMD_func_0044b7b0
#define __DECL_SIMD_func_0044b610
#define __DECL_SIMD_func_0044b868
#define __DECL_SIMD_func_0044b340
#define __DECL_SIMD_func_0044b6c0
#define sinf  func_0044b7b0
#define cosf  func_0044b610
#define tanf  func_0044b868
#define atanf func_0044b340
#define fabsf func_0044b6c0
#define sqrtf func_0044a4d0
#define __ieee754_sqrtf func_0044a4d0

/* RenderWare's double sin, cos and atan2 (RwSin/RwCos/RwATan2 expand to them here) are retail's float cores too: retail has no double versions
 * (RwMatrixRotate calls sinf and cosf), and glibc's under round-toward-zero returned a NaN for cos(98 degrees) that the EE add read as FLT_MAX. */
#define __DECL_SIMD_p4pc_rw_sin
#define __DECL_SIMD_p4pc_rw_sinf
#define __DECL_SIMD_p4pc_rw_sinl
#define __DECL_SIMD_p4pc_rw_sinf16
#define __DECL_SIMD_p4pc_rw_sinf32
#define __DECL_SIMD_p4pc_rw_sinf64
#define __DECL_SIMD_p4pc_rw_sinf128
#define __DECL_SIMD_p4pc_rw_sinf32x
#define __DECL_SIMD_p4pc_rw_sinf64x
#define __DECL_SIMD_p4pc_rw_cos
#define __DECL_SIMD_p4pc_rw_cosf
#define __DECL_SIMD_p4pc_rw_cosl
#define __DECL_SIMD_p4pc_rw_cosf16
#define __DECL_SIMD_p4pc_rw_cosf32
#define __DECL_SIMD_p4pc_rw_cosf64
#define __DECL_SIMD_p4pc_rw_cosf128
#define __DECL_SIMD_p4pc_rw_cosf32x
#define __DECL_SIMD_p4pc_rw_cosf64x
#define __DECL_SIMD_p4pc_rw_atan2
#define __DECL_SIMD_p4pc_rw_atan2f
#define __DECL_SIMD_p4pc_rw_atan2l
#define __DECL_SIMD_p4pc_rw_atan2f16
#define __DECL_SIMD_p4pc_rw_atan2f32
#define __DECL_SIMD_p4pc_rw_atan2f64
#define __DECL_SIMD_p4pc_rw_atan2f128
#define __DECL_SIMD_p4pc_rw_atan2f32x
#define __DECL_SIMD_p4pc_rw_atan2f64x
#define sin   p4pc_rw_sin
#define cos   p4pc_rw_cos
#define atan2 p4pc_rw_atan2

/* RenderWare's squared-length roots use the retail integer sqrtf core, whose
 * nearest result on these nonnegative normal/zero inputs is the EE sqrt.s
 * result (RwV3dNormalize, 003e40b0). The generic RW approximation could produce
 * length 1.00007 and an out-of-domain heading acos. Disable those tables; the
 * EE launcher supplies the reciprocal division's nearest rounding. */
extern float sqrtf(float);
#define rwSqrtMacro(_root, _input) (*(_root) = sqrtf(_input))
#define rwInvSqrtMacro(_recip, _input) (*(_recip) = 1.0f / sqrtf(_input))
#define rwSqrtInvSqrtMacro(_root, _recip, _input) \
    do { \
        float p4_len = sqrtf(_input); \
        *(_root) = p4_len; \
        *(_recip) = (p4_len > 0.0f) ? 1.0f / p4_len : p4_len; \
    } while (0)

/* Double pow/log/sqrt run the literal retail cores in ee_dlibm.c through
 * ee_dfloat.c's register-class adapters. Their operations are software-double
 * fp-bit arithmetic, including truncation, FTZ and retail NaN propagation. */
#define __DECL_SIMD_p4pc_ee_pow
#define __DECL_SIMD_p4pc_ee_powf
#define __DECL_SIMD_p4pc_ee_powl
#define __DECL_SIMD_p4pc_ee_powf16
#define __DECL_SIMD_p4pc_ee_powf32
#define __DECL_SIMD_p4pc_ee_powf64
#define __DECL_SIMD_p4pc_ee_powf128
#define __DECL_SIMD_p4pc_ee_powf32x
#define __DECL_SIMD_p4pc_ee_powf64x
#define __DECL_SIMD_p4pc_ee_log
#define __DECL_SIMD_p4pc_ee_logf
#define __DECL_SIMD_p4pc_ee_logl
#define __DECL_SIMD_p4pc_ee_logf16
#define __DECL_SIMD_p4pc_ee_logf32
#define __DECL_SIMD_p4pc_ee_logf64
#define __DECL_SIMD_p4pc_ee_logf128
#define __DECL_SIMD_p4pc_ee_logf32x
#define __DECL_SIMD_p4pc_ee_logf64x
#define pow p4pc_ee_pow
#define log p4pc_ee_log
#define sqrt p4pc_ee_dsqrt

#endif /* P4PC_LIBC_RENAME_H */
