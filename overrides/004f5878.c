/* Retail 004f5878: splits a 64-bit value into its high word (param_2) and low word (param_3). */
void RFMAI_CnvS64toU32plusU32(u64 param_1, u32 *param_2, u32 *param_3)
{
    *param_3 = (u32)param_1;
    *param_2 = (u32)(param_1 >> 32);
}
