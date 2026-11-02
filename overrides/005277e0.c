/* Retail 005277e0: links record param_3 (0x44 bytes, word at +8000 = +0x1f40) to record
 * param_2 (0x74 bytes, word at +0x1360), each holding the other's index. */
void sftrn_ConnBufTrn(int param_1, int param_2, int param_3)
{
    *(int *)(param_1 + param_3 * 0x44 + 8000) = param_2;
    *(int *)(param_1 + param_2 * 0x74 + 0x1360) = param_3;
}
