/* func_003cf070 (PS2AllPipelineNodeInit, the PS2All generic node's pipelineNodeInit): fills the
 * node's private data (node + 0x14) with the defaults of a PS2All node: word 6 = 0,
 * word 7 = 976, word 8 = 32, word 9 = the 16-byte-aligned address just past the header
 * (priv + 40 rounded up), words 4 and 5 = 0, and a set of one-byte flags at offsets 1..13.
 * The 16 bytes at the aligned address word 9 are a PS2 DMA/GIF tag template
 * {0, 0, 0x01000404, (priv[7] + 4) | 0x6C010000} followed (at +16..+28) by four zero words;
 * the template is written because retail code that reads the private data back (and the PS2
 * node bodies the GL renderer replaces) expect it; nothing in the port executes it.
 *
 * Direct transliteration of the retail code. The retail quad-word store is a plain 128-bit
 * pcpyld/sq of two 64-bit halves; here the four words are stored in little-endian order. */

void func_003cf070(u8 *node)
{
    u8 *priv = *(u8 **)(node + 0x14);
    u32 *tag;

    *(s32 *)(priv + 24) = 0;
    *(s32 *)(priv + 28) = 976;
    *(s32 *)(priv + 32) = 32;
    tag = (u32 *)(((u32)priv + 40 + 15) & ~15u);
    *(u32 **)(priv + 36) = tag;
    tag[4] = 0;
    tag[5] = 0;
    tag[6] = 0;
    tag[7] = 0;
    tag[0] = 0;
    tag[1] = 0;
    tag[2] = 0x01000404;
    tag[3] = (*(u32 *)(priv + 28) + 4) | 0x6C010000;
    *(s32 *)(priv + 16) = 0;
    *(s32 *)(priv + 20) = 0;
    priv[1] = 1;
    priv[2] = 2;
    priv[3] = 1;
    priv[4] = 1;
    priv[5] = 3;
    priv[6] = 1;
    priv[8] = 0;
    priv[9] = 1;
    priv[10] = 0;
    priv[11] = 2;
    priv[12] = 2;
    priv[13] = 0;
}
