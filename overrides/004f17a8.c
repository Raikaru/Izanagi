/* Retail 004f17a8 (rofs hn): tells whether the image handle's entry table holds an in-use entry
 * (kind 1, stride 0x3c, key at *(entry + 0x1c)) matching `key`. With param_2 == 0 the key is read
 * through the handle (*(*(handle + 0x1c))). An in-use entry with a null key pointer does not
 * advance the entry pointer, exactly as the retail loop (the count still runs out). */
u32 RFHN_IsUseImgHn(long param_1, int param_2)
{
    int base;
    int entry;
    int key;
    int i;
    int count;

    if (param_1 == 0) {
        return 0;
    }
    base = (int)param_1;
    if (param_2 != 0) {
        key = param_2;
        base = *(int *)(base + 8);
    } else {
        key = **(int **)(base + 0x1c);
        base = *(int *)(base + 8);
    }
    count = *(int *)(base + 4);
    if (count <= 0) {
        return 0;
    }
    entry = base + 0x108c;
    for (i = 0; i < count; i++) {
        if (*(short *)(entry + 0x36) == 1) {
            if (*(int **)(entry + 0x1c) != 0) {
                if (**(int **)(entry + 0x1c) == key) {
                    return 1;
                }
                entry += 0x3c;
            }
        } else {
            entry += 0x3c;
        }
    }
    return 0;
}
