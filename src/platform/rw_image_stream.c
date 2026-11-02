/* func_003ebbf0 (496 bytes): RwImageStreamRead(stream arg0): finds the struct chunk (func_003df050(stream, 1, &size, &version), 0 when it is missing), reads the four word header {width,
 * height, depth, stride} (func_003e2910) and creates the image: a 0x30018 byte request to the allocator slot 0x008873F8 for the pool pointed to by the word at 0x008872E0 + *(int32_t *)P4_G2H(0x007648B8u)
 * (gp - 0x4838; gp is 0x007690F0), with image[0] = 0, image[1] = width, image[2] = height, image[3] = depth, image[5] = image[6] = 0, registered by func_003e3b70(0x0070B7C0, image) and given its
 * pixel and palette storage by func_003ea3e0(image) (it sets image[4], the stride, image[5], the pixels, and image[6], the palette). When the stored stride equals the image stride the
 * height * stride pixel bytes are read at once, else row by row: min(stored stride, image stride) bytes per row into the next image row and, when the stored rows are longer, the rest
 * skipped with func_003e2ce0(stream, stored stride - image stride). A depth below 9 then reads the palette, (1 << depth) * 4 bytes, into image[6]. Returns the image.
 * (When the allocator returns 0 retail goes on with a null image: func_003ea3e0(0) and the field reads from address 0x10; the port returns 0 after that call.)
 *
 * The decomp's C for RwImageStreamRead is compiled under its RenderWare name (src/renderware/core/baimage.c), but the marker of this retail address is in the unbuilt
 * src/rw/baimage.c, so an override file waits for that unit and the retail address stayed a trap (its callers func_003db630 / func_003dbe40 use it): plain port C instead. */
#include <stdint.h>

#include "p4pc/gmem.h"

extern int32_t func_003df050(void *stream, int32_t type, uint32_t *size, uint32_t *version);
extern int32_t func_003e2910(void *stream, void *buf, int32_t n);
extern void *func_003e2ce0(void *stream, int32_t n);
extern void func_003e3b70(void *list, void *image);
extern void func_003ea3e0(void *image);

typedef void *(*P4AllocFn_003ebbf0)(uint32_t pool, uint32_t size);

uint32_t *func_003ebbf0(int32_t arg0)
{
    uint32_t size;
    uint32_t version;
    int32_t hdr[4];
    uint32_t *image;
    int32_t width, height, depth;
    int32_t i;
    int32_t off;
    uint32_t pool;

    if (func_003df050(P4_G2H(arg0), 1, &size, &version) == 0) {
        return (uint32_t *)0;
    }
    func_003e2910(P4_G2H(arg0), hdr, 16);
    pool = P4_G32(0x008872E0u + P4_G32(0x007648B8u));
    width = hdr[0];
    height = hdr[1];
    depth = hdr[2];
    image = (uint32_t *)((P4AllocFn_003ebbf0)p4_fn_g2h_sig(P4_G32(0x008873F8u), P4_SIG(2, 1, 0, 0, 0, 0, 0, 0)))(pool, 0x30018);
    if (image != (uint32_t *)0) {
        image[1] = width;
        image[2] = height;
        image[3] = depth;
        image[5] = 0;
        image[6] = 0;
        image[0] = 0;
        func_003e3b70((void *)P4_G2H(0x0070B7C0u), image);
    }
    func_003ea3e0(image);
    if (image == (uint32_t *)0) {
        return (uint32_t *)0;
    }
    if ((int32_t)image[4] == hdr[3]) {
        func_003e2910(P4_G2H(arg0), P4_G2H(image[5]), (int32_t)image[2] * (int32_t)image[4]);
    } else if (hdr[1] > 0) {
        off = 0;
        for (i = 0; i < hdr[1]; i++) {
            if (hdr[3] < (int32_t)image[4]) {
                func_003e2910(P4_G2H(arg0), (uint8_t *)P4_G2H(image[5]) + off, hdr[3]);
            } else {
                func_003e2910(P4_G2H(arg0), (uint8_t *)P4_G2H(image[5]) + off, (int32_t)image[4]);
                func_003e2ce0(P4_G2H(arg0), hdr[3] - (int32_t)image[4]);
            }
            off += (int32_t)image[4];
        }
    }
    if ((int32_t)image[3] < 9) {
        func_003e2910(P4_G2H(arg0), P4_G2H(image[6]), 1 << image[3] << 2);
    }
    return image;
}
