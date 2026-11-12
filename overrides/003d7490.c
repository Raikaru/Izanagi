/* func_003d7490 (288 bytes): keeps the two shared state objects of the object arg0's animation table in step with its slots. The table is
 * arg0 itself (words +0 and +4 are the two state objects, the eight slots are words +8 on). For each index k of 0 and 1 the slots are scanned
 * (slot, element j) for an element whose tag word at ((slot[0])[5] + 4 * j) + 0x20 equals k (j runs to slot[0x2C / 4]); when
 * one is found and the state object k is missing it is made with func_003e0f80() (0 fails the call), and when none is found an existing
 * object is released with func_003e0f40 and cleared. Returns arg0. */

extern void p4c_003e0f40_003d7490(u32 o) __asm__("func_003e0f40");
extern u32 p4c_003e0f80_003d7490(void) __asm__("func_003e0f80");

u8 *func_003d7490(u8 *arg0)
{
    u32 *blk = (u32 *)arg0;
    u32 k;
    u32 i;
    u32 j;
    s32 found;
    u32 *slot;

    for (k = 0; k < 2; k++) {
        i = 0;
        j = 0;
        found = 0;
        while (i < 8) {
            slot = (u32 *)blk[2 + i];
            if (slot == (u32 *)0) {
                i++;
                continue;
            }
            if (k == *(u32 *)(((u8 *)((u32 *)slot[0])[5]) + j * 4 + 0x20)) {
                found = 1;
                break;
            }
            j++;
            if (!(j < slot[11])) {
                j = 0;
                i++;
            }
        }
        if (found) {
            if (blk[k] == 0) {
                blk[k] = p4c_003e0f80_003d7490();
                if (blk[k] == 0) {
                    return (u8 *)0;
                }
            }
        } else if (blk[k] != 0) {
            p4c_003e0f40_003d7490(blk[k]);
            blk[k] = 0;
        }
    }
    return arg0;
}
