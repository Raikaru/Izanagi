/* func_003db550 (212 bytes): queues a render job for an object. arg0 is the object (flags at +8), arg1
 * the parameter block (floats +0x68 and +0x6C). Does nothing when bit 24 of the flags is set or neither
 * bit 2 nor bit 7 is.
 *
 * func_003e89c0() returns a handle for func_003e8970(&dims, handle), which fills two ints (width and
 * height). The job record is { 0, arg0, ..., float scale } with scale = (float)(width * height) / ((2 * arg1[0x68]) *
 * (2 * arg1[0x6C])) at +0x14. When func_003ef6d0() returns non-zero, func_003ef260(func_003ef6d0(), func_003db490,
 * &record) runs. */

extern void p4c_003e8970_003db550(s32 *d, u32 h) __asm__("func_003e8970");
extern u32 p4c_003e89c0_003db550(void) __asm__("func_003e89c0");
extern void p4c_003ef260_003db550(u32 a, void *cb, void *arg) __asm__("func_003ef260");
extern u32 p4c_003ef6d0_003db550(void) __asm__("func_003ef6d0");

typedef struct P4Job_003db550 {
    u32 zero;
    u8 *obj;
    u32 pad[3];
    f32 scale;
} P4Job_003db550;

extern void p4c_003db490_003db550(void) __asm__("func_003db490");

void func_003db550(u8 *arg0, u8 *arg1)
{
    u32 flags;
    s32 dims[8];   /* func_003e8970 fills an RwVideoMode (width, height, depth, flags, refRate, format: 24 bytes); retail has 32 bytes of frame there, a 2-int array let it overwrite the job record's object pointer (0x500 = 1280, the width) */
    P4Job_003db550 job;
    f32 a;
    f32 b;
    u32 handle;

    flags = *(u32 *)(arg0 + 8);
    if ((flags & 0x01000000) == 0x01000000) {
        return;
    }
    if ((flags & 0x84) == 0) {
        return;
    }
    handle = p4c_003e89c0_003db550();
    p4c_003e8970_003db550(dims, handle);
    a = 2.0f * *(f32 *)(arg1 + 0x68);
    b = 2.0f * *(f32 *)(arg1 + 0x6C);
    a = a * b;
    job.obj = arg0;
    job.zero = 0;
    job.scale = (f32)(s32)(dims[1] * dims[0]) / a;
    if (p4c_003ef6d0_003db550() != 0) {
        p4c_003ef260_003db550(p4c_003ef6d0_003db550(), p4c_003db490_003db550, &job);
    }
}
