/* The SPU2 reverb (effect) unit, written after PCSX2's SPU2 emulation (Reverb.cpp, ReverbResample.cpp),
 * which follows the SPU's network: two same-side and two cross-side reflection filters, four comb taps and
 * two all-pass filters per side, in a ring buffer of the effect area. The 48 kHz output rate is halved by a
 * 39 tap half-band filter on each side (the left channel is calculated on even cycles, the right on odd
 * ones) and doubled again by the same filter, so the network runs at 24 kHz per channel.
 *
 * The registers are the 32 values libsd loads for a mode, in its order:
 *   0 APF1_SIZE   1 APF2_SIZE   2 IIR_VOL     3 COMB1_VOL   4 COMB2_VOL   5 COMB3_VOL   6 COMB4_VOL
 *   7 WALL_VOL    8 APF1_VOL    9 APF2_VOL   10 SAME_L_DST 11 SAME_R_DST 12 COMB1_L_SRC 13 COMB1_R_SRC
 *  14 COMB2_L_SRC 15 COMB2_R_SRC 16 SAME_L_SRC 17 SAME_R_SRC 18 DIFF_L_DST 19 DIFF_R_DST 20 COMB3_L_SRC
 *  21 COMB3_R_SRC 22 COMB4_L_SRC 23 COMB4_R_SRC 24 DIFF_L_SRC 25 DIFF_R_SRC 26 APF1_L_DST 27 APF1_R_DST
 *  28 APF2_L_DST 29 APF2_R_DST 30 IN_COEF_L 31 IN_COEF_R
 * Addresses and the all-pass sizes are in units of four 16-bit cells and relative to the start of the area. */
#include "p4snd_reverb.h"

#include <stdlib.h>
#include <string.h>

#define NUM_TAPS 39

/* the half-band filter, 1.15 fixed point; the upsampling copy is doubled and the centre tap clamps */
static const int32_t fir_down[NUM_TAPS] = {
    -1, 0, 2, 0, -10, 0, 35, 0, -103, 0, 266, 0, -616, 0, 1332, 0, -2960, 0, 10246, 16384,
    10246, 0, -2960, 0, 1332, 0, -616, 0, 266, 0, -103, 0, 35, 0, -10, 0, 2, 0, -1,
};

static struct {
    int enabled;
    int16_t reg[32];
    int32_t evol_l, evol_r; /* 1.15 */
    int16_t *cell;          /* the effect area, 16-bit cells */
    int32_t ncell;
    uint32_t cycles;        /* 48 kHz frames; the address base advances every second one */
    int pos;                /* 0..63, the slot of the newest sample in the resampling buffers */
    int32_t down[2][128];   /* input history per side, every slot written twice (slot and slot | 64) */
    int32_t up[2][128];     /* output history per side: the network's sample on its own cycle, 0 on the other */
} R;

static int32_t clamp16(int32_t x)
{
    return x > 32767 ? 32767 : (x < -32768 ? -32768 : x);
}

#define MUL(x, y) (((x) * (y)) >> 15)

/* the cell of a register value (units of four cells; `adj` cells before it) under the moving base */
static int32_t indexer(int32_t offset)
{
    int64_t x = (int64_t)(R.cycles >> 1) + offset;

    x %= R.ncell;
    if (x < 0) {
        x += R.ncell;
    }
    return (int32_t)x;
}

void p4rev_setup(const int16_t regs[32], uint32_t size_bytes)
{
    uint32_t n = size_bytes / 2;

    free(R.cell);
    R.cell = n ? (int16_t *)calloc(n, sizeof(int16_t)) : NULL;
    R.ncell = R.cell ? (int32_t)n : 0;
    memcpy(R.reg, regs, sizeof(R.reg));
    R.cycles = 0;
    R.pos = 0;
    memset(R.down, 0, sizeof(R.down));
    memset(R.up, 0, sizeof(R.up));
}

void p4rev_enable(int on)
{
    R.enabled = on;
}

void p4rev_volume(int32_t evol_l, int32_t evol_r)
{
    R.evol_l = evol_l;
    R.evol_r = evol_r;
}

int p4rev_active(void)
{
    return R.enabled && R.cell != NULL;
}

