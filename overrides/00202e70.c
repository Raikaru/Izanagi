/* Native 00202e70. Only explicit battle-tool policies supply native input
 * codes; ordinary play and tape mode retain the original controller path. */
#include "p4pc/battle_sim.h"
#include "p4pc/guest.h"

s32 btlSelectionReadInput(u8 *input)
{
    extern u16 D_008C024E[];
    extern u16 D_008C0276[];
    extern u16 D_008C027A[];
    BtlSelection *selection;
    s32 count;
    u32 repeat;
    u32 flags;

    if (p4_battle_sim_active) {
        s32 code = p4_battle_policy_input(P4_H2G(input));
        if (code >= 0) return code;
    }
    selection = (BtlSelection *)input;
    count = selection->count;
    if (selection->selected >= count) {
        s32 first;
        selection->selected = count - 1;
        first = selection->count - selection->pageSize;
        selection->first = (u16)first;
        if ((s16)first < 0) {
            selection->first = 0;
        }
    }
    if (D_008C024E[0] & 0x40) {
        return 2;
    }
    if (D_008C024E[0] & 0x20) {
        return 1;
    }
    if (D_008C024E[0] & 0x80) {
        return 3;
    }
    flags = selection->flags;
    if ((flags & 4) != 0 && (D_008C024E[0] & 4) != 0) {
        return 4;
    }
    repeat = D_008C027A[0];
    if ((repeat & selection->previous) != 0 ||
        ((flags & 1) != 0 && (repeat & selection->previousPage) != 0)) {
        s32 moved;
        s32 page;
        s32 single;
        s16 amount;
        s32 steps;
        s16 i;

        moved = 1;
        page = (repeat & selection->previousPage) != 0;
        single = !page;
        if (single != 0) {
            amount = 1;
        } else {
            amount = selection->pageSize;
        }
        steps = amount;
        for (i = 0; i < steps; ++i) {
            s32 first;
            s32 selected;
            first = selection->first;
            selected = selection->selected;
            if (selected <= first + 1) {
                if (first > 0) {
                    selection->first = first - 1;
                    selection->selected = selection->selected - 1;
                } else if (selected == 1) {
                    selection->selected = 0;
                } else {
                    if ((selection->flags & 2) == 0 ||
                        (D_008C0276[0] & selection->previous) != 0 ||
                        (page != 0 && (D_008C0276[0] & selection->previousPage) != 0)) {
                        s32 lastPage;
                        lastPage = selection->count - selection->pageSize;
                        selection->first = (u16)lastPage;
                        if ((s16)lastPage < 0) {
                            selection->first = 0;
                        }
                        selection->selected = selection->count - 1;
                    } else {
                        moved = 0;
                        selection->first = 0;
                        selection->selected = 0;
                    }
                    break;
                }
            } else {
                selection->selected = selected - 1;
            }
        }
        if (moved != 0 && (selection->flags & 8) != 0) {
            if (page == 0) {
                sdkSndStartStream(0, 0, 0, 0);
            } else {
                sdkSndStartStream(0, 0, 0, 5);
            }
        }
        return (u16)(single != 0 ? 5 : 7);
    } else if ((repeat & selection->next) != 0 ||
               ((flags & 1) != 0 && (repeat & selection->nextPage) != 0)) {
        s32 moved;
        s32 page;
        s32 single;
        s16 amount;
        s32 steps;
        s16 i;

        moved = 1;
        page = (repeat & selection->nextPage) != 0;
        single = !page;
        if (single != 0) {
            amount = 1;
        } else {
            amount = selection->pageSize;
        }
        steps = amount;
        for (i = 0; i < steps; ++i) {
            s32 pageSize;
            s32 first;
            s32 selected;
            pageSize = selection->pageSize;
            first = selection->first;
            selected = selection->selected;
            if (selected >= first + pageSize - 2 ||
                selected >= selection->count - 2) {
                s32 count;
                s32 lastPage;
                count = selection->count;
                lastPage = count - pageSize;
                if (first < lastPage) {
                    ++selection->first;
                    ++selection->selected;
                } else if (selected < count - 1) {
                    ++selection->selected;
                } else {
                    if ((selection->flags & 2) == 0 ||
                        (D_008C0276[0] & selection->next) != 0 ||
                        (page != 0 && (D_008C0276[0] & selection->nextPage) != 0)) {
                        selection->first = 0;
                        selection->selected = 0;
                    } else {
                        moved = 0;
                        if (count < pageSize) {
                            lastPage = 0;
                        }
                        selection->first = (u16)lastPage;
                        selection->selected = selection->count - 1;
                    }
                    break;
                }
            } else {
                selection->selected = selected + 1;
            }
        }
        if (moved != 0 && (selection->flags & 8) != 0) {
            if (page == 0) {
                sdkSndStartStream(0, 0, 0, 0);
            } else {
                sdkSndStartStream(0, 0, 0, 5);
            }
        }
        return (u16)(single != 0 ? 6 : 8);
    }
    return 0;
}
