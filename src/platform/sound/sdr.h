/* The IOP sound driver as the EE side sees it: the command set behind sceSdRemote(1, 0x90f0).
 *
 * The retail EE wrappers (ps2_sndcom.c) pack a 64-byte packet
 *     word 0   handler id in bits 24..30 (0 sksound, 1 skhsynth, 4 sksesq)
 *     word 1   sub-command
 *     word 2.. arguments
 * and the IOP module of that id interprets it and returns a result (>= 0 a handle or a status,
 * negative an error). p4sdr_command is that interpreter. The commands, their arguments and the
 * results the game checks are listed in docs/audio.md. */
#ifndef P4PC_SDR_H
#define P4PC_SDR_H

#include <stdint.h>

/* Handler ids. */
#define P4SDR_ID_SKSOUND 0
#define P4SDR_ID_SKHSYNTH 1
#define P4SDR_ID_SKMIDI 2
#define P4SDR_ID_SKMSIN 3
#define P4SDR_ID_SKSESQ 4

/* Error results (the values the IOP modules return). */
#define P4SDR_ERR_PARAM (-101)       /* 0xff9b */
#define P4SDR_ERR_RANGE (-102)       /* 0xff9a */
#define P4SDR_ERR_BAD_SEQ (-201)     /* 0xff37 not loaded / not registered */
#define P4SDR_ERR_BAD_HANDLE (-202)  /* 0xff36 */
#define P4SDR_ERR_NOT_PLAYING (-299) /* 0xfed5 */
#define P4SDR_ERR_CMD (-310)         /* 0xfeca unknown command */
#define P4SDR_ERR_LOAD (-402)        /* 0xfe6e */

/* Runs one command. hdr and sub are packet words 0 and 1; args are words 2.. (nargs <= 6). */
int32_t p4sdr_command(uint32_t hdr, uint32_t sub, const int32_t *args, int nargs);

/* Forgets every bank, player and playing sound, and stops their voices. Used by tests and when
 * the driver is re-initialised. */
void p4sdr_reset(void);

/* SPU RAM allocator of the EE-side sound heap (0x0043bb70, 0x0043bcb8, 0x0043bec8). */
void p4sdr_spu_heap_init(uint32_t mode);
int32_t p4sdr_spu_alloc(uint32_t size);
void p4sdr_spu_free(uint32_t addr);

#endif
