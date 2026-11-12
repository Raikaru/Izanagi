/* PS2All nodeBody (retail 0x003cf140). The retail body opens a sky2 DMA packet and then calls
 * func_003cf610 with a stack packet {privateData, 0, atomic}. The port drops the DMA packet
 * and keeps the same packet shape, so the GL draw in func_003cf610 is the only draw path.
 * node is the RxPipelineNode (privateData at +20); params is the RxPipelineNodeParam (dataParam
 * at +0, the atomic). */
extern s32 func_003cf610(void *packet, void *unused, s32 nI);

s32 func_003cf140(void *node, void *params, s32 nI)
{
    void *packet[3];

    (void)nI;
    packet[0] = *(void **)((u8 *)node + 20);
    packet[1] = NULL;
    packet[2] = *(void **)params;
    return func_003cf610(packet, NULL, 0);
}
