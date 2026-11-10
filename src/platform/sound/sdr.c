/* The IOP sound driver (sksound, skhsynth, sksesq) on the mixer. See sdr.h and docs/audio.md.
 *
 * State mirrors the IOP modules' tables:
 *   banks     skhsynth entries: a .hd in IOP RAM and a .bd in SPU RAM (8 slots, the module's
 *             maxentry default)
 *   voices    skhsynth instances ("tracks"): a bank loaded into the synthesizer (8 slots)
 *   sq data   sksesq entries: a .sq in IOP RAM (8 slots)
 *   players   sksesq players: a synthesizer instance bound to sq data; the player handle is
 *             the instance handle (8 slots)
 *   plays     running SE sequences (selected by set and sequence number on a player)
 * A running sequence is advanced by the mixer's tick hook, so notes start on the audio clock
 * (5 ms resolution). Notes become mixer sample voices with the tone's VAG, ADSR, volume and pan.
 *
 * Everything is serialised by one lock, taken before the mixer's lock. */
#include <math.h>

#include "sdr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "hd_bank.h"
#include "spu_pitch.h"
#include "p4snd.h"
#include "sdr_mem.h"
#include "vag.h"
#include "vfs.h"

#define MAX_BANKS 8
#define MAX_INST 8
#define MAX_SQ 8
#define MAX_PLAYERS 8
#define PLAYS_PER_PLAYER 8
#define MAX_PLAYS (MAX_PLAYERS * PLAYS_PER_PLAYER)
#define MAX_NOTES 16
#define MAX_SE_VOICES 48
#define SE_TYPE 10 /* the sequence type word the SE commands carry (0xa) */

typedef struct {
    int used;
    uint32_t flags; /* bit 0 header set, bit 1 samples set */
    uint32_t hd_iop, hd_size, bd_spu, bd_size;
} HsBank;

typedef struct {
    P4SndSample s;
    int16_t *buf;
    uint32_t rate;
    int state; /* 0 not decoded, 1 decoded, -1 not decodable */
} VagPcm;

typedef struct {
    int used;
    int bank;
    P4HdBank hd;
    VagPcm *pcm; /* hd.nvags entries */
} HsInst;

typedef struct {
    int used;
    uint32_t iop, size;
    P4Sq sq;
    int valid;
} SqData;

typedef struct {
    int used;
    int inst; /* the handle, also the index of the synthesizer instance */
    int sq;
} SqPlayer;

typedef struct {
    P4SndVoice *v;
    int inst; /* -1 when the slot is free */
} SeVoice;

typedef struct {
    int used;
    int player;
    const P4SqSeq *seq;
    const P4SqSet *set;
    uint32_t pos;
    int pending;
    int no_delta; /* the next event is read from its status byte (a loop jump) */
    P4SqEvent next;
    double remaining; /* ticks until the pending event */
    double tick_hz;
    int ended;
    struct {
        uint16_t tone;
        int sev; /* index into se_voices, -1 none */
        double base_rate;  /* Hz the tone plays at without a pitch ramp */
        uint32_t sample_rate;
        int32_t tone_pitch; /* combine with bend before the one Note2Pitch lookup */
        int ramp;          /* a pitch ramp is running */
        double ramp_t, ramp_dur; /* seconds elapsed, total */
        double ramp_from, ramp_to; /* pitch offset in 1/128 semitone */
        double bend;       /* the pitch offset now, 1/128 semitone */
    } notes[MAX_NOTES];
    struct {
        int id;   /* loop number, -1 free */
        int left; /* repeats left (0: for ever) */
    } loops[8];
} SePlay;

static struct {
    SDL_Mutex *lock;
    int master_l, master_r, muted; /* the 0x80 master volume and the 0x81 mute */
    int presets_tried, presets_ok;
    int16_t presets[10][32]; /* the effect presets of libsd, by mode */
    HsBank banks[MAX_BANKS];
    HsInst inst[MAX_INST];
    SqData sqd[MAX_SQ];
    SqPlayer players[MAX_PLAYERS];
    SePlay plays[MAX_PLAYS];
    SeVoice se_voices[MAX_SE_VOICES];
    int hooked;
    int ticker;
} S;

/* ------------------------------------------------------------------ logging */

static int log_on(void)
{
    static int on = -1;

    if (on < 0) {
        const char *env = getenv("P4PC_SOUND_LOG");

        on = env != NULL && strcmp(env, "1") == 0;
    }
    return on;
}

/* A line on stderr the first time a (handler, sub-command, command) is not implemented. */
static void log_once(uint32_t id, uint32_t sub, int32_t cmd, const char *why)
{
    static uint32_t seen[64];
    static int nseen;
    uint32_t key = id << 24 | (sub & 0xff) << 16 | ((uint32_t)cmd & 0xffff);
    int i;

    for (i = 0; i < nseen; i++) {
        if (seen[i] == key) {
            return;
        }
    }
    if (nseen < 64) {
        seen[nseen++] = key;
    }
    fprintf(stderr, "[sound] not implemented: handler %u sub %u cmd %d (%s)\n", (unsigned)id, (unsigned)sub,
            (int)cmd, why);
}

/* ------------------------------------------------------------------ SPU RAM allocator */

/* EE-side bookkeeping of SPU RAM, retail 0x0043bb70..0x0043bec8: a first-fit list over
 * 0x5010..0x1c0000 (0x1faff0 end with mode != 0) with a high-water tail. Blocks are 16-byte
 * aligned. Only addresses are managed; the memory is in sdr_mem.c. */
