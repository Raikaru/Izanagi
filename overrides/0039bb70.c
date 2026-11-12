/* PDSRegistryGetPipe (retail func_0039bb70): returns the registry entry (20 bytes,
 * {spec, parentKey, key, type, pipe}) whose key (word 2) equals arg1, or NULL. The registry
 * header is {items, count, capacity, flags}. The caller reads the pipe from entry + 0x10.
 *
 * Direct transliteration of the retail code. */

u8 *PDSRegistryGetPipe(u8 **arg0, s32 arg1)
{
    u32 count = *(u32 *)((u8 *)arg0 + 4);
    u8 *base = arg0[0];
    u32 i;

    for (i = 0; i < count; i++) {
        if (arg1 == *(s32 *)(base + i * 20 + 8))
            return base + i * 20;
    }
    return NULL;
}
