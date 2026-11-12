/* Leaf to palette colour: each channel is the weighted mean, scaled to 0..255
 * (truncate after multiplying by 255.99, clamp at 255). Channels 0 and 2 undo
 * the K1 and K2 factors applied in func_003eff30. */
void func_003f0510(u8 *out, const float *leaf)
{
    const float k1 = 0.50936949f;
    const float k2 = 0.19420784f;
    const float scale = 255.99f;
    float invw = 1.0f / leaf[0];
    float m[4];
    s32 i;

    m[0] = leaf[1] * invw / k1;
    m[1] = leaf[2] * invw;
    m[2] = leaf[3] * invw / k2;
    m[3] = leaf[4] * invw;
    for (i = 0; i < 4; i++) {
        s32 v = (s32)(scale * m[i]);
        out[i] = (v < 255) ? (u8)v : 255;
    }
}
