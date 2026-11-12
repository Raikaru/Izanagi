/* Octree descent for the palette quantizer. Consumes two key nibbles per call:
 * nibble 0 indexes node[7 + nibble]; nibble 1 indexes the child's slots.
 * Children are 92-byte nodes (28-byte header + 16 word slots at +28).
 * A node created at leaf depth gets its header cleared (words 0..5, byte 24). */

u32 *palquant_new_node(u8 *ctx, s32 leaf);

u8 *func_003f0610(u8 *ctx, u32 *node, u64 key, s32 depth)
{
    u32 *child;
    u32 *grand;
    s32 k;

    if (depth == 0) {
        return (u8 *)node;
    }

    k = (s32)(key & 0xf);
    if (node[7 + k] == 0) {
        node[7 + k] = (u32)palquant_new_node(ctx, depth == 1);
    }
    child = (u32 *)node[7 + k];
    if (depth == 1) {
        return (u8 *)child;
    }

    k = (s32)((key >> 4) & 0xf);
    if (child[7 + k] == 0) {
        child[7 + k] = (u32)palquant_new_node(ctx, depth - 1 == 1);
    }
    grand = (u32 *)child[7 + k];
    return func_003f0610(ctx, grand, key >> 8, depth - 2);
}
