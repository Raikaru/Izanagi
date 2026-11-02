/* Retail 003fe630: RenderWare driver plugin registration (_rwDeviceRegisterPlugin, called
 * from RwEngineInit in decomp src/renderware/core/badevice.c). Retail allocates the driver
 * pools and registers the driver's plugin table. Each result is kept in a retail gp global
 * at 0x007690F0 - N (N = 18120, 18124, 18128). A negative result returns 0. The last call's
 * result is returned as (result >= 0).
 *
 * The retail body is in decomp src/rw/basky.c (sky2 driver, excluded), so its `// FUN_`
 * marker is not in a built unit and an overrides/ file cannot attach to it. The first call
 * is retail func_003ec3a0, a one-line wrapper that calls func_003e3870 with 0x0070B7E0
 * prepended; that wrapper is not compiled, so the call is made directly here.
 *
 * Without this, RwEngineInit fails, RwEngineOpen never fills dOpenDevice, and the game
 * reaches RwEngineSetVideoMode with no device. */
#include <stdint.h>

#include "p4pc/gmem.h"

extern int32_t func_003e3870(void *, int32_t, int32_t, void *, void *, void *);
extern int32_t func_003e8410(int32_t, int32_t, uint32_t, uint32_t, uint32_t);   /* decomp: three register words */
extern int32_t func_003ef890(int32_t, int32_t, void *, void *, void *);
extern int32_t func_003e6210(int32_t, void *, void *, void *);
/* The texture plugin's stream callbacks (read, write: port-owned in src/render/p4gl_native.c; size: skyGetSizeTexCB). */
extern void func_003f6910(void);
extern void func_003f6990(void);
extern int32_t func_003f69f0(uint32_t *texture);

int32_t func_003fe630(void)
{
    int32_t r;

    r = func_003e3870((void *)P4_G2H(0x0070B7E0u), 92, 1036, 0, 0, 0);
    P4_GPDATA(int32_t, -18120) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e8410(216, 1036, 0, 0, 0);
    P4_GPDATA(int32_t, -18124) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003ef890(0, 272, 0, 0, 0);
    P4_GPDATA(int32_t, -18128) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e6210(272, (void *)func_003f6910, (void *)func_003f6990, (void *)func_003f69f0);
    P4_GPDATA(int32_t, -18128) = r;
    return r >= 0;
}
