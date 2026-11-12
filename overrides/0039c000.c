/* func_0039c000 (864 bytes): gathers vertices by index. arg0 = {u16 *indices, s32 count}; arg1 + 0x18 points at a table of
 * 16-byte vertices (four words each); for each of the count (signed, positive only) indices in turn the vertex
 * is copied to the next 16 bytes at arg2. Returns nothing. Retail unrolls the copy by eight vertices and re-reads the
 * index for every word; the result is the same unless the destination overlaps the index array. */

void func_0039c000(u32 *arg0, u8 *arg1, u32 *arg2)
{
    u16 *idx = (u16 *)arg0[0];
    u8 *verts = *(u8 **)(arg1 + 0x18);
    s32 n = (s32)arg0[1];
    s32 i;
    u32 *v;

    for (i = 0; i < n; i++) {
        v = (u32 *)(verts + ((u32)idx[i] << 4));
        arg2[0] = v[0];
        arg2[1] = v[1];
        arg2[2] = v[2];
        arg2[3] = v[3];
        arg2 += 4;
    }
}
