#include "p4pc/native_rows.h"

/* 002eb270: fclCombineLayoutTransition, retail; while the card list includes a mod card
 * (include/p4pc/native_rows.h) it is laid out by p4VelvetLayout, which places any count up to ten.
 * The entry (transition 0) first lets the native-rows service add or drop the mod card. */
static void p4VelvetLayout(u8 *task, u8 *work, s32 transition);
void fclCombineLayoutTransition(u8 *task, s32 transition) {
    extern u8 D_006407F0[];
    FclCombineLayoutSlot *slot;
    u8 *work;
    s16 jitter;

    /* Only the signed low byte selects the transition: zero enters, every
       other value exits, and one also hides the six selection overlays. */
    work = ((u8*)((KwlnTask *)task)->work);
    if ((s8)transition == 0) {
        p4_native_rows_velvet_sync(work);
    }
    if (p4_native_rows_velvet_active(work)) {
        p4VelvetLayout(task, work, (s8)transition);
    } else {
        if (((FclCombineWork *)(work))->commandCount % 2 == 0) {
            if (((FclCombineWork *)(work))->commandCount == 6) {
                s32 transition6;
                s16 left6;

                for (left6 = 0, transition6 = (s8)transition; left6 < 3; left6++) {
                    slot = (FclCombineLayoutSlot *)D_00640760 + (left6 + 1);
                    if (transition6 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), left6, left6 * 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[left6].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[left6].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), left6, left6 * 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                slot = (FclCombineLayoutSlot *)D_006407A8;
                if (transition6 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 8, 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[5].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[5].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 8, 1, slot->col * 3 + 0x3E, slot->row + 0x57);
                }
                slot = (FclCombineLayoutSlot *)D_0064079C;
                if (transition6 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 7, 6, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[4].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[4].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 7, 3, slot->col * 3 + 0x3E, slot->row + 0x57);
                }
                slot = (FclCombineLayoutSlot *)D_00640790;
                if (transition6 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 6, 0xA, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[3].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[3].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 6, 5, slot->col * 3 + 0x3E, slot->row + 0x57);
                }
            } else if (((FclCombineWork *)(work))->commandCount == 8) {
                s32 leftTransition8;
                s32 rightTransition8;
                s16 left8;
                s16 right8;

                /* Each edge traversal owns its transition value. */
                for (left8 = 0, leftTransition8 = (s8)transition; left8 < 4; left8++) {
                    slot = (FclCombineLayoutSlot *)D_00640760 + left8;
                    if (leftTransition8 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), left8, left8 * 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[left8].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[left8].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), left8, left8 * 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                /* The left phase has finished; carry its mode into the right
                   traversal and the remaining central placement. */
                for (right8 = 0, rightTransition8 = leftTransition8; right8 < 3; right8++) {
                    slot = (FclCombineLayoutSlot *)D_00640760 + (7 - right8);
                    if (rightTransition8 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 8 - right8, right8 * 4 + 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[(7 - right8)].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[(7 - right8)].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 8 - right8, right8 * 2 + 1, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                slot = (FclCombineLayoutSlot *)D_00640790;
                if (rightTransition8 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 4, 0xE, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[4].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[4].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 4, 7, slot->col * 3 + 0x3E, slot->row + 0x57);
                }
            }
        } else {
            if (((FclCombineWork *)(work))->commandCount == 5) {
                s32 transition5;
                s16 pair5;

                for (pair5 = 0, transition5 = (s8)transition; pair5 < 2; pair5++) {
                    slot = (FclCombineLayoutSlot *)D_006407C0 + (pair5 + 2);
                    if (transition5 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), pair5, pair5 * 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[pair5].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[pair5].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), pair5, pair5 * 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                    slot = (FclCombineLayoutSlot *)D_006407C0 + (6 - pair5);
                    if (transition5 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 8 - pair5, pair5 * 4 + 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[(4 - pair5)].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[(4 - pair5)].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 8 - pair5, pair5 * 2 + 1, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                slot = (FclCombineLayoutSlot *)D_006407F0;
                if (transition5 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 6, 8, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[2].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[2].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    if (RpRandom() % 100 >= 50) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), 6, 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                    } else {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 6, 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
            } else if (((FclCombineWork *)(work))->commandCount == 7) {
                s32 transition7;
                s16 pair7;

                for (pair7 = 0, transition7 = (s8)transition; pair7 < 3; pair7++) {
                    slot = (FclCombineLayoutSlot *)D_006407C0 + (pair7 + 1);
                    if (transition7 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), pair7, pair7 * 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[pair7].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[pair7].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), pair7, pair7 * 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                    slot = (FclCombineLayoutSlot *)D_006407C0 + (7 - pair7);
                    if (transition7 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 8 - pair7, pair7 * 4 + 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[(6 - pair7)].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[(6 - pair7)].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 8 - pair7, pair7 * 2 + 1, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                slot = (FclCombineLayoutSlot *)D_006407F0;
                if (transition7 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 3, 0xC, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[3].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[3].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    if (RpRandom() % 100 >= 50) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), 3, 6, slot->col * 3 + 0x3E, slot->row + 0x57);
                    } else {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 3, 6, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
            } else if (((FclCombineWork *)(work))->commandCount == 9) {
                s32 transition9;
                s16 pair9;

                for (pair9 = 0, transition9 = (s8)transition; pair9 < 4; pair9++) {
                    slot = (FclCombineLayoutSlot *)D_006407C0 + pair9;
                    if (transition9 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), pair9, pair9 * 4, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[pair9].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[pair9].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), pair9, pair9 * 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                    slot = (FclCombineLayoutSlot *)D_006407C0 + (8 - pair9);
                    if (transition9 == 0) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(700.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 8 - pair9, pair9 * 4 + 2, slot->col * 3 + 0x3E, slot->row + 0x57);
                        ((FclCombineWork *)(work))->commandRows[(8 - pair9)].x = slot->col * 3 + 0x3E;
                        ((FclCombineWork *)(work))->commandRows[(8 - pair9)].y = slot->row + 0x57;
                    } else {
                        jitter = RpRandom() % 300 - 150;
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 8 - pair9, pair9 * 2 + 1, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
                slot = (FclCombineLayoutSlot *)D_006407F0;
                if (transition9 == 0) {
                    fclCombineDrawEntryCell(task, fclMakeVec2(-200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y), 4, 0x10, slot->col * 3 + 0x3E, slot->row + 0x57);
                    ((FclCombineWork *)(work))->commandRows[4].x = slot->col * 3 + 0x3E;
                    ((FclCombineWork *)(work))->commandRows[4].y = slot->row + 0x57;
                } else {
                    jitter = RpRandom() % 300 - 150;
                    if (RpRandom() % 100 >= 50) {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(-300.0f, slot->y + jitter), 4, 8, slot->col * 3 + 0x3E, slot->row + 0x57);
                    } else {
                        fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(700.0f, slot->y + jitter), 4, 8, slot->col * 3 + 0x3E, slot->row + 0x57);
                    }
                }
            }
        }
    }
    /* Each resource is looked up again after hiding it in retail. */
    if ((s8)transition == 1) {
        fclSpriteEntry(0x216)[0x73] = 0;
        fclSpriteEntry(0x216);
        fclSpriteEntry(0x217)[0x73] = 0;
        fclSpriteEntry(0x217);
        fclSpriteEntry(0x218)[0x73] = 0;
        fclSpriteEntry(0x218);
        fclSpriteEntry(0x219)[0x73] = 0;
        fclSpriteEntry(0x219);
        fclSpriteEntry(0x21A)[0x73] = 0;
        fclSpriteEntry(0x21A);
        fclSpriteEntry(0x21B)[0x73] = 0;
        fclSpriteEntry(0x21B);
    }
}

