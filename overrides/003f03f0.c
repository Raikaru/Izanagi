/* Remove leaf b from leaf a into dst: moments subtract; the variance term
 * gets the cross term against the remainder when both weights are positive. */
void func_003f03f0(float *dst, const float *a, const float *b)
{
    float d1;
    float d2;
    float d3;
    float d4;
    float inv_b;
    float inv_d;
    float sum;
    s32 k;

    for (k = 0; k < 5; k++) {
        dst[k] = a[k] - b[k];
    }
    dst[5] = a[5] - b[5];
    if (dst[0] <= 0.0f || b[0] <= 0.0f) {
        return;
    }
    inv_b = 1.0f / b[0];
    inv_d = 1.0f / dst[0];
    d1 = b[1] * inv_b - dst[1] * inv_d;
    d2 = b[2] * inv_b - dst[2] * inv_d;
    d3 = b[3] * inv_b - dst[3] * inv_d;
    d4 = b[4] * inv_b - dst[4] * inv_d;
    sum = d1 * d1 + d2 * d2 + d3 * d3 + d4 * d4;
    dst[5] -= sum / (inv_b + inv_d);
}
