/* Retail 005159a0: reads word +0x131c of the 0x74-byte record `param_2`. */
unsigned int func_005159a0(int param_1, int param_2)
{
    return *(u32 *)(param_1 + param_2 * 0x74 + 0x131c);
}
