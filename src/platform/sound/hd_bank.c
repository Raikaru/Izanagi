/* .hd and .sq readers, see hd_bank.h for the layouts. */
#include "hd_bank.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TONE_BYTES 50
#define SPU_RATE 48000.0

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t rd16(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

/* "SCEI" + type, each word reversed in the file. */
static int is_chunk(const uint8_t *img, uint32_t size, uint32_t off, const char type[4])
{
    return off <= size && size - off >= 12 && memcmp(img + off, "IECS", 4) == 0 &&
           img[off + 4] == (uint8_t)type[3] && img[off + 5] == (uint8_t)type[2] &&
           img[off + 6] == (uint8_t)type[1] && img[off + 7] == (uint8_t)type[0];
}

void p4hd_free(P4HdBank *bank)
{
    uint32_t i;

    if (bank == NULL) {
        return;
    }
    for (i = 0; i < bank->nsets; i++) {
        free(bank->sets[i].tones);
    }
    free(bank->sets);
    free(bank->vags);
    memset(bank, 0, sizeof(*bank));
}

int p4hd_parse(const uint8_t *hd, uint32_t size, P4HdBank *out)
{
    uint32_t vagi, setb, nvag, nsets, i;
    uint32_t setoff0;
    P4HdBank b;

    memset(out, 0, sizeof(*out));
    memset(&b, 0, sizeof b);
    if (hd == NULL || !is_chunk(hd, size, 0, "Vers") || !is_chunk(hd, size, 0x10, "Head") || size < 0x38) {
        return -1;
    }
    vagi = rd32(hd + 0x30);
    setb = rd32(hd + 0x34);
    if (!is_chunk(hd, size, vagi, "Vagi") || !is_chunk(hd, size, setb, "Setb")) {
        return -1;
    }

    nvag = rd32(hd + vagi + 0x0c) + 1;
    if (nvag == 0 || nvag > 0x10000 || vagi + 0x10 + 4 * (uint64_t)nvag > size) {
        return -1;
    }
    b.vags = calloc(nvag, sizeof(P4HdVag));
    if (b.vags == NULL) {
        return -1;
    }
    for (i = 0; i < nvag; i++) {
        uint32_t eo = vagi + rd32(hd + vagi + 0x10 + 4 * i);

        if (eo > size || size - eo < 8) {
            p4hd_free(&b);
            return -1;
        }
        b.vags[i].offset = rd32(hd + eo);
        b.vags[i].rate = rd16(hd + eo + 4);
    }
    b.nvags = nvag;

    /* one bank; its table of timbre sets */
    if (rd32(hd + setb + 0x0c) != 0 || size - setb < 0x18) {
        p4hd_free(&b);
        return -1;
    }
    setoff0 = setb + rd32(hd + setb + 0x10);
    if (setoff0 > size || size - setoff0 < 8) {
        p4hd_free(&b);
        return -1;
    }
    nsets = rd32(hd + setoff0) + 1;
    if (nsets == 0 || nsets > 256 || setoff0 + 4 + 4 * (uint64_t)nsets > size) {
        p4hd_free(&b);
        return -1;
    }
    b.sets = calloc(nsets, sizeof(P4HdSet));
    if (b.sets == NULL) {
        p4hd_free(&b);
        return -1;
    }
    b.nsets = nsets;
    {
        uint32_t chunk_end = setb + rd32(hd + setb + 8);

        if (chunk_end > size) {
            chunk_end = size;
        }
        for (i = 0; i < nsets; i++) {
            uint32_t so = setb + rd32(hd + setoff0 + 4 + 4 * i);
            uint32_t end = i + 1 < nsets ? setb + rd32(hd + setoff0 + 4 + 4 * (i + 1)) : chunk_end;
            uint32_t n, k;

            if (so + 2 > end || end > size) {
                p4hd_free(&b);
                return -1;
            }
            n = (end - so - 2) / TONE_BYTES;
            b.sets[i].tones = calloc(n ? n : 1, sizeof(P4HdTone));
            if (b.sets[i].tones == NULL) {
                p4hd_free(&b);
                return -1;
            }
            b.sets[i].ntones = n;
            for (k = 0; k < n; k++) {
                const uint8_t *t = hd + so + 2 + k * TONE_BYTES;

                b.sets[i].tones[k].vag = (uint16_t)rd16(t + 6);
                b.sets[i].tones[k].vol = t[9];
                b.sets[i].tones[k].pan = t[10];
                b.sets[i].tones[k].adsr1 = (uint16_t)rd16(t + 20);
                b.sets[i].tones[k].adsr2 = (uint16_t)rd16(t + 22);
                b.sets[i].tones[k].key = (int8_t)t[11];
                b.sets[i].tones[k].fine = (int8_t)t[12];
                b.sets[i].tones[k].mix = t[19];
            }
        }
    }
    *out = b;
    return 0;
}

/* ------------------------------------------------------------------ ADSR */

/* Seconds for the SPU2 envelope counter to move over `span` (0..0x7fff units) with a linear
 * step of `step`: each counter cycle (one sample, or 2^(shift-11) samples for slow rates)
 * changes the level by step << max(0, 11 - shift). */
static double adsr_linear_seconds(int shift, int step, double span)
{
    double per_cycle = (shift < 11 ? (double)step * (double)(1u << (11 - shift)) : (double)step);
    double cycle = shift > 11 ? (double)(1u << (shift - 11)) : 1.0;

    return span / per_cycle * cycle / SPU_RATE;
}

/* Seconds for an exponentially decreasing envelope (level *= 1 - k/32768 per counter cycle)
 * to fall by the ratio `to` (0 < to < 1); k = 8 << max(0, 11 - shift) for the fixed step. */
static double adsr_exp_seconds(int shift, double to)
{
    double k = 8.0 * (shift < 11 ? (double)(1u << (11 - shift)) : 1.0);
    double cycle = shift > 11 ? (double)(1u << (shift - 11)) : 1.0;
    double f = 1.0 - k / 32768.0;

    if (f <= 0.0) {
        return 0.0;
    }
    return log(to) / log(f) * cycle / SPU_RATE;
}

static float clampf(double v, double lo, double hi)
{
    return (float)(v < lo ? lo : (v > hi ? hi : v));
}

void p4hd_adsr_to_env(uint16_t a1, uint16_t a2, P4SndEnv *env)
{
    const int a_exp = (a1 >> 15) & 1;
    const int a_shift = (a1 >> 10) & 0x1f;
    const int a_step = 7 - ((a1 >> 8) & 3);
    const int d_shift = (a1 >> 4) & 0xf;
    const double sl = (double)((a1 & 0xf) + 1) / 16.0;
    const int s_exp = (a2 >> 15) & 1;
    const int s_dec = (a2 >> 14) & 1;
    const int s_shift = (a2 >> 8) & 0x1f;
    const int s_step = (a2 >> 6) & 3;
    const int r_exp = (a2 >> 5) & 1;
    const int r_shift = a2 & 0x1f;
    double t;

    /* attack: linear, or exponential (a quarter of the rate above 0x6000) */
    if (a_exp) {
        t = adsr_linear_seconds(a_shift, a_step, 0x6000) + 4.0 * adsr_linear_seconds(a_shift, a_step, 0x1fff);
    } else {
        t = adsr_linear_seconds(a_shift, a_step, 0x7fff);
    }
    env->attack = clampf(t, 0.0, 30.0);

    env->decay = sl >= 1.0 ? 0.0f : clampf(adsr_exp_seconds(d_shift, sl), 0.0, 30.0);
    env->sustain = (float)sl;

    env->sustain_decay = 0.0f;
    if (s_dec) {
        int mag = 8 - s_step;

        if (s_exp) {
            t = adsr_exp_seconds(s_shift, 0.001);
        } else {
            t = adsr_linear_seconds(s_shift, mag, 0x7fff * sl);
        }
        env->sustain_decay = t > 60.0 ? 0.0f : clampf(t, 0.0, 60.0);
    }

    if (r_exp) {
        t = adsr_exp_seconds(r_shift, 0.001);
    } else {
        t = adsr_linear_seconds(r_shift, 8, 0x7fff);
    }
    /* never an instant cut: it clicks, and the mixer envelope reaches silence in a few ms */
    env->release = clampf(t, 0.003, 30.0);
}

/* ------------------------------------------------------------------ .sq */

void p4sq_free(P4Sq *sq)
{
    uint32_t i;

    if (sq == NULL) {
        return;
    }
    for (i = 0; i < sq->nsets; i++) {
        free(sq->sets[i].seqs);
    }
    free(sq->sets);
    free(sq->image);
    memset(sq, 0, sizeof(*sq));
}

int p4sq_parse(const uint8_t *sq, uint32_t size, P4Sq *out)
{
    P4Sq s;
    uint32_t base, nsets, i;

    memset(out, 0, sizeof(*out));
    memset(&s, 0, sizeof s);
    if (sq == NULL || !is_chunk(sq, size, 0, "Vers") || !is_chunk(sq, size, 0x10, "Sequ") || size < 0x2c) {
        return -1;
    }
    base = rd32(sq + 0x28);
    if (!is_chunk(sq, size, base, "Sesq") || size - base < 0x20) {
        return -1;
    }
    s.image = malloc(size);
    if (s.image == NULL) {
        return -1;
    }
    memcpy(s.image, sq, size);
    nsets = rd32(sq + base + 0x0c) + 1;
    if (nsets == 0 || nsets > 256 || base + 0x20 + 4 * (uint64_t)nsets > size) {
        p4sq_free(&s);
        return -1;
    }
    s.sets = calloc(nsets, sizeof(P4SqSet));
    if (s.sets == NULL) {
        p4sq_free(&s);
        return -1;
    }
    s.nsets = nsets;
    for (i = 0; i < nsets; i++) {
        uint32_t so = base + rd32(sq + base + 0x20 + 4 * i);
        uint32_t nseqs, k;
        P4SqSet *set = &s.sets[i];

        if (so > size || size - so < 0x10) {
            p4sq_free(&s);
            return -1;
        }
        nseqs = rd32(sq + so) + 1;
        if (nseqs == 0 || nseqs > 1024 || so + 0x10 + 4 * (uint64_t)nseqs > size) {
            p4sq_free(&s);
            return -1;
        }
        set->vol = sq[so + 8];
        set->pan = sq[so + 9];
        set->ticks_per_sec = (uint16_t)rd16(sq + so + 10);
        set->seqs = calloc(nseqs, sizeof(P4SqSeq));
        if (set->seqs == NULL) {
            p4sq_free(&s);
            return -1;
        }
        set->nseqs = nseqs;
        for (k = 0; k < nseqs; k++) {
            uint32_t qo = base + rd32(sq + so + 0x10 + 4 * k);
            uint32_t hsz, len;

            if (qo > size || size - qo < 0x10) {
                p4sq_free(&s);
                return -1;
            }
            hsz = rd32(sq + qo);
            len = rd32(sq + qo + 8);
            if (hsz < 0x10 || hsz > size - qo || len > size - qo - hsz) {
                p4sq_free(&s);
                return -1;
            }
            set->seqs[k].vol = sq[qo + 4];
            set->seqs[k].pan = sq[qo + 5];
            set->seqs[k].ticks_per_sec = (uint16_t)rd16(sq + qo + 6);
            set->seqs[k].len = len;
            set->seqs[k].events = s.image + qo + hsz;
        }
    }
    *out = s;
    return 0;
}

/* Variable-length quantity: 7 bits per byte, high bit continues. */
static int read_vlq(const uint8_t *d, uint32_t len, uint32_t *pos, uint32_t *v)
{
    uint32_t x = 0;
    int n;

    for (n = 0; n < 4; n++) {
        uint8_t c;

        if (*pos >= len) {
            return -1;
        }
        c = d[(*pos)++];
        x = (x << 7) | (c & 0x7f);
        if (!(c & 0x80)) {
            *v = x;
            return 0;
        }
    }
    return -1;
}

int p4sq_next_event(const P4SqSeq *seq, uint32_t *pos, P4SqEvent *ev)
{
    return p4sq_read_event(seq, pos, ev, 1);
}

int p4sq_read_event(const P4SqSeq *seq, uint32_t *pos, P4SqEvent *ev, int with_delta)
{
    const uint8_t *d = seq->events;
    const uint32_t len = seq->len;
    uint32_t p = *pos;
    uint32_t delta = 0;
    uint8_t st;

    memset(ev, 0, sizeof(*ev));
    ev->loop_kind = -1;
    if (p >= len) {
        return 0;
    }
    if (with_delta && read_vlq(d, len, &p, &delta) != 0) {
        return -1;
    }
    if (p >= len) {
        return -1;
    }
    ev->delta = delta;
    st = d[p++];
    switch (st) {
    case 0x90:
    case 0xa0:
        if (len - p < 3) {
            return -1;
        }
        ev->type = st == 0x90 ? P4SQ_NOTE_ON : P4SQ_NOTE_OFF;
        ev->tone = (uint16_t)(d[p] << 8 | d[p + 1]);
        ev->vel = d[p + 2];
        p += 3;
        break;
    case 0xb0: {
        uint32_t t;

        if (len - p < 3) {
            return -1;
        }
        ev->type = P4SQ_CONTROL;
        ev->tone = (uint16_t)(d[p] << 8 | d[p + 1]);
        ev->control = d[p + 2];
        p += 3;
        if ((ev->control != 0x0e && ev->control != 0x0f) || read_vlq(d, len, &p, &t) != 0 || len - p < 2) {
            return -1;
        }
        ev->time = (int32_t)t;
        ev->value = (int32_t)(d[p] | d[p + 1] << 8);
        p += 2;
        break;
    }
    case 0xc0:
        if (p >= len) {
            return -1;
        }
        ev->type = P4SQ_META;
        if (d[p] == 0x7f) {
            p += 1;
        } else if (d[p] <= 1) {
            if (len - p < 5) {
                return -1;
            }
            ev->loop_kind = d[p];
            ev->loop_id = d[p + 1];
            ev->loop_offset = (uint32_t)(d[p + 2] | d[p + 3] << 8);
            ev->loop_reps = d[p + 4];
            p += 5;
        } else {
            return -1;
        }
        break;
    case 0xff:
        ev->type = P4SQ_END;
        break;
    default:
        return -1;
    }
    *pos = p;
    return 1;
}
