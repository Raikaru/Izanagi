/* func_0039b720 (PDS registry: build the pipe of a type-2 entry): a type-2 entry is a
 * pipe derived from another registered pipe. entry = {spec, parentKey, key, type, pipe},
 * list = {items, count, capacity, flags}; both are 32-bit word arrays here.
 *
 * For type 2 the spec's word 4 (+0x10, the parent pipe) is set first: to the pipe of the
 * list item whose key equals parentKey, or to 0 when parentKey is 0. When parentKey is
 * non-zero and no item has that key, the spec word is left unchanged. The pipe is then
 * built by func_0039bdf0 from the spec, stored in entry word 4, and its pluginData
 * (pipe + 0x30) is set to the entry key. Other types are ignored.
 *
 * Direct transliteration of the retail code. */

extern u8 *func_0039bdf0(u8 *spec);

void func_0039b720(u32 *entry, u32 *list)
{
    u32 *spec;
    u32 parent_key;
    u32 count;
    u32 *item;
    u32 i;

    if ((s32)entry[3] != 2)
        return;

    parent_key = entry[1];
    spec = (u32 *)entry[0];
    if (parent_key == 0) {
        spec[4] = 0;
    } else {
        count = list[1];
        item = (u32 *)list[0];
        for (i = 0; i < count; i++, item += 5) {
            if (item[2] == parent_key) {
                spec[4] = item[4];
                break;
            }
        }
    }
    entry[4] = (u32)func_0039bdf0((u8 *)spec);
    ((u32 *)entry[4])[12] = entry[2];
}
