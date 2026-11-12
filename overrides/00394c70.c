/* func_00394c70 (256 bytes): transforms a bounding sphere and dispatches to the cull test.
 * arg0 is a draw record (+4 a frame, +0x18 an object, +0x4C the clip block), arg1 a sphere (+0xC
 * its radius), arg2, arg3 and arg4 are passed through.
 *
 * func_003e9700(frame) gives the frame's matrix, copied to a 64-byte local by func_003e0960;
 * func_003e42e0(out, arg1, 1, local) transforms the sphere. The out sphere's radius (+0xC) becomes
 * the length of the local matrix's third row (floats +0x20..+0x28; summed y*y, x*x, z*z in that
 * order, then square-rooted) times arg1's radius. With *(object+0x18) >= 2 the result goes to
 * func_00394870(object, arg0+0x4C, out, arg2, arg3, arg4), else to
 * func_00393900(object, out, arg2, arg3, arg4). Returns that callee's result. */

s32 func_00394c70(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3, s32 arg4)
{
    u8 *obj;
    u8 *mat;
    f32 mtx[16] __attribute__((aligned(16)));
    f32 out[4] __attribute__((aligned(16)));
    f32 x;
    f32 y;
    f32 z;
    f32 len;

    obj = *(u8 **)(arg0 + 0x18);
    mat = (u8 *)func_003e9700(*(s32 *)(arg0 + 4));
    func_003e0960(mtx, mat);
    func_003e42e0(out, arg1, 1, mtx);
    y = mtx[0x24 / 4];
    x = mtx[0x20 / 4];
    z = mtx[0x28 / 4];
    len = (y * y);
    len = len + (x * x);
    len = len + (z * z);
    len = __builtin_sqrtf(len);
    out[3] = len * *(f32 *)(arg1 + 0xC);
    if (*(s32 *)(obj + 0x18) >= 2) {
        return func_00394870(obj, arg0 + 0x4C, out, arg2, arg3, arg4);
    }
    return func_00393900(obj, out, arg2, arg3, arg4);
}
