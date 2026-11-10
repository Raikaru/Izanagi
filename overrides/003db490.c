/* func_003db490 (192 bytes): the render job callback queued by func_003db550 (arg1 is the job record the caller built: +4 the texture-chain owner, +8 written here, +0xC the
 * float sum and +0x10 the count the walk leaves). arg0 (the first argument of the job call) is stored at +8, the sum and the count are cleared, and func_003ca320(record +4,
 * func_003db440, record) walks the chain with func_003db440 adding to the sum and the count. When it counted something, the mean (sum / count) goes through the logarithm
 * func_0044b968 and the sky mipmap K of arg0 is set to -(log(mean) / log(2)) with func_003f6800. Returns arg0. */

extern void *p4c_003ca320_003db490(void *chain, void *cb, void *arg) __asm__("func_003ca320");
extern void p4c_003db440_003db490(void) __asm__("func_003db440");
extern f32 p4c_0044b968_003db490(f32 x) __asm__("func_0044b968");
extern void p4c_003f6800_003db490(void *tex, f32 k) __asm__("func_003f6800");

void *func_003db490(void *arg0, u8 *arg1)
{
    f32 mean;
    f32 value;
    f32 base;
    s32 count;

    *(void **)(arg1 + 8) = arg0;
    *(s32 *)(arg1 + 0xC) = 0;
    *(s32 *)(arg1 + 0x10) = 0;
    p4c_003ca320_003db490(*(void **)(arg1 + 4), (void *)p4c_003db440_003db490, arg1);
    count = *(s32 *)(arg1 + 0x10);
    if (count > 0) {
        mean = *(f32 *)(arg1 + 0xC) / (f32)count;
        value = p4c_0044b968_003db490(mean);
        base = p4c_0044b968_003db490(2.0f);
        value = value / base;
        p4c_003f6800_003db490(arg0, -value);
    }
    return arg0;
}
