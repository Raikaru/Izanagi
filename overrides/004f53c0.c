/* Retail 004f53c0, through the return at 004f5498. Started with the native
 * romwright R5900 draft and checked every branch against the instructions.
 * The line is sign-extended from 16 bits even in the no-detail format.
 * cri_rofs.h's RofsErrFunc is void(void *, Char8 *, Sint32); the saved
 * context word is therefore a guest pointer, not an opaque integer.
 */
#include <stdio.h>
#include <stdint.h>
#include "p4pc/gmem.h"

int32_t func_004f53c0(void *file, int32_t line, void *message, void *detail, int32_t code)
{
    uint32_t work = P4_G32(0x00743a50);

    if (work == 0) {
        code = -203;
    } else if (P4_G32(work + 0x40) != 0) {
        char *text = P4_G2H(0x00925ea8);
        void (*report)(void *, char *, int32_t);

        if (detail != 0) {
            sprintf(text, P4_G2H(0x0075d938), code, message, detail,
                    file, (int)(int16_t)line);
        } else {
            sprintf(text, P4_G2H(0x0075d950), code, message,
                    file, (int)(int16_t)line);
        }
        work = P4_G32(0x00743a50);
        report = (void (*)(void *, char *, int32_t))p4_fn_g2h_sig(
            P4_G32(work + 0x40), P4_SIG(3, 0, 1, 1, 0, 0, 0, 0));
        report(P4_G2H(P4_G32(work + 0x44)), text, code);
    }
    P4_G32(0x00743a54) = (uint32_t)code;
    return code;
}
