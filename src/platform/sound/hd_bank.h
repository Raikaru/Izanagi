/* The sound header of a sample bank (.hd) and the SE sequence data (.sq) that go with a .bd.
 *
 * Both are chunk files (creator "SCEI", stored byte-reversed in each 32-bit word, then a type
 * word: "Vers", "Head", "Vagi", "Setb" in a .hd; "Vers", "Sequ", "Sesq" in a .sq). Offsets
 * inside a chunk are relative to the start of that chunk. What is read here is what the retail
 * IOP modules (modhsyn/modsesq through sksound) read for the sound effect banks the game ships
 * (spu/comse, spu/bse, spu/test); the layouts below are derived from those files and the
 * module code. See docs/audio.md.
 *
 * .hd
 *   +0x00 "Vers" chunk (0x10 bytes)
 *   +0x10 "Head" chunk: +0x08 chunk size, +0x0c header size, +0x10 body (.bd) size, then chunk
 *         offsets at +0x14 program, +0x18 sample set, +0x1c sample, +0x20 vag info, +0x24 setb
 *         (0xffffffff = none; the SE banks have only vag info and setb)
 *   "Vagi": +0x0c maximum index n; offsets[n+1] at +0x10 to 8-byte entries
 *         {u32 byte offset of the VAG in the .bd, u16 sample rate, u8, u8}
 *   "Setb": +0x0c maximum bank index (0); offsets[1] at +0x10 to the timbre sets:
 *         {u32 maximum set index m, u32 offsets[m+1] to the sets}; a set is {u16, 50-byte
 *         tones}, a tone id is (set << 8 | tone). Tone bytes: +6 u16 VAG index, +9 volume,
 *         +10 pan (0x40 centre), +11 s8 pitch in semitones, +12 s8 fine pitch in 1/128
 *         semitone, +19 the voice mix switches (the synthesizer module writes them to the SPU2
 *         VMIXL, VMIXR, VMIXEL, VMIXER registers: bits 0 to 3), +20 u16 ADSR1, +22 u16 ADSR2
 *         (SPU2 registers).
 *
 * .sq
 *   "Sequ" chunk at +0x10: u32 at +0x18 is the "Sesq" chunk offset from the file start.
 *   "Sesq": +0x0c maximum set index n; offsets[n+1] at +0x20 to the sets (chunk relative).
 *   a set: {u32 maximum sequence index m, u32 header size 0x0c, u8 volume, u8 pan, u16 ticks
 *          per second, u32 0xffffffff, u32 offsets[m+1] to the sequences}
 *   a sequence: {u32 header size 0x10, u8 volume, u8 pan, u16 ticks per second, u32 event
 *          bytes, u32 0}, followed by the events: a variable-length delta in ticks (7 bits per
 *          byte, high bit continues) then a status byte:
 *            0x90 note on   {set, tone, velocity}
 *            0xa0 note off  {set, tone, 0}
 *            0xb0 control   {set, tone, control 0x0e or 0x0f, vlq time, u16 value}
 *            0xc0 meta      {0x7f}: a marker, no effect; {0x00|0x01, loop id, u16 offset, u8 repeats}: a
 *                           loop end: jumps to the status byte at (offset - 8) of the event bytes (the offset
 *                           counts from the header's event size word, 8 bytes before the events), for the
 *                           number of repeats (0: for ever); the target's own delta is not waited for
 *            0xff end */
#ifndef P4PC_HD_BANK_H
#define P4PC_HD_BANK_H

#include <stdint.h>

#include "p4snd.h"

typedef struct P4HdVag {
    uint32_t offset; /* byte offset in the .bd */
    uint32_t rate;   /* Hz */
} P4HdVag;

typedef struct P4HdTone {
    uint16_t vag;
    uint8_t vol;
    uint8_t pan;
    uint16_t adsr1;
    uint16_t adsr2;
    int8_t key;      /* +11: pitch offset in semitones */
    int8_t fine;     /* +12: and in 1/128 semitone */
    uint8_t mix;     /* +19: voice routing, bit 0 dry left, 1 dry right, 2 reverb left, 3 reverb right */
} P4HdTone;

typedef struct P4HdSet {
    uint32_t ntones;
    P4HdTone *tones;
} P4HdSet;

typedef struct P4HdBank {
    uint32_t nvags;
    P4HdVag *vags;
    uint32_t nsets;
    P4HdSet *sets;
} P4HdBank;

/* Parses a .hd image. Returns 0, or -1 with *out left empty when it is not a bank header the
 * SE modules would accept. */
int p4hd_parse(const uint8_t *hd, uint32_t size, P4HdBank *out);
void p4hd_free(P4HdBank *bank);

/* SPU2 ADSR1/ADSR2 register values as a mixer envelope. */
void p4hd_adsr_to_env(uint16_t adsr1, uint16_t adsr2, P4SndEnv *env);

/* ------------------------------------------------------------------ .sq */

typedef struct P4SqSeq {
    uint8_t vol, pan;
    uint16_t ticks_per_sec;
    uint32_t len;
    const uint8_t *events; /* points into the P4Sq image */
} P4SqSeq;

typedef struct P4SqSet {
    uint8_t vol, pan;
    uint16_t ticks_per_sec;
    uint32_t nseqs;
    P4SqSeq *seqs;
} P4SqSet;

typedef struct P4Sq {
    uint8_t *image; /* a copy of the file */
    uint32_t nsets;
    P4SqSet *sets;
} P4Sq;

int p4sq_parse(const uint8_t *sq, uint32_t size, P4Sq *out);
void p4sq_free(P4Sq *sq);

enum { P4SQ_NOTE_ON = 1, P4SQ_NOTE_OFF, P4SQ_CONTROL, P4SQ_META, P4SQ_END };

typedef struct P4SqEvent {
    uint32_t delta; /* ticks before the event */
    int type;
    uint16_t tone;  /* (set << 8) | tone for note and control events */
    uint8_t vel;
    uint8_t control;
    int32_t time, value; /* control: duration in ticks and the u16 value */
    int loop_kind;       /* meta 0 or 1: a loop end; -1 for every other event (the 0x7f marker too) */
    int loop_id;         /* the loop's number: loops are counted per id */
    int loop_reps;       /* repeats (0: for ever) */
    uint32_t loop_offset; /* where it jumps back to, in event bytes from the first event */
} P4SqEvent;

/* Reads the event at *pos (advancing it). Returns 1 for an event, 0 at the end of the data,
 * and -1 for a status byte or control the format reader does not know (the sequence is
 * abandoned). */
int p4sq_next_event(const P4SqSeq *seq, uint32_t *pos, P4SqEvent *ev);

/* The same, with the position at a status byte instead of a delta when with_delta is 0 (a loop jump: the
 * target's own delta is not waited for). */
int p4sq_read_event(const P4SqSeq *seq, uint32_t *pos, P4SqEvent *ev, int with_delta);

#endif
