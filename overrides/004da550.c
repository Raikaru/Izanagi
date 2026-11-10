/* Retail 004da550: the direct level byte (+0x205) when the channel is active (+0x1a5) and the
 * enable byte (+0x204) is set, else 0. */
u8 AHXDCD_GetDrctLvl(int param_1)
{
    s8 v = 0;

    if (*(s8 *)(param_1 + 0x1a5) != 0 && *(s8 *)(param_1 + 0x204) != 0) {
        v = *(s8 *)(param_1 + 0x205);
    }
    return (u8)v;
}
