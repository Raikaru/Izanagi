/* overrides/003d8c00.c (BMP file reader) against a memory-backed file layer: the callees (file open,
 * read, skip, close, image create/alloc/free, row writer, allocator slots) are stubs defined here, so the
 * test exercises exactly the reader's header parsing, palette conversion, row order, RLE8 decoding and
 * failure paths. Bitmaps are built by hand. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ovr_guest.h"

unsigned char *func_003d8c00(const char *path);

static int checks, failures;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (!(cond)) {                                                   \
            failures++;                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
        }                                                                \
    } while (0)

/* The file under test. */
enum { FILE_BYTES = 4096, IMAGE_BYTES = 0x40, PALETTE_BYTES = 1024 };
static unsigned char *file_data;
static unsigned file_size, file_pos;
static int closed, images_freed, buffers_freed, errors_posted;

/* The image object: only the fields the reader touches (palette pointer at +0x18). */
static unsigned char *image_obj;
static unsigned char *palette_store;
static unsigned char rows_out[16][64];
static int rows_seen[16], rows_rle[16], rows_bpp[16], rows_count;
static int create_w, create_h, create_depth;

void *func_003e2f60(int a, int b, const char *path)
{
    (void)a; (void)b; (void)path;
    file_pos = 0;
    return file_size ? (void *)file_data : 0;
}

int func_003e2910(void *file, void *buffer, int size)
{
    (void)file;
    if (file_pos + (unsigned)size > file_size) {
        size = (int)(file_size - file_pos);
    }
    memcpy(buffer, file_data + file_pos, (size_t)size);
    file_pos += (unsigned)size;
    return size;
}

int func_003e2ce0(void *file, int bytes)
{
    (void)file;
    file_pos += (unsigned)bytes;
    return 1;
}

void func_003e2e40(void *file, int flag)
{
    (void)file; (void)flag;
    closed++;
}

unsigned char *func_003ea2c0(int w, int h, int depth)
{
    create_w = w; create_h = h; create_depth = depth;
    memset(image_obj, 0, IMAGE_BYTES);
    *(uint32_t *)(image_obj + 0x18) = P4_H2G(palette_store);       /* a guest address: the reader loads the palette pointer from the object */
    memset(palette_store, 0, PALETTE_BYTES);
    return image_obj;
}

int func_003ea3e0(void *image) { (void)image; return 1; }
void func_003ea370(void *image) { (void)image; images_freed++; }

void func_003d8760(void *image, int row, void *data, int bpp, int rle)
{
    (void)image;
    rows_seen[rows_count] = row;
    rows_bpp[rows_count] = bpp;
    rows_rle[rows_count] = rle;
    memcpy(rows_out[rows_count], data, 64);
    rows_count++;
}

int func_003df590(unsigned int code, int size) { (void)code; (void)size; return 1; }
void func_003df4d0(void *record) { (void)record; errors_posted++; }
void *func_0043f9c8(void *dst, int v, unsigned int n) { return memset(dst, v, n); }

static void *rw_malloc(int size, int flags) { (void)flags; return p4pc_heap_calloc(1, (size_t)size + 64); }
static void rw_free(void *p) { buffers_freed++; p4pc_heap_free(p); }
OVR_RW_SLOTS(rw_malloc, rw_free)

static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, unsigned v) { put16(p, v); put16(p + 2, v >> 16); }

/* A Windows (info size 40) bitmap header; returns the header size, palette follows. */
static unsigned win_header(int w, int h, int bpp, unsigned compression, unsigned colors, unsigned offset)
{
    memset(file_data, 0, 64);
    file_data[0] = 'B'; file_data[1] = 'M';
    put32(file_data + 10, offset);
    put32(file_data + 14, 40);
    put32(file_data + 18, (unsigned)w);
    put32(file_data + 22, (unsigned)h);
    put16(file_data + 26, 1);
    put16(file_data + 28, (unsigned)bpp);
    put32(file_data + 30, compression);
    put32(file_data + 46, colors);
    return 54;
}

static void reset(void)
{
    closed = images_freed = buffers_freed = errors_posted = rows_count = 0;
    memset(rows_out, 0, sizeof rows_out);
}

