/* Retail 004f6a70: copies the 7 TIM-info bytes. */
void cvmh_GetTimInfo(u8 *param_1, u8 *param_2)
{
    param_2[0] = param_1[0];
    param_2[1] = param_1[1];
    param_2[2] = param_1[2];
    param_2[3] = param_1[3];
    param_2[4] = param_1[4];
    param_2[5] = param_1[5];
    param_2[6] = param_1[6];
}
