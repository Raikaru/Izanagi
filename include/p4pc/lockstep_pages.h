/* Per-update page witness (NG-7m): a digest of every 4 KiB page of guest RAM at the end of each update after
 * the accepted anchor, written identically by the port and by retail's hook, so an offline comparison can say
 * which hidden state first differs while the fingerprint is still equal. Diagnostic only; off unless asked. */
#ifndef P4PC_LOCKSTEP_PAGES_H
#define P4PC_LOCKSTEP_PAGES_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define P4LP_BASE 0x00100000u
#define P4LP_SIZE 0x01f00000u
#define P4LP_PAGE 4096u
#define P4LP_PAGES (P4LP_SIZE / P4LP_PAGE)
#define P4LP_MAGIC "P4PAGE1\n"

static inline int p4lp_open(FILE **file, const char *path) {
    const uint32_t pages = P4LP_PAGES;
    *file = fopen(path, "wb");
    return *file && fwrite(P4LP_MAGIC, 1, 8, *file) == 8 && fwrite(&pages, 4, 1, *file) == 1;
}
static inline int p4lp_update(FILE *file, uint32_t index, uint32_t game_frame, const uint8_t *ram) {
    static uint32_t digest[P4LP_PAGES];
    for (uint32_t page = 0; page < P4LP_PAGES; page++) {
        uint64_t h = 1469598103934665603ull;
        const uint8_t *p = ram + (size_t)page * P4LP_PAGE;
        for (uint32_t i = 0; i < P4LP_PAGE / 8; i++) {
            uint64_t w;
            memcpy(&w, p + i * 8, 8);
            h = (h ^ w) * 1099511628211ull;
            h ^= h >> 29;
        }
        digest[page] = (uint32_t)(h ^ (h >> 32));
    }
    return fwrite(&index, 4, 1, file) == 1 && fwrite(&game_frame, 4, 1, file) == 1 &&
           fwrite(digest, 4, P4LP_PAGES, file) == P4LP_PAGES && fflush(file) == 0;
}
#endif
