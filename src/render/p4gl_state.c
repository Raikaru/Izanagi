/* RenderWare render-state set/get for the OpenGL device (device block +0x10 / +0x14, see
 * src/render/p4gl.h). The core calls fpRenderStateSet(state, value) with the value passed
 * by value, and fpRenderStateGet(state, out) with a pointer that receives the value.
 * Values are recorded per state id; the GL draw path reads them when it draws. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "p4pc/lockstep.h"

/* RwRenderState ids 0..28 (decomp/include/rw/sky2/rwplcore.h:5162-5279). The core passes
 * RW ids straight to the device (baimmedi.c RwRenderStateSetMacro). */
#define P4GL_RENDERSTATE_COUNT 29u

/* Render state the game never set reads 0, which is not "off": the defaults differ per path (see
 * p4gl_render_state_or). Im3D and PTank draw with depth test on and depth write off until the game
 * sets them (sky2hook measured that against PCSX2); atomics default both on. */
static uint32_t sRenderState[P4GL_RENDERSTATE_COUNT];
static uint32_t sRenderStateHit;   /* bit per id the game has set */

/* fpRenderStateSet: returns 1 for a known state id, 0 otherwise. The value is a 32-bit register word: an integer
 * (filter, address mode, a flag) or the guest address of a raster, never converted. */
int32_t func_003f5070(int32_t state, uint32_t value)
{
    if ((uint32_t)state >= P4GL_RENDERSTATE_COUNT) {
        return 0;
    }
    sRenderState[state] = value;
    sRenderStateHit |= 1u << (unsigned)state;
    return 1;
}

/* fpRenderStateGet: writes the recorded value through out. Returns 1 for a known state id. */
int32_t func_003f5d90(int32_t state, void *out)
{
    if ((uint32_t)state >= P4GL_RENDERSTATE_COUNT || out == NULL) {
        return 0;
    }
    *(uint32_t *)out = sRenderState[state];
    return 1;
}

/* Current value of a render state id, 0 if unset. The GL draw path reads texture and
 * filter state through this. */
uint32_t p4gl_render_state(uint32_t state)
{
    if (state >= P4GL_RENDERSTATE_COUNT) {
        return 0;
    }
    return sRenderState[state];
}

uint32_t p4gl_render_state_written(void) { return sRenderStateHit; }
void p4gl_render_state_restore(uint32_t state, uint32_t value, uint32_t written_mask)
{
    if (!func_003f5070((int32_t)state, value)) return;
    const uint32_t bit = 1u << state;
    sRenderStateHit = (sRenderStateHit & ~bit) | (written_mask & bit);
}

/* The value the game set for a state id, else dflt (a state it never set reads 0, which is not
 * "off": RenderWare's defaults differ per state). */
uint32_t p4gl_render_state_or(uint32_t state, uint32_t dflt)
{
    if (state >= P4GL_RENDERSTATE_COUNT || (sRenderStateHit & (1u << state)) == 0u) {
        return dflt;
    }
    return sRenderState[state];
}

/* Retail globals written by RpSkySelectDeepZBuffer (basky.c). The core reads D_008872F8
 * as the depth scale and iGpffffb958 as the sky driver's disable flag. */
#define iGpffffac00 P4_GPDATA(int32_t, 0xac00)
#define iGpffffb958 P4_GPDATA(int32_t, 0xb958)
#define D_008872F8 P4_GARRAY(int32_t, 0x008872F8u)

/* RpSkySelectDeepZBuffer: arg0 != 0 selects the 24-bit deep depth scale, 0 the 16-bit one.
 * Returns 0 while iGpffffb958 is set, else 1. Same bits as retail (basky.c:281). */
int32_t RpSkySelectDeepZBuffer(int32_t arg0)
{
    if (iGpffffb958 != 0) {
        return 0;
    }
    if (arg0 != 0) {
        iGpffffac00 = 0x20;
        D_008872F8[0] = 0x4B7FFFFF;
    } else {
        iGpffffac00 = 0x10;
        D_008872F8[0] = 0x477FFF00;
    }
    return 1;
}

