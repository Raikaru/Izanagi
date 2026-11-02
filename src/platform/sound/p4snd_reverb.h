/* The SPU2 reverb unit (see p4snd_reverb.c). Called with the mixer lock held. */
#ifndef P4PC_P4SND_REVERB_H
#define P4PC_P4SND_REVERB_H

#include <stdint.h>

/* Loads a preset: the 32 register values in libsd's order and the size of the work area in bytes. The
 * work area is cleared and the unit restarts. */
void p4rev_setup(const int16_t regs[32], uint32_t size_bytes);
void p4rev_enable(int on);
/* The effect volume of the left and right output, 1.15 fixed point (0x7fff is full). */
void p4rev_volume(int32_t evol_l, int32_t evol_r);
int p4rev_active(void);
/* Runs `frames` stereo frames of input (the wet sends of the voices) through the unit and adds its output to
 * `out`. */
void p4rev_process(const float *in, float *out, int frames);

#endif
