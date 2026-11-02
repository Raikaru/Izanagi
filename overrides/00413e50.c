extern s32 func_003df590();
extern u8 *func_003df4d0();
extern u32 func_00411740(s32 size);

/* func_00413e50: second pass over a registry's entries: groups the cells of each entry's cell
 * table (descriptor word 7 -> word 2, 36-byte cells; see func_00413350) that share a link.
 * `head` is the head of the group list; a group node is 16 bytes (+0 first cell, +4 sibling
 * chain, +8 next group, +0xC parent) and the cells of a group chain through cell word 4, a cell's
 * word 3 pointing at its group.
 *  - Every cell without a group gets a new node (func_00411740(16), pushed on *head, the cell its
 *    first member). Allocation failure reports error {1, 0x20} and returns 0x20.
 *  - For each link j of entry E (link indices at E+8, -1 = none) to entry T, every cell C of T's
 *    table with word 1 set is looked up by id in E's table; a cell found there whose mask (+0x18)
 *    has bit j set joins C to its group: when C has a group the two groups are merged with
 *    func_00413640(head, C.group, found.group), otherwise C is pushed on found's group.
 * Returns 0. */
s32 func_00413e50(s32 reg_addr, s32 *head_word)
{
    u8 *reg = (u8 *)reg_addr;
    u8 **head = (u8 **)head_word;
    s32 n = *(s32 *)(reg + 4);
    u8 *entry = *(u8 **)(reg + 8);
    s32 err[2];
    u32 i;
    u32 j;
    u32 t;
    u32 *tbl;

    do {
        tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
        for (i = 0; i < tbl[0]; ) {
            s32 *cell = (i < tbl[0]) ? (s32 *)(tbl[3] + i * 36) : 0;

            if (cell[3] == 0) {
                s32 *node = (s32 *)func_00411740(16);

                if (node != 0) {
                    node[1] = 0;
                    node[0] = 0;
                    node[3] = 0;
                    node[2] = *(s32 *)head;
                    *(s32 **)head = node;
                }
                cell[3] = (s32)node;
                if (node == 0) {
                    err[0] = 1;
                    err[1] = func_003df590(0x20);
                    func_003df4d0(err);
                    return 0x20;
                }
                cell[4] = node[0];
                node[0] = (s32)cell;
            }
            i++;
            tbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
        }
        for (j = 0; j < *(u32 *)(entry + 4); j++) {
            s32 link = *(s32 *)(*(u8 **)(entry + 8) + j * 4);
            u8 *target;
            u32 *ttbl;
            u32 mask;

            if (link == -1) {
                continue;
            }
            target = *(u8 **)(reg + 8) + link * 40;
            ttbl = *(u32 **)(*(u8 **)(target + 0x1C) + 8);
            if (ttbl[0] == 0) {
                continue;
            }
            mask = 1u << j;
            for (t = 0; t < ttbl[0]; ) {
                s32 *ct = (t < ttbl[0]) ? (s32 *)(ttbl[3] + t * 36) : 0;

                if (ct[1] != 0) {
                    u32 *etbl = *(u32 **)(*(u8 **)(entry + 0x1C) + 8);
                    s32 *ce = 0;
                    u32 x;

                    if (etbl[0] != 0) {
                        s32 *c = (s32 *)etbl[3];

                        for (x = 0; x < etbl[0]; x++, c += 9) {
                            if (c[0] == ct[0]) {
                                ce = c;
                                break;
                            }
                        }
                    }
                    if (ce != 0 && (ce[6] & mask) != 0) {
                        if (ct[3] != 0) {
                            func_00413640(head, (u8 **)ct[3], (u8 **)ce[3]);
                        } else {
                            s32 *group = (s32 *)ce[3];

                            ct[4] = group[0];
                            group[0] = (s32)ct;
                            ct[3] = ce[3];
                        }
                    }
                }
                t++;
                ttbl = *(u32 **)(*(u8 **)(target + 0x1C) + 8);
            }
        }
        entry += 40;
        n--;
    } while (n != 0);
    return 0;
}
