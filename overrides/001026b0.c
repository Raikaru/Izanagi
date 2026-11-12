/* The original field gate reads GS CSR.FIELD (bit 13 at 0x12001000), not
 * INTC_STAT. A negative return stops the VBLANK chain on the rejected field.
 * Only the hardware register access is replaced; the game's predicate is kept. */
extern u64 p4pc_gs_csr_read(void);

s32 FldShadow_PollIntcBit13(void)
{
    if (((p4pc_gs_csr_read() >> 13) & 1) == 0)
    {
        return -1;
    }
    return 0;
}
