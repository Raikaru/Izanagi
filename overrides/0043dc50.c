/* Retail 0043dc50: returns the word at 0x00710070 (a libmc2 pointer, `lw v0,0x70(0x710000)`). */
u8 *func_0043dc50(void)
{
    extern u8 *D_00710070;

    return D_00710070;
}
