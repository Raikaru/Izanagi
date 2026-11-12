/* func_003d30b0 (PS2AllMat pipelineNodeInit): sets up the node's private data (node + 0x14,
 * 0x32C bytes; RxPipelineNode.privateData): the five callback slots (words 0..4) start
 * empty, the default vertex-data buffer is D_0070C260 with 0x20 entries (+0x18, +0x1C),
 * +0x14 is 0x114 and +0x328 holds the marker 0x34F9. The slot setters
 * (func_003d38e0, func_003d3920, func_003d39a0) and the PS2 node bodies then work on this block.
 *
 * Port change: retail first calls func_003d2c40 and returns without touching the block when
 * it reports an invalid configuration. func_003d2c40 (with its callee func_003cc720) derives
 * the PS2 VU1 layout of the node (per-cluster strides, offsets and the double-buffer
 * partition of VU1 memory, at private-data offsets 0x20..0x1F4) from the init data that
 * func_003d3130 and friends fill. Only the PS2 node bodies read that layout; the GL renderer
 * (src/render) replaces those bodies and never sees it, so it is not computed. The init data
 * (what the game configured) is intact for anyone who needs it. */

extern u8 D_0070C260[];

void func_003d30b0(u8 *node)
{
    u8 *priv = *(u8 **)(node + 0x14);

    *(u8 **)(priv + 0x18) = D_0070C260;
    *(s32 *)(priv + 0x1C) = 0x20;
    *(s32 *)(priv + 0x14) = 0x114;
    *(s32 *)(priv + 0x0) = 0;
    *(s32 *)(priv + 0x4) = 0;
    *(s32 *)(priv + 0x8) = 0;
    *(s32 *)(priv + 0xC) = 0;
    *(s32 *)(priv + 0x10) = 0;
    *(s32 *)(priv + 0x328) = 0x34F9;
}
