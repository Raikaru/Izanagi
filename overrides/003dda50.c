/* func_003dda50 (440 bytes): claims a free request slot from the device arg0 and starts it; returns the slot, or 0.
 * arg0 may be null (the error callback at 0x00887194, when set, is called with 4). Under the lock gp - 0x48AC
 * (0x00764844; func_00421820 / func_00421800) the arg0[1] slots, fetched with the function at arg0[5](arg0, i), are scanned for the first
 * one whose state word +0x38 is 1 and it is set to 5. With no free slot the error word at func_003ddc10()[4] is set to 8 and
 * the callback is called with 8 (result 0). Otherwise, unless the flag 0x10 of arg2 is set and the slot has no +0x30 value
 * (and arg0[4] is 0), the slot gets +0x40 = 2, +0x48 = arg3 and +0x4C = arg4 (an arg0[4] flag also adds 0x10 to arg2); then
 * the start function arg0[10](arg0, slot, arg1, arg2) must return 1 (else the slot goes back to state 1 and 0 is returned) and +0x34 is set to 2. */

extern u8 *p4c_003ddc10_003dda50(void) __asm__("func_003ddc10");
extern void p4c_00421800_003dda50(u32 s) __asm__("func_00421800");
extern void p4c_00421820_003dda50(u32 s) __asm__("func_00421820");

typedef u8 *(*P4SlotFn_003dda50)(u8 *dev, s32 i);
typedef s32 (*P4StartFn_003dda50)(u8 *dev, u8 *slot, u32 a, u32 b);
typedef void (*P4ErrFn_003dda50)(s32 code);

u8 *func_003dda50(u8 *arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
    u8 *slot = (u8 *)0;
    s32 i;
    s32 n;
    P4ErrFn_003dda50 err;

    if (arg0 == (u8 *)0) {
        err = *(P4ErrFn_003dda50 *)0x00887194;
        if (err != (P4ErrFn_003dda50)0) {
            err(4);
        }
        return (u8 *)0;
    }
    p4c_00421820_003dda50(*(u32 *)0x00764844);
    n = *(s32 *)(arg0 + 4);
    for (i = 0; i < n; i++) {
        u8 *cand = (*(P4SlotFn_003dda50 *)(arg0 + 0x14))(arg0, i);

        if (*(s32 *)(cand + 0x38) == 1) {
            slot = cand;
            *(u32 *)(cand + 0x38) = 5;
            break;
        }
        n = *(s32 *)(arg0 + 4);
    }
    p4c_00421800_003dda50(*(u32 *)0x00764844);
    if (slot == (u8 *)0) {
        *(u32 *)((u8 *)p4c_003ddc10_003dda50() + 0x10) = 8;
        err = *(P4ErrFn_003dda50 *)0x00887194;
        if (err != (P4ErrFn_003dda50)0) {
            err(8);
        }
        return (u8 *)0;
    }
    if ((arg2 & 0x10) != 0 || *(u32 *)(slot + 0x30) != 0) {
        *(u32 *)(slot + 0x40) = 2;
        *(u32 *)(slot + 0x48) = arg3;
        *(u32 *)(slot + 0x4C) = arg4;
    }
    if (*(u32 *)(arg0 + 0x10) != 0) {
        *(u32 *)(slot + 0x40) = 2;
        arg2 = arg2 | 0x10;
        *(u32 *)(slot + 0x48) = arg3;
        *(u32 *)(slot + 0x4C) = arg4;
    }
    if ((*(P4StartFn_003dda50 *)(arg0 + 0x28))(arg0, slot, arg1, arg2) != 1) {
        *(u32 *)(slot + 0x38) = 1;
        slot = (u8 *)0;
    }
    if (slot != (u8 *)0) {
        *(u32 *)(slot + 0x34) = 2;
    }
    return slot;
}
