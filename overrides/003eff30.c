/* Accumulate one palette colour into an octree leaf (retail 003eff30).
 * count is the per-channel bit depth, read from gp-21552 (0x007690F0 - 21552).
 * Channels are masked to (8 - count) significant bits; n_i = c_i / 255.
 * leaf[0] += w; leaf[1] += (n0*K1)*w; leaf[2] += n1*w; leaf[3] += (n2*K2)*w;
 * leaf[4] += n3*w; leaf[5] += w*(n1^2 + (n0*K1)^2 + (n2*K2)^2 + n3^2). */
void func_003eff30(float *leaf, const u8 *px, float w)
{
    const float inv255 = 0.0039215688f;
    const float k1 = 0.50936949f;
    const float k2 = 0.19420784f;
    s32 count = *(s32 *)(0x007690F0 - 21552);
    u32 mask = (1u << (8 - count)) - 1u;
    float n0 = (float)(px[0] & mask) * inv255;
    float n1 = (float)(px[1] & mask) * inv255;
    float n2 = (float)(px[2] & mask) * inv255;
    float n3 = (float)(px[3] & mask) * inv255;
    float a = n0 * k1;
    float b = n2 * k2;
    float sq = n1 * n1 + a * a + b * b + n3 * n3;

    leaf[0] += w;
    leaf[1] += a * w;
    leaf[2] += n1 * w;
    leaf[3] += b * w;
    leaf[4] += n3 * w;
    leaf[5] += w * sq;
}