/* RpSky render state: retail RpSkyRenderStateSet 0x003f6440 and RpSkyRenderStateGet 0x003f6690
 * (decomp/src/rw/basky.c, excluded). The retail functions wrote GIF packets and gp-globals for
 * the GS registers. The port records the same values here and the GL draw paths read them:
 * ALPHA_1 and TEST_1 drive blending and the alpha test. FARFOGPLANE, DITHER and MAXMIPLEVELS
 * are recorded for Get and not applied on GL (docs/renderer_port.md section 9). Ids follow
 * RpSkyRenderState (decomp/include/rw/sky2/rwcore.h:5683-5716). */
#define P4GL_SKY_DITHER 1
#define P4GL_SKY_ALPHA_1 2
#define P4GL_SKY_ATEST_1 3
#define P4GL_SKY_FARFOGPLANE 4
#define P4GL_SKY_MAXMIPLEVELS 5

/* Initial values. ALPHA_1 0x44 is SCE_GS_SET_ALPHA_1(0,1,0,1,0), the standard source-alpha
 * blend that Im2D drew with before this state existed. TEST_1 0 leaves the alpha test off.
 * MAXMIP 0 means never set. */
static uint32_t sSkyDither;
static uint32_t sSkyAlpha1 = 0x44u;
static uint32_t sSkyTest1;
static const float *sSkyFarFog;   /* the game's float, read per draw (sky2hook device.cpp) */
static int32_t sSkyMaxMip;
static int sSkyBlendWarned;

/* RpSkyRenderStateSet(state, value). Ids 1-3 take the raw 32-bit value; ATEST_1 keeps the low
 * 16 bits, as retail did. Ids 4-5 take the guest address of a float and of an int32. MAXMIPLEVELS accepts
 * 1..7. Returns 1 for a known state with a valid value, else 0. The value is an integer parameter: a payload
 * (colour, flags) is never an address and must not be translated. */
int32_t RpSkyRenderStateSet(int32_t state, uint32_t value)
{
    switch (state) {
    case P4GL_SKY_DITHER:
        sSkyDither = value;
        return 1;
    case P4GL_SKY_ALPHA_1:
        sSkyAlpha1 = value;
        return 1;
    case P4GL_SKY_ATEST_1:
        sSkyTest1 = value & 0xffffu;
        return 1;
    case P4GL_SKY_FARFOGPLANE:
        if (value == 0) {
            return 0;
        }
        sSkyFarFog = (const float *)P4_G2H(value);
        return 1;
    case P4GL_SKY_MAXMIPLEVELS:
        if (value == 0 || *(const int32_t *)P4_G2H(value) < 1 || *(const int32_t *)P4_G2H(value) > 7) {
            return 0;
        }
        sSkyMaxMip = *(const int32_t *)P4_G2H(value);
        return 1;
    default:
        return 0;
    }
}

/* RpSkyRenderStateGet(state, out): writes the value that Set stored. Returns 1 for a known id. */
int32_t RpSkyRenderStateGet(int32_t state, void *out)
{
    if (out == NULL) {
        return 0;
    }
    switch (state) {
    case P4GL_SKY_DITHER:
        *(uint32_t *)out = sSkyDither;
        return 1;
    case P4GL_SKY_ALPHA_1:
        *(uint32_t *)out = sSkyAlpha1;
        return 1;
    case P4GL_SKY_ATEST_1:
        *(uint32_t *)out = sSkyTest1;
        return 1;
    case P4GL_SKY_FARFOGPLANE:
        *(float *)out = sSkyFarFog != NULL ? *sSkyFarFog : 0.0f;
        return 1;
    case P4GL_SKY_MAXMIPLEVELS:
        *(int32_t *)out = sSkyMaxMip;
        return 1;
    default:
        return 0;
    }
}

