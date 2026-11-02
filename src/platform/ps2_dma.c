/* SCE DMA library calls the movie task makes (libdma, retail 0x00418df8 and 0x00418f50). The EE register
 * page is plain memory in the port (src/image.c), so a channel is always idle: waiting for it returns at
 * once. GS uploads themselves are replaced at their callers (src/movie/movie_upload.c). */
#include <stdint.h>

/* sceDmaGetChan: the channel register block for channel n (0..9), from the table at 0x0070c410 that
 * holds the EE register addresses; 0 for any other number. */
uint32_t func_00418f50(uint32_t n) {
    return n < 10 ? *(const uint32_t *)P4_G2H(0x0070c410u + n * 4u) : 0;
}

/* The wait loop behind sceDmaSync: polls CHCR.STR until the transfer ends. Nothing starts a transfer here,
 * but a stray STR bit is cleared rather than waited on forever. */
void func_00418df8(uint32_t *chan) {
    if (chan) *chan &= ~0x100u;
}
