/* func_00398410 (304 bytes): releases the two 0x40-byte slots at arg0 (slot i at arg0 + 64*i, kind word at
 * +0x20) and clears the 0x90 bytes of arg0 (func_0043f9c8 is the port's memset). Returns arg0.
 *
 * Kind 5 clears words +0 and +4. Kind 4 passes word +0 to func_003ef3a0 when set. Kind 2 passes word +4 when set.
 * Kind 1 passes word +4 and then word +8 when set. Other kinds do nothing. */

extern void p4c_003ef3a0_00398410(u32 p) __asm__("func_003ef3a0");
extern void *p4c_0043f9c8_00398410(void *dst, s32 c, u32 n) __asm__("func_0043f9c8");

u8 *func_00398410(u8 *arg0)
{
    u32 i;

    for (i = 0; i < 2; i++) {
        u8 *slot = arg0 + (i << 6);

        switch (*(s32 *)(slot + 0x20)) {
        case 5:
            *(u32 *)slot = 0;
            *(u32 *)(slot + 4) = 0;
            break;
        case 4:
            if (*(u32 *)slot != 0) {
                p4c_003ef3a0_00398410(*(u32 *)slot);
            }
            break;
        case 2:
            if (*(u32 *)(slot + 4) != 0) {
                p4c_003ef3a0_00398410(*(u32 *)(slot + 4));
            }
            break;
        case 1:
            if (*(u32 *)(slot + 4) != 0) {
                p4c_003ef3a0_00398410(*(u32 *)(slot + 4));
            }
            if (*(u32 *)(slot + 8) != 0) {
                p4c_003ef3a0_00398410(*(u32 *)(slot + 8));
            }
            break;
        default:
            break;
        }
    }
    p4c_0043f9c8_00398410(arg0, 0, 0x90);
    return arg0;
}