/* Derived from sky2hook src/rw_to_gl.cpp (gsAlphaToBlend), tge-was-taken, reused with permission;
 * see third_party/README.md.
 *
 * GS ALPHA register to GL blending. The register is (A - B) * C / 128 + D with A, B, D picking Cs
 * (0), Cd (1) or zero (2) and C picking As (0), Ad (1) or FIX (2). reg holds A, B, C, D in bits
 * 0-7 and FIX in bits 32-39. Mapped combinations: over (A=Cs, B=Cd, D=Cd; 0x44), additive (A=Cs,
 * B=0, D=Cd; 0x48), subtractive (A=0, B=Cs, D=Cd; 0x42, reverse subtract), modulate (A=Cs, B=0,
 * D=0) and keep-destination (A=0, B=0, D=Cd). All zero fields give Cs, which is blending off.
 * Anything else (the FIX+1 over-bright combination A=Cs, B=Cd, C=FIX, D=Cs, and unused fields)
 * blends as source-alpha over and logs once. The alpha channel takes the source alpha (GS: the frame buffer alpha is As, not blended). Not applied: the destination alpha test and the
 * framebuffer's own alpha as a blend factor beyond Ad. */
void p4gl_gs_alpha_apply(uint64_t reg)
{
    const uint32_t fieldA = (uint32_t)(reg & 3u);
    const uint32_t fieldB = (uint32_t)((reg >> 2) & 3u);
    const uint32_t fieldC = (uint32_t)((reg >> 4) & 3u);
    const uint32_t fieldD = (uint32_t)((reg >> 6) & 3u);
    const float fix = (float)((reg >> 32) & 0xffu) / 128.0f;
    GLenum cf;
    GLenum cf1m;
    GLenum equation = GL_FUNC_ADD;
    GLenum src;
    GLenum dst;

    if ((reg & 0xffu) == 0u) {
        p4gl_glDisable(GL_BLEND);
        return;
    }
    cf = fieldC == 0u ? GL_SRC_ALPHA : fieldC == 1u ? GL_DST_ALPHA : GL_CONSTANT_ALPHA;
    cf1m = fieldC == 0u ? GL_ONE_MINUS_SRC_ALPHA : fieldC == 1u ? GL_ONE_MINUS_DST_ALPHA
                                                                 : GL_ONE_MINUS_CONSTANT_ALPHA;
    if (fieldA == 0u && fieldB == 1u && fieldD == 1u) {
        src = cf;
        dst = cf1m;
    } else if (fieldA == 0u && fieldB == 2u && fieldD == 1u) {
        src = cf;
        dst = GL_ONE;
    } else if (fieldA == 2u && fieldB == 0u && fieldD == 1u) {
        equation = GL_FUNC_REVERSE_SUBTRACT;
        src = cf;
        dst = GL_ONE;
    } else if (fieldA == 0u && fieldB == 2u && fieldD == 2u) {
        src = cf;
        dst = GL_ZERO;
    } else if (fieldA == 2u && fieldB == 2u && fieldD == 1u) {
        src = GL_ZERO;
        dst = GL_ONE;
    } else {
        if (!sSkyBlendWarned) {
            fprintf(stderr, "p4gl: GS ALPHA 0x%02x has no GL blend mapping; using source-alpha blend\n",
                    (unsigned)(reg & 0xffu));
            sSkyBlendWarned = 1;
        }
        src = GL_SRC_ALPHA;
        dst = GL_ONE_MINUS_SRC_ALPHA;
    }
    p4gl_glEnable(GL_BLEND);
    p4gl_glBlendEquation(equation);
    p4gl_glBlendColor(0.0f, 0.0f, 0.0f, fix);
    /* The GS blends the colour only; the alpha channel of the frame buffer receives the source alpha, which later
     * draws read back as Ad. */
    p4gl_glBlendFuncSeparate(src, dst, GL_ONE, GL_ZERO);
}

/* The RpSky ALPHA_1 state as a GS register: A, B, C, D in the low byte, FIX moved from bits 24-31
 * of the render-state word to bits 32-39 of the register. */
void p4gl_sky_apply_blend(void)
{
    p4gl_gs_alpha_apply((uint64_t)(sSkyAlpha1 & 0xffu) | ((uint64_t)(sSkyAlpha1 & 0xff000000u) << 8));
}

