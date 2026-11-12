/* Retail 004f69d8: byte-swaps the two words of param_1 and stores them swapped end for end
 * (param_2[0] = bswap(param_1[1]), param_2[1] = bswap(param_1[0])). */
void cvmh_GetSize(u32 *param_1, u32 *param_2)
{
    u32 high = __builtin_bswap32(param_1[0]);
    u32 low = __builtin_bswap32(param_1[1]);

    param_2[1] = high;
    param_2[0] = low;
}
