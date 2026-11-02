/* func_003b8050 (retail 0x003b8050, 384-byte frame). The object allocator and free are the
 * retail table entries at 0x008873F8 and 0x008873FC, both keyed by the handle at
 * 0x008864A8 (D_008864A8[0]). The retry path after a failed func_003b7680 frees the object,
 * sets it to NULL and carries on: the next field read then dereferences NULL. That is the
 * retail behaviour and is kept as is. */
u8 *func_003b8050(u8 *arg0, s32 arg1, u8 *arg2, s32 arg3, s32 arg4)
{
    u8 *object = *(u8 **)(arg2 + 0x18);
    u8 *s4;
    u32 n;
    u32 rows;
    u32 cnt;
    u8 lanes[256];

    if (func_003b83f0((int)object) != 0) {
        return func_003e2ce0(arg0, arg4) != 0 ? arg0 : NULL;
    }
    if (func_003df360(arg0, &rows, 4) == 0) {
        return NULL;
    }
    n = *(u32 *)(object + 20);
    s4 = D_008873F8[0](D_008864A8[0], 0x30116);
    memset(s4, 0, 72);
    cnt = rows;
    if (cnt == 0) {
        SkinFindNumUsedBones(s4, NULL, NULL, lanes, &cnt, n);
    }
    if (func_003b7680(s4, (s32)rows, cnt, n, lanes, NULL, NULL, NULL) == 0) {
        jtbl_008873FC[0](D_008864A8[0], s4);
        s4 = NULL;
    }
    if (func_003e2ce0(arg0, 4) == 0) {
        return NULL;
    }
    if (func_003df360(arg0, *(void **)(s4 + 20), (s32)(4 * n)) == 0) {
        return NULL;
    }
    if (func_003df300(arg0, *(void **)(s4 + 24), (s32)(16 * n)) == 0) {
        return NULL;
    }
    {
        u32 k;
        u32 off = 0;

        for (k = 0; k < *(u32 *)s4; k++, off += 64) {
            if (func_003e2ce0(arg0, 12) == 0) {
                return NULL;
            }
            if (func_003df300(arg0, *(u8 **)(s4 + 12) + off, 64) == 0) {
                return NULL;
            }
        }
    }
    SkinFindMaxWeights(s4, *(s32 *)(s4 + 24), n);
    SkinFindNumUsedBones(s4, *(void **)(s4 + 20), *(void **)(s4 + 24), *(void **)(s4 + 8),
                         (u32 *)(s4 + 4), n);
    func_003b8410(object, s4);
    return arg0;
}