/* Derived from sky2hook src/rw_to_gl.cpp (gsTestToAtest) and src/device.cpp (setGsAtest),
 * tge-was-taken, reused with permission; see third_party/README.md.
 *
 * Alpha test code for the GL shaders, from a GS TEST register (ATE bit 0, ATST bits 1-3, AREF bits
 * 4-11, AFAIL bits 12-13). 0 means no discard: the test is off, ALWAYS, or its failure mode still
 * writes colour (FB_ONLY and RGB_ONLY, which is how ATST NEVER with FB_ONLY means "write colour,
 * skip depth") or Z (ZB_ONLY with NEVER writes depth and no colour: p4gl_frame_mask_apply masks the colour). 2..7 are LESS, LEQUAL, EQUAL, GEQUAL, GREATER and NOTEQUAL; 8 is NEVER with a
 * failure mode that writes no colour. The shaders compare PS2 alpha, where 0x80 is 1.0, so GL alpha
 * is scaled by 128 before the comparison against the AREF value. The destination alpha test (DATE)
 * is not applied. */
int32_t p4gl_gs_test_func(uint32_t test)
{
    const uint32_t atst = (test >> 1) & 7u;
    const uint32_t afail = (test >> 12) & 3u;

    if ((test & 1u) == 0u || atst == 1u || afail == 1u || afail == 3u || (afail == 2u && atst == 0u)) {
        return 0;
    }
    return atst == 0u ? 8 : (int32_t)atst;
}

float p4gl_gs_test_ref(uint32_t test)
{
    return (float)((test >> 4) & 0xffu);
}

/* Colour-writing alpha failures need a second pass without Z writes. RGB_ONLY also
 * preserves destination alpha. Return the GS AFAIL value (1 or 3), or zero. */
int p4gl_gs_alpha_color_fail(uint32_t test, int32_t *func, float *ref)
{
    const uint32_t atst = (test >> 1) & 7u;
    const uint32_t afail = (test >> 12) & 3u;

    if ((test & 1u) == 0u || (afail != 1u && afail != 3u) || atst == 1u) {
        return 0;
    }
    *func = atst == 0u ? 8 : (int32_t)atst;
    *ref = (float)((test >> 4) & 0xffu);
    return (int)afail;
}

/* The destination alpha test of TEST_1 (DATE, bit 14; DATM, bit 15): a fragment is drawn only where the frame buffer's alpha MSB equals
 * DATM. Returns 0 when it is off, else 1 + DATM. */
int p4gl_sky_date(void)
{
    return (sSkyTest1 & 0x4000u) != 0u ? 1 + (int)((sSkyTest1 >> 15) & 1u) : 0;
}

/* The raw ALPHA_1 and TEST_1 values the game last set (the draw log P4PC_GS_LOG prints them in the form of the PCSX2 GS-dump draw list). */
uint32_t p4gl_sky_alpha_raw(void)
{
    return sSkyAlpha1;
}

uint32_t p4gl_sky_test_raw(void)
{
    return sSkyTest1;
}

/* P4PC_GS_LOG=<first flip>,<last flip> (the draw log, in the terms of the PCSX2 GS-dump draw list: docs/renderer_port.md 11): 1 when the frame being drawn is inside the
 * window. */
static uint32_t sLogFlip = 0xFFFFFFFFu;
static int sLogIndex;
static int sLogNow;

int p4gl_gs_log_on(void)
{
    static int init;
    static unsigned first, last;
    extern uint32_t p4pc_flip_count(void);
    uint32_t flip;

    if (!init) {
        const char *e = getenv("P4PC_GS_LOG");

        init = 1;
        first = 1;
        last = 0;
        if (e != NULL) {
            first = (unsigned)strtoul(e, (char **)&e, 0);
            last = *e == ',' ? (unsigned)strtoul(e + 1, NULL, 0) : first;
        }
    }
    flip = p4pc_flip_count();
    sLogNow = 0;
    if (last >= first && flip >= first && flip <= last) {
        sLogNow = 1;
        if (flip != sLogFlip) {
            sLogFlip = flip;
            sLogIndex = 0;
        } else {
            sLogIndex++;
        }
        return 1;
    }
    return 0;
}

