/* Retail 00463ea0 copies whole quadwords through the 16 KiB scratchpad with EE DMAC channels 9 and 8
 * (MMIO at 0x1000Dxxx, BC0F waits on the hardware condition line). The result is a plain copy of
 * (byte_count >> 4) quadwords, so the port does the copy directly; func_00421a60(0) and
 * _rwDMAOpenVIFPktInline only prepare the DMA and VIF path that this copy no longer uses. */
void tmxDmaCopy(u32 destination, u32 source, u32 byte_count)
{
    memmove((void *)destination, (const void *)source, (size_t)((byte_count >> 4) << 4));
}
