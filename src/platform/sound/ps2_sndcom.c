/* The EE side of the IOP sound driver for the PC port: libsd's remote client (sceSdInit,
 * sceSdRemote), the sdr/sndcom command wrappers and the IOP heap.
 *
 * Retail packs commands into a 64-byte packet at 0x0070fcc0 / 0x0070fd00 / 0x0070fd40 and sends
 * it with sceSdRemote(1, 0x90f0, packet, 64) over SIF RPC to sdrdrv, which hands it to sksound.
 * Here each wrapper builds the same packet and p4sdr_command (sdr.c) is the IOP side. The packet
 * layout and the commands are in docs/audio.md.
 *
 * Address  retail name      packet (word 0 / word 1)       arguments used
 * 0043c0c0 sndcomControl    0x00000000 / 2                 kind, op, value, value, value
 * 0043c180 sndcomRpcCmd     0x00000000 / 10                channel, IOP addr, SPU addr, bytes
 * 0043c230 sndcomQueue      0x01000000 / 0 (skhsynth)      cmd, handle, args...
 * 0043c308 sndcomQueueFree  0x01000000 / 1                 cmd, handle
 * 0043c3b0 sndcomDecode     0x04000000 / 0 (sksesq)        cmd, handle, args...
 * 0043c470 sndcomDecodeFree 0x04000000 / 1                 cmd, handle
 * 0043c518 sndcomHandle     0x04000000 / 2                 player, op, type, set/handle, seq
 * 0043c5e8 sndcomHandleSet  0x04000000 / 3                 player, op, type
 *
 * The wrappers take their arguments as plain ints. A wrapper sends only the arguments retail
 * packs (0043c5e8 sends three whatever the caller passes), and the rest of the packet is zero,
 * as retail's memset leaves it.
 *
 * P4PC_SOUND_LOG=1 logs every command and its result to stderr. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4snd.h"
#include "sdr.h"

#define PACKET_WORDS 16

static int32_t sndcom_send(uint32_t hdr, uint32_t sub, const int32_t *args, int nargs)
{
    int32_t a[PACKET_WORDS - 2];
    int i;

    memset(a, 0, sizeof a);
    for (i = 0; i < nargs && i < PACKET_WORDS - 2; i++) {
        a[i] = args[i];
    }
    return p4sdr_command(hdr, sub, a, PACKET_WORDS - 2);
}

int32_t func_0043c0c0(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0)
{
    const int32_t args[5] = {a0, a1, a2, a3, t0};

    return sndcom_send(0x00000000u, 2, args, 5);
}

int32_t func_0043c180(int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    const int32_t args[4] = {a0, a1, a2, a3};

    return sndcom_send(0x00000000u, 10, args, 4);
}

int32_t func_0043c230(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0, int32_t t1)
{
    const int32_t args[6] = {a0, a1, a2, a3, t0, t1};

    return sndcom_send(0x01000000u, 0, args, 6);
}

int32_t func_0043c308(int32_t a0, int32_t a1)
{
    const int32_t args[2] = {a0, a1};

    return sndcom_send(0x01000000u, 1, args, 2);
}

int32_t func_0043c3b0(int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    const int32_t args[4] = {a0, a1, a2, a3};

    return sndcom_send(0x04000000u, 0, args, 4);
}

int32_t func_0043c470(int32_t a0, int32_t a1)
{
    const int32_t args[2] = {a0, a1};

    return sndcom_send(0x04000000u, 1, args, 2);
}

int32_t func_0043c518(int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t t0)
{
    const int32_t args[5] = {a0, a1, a2, a3, t0};

    return sndcom_send(0x04000000u, 2, args, 5);
}

int32_t func_0043c5e8(int32_t a0, int32_t a1, int32_t a2)
{
    const int32_t args[3] = {a0, a1, a2};

    return sndcom_send(0x04000000u, 3, args, 3);
}

/* ------------------------------------------------------------------ libsd remote client */

/* sceSdInit (0x00430e28): binds the SIF RPC server 0x80000701 and waits for it. Returns 0.
 * The port opens the audio device here, and the sound memory and driver state start empty. */
int32_t func_00430e28(void)
{
    p4snd_open();
    p4sdr_reset();
    return 0;
}

