/* func_003d6680 (rpuvanim, 260 bytes): stream reader for the UV-anim keyframe
 * table. Reads a 4-byte header (unused), stores func_003d7a20(stream) at obj+20,
 * then for each of obj[+4] records: reads 28 bytes into record+4, reads a 4-byte
 * index, and sets record[0] = base + index * 32 (base = obj+16). Returns obj, or
 * NULL on any read failure. */

extern s32 func_003df300(s32 arg0, void *arg1, s32 arg2);

void *func_003d6680(u8 *arg0, u8 *arg1)
{
    s32 header;
    s32 index;
    s32 count;
    s32 i;
    void *ext;
    u8 *base;
    u8 *record;

    if (func_003df360((s32)arg0, &header, 4) == 0) {
        return NULL;
    }

    ext = func_003d7a20((s32)arg0);
    if (ext == NULL) {
        return NULL;
    }
    *(void **)(arg1 + 20) = ext;

    count = *(s32 *)(arg1 + 4);
    base = *(u8 **)(arg1 + 16);
    if (count <= 0) {
        return arg1;
    }

    for (i = 0; i < count; i++) {
        record = base + i * 32;
        if (func_003df300((s32)arg0, record + 4, 28) == 0) {
            return NULL;
        }
        if (func_003df360((s32)arg0, &index, 4) == 0) {
            return NULL;
        }
        *(u8 **)record = base + (index << 5);
    }
    return arg1;
}