#define SPU_HEAP_BASE 0x5010u
#define SPU_HEAP_BLOCKS 32

static struct {
    uint32_t addr, size;
    int free;
    int used;
} spu_blk[SPU_HEAP_BLOCKS];
static int spu_blk_order[SPU_HEAP_BLOCKS];
static int spu_nblk;
static uint32_t spu_end;

void p4sdr_spu_heap_init(uint32_t mode)
{
    memset(spu_blk, 0, sizeof spu_blk);
    spu_end = mode ? 0x200000u : 0x1c0000u;
    spu_blk[0].used = 1;
    spu_blk[0].free = 1;
    spu_blk[0].addr = SPU_HEAP_BASE;
    spu_blk[0].size = spu_end - SPU_HEAP_BASE;
    spu_blk_order[0] = 0;
    spu_nblk = 1;
}

int32_t p4sdr_spu_alloc(uint32_t size)
{
    uint32_t want = (size + 15u) & ~15u;
    int i;

    if (spu_nblk == 0 || want == 0) {
        return -1;
    }
    for (i = 0; i < spu_nblk; i++) {
        int b = spu_blk_order[i];

        if (!spu_blk[b].free || spu_blk[b].size < want) {
            continue;
        }
        if (spu_blk[b].size > want && spu_nblk < SPU_HEAP_BLOCKS) {
            int n;
            int j;

            for (n = 0; n < SPU_HEAP_BLOCKS && spu_blk[n].used; n++) {
            }
            if (n < SPU_HEAP_BLOCKS) {
                spu_blk[n].used = 1;
                spu_blk[n].free = 1;
                spu_blk[n].addr = spu_blk[b].addr + want;
                spu_blk[n].size = spu_blk[b].size - want;
                for (j = spu_nblk; j > i + 1; j--) {
                    spu_blk_order[j] = spu_blk_order[j - 1];
                }
                spu_blk_order[i + 1] = n;
                spu_nblk++;
                spu_blk[b].size = want;
            }
        }
        spu_blk[b].free = 0;
        return (int32_t)spu_blk[b].addr;
    }
    return -1;
}

void p4sdr_spu_free(uint32_t addr)
{
    int i;

    for (i = 0; i < spu_nblk; i++) {
        int b = spu_blk_order[i];

        if (!spu_blk[b].free && spu_blk[b].addr == addr) {
            spu_blk[b].free = 1;
            /* merge with the next block when free */
            if (i + 1 < spu_nblk && spu_blk[spu_blk_order[i + 1]].free) {
                int nb = spu_blk_order[i + 1];
                int j;

                spu_blk[b].size += spu_blk[nb].size;
                spu_blk[nb].used = 0;
                for (j = i + 1; j + 1 < spu_nblk; j++) {
                    spu_blk_order[j] = spu_blk_order[j + 1];
                }
                spu_nblk--;
            }
            /* and with the previous */
            if (i > 0 && spu_blk[spu_blk_order[i - 1]].free) {
                int pb = spu_blk_order[i - 1];
                int j;

                spu_blk[pb].size += spu_blk[b].size;
                spu_blk[b].used = 0;
                for (j = i; j + 1 < spu_nblk; j++) {
                    spu_blk_order[j] = spu_blk_order[j + 1];
                }
                spu_nblk--;
            }
            return;
        }
    }
}

/* ------------------------------------------------------------------ voices */

static void se_voice_kill(int sev)
{
    int p;
    int n;

    if (sev < 0 || S.se_voices[sev].inst < 0) {
        return;
    }
    p4snd_voice_destroy(S.se_voices[sev].v);
    S.se_voices[sev].v = NULL;
    S.se_voices[sev].inst = -1;
    for (p = 0; p < MAX_PLAYS; p++) {
        for (n = 0; n < MAX_NOTES; n++) {
            if (S.plays[p].notes[n].sev == sev) {
                S.plays[p].notes[n].sev = -1;
            }
        }
    }
}

static void kill_inst_voices(int inst)
{
    int i;

    for (i = 0; i < MAX_SE_VOICES; i++) {
        if (S.se_voices[i].inst == inst) {
            se_voice_kill(i);
        }
    }
}

static VagPcm *get_pcm(HsInst *in, int vag)
{
    VagPcm *p;
    HsBank *bank = &S.banks[in->bank];
    uint32_t off;
    uint32_t avail;
    const uint8_t *src;
    P4VagInfo info;

    if ((uint32_t)vag >= in->hd.nvags) {
        return NULL;
    }
    p = &in->pcm[vag];
    if (p->state != 0) {
        return p->state > 0 ? p : NULL;
    }
    p->state = -1;
    off = in->hd.vags[vag].offset;
    if (off >= bank->bd_size) {
        return NULL;
    }
    avail = bank->bd_size - off;
    src = p4sdr_spu_ptr(bank->bd_spu + off, avail);
    if (src == NULL) {
        return NULL;
    }
    if (p4vag_decode(src, avail, NULL, &info) == 0) {
        return NULL;
    }
    p->buf = malloc((size_t)info.frames * sizeof(int16_t));
    if (p->buf == NULL) {
        return NULL;
    }
    p4vag_decode(src, avail, p->buf, &info);
    p->s.pcm = p->buf;
    p->s.frames = info.frames;
    p->s.loop_start = info.loop_start;
    p->s.has_loop = info.has_loop;
    p->rate = in->hd.vags[vag].rate ? in->hd.vags[vag].rate : 44100;
    p->state = 1;
    return p;
}

