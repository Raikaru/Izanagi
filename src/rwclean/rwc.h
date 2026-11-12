/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: shared declarations (docs/rw_clean.md).
 *
 * Original code. It implements the RenderWare Graphics 3.7 C API the game calls, written from the API's observable
 * contract (the game's call sites, the retail program run as a black box in the R5900 interpreter, recorded
 * behaviour), never from the Criterion SDK source. Struct layouts and retail addresses are interoperability
 * facts: the game reads these objects at fixed offsets, so every layout here is asserted.
 *
 * The units are compiled as mirror units (tools/gen_port.py, config/rw_clean.json): the EE float rewrite gives plain
 * float arithmetic the EE FPU's add/sub/div/sqrt, the 64-bit build stores pointers in guest structs as guest words, and a
 * `// FUN_xxxxxxxx` marker binds a definition to its retail address. */
#ifndef RWC_H
#define RWC_H

typedef int RwInt32;
typedef unsigned int RwUInt32;
typedef short RwInt16;
typedef unsigned short RwUInt16;
typedef unsigned char RwUInt8;
typedef float RwReal;
typedef int RwBool;

#define RWC_TRUE 1
#define RWC_FALSE 0

typedef struct RwV2d { RwReal x, y; } RwV2d;
typedef struct RwV3d { RwReal x, y, z; } RwV3d;

/* A 4x4 matrix as four rows of three reals; the fourth word of the first row holds the type flags, the other three
 * are padding the game leaves uninitialised. */
typedef struct RwMatrix {
    RwV3d right;
    RwUInt32 flags;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    RwV3d pos;
    RwUInt32 pad3;
} __attribute__((aligned(16))) RwMatrix;

_Static_assert(sizeof(RwMatrix) == 64, "RwMatrix is 64 bytes");
_Static_assert(__builtin_offsetof(RwMatrix, flags) == 12, "RwMatrix flags");
_Static_assert(__builtin_offsetof(RwMatrix, pos) == 48, "RwMatrix pos");

/* Matrix type flags. */
#define RWC_MATRIX_NORMAL 0x00000001u      /* the three axes have unit length */
#define RWC_MATRIX_ORTHOGONAL 0x00000002u  /* the three axes are mutually perpendicular */
#define RWC_MATRIX_ORTHONORMAL 0x00000003u
#define RWC_MATRIX_TYPEMASK 0x00000003u
#define RWC_MATRIX_IDENTITY 0x00020000u    /* the matrix is the identity */

/* How a transformation combines with a matrix. */
enum { RWC_COMBINE_REPLACE = 0, RWC_COMBINE_PRECONCAT = 1, RWC_COMBINE_POSTCONCAT = 2 };

typedef struct RwBBox { RwV3d sup, inf; } RwBBox;

/* An engine module's bookkeeping: where its globals sit inside the engine block and how many opens are outstanding. */
typedef struct RwcModuleInfo {
    RwInt32 globalsOffset;
    RwInt32 numInstances;
} RwcModuleInfo;

/* The engine's globals block (retail 0x008872E0); a module's globals are at block + globalsOffset. */
extern RwUInt8 D_008872E0[];
#define RWC_MODULE_GLOBALS(T, module) ((T *)(D_008872E0 + (module).globalsOffset))

/* The engine's allocator slots (retail 0x008873E8..0x008873FC): malloc, free, realloc, calloc, then the free-list
 * allocate and free entries. Each takes the RenderWare allocation hint as its last argument. */
typedef void *(*RwcFreeListAllocFn)(void *freeList, RwUInt32 hint);
typedef RwBool (*RwcFreeListFreeFn)(void *freeList, void *block);
extern RwcFreeListAllocFn D_008873F8;
extern RwcFreeListFreeFn D_008873FC;

/* Error reporting: _rwerror (retail 0x003df590) turns a core error code into the value RwErrorSet (0x003df4d0) records
 * with the plugin id 1 (the core). */
typedef struct RwError { RwInt32 pluginID; RwInt32 errorCode; } RwError;
RwInt32 _rwerror(RwInt32 code, ...);
RwError *RwErrorSet(RwError *code);

/* The retail float libm (include/p4pc/libc_rename.h binds these names to the retail cores). */
float sinf(float x);
float cosf(float x);

/* A real's bit pattern (retail tests some reals as words: a +0 and a -0 behave differently). */
static inline RwUInt32 rwcBits(RwReal x)
{
    union {
        RwReal f;
        RwUInt32 u;
    } c;

    c.f = x;
    return c.u;
}

/* EE/VU0 arithmetic for code the retail program runs on VU0 macro instructions (the EE FPU's own arithmetic is what
 * plain float code gets from the mirror's EE float rewrite): an operand with a zero exponent reads as a signed zero and
 * one with an all-ones exponent as the signed largest float (the VU has no infinities or NaNs); a product is the exact
 * product truncated toward zero; an accumulate adds the truncated product to the accumulator exactly and truncates the
 * sum (MXCSR rounds toward zero); results clamp to +-FLT_MAX and denormal results are zero. */
static inline double rwcVuIn(RwReal x)
{
    union {
        float f;
        unsigned int u;
    } c;

    c.f = x;
    if (((c.u >> 23) & 255u) == 0)
        return (c.u & 0x80000000u) ? -0.0 : 0.0;
    if (((c.u >> 23) & 255u) == 255u)
        return (c.u & 0x80000000u) ? -3.4028234663852886e38 : 3.4028234663852886e38;
    return (double)x;
}

static inline RwReal rwcVuMul(RwReal a, RwReal b)
{
    return p4_vu0_fixup((float)(rwcVuIn(a) * rwcVuIn(b)));
}

static inline RwReal rwcVuMadd(RwReal acc, RwReal a, RwReal b)
{
    return p4_vu0_fixup((float)(rwcVuIn(acc) + (double)rwcVuMul(a, b)));
}

static inline RwReal rwcVuAdd(RwReal a, RwReal b)
{
    return p4_vu0_fixup((float)(rwcVuIn(a) + rwcVuIn(b)));
}

static inline RwReal rwcVuSub(RwReal a, RwReal b)
{
    return p4_vu0_fixup((float)(rwcVuIn(a) - rwcVuIn(b)));
}

#endif
