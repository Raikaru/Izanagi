/* func_003cad90 (220 bytes): walks the node list headed at obj+100 (sentinel
 * obj+100, next pointer at node+0). Each record is node-12. A record whose +8 and
 * +20 words are both set is unlinked (rec[4]->next = rec[0], rec[0]+4 = rec[4],
 * rec[16]->next = rec[12], rec[12]+4 = rec[16]) and passed to jtbl_008873FC[0] with
 * the parameter at 0x008872E0 + [gp-18664]. Finally obj+16 = [obj + [gp-18676] + 4].
 * Returns obj. */

extern void (*jtbl_008873FC[])(s32 param, u8 *arg);

u8 *func_003cad90(u8 *arg0)
{
    u8 *node;
    u8 *rec;
    s32 off;

    off = *(s32 *)(0x007690F0 - 18676);

    node = *(u8 **)(arg0 + 100);
    while (node != arg0 + 100) {
        rec = node - 12;
        node = *(u8 **)node;
        if (*(s32 *)(rec + 8) != 0 && *(s32 *)(rec + 20) != 0) {
            *(u8 **)*(u8 **)(rec + 4) = *(u8 **)rec;
            *(u8 **)(*(u8 **)rec + 4) = *(u8 **)(rec + 4);
            *(u8 **)*(u8 **)(rec + 16) = *(u8 **)(rec + 12);
            *(u8 **)(*(u8 **)(rec + 12) + 4) = *(u8 **)(rec + 16);
            jtbl_008873FC[0](*(s32 *)(0x008872E0 + *(s32 *)(0x007690F0 - 18664)), rec);
        }
    }

    *(u32 *)(arg0 + 16) = *(u32 *)(arg0 + off + 4);
    return arg0;
}
