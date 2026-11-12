/* Retail 003b8930 (code1_003b.c): writes the user-data plugin chunk of an object. The plugin data
 * (pointer at object + the offset word at 0x00886498) is: [0] element count (first byte used), [1]
 * name length, [2] name text, [3] element block (count << 6 bytes), [4] and [0x10] bytes packed into the
 * flags word, +0x1c 16 bytes of ids. Chunk: header (type 1, size, version 0x37002, build 0x37), the
 * word 4, the packed flag word, the name, the element block, the id words, then the nested plugin
 * data (func_003b71b0). Returns the stream, or 0 on any write failure. */
extern unsigned int (*p4_ud_strlen[])(const char *) __asm__("D_008873D4");
extern char *(*p4_ud_strcpy[])(char *, const char *) __asm__("D_008873AC");
extern void *(*p4_ud_malloc[])(int, int) __asm__("D_008873E8");
extern void *p4_ud_stream_write_word(void *stream, void *data, int size) __asm__("func_003df240");
extern void *p4_ud_stream_write_raw(void *stream, void *data, int size) __asm__("func_003e2ab0");
extern int p4_ud_data_size(void *data, int base) __asm__("func_003b7450");
extern int p4_ud_chunk_header(void *stream, int type, int size, int version, int build) __asm__("func_003deff0");
extern int p4_ud_write_nested(void *stream, void *data) __asm__("func_003b71b0");
extern unsigned int p4_ud_data_offset __asm__("D_00886498");

void *func_003b8930(void *stream, unsigned char *object)
{
    unsigned int version_word = 4;
    unsigned int packed;
    unsigned int *data = *(unsigned int **)(object + p4_ud_data_offset);
    int size = 20 + (int)(data[0] << 6) + (int)data[1] + 16;

    size += p4_ud_data_size(data, 20);
    if (!p4_ud_chunk_header(stream, 1, size - 12, 0x37002, 0x37)) {
        return 0;
    }
    if (!p4_ud_stream_write_word(stream, &version_word, 4)) {
        return 0;
    }
    packed = *(unsigned char *)data | ((data[1] << 8) & 0xff00u) | ((data[4] << 16) & 0xff0000u);
    if (!p4_ud_stream_write_word(stream, &packed, 4)) {
        return 0;
    }
    if (!p4_ud_stream_write_raw(stream, (void *)data[2], (int)data[1])) {
        return 0;
    }
    if (!p4_ud_stream_write_raw(stream, (void *)data[3], (int)(data[0] << 6))) {
        return 0;
    }
    if (!p4_ud_stream_write_word(stream, data + 7, 16)) {
        return 0;
    }
    return p4_ud_write_nested(stream, data) ? stream : 0;
}
