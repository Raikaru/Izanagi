/* Retail 003bbf80 (code1_003b.c): copies a user-data array {name, format, count, data} from `src` to
 * `dst`. The name is duplicated (strlen + malloc(len + 1, 0x30002) + strcpy through the RenderWare
 * string slots). The data block is allocated as count * element size (func_003bd110(format), hint
 * 0x3011f); format 3 (string) copies every element string the same way, with a null element kept null,
 * any other format is a memcpy. */
extern unsigned int (*p4_ud_strlen[])(const char *) __asm__("D_008873D4");
extern char *(*p4_ud_strcpy[])(char *, const char *) __asm__("D_008873AC");
extern void *(*p4_ud_malloc[])(int, int) __asm__("D_008873E8");
extern void *p4_ud_stream_write_word(void *stream, void *data, int size) __asm__("func_003df240");
extern void *p4_ud_stream_write_raw(void *stream, void *data, int size) __asm__("func_003e2ab0");
extern int p4_ud_elem_size(int format) __asm__("func_003bd110");
extern void *p4_ud_memcpy(void *dst, const void *src, unsigned int n) __asm__("func_0043f810");

static void p4_ud_dup_string(unsigned int *slot, const char *text)
{
    *slot = 0;
    *slot = (unsigned int)p4_ud_malloc[0]((int)p4_ud_strlen[0](text) + 1, 0x30002);
    if (*slot != 0) {
        p4_ud_strcpy[0]((char *)*slot, text);
    }
}

void func_003bbf80(unsigned int *dst, unsigned int *src)
{
    dst[1] = src[1];
    dst[2] = src[2];
    if (src[0] != 0) {
        dst[0] = 0;
        if (src[0] != 0) {
            p4_ud_dup_string(&dst[0], (const char *)src[0]);
        }
    }
    if (src[3] != 0) {
        int total = (int)dst[2] * p4_ud_elem_size((int)dst[1]);

        dst[3] = (unsigned int)p4_ud_malloc[0](total, 0x3011f);
        if (dst[1] == 3) {
            unsigned int *from = (unsigned int *)src[3];
            unsigned int *to = (unsigned int *)dst[3];
            int i;

            for (i = 0; i < (int)dst[2]; i++, from++, to++) {
                if (*from != 0) {
                    p4_ud_dup_string(to, (const char *)*from);
                } else {
                    *to = 0;
                }
            }
        } else {
            p4_ud_memcpy((void *)dst[3], (const void *)src[3], (unsigned int)total);
        }
    }
}
