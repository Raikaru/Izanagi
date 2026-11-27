#ifndef P4GL_GOLDEN_H
#define P4GL_GOLDEN_H
#include "p4pc/golden_texture.h"
/* Render/context thread. PS2 mode refuses before any GL call. New GL texture,
 * caller-owned through ordinary glDeleteTextures; zero on error. Host handles
 * never enter a guest slot. Upload keeps source mip levels, sRGB and alpha. */
uint32_t p4gl_golden_texture_upload(const P4GoldenTexture *texture);
#endif