static void inst_free(int i)
{
    HsInst *in = &S.inst[i];
    uint32_t v;
    int p;

    kill_inst_voices(i);
    for (p = 0; p < MAX_PLAYS; p++) {
        if (S.plays[p].used && S.players[S.plays[p].player].inst == i) {
            S.plays[p].used = 0;
        }
    }
    for (p = 0; p < MAX_PLAYERS; p++) {
        if (S.players[p].used && S.players[p].inst == i) {
            S.players[p].used = 0;
        }
    }
    if (in->pcm != NULL) {
        for (v = 0; v < in->hd.nvags; v++) {
            free(in->pcm[v].buf);
        }
        free(in->pcm);
    }
    p4hd_free(&in->hd);
    memset(in, 0, sizeof(*in));
}

static int se_voice_alloc(int inst)
{
    int i;

    for (i = 0; i < MAX_SE_VOICES; i++) {
        if (S.se_voices[i].inst < 0 && S.se_voices[i].v == NULL) {
            P4SndVoice *v = p4snd_sample_create();

            if (v == NULL) {
                return -1;
            }
            S.se_voices[i].v = v;
            S.se_voices[i].inst = inst;
            return i;
        }
    }
    return -1;
}

static void pan_gains(int pan, float amp, float *l, float *r)
{
    float p = (float)(pan - 64) / 64.0f;

    if (p > 1.0f) {
        p = 1.0f;
    } else if (p < -1.0f) {
        p = -1.0f;
    }
    *l = amp * (p > 0.0f ? 1.0f - p : 1.0f);
    *r = amp * (p < 0.0f ? 1.0f + p : 1.0f);
}

static void note_on(SePlay *pl, uint16_t toneid, int vel)
{
    SqPlayer *pp = &S.players[pl->player];
    HsInst *in = &S.inst[pp->inst];
    const uint32_t set = (uint32_t)toneid >> 8;
    const uint32_t idx = toneid & 0xff;
    const P4HdTone *t;
    VagPcm *pcm;
    P4SndEnv env;
    float l, r, amp;
    int sev, n;

    if (set >= in->hd.nsets || idx >= in->hd.sets[set].ntones) {
        log_once(P4SDR_ID_SKSESQ, 90, toneid, "note on for a tone the bank does not have");
        return;
    }
    t = &in->hd.sets[set].tones[idx];
    pcm = get_pcm(in, t->vag);
    if (pcm == NULL) {
        log_once(P4SDR_ID_SKSESQ, 91, toneid, "tone whose VAG cannot be decoded");
        return;
    }
    for (n = 0; n < MAX_NOTES && pl->notes[n].sev >= 0; n++) {
    }
    if (n == MAX_NOTES) {
        return;
    }
    sev = se_voice_alloc(pp->inst);
    if (sev < 0) {
        log_once(P4SDR_ID_SKSESQ, 92, 0, "no free mixer voice for a note");
        return;
    }
    amp = ((float)vel / 127.0f) * ((float)t->vol / 127.0f) * ((float)pl->seq->vol / 127.0f) *
          ((float)pl->set->vol / 127.0f);
    pan_gains(t->pan, amp, &l, &r);
    p4hd_adsr_to_env(t->adsr1, t->adsr2, &env);
    p4snd_voice_set_gain(S.se_voices[sev].v, 0, l, r);
    p4snd_voice_set_mix(S.se_voices[sev].v, t->mix & 1, t->mix >> 1 & 1, t->mix >> 2 & 1, t->mix >> 3 & 1);
    /* the tone's own pitch: semitones and 1/128 semitone on top of the VAG's rate */
    pl->notes[n].sample_rate = pcm->rate;
    pl->notes[n].tone_pitch = (int32_t)t->key * 128 + t->fine;
    pl->notes[n].base_rate = p4spu_pitch_rate(pcm->rate, pl->notes[n].tone_pitch);
    p4snd_sample_start(S.se_voices[sev].v, &pcm->s, pl->notes[n].base_rate, &env);
    pl->notes[n].tone = toneid;
    pl->notes[n].sev = sev;
    pl->notes[n].ramp = 0;
    pl->notes[n].bend = 0.0;
}

static void note_off(SePlay *pl, uint16_t toneid)
{
    int n;

    for (n = 0; n < MAX_NOTES; n++) {
        if (pl->notes[n].sev >= 0 && pl->notes[n].tone == toneid) {
            p4snd_sample_release(S.se_voices[pl->notes[n].sev].v);
            pl->notes[n].sev = -1;
            return;
        }
    }
}

/* The pitch ramp of a sequence control event (control 0x0e up, 0x0f down), as the synthesizer module does it:
 * every playing note of the tone changes its pitch offset, from where it is now, by value / 100 semitones
 * (the module keeps 1/128 semitone: value * 128 / 100) in `time` sequence ticks, and keeps the offset
 * afterwards. */
static void note_ramp(SePlay *pl, const P4SqEvent *ev)
{
    int n;
    double total = (double)((ev->value & 0xffff) * 128 / 100);
    double dur = (double)(ev->time > 0 ? ev->time : 1) / pl->tick_hz;

    if (ev->control == 0x0f) {
        total = -total;
    }
    for (n = 0; n < MAX_NOTES; n++) {
        if (pl->notes[n].sev >= 0 && pl->notes[n].tone == ev->tone) {
            pl->notes[n].ramp_from = pl->notes[n].bend;
            pl->notes[n].ramp_to = pl->notes[n].bend + total;
            pl->notes[n].ramp_t = 0.0;
            pl->notes[n].ramp_dur = dur;
            pl->notes[n].ramp = 1;
        }
    }
}