/* P4PC_GS_SKIP=<first>[-<last>][,<first>[-<last>]...]: the draws with these indices (the order of the GSLOG lines of the frame, from 0) are not drawn, to
 * find which draw makes a pixel. Valid right after p4gl_gs_log_on() returned 1. */
int p4gl_gs_skip(void)
{
    static int init;
    static int nRanges;
    static int range[64][2];
    const char *e;
    int i;

    if (!init) {
        init = 1;
        e = getenv("P4PC_GS_SKIP");
        while (e != NULL && *e != '\0' && nRanges < 64) {
            range[nRanges][0] = (int)strtol(e, (char **)&e, 0);
            range[nRanges][1] = *e == '-' ? (int)strtol(e + 1, (char **)&e, 0) : range[nRanges][0];
            nRanges++;
            if (*e != ',') {
                break;
            }
            e++;
        }
    }
    for (i = 0; sLogNow && i < nRanges; i++) {
        if (sLogIndex >= range[i][0] && sLogIndex <= range[i][1]) {
            return 1;
        }
    }
    return 0;
}

/* One log line of a draw: kind ("im2d", "atomic", "im3d", "ptank"), primitive count (Im2D: GL mode and vertex count), screen box (NULL: unknown), texture
 * raster (a frame raster is marked FRAME), the ALPHA and TEST values the draw ran with, and the depth, frame-mask and cull state. */
void p4gl_gs_log_draw(const char *kind, unsigned mode, int count, const float *box, uint32_t z, const float *rgba, uint32_t tex, uint64_t alpha, uint32_t test,
                      unsigned zt, unsigned zw)
{
    extern uint32_t p4pc_flip_count(void);

    fprintf(stderr, "GSLOG %u %s mode %u n %d box %.0f %.0f %.0f %.0f z %u rgba %.0f,%.0f,%.0f,%.0f tex %08x%s alpha %llx test %x zt %u zw %u fbmsk %08x cull %u rwfog %u fogcol %06x\n",
            (unsigned)p4pc_flip_count(), kind, mode, count, box != NULL ? (double)box[0] : -9999.0, box != NULL ? (double)box[1] : -9999.0,
            box != NULL ? (double)box[2] : -9999.0, box != NULL ? (double)box[3] : -9999.0, (unsigned)z, rgba != NULL ? (double)rgba[0] : 0.0,
            rgba != NULL ? (double)rgba[1] : 0.0, rgba != NULL ? (double)rgba[2] : 0.0, rgba != NULL ? (double)rgba[3] : 0.0, (unsigned)tex,
            tex != 0u && p4gl_raster_is_frame((const P4RwRaster *)P4_G2H(tex)) ? " FRAME" : "", (unsigned long long)alpha, (unsigned)test, zt, zw,
            (unsigned)p4gl_frame_mask_get(), (unsigned)p4gl_render_state_or(P4GL_RS_CULLMODE, 1u),
            (unsigned)p4gl_render_state_or(P4GL_RS_FOGENABLE, 0u), (unsigned)p4gl_render_state(P4GL_RS_FOGCOLOR) & 0xFFFFFFu);
}

/* The ALPHA_1 blend factor C selects the source alpha (as opposed to the destination alpha or FIX). */
int p4gl_sky_alpha_uses_source(void)
{
    return ((sSkyAlpha1 >> 4) & 3u) == 0u;
}

int32_t p4gl_sky_alpha_func(void)
{
    return p4gl_gs_test_func(sSkyTest1);
}

float p4gl_sky_alpha_ref(void)
{
    return p4gl_gs_test_ref(sSkyTest1);
}

/* The fog end distance: the FARFOGPLANE float the game gave RpSkyRenderStateSet when it is finite
 * and beyond fogStart, else 0 (the caller then uses the camera far plane). Derived from sky2hook
 * src/device.cpp (captureCameraState), tge-was-taken, reused with permission; see
 * third_party/README.md. */
float p4gl_sky_far_fog(float fogStart)
{
    float far;

    if (sSkyFarFog == NULL) {
        return 0.0f;
    }
    far = *sSkyFarFog;
    return (far == far && far < 1e30f && far > fogStart) ? far : 0.0f;
}

