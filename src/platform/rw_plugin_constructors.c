/* Retail 003c95a0 (0x003c95a0..0x003c9638): constructor for RW plugin 0x507, registered in
 * code1_003c.c as func_003e8930(0xC, 0x507, func_003c95a0, func_003c9530). Called by
 * _rwPluginRegistryInitObject as constructCB(object, offset).
 *
 * No built unit has a `// FUN_003C95A0` marker (the decomp's C body is only in the unbuilt
 * code1_003c_cw119.c), so the trap stub would otherwise return 0 and fail the registry init.
 * This is a transcription of the retail code:
 * - iGpffffb700 (gp-18688) = offset.
 * - Returns 0 when func_003cc170() is 0.
 * - Creates a freelist with func_003e1220(16, 8, 4, 1, 0x008865B0, 0x4000B) and stores it at
 *   D_008872E0 + offset. Returns 0 on NULL.
 * - Points slot+4 and slot+8 at slot+4 (an empty list head), bumps iGpffffb704 (gp-18684),
 *   and returns the object. */
#include <stdint.h>

#include "p4pc/gmem.h"

extern int32_t func_003cc170(void);
extern int32_t func_003e1220(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, void *arg4,
                             int32_t arg5);
#define iGpffffb700 P4_GPDATA(int32_t, 0xb700)
#define iGpffffb704 P4_GPDATA(int32_t, 0xb704)
#define D_008872E0 P4_GARRAY(uint8_t, 0x008872E0u)

int32_t func_003c95a0(int32_t object, int32_t offset)
{
    int32_t alloc;
    int32_t *slot;

    iGpffffb700 = offset;
    if (func_003cc170() == 0) {
        return 0;
    }

    alloc = func_003e1220(16, 8, 4, 1, P4_G2H(0x008865B0u), 0x4000B);
    slot = (int32_t *)(D_008872E0 + iGpffffb700);
    slot[0] = alloc;
    if (slot[0] == 0) {
        return 0;
    }

    slot[1] = P4_H2G(&slot[1]);
    slot[2] = slot[1];
    iGpffffb704 += 1;
    return object;
}

/* Retail 003c4140: RpMaterialCreate. Its decomp body is only in the unbuilt code1_003c_cw119.c, so no
 * marker override can reach it; retail callers (the PTank create callback, FldShadow_*) would hit the
 * trap stub. The decomp's named RpMaterialCreate (bamateri.c) is that source: it allocates from the
 * material freelist (module globals at 0x008872E0 + the offset at 0x007647D0), sets reference count 1,
 * colour white, no texture or pipeline, the default surface properties and runs the plugin
 * constructors. */
extern void *RpMaterialCreate(void);

void *func_003c4140(void)
{
    return RpMaterialCreate();
}