/* Advance the existing ramp timing, then add the tone's fine pitch and bend
 * before LIBSD's one integer-table conversion. Keep the mixer's rate cap. */
static void play_ramps(SePlay *pl, double seconds)
{
    int n;

    for (n = 0; n < MAX_NOTES; n++) {
        double f, rate;

        if (pl->notes[n].sev < 0 || !pl->notes[n].ramp) {
            continue;
        }
        pl->notes[n].ramp_t += seconds;
        f = pl->notes[n].ramp_t >= pl->notes[n].ramp_dur ? 1.0 : pl->notes[n].ramp_t / pl->notes[n].ramp_dur;
        pl->notes[n].bend = pl->notes[n].ramp_from + (pl->notes[n].ramp_to - pl->notes[n].ramp_from) * f;
        if (f >= 1.0) {
            pl->notes[n].ramp = 0;
        }
        rate = p4spu_pitch_rate(pl->notes[n].sample_rate, pl->notes[n].tone_pitch + (int32_t)pl->notes[n].bend);
        if (rate > pl->notes[n].base_rate * 4.0) {
            rate = pl->notes[n].base_rate * 4.0;
        }
        p4snd_voice_set_rate(S.se_voices[pl->notes[n].sev].v, rate);
    }
}

/* A loop end (meta 0 / 1): the first time its id is met it takes a repeat count and jumps; later
 * encounters count down and fall through when the count reaches zero (a count of 0 loops for ever).
 * Returns 1 when it jumps. */
