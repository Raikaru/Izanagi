#ifndef P4PC_RESOURCE_REF_H
#define P4PC_RESOURCE_REF_H
#include <stdint.h>
/* Immutable semantic asset identity. Resolve only against the exact prepared
 * real resource service, checking owner/key/revision before pinning its token.
 * An all-zero reference is absent; partial references are invalid. Never save
 * a runtime token/raster/guest pointer in this identity. */
typedef struct P4ResourceRef {
    char owner[64], key[96];
    uint32_t revision;
} P4ResourceRef;
#endif