/* Depth test and depth write for the draw paths that follow them (Im3D, PTank). Test off with write
 * on draws with an always-pass depth test, as the PS2 GS does with ZTST ALWAYS. The caller undoes
 * it with p4gl_depth_restore. */
void p4gl_depth_apply_values(int zTest, int zWrite)
{
    if (zTest) {
        p4gl_glEnable(GL_DEPTH_TEST);
        p4gl_glDepthFunc(GL_LEQUAL);
    } else if (zWrite) {
        p4gl_glEnable(GL_DEPTH_TEST);
        p4gl_glDepthFunc(GL_ALWAYS);
    } else {
        p4gl_glDisable(GL_DEPTH_TEST);
    }
    p4gl_glDepthMask(zWrite ? GL_TRUE : GL_FALSE);
}

/* RwRenderState ZTESTENABLE and ZWRITEENABLE as set by the game. */
void p4gl_depth_apply(void)
{
    p4gl_depth_apply_values(p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u) != 0u,
                            p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 0u) != 0u);
}

/* The other GL paths and the framebuffer clear assume depth writes on and LEQUAL. */
void p4gl_depth_restore(void)
{
    p4gl_glDepthMask(GL_TRUE);
    p4gl_glDepthFunc(GL_LEQUAL);
}

/* GS FRAME_1.FBMSK (the high 32 bits of the register) for the draw paths: a channel whose byte is
 * 0xff is not written. The mdlEffect packet emitters (src/render/p4gl_mdleffect.c) set it; the
 * default is 0, all channels written. Partial masks within a channel are written in full. The
 * caller undoes it with p4gl_frame_mask_restore. */
static uint32_t sFrameMask;

void p4gl_frame_mask_set(uint32_t fbmsk)
{
    sFrameMask = fbmsk;
}

uint32_t p4gl_frame_mask_get(void)
{
    return sFrameMask;
}

/* TEST_1 with ATST NEVER and AFAIL = RGB_ONLY (0x33001 / 0x53001, the title's glow sprites): every fragment fails the alpha test,
 * and a failing fragment updates the colour but not the frame buffer's alpha (nor Z). Such a draw never writes alpha. */
static int rgb_only_never(void)
{
    return (sSkyTest1 & 1u) != 0u && ((sSkyTest1 >> 1) & 7u) == 0u && ((sSkyTest1 >> 12) & 3u) == 3u;
}

/* TEST_1 with ATST NEVER and AFAIL = ZB_ONLY (0x2001 | aref << 4: the dream battle's TV-outline passes): every fragment fails and
 * only its Z is written, so a Z-only draw. The later ZTST GEQUAL draws (the noise, the command menu's bar) then show only
 * where this Z lets them. */
static int zb_only_never(void)
{
    return (sSkyTest1 & 1u) != 0u && ((sSkyTest1 >> 1) & 7u) == 0u && ((sSkyTest1 >> 12) & 3u) == 2u;
}

static int sColorMaskSet;

void p4gl_frame_mask_apply(void)
{
    const int noAlpha = rgb_only_never();

    if (zb_only_never()) {
        p4gl_glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        sColorMaskSet = 1;
    } else if (sFrameMask != 0u || noAlpha) {
        p4gl_glColorMask((sFrameMask & 0x000000ffu) != 0x000000ffu,
                         (sFrameMask & 0x0000ff00u) != 0x0000ff00u,
                         (sFrameMask & 0x00ff0000u) != 0x00ff0000u,
                         !noAlpha && (sFrameMask & 0xff000000u) != 0xff000000u);
        sColorMaskSet = 1;
    }
}

void p4gl_frame_mask_rgb_only(void)
{
    p4gl_glColorMask((sFrameMask & 0x000000ffu) != 0x000000ffu,
                     (sFrameMask & 0x0000ff00u) != 0x0000ff00u,
                     (sFrameMask & 0x00ff0000u) != 0x00ff0000u, GL_FALSE);
    sColorMaskSet = 1;
}

void p4gl_frame_mask_restore(void)
{
    if (sColorMaskSet) {
        p4gl_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        sColorMaskSet = 0;
    }
}

