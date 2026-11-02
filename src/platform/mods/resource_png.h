#ifndef P4PC_RESOURCE_PNG_H
#define P4PC_RESOURCE_PNG_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/packages.h"

typedef struct P4ResourcePng {
    uint8_t *rgba;
    uint32_t width, height;
    size_t size;
} P4ResourcePng;
/* Exact, already-contained package file. The shared CPU budget covers encoded
 * input, codec workspace and retained decoded output. No guest/GL work. */
int p4_resource_png_decode(P4PackageFile *file, P4ResourcePng *out,
                           char *error, size_t error_size);
void p4_resource_png_free(P4ResourcePng *image);
#endif
