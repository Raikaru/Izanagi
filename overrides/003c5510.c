/* func_003c5510 (retail 0x003c5510): constructor for RW plugin 0x502, registered in
 * code1_003c.c as func_003e8930(0x30, 0x502, func_003c5510, func_003c54c0). Called by
 * _rwPluginRegistryInitObject as constructCB(object, offset, size).
 *
 * Transcribed from the retail disassembly (0x3c5510..0x3c55e8):
 * - gp-18704 (iGpffffb6f0) = offset.
 * - When the construction count gp-18700 (iGpffffb6f4) is 0, creates the entry freelist
 *   with func_003e1210(12, 50, 4, 0x40502) into gp-18712 (iGpffffb6e8). On NULL it stores
 *   NULL and returns 0. The retail cleanup call after that test is unreachable.
 * - Writes the entry descriptor at D_008872E0 + offset, bumps the count, returns object.
 *
 * A trapped constructor returned 0, which made _rwPluginRegistryInitObject return NULL.
 * RwEngineStart then never reached func_003c3e90 (plugin 0x505), so the per-id freelist
 * at D_008872E0 stayed NULL and kwlnInitSceneRig segfaulted in _rwFreeListAllocReal. */

s32 func_003c5510(s32 arg0, s32 arg1)
{
    extern u8 *func_003e1210(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
    extern u8 *iGpffffb6f0;
    extern u8 *iGpffffb6e8;
    extern s32 iGpffffb6f4;
    extern u8 D_008872E0[];
    u8 *p;

    iGpffffb6f0 = (u8 *)arg1;
    if (iGpffffb6f4 == 0) {
        iGpffffb6e8 = func_003e1210(0xC, 0x32, 4, 0x40502);
        if (iGpffffb6e8 == NULL) {
            return 0;
        }
    }
    p = D_008872E0 + (s32)iGpffffb6f0;
    *(s16 *)(p + 0) = 1;
    p[0x8] = 3;
    p[0x9] = 4;
    p[0xA] = 5;
    p[0xC] = 1;
    p[0x10] = 2;
    p[0x18] = 6;
    p[0x28] = 4;
    p[0x29] = 8;
    p[0x2A] = 0;
    p[0x2B] = 1;
    p[0x2C] = 2;
    p[0x2D] = 0x10;
    iGpffffb6f4 += 1;
    return arg0;
}
