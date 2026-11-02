/* Merge two leaves into dst: moments add; the variance term gets the
 * between-box cross term when both weights are positive. */
void func_003f02d0(float *dst, const float *a, const float *b)
{
    float d1;
    float d2;
    float d3;
    float d4;
    float inv_a;
    float inv_b;
    float sum;
    s32 k;

    dst[5] = a[5] + b[5];
    if (a[0] > 0.0f && b[0] > 0.0f) {
        inv_a = 1.0f / a[0];
        inv_b = 1.0f / b[0];
        d1 = a[1] * inv_a - b[1] * inv_b;
        d2 = a[2] * inv_a - b[2] * inv_b;
        d3 = a[3] * inv_a - b[3] * inv_b;
        d4 = a[4] * inv_a - b[4] * inv_b;
        sum = d1 * d1 + d2 * d2 + d3 * d3 + d4 * d4;
        dst[5] += sum / (inv_a + inv_b);
    }
    for (k = 0; k < 5; k++) {
        dst[k] = a[k] + b[k];
    }
}
