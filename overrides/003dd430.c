/* func_003dd430 (256 bytes): joins a directory (arg2) and a name (arg3) into the buffer arg0 of arg1 bytes, turning every '/'
 * or '\' of the name (and its terminating NUL kept as it is) into the separator arg4. Returns arg0, or 0 when the two strings
 * (strlen = func_00442948) are longer than arg1 - 1 together. The directory is copied with the strcpy slot 0x008873AC. */

extern s32 p4c_00442948_003dd430(u8 *s) __asm__("func_00442948");

typedef void (*P4StrcpyFn_003dd430)(u8 *d, u8 *s);

u8 *func_003dd430(u8 *arg0, s32 arg1, u8 *arg2, u8 *arg3, s32 arg4)
{
    s32 dirLen = p4c_00442948_003dd430(arg2);
    s32 nameLen = p4c_00442948_003dd430(arg3);
    s32 i;
    s8 c;
    u8 *d;

    if (arg1 - 1 < dirLen + nameLen) {
        return (u8 *)0;
    }
    (*(P4StrcpyFn_003dd430 *)0x008873AC)(arg0, arg2);
    d = arg0 + dirLen;
    for (i = 0; i <= nameLen; i++) {
        c = (s8)arg3[i];
        if (c == 0x2F || c == 0x5C) {
            d[i] = (u8)arg4;
        } else {
            d[i] = (u8)c;
        }
    }
    return arg0;
}
