/* Retail 004da570: effect send levels. *param_2 = 0 and *param_3 = -45 unless the channel is
 * active (+0x1a5); then *param_2 = byte +0x207 when byte +0x206 > 0, and *param_3 = byte +0x209
 * when byte +0x208 > 0. All bytes are signed. */
void AHXDCD_GetFx(int param_1, int *param_2, int *param_3)
{
    *param_2 = 0;
    *param_3 = -0x2d;
    if (*(s8 *)(param_1 + 0x1a5) != 0) {
        if (*(s8 *)(param_1 + 0x206) > 0) {
            *param_2 = *(s8 *)(param_1 + 0x207);
        }
        if (*(s8 *)(param_1 + 0x208) > 0) {
            *param_3 = *(s8 *)(param_1 + 0x209);
        }
    }
}
