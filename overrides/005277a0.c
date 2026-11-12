/* Retail 005277a0: links record param_2 (0x44 bytes, table at +0x1f44) slot param_3 to
 * record param_4 (0x74 bytes, table at +0x135c): slot <- param_4, back-link <- param_2. */
void func_005277a0(int param_1, int param_2, int param_3, int param_4)
{
    *(int *)(param_3 * 4 + param_1 + param_2 * 0x44 + 0x1f44) = param_4;
    *(int *)(param_1 + param_4 * 0x74 + 0x135c) = param_2;
}
