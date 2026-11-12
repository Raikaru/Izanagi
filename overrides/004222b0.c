/* Retail 004222b0 returns (SR ^ 1) & 1: 1 when the CP0 status IE bit is clear. The port runs the
 * game on one host thread with no interrupt dispatch, so the status register's IE bit is always
 * set and the answer is 0. (DIntr/EIntr model only the EIE mask: src/platform/ps2_kernel.c.) */
u32 QueryIntrContext(void)
{
    return 0;
}
