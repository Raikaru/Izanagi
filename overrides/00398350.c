/* func_00398350 (module 0x120 open callback, registered by func_003999a0 through
 * func_003e8930; func_003982e0 is the matching close): reference-counted set-up. The first open
 * creates the module's 0x90-byte-entry free list into D_00884A80 (in-place free-list header
 * D_00884A90, hint 0x00040120) and runs func_0039aa30; a failure of either returns 0.
 * Returns arg0 on success and counts the instance in D_00884A7C.
 *
 * This is the decomp's NON_MATCHING body (same behaviour as retail; only MWCC's argument
 * scheduling differs), checked against the retail asm. */

s32 func_00398350(s32 arg0)
{
    s32 list;

    if (D_00884A7C[0] == 0) {
        list = func_003e1220(0x90, iGpffffaa08, 0x10, iGpffffaa0c, D_00884A90, 0x40120);
        D_00884A80[0] = list;
        if (list == 0)
            return 0;
        if (func_0039aa30() == 0)
            return 0;
    }
    D_00884A7C[0] += 1;
    return arg0;
}
