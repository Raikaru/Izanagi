/* func_003d6350 (retail 0x003d6350, 644 bytes): stream reader for an animation record set.
 * Reads a count from the stream, allocates a 8-byte record (+4 holds the storage from
 * func_003df5d0), links it through func_003df6e0 and then reads `remaining` entries, each
 * linked through the returned slot. obj+44 and obj+40 are read/transfer callbacks, obj+24
 * and obj+28 are per-entry callbacks. On failure the record is unwound as in retail. */
extern void (*jtbl_008873EC[])(u8 *value);
/* The RenderWare malloc slot (size, flags) at 0x008873e8: retail loads it at 0x003d63e4. Slot 0x008873f8 is
 * the free-list allocator (RwFreeListAlloc(freelist, flags)), a different function. */
extern u8 *(*p4_rw_malloc_slot[])(s32 size, s32 flags) __asm__("D_008873E8");
extern s32 func_003df050(s32 stream, s32 count, s32 *value, s32 *index);
extern s32 func_003df590(s32 code, s32 size);
extern void func_003df4d0(void *err);
extern s32 func_003df5d0(s32 size, s32 flags);
extern u8 *func_003df6e0(s32 size, s32 flags);
extern void func_003d6030(u8 *object, u8 *record);
extern void func_003d60f0(u8 *record);

u8 *func_003d6350(u8 *arg0, s32 arg1)
{
    u8 *obj = arg0;
    s32 stream = arg1;
    s32 remaining = 0;
    s32 index = 0;
    s32 value = 0;
    u8 *rec;
    u8 *link;
    u32 s1;
    s32 err[2];

    if (func_003df050(stream, 1, &value, &index) == 0) {
        return NULL;
    }
    if ((u32)index < *(u32 *)(obj + 12)) {
        return (u8 *)(uintptr_t)(((s32 (*)(s32))(uintptr_t)*(u32 *)(obj + 44))(stream));
    }
    if (func_003e2910(stream, &remaining, value) == 0) {
        return NULL;
    }

    rec = (u8 *)p4_rw_malloc_slot[0](8, *(s32 *)(obj + 4) | 0x30000);
    if (rec == NULL) {
        err[0] = 447;
        err[1] = func_003df590(0x80000013, 8);
        func_003df4d0(err);
        return NULL;
    }
    *(u32 *)(rec + 4) = (u32)func_003df5d0(4, *(s32 *)(obj + 4) | 0x30000);
    if (*(u32 *)(rec + 4) == 0) {
        jtbl_008873EC[0](rec);
        return NULL;
    }

    *(u8 **)rec = obj;
    link = func_003df6e0(*(s32 *)(obj + 16), *(s32 *)(obj + 4) | 0x30000);
    if (link == NULL) {
        func_003df7f0(*(s32 *)(rec + 4));
        jtbl_008873EC[0](rec);
        return NULL;
    }
    *(u8 **)link = rec;

    for (;;) {
        s32 old = remaining;

        remaining = old - 1;
        if (old == 0) {
            return rec;
        }
        if (func_003df050(stream, *(s32 *)(obj + 8), &value, &index) == 0) {
            goto fail;
        }
        s1 = ((u32 (*)(s32))(uintptr_t)*(u32 *)(obj + 40))(stream);
        if (s1 == 0) {
            goto fail;
        }
        /* No null check on the returned slot: retail stores through it directly. */
        link = func_003df6e0(*(s32 *)(rec + 4), *(s32 *)(obj + 4) | 0x30000);
        *(u32 *)link = s1;
        ((void (*)(u32))(uintptr_t)*(u32 *)(obj + 24))(s1);
        ((void (*)(u32))(uintptr_t)*(u32 *)(obj + 28))(s1);
    }

fail:
    if (*(u8 **)(obj + 20) == rec) {
        *(u32 *)(obj + 20) = 0;
    }
    func_003d6030(obj, rec);
    func_003d60f0(rec);
    jtbl_008873EC[0](rec);
    return NULL;
}
