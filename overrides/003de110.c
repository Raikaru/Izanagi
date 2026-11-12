/* func_003de110 (352 bytes): opens a path through the device list. When the device count at 0x00887188 is non-zero the path
 * arg0 is scanned (length from the strlen slot 0x008873D4) for ':'; for each one the prefix up to and including it is
 * copied (func_00442de8) and terminated, and the list at 0x00887180 (links in word 0) is searched for the first node whose name
 * at +0xC the compare slot 0x008873D0 reports equal to the prefix (result 0). That device, or failing that the default
 * device at gp - 0x48B0 (0x00764840), has its open function at +0x4C called as open(device, arg0) and its result returned. With no
 * device, the callback at 0x00887194 (when set) is called with 6 and 0 returned. */

extern u8 *p4c_00442de8_003de110(u8 *d, u8 *s, u32 n) __asm__("func_00442de8");

typedef s32 (*P4StrlenFn_003de110)(u8 *s);
typedef s32 (*P4StrcmpFn_003de110)(u8 *a, u32 b);
typedef s32 (*P4OpenFn_003de110)(u8 *dev, u8 *path);
typedef void (*P4ErrFn_003de110)(s32 code);

s32 func_003de110(u8 *arg0)
{
    u8 buf[0x70];
    u32 len;
    u32 i;
    u8 *dev;
    u8 *node;

    if (*(u32 *)0x00887188 != 0) {
        len = (u32)(*(P4StrlenFn_003de110 *)0x008873D4)(arg0);
        for (i = 0; i < len; i++) {
            if ((s8)arg0[i] == 0x3A) {
                p4c_00442de8_003de110(buf, arg0, i + 1);
                buf[i + 1] = 0;
                dev = (u8 *)0;
                for (node = *(u8 **)0x00887180; node != (u8 *)0; node = *(u8 **)node) {
                    if ((*(P4StrcmpFn_003de110 *)0x008873D0)(buf, *(u32 *)(node + 0xC)) == 0) {
                        dev = node;
                        break;
                    }
                }
                if (dev != (u8 *)0) {
                    return (*(P4OpenFn_003de110 *)(dev + 0x4C))(dev, arg0);
                }
            }
        }
        dev = *(u8 **)0x00764840;
        if (dev != (u8 *)0) {
            return (*(P4OpenFn_003de110 *)(dev + 0x4C))(dev, arg0);
        }
    }
    if (*(P4ErrFn_003de110 *)0x00887194 != (P4ErrFn_003de110)0) {
        (*(P4ErrFn_003de110 *)0x00887194)(6);
    }
    return 0;
}
