/* RSU block byte permutation: the eight bytes of a0 are rearranged into a2, and four of them are combined with the key bytes of a1 by xor or byte addition. Returns the last byte written. */
u32 func_004f96a0(u8 *a0, u8 *a1, u8 *a2)
{
    a2[0] = a0[2] ^ a1[1];
    a2[1] = a0[7];
    a2[2] = a0[5];
    a2[3] = a0[3] ^ a1[2];
    a2[4] = a0[0] ^ a1[0];
    a2[5] = a0[6];
    a2[6] = a0[4];
    a2[7] = a0[1] ^ a1[3];
    return a2[7];
}
