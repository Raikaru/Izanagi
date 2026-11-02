/* PDSRegistryCreatePipes (retail func_0039ba80): builds the pipe of every registry entry
 * that was added before the registry was live, then marks the registry live (flags = 1).
 * list = {items, count, capacity, flags}; entries are 20 bytes {spec, parentKey, key,
 * type, pipe} (func_0039b8d0 appends them).
 *
 * Pass 1 builds the pipe of every type-1 entry from its spec (func_0039bbd0) and stores
 * the entry key in the pipe's pluginData (pipe + 0x30). Pass 2 lets func_0039b720 build
 * the type-2 entries, which derive from pipes made in pass 1 or earlier in pass 2.
 *
 * Direct transliteration of the retail code. */

extern void *func_0039bbd0(u32 a0);
extern void func_0039b720(u32 *entry, u32 *list);

void PDSRegistryCreatePipes(s32 list_arg)
{
    u32 *list = (u32 *)list_arg;
    u32 i;

    for (i = 0; i < list[1]; i++) {
        u32 *entry = (u32 *)list[0] + i * 5;

        if (entry[3] == 1) {
            entry[4] = (u32)func_0039bbd0(entry[0]);
            ((u32 *)entry[4])[12] = entry[2];
        }
    }
    for (i = 0; i < list[1]; i++)
        func_0039b720((u32 *)list[0] + i * 5, list);
    list[3] = 1;
}
