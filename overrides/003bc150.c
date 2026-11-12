/* func_003bc150 (688 bytes): reads a name list record from the stream arg1 into arg0 (the layout func_003bc740 frees and
 * func_003bc620 sizes). Returns arg1, or 0 as soon as a read or allocation fails (allocator slot 0x008873E8, tag 0x3011F).
 *
 * A word n is read; when it is positive a string of n bytes (func_003e2910) is allocated into arg0[0] (it is left untouched
 * for n <= 0). Then the type word arg0[1] and the count arg0[2] are read. Type 1 allocates count words into arg0[3] and reads them
 * (func_003df360, count * 4 bytes), type 2 does the same with floats (func_003df300), and type 3 allocates an array of
 * count string pointers and reads each as a length word plus that many bytes (pointer 0 when the length is not positive). Other types return 0. */

extern s32 p4c_003df300_003bc150(s32 stream, void *p, s32 n) __asm__("func_003df300");
extern s32 p4c_003df360_003bc150(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern s32 p4c_003e2910_003bc150(s32 s, void *b, s32 n) __asm__("func_003e2910");

typedef void *(*P4AllocFn_003bc150)(u32 size, u32 tag);

u32 func_003bc150(u32 *arg0, s32 arg1)
{
    P4AllocFn_003bc150 *slot = (P4AllocFn_003bc150 *)0x008873E8;
    s32 n;
    s32 len;
    s32 i;
    u32 *list;

    if (p4c_003df360_003bc150(arg1, &n, 4) == 0) {
        return 0;
    }
    if (n > 0) {
        arg0[0] = (u32)(*slot)((u32)n, 0x3011F);
        if (arg0[0] == 0) {
            return 0;
        }
        if (p4c_003e2910_003bc150(arg1, (void *)arg0[0], n) == 0) {
            return 0;
        }
    }
    if (p4c_003df360_003bc150(arg1, &arg0[1], 4) == 0) {
        return 0;
    }
    if (p4c_003df360_003bc150(arg1, &arg0[2], 4) == 0) {
        return 0;
    }
    switch (arg0[1]) {
    case 3:
        arg0[3] = (u32)(*slot)(arg0[2] << 2, 0x3011F);
        list = (u32 *)arg0[3];
        if (list == (u32 *)0) {
            return 0;
        }
        for (i = 0; i < (s32)arg0[2]; i++) {
            if (p4c_003df360_003bc150(arg1, &len, 4) == 0) {
                return 0;
            }
            if (len > 0) {
                list[i] = (u32)(*slot)((u32)len, 0x3011F);
                if (list[i] == 0) {
                    return 0;
                }
                if (p4c_003e2910_003bc150(arg1, (void *)list[i], len) == 0) {
                    return 0;
                }
            } else {
                list[i] = 0;
            }
        }
        return (u32)arg1;
    case 2:
        arg0[3] = (u32)(*slot)(arg0[2] << 2, 0x3011F);
        if (arg0[3] == 0) {
            return 0;
        }
        if (p4c_003df300_003bc150(arg1, (void *)arg0[3], (s32)(arg0[2] << 2)) == 0) {
            return 0;
        }
        return (u32)arg1;
    case 1:
        arg0[3] = (u32)(*slot)(arg0[2] << 2, 0x3011F);
        if (arg0[3] == 0) {
            return 0;
        }
        if (p4c_003df360_003bc150(arg1, (void *)arg0[3], (s32)(arg0[2] << 2)) == 0) {
            return 0;
        }
        return (u32)arg1;
    default:
        return 0;
    }
}
