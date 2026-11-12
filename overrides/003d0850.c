/* func_003d0850 (224 bytes): (arg0 object, arg1 = slot, arg2 = size, arg3 = callback argument): d = arg0[8 / 4], a pointer. When the byte at d is not 0xFF,
 * func_003d0460(arg0, arg1, arg2, arg3) fills the slot (its result is not used). Otherwise, when bit 25 of the word at (*(u32 *)0x008872E4) + 8 is set, a block of
 * arg2 + 0x18 bytes comes from the allocator slot 0x008873E8 (tag 0x3050D) into *arg1 with block[0] = block[1] = 0, block[2] = arg2, block[3] = d, block[4] = arg1 and
 * block[5] = arg3; else *arg1 = func_003e1ff0(d, arg1, arg2, arg3) (RwResourcesAllocateResEntry(owner, ownerRef, size, destroyNotify): the asm only sets $4 and leaves the other three argument registers as they
 * came in). Returns *arg1. */

extern void p4c_003d0460_003d0850(u8 *a0, u32 *a1, u32 a2, u32 a3) __asm__("func_003d0460");
extern u32 p4c_003e1ff0_003d0850(u8 *d, u32 *ref, u32 size, u32 notify) __asm__("func_003e1ff0");

typedef void *(*P4AllocFn_003d0850)(u32 size, u32 tag);

u32 func_003d0850(u8 *arg0, u32 *arg1, u32 arg2, u32 arg3)
{
    u8 *d = *(u8 **)(arg0 + 8);
    u32 *block;

    if (*d != 0xFF) {
        p4c_003d0460_003d0850(arg0, arg1, arg2, arg3);
        return *arg1;
    }
    if ((*(u32 *)(*(u8 **)0x008872E4 + 8) & 0x02000000) != 0) {
        *arg1 = (u32)(*(P4AllocFn_003d0850 *)0x008873E8)(arg2 + 0x18, 0x3050D);
        block = (u32 *)*arg1;
        block[0] = 0;
        block[1] = 0;
        block[3] = (u32)d;
        block[2] = arg2;
        block[4] = (u32)arg1;
        block[5] = arg3;
    } else {
        *arg1 = p4c_003e1ff0_003d0850(d, arg1, arg2, arg3);
    }
    return *arg1;
}
