/* 8-byte alignment for a 64-bit member of a struct the port shares with the decomp.
 *
 * The decomp mirror is built with -malign-double, which aligns 64-bit members to 8 bytes
 * inside structs. Port-owned code is built without that flag, so any port struct that
 * mirrors a game struct with a 64-bit member must spell the alignment out with this macro
 * and assert the offsets. Force-included through p4pc/compat.h. */
#ifndef P4PC_ALIGN_H
#define P4PC_ALIGN_H

#define P4_PS2_ALIGN64 __attribute__((aligned(8)))

#endif /* P4PC_ALIGN_H */