/* sceSdRemote (0x00430f80): flag, command, arguments. The game reaches it directly only for
 * libsd parameter writes (0x8010 sceSdSetParam, SPU2 voice and core registers) during start-up;
 * everything else goes through the wrappers above as the 0x90f0 packet. A parameter write
 * changes SPU2 registers the mixer does not have, so it succeeds and does nothing. */
int32_t func_00430f80(int32_t flag, int32_t cmd, int32_t a2, int32_t a3)
{
    static uint32_t seen_mask;

    (void)flag;
    (void)a2;
    (void)a3;
    if (cmd == 0x8010) {
        return 0;
    }
    if (!(seen_mask & 1u)) {
        seen_mask |= 1u;
        fprintf(stderr, "[sound] not implemented: sceSdRemote command 0x%x (first one seen)\n", (unsigned)cmd);
    }
    return 0;
}

/* sdr client start-up (0x0043c010): the argument is the module's maximum track count, then it
 * sets the SPU2 master volume (a no-op here). Returns 0. */
int32_t func_0043c010(int32_t max_tracks)
{
    (void)max_tracks;
    return 0;
}

/* ------------------------------------------------------------------ SPU RAM heap */

/* 0x0043bb70: (table, entries, mode). The block table lives in EE memory in retail; the
 * allocator state is port-owned, so the table argument is not used. */
int32_t func_0043bb70(void *table, int32_t entries, int32_t mode)
{
    (void)entries;
    if (table == NULL) {
        return -1;
    }
    p4sdr_spu_heap_init((uint32_t)mode);
    return 0;
}

/* 0x0043bcb8: SPU RAM address of a block of `size` bytes, or -1. */
int32_t func_0043bcb8(int32_t size)
{
    return p4sdr_spu_alloc((uint32_t)size);
}

/* ------------------------------------------------------------------ IOP heap */

/* Retail wrappers 0x00429d10 (alloc: mode, size, address) and 0x00429d90 (free: address) call
 * the IOP memory server over SIF RPC; sdkSndcom.c uses the returned IOP address as the
 * destination of sound data uploads (SifSetDma into the IOP RAM of sdr_mem.c) and treats 0 as
 * failure. Addresses come from a small first-fit table over the upper half of the IOP's 2 MiB
 * (the lower half holds the IOP's OS and modules), quadword aligned. `mode` and a requested
 * fixed `address` are not honoured: every request is served from the heap. Free returns 0, or
 * -1 for an address that is not allocated. */
#define IOP_HEAP_BASE 0x00100000u
#define IOP_HEAP_SIZE 0x00100000u
#define IOP_HEAP_BLOCKS 64

static struct {
    uint32_t addr;
    uint32_t size;
} iop_blocks[IOP_HEAP_BLOCKS];

int32_t func_00429d10(int32_t mode, uint32_t size, int32_t address)
{
    uint32_t want = (size + 15u) & ~15u;
    uint32_t at = IOP_HEAP_BASE;
    int slot = -1;
    int i, j;

    (void)mode;
    (void)address;
    if (want == 0 || want > IOP_HEAP_SIZE) {
        return 0;
    }
    for (i = 0; i < IOP_HEAP_BLOCKS; i++) {
        if (iop_blocks[i].size == 0 && slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) {
        return 0;
    }
    /* first fit: the lowest address that overlaps no block */
    for (;;) {
        int clash = 0;

        if (at + want > IOP_HEAP_BASE + IOP_HEAP_SIZE) {
            return 0;
        }
        for (j = 0; j < IOP_HEAP_BLOCKS; j++) {
            if (iop_blocks[j].size != 0 && at < iop_blocks[j].addr + iop_blocks[j].size &&
                iop_blocks[j].addr < at + want) {
                at = iop_blocks[j].addr + iop_blocks[j].size;
                clash = 1;
                break;
            }
        }
        if (!clash) {
            break;
        }
    }
    iop_blocks[slot].addr = at;
    iop_blocks[slot].size = want;
    return (int32_t)at;
}

int32_t func_00429d90(int32_t address)
{
    int i;

    for (i = 0; i < IOP_HEAP_BLOCKS; i++) {
        if (iop_blocks[i].size != 0 && iop_blocks[i].addr == (uint32_t)address) {
            iop_blocks[i].size = 0;
            return 0;
        }
    }
    return -1;
}
