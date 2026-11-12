/* Retail 004da5b8: output pan, the top five bits of a pan byte sign-extended (byte << 24 >> 27).
 * Inactive channel: 0. Mode word +0x1e8 == 2 (two-channel): channel param_2 == 1 reads byte
 * +0x217 (enable +0x216, else 15), channel 0 reads +0x20b (enable +0x20a, else -15). Other
 * modes read +0x20b when +0x20a > 0, else 0. */
int AHXDCD_GetOutPan(int param_1, int param_2)
{
    u32 raw;

    if (*(s8 *)(param_1 + 0x1a5) == 0) {
        return 0;
    }
    if (*(int *)(param_1 + 0x1e8) != 2) {
        if (*(s8 *)(param_1 + 0x20a) <= 0) {
            return 0;
        }
        raw = *(u8 *)(param_1 + 0x20b);
    } else if (param_2 != 1) {
        if (*(s8 *)(param_1 + 0x20a) <= 0) {
            return -15;
        }
        raw = *(u8 *)(param_1 + 0x20b);
    } else {
        if (*(s8 *)(param_1 + 0x216) <= 0) {
            return 15;
        }
        raw = *(u8 *)(param_1 + 0x217);
    }
    return (int)(raw << 24) >> 27;
}
