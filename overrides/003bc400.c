/* Retail 003bc400 (code1_003b.c): writes a user-data array {name, format, count, data} to a stream:
 * the name length + 1 (0 for no name) as a word, the name bytes when the length is positive, the
 * format word, the count word, then the elements: format 1 as count words, format 2 as count
 * halfword-swapped words (func_003df1a0), format 3 as strings (length word, then the text), any
 * other format fails. Returns the stream, or 0 on any failure. */
extern unsigned int (*p4_ud_strlen[])(const char *) __asm__("D_008873D4");
extern char *(*p4_ud_strcpy[])(char *, const char *) __asm__("D_008873AC");
extern void *(*p4_ud_malloc[])(int, int) __asm__("D_008873E8");
extern void *p4_ud_stream_write_word(void *stream, void *data, int size) __asm__("func_003df240");
extern void *p4_ud_stream_write_raw(void *stream, void *data, int size) __asm__("func_003e2ab0");
extern void *p4_ud_stream_write_half(void *stream, void *data, int size) __asm__("func_003df1a0");

static int p4_ud_string_length(const char *text)
{
    return text != 0 ? (int)p4_ud_strlen[0](text) + 1 : 0;
}

void *func_003bc400(unsigned int *array, void *stream)
{
    int length = p4_ud_string_length((const char *)array[0]);

    if (!p4_ud_stream_write_word(stream, &length, 4)) {
        return 0;
    }
    if (length > 0 && !p4_ud_stream_write_raw(stream, (void *)array[0], length)) {
        return 0;
    }
    if (!p4_ud_stream_write_word(stream, &array[1], 4)) {
        return 0;
    }
    if (!p4_ud_stream_write_word(stream, &array[2], 4)) {
        return 0;
    }
    if (array[1] == 3) {
        unsigned int *item = (unsigned int *)array[3];
        int i;

        for (i = 0; i < (int)array[2]; i++, item++) {
            length = p4_ud_string_length((const char *)*item);
            if (!p4_ud_stream_write_word(stream, &length, 4)) {
                return 0;
            }
            if (length > 0 && !p4_ud_stream_write_raw(stream, (void *)*item, length)) {
                return 0;
            }
        }
        return stream;
    }
    if (array[1] == 2) {
        return p4_ud_stream_write_half(stream, (void *)array[3], (int)(array[2] << 2)) ? stream : 0;
    }
    if (array[1] == 1) {
        return p4_ud_stream_write_word(stream, (void *)array[3], (int)(array[2] << 2)) ? stream : 0;
    }
    return 0;
}
