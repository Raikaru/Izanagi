/* func_004140f0: assigns each group of linked records a colour (a bit number) and stores the
 * number of colours used at reg+12. `head` points at the first group; groups chain through word 2
 * (+8), a group's members chain through word 1 (+4), and each member is the head of a list that
 * chains through +0x10. Every record in a list has a descriptor at +0x14 whose word at +8 is a
 * bitmask of the colours already taken. For each group the lowest bit clear in the union of the
 * masks of all its records is chosen; it is written to every record's +0x1c and OR-ed into every
 * record's descriptor mask, and the colour count becomes max(count, colour + 1). Returns 0. */
s32 func_004140f0(u8 *reg, u8 **head)
{
    u8 *group;
    u8 *member;
    u8 *rec;
    u32 mask;
    u32 colour;
    u32 count = 0;

    for (group = *head; group != 0; group = *(u8 **)(group + 8)) {
        mask = 0;
        for (member = group; member != 0; member = *(u8 **)(member + 4)) {
            rec = *(u8 **)member;
            if (rec != 0) {
                do {
                    mask |= *(u32 *)(*(u8 **)(rec + 0x14) + 8);
                    rec = *(u8 **)(rec + 0x10);
                } while (rec != 0);
            }
        }
        colour = 0;
        if (mask & 1) {
            do {
                mask >>= 1;
                colour++;
            } while (mask & 1);
        }
        if (!(colour < count)) {
            count = colour + 1;
        }
        for (member = group; member != 0; member = *(u8 **)(member + 4)) {
            rec = *(u8 **)member;
            if (rec != 0) {
                do {
                    *(u32 *)(rec + 0x1C) = colour;
                    *(u32 *)(*(u8 **)(rec + 0x14) + 8) |= 1u << colour;
                    rec = *(u8 **)(rec + 0x10);
                } while (rec != 0);
            }
        }
    }
    *(u32 *)(reg + 12) = count;
    return 0;
}
