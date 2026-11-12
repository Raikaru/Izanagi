/* Retail 00505078: stores three words in the 12-byte record `param_2` of the table at
 * param_1 + 0x3b8 (+0 <- param_3, +4 <- param_4, +8 <- param_5). */
void func_00505078(int param_1, int param_2, unsigned int param_3, unsigned int param_4, unsigned int param_5)
{
    param_1 += param_2 * 0xc;
    *(u32 *)(param_1 + 0x3c0) = param_5;
    *(u32 *)(param_1 + 0x3b8) = param_3;
    *(u32 *)(param_1 + 0x3bc) = param_4;
}