static int32_t downsample(int right)
{
    const int index = (R.pos - NUM_TAPS) & 63;
    int32_t out = 0;
    int i;

    for (i = 0; i < NUM_TAPS; i++) {
        out += R.down[right][index + i] * fir_down[i];
    }
    return clamp16(out >> 15);
}

/* One calculation for one side. in_l / in_r are only stored (the history); the side is picked by the cycle. */
static void calc_side(int right)
{
    const int16_t *r = R.reg;
    const int32_t a1 = (int32_t)(uint16_t)r[0] * 4, a2 = (int32_t)(uint16_t)r[1] * 4;
#define A(n) ((int32_t)(uint16_t)r[n] * 4)
    const int32_t same_src = indexer(A(right ? 17 : 16));
    const int32_t same_dst = indexer(A(right ? 11 : 10));
    const int32_t same_prv = indexer(A(right ? 11 : 10) - 1);
    const int32_t diff_src = indexer(A(right ? 24 : 25));
    const int32_t diff_dst = indexer(A(right ? 19 : 18));
    const int32_t diff_prv = indexer(A(right ? 19 : 18) - 1);
    const int32_t comb1 = indexer(A(right ? 13 : 12));
    const int32_t comb2 = indexer(A(right ? 15 : 14));
    const int32_t comb3 = indexer(A(right ? 21 : 20));
    const int32_t comb4 = indexer(A(right ? 23 : 22));
    const int32_t apf1_src = indexer(A(right ? 27 : 26) - a1);
    const int32_t apf1_dst = indexer(A(right ? 27 : 26));
    const int32_t apf2_src = indexer(A(right ? 29 : 28) - a2);
    const int32_t apf2_dst = indexer(A(right ? 29 : 28));
#undef A
    const int16_t *m = R.cell;
    int32_t in, same, diff, apf1, apf2, out;

    in = MUL((int32_t)r[right ? 31 : 30], downsample(right));
    same = MUL(r[2], in + MUL(r[7], m[same_src]) - m[same_prv]) + m[same_prv];
    diff = MUL(r[2], in + MUL(r[7], m[diff_src]) - m[diff_prv]) + m[diff_prv];
    out = MUL(r[3], m[comb1]) + MUL(r[4], m[comb2]) + MUL(r[5], m[comb3]) + MUL(r[6], m[comb4]);
    apf1 = out - MUL(r[8], m[apf1_src]);
    out = m[apf1_src] + MUL(r[8], apf1);
    apf2 = out - MUL(r[9], m[apf2_src]);
    out = m[apf2_src] + MUL(r[9], apf2);
    R.cell[same_dst] = (int16_t)clamp16(same);
    R.cell[diff_dst] = (int16_t)clamp16(diff);
    R.cell[apf1_dst] = (int16_t)clamp16(apf1);
    R.cell[apf2_dst] = (int16_t)clamp16(apf2);
    out = clamp16(out);
    R.up[right][R.pos] = R.up[right][R.pos | 64] = out;
    R.up[!right][R.pos] = R.up[!right][R.pos | 64] = 0;
}

void p4rev_process(const float *in, float *out, int frames)
{
    int i;

    if (!p4rev_active()) {
        return;
    }
    for (i = 0; i < frames; i++) {
        const int32_t il = clamp16((int32_t)(in[i * 2] * 32768.0f));
        const int32_t ir = clamp16((int32_t)(in[i * 2 + 1] * 32768.0f));
        int32_t l = 0, r = 0;
        int index, k;

        R.down[0][R.pos] = R.down[0][R.pos | 64] = il;
        R.down[1][R.pos] = R.down[1][R.pos | 64] = ir;
        calc_side((int)(R.cycles & 1));
        R.pos = (R.pos + 1) & 63;
        index = (R.pos - NUM_TAPS) & 63;
        for (k = 0; k < NUM_TAPS; k++) {
            /* the upsampling filter is the downsampling one doubled, the centre tap clamped to 16 bits */
            const int32_t c = clamp16(fir_down[k] * 2);

            l += R.up[0][index + k] * c;
            r += R.up[1][index + k] * c;
        }
        l = clamp16(l >> 15);
        r = clamp16(r >> 15);
        out[i * 2] += (float)l * (1.0f / 32768.0f) * ((float)R.evol_l / 32768.0f);
        out[i * 2 + 1] += (float)r * (1.0f / 32768.0f) * ((float)R.evol_r / 32768.0f);
        R.cycles++;
    }
}
