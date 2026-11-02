/* ahxsbf_mult_win4 (AHX subband synthesis window, retail 004dbec8): 32 output samples, each the sum of
 * 16 products of a coefficient (param_2, 16 per output) with a sample at fixed offsets of the
 * circular buffer (param_1, advancing by one sample per output), clamped to 16 bits after a
 * shift right by 11 (sums above 0x3fff800 give 0x7fff, below -0x4000000 give 0x8000). The 16
 * window variants differ only in which offsets wrap back into the buffer. */
void ahxsbf_mult_win4(short *param_1, short *param_2, u16 *param_3)
{
    static const int offsets[16] = { 0, 96, 128, 224, 256, 352, 384, 480, 512, 608, 640, 736, -256, -160, -128, -32 };
    int n, k;

    for (n = 0; n < 32; n++) {
        int sum = 0;

        for (k = 0; k < 16; k++) {
            sum += (int)param_2[k] * (int)param_1[offsets[k]];
        }
        param_2 += 16;
        if (sum >= 0x3fff801) {
            *param_3 = 0x7fff;
        } else if (sum >= -0x4000000) {
            *param_3 = (u16)(sum >> 11);
        } else {
            *param_3 = 0x8000;
        }
        param_3++;
        param_1++;
    }
}
