/* func_003dd390 (160 bytes): skips a device prefix. Scans the string arg0 for its first ':' and returns the pointer just
 * after it; a string without ':' (or an empty one) is returned as it is. The length comes from the strlen slot
 * at 0x008873D4 and is re-read on every step, as in retail. */

typedef s32 (*P4StrlenFn_003dd390)(u8 *s);

u8 *func_003dd390(u8 *arg0)
{
    P4StrlenFn_003dd390 *slot = (P4StrlenFn_003dd390 *)0x008873D4;
    s32 i = 0;

    if ((*slot)(arg0) > 0) {
        do {
            if (arg0[i] == 0x3A) {
                return arg0 + (i + 1);
            }
            i++;
        } while (i < (*slot)(arg0));
    }
    return arg0;
}