static int loop_end(SePlay *pl, const P4SqEvent *ev)
{
    int i, free_slot = -1;

    for (i = 0; i < 8; i++) {
        if (pl->loops[i].id == ev->loop_id) {
            if (pl->loops[i].left == 0) {
                return 1;
            }
            if (--pl->loops[i].left == 0) {
                pl->loops[i].id = -1;
                return 0;
            }
            return 1;
        }
        if (pl->loops[i].id < 0 && free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) {
        log_once(P4SDR_ID_SKSESQ, 96, 0, "loop table overflow");
        return 0;
    }
    pl->loops[free_slot].id = ev->loop_id;
    pl->loops[free_slot].left = ev->loop_reps;
    return 1;
}

static void play_stop(SePlay *pl)
{
    int n;

    for (n = 0; n < MAX_NOTES; n++) {
        if (pl->notes[n].sev >= 0) {
            p4snd_sample_release(S.se_voices[pl->notes[n].sev].v);
            pl->notes[n].sev = -1;
        }
    }
    pl->ended = 1;
}

static int play_active(const SePlay *pl)
{
    int n;

    if (!pl->ended) {
        return 1;
    }
    for (n = 0; n < MAX_NOTES; n++) {
        if (pl->notes[n].sev >= 0 && !p4snd_sample_done(S.se_voices[pl->notes[n].sev].v)) {
            return 1;
        }
    }
    return 0;
}

/* Advances one running sequence by `ticks`. */
static void play_run(SePlay *pl, double ticks)
{
    int guard = 0;

    while (!pl->ended && guard++ < 4096) {
        if (!pl->pending) {
            int r = p4sq_read_event(pl->seq, &pl->pos, &pl->next, !pl->no_delta);

            pl->no_delta = 0;
            if (r <= 0) {
                if (r < 0) {
                    log_once(P4SDR_ID_SKSESQ, 93, 0, "sequence event the reader does not know");
                }
                pl->ended = 1;
                break;
            }
            pl->pending = 1;
            pl->remaining = (double)pl->next.delta;
        }
        if (ticks < pl->remaining) {
            pl->remaining -= ticks;
            break;
        }
        ticks -= pl->remaining;
        pl->pending = 0;
        switch (pl->next.type) {
        case P4SQ_NOTE_ON:
            note_on(pl, pl->next.tone, pl->next.vel);
            break;
        case P4SQ_NOTE_OFF:
            note_off(pl, pl->next.tone);
            break;
        case P4SQ_CONTROL:
            note_ramp(pl, &pl->next);
            break;
        case P4SQ_META:
            if (pl->next.loop_kind >= 0 && pl->next.loop_offset >= 8 && loop_end(pl, &pl->next)) {
                /* the target is a status byte at (offset - 8); the next event is read from there without
                 * its own delta, so a loop whose end sits at delta d repeats every d ticks past the body */
                pl->pos = pl->next.loop_offset - 8;
                pl->no_delta = 1;
            }
            break;
        case P4SQ_END:
            pl->ended = 1;
            break;
        default:
            break;
        }
    }
}

/* The mixer clock: runs every sequence and reaps finished voices. */
static void se_tick(double seconds)
{
    int i;

    if (S.lock == NULL) {
        return;
    }
    SDL_LockMutex(S.lock);
    for (i = 0; i < MAX_PLAYS; i++) {
        SePlay *pl = &S.plays[i];

        if (pl->used && !pl->ended) {
            play_run(pl, seconds * pl->tick_hz);
        }
        if (pl->used) {
            play_ramps(pl, seconds);
        }
    }
    for (i = 0; i < MAX_SE_VOICES; i++) {
        if (S.se_voices[i].inst >= 0 && p4snd_sample_done(S.se_voices[i].v)) {
            se_voice_kill(i);
        }
    }
    SDL_UnlockMutex(S.lock);
}

static void ensure_init(void)
{
    int i;

    if (S.lock == NULL) {
        S.lock = SDL_CreateMutex();
        for (i = 0; i < MAX_SE_VOICES; i++) {
            S.se_voices[i].inst = -1;
        }
        for (i = 0; i < MAX_PLAYS; i++) {
            int n;

            for (n = 0; n < MAX_NOTES; n++) {
                S.plays[i].notes[n].sev = -1;
            }
            for (n = 0; n < 8; n++) {
                S.plays[i].loops[n].id = -1;
            }
        }
    }
}

void p4sdr_reset(void)
{
    int i;

    ensure_init();
    S.muted = 0;
    p4snd_reverb_enable(0);
    p4snd_reverb_volume(0, 0);
    SDL_LockMutex(S.lock);
    for (i = 0; i < MAX_INST; i++) {
        if (S.inst[i].used) {
            inst_free(i);
        }
    }
    for (i = 0; i < MAX_SQ; i++) {
        if (S.sqd[i].used) {
            p4sq_free(&S.sqd[i].sq);
        }
    }
    memset(S.banks, 0, sizeof S.banks);
    memset(S.sqd, 0, sizeof S.sqd);
    memset(S.players, 0, sizeof S.players);
    memset(S.plays, 0, sizeof S.plays);
    for (i = 0; i < MAX_PLAYS; i++) {
        int n;

        for (n = 0; n < MAX_NOTES; n++) {
            S.plays[i].notes[n].sev = -1;
        }
    }
    for (i = 0; i < MAX_SE_VOICES; i++) {
        se_voice_kill(i);
    }
    SDL_UnlockMutex(S.lock);
}

/* ------------------------------------------------------------------ id 0: sksound */

/* ------------------------------------------------------------------ SPU2 effect unit */

static void apply_master(void)
{
    p4snd_set_master(S.muted ? 0.0f : (float)(S.master_l + S.master_r) / 254.0f);
}

/* The work area of each effect mode in bytes (the SPU's: room, studio A, B, C, hall, space echo, echo, delay,
 * pipe). */
static const uint32_t rev_size[10] = { 0, 0x26C0, 0x1F40, 0x4840, 0x6FE0, 0xADE0, 0xF6C0, 0x18040, 0x18040, 0x3C00 };

/* The effect presets are data of libsd (LIB31/LIBSD.IRX on the disc): nine records of 0x44 bytes, a zero word
 * and the 32 registers, in mode order, the first one the room preset (found by its first registers). */
static int load_presets(void)
{
    static const uint8_t room[16] = { 0x7d, 0x00, 0x5b, 0x00, 0x80, 0x6d, 0xb8, 0x54,
                                      0xd0, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x80, 0xba };
    uint8_t *buf;
    long long end, got = 0, i;
    int fd, m, k;

    if (S.presets_tried) {
        return S.presets_ok;
    }
    S.presets_tried = 1;
    fd = vfs_open("LIB31/LIBSD.IRX", 0);
    if (fd < 0) {
        return 0;
    }
    end = vfs_seek(fd, 0, 2);
    vfs_seek(fd, 0, 0);
    buf = end > 0 && end < (1 << 24) ? (uint8_t *)malloc((size_t)end) : NULL;
    while (buf != NULL && got < end) {
        int n = vfs_read(fd, buf + got, (int)(end - got > (1 << 20) ? (1 << 20) : end - got));

        if (n <= 0) {
            break;
        }
        got += n;
    }
    vfs_close(fd);
    if (buf == NULL || got != end) {
        free(buf);
        return 0;
    }
    for (i = 0; i + 16 + 8 * 0x44 <= end; i++) {
        if (memcmp(buf + i, room, sizeof room) == 0) {
            for (m = 1; m <= 9; m++) {
                const uint8_t *r = buf + i + (m - 1) * 0x44;

                for (k = 0; k < 32; k++) {
                    S.presets[m][k] = (int16_t)(r[k * 2] | r[k * 2 + 1] << 8);
                }
            }
            S.presets_ok = 1;
            break;
        }
    }
    free(buf);
    return S.presets_ok;
}

/* sceSdSetEffectAttr: the mode selects the preset (0: off) and the work area; mode | 0x100 clears the area, which
 * loading a preset does anyway. The delay and feedback only matter to the echo and delay modes. */
static void reverb_mode(int32_t mode, int32_t delay, int32_t feedback)
{
    int m = mode & 0xff;
    static const int16_t none[32];

    (void)delay;
    (void)feedback;
    if (m > 9) {
        return;
    }
    if (m == 0) {
        p4snd_reverb_setup(none, 0);
        return;
    }
    if (!load_presets()) {
        log_once(P4SDR_ID_SKSOUND, 3, m, "the effect presets (LIB31/LIBSD.IRX) are not on the disc: no reverb");
        return;
    }
    if (m == 7 || m == 8) {
        log_once(P4SDR_ID_SKSOUND, 4, m, "echo and delay effect modes: the delay and feedback are not applied");
    }
    p4snd_reverb_setup(S.presets[m], rev_size[m]);
}

/* sub 2 (the "sdr control" command): args {kind, op, ...}. The retail module turns these into
 * SPU2 register writes (master volume, mute, the effect unit's mode, volume and enable): here they set the
 * mixer's master gain and its reverb unit. */
static int32_t sksound_control(const int32_t *a, int n)
{
    int32_t op = n > 1 ? a[1] : 0;

    switch (op) {
    case 0x80: /* master volume left, right (0..127) */
        S.master_l = n > 2 ? a[2] : 127;
        S.master_r = n > 3 ? a[3] : 127;
        apply_master();
        return 0;
    case 1: /* start the module's tick */
    case 2: /* stop it */
        return 0;
    case 0x81: /* core attributes 6 and 7, the cores' mute */
        S.muted = n > 2 && a[2] != 0;
        apply_master();
        return 0;
    case 0x82: /* core attributes 2 and 3: the effect unit of both cores on or off */
        p4snd_reverb_enable(n > 2 && a[2] != 0);
        return 0;
    case 0x83: /* the effect mode, sceSdSetEffectAttr: mode | 0x100 clears the work area, delay, feedback */
        reverb_mode(n > 2 ? a[2] : 0, n > 3 ? a[3] : 0, n > 4 ? a[4] : 0);
        return 0;
    case 0x84: /* the effect volume (EVOLL, EVOLR) left, right, 0..127 of 0x7fff */
        p4snd_reverb_volume((n > 2 ? a[2] : 0) * 0x7fff / 127, (n > 3 ? a[3] : 0) * 0x7fff / 127);
        return 0;
    case 0x8f: /* core attribute 10, the digital output mode: nothing to do for the mixer */
        return 0;
    default:
        return P4SDR_ERR_PARAM;
    }
}

static int32_t sksound_command(uint32_t sub, const int32_t *a, int n)
{
    switch (sub) {
    case 0:
    case 1:
        return 0;
    case 2:
        return sksound_control(a, n);
    case 3:
        return n > 0 && a[0] == 0 ? 0 : P4SDR_ERR_CMD; /* the tick counter, read only */
    case 10: /* voice transfer: {channel, IOP address, SPU address, bytes} */
        if (n < 4) {
            return P4SDR_ERR_PARAM;
        }
        return p4sdr_iop_to_spu((uint32_t)a[1], (uint32_t)a[2], (uint32_t)a[3]) == 0 ? 0 : P4SDR_ERR_PARAM;
    default:
        return P4SDR_ERR_CMD;
    }
}

/* ------------------------------------------------------------------ id 1: skhsynth */

static int free_index(const int *used_flags, size_t stride, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (!*(const int *)((const char *)used_flags + stride * (size_t)i)) {
            return i;
        }
    }
    return -1;
}

