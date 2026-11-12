/* func_003ddfd0 (80 bytes): RwFwrite. Same shape as func_003ddf80 with the write slot at +0x34:
 * calls it with (arg3, arg0, arg1*arg2) and returns the result divided by arg1 (unsigned). The
 * retail divu by zero leaves LO = 0xFFFFFFFF; that is returned for arg1 == 0 (x86 would trap). */

typedef u32 (*P4StreamIoFn)(u8 *stream, u8 *buffer, u32 bytes);

u32 func_003ddfd0(u8 *arg0, u32 arg1, u32 arg2, u8 *arg3)
{
    P4StreamIoFn fn;
    u32 bytes;
    u32 got;

    bytes = arg1 * arg2;
    fn = *(P4StreamIoFn *)(*(u8 **)(arg3 + 0x50) + 0x34);
    got = fn(arg3, arg0, bytes);
    if (arg1 == 0) {
        return 0xFFFFFFFFu;
    }
    return got / arg1;
}
