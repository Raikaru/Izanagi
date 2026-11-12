/* p4png: writes RGBA8 pixels as a PNG file for p4pc_viewer. See p4png.c. */
#ifndef P4PC_P4PNG_H
#define P4PC_P4PNG_H

#include <stdint.h>

/* Writes w x h RGBA8 pixels to path. rgba holds h rows of w pixels, top row first, four
 * bytes per pixel, rows packed with no padding. Returns 0 on success, -1 on failure. */
int p4png_write_rgba(const char *path, uint32_t w, uint32_t h, const uint8_t *rgba);

#endif /* P4PC_P4PNG_H */