static int32_t hsyn_command(uint32_t sub, const int32_t *a, int n)
{
    int32_t cmd = n > 0 ? a[0] : -1;
    int32_t h = n > 1 ? a[1] : -1;

    if (sub == 0) {
        switch (cmd) {
        case 1:
        case 2:
        case 3: { /* header, samples, or both for a bank entry */
            HsBank *b;

            if (h == -1) {
                h = free_index(&S.banks[0].used, sizeof(HsBank), MAX_BANKS);
            }
            if (h < 0 || h >= MAX_BANKS) {
                return P4SDR_ERR_PARAM;
            }
            b = &S.banks[h];
            if (cmd == 1 || cmd == 3) {
                if (n < 4) {
                    return P4SDR_ERR_PARAM;
                }
                b->hd_iop = (uint32_t)a[2];
                b->hd_size = (uint32_t)a[3];
                b->flags |= 1;
            }
            if (cmd == 2) {
                if (n < 4) {
                    return P4SDR_ERR_PARAM;
                }
                b->bd_spu = (uint32_t)a[2];
                b->bd_size = (uint32_t)a[3];
                b->flags |= 2;
            }
            if (cmd == 3) {
                if (n < 6) {
                    return P4SDR_ERR_PARAM;
                }
                b->bd_spu = (uint32_t)a[4];
                b->bd_size = (uint32_t)a[5];
                b->flags |= 2;
            }
            b->used = 1;
            return h;
        }
        case 5: { /* load a bank entry into the synthesizer: {handle or -1, bank, 0..15} */
            HsInst *in;
            const uint8_t *hd;
            uint32_t v;
            int bank = n > 2 ? a[2] : -1;

            if (h == -1) {
                h = free_index(&S.inst[0].used, sizeof(HsInst), MAX_INST);
            }
            if (h < 0 || h >= MAX_INST || bank < 0 || bank >= MAX_BANKS) {
                return P4SDR_ERR_PARAM;
            }
            if (!S.banks[bank].used || S.banks[bank].flags != 3) {
                return P4SDR_ERR_BAD_SEQ;
            }
            if (n > 3 && (uint32_t)a[3] >= 16) {
                return P4SDR_ERR_RANGE;
            }
            in = &S.inst[h];
            if (in->used) {
                inst_free(h);
            }
            hd = p4sdr_iop_ptr(S.banks[bank].hd_iop, S.banks[bank].hd_size);
            if (hd == NULL || p4hd_parse(hd, S.banks[bank].hd_size, &in->hd) != 0) {
                fprintf(stderr, "[sound] bank header at IOP 0x%x (%u bytes) is not a sound header\n",
                        (unsigned)S.banks[bank].hd_iop, (unsigned)S.banks[bank].hd_size);
                return P4SDR_ERR_LOAD;
            }
            in->pcm = calloc(in->hd.nvags, sizeof(VagPcm));
            if (in->pcm == NULL) {
                p4hd_free(&in->hd);
                return P4SDR_ERR_LOAD;
            }
            for (v = 0; v < in->hd.nvags; v++) {
                in->pcm[v].buf = NULL;
            }
            in->bank = bank;
            in->used = 1;
            return h;
        }
        default:
            log_once(P4SDR_ID_SKHSYNTH, sub, cmd, "hsyn command");
            return P4SDR_ERR_CMD;
        }
    }
    if (sub == 1) {
        switch (cmd) {
        case 1:
        case 2:
        case 3:
            if (n < 2 || h < 0 || h >= MAX_BANKS) {
                return P4SDR_ERR_PARAM;
            }
            if (cmd == 3) {
                memset(&S.banks[h], 0, sizeof(S.banks[h]));
            } else {
                S.banks[h].flags &= ~(cmd == 1 ? 1u : 2u);
            }
            return 0;
        case 5:
            if (n < 2 || h < 0 || h >= MAX_INST) {
                return P4SDR_ERR_PARAM;
            }
            if (S.inst[h].used) {
                inst_free(h);
            }
            return 0;
        default:
            log_once(P4SDR_ID_SKHSYNTH, sub, cmd, "hsyn command");
            return P4SDR_ERR_CMD;
        }
    }
    log_once(P4SDR_ID_SKHSYNTH, sub, cmd, "hsyn sub-command");
    return P4SDR_ERR_CMD;
}