/* The card list with the mod card, laid out as retail lays out N cards: N
 * slots left to right from D_00640760 (even N <= 8) or D_006407C0 (odd N <= 9),
 * or the ten-card arc below; the left half enters from x -200 and the right
 * half from x 700 with retail's per-card delays, an odd list's centre card
 * enters from the left and leaves to a random side, and every card leaves to
 * its side with retail's vertical jitter. The mod card leaves without drawing
 * RpRandom (jitter 0, the right side), so it adds no random draws. */
static void p4VelvetLayout(u8 *task, u8 *work, s32 transition)
{
    /* Ten cards: the eight-card arc's centre pair (246, 321) at its peak, 59
     * pixels apart outward (the eight-card arc is 66), the arc continuing to
     * y 167 with the outermost cards one depth step and one layer below. */
    static const FclCombineLayoutSlot ten[10] = {
        {10.0f, 167.0f, 6, -3}, {69.0f, 144.0f, 5, 0}, {128.0f, 121.0f, 4, 3}, {187.0f, 98.0f, 3, 6},
        {246.0f, 75.0f, 2, 9}, {321.0f, 75.0f, 2, 9}, {380.0f, 98.0f, 3, 6}, {439.0f, 121.0f, 4, 3},
        {498.0f, 144.0f, 5, 0}, {557.0f, 167.0f, 6, -3},
    };
    s32 count = ((FclCombineWork *)work)->commandCount;
    s32 half = count / 2;
    s32 d;

    for (d = 0; d < count; d++) {
        FclCombineRow *row = (FclCombineRow *)p4_native_rows_velvet_row(work, d);
        const FclCombineLayoutSlot *slot;
        s32 right = d >= count - half;
        s32 centre = (count & 1) && d == half;
        s32 k = right ? count - 1 - d : d;
        s32 mod = row->id == P4_NATIVE_ROWS_VELVET_ENTRY;
        s16 depth;
        s16 layer;

        if (count == 10) {
            slot = &ten[d];
        } else if (count % 2 == 0) {
            slot = (FclCombineLayoutSlot *)D_00640760 + ((8 - count) / 2 + d);
        } else {
            slot = (FclCombineLayoutSlot *)D_006407C0 + ((9 - count) / 2 + d);
        }
        depth = slot->col * 3 + 0x3E;
        layer = slot->row + 0x57;
        if (transition == 0) {
            fclCombineDrawEntryCell(task, fclMakeVec2(right ? 700.0f : -200.0f, 100.0f + slot->y), fclMakeVec2(slot->x, slot->y),
                row->id, centre ? half * 4 : right ? k * 4 + 2 : k * 4, depth, layer);
            row->x = depth;
            row->y = layer;
        } else {
            s16 jitter = mod ? 0 : RpRandom() % 300 - 150;
            f32 side = right ? 700.0f : -300.0f;

            if (centre) {
                side = !mod && RpRandom() % 100 >= 50 ? -300.0f : 700.0f;
            }
            fclCombineDrawEntryCell(task, fclMakeVec2(slot->x, slot->y), fclMakeVec2(side, slot->y + jitter),
                row->id, centre ? half * 2 : right ? k * 2 + 1 : k * 2, depth, layer);
        }
    }
}
