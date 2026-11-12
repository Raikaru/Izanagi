/* _rpSkinPipelinesAttach (0x003b8d40, 264 bytes): picks the skin pipeline for an atomic. arg0 is the
 * atomic, arg1 the pipeline kind (0..3). Returns arg0 with +0x6C set to func_0039b6e0(id).
 *
 * s = ((*(arg0+0x18))[8] & 0x80) != 0 (the geometry has skin weights); b is the byte at arg0 +
 * func_003c1b80(0x120) (the plugin offset). The id passed to func_0039b6e0 is
 *   kind 0: b == 0 -> 0x11002; else s == 1 -> 0x11014, else 0x11013
 *   kind 2: s == 1 -> 0x11014, else 0x11013
 *   kinds 1, 3 and any other: 0x11002 */

extern u32 p4c_0039b6e0_003b8d40(u32 id) __asm__("func_0039b6e0");
extern s32 p4c_003c1b80_003b8d40(s32 a) __asm__("func_003c1b80");

u8 *func_003b8d40(u8 *arg0, s32 arg1)
{
    u32 id;
    s32 s;
    u8 b;

    s = (*(u32 *)(*(u8 **)(arg0 + 0x18) + 8) & 0x80) != 0;
    b = *(u8 *)(arg0 + p4c_003c1b80_003b8d40(0x120));
    if (arg1 == 0) {
        if (b == 0) {
            id = 0x11002;
        } else if (s == 1) {
            id = 0x11014;
        } else {
            id = 0x11013;
        }
    } else if (arg1 == 3) {
        id = 0x11002;
    } else if (arg1 == 2) {
        id = (s == 1) ? 0x11014 : 0x11013;
    } else {
        id = 0x11002;
    }
    *(u32 *)(arg0 + 0x6C) = p4c_0039b6e0_003b8d40(id);
    return arg0;
}
