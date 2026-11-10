/* RenderWare device system request handler on OpenGL 3.3 core. This replaces the retail
 * sky2 dispatcher at 0x004010c0 (decomp/src/rw/basky.c, excluded). The request layout,
 * the device block and the standard-function table are the retail ones; see
 * docs/renderer_port.md sections 1-3 for the addresses and the retail behaviour of each
 * request. Requests the port does not implement yet return 0, as the retail switch does
 * for unsupported ids. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4pc/gmem.h"
#include "p4pc/rw_skytran.h"

/* Retail data: the device block (badevice.h RwDevice) and the stdFunc (id, handler) table. */
#define D_0070C220 P4_GARRAY(uint32_t, 0x0070C220u)
#define D_0070C140 P4_GARRAY(uint32_t, 0x0070C140u)

/* The retail mode table at 0x0070B840 (retail GETMODEINFO 0x40111c loads it as lui 0x71, -18368):
 * 24-byte RwVideoMode records, 90 modes, static data in the ELF. The record of the current mode is
 * kept at 0x0088C9B0, which USEMODE (0x3fe470) fills and GETMODEINFO of mode -1 returns. */
#define P4GL_NUM_MODES 90
#define P4GL_MODE_RECORD_BYTES 24
#define D_0070B840 P4_GARRAY(uint8_t, 0x0070B840u)
#define D_0088C9B0 P4_GARRAY(uint8_t, 0x0088C9B0u)

/* Standard-function table size (retail STANDARDS request passes 28 pairs). */
#define P4GL_NUM_STD_PAIRS 28

/* Texture memory reported to the core: retail reports (0x100000 - used) << 2 bytes. */
#define P4GL_TEXMEM_WORDS 0x100000u

/* Texture max size reported (retail GETMAXTEXTURESIZE). */
#define P4GL_MAX_TEXTURE_SIZE 1024u

/* rwDEVICE_SKY2: the matched core branches on this id, so the port keeps reporting it. */
#define P4GL_DEVICE_ID 6u

static uint32_t sCurrentMode;

/* RenderWare's engine multiply slots (decomp plcore bamatrix.c 003e00f0, bavector.c 003e3f80). */
extern int32_t _rwMatrixSetMultFn(void (*multMat)(void *dst, const void *a, const void *b));
extern int32_t _rwVectorSetMultFn(void *(*multPoint)(void *out, const void *in, const void *matrix),
                                  void *(*multPoints)(void *out, const void *in, int32_t count, const void *matrix),
                                  void *(*multVector)(void *out, const void *in, const void *matrix),
                                  void *(*multVectors)(void *out, const void *in, int32_t count, const void *matrix));

/* Default for a stdFunc slot no pair assigns. Calling it is a bug in the core or in the
 * table, so it stops the process instead of returning a value. */
static void p4gl_std_unassigned(void)
{
    fprintf(stderr, "p4gl: call to an unassigned RenderWare standard function\n");
    abort();
}

/* STANDARDS: fills numIn slots of the stdFunc array (pOut: guest storage, one guest code address per slot)
 * with the default, then writes each (id, handler) pair of the retail table into its slot. */
static int32_t standards(uint32_t *out, int32_t numIn)
{
    int32_t i;
    uint32_t k;

    if (out == NULL || numIn < 0) {
        return 0;
    }
    for (i = 0; i < numIn; i++) {
        out[i] = p4_fn_h2g_sig(p4gl_std_unassigned, "v");
    }
    for (k = 0; k < P4GL_NUM_STD_PAIRS; k++) {
        uint32_t id = D_0070C140[2 * k];
        uint32_t fn = D_0070C140[2 * k + 1];

        if (id < (uint32_t)numIn) {
            out[id] = fn;
        }
    }
    return 1;
}

/* The retail sky2 dispatcher, with the same request numbers (badevice.h RwCoreDeviceSystemFn). */
int32_t func_004010c0(int32_t requestID, void *pOut, void *pInOut, int32_t numIn)
{
    (void)pInOut;

    switch (requestID) {
    case 0: /* OPEN: GL 3.3 core context on the shared window. */
        return p4gl_context_open() == 0 ? 1 : 0;
    case 1: /* CLOSE */
        p4gl_context_close();
        return 1;
    case 2: /* START: the context is current from OPEN; no per-start GL state is needed. */
        return 1;
    case 3: /* STOP */
        return 1;
    case 4: /* REGISTER: copy the device block to the core's dOpenDevice. */
        if (pOut == NULL) {
            return 0;
        }
        p4gl_im2d_install((P4RwDevice *)D_0070C220);
        memcpy(pOut, D_0070C220, P4GL_REGISTER_BYTES);
        return 1;
    case 5: /* GETNUMMODES */
        *(uint32_t *)pOut = P4GL_NUM_MODES;
        return 1;
    case 6: /* GETMODEINFO: copy the retail mode record for mode numIn. */
        if (numIn < -1 || numIn >= P4GL_NUM_MODES) {
            return 0;
        }
        memcpy(pOut, numIn < 0 ? D_0088C9B0 : D_0070B840 + (size_t)numIn * P4GL_MODE_RECORD_BYTES,
               P4GL_MODE_RECORD_BYTES);
        return 1;
    case 7: /* USEMODE: records the mode. The window keeps its size. */
        if (numIn < 0 || numIn >= P4GL_NUM_MODES) {
            return 0;
        }
        sCurrentMode = (uint32_t)numIn;
        memcpy(D_0088C9B0, D_0070B840 + (size_t)numIn * P4GL_MODE_RECORD_BYTES,
               P4GL_MODE_RECORD_BYTES);
        return 1;
    case 8: /* FOCUS */
        return 1;
    case 9: /* INITPIPELINE */
        return 1;
    case 10: /* GETMODE */
        *(uint32_t *)pOut = sCurrentMode;
        return 1;
    case 11: /* STANDARDS */
        return standards((uint32_t *)pOut, numIn);
    case 12: /* GETTEXMEMSIZE: bytes, retail (0x100000 - used) << 2 with used = 0. */
        *(uint32_t *)pOut = P4GL_TEXMEM_WORDS << 2;
        return 1;
    case 17: /* FINALIZESTART: no PS2 pipeline to finish; GL state is set per draw. Retail (0x00401244), once its pipeline is up,
              * installs the sky driver's VU0 matrix and vector multiplies into the engine slots (0x004012ac); so does the port
              * (src/platform/rw_skytran.c), so RwV3dTransformPoint(s)/Vector(s) round as retail's VU0 does. */
        _rwMatrixSetMultFn(func_00402390);
        _rwVectorSetMultFn(func_00402410, func_00402470, func_00402510, func_00402570);
        return 1;
    case 18: /* INITIATESTOP: no PS2 pipeline to tear down. */
        return 1;
    case 19: /* GETMAXTEXTURESIZE */
        *(uint32_t *)pOut = P4GL_MAX_TEXTURE_SIZE;
        return 1;
    case 22: /* GETID */
        *(uint16_t *)pOut = (uint16_t)P4GL_DEVICE_ID;
        return 1;
    default:
        /* 13-16 (subsystems), 20 (pipeline request), 21 (metric block): retail returns 0. */
        return 0;
    }
}

/* HINTRENDERF2B (stdFunc 22): retail accepts the hint and returns 1 (003fe1f0 is li v0,1;
 * jr ra). The GL device honours no render hints, so there is nothing to apply. */
int32_t func_003fe1f0(void *pOut, void *pInOut, int32_t nI)
{
    (void)pOut;
    (void)pInOut;
    (void)nI;
    return 1;
}
