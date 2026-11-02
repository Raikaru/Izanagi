/* func_003d2010 (192 bytes): fills in the two request records for a draw job. arg1 + 0x10 gets the address 0x00763BB8
 * (gp - 0x5538) and arg1 + 0xC the record at 0x0070B150, whose words are set: +0 = the byte at
 * *(0x007647E0 + 0x008872E0) + arg0[0x24] + 0x27 (a table lookup by arg0's byte at +0x24), with bit 8 (0x100) set when arg0[0x28]
 * is 0; +8 = arg0[0x2C], +0xC = 0, +0x10 = arg0[0x28], +0x14 = arg0[0x2C], +0x18 = 0x0070B170. The word at 0x00763BBC is
 * cleared. Returns nothing. */

void func_003d2010(u8 *arg0, u8 *arg1)
{
    u32 *rec = (u32 *)0x0070B150;

    *(u32 *)(arg1 + 0x10) = 0x00763BB8;
    *(u32 **)(arg1 + 0xC) = rec;
    rec[2] = *(u32 *)(arg0 + 0x2C);
    rec[3] = 0;
    rec[0] = 0;
    rec[0] = (rec[0] & ~0xFFu) | *(u8 *)(0x008872E0 + *(s32 *)0x007647E0 + *(u8 *)(arg0 + 0x24) + 0x27);
    if (*(u32 *)(arg0 + 0x28) == 0) {
        rec[0] = rec[0] | 0x100;
    }
    *(u32 *)(*(u32 *)(arg1 + 0x10) + 4) = 0;
    rec[6] = 0x0070B170;
    rec[4] = *(u32 *)(arg0 + 0x28);
    rec[5] = *(u32 *)(arg0 + 0x2C);
}
