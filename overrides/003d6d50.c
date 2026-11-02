/* func_003d6d50 (rpuvanim, 292 bytes): keyframe blend like func_003d6950 with
 * an angle wrap on float 2. ratio = (t - a[1]) / (b[1] - a[1]). w = b[2] - a[2]
 * is moved into [-pi, pi] (add 2*pi if w < -pi, subtract 2*pi if w > pi), then
 * out[2] = a[2] + ratio * w. out[i] = a[i] + ratio * (b[i] - a[i]) for i = 3..7. */

void func_003d6d50(u8 *arg0, u8 *arg1, u8 *arg2, float arg3)
{
    float *out = (float *)arg0;
    float *a = (float *)arg1;
    float *b = (float *)arg2;
    float ratio;
    float w;
    s32 i;

    ratio = (arg3 - a[1]) / (b[1] - a[1]);

    w = b[2] - a[2];
    if (w < -3.14159274f) {
        w += 6.28318548f;
    } else if (w > 3.14159274f) {
        w -= 6.28318548f;
    }
    out[2] = a[2] + ratio * w;

    for (i = 3; i < 8; i++) {
        out[i] = a[i] + ratio * (b[i] - a[i]);
    }
}
