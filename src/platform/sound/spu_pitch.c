/* LIBSD.IRX Note2Pitch (export 13, text 0x3bd0..0x3ce4).
 * MODHSYN.IRX calls it at 0xa8d4 through import 0xbd64. The host mixer uses
 * the same Q12 ratio, including half-up right-shift rounding and low-word
 * truncation. No host libm or generated floating-point approximation.
 */
#include "spu_pitch.h"
#include "spu_pitch_table.h"

uint16_t p4spu_pitch_for_offset(int32_t offset128)
{
    /* The caller expresses a relative note/fine pair. Normalize fine to
     * (-128,128), the same domain as the signed fine byte in a tone. This is
     * Note2Pitch(center_note=0, center_fine=0, note=coarse, fine=fine). */
    uint16_t word = (uint16_t)(offset128 / 128);
    int32_t delta = (int32_t)(word ^ 0x8000u) - 0x8000;
    int32_t fine = offset128 % 128;
    int32_t quotient = delta / 12; /* signed division truncates toward zero */
    int32_t semitone = delta - quotient * 12;
    int32_t octave = quotient - 2;
    uint32_t pitch;

    /* 3c40..3c84: normalize the negative semitone/fine remainder. */
    if (semitone < 0 || (semitone == 0 && fine < 0)) {
        semitone += 12;
        octave = quotient - 3;
    }
    if (fine < 0) {
        semitone--;
        fine += 128;
    }
    pitch = ((uint32_t)p4spu_semitone[semitone] * p4spu_fine[fine]) >> 16;
    if (octave < 0) {
        /* 3cc4..3cd8: the IOP's variable word shifts use the low five bits.
         * This also defines very large offsets without C shift UB. */
        uint32_t shift = (uint32_t)-octave;
        pitch += UINT32_C(1) << ((shift - 1u) & 31u);
        pitch >>= shift & 31u;
    }
    return (uint16_t)pitch;
}

double p4spu_pitch_rate(double sample_rate, int32_t offset128)
{
    /* For the mixer's finite bank rates these are exact binary operations:
     * a Q12 integer scale, never a transcendental approximation. */
    return (sample_rate / 4096.0) * p4spu_pitch_for_offset(offset128);
}
