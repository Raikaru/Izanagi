/* func_003e0e20 (RwMatrixTransform): combines the 64-byte matrix `matrix` with `transform`.
 * Mode 0 replaces matrix with transform, mode 1 stores transform * matrix, mode 2 stores
 * matrix * transform (RwMatrixMultiply(result, left, right) into a stack copy, then copied
 * back). Any other mode reports error {plugin 1, "Invalid combination type"} and returns 0;
 * otherwise the matrix pointer is returned. The unit's callers declare it void and ignore the
 * result. */
u8 *func_003e0e20(u8 *matrix, u8 *transform, s32 mode)
{
    u32 tmp[16] __attribute__((aligned(16)));
    s32 err[2];
    u32 *dst;
    u32 *src;
    s32 i;

    if (mode == 2) {
        RwMatrixMultiply((u8 *)tmp, matrix, transform);
        src = tmp;
    } else if (mode == 1) {
        RwMatrixMultiply((u8 *)tmp, transform, matrix);
        src = tmp;
    } else if (mode == 0) {
        src = (u32 *)transform;
    } else {
        err[0] = 1;
        err[1] = func_003df590(0x80000003, (char *)0x0070B6F0);
        func_003df4d0(err);
        return (u8 *)0;
    }
    dst = (u32 *)matrix;
    for (i = 0; i < 16; i++) {
        dst[i] = src[i];
    }
    return matrix;
}