/* ------------------------------------------------------------------ id 4: sksesq */

static int player_of(int32_t handle)
{
    int i;

    for (i = 0; i < MAX_PLAYERS; i++) {
        if (S.players[i].used && S.players[i].inst == handle) {
            return i;
        }
    }
    return -1;
}

/* A play is named by its number within the player (what sceSESq_SelectSeq hands out). */
static SePlay *play_of(int player, int32_t handle)
{
    if (player < 0 || handle < 0 || handle >= PLAYS_PER_PLAYER ||
        !S.plays[player * PLAYS_PER_PLAYER + handle].used) {
        return NULL;
    }
    return &S.plays[player * PLAYS_PER_PLAYER + handle];
}

static int32_t sesq_command(uint32_t sub, const int32_t *a, int n)
{
    int32_t cmd = n > 0 ? a[0] : -1;
    int32_t h = n > 1 ? a[1] : -1;

    if (sub == 0) {
        if (cmd == 0) { /* register .sq data: {-1 or entry, IOP address, bytes} */
            SqData *d;
            const uint8_t *sq;

            if (h == -1) {
                h = free_index(&S.sqd[0].used, sizeof(SqData), MAX_SQ);
            }
            if (h < 0 || h >= MAX_SQ || n < 4) {
                return P4SDR_ERR_PARAM;
            }
            d = &S.sqd[h];
            if (d->used) {
                p4sq_free(&d->sq);
            }
            memset(d, 0, sizeof(*d));
            d->used = 1;
            d->iop = (uint32_t)a[2];
            d->size = (uint32_t)a[3];
            sq = p4sdr_iop_ptr(d->iop, d->size);
            d->valid = sq != NULL && p4sq_parse(sq, d->size, &d->sq) == 0;
            if (!d->valid) {
                fprintf(stderr, "[sound] sequence data at IOP 0x%x (%u bytes) is not an SE sequence file\n",
                        (unsigned)d->iop, (unsigned)d->size);
            }
            return h;
        }
        if (cmd == 5) { /* player: {synthesizer instance handle or -1, sq data entry} */
            int sqd = n > 2 ? a[2] : -1;
            int p;

            if (h == -1) {
                h = free_index(&S.inst[0].used, sizeof(HsInst), MAX_INST);
            }
            if (h < 0 || h >= MAX_INST || sqd < 0 || sqd >= MAX_SQ) {
                return P4SDR_ERR_PARAM;
            }
            if (!S.sqd[sqd].used) {
                return P4SDR_ERR_BAD_SEQ;
            }
            if (!S.inst[h].used) {
                return P4SDR_ERR_BAD_HANDLE;
            }
            if (!S.sqd[sqd].valid) {
                return P4SDR_ERR_LOAD;
            }
            p = free_index(&S.players[0].used, sizeof(SqPlayer), MAX_PLAYERS);
            if (p < 0) {
                return P4SDR_ERR_PARAM;
            }
            S.players[p].used = 1;
            S.players[p].inst = h;
            S.players[p].sq = sqd;
            return h;
        }
        log_once(P4SDR_ID_SKSESQ, sub, cmd, "sesq command");
        return P4SDR_ERR_CMD;
    }
    if (sub == 1) {
        if (cmd == 0) {
            if (n < 2 || h < 0 || h >= MAX_SQ) {
                return P4SDR_ERR_PARAM;
            }
            if (S.sqd[h].used) {
                int p;

                for (p = 0; p < MAX_PLAYERS; p++) {
                    if (S.players[p].used && S.players[p].sq == h) {
                        return P4SDR_ERR_PARAM; /* still in use */
                    }
                }
                p4sq_free(&S.sqd[h].sq);
                memset(&S.sqd[h], 0, sizeof(S.sqd[h]));
            }
            return 0;
        }
        if (cmd == 5) {
            int p = player_of(h);
            int i;

            if (p < 0) {
                return P4SDR_ERR_PARAM;
            }
            for (i = 0; i < MAX_PLAYS; i++) {
                if (S.plays[i].used && S.plays[i].player == p) {
                    play_stop(&S.plays[i]);
                    S.plays[i].used = 0;
                }
            }
            S.players[p].used = 0;
            return 0;
        }
        log_once(P4SDR_ID_SKSESQ, sub, cmd, "sesq command");
        return P4SDR_ERR_CMD;
    }
    if (sub == 2) {
        /* {player handle, op, type 10, set, sequence}: op 0 and 7 select a sequence and start
         * it (the result is its play handle), op 2 releases a play handle (a1 = the handle) */
        int32_t op = n > 1 ? a[1] : -1;
        int p = player_of(a[0]);

        if (n < 3 || p < 0) {
            return P4SDR_ERR_PARAM;
        }
        if (a[2] != SE_TYPE) {
            return 0; /* the module ignores a command whose type word is not the SE type */
        }
        if (op == 0 || op == 7) {
            const SqData *d = &S.sqd[S.players[p].sq];
            uint32_t set = n > 3 ? (uint32_t)a[3] & 0xff : 0;
            uint32_t seq = n > 4 ? (uint32_t)a[4] & 0xff : 0;
            int i;

            if (!d->valid || set >= d->sq.nsets || seq >= d->sq.sets[set].nseqs) {
                return P4SDR_ERR_PARAM;
            }
            for (i = 0; i < PLAYS_PER_PLAYER && S.plays[p * PLAYS_PER_PLAYER + i].used; i++) {
            }
            if (i == PLAYS_PER_PLAYER) {
                /* reuse a finished sequence the game never released */
                for (i = 0; i < PLAYS_PER_PLAYER; i++) {
                    if (!play_active(&S.plays[p * PLAYS_PER_PLAYER + i])) {
                        S.plays[p * PLAYS_PER_PLAYER + i].used = 0;
                        break;
                    }
                }
                if (i == PLAYS_PER_PLAYER) {
                    return P4SDR_ERR_PARAM;
                }
            }
            {
                SePlay *pl = &S.plays[p * PLAYS_PER_PLAYER + i];
                int k;

                memset(pl, 0, sizeof(*pl));
                for (k = 0; k < MAX_NOTES; k++) {
                    pl->notes[k].sev = -1;
                }
                for (k = 0; k < 8; k++) {
                    pl->loops[k].id = -1;
                }
                pl->used = 1;
                pl->player = p;
                pl->set = &d->sq.sets[set];
                pl->seq = &d->sq.sets[set].seqs[seq];
                pl->tick_hz = pl->seq->ticks_per_sec ? (double)pl->seq->ticks_per_sec : 1000.0;
                if (!S.hooked) {
                    p4snd_add_tick_hook(se_tick);
                    S.hooked = 1;
                }
                play_run(pl, 0.0);
                return i; /* the play handle: the number within this player */
            }
        }
        if (op == 2) {
            SePlay *pl = play_of(p, a[3]);

            if (pl == NULL) {
                return 0; /* already gone: releasing twice is not an error */
            }
            play_stop(pl);
            pl->used = 0;
            return 0;
        }
        log_once(P4SDR_ID_SKSESQ, sub, op, "sesq play operation");
        return P4SDR_ERR_CMD;
    }
    if (sub == 3) {
        /* {player handle, op, type 10, play handle}: op 1 stops a play */
        int32_t op = n > 1 ? a[1] : -1;
        int p = player_of(a[0]);

        if (n < 4 || p < 0) {
            return P4SDR_ERR_PARAM;
        }
        if (a[2] != SE_TYPE) {
            return 0;
        }
        if (op == 1) {
            /* the retail wrapper sends three arguments, so the handle word is 0 whatever the
             * game passed: it stops that player's play number a[3] */
            SePlay *pl = play_of(p, a[3]);

            if (pl == NULL || !play_active(pl)) {
                return P4SDR_ERR_NOT_PLAYING;
            }
            play_stop(pl);
            return 0;
        }
        log_once(P4SDR_ID_SKSESQ, sub, op, "sesq stop operation");
        return P4SDR_ERR_CMD;
    }
    log_once(P4SDR_ID_SKSESQ, sub, cmd, "sesq sub-command");
    return P4SDR_ERR_CMD;
}

