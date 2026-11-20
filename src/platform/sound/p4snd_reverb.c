/* Clean-room implementation written from psx-spx hardware documentation:
 * https://psx-spx.consoledev.net/soundprocessingunitspu/
 * Sections: SPU Reverb Registers, SPU Reverb Formula, Reverb Buffer Resampling.
 */
#include "p4snd_reverb.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* The documented 39-tap half-band filter, in 1.15 fixed point. */
static const int16_t fir[39] = {
    -0x0001, 0, 0x0002, 0, -0x000a, 0, 0x0023, 0,
    -0x0067, 0, 0x010a, 0, -0x0268, 0, 0x0534, 0,
    -0x0b90, 0, 0x2806, 0x4000, 0x2806, 0, -0x0b90, 0,
    0x0534, 0, -0x0268, 0, 0x010a, 0, -0x0067, 0,
    0x0023, 0, -0x000a, 0, 0x0002, 0, -0x0001
};

static struct {
    int16_t reg[32];
    int16_t *ram;
    uint32_t length, cursor;
    int enabled;
    int32_t volume[2];
    int16_t input[2][39], output[2][39];
    unsigned history, phase;
} rev;

static int16_t clamp16(int64_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

/* Not specified by psx-spx; chosen arithmetic right shifts for fixed-point
 * products, and a causal FIR with unity DC gain and 2x interpolation gain. */
static int32_t mul15(int32_t x, int32_t y)
{
    return (int32_t)(((int64_t)x * y) >> 15);
}

static int16_t filter(const int16_t samples[39], unsigned pos, int gain)
{
    int64_t sum = 0;
    unsigned i, at = pos;
    for (i = 0; i < 39; ++i) {
        sum += (int32_t)fir[i] * samples[at];
        at = at == 0 ? 38 : at - 1;
    }
    return clamp16((sum * gain) >> 15);
}

/* Register addresses are in eight-byte units, while RAM contains halfwords.
 * The previous-sample reflection tap is one halfword before its destination. */
static uint32_t address(int reg, int subtract, int previous)
{
    int64_t a = (int64_t)rev.cursor + 4 * (uint16_t)rev.reg[reg]
        - (subtract >= 0 ? 4 * (uint16_t)rev.reg[subtract] : 0) - previous;
    a %= rev.length;
    if (a < 0) a += rev.length;
    return (uint32_t)a;
}

static int32_t load(int reg, int subtract, int previous)
{
    return rev.ram[address(reg, subtract, previous)];
}

static void store(int reg, int32_t value)
{
    rev.ram[address(reg, -1, 0)] = clamp16(value);
}

/* SPU Reverb Formula: reflection taps feed four combs and two all-pass stages.
 * The documented -8000h IIR quirk negates the complete reflection result. */
static void reflect(int dest, int source, int32_t input)
{
    int32_t old = load(dest, -1, 1);
    int32_t result = mul15(input + mul15(load(source, -1, 0), rev.reg[7]) - old, rev.reg[2]) + old;
    if (rev.reg[2] == -32768) result = -result;
    store(dest, result);
}

static int16_t channel(int side, int16_t sample)
{
    int32_t x, delayed;
    int apf;

    /* psx-spx writes reflections before reading combs; aliased addresses may
     * consequently behave differently from other implementations. */
    if (rev.enabled) {
        int32_t input = mul15(sample, rev.reg[30 + side]);
        reflect(10 + side, 16 + side, input);
        reflect(18 + side, 24 + (side ^ 1), input);
    }
    x = mul15(load(12 + side, -1, 0), rev.reg[3])
      + mul15(load(14 + side, -1, 0), rev.reg[4])
      + mul15(load(20 + side, -1, 0), rev.reg[5])
      + mul15(load(22 + side, -1, 0), rev.reg[6]);
    for (apf = 0; apf < 2; ++apf) {
        int dest = 26 + 2 * apf + side;
        delayed = load(dest, apf, 0);
        x -= mul15(delayed, rev.reg[8 + apf]);
        if (rev.enabled) store(dest, x);
        x = mul15(x, rev.reg[8 + apf]) + delayed;
    }
    return clamp16(x);
}

void p4rev_setup(const int16_t regs[32], uint32_t size_bytes)
{
    int16_t *ram = NULL;
    uint32_t length = size_bytes / sizeof(*ram);
    if (length) ram = (int16_t *)calloc(length, sizeof(*ram));
    free(rev.ram);
    rev.ram = ram;
    rev.length = ram ? length : 0;
    rev.cursor = 0;
    memcpy(rev.reg, regs, sizeof(rev.reg));
    memset(rev.input, 0, sizeof(rev.input));
    memset(rev.output, 0, sizeof(rev.output));
    rev.history = 0;
    rev.phase = 1;
}

void p4rev_enable(int on)
{
    rev.enabled = on != 0;
}

void p4rev_volume(int32_t evol_l, int32_t evol_r)
{
    rev.volume[0] = evol_l;
    rev.volume[1] = evol_r;
}

int p4rev_active(void)
{
    return rev.enabled && rev.length != 0;
}

void p4rev_process(const float *in, float *out, int frames)
{
    int frame, side;
    if (!rev.length) return;
    for (frame = 0; frame < frames; ++frame) {
        unsigned pos = rev.history;
        for (side = 0; side < 2; ++side) {
            float value = in[2 * frame + side];
            /* Mixer sends are floats; clamp before converting to SPU samples. */
            if (value > 1.0f) value = 1.0f;
            if (value < -1.0f) value = -1.0f;
            rev.input[side][pos] = clamp16((int32_t)(value * 32768.0f));
            rev.output[side][pos] = 0;
        }
        /* Not specified by psx-spx; chosen right-first causal resampling phase.
         * The hardware calculates one side per output sample and advances its
         * work pointer after the right side. */
        side = (int)rev.phase;
        rev.output[side][pos] = channel(side, filter(rev.input[side], pos, 1));
        if (side) rev.cursor = (rev.cursor + 1) % rev.length;
        for (side = 0; side < 2; ++side) {
            int32_t value = mul15(filter(rev.output[side], pos, 2), rev.volume[side]);
            out[2 * frame + side] += (float)value / 32768.0f;
        }
        rev.history = (pos + 1) % 39;
        rev.phase ^= 1;
    }
}
