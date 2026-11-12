/* func_00399470 (176 bytes): builds the merged name string at arg0 from the names of two records: the characters of
 * *(arg1 + 0x10 ...) and *(arg2 + 0x10 ...) are interleaved one at a time (first string first) until both end or 30 characters
 * have been written, then a terminating 0. When arg1 is null the name of arg2 is used for both sides, so every
 * character appears twice. Returns nothing. */

void func_00399470(u8 *arg0, u8 *arg1, u8 *arg2)
{
    s8 *p[2];
    s32 n = 0;
    s32 j;
    s8 c;

    if (arg1 == (u8 *)0) {
        p[0] = (s8 *)(arg2 + 0x10);
        p[1] = (s8 *)(arg2 + 0x10);
    } else {
        p[0] = (s8 *)(arg1 + 0x10);
        p[1] = (s8 *)(arg2 + 0x10);
    }
    while (n < 0x1E) {
        if (*p[0] == 0 && *p[1] == 0) {
            break;
        }
        for (j = 0; j < 2; j++) {
            c = *p[j];
            if (c != 0) {
                *arg0 = (u8)c;
                n++;
                p[j] = p[j] + 1;
                arg0++;
            }
        }
    }
    *arg0 = 0;
}
