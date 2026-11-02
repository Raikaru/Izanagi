/* Retail 004da530: the output volume field (+0x202) when the channel is active (+0x1a5) and the
 * volume enable halfword (+0x200) is set, else 0. */
u16 AHXDCD_GetOutVol(int param_1)
{
    s16 v = 0;

    if (*(s8 *)(param_1 + 0x1a5) != 0 && *(s16 *)(param_1 + 0x200) != 0) {
        v = *(s16 *)(param_1 + 0x202);
    }
    return (u16)v;
}