/* GS CLAMP_1 override: the game writes the register itself (mdlEffectEmitValuePacket) for effects
 * that read the frame buffer. While it is set, WMS and WMT (bits 0-1 and 2-3: 0 repeat, 1 clamp,
 * 2 region clamp, 3 region repeat) replace the RwRenderState addressing of every texture bind.
 * Region clamp is clamp to edge; the region bounds are not applied. */
static int sClampOverride;
static uint64_t sClampValue;

void p4gl_gs_clamp_set(uint64_t clamp)
{
    sClampOverride = 1;
    sClampValue = clamp;
}

void p4gl_gs_clamp_clear(void)
{
    sClampOverride = 0;
}

/* Returns 1 with the clamp modes when an override is set, else 0. */
int p4gl_gs_clamp_get(uint32_t *wms, uint32_t *wmt)
{
    if (!sClampOverride) {
        return 0;
    }
    *wms = (uint32_t)(sClampValue & 3u);
    *wmt = (uint32_t)((sClampValue >> 2) & 3u);
    return 1;
}

/* RwRenderState CULLMODE (rwCULLMODECULLNONE 1, CULLBACK 2, CULLFRONT 3; unset is none). RenderWare
 * follows the Direct3D convention: a triangle is front-facing when its vertices run clockwise on its
 * screen, whose y axis points down. The port's clip space has y up (and mirrors x, see
 * p4gl_camera_object_matrix, which together keep RenderWare's orientation), so front is
 * counter-clockwise in GL: GL_CCW. Checked on the model m001_005.RMD, whose closed solids come out
 * complete under CULLBACK. Applied per draw by the atomic and Im3D paths; PTank quads are never
 * culled. */
void p4gl_cull_apply(void)
{
    const uint32_t mode = p4gl_render_state_or(P4GL_RS_CULLMODE, 1u);

    if (mode == 2u || mode == 3u) {
        p4gl_glEnable(GL_CULL_FACE);
        p4gl_glFrontFace(GL_CCW);
        p4gl_glCullFace(mode == 2u ? GL_BACK : GL_FRONT);
    } else {
        p4gl_glDisable(GL_CULL_FACE);
    }
}

/* P4PC_DRAW_LOG=<first>[-<last>]: one line per draw of the presented frames first..last, in the GS terms of the PCSX2 GS-dump draw list, so that a port frame and a retail GS dump read side
 * by side: the draw path, primitive, vertex count, the screen bounding box (Im2D: pixels; the 3D paths pass none), the last vertex colour, the texture raster, the blend
 * (ALPHA_1 A B C D and FIX; the blend itself is on only with VERTEXALPHAENABLE), the alpha test, depth test and write, the frame mask and fog. */
/* P4PC_DRAW_LOG=<first>[-<last>] (or `all` for the viewer, whose flips are not scene frames) prints one DRAWLOG line per GL draw of those flips: the path (im2d, im3d, atomic), the primitive,
 * the vertex or index count, the 2D bounding box and colour, the texture, the alpha blend, the alpha test (TEST_1), the depth
 * flags, the frame mask and the fog flag; an atomic's line is followed by its material. A PCSX2 GS dump of retail decodes to
 * a draw list in the same terms. P4PC_DRAW_UPTO=<n> draws only the first n draws of each logged flip and P4PC_DRAW_SKIP=<k>
 * leaves draw k of each out: that shows which draw makes a pixel. Returns 0 when the draw is to be left out. */
static unsigned long sDrawNumber;

/* The viewer calls this at the start of every scene frame (drawWindow): its flips do not count frames. */
void p4gl_drawlog_frame(void)
{
    sDrawNumber = 0;
}

