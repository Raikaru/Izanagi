/* Retail 003d8300 (code1_003d.c): writes the 40-byte header of a palettised image chunk and its
 * palette, returning the number of bytes written. Header words: 0x28, obj[1] (width), obj[2]
 * (height), then the halfwords 1 and `depth`, four zero words, then either (depth < 9) the palette
 * entry count 1 << depth twice and that many 4-byte entries (obj[6] holds 3-byte RGB triplets at a
 * stride of 4; each is written as B, G, R, 255), or two zero words. The result is 40 plus 4 bytes
 * per palette entry written. */
extern void *p4_img_write_word(void *stream, void *data, int size) __asm__("func_003df240");
extern void *p4_img_write_half(void *stream, void *data, int size) __asm__("func_003df270");
extern void *p4_img_write_raw(void *stream, void *data, int size) __asm__("func_003e2ab0");

int func_003d8300(void *stream, unsigned int depth, unsigned int *image)
{
    int written = 0x28;
    unsigned int word;
    unsigned short half;

    word = 0x28;
    p4_img_write_word(stream, &word, 4);
    word = image[1];
    p4_img_write_word(stream, &word, 4);
    word = image[2];
    p4_img_write_word(stream, &word, 4);
    half = 1;
    p4_img_write_half(stream, &half, 2);
    half = (unsigned short)depth;
    p4_img_write_half(stream, &half, 2);
    word = 0;
    p4_img_write_word(stream, &word, 4);
    p4_img_write_word(stream, &word, 4);
    p4_img_write_word(stream, &word, 4);
    p4_img_write_word(stream, &word, 4);
    if (depth < 9) {
        const unsigned char *palette = (const unsigned char *)image[6];
        int count = 1 << (depth & 0x1f);
        int i;

        word = (unsigned int)count;
        p4_img_write_word(stream, &word, 4);
        p4_img_write_word(stream, &word, 4);
        for (i = 0; i < count; i++) {
            unsigned char entry[4];

            entry[0] = palette[2];
            entry[1] = palette[1];
            entry[2] = palette[0];
            entry[3] = 0xff;
            p4_img_write_raw(stream, entry, 4);
            written += 4;
            palette += 4;
        }
    } else {
        word = 0;
        p4_img_write_word(stream, &word, 4);
        p4_img_write_word(stream, &word, 4);
    }
    return written;
}
