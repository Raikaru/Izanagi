/* Retail 0x003E18C0 is RwFreeListForAllUsed (rwplcore.h). The C body is native in
 * src/renderware/plcore/bamemory.c, but the retail address has no `// FUN_003E18C0` marker in a
 * built unit (the marker sits on the next function, 0x003E1A70), so gen_port left the retail
 * address as a trap stub. Forward the retail address to the native body. */
#include <stdint.h>

extern void *RwFreeListForAllUsed(void *freeList, void (*fpCallBack)(void *mem, void *data), void *pData);

int32_t func_003e18c0(int32_t freeList, int32_t callback, int32_t data)
{
    return P4_H2G(RwFreeListForAllUsed((void *)P4_G2H(freeList),
                                                    (void (*)(void *, void *))p4_fn_g2h_sig(callback, P4_SIG(2, 0, 1, 1, 0, 0, 0, 0)),   /* the callback is called as (mem, data) with host pointers */
                                                    (void *)P4_G2H(data)));
}
