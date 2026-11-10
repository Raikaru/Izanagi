/* func_003d6aa0 (rpuvanim, 204 bytes): floats 2..7 of arg0 transformed from
 * src = arg1. Let det = s2*s5 - s3*s4 (s[k] = src[k]). When det != 0, with
 * inv = 1/det, the pairs (d2,d4) and (d3,d5) of arg0 are rewritten from their
 * old values:
 *   d2' = s5*inv*d2 - s3*inv*d4
 *   d3' = s5*inv*d3 - s3*inv*d5
 *   d4' = -s4*inv*(d2 + d4)
 *   d5' = -s4*inv*(d3 + d5)
 * Then d6 -= s6 and d7 -= s7 on every path. The retail code writes the
 * floats at arg0+8.. and reads src at arg1+8..+28. */

void func_003d6aa0(u8 *arg0, u8 *arg1)
{
    float *dst = (float *)(arg0 + 8);
    float *src = (float *)arg1;
    float s2 = src[2];
    float s3 = src[3];
    float s4 = src[4];
    float s5 = src[5];
    float det;
    float inv;
    float a;
    float b;
    float c;
    float d0;
    float d1;
    float d2;
    float d3;

    det = s2 * s5 - s3 * s4;
    if (det != 0.0f) {
        inv = 1.0f / det;
        a = s5 * inv;
        b = -s4 * inv;
        c = -s3 * inv;
        d0 = dst[0];
        d1 = dst[1];
        d2 = dst[2];
        d3 = dst[3];
        dst[0] = a * d0 + c * d2;
        dst[1] = a * d1 + c * d3;
        dst[2] = b * d2 + b * d0;
        dst[3] = b * d3 + b * d1;
    }
    dst[4] -= src[6];
    dst[5] -= src[7];
}
