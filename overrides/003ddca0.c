/* func_003ddca0 (380 bytes): resolves the device named by the "name:" prefix of a path. arg0 is the
 * path. Returns the device entry, the default device, or 0.
 *
 * With *(s32 *)0x00887188 == 0 (not initialized) the hook at 0x00887194 (when set) is called with 6 and
 * 0 returned. Otherwise the path length comes from the function at 0x008873D4 and every ':' at index i
 * copies the first i+1 characters (func_00442de8 is strncpy) to a local buffer, terminates it after
 * the colon, and searches the device list (head *(u32 *)0x00887180, next pointer at +0, name at +0xC)
 * with the comparison function at 0x008873D0 (a result of 0 is a match); the first match is returned.
 * A path without a matching prefix returns the default device *(u32 *)0x00764840 (gp-0x48B0), or, when
 * that is 0, calls the hook with 6 and returns 0. */

extern char *p4c_00442de8_003ddca0(char *dst, const char *src, u32 n) __asm__("func_00442de8");

typedef u32 (*P4LenFn_003ddca0)(u8 *s);
typedef s32 (*P4CmpFn_003ddca0)(u8 *a, u8 *b);
typedef void (*P4HookFn_003ddca0)(s32 code);

u8 *func_003ddca0(u8 *arg0)
{
    u8 buf[0x70];
    u32 len;
    u32 i;
    u8 *dev;

    if (*(s32 *)0x00887188 == 0) {
        if (*(u32 *)0x00887194 != 0) {
            (*(P4HookFn_003ddca0 *)0x00887194)(6);
        }
        return (u8 *)0;
    }
    len = (*(P4LenFn_003ddca0 *)0x008873D4)(arg0);
    for (i = 0; i < len; i++) {
        if (*(s8 *)(arg0 + i) == 0x3A) {
            p4c_00442de8_003ddca0(buf, arg0, i + 1);
            buf[i + 1] = 0;
            dev = *(u8 **)0x00887180;
            while (dev != (u8 *)0) {
                if ((*(P4CmpFn_003ddca0 *)0x008873D0)(buf, *(u8 **)(dev + 0xC)) == 0) {
                    break;
                }
                dev = *(u8 **)dev;
            }
            if (dev != (u8 *)0) {
                return dev;
            }
        }
    }
    if (*(s32 *)0x00764840 == 0) {
        if (*(u32 *)0x00887194 != 0) {
            (*(P4HookFn_003ddca0 *)0x00887194)(6);
        }
        return (u8 *)0;
    }
    return *(u8 **)0x00764840;
}