static int run_test(int argc, char **argv)
{
    unsigned n;

    (void)argc;
    (void)argv;
    if (ovr_slots_init() != 0) {
        return 1;
    }
    file_data = p4t_galloc(FILE_BYTES + IMAGE_BYTES + PALETTE_BYTES, 16);
    if (file_data == NULL) {
        return 1;
    }
    image_obj = file_data + FILE_BYTES;
    palette_store = image_obj + IMAGE_BYTES;

    /* 8 bpp, 2x2, 4 colours, uncompressed: rows are padded to 4 bytes and arrive bottom-up. */
    n = win_header(2, 2, 8, 0, 4, 54 + 16);
    file_data[n + 0] = 10; file_data[n + 1] = 20; file_data[n + 2] = 30;      /* B G R of colour 0 */
    file_data[n + 4] = 11; file_data[n + 5] = 21; file_data[n + 6] = 31;
    n += 16;
    memcpy(file_data + n, "\x00\x01\xaa\xbb" "\x02\x03\xcc\xdd", 8);
    file_size = n + 8;
    reset();
    CHECK(func_003d8c00("x.bmp") == image_obj);
    CHECK(create_w == 2 && create_h == 2 && create_depth == 8);
    CHECK(palette_store[0] == 30 && palette_store[1] == 20 && palette_store[2] == 10 && palette_store[3] == 0xff);
    CHECK(palette_store[4] == 31 && palette_store[5] == 21 && palette_store[6] == 11 && palette_store[7] == 0xff);
    CHECK(rows_count == 2 && rows_seen[0] == 1 && rows_seen[1] == 0 && rows_bpp[0] == 8 && rows_rle[0] == 0);
    CHECK(rows_out[0][0] == 0x00 && rows_out[0][1] == 0x01 && rows_out[1][0] == 0x02 && rows_out[1][1] == 0x03);
    CHECK(closed == 1 && images_freed == 0 && buffers_freed == 1 && errors_posted == 0);

    /* 24 bpp, 1x1: created 32 bit, no palette, the row is 3 bytes padded to 4. */
    n = win_header(1, 1, 24, 0, 0, 54);
    memcpy(file_data + n, "\x01\x02\x03\x00", 4);
    file_size = n + 4;
    reset();
    CHECK(func_003d8c00("x.bmp") == image_obj);
    CHECK(create_depth == 32 && rows_count == 1 && rows_seen[0] == 0 && rows_bpp[0] == 24);
    CHECK(rows_out[0][0] == 1 && rows_out[0][1] == 2 && rows_out[0][2] == 3);

    /* 8 bpp RLE8, 4x2: row 0 = encoded run of 3 x 0x07 then an absolute run of 3 (1,2,3) with its pad byte, end of line;
     * row 1 = an encoded run of 4 x 0x05, end of bitmap. */
    n = win_header(4, 2, 8, 1, 2, 54 + 8);
    memset(file_data + n, 0, 8);
    n += 8;
    {
        static const unsigned char rle[] = {3, 7, 0, 3, 1, 2, 3, 0, 0, 0, 4, 5, 0, 1};
        memcpy(file_data + n, rle, sizeof rle);
        file_size = n + sizeof rle;
    }
    reset();
    CHECK(func_003d8c00("x.bmp") == image_obj);
    CHECK(rows_count == 2 && rows_rle[0] == 1 && rows_seen[0] == 1 && rows_seen[1] == 0);
    CHECK(rows_out[0][0] == 7 && rows_out[0][1] == 7 && rows_out[0][2] == 7 && rows_out[0][3] == 1 && rows_out[0][4] == 2 && rows_out[0][5] == 3);
    CHECK(rows_out[1][0] == 5 && rows_out[1][3] == 5);

    /* OS/2 header (size 12): 16-bit dimensions, 3-byte palette entries, 1 bpp, 8x1: two colours, the
     * row is one byte padded to 4, the image is created 4 bit. */
    memset(file_data, 0, 64);
    file_data[0] = 'B'; file_data[1] = 'M';
    put32(file_data + 10, 26 + 6);
    put32(file_data + 14, 12);
    put16(file_data + 18, 8); put16(file_data + 20, 1); put16(file_data + 22, 1); put16(file_data + 24, 1);
    file_data[26] = 1; file_data[27] = 2; file_data[28] = 3;       /* colour 0 */
    file_data[29] = 4; file_data[30] = 5; file_data[31] = 6;       /* colour 1 */
    memcpy(file_data + 32, "\xa5\x00\x00\x00", 4);
    file_size = 36;
    reset();
    CHECK(func_003d8c00("x.bmp") == image_obj);
    CHECK(create_w == 8 && create_h == 1 && create_depth == 4 && rows_count == 1 && rows_bpp[0] == 1);
    CHECK(palette_store[0] == 3 && palette_store[1] == 2 && palette_store[2] == 1 && palette_store[3] == 0xff);
    CHECK(palette_store[4] == 6 && palette_store[5] == 5 && palette_store[6] == 4 && rows_out[0][0] == 0xa5);

    /* A truncated palette: the file ends inside it, so the file is closed and 0 returned. */
    n = win_header(2, 2, 8, 0, 4, 54 + 16);
    file_size = n + 6;
    reset();
    CHECK(func_003d8c00("x.bmp") == NULL && closed == 1);

    /* Not a bitmap, and a missing file. */
    n = win_header(1, 1, 8, 0, 0, 54);
    file_data[0] = 'X';
    file_size = n + 8;
    reset();
    CHECK(func_003d8c00("x.bmp") == NULL && closed == 1);
    file_size = 0;
    reset();
    CHECK(func_003d8c00("x.bmp") == NULL && closed == 0);

    /* RLE4 is rejected with a posted error, the file closed and the image never created. */
    n = win_header(2, 2, 4, 2, 2, 54 + 8);
    file_size = n + 8;
    reset();
    CHECK(func_003d8c00("x.bmp") == NULL && errors_posted == 1 && closed == 1);

    printf("bmp_test: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
