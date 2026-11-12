/* func_003dddf0 (300 bytes): opens a file by path (arg0) and mode string (arg1). Returns the stream, or 0.
 *
 * The mode string is searched (func_00443010 is strstr) for the five strings at 0x00763C2C, 0x00763C30,
 * 0x00763C34, 0x00763C38 and 0x00763C3C. A = found(first), B = found(second); flag bits: the third string sets
 * bit 0 and, with A, bits 1 and 3; the fourth sets bits 1 and 2 and, with A, bit 0; the fifth sets bits 1
 * and 3 and, with A, bit 0, and with B bit 2. func_003ddca0(arg0) resolves the device and
 * func_003dda50(device, arg0, flags, 0, 0) opens the file. */

extern u8 *p4c_003dda50_003dddf0(u8 *d, u8 *p, u32 f, s32 a, s32 b) __asm__("func_003dda50");
extern u8 *p4c_003ddca0_003dddf0(u8 *p) __asm__("func_003ddca0");
extern u8 *p4c_00443010_003dddf0(u8 *h, u32 n) __asm__("func_00443010");

u8 *func_003dddf0(u8 *arg0, u8 *arg1)
{
    u32 flags;
    s32 a;
    s32 b;
    u8 *dev;

    flags = 0;
    a = 0;
    b = 0;
    if (p4c_00443010_003dddf0(arg1, 0x00763C2C) != 0) {
        a = 1;
    }
    if (p4c_00443010_003dddf0(arg1, 0x00763C30) != 0) {
        b = 1;
    }
    if (p4c_00443010_003dddf0(arg1, 0x00763C34) != 0) {
        flags |= 1;
        if (a != 0) {
            flags |= 2;
            flags |= 8;
        }
    }
    if (p4c_00443010_003dddf0(arg1, 0x00763C38) != 0) {
        flags = (flags | 2) | 4;
        if (a != 0) {
            flags |= 1;
        }
    }
    if (p4c_00443010_003dddf0(arg1, 0x00763C3C) != 0) {
        flags = (flags | 2) | 8;
        if (a != 0) {
            flags |= 1;
        }
        if (b != 0) {
            flags |= 4;
        }
    }
    dev = (u8 *)p4c_003ddca0_003dddf0(arg0);
    if (dev == (u8 *)0) {
        return (u8 *)0;
    }
    return (u8 *)p4c_003dda50_003dddf0(dev, arg0, flags, 0, 0);
}
