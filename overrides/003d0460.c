/* func_003d0460 (224 bytes): (arg0 object, arg1 = slot, arg2 = size, arg3 = callback argument). When the word at arg0 + 0x24 is not 0 the slot gets
 * func_003e1ff0(arg0[2], arg1, arg2, arg3). Otherwise d = the pointer at (arg0[2] + 0x18); when bit 25 of the word at d + 8 is set a block of arg2 + 0x18 bytes comes from the
 * allocator slot 0x008873E8 (tag 0x3050D) into *arg1 with block[0] = block[1] = 0, block[2] = arg2, block[3] = d, block[4] = arg1 and block[5] = arg3; else *arg1 =
 * func_003e1ff0(d, arg1, arg2, arg3). func_003e1ff0 is RwResourcesAllocateResEntry(owner, ownerRef, size, destroyNotify): the asm only sets $4 and the other three
 * argument registers keep the values the function came in with. */

extern u32 p4c_003e1ff0_003d0460(u8 *owner, u32 *ref, u32 size, u32 notify) __asm__("func_003e1ff0");

typedef void *(*P4AllocFn_003d0460)(u32 size, u32 tag);

void func_003d0460(u8 *arg0, u32 *arg1, u32 arg2, u32 arg3)
{
    u8 *d;
    u32 *block;

    if (*(s32 *)(arg0 + 0x24) != 0) {
        *arg1 = p4c_003e1ff0_003d0460(*(u8 **)(arg0 + 8), arg1, arg2, arg3);
        return;
    }
    d = *(u8 **)(*(u8 **)(arg0 + 8) + 0x18);
    if ((*(u32 *)(d + 8) & 0x02000000) != 0) {
        *arg1 = (u32)(*(P4AllocFn_003d0460 *)0x008873E8)(arg2 + 0x18, 0x3050D);
        block = (u32 *)*arg1;
        block[0] = 0;
        block[1] = 0;
        block[3] = (u32)d;
        block[2] = arg2;
        block[4] = (u32)arg1;
        block[5] = arg3;
    } else {
        *arg1 = p4c_003e1ff0_003d0460(d, arg1, arg2, arg3);
    }
}
