/* func_003bb0d0 (spline plugin, module 0x102 open callback, registered by func_003bbe20
 * through func_003e8930; func_003bb030 is the matching close): reference-counted set-up of the
 * spline tables. The count lives in the retail gp global at 0x007690F0 - 18860. The first open
 * builds the basis tables (func_003b8e50) and the natural-spline matrices of order 4..10
 * (func_003b90b0), kept in the retail slots 0x008864D0 + 4 * (order - 4). If a matrix cannot
 * be built, the matrices already built are freed and cleared, and the open fails; a failure
 * reports error code 3 for module 0x102. Returns arg0 on success, 0 on failure.
 *
 * Direct transliteration of the retail code. */

typedef void (*P4SplineFreeFn)(void *p);

extern s32 func_003b8e50(void);
extern f32 *func_003b90b0(s32 n);

s32 func_003bb0d0(s32 arg0)
{
    s32 *refcount = (s32 *)(0x007690F0 - 18860);
    u32 *slots = (u32 *)0x008864C0;
    P4SplineFreeFn release = *(P4SplineFreeFn *)0x008873EC;
    s32 err[2];
    s32 order;
    s32 k;

    if (*refcount != 0) {
        *refcount = *refcount + 1;
        return arg0;
    }

    if (func_003b8e50() != 0) {
        for (order = 4; order <= 10; order++) {
            slots[order] = (u32)func_003b90b0(order);
            if (slots[order] == 0) {
                for (k = order - 1; k >= 4; k--) {
                    release((void *)slots[k]);
                    slots[k] = 0;
                }
                goto fail;
            }
        }
        *refcount = *refcount + 1;
        return arg0;
    }

fail:
    err[0] = 258;
    err[1] = func_003df590(3);
    func_003df4d0(err);
    return 0;
}
