/* Retail 003d8c00 (code1_003d.c): reads a Windows/OS2 .BMP file into a new image object and returns
 * it, or 0 on any failure (the file is closed on every path; the image and the row buffer are freed on
 * the paths that created them). Behaviour, from the retail asm and its machine draft:
 *  - file header: 'BM', then the pixel-data offset (at +10) and the info-header size (at +14);
 *  - OS/2 (size 12): 16-bit width, height, bits per pixel, 3-byte palette entries;
 *  - Windows (any other size): 32-bit width and height, bits per pixel at +14 of the info header,
 *    compression at +16 (RLE4 = 2 is rejected, only RLE8 = 1 is decoded, and only for depths below 24),
 *    colour count at +32, 4-byte palette entries;
 *  - the image is created 8 bit for 8 bpp, 4 bit for 1 and 4 bpp and 32 bit otherwise; palettes are
 *    converted from BGR(X) to the image's RGBA order with alpha 0xff;
 *  - rows are read bottom-up, each row rounded up to a multiple of 4 bytes, and stored through the row
 *    writer func_003d8760 (its last argument is 1 for RLE data). Errors 0x19d are reported through
 *    func_003df590 / func_003df4d0 as in retail. */
extern void *p4_bmp_open(int a, int b, const char *path) __asm__("func_003e2f60");
extern int p4_bmp_read(void *file, void *buffer, int size) __asm__("func_003e2910");
extern void p4_bmp_close(void *file, int flag) __asm__("func_003e2e40");
extern int p4_bmp_skip(void *file, int bytes) __asm__("func_003e2ce0");
extern unsigned char *p4_bmp_image_create(int width, int height, int depth) __asm__("func_003ea2c0");
extern int p4_bmp_image_alloc(void *image) __asm__("func_003ea3e0");
extern void p4_bmp_image_free(void *image) __asm__("func_003ea370");
extern void p4_bmp_row(void *image, int row, void *data, int bpp, int rle) __asm__("func_003d8760");
extern int p4_bmp_error_code(unsigned int code, int size) __asm__("func_003df590");
extern void p4_bmp_error_post(void *record) __asm__("func_003df4d0");
extern void *(*p4_bmp_malloc[])(int size, int flags) __asm__("D_008873E8");
extern void (*p4_bmp_free[])(void *) __asm__("D_008873EC");
extern void *p4_bmp_memset(void *dst, int value, unsigned int n) __asm__("func_0043f9c8");

static void p4_bmp_report(unsigned int code, int size)
{
    unsigned int record[2];

    record[0] = 413;
    record[1] = (unsigned int)p4_bmp_error_code(code, size);
    p4_bmp_error_post(record);
}

