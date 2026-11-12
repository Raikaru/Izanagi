/* Leaf variance: leaf[5] = max(0, leaf[5] - sum(moment^2) / weight), then
 * add the raw colour (scaled by the same constants) times the weight into the
 * first-moment slots. Literal transliteration of the retail order. */
void func_003f0110(float *leaf, const u8 *px)
{
    const float inv255 = 0.0039215688f;
    const float k1 = 0.50936949f;
    const float k2 = 0.19420784f;
    float var = leaf[5] - (leaf[1] * leaf[1] + leaf[2] * leaf[2]
                           + leaf[3] * leaf[3] + leaf[4] * leaf[4]) / leaf[0];
    float r = (float)px[0] * inv255;
    float g = (float)px[1] * inv255;
    float b = (float)px[2] * inv255;
    float a = (float)px[3] * inv255;

    leaf[5] = (var < 0.0f) ? 0.0f : var;
    leaf[1] += (r * k1) * leaf[0];
    leaf[2] += g * leaf[0];
    leaf[3] += (b * k2) * leaf[0];
    leaf[4] += a * leaf[0];
}
