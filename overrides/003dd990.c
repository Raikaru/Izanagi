/* func_003dd990 (192 bytes): looks a name up in the device list. arg0 is the name. The list head is
 * *(u32 *)0x00887180; each entry's name sits at entry+0x50 and the next pointer at entry+0. The
 * comparison function is the pointer at 0x008873C8, called as cmp(entry+0x50, arg0); a result of 0
 * is a match. Without a match the hook at 0x00887194 (when set) is called with 6 and 0 is returned;
 * with a match it is called with 7 and 1 is returned. */

typedef s32 (*P4CmpFn_003dd990)(u8 *a, u8 *b);
typedef void (*P4HookFn_003dd990)(s32 code);

s32 func_003dd990(u8 *arg0)
{
    u8 *entry;

    entry = *(u8 **)0x00887180;
    while (entry != (u8 *)0) {
        if ((*(P4CmpFn_003dd990 *)0x008873C8)(entry + 0x50, arg0) == 0) {
            break;
        }
        entry = *(u8 **)entry;
    }
    if (entry == (u8 *)0) {
        if (*(u32 *)0x00887194 != 0) {
            (*(P4HookFn_003dd990 *)0x00887194)(6);
        }
        return 0;
    }
    if (*(u32 *)0x00887194 != 0) {
        (*(P4HookFn_003dd990 *)0x00887194)(7);
    }
    return 1;
}
