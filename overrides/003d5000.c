/* func_003d5000: registers one 48-byte record (12 words, word 0 = plugin id)
 * in the 16-entry table at retail 0x00886E50. The record count lives at
 * retail 0x00764824 (gp-relative). Returns 1 when stored; returns 0 when the
 * id is already present (error 0) or the table is full (error 1). The error
 * path reports pluginID 439 through _rwerror/RwErrorSet, as the retail code. */

typedef struct P4RwError {
    u32 pluginID;
    s32 errorCode;
} P4RwError;

s32 func_003d5000(u32 *arg0)
{
    u32 *table = (u32 *)0x00886E50;
    s32 *count = (s32 *)0x00764824;
    P4RwError err;
    s32 i;

    if (*count >= 16) {
        err.pluginID = 439;
        err.errorCode = func_003df590(1);
        func_003df4d0((void *)&err);
        return 0;
    }

    for (i = 0; i < *count; i++) {
        if (arg0[0] == table[i * 12]) {
            err.pluginID = 439;
            err.errorCode = func_003df590(0);
            func_003df4d0((void *)&err);
            return 0;
        }
    }

    for (i = 0; i < 12; i++) {
        table[*count * 12 + i] = arg0[i];
    }
    *count += 1;
    return 1;
}
