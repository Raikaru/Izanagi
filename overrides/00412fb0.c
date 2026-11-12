extern s32 func_003df590();
extern u8 *func_003df4d0();
extern s32 func_00411ce0(u8 *reg, u8 *entry, s32 id);
extern u8 *func_00413290(u8 *reg, u32 *out, u8 *entry);
extern void func_00411130(u8 *entry, u8 *reg);

/* func_00412fb0: appends one table entry per non-zero id in the zero-terminated variadic list
 * to the open registry `reg` (count at +4, 40-byte entries at +8) and returns `reg`; *out_base
 * (when given) receives the entry count before the append. The list is read twice: the first
 * pass counts the ids, and the registry must have room for them under the engine's
 * per-instance limit (*(0x00764AA8 + 0x887318)). Each new entry is initialised with
 * func_00411ce0(reg, entry, id); from the second entry on, func_00413290 links the previous
 * entry's word 2 to it (when the previous entry is live: words 0 and 1 non-zero). A failure
 * destroys the entries added so far through func_00411130 and returns 0.
 * Errors, each {1, code}: reg == 0 -> 0x80000016; reg not open (word 0 is 0) -> 52; no room -> 42. */
u8 *func_00412fb0(u8 *reg, s32 *out_base, ...)
{
    __builtin_va_list ap;
    __builtin_va_list ap2;
    s32 err[2];
    s32 id;
    s32 n;
    s32 base;
    s32 done;
    u8 *entry;
    u8 *prev;
    u32 *link;

    if (reg == 0) {
        err[0] = 1;
        err[1] = func_003df590(0x80000016);
        func_003df4d0(err);
        return 0;
    }
    if (*(s32 *)reg == 0) {
        err[0] = 1;
        err[1] = func_003df590(52);
        func_003df4d0(err);
        return 0;
    }

    __builtin_va_start(ap, out_base);
    __builtin_va_copy(ap2, ap);
    n = 0;
    for (id = __builtin_va_arg(ap2, s32); id != 0; id = __builtin_va_arg(ap2, s32)) {
        n++;
    }
    __builtin_va_end(ap2);
    if (n == 0) {
        __builtin_va_end(ap);
        return 0;
    }

    base = *(s32 *)(reg + 4);
    if ((u32)(*(s32 *)(*(s32 *)0x00764AA8 + 0x887318)) < (u32)(base + n)) {
        err[0] = 1;
        err[1] = func_003df590(42);
        func_003df4d0(err);
        __builtin_va_end(ap);
        return 0;
    }

    prev = 0;
    done = 0;
    id = __builtin_va_arg(ap, s32);
    while (id != 0) {
        entry = *(u8 **)(reg + 8) + (base + done) * 40;
        if (func_00411ce0(reg, entry, id) == 0) {
            break;
        }
        if (prev != 0) {
            if (*(s32 *)prev != 0 && *(s32 *)(prev + 4) != 0) {
                link = *(u32 **)(prev + 8);
            } else {
                link = 0;
            }
            if (func_00413290(reg, link, (entry != 0 && *(s32 *)entry != 0) ? entry : 0) == 0) {
                func_00411130(entry, reg);
                break;
            }
        }
        prev = entry;
        done++;
        id = __builtin_va_arg(ap, s32);
    }
    __builtin_va_end(ap);

    if (done == n) {
        if (out_base != 0) {
            *out_base = base;
        }
        return reg;
    }
    while (done != 0) {
        done--;
        func_00411130(*(u8 **)(reg + 8) + (done + base) * 40, reg);
    }
    return 0;
}
