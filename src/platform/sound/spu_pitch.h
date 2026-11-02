#ifndef P4PC_SPU_PITCH_H
#define P4PC_SPU_PITCH_H

#include <stdint.h>

/* Pitch in 1/128 semitone relative to the sample's own rate. These are the
 * LIBSD Note2Pitch integer tables/rounding, not host pow/exp2. */
uint16_t p4spu_pitch_for_offset(int32_t offset128);
double p4spu_pitch_rate(double sample_rate, int32_t offset128);

#endif
