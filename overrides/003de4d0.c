/* func_003de4d0 (464 bytes): reads arg2 bytes from the file stream arg0 into arg1 and returns the number of bytes read (0 on a failed
 * direct read or when nothing is left). Stream fields: u64 +0 = size, u64 +0x10 = position (saved to +0x20 on entry), word +0x30 = unbuffered
 * flag (when set +0x40 is set to 5 and the read goes straight to sceRead), +0x38 = state (3 while reading, 2 after a buffered refill), +0x60 = file handle,
 * +0x64 = read offset in the buffer, +0x68 = buffer, +0x6C = number of valid bytes in the buffer. The request is clipped at the end of the
 * file. A buffered read first copies what is left in the buffer, then reads big remainders (at least a buffer) directly and refills the
 * buffer for small ones. The position advances by the number of bytes returned. */

extern void *p4c_0043f810_003de4d0(void *dst, const void *src, u32 n) __asm__("func_0043f810");

extern s32 sceRead(s32 fd, void *buf, s32 n);

s32 func_003de4d0(u8 *arg0, u8 *arg1, s32 arg2)
{
    u64 lim = *(u64 *)(arg0 + 0);
    u64 pos = *(u64 *)(arg0 + 0x10);
    s32 n = arg2;
    s32 total = 0;
    s32 r;
    s32 left;

    *(u64 *)(arg0 + 0x20) = pos;
    if (*(s32 *)(arg0 + 0x30) != 0) {
        *(s32 *)(arg0 + 0x40) = 5;
    }
    if ((s64)lim < (s64)(u64)(u32)((u32)pos + (u32)n)) {
        n = (s32)(lim - pos);
    }
    if (n == 0) {
        return 0;
    }
    if (*(s32 *)(arg0 + 0x30) != 0) {
        r = sceRead(*(s32 *)(arg0 + 0x60), arg1, n);
        if (r < 0) {
            return 0;
        }
        *(s32 *)(arg0 + 0x38) = 3;
        *(u64 *)(arg0 + 0x10) = *(u64 *)(arg0 + 0x10) + (u64)(u32)n;
        return n;
    }
    if (*(s32 *)(arg0 + 0x64) < *(s32 *)(arg0 + 0x6C)) {
        total = *(s32 *)(arg0 + 0x6C) - *(s32 *)(arg0 + 0x64);
        if ((u32)n < (u32)total) {
            total = n;
        }
        p4c_0043f810_003de4d0(arg1, *(u8 **)(arg0 + 0x68) + *(s32 *)(arg0 + 0x64), (u32)total);
        *(s32 *)(arg0 + 0x64) = *(s32 *)(arg0 + 0x64) + total;
        arg1 += total;
    }
    left = n - total;
    if (left != 0) {
        *(s32 *)(arg0 + 0x38) = 3;
        if (!(left < *(s32 *)(arg0 + 0x6C))) {
            r = sceRead(*(s32 *)(arg0 + 0x60), arg1, left);
            total += r;
        } else {
            sceRead(*(s32 *)(arg0 + 0x60), *(u8 **)(arg0 + 0x68), *(s32 *)(arg0 + 0x6C));
            *(s32 *)(arg0 + 0x64) = left;
            p4c_0043f810_003de4d0(arg1, *(u8 **)(arg0 + 0x68), (u32)*(s32 *)(arg0 + 0x64));
            total += *(s32 *)(arg0 + 0x64);
        }
        *(s32 *)(arg0 + 0x38) = 2;
    }
    *(u64 *)(arg0 + 0x10) = *(u64 *)(arg0 + 0x10) + (u64)(s64)total;
    return total;
}
