/* func_003d6e80 (rpuvanim, 268 bytes): as func_003d6d50, but the caller's
 * ratio is passed directly as arg3 (no division by the time span).
 * w = b[2] - a[2] is wrapped into [-pi, pi]; out[2] = a[2] + t * w;
 * out[i] = a[i] + t * (b[i] - a[i]) for i = 3..7. */

void func_003d6e80(u8 *arg0, u8 *arg1, u8 *arg2, float arg3)
{
    float *out = (float *)arg0;
    float *a = (float *)arg1;
    float *b = (float *)arg2;
    float w;
    s32 i;

    w = b[2] - a[2];
    if (w < -3.14159274f) {
        w += 6.28318548f;
    } else if (w > 3.14159274f) {
        w -= 6.28318548f;
    }
    out[2] = a[2] + arg3 * w;

    for (i = 3; i < 8; i++) {
        out[i] = a[i] + arg3 * (b[i] - a[i]);
    }
}
