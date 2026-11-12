/* func_003d60f0 (124 bytes): calls the destroy slot of an object for every element of its list. arg0+4
 * is the list; func_003df8a0(list) is the end iterator and func_003df890(list) the begin one (the end is
 * fetched first). For each element pointer p from begin to end (step 4) the function at
 * (*arg0)[0x1C] is called with *p. Then func_003df7f0(list) runs. Returns 1. */

extern void p4c_003df7f0_003d60f0(s32 a) __asm__("func_003df7f0");
extern s32 p4c_003df890_003d60f0(s32 a) __asm__("func_003df890");
extern s32 p4c_003df8a0_003d60f0(s32 a) __asm__("func_003df8a0");

typedef void (*P4ElemFn_003d60f0)(u32 elem);

s32 func_003d60f0(u8 *arg0)
{
    u32 *end;
    u32 *p;

    end = (u32 *)p4c_003df8a0_003d60f0(*(s32 *)(arg0 + 4));
    p = (u32 *)p4c_003df890_003d60f0(*(s32 *)(arg0 + 4));
    if (p != end) {
        do {
            u32 elem = *p;

            p += 1;
            (*(P4ElemFn_003d60f0 *)(*(u8 **)arg0 + 0x1C))(elem);
        } while (p != end);
    }
    p4c_003df7f0_003d60f0(*(s32 *)(arg0 + 4));
    return 1;
}
