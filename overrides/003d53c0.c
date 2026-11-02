/* func_003d53c0 (576 bytes): reads one animation object from a stream. arg0 is the stream (func_003df360
 * reads a word and returns 0 on failure, func_003df300 reads a float). Returns the object, or 0.
 *
 * The stream holds the magic 0x100, a type id, a count, a flags word and a float. The id is looked up in
 * the type table (0x30-byte records from 0x00886E50, count at 0x00764824 = gp-0x48CC; gp is 0x007690F0);
 * a missing id reports error {0x1B7, func_003df590(2)} through func_003df4d0 and returns 0. The
 * object is 0x18 + type[0x2C] + count * type[8] bytes from the allocator slot 0x008873E8 (tag 0x3001B):
 * +0 the type record, +4 the count, +8 the flags, +0xC the float, +0x10 = object + 0x18, and +0x14 =
 * +0x10 + count * type[8] when type[0x2C] > 0, else 0. The type's reader (slot +0x20 of the record that
 * the first lookup found) is then called as reader(stream, object). */

extern s32 p4c_003df300_003d53c0(s32 stream, void *p, s32 n) __asm__("func_003df300");
extern s32 p4c_003df360_003d53c0(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003df4d0_003d53c0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003d53c0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4AllocFn_003d53c0)(u32 size, u32 tag);
typedef void (*P4ReadFn_003d53c0)(s32 stream, u8 *obj);

static u8 *p4_53c0_find_type(u32 id)
{
    s32 count;
    s32 i;
    u32 *rec;

    count = *(s32 *)0x00764824;
    rec = (u32 *)0x00886E50;
    for (i = 0; i < count; i++) {
        if (id == rec[0]) {
            return (u8 *)(0x00886E50 + 0x30 * i);
        }
        rec += 12;
    }
    {
        s32 err[2];

        err[0] = 0x1B7;
        err[1] = p4c_003df590_003d53c0(2);
        p4c_003df4d0_003d53c0(err);
    }
    return (u8 *)0;
}

u8 *func_003d53c0(s32 arg0)
{
    s32 magic;
    s32 id;
    s32 count;
    s32 flags;
    f32 fval;
    u8 *first;
    u8 *type;
    u8 *obj;
    s32 bytes;

    if (p4c_003df360_003d53c0(arg0, &magic, 4) == 0) {
        return (u8 *)0;
    }
    if (magic != 0x100) {
        return (u8 *)0;
    }
    if (p4c_003df360_003d53c0(arg0, &id, 4) == 0) {
        return (u8 *)0;
    }
    first = p4_53c0_find_type((u32)id);
    if (first == (u8 *)0) {
        return (u8 *)0;
    }
    if (p4c_003df360_003d53c0(arg0, &count, 4) == 0) {
        return (u8 *)0;
    }
    if (p4c_003df360_003d53c0(arg0, &flags, 4) == 0) {
        return (u8 *)0;
    }
    if (p4c_003df300_003d53c0(arg0, &fval, 4) == 0) {
        return (u8 *)0;
    }
    type = p4_53c0_find_type((u32)id);
    obj = (u8 *)0;
    if (type != (u8 *)0) {
        bytes = count * *(s32 *)(type + 8);
        obj = (u8 *)(*(P4AllocFn_003d53c0 *)0x008873E8)((u32)(*(s32 *)(type + 0x2C) + bytes + 0x18), 0x3001B);
        *(s32 *)(obj + 4) = count;
        *(f32 *)(obj + 0xC) = fval;
        *(s32 *)(obj + 8) = flags;
        *(u8 **)(obj + 0) = type;
        *(u8 **)(obj + 0x10) = obj + 0x18;
        if (*(s32 *)(type + 0x2C) > 0) {
            *(u32 *)(obj + 0x14) = *(u32 *)(obj + 0x10) + (u32)(count * *(s32 *)(type + 8));
        } else {
            *(u32 *)(obj + 0x14) = 0;
        }
    }
    if (obj == (u8 *)0) {
        return (u8 *)0;
    }
    (*(P4ReadFn_003d53c0 *)(first + 0x20))(arg0, obj);
    return obj;
}
