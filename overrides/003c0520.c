/* func_003c0520 (276 bytes): clones an object. Returns the clone, or 0 on failure.
 *
 * func_003c0640(arg0) makes the clone (0 on failure). Its word +4 = func_003e90f0(arg0[4]); a null
 * result destroys the clone with func_003c0700 and returns 0. Otherwise every entry of the circular list
 * headed at arg0 + 8 (an entry sits 0x40 bytes before its list node) goes to func_003bea60(entry,
 * &state) where state = { arg0, clone, ok = 1 }; the walk stops when it returns 0 or ends. Then
 * func_003e9140(arg0[4]) runs; when the callee cleared state.ok the clone is destroyed (func_003c0700)
 * and 0 returned; otherwise the clone's +0x28 = arg0[0x28], func_003e3c90(0x0070AF90, clone, arg0)
 * registers it and the clone is returned. */

extern s32 p4c_003bea60_003c0520(u8 *e, void *st) __asm__("func_003bea60");
extern u8 *p4c_003c0640_003c0520(u8 *o) __asm__("func_003c0640");
extern void p4c_003c0700_003c0520(u8 *o) __asm__("func_003c0700");
extern void p4c_003e3c90_003c0520(u32 l, u8 *a, u8 *b) __asm__("func_003e3c90");
extern u32 p4c_003e90f0_003c0520(u32 a) __asm__("func_003e90f0");
extern void p4c_003e9140_003c0520(u32 a) __asm__("func_003e9140");

typedef struct P4CloneState_003c0520 {
    u8 *src;
    u8 *dst;
    s32 ok;
} P4CloneState_003c0520;

u8 *func_003c0520(u8 *arg0)
{
    P4CloneState_003c0520 st;
    u8 *head;
    u8 *node;
    u8 *next;

    st.src = arg0;
    st.dst = (u8 *)p4c_003c0640_003c0520(arg0);
    if (st.dst == (u8 *)0) {
        return (u8 *)0;
    }
    *(u32 *)(st.dst + 4) = p4c_003e90f0_003c0520(*(u32 *)(st.src + 4));
    if (*(u32 *)(st.dst + 4) == 0) {
        p4c_003c0700_003c0520(st.dst);
        return (u8 *)0;
    }
    st.ok = 1;
    head = st.src + 8;
    node = *(u8 **)head;
    if (node != head) {
        do {
            next = *(u8 **)node;

            if (p4c_003bea60_003c0520(node - 0x40, &st) == 0) {
                break;
            }
            node = next;
        } while (next != head);
    }
    p4c_003e9140_003c0520(*(u32 *)(st.src + 4));
    if (st.ok == 0) {
        p4c_003c0700_003c0520(st.dst);
        return (u8 *)0;
    }
    *(u32 *)(st.dst + 0x28) = *(u32 *)(st.src + 0x28);
    p4c_003e3c90_003c0520(0x0070AF90, st.dst, st.src);
    return st.dst;
}
