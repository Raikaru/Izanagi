/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: the colour module's engine hooks (docs/rw_clean.md, phase 1).
 *
 * The colour module keeps no globals; opening and closing only count instances (retail 0x0076484C). */
#include "rwc.h"

extern RwInt32 D_0076484C;

// FUN_003DF440
void *_rwColorOpen(void *instance, RwInt32 offset, RwInt32 size)
{
    (void)offset;
    (void)size;
    D_0076484C++;
    return instance;
}

// FUN_003DF460
void *_rwColorClose(void *instance, RwInt32 offset, RwInt32 size)
{
    (void)offset;
    (void)size;
    D_0076484C--;
    return instance;
}
