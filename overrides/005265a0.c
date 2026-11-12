/* Retail 005265a0: converts a time record (param_2: +8 hours, +0xc minutes, +0x10 seconds,
 * +0x14 and +0x18 milliseconds parts, halfword +0x1e frames) at rate param_1 into a tick count
 * (*param_3) and the rate (*param_4). The register v0 at return is the hours, minutes and
 * milliseconds part of the sum; the callers pass it on, so it is returned. Plain C types: the
 * declarations of func_<addr> in every unit (some without the decomp's typedefs) take this
 * signature. */
int func_005265a0(int param_1, int param_2, int *param_3, int *param_4)
{
    unsigned char *rec = (unsigned char *)param_2;
    int hours = *(int *)(rec + 8);
    int minutes = *(int *)(rec + 0xc);
    int seconds = *(int *)(rec + 0x10);
    int ms_a = *(int *)(rec + 0x14);
    int ms_b = *(int *)(rec + 0x18);
    short frames = *(short *)(rec + 0x1e);
    int part = hours * param_1 * 0xe10 + minutes * param_1 * 0x3c + (ms_a + ms_b) * 1000;

    *param_3 = seconds * param_1 + (frames * 0x7c + frames) * 4 + part;
    *param_4 = param_1;
    return part;
}
