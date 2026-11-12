/* func_004147a0: finalises a registry after its entries were added. Runs func_00413700, then
 * func_00413b80(reg), func_00413e50(reg, &var) and func_004140f0(reg, &var), returning the first
 * non-zero result; when all pass it runs func_004143f0(reg, var), which compacts the table, and
 * returns 0 (retail's func_004143f0 always returns 0). */
s32 func_004147a0(u8 *reg)
{
    s32 var = 0;
    s32 r;

    r = func_00413700((s32)reg);
    if (r != 0) {
        return r;
    }
    r = func_00413b80((s32)reg);
    if (r != 0) {
        return r;
    }
    r = func_00413e50((s32)reg, &var);
    if (r != 0) {
        return r;
    }
    r = func_004140f0(reg, (u8 **)&var);
    if (r != 0) {
        return r;
    }
    func_004143f0((s32)reg, var);
    return 0;
}
