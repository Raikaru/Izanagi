/* Retail 004da518: copies 8 bytes (an unaligned ldl/ldr pair to an unaligned sdl/sdr pair) from
 * param_2 to param_1 + 0xa20. */
void AHXDCD_SetExtPrm(int param_1, u64 *param_2)
{
    __builtin_memcpy((u8 *)(param_1 + 0xa20), (const u8 *)param_2, 8);
}
