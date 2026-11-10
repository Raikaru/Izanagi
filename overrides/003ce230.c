/* func_003ce230 (116 bytes): the inverse of func_003ce170. Walks the DMA chain at *(arg0+8); types 0,
 * 6, 7 end it, type 1 skips q+1 qwords (q = tag bits 0..15), type 3 replaces its address word (+4) by
 * *(arg0+8) + 16 * index and moves on one qword, other types do not advance (as retail). Returns 1, or
 * 0 once a type-3 tag was converted. */

s32 func_003ce230(u8 *arg0)
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
            *(u32 *)(p + 4) = *(u32 *)(arg0 + 8) + (*(u32 *)(p + 4) << 4);
            ret = 0;
            p += 0x10;
            break;
        default:
            break;
        }
    } while (!done);
    return ret;
}