/* ------------------------------------------------------------------ dispatch */

int32_t p4sdr_command(uint32_t hdr, uint32_t sub, const int32_t *args, int nargs)
{
    const uint32_t id = (hdr >> 24) & 0x7f;
    int32_t r;

    ensure_init();
    if (log_on()) {
        fprintf(stderr, "[sound] id=%u sub=%u args=", (unsigned)id, (unsigned)sub);
        for (r = 0; r < nargs; r++) {
            fprintf(stderr, "%s%d", r ? "," : "", (int)args[r]);
        }
        fputc('\n', stderr);
    }
    SDL_LockMutex(S.lock);
    switch (id) {
    case P4SDR_ID_SKSOUND:
        r = sksound_command(sub, args, nargs);
        break;
    case P4SDR_ID_SKHSYNTH:
        r = hsyn_command(sub, args, nargs);
        break;
    case P4SDR_ID_SKSESQ:
        r = sesq_command(sub, args, nargs);
        break;
    default:
        log_once(id, sub, nargs > 0 ? args[0] : -1, "module the game does not use (midi, midi stream)");
        r = P4SDR_ERR_CMD;
        break;
    }
    SDL_UnlockMutex(S.lock);
    if (log_on()) {
        fprintf(stderr, "[sound]   -> %d\n", (int)r);
    }
    return r;
}
