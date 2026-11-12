/* func_003bfd00 (retail 0x003bfd00..0x003bfdec): constructor for RW plugin 0x504,
 * registered in code1_003c.c as func_003e8930(8, 0x504, func_003bfd00, func_003bfc40).
 * Called by _rwPluginRegistryInitObject as constructCB(object, offset).
 *
 * Transcribed from the retail disassembly (asm-only in code1_003b.c; its C attempt
 * did not match):
 * - iGpffffb6c0 (gp-18752) = offset.
 * - Creates the first freelist with func_003e1220(*D_0070AF70, aa88, 16, aa8c, 0x886520,
 *   0x40010) and stores it at D_008872E0 + offset. On NULL, returns 0.
 * - Creates the second freelist with func_003e1220(*D_0070AF90, aa90, 16, aa94, 0x8864F0,
 *   0x40014) and stores it at D_008872E0 + offset + 4. On success, bumps the count
 *   iGpffffb6c4 (gp-18748) and returns the object.
 * - On failure of the second one, frees the first with func_003e12f0 and clears the
 *   slot at D_008872E0 + offset, then returns 0.
 *
 * The slot is recomputed from iGpffffb6c0 after each call, as the retail code re-reads
 * gp-18752. The aa88..aa94 globals and the two 0x0088xxxx buffers have no names in the
 * port symbol table, so they are referenced by their retail addresses. */

s32 func_003bfd00(s32 arg0, s32 arg1)
{
    extern s32 iGpffffb6c0;
    extern s32 iGpffffb6c4;
    extern u8 D_008872E0[];
    extern u8 D_0070AF70[];
    extern u8 D_0070AF90[];
    extern void func_003e12f0(u8 *arg0);
    extern s32 func_003e1220(s32 arg0, s32 arg1, s32 arg2, s32 arg3, void *arg4, s32 arg5);
    s32 first;
    s32 second;

    iGpffffb6c0 = arg1;

    first = func_003e1220(*(s32 *)D_0070AF70, *(s32 *)0x00763B78, 16,
                          *(s32 *)0x00763B7C, (void *)0x00886520, 0x40010);
    *(s32 *)(D_008872E0 + iGpffffb6c0) = first;
    if (first == 0) {
        return 0;
    }

    second = func_003e1220(*(s32 *)D_0070AF90, *(s32 *)0x00763B80, 16,
                           *(s32 *)0x00763B84, (void *)0x008864F0, 0x40014);
    *(s32 *)(D_008872E0 + iGpffffb6c0 + 4) = second;
    if (second != 0) {
        iGpffffb6c4 += 1;
        return arg0;
    }

    func_003e12f0((u8 *)first);
    *(s32 *)(D_008872E0 + iGpffffb6c0) = 0;
    return 0;
}
