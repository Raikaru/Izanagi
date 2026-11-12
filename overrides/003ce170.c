/* func_003ce170 (136 bytes): converts the 16-byte DMA tags of a chain from pointers to qword indices.
 * arg0+8 holds the chain start. Each tag word's bits 28..30 select the type and the low 16 bits are the
 * qword count q: types 0, 6 and 7 end the walk, type 1 skips q+1 qwords, type 3 replaces its address
 * word (+4) by (address - *(arg0+8)) / 16 (signed) and the walk moves on one qword, other types do
 * not advance (the retail code loops on them). Returns 1, or 0 once a type-3 tag was converted. */

s32 func_003ce170(u8 *arg0)
{
    u8 *p;
    s32 ret;
    s32 done;

    p = *(u8 **)(arg0 + 8);
    done = 0;
    ret = 1;
    do {
        u32 tag = *(u32 *)p;
        u32 q = tag & 0xFFFF;

        switch ((tag >> 28) & 7) {
        case 0:
        case 6:
        case 7:
            done = 1;
            break;
        case 1:
            p += (q + 1) << 4;
            break;
        case 3:
            *(s32 *)(p + 4) = ((s32)*(u32 *)(p + 4) - (s32) * (u32 *)(arg0 + 8)) / 16;
            ret = 0;
            p += 0x10;
            break;
        default:
            break;
        }
    } while (!done);
    return ret;
}
