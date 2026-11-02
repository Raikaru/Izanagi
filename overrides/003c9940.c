/* Retail 003c9940 (code1_003c: the C is only in the excluded code1_003c_cw119.c, so the asm was a trap): takes an element
 * from the free list at *(D_008872E0 + iGpffffb700) (allocator slot 0x008873F8, tag 0x40507), stores arg0 and arg1 in its
 * first two words and links its +8 node at the head of the list that starts at slot + 4 (the old first node's previous
 * pointer at +4, the element's next at +8, its list slot at +0xC). Does nothing when the allocator returns NULL. The
 * geometry reader func_003be180 calls it with the new block and the block's byte size (s4 in retail). */
extern s32 iGpffffb700;
extern u8 D_008872E0[];
extern void *(*D_008873F8[])(s32 arg0, s32 arg1);

void func_003c9940(s32 arg0, s32 arg1)
{
    u8 *list = D_008872E0 + iGpffffb700 + 4;
    u8 *elem = (u8 *)D_008873F8[0](*(s32 *)(D_008872E0 + iGpffffb700), 0x40507);

    if (elem != NULL) {
        *(s32 *)(elem + 0) = arg0;
        *(s32 *)(elem + 4) = arg1;
        *(s32 *)(elem + 8) = *(s32 *)list;
        *(u8 **)(elem + 0xC) = list;
        *(u8 **)(*(u8 **)list + 4) = elem + 8;
        *(u8 **)list = elem + 8;
    }
}
