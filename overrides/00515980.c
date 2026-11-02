/* Retail 00515980: stores param_3 in word +0x131c of the 0x74-byte record `param_2`. The register v0
 * at return is the record offset (param_2 * 0x74); a caller returns it, so it is returned. Plain C
 * types, as the declarations in every unit take this signature. */
int func_00515980(int param_1, int param_2, unsigned int param_3)
{
    int offset = param_2 * 0x74;

    *(unsigned int *)(param_1 + offset + 0x131c) = param_3;
    return offset;
}
