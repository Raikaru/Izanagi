/* Retail 004f14e0..004f1514. The native R5900 draft lost both the live
 * handle passed to 004e3448 and the returned value. That query returns a
 * full 64-bit transfer count, not a 32-bit readiness flag. Only a genuinely
 * zero count falls back to the legacy signed 32-bit transfer-count tail.
 */
u64 func_004f14e0(void *handle)
{
    u64 count = func_004e3448(handle);

    if (count != 0) {
        return count;
    }
    return (u64)(s64)func_004f14c8(handle);
}
