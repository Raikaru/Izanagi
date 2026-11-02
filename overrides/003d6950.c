/* func_003d6950 (rpuvanim, 172 bytes): linear keyframe blend of floats 2..7.
 * ratio = (t - a[1]) / (b[1] - a[1]); dst[i] = a[i] + ratio * (b[i] - a[i])
 * for i = 2..7. Floats 0 and 1 are not written (retail stores at +8..+28). */

void func_003d6950(u8 *arg0, u8 *arg1, u8 *arg2, float arg3)
{
    float *dst = (float *)arg0;
    float *a = (float *)arg1;
    float *b = (float *)arg2;
    float ratio;
    s32 i;

    ratio = (arg3 - a[1]) / (b[1] - a[1]);
    for (i = 2; i < 8; i++) {
        dst[i] = a[i] + ratio * (b[i] - a[i]);
    }
}