unsigned char *func_003d8c00(const char *path)
{
    unsigned char hdr[1036];
    unsigned char marker[4];
    unsigned char *image;
    unsigned char *palette;
    unsigned char *rows;
    unsigned char code[2];
    void *file;
    int offset;
    int info_size;
    int info_bytes;
    int width;
    int height;
    int bpp;
    int compression;
    int colors;
    int depth;
    int consumed;
    int count;
    int i;
    int row_bytes;
    int row_alloc;
    int bytes_read;

    file = p4_bmp_open(2, 1, path);
    if (file == 0) {
        return 0;
    }
    if (p4_bmp_read(file, marker, 4) != 4 || *(unsigned short *)marker != 0x4d42) {
        p4_bmp_close(file, 0);
        return 0;
    }
    /* 14 more bytes land at hdr + 2: the rest of the file header, then the info-header size. */
    if (p4_bmp_read(file, hdr + 2, 14) != 14) {
        p4_bmp_close(file, 0);
        return 0;
    }
    offset = *(int *)(hdr + 8);
    info_size = *(int *)(hdr + 12);
    info_bytes = info_size - 4;
    bytes_read = p4_bmp_read(file, hdr, info_bytes);
    if (info_bytes != bytes_read) {
        p4_bmp_close(file, 0);
        return 0;
    }
    consumed = info_size + 14;
    if (info_size == 12) {
        width = *(short *)(hdr + 0);
        height = *(short *)(hdr + 2);
        bpp = *(short *)(hdr + 6);
        compression = 0;
        colors = 0;
    } else {
        width = *(int *)(hdr + 0);
        height = *(int *)(hdr + 4);
        bpp = *(short *)(hdr + 10);
        compression = *(int *)(hdr + 12);
        colors = *(int *)(hdr + 28);
        if (compression == 2) {
            p4_bmp_report(0x80000009u, 0);
            p4_bmp_close(file, 0);
            return 0;
        }
    }
    count = 1 << (bpp & 0x1f);
    if (colors < 1 || colors > count) {
        colors = count;
    }
    if (bpp == 0x20 || bpp == 0x18 || bpp == 0x10 || bpp == 0xf) {
        depth = 32;
    } else if (bpp == 8) {
        depth = 8;
    } else if (bpp == 4 || bpp == 1) {
        depth = 4;
    } else {
        depth = 32;
    }
    image = p4_bmp_image_create(width, height, depth);
    if (image == 0) {
        p4_bmp_close(file, 0);
        return 0;
    }
    if (!p4_bmp_image_alloc(image)) {
        p4_bmp_image_free(image);
        p4_bmp_close(file, 0);
        return 0;
    }
    if (bpp < 9) {
        int entry = info_size == 12 ? 3 : 4;

        palette = *(unsigned char **)(image + 0x18);
        if (info_size != 12 && info_size != 0x28) {
            p4_bmp_report(0x80000009u, 0);
            p4_bmp_close(file, 0);
            return 0;
        }
        bytes_read = p4_bmp_read(file, hdr, colors * entry);
        if (colors * entry != bytes_read) {
            p4_bmp_close(file, 0);
            return 0;
        }
        consumed = colors * entry + (info_size == 12 ? 0x1a : 0x36);
        for (i = 0; i < colors; i++) {
            palette[i * 4 + 0] = hdr[i * entry + 2];
            palette[i * 4 + 1] = hdr[i * entry + 1];
            palette[i * 4 + 2] = hdr[i * entry + 0];
            palette[i * 4 + 3] = 0xff;
        }
    }
    if (!p4_bmp_skip(file, offset - consumed)) {
        p4_bmp_image_free(image);
        p4_bmp_close(file, 0);
        return 0;
    }
    row_bytes = (((width * bpp + 7) / 8) + 3) & ~3;
    row_alloc = ((width + 7) & ~7) * ((bpp + 7) / 8);
    rows = (unsigned char *)p4_bmp_malloc[0](row_alloc, 0x10000);
    if (rows == 0) {
        p4_bmp_report(0x80000013u, row_alloc);
        p4_bmp_image_free(image);
        p4_bmp_close(file, 0);
        return 0;
    }
    if (compression != 0 && bpp != 0x18 && bpp != 0x20) {
        int y = 0;

        if (compression != 1) {
            p4_bmp_report(0x80000009u, 0);
            p4_bmp_free[0](rows);
            p4_bmp_image_free(image);
            p4_bmp_close(file, 0);
            return 0;
        }
        while (y < height) {
            int x = 0;
            int done = 0;

            do {
                if (p4_bmp_read(file, code, 2) != 2) {
                    goto fail;
                }
                if (code[0] == 0) {
                    if (code[1] < 3) {
                        if (code[1] != 0) {
                            done = 1;
                            if (code[1] != 1) {
                                goto fail;
                            }
                            y = height - 1;
                        } else {
                            done = 1;
                        }
                    } else {
                        if (p4_bmp_read(file, rows + x, code[1]) != code[1]) {
                            goto fail;
                        }
                        x += code[1];
                        if ((code[1] & 1) != 0 && p4_bmp_read(file, code, 1) != 1) {
                            goto fail;
                        }
                    }
                } else {
                    p4_bmp_memset(rows + x, code[1], code[0]);
                    x += code[0];
                }
            } while (!done);
            p4_bmp_row(image, (height - 1) - y, rows, bpp, 1);
            y++;
        }
    } else {
        for (i = 0; i < height; i++) {
            if (p4_bmp_read(file, rows, row_bytes) != row_bytes) {
                goto fail;
            }
            p4_bmp_row(image, (height - 1) - i, rows, bpp, 0);
        }
    }
    p4_bmp_free[0](rows);
    p4_bmp_close(file, 0);
    return image;

fail:
    p4_bmp_free[0](rows);
    p4_bmp_image_free(image);
    p4_bmp_close(file, 0);
    return 0;
}