int p4gl_drawlog(const char *path, int primType, int numVerts, const float *bbox, const float *rgba, const char *extra)
{
    static int parsed, first = -1, last = -1, upto = -1, skip = -1;
    unsigned long n;
    static uint32_t lastFlip = ~0u;
    const uint32_t tex = p4gl_render_state_or(P4GL_RS_TEXTURERASTER, 0u);
    char box[64] = "", col[24] = "", texs[96] = "untextured";
    char line[1024];
    int logThis;

    if (!parsed) {
        const char *e = getenv("P4PC_DRAW_LOG");

        parsed = 1;
        if (e != NULL) {
            first = atoi(e);
            last = strchr(e, '-') != NULL ? atoi(strchr(e, '-') + 1) : first;
            if (strcmp(e, "all") == 0) {
                first = 0;
                last = 0x7fffffff; /* every draw the viewer's draw window lets through */
            }
        }
        if (getenv("P4PC_DRAW_UPTO") != NULL) {
            upto = atoi(getenv("P4PC_DRAW_UPTO"));
        }
        if (getenv("P4PC_DRAW_SKIP") != NULL) {
            skip = atoi(getenv("P4PC_DRAW_SKIP"));
        }
    }
    logThis = first >= 0 && (int)p4pc_flip_count() >= first && (int)p4pc_flip_count() <= last;
    if ((!logThis && !p4pc_lockstep_draw_active()) || !p4gl_gl_draw_enabled()) {
        return 1; /* not a logged flip, or a frame the viewer's P4PC_DRAW_WINDOW does not draw */
    }
    if (p4pc_flip_count() != lastFlip) {
        lastFlip = p4pc_flip_count();
        sDrawNumber = 0; /* the draw number counts within a flip, so P4PC_DRAW_UPTO and P4PC_DRAW_SKIP act on every logged flip alike */
    }
    n = sDrawNumber++;
    if (bbox != NULL) {
        snprintf(box, sizeof box, "(%4.0f,%4.0f)-(%4.0f,%4.0f)", (double)bbox[0], (double)bbox[1], (double)bbox[2], (double)bbox[3]);
    }
    if (rgba != NULL) {
        snprintf(col, sizeof col, "%02x%02x%02x%02x", (unsigned)(rgba[0] + 0.5f), (unsigned)(rgba[1] + 0.5f), (unsigned)(rgba[2] + 0.5f), (unsigned)(rgba[3] + 0.5f));
    }
    if (tex != 0) {
        const P4RwRaster *r = (const P4RwRaster *)P4_G2H(tex);

        snprintf(texs, sizeof texs, "tex %08x %dx%d d%d type%d fmt%02x%s", (unsigned)tex, (int)r->width, (int)r->height, (int)r->depth, (int)(r->cType & 15u), (unsigned)r->cFormat,
                 p4gl_raster_is_frame(r) ? " FRAME" : "");
    }
    snprintf(line, sizeof line, "DRAWLOG flip %u #%lu %-6s prim %d x%-4d %s rgba %s | %s | alpha %u%u%u%u fix %02x %s | test %04x | z%c%c | fbmsk %08x | fog %u\n", p4pc_flip_count(), n, path, primType,
            numVerts, box, col, texs, (unsigned)(sSkyAlpha1 & 3u), (unsigned)((sSkyAlpha1 >> 2) & 3u), (unsigned)((sSkyAlpha1 >> 4) & 3u), (unsigned)((sSkyAlpha1 >> 6) & 3u),
            (unsigned)(sSkyAlpha1 >> 24), p4gl_render_state_or(P4GL_RS_VERTEXALPHAENABLE, 0u) ? "ABE" : "opaque", (unsigned)sSkyTest1, p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u) ? 'T' : '-',
            p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u) ? 'W' : '-', (unsigned)sFrameMask, (unsigned)p4gl_render_state_or(P4GL_RS_FOGENABLE, 0u));
    p4pc_lockstep_drawline(line);
    if (logThis) fputs(line, stderr);
    if (extra != NULL) {
        if (logThis) fprintf(stderr, "        %s\n", extra);
        p4pc_lockstep_drawline(extra);
        p4pc_lockstep_drawline("\n");
    }
    /* P4PC_DRAW_UPTO=<n> draws only the first n draws of the logged frames; P4PC_DRAW_SKIP=<k> leaves out draw k: which draw makes a difference. */
    return !logThis || !((upto >= 0 && (int)n >= upto) || (skip >= 0 && (int)n == skip));
}
