/* Retail 004f1460..004f14ac. The native romwright R5900 draft dropped
 * the live a0 at the cvFsGetStat call. Its result is signed 32-bit:
 * states 0, 1 and 2 pass through; every other result maps to state 3.
 */
s32 func_004f1460(void *handle)
{
    s32 status = func_004e22f8(handle);

    return (u32)status <= 2 ? status : 3;
}
