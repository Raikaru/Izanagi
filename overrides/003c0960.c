/* Retail 003c0960 (code1_003c.c): writes the extension chunk (type 0x10) of an object to a stream and
 * returns the object, or 0 on any failure. Shape, taken from the retail asm and its machine draft:
 *   1. Size pass: 0x18 + the plugin table size + 12 + the streamed size of the array that func_003bebb0
 *      builds (0x10 + sum of element sizes + 12 each, + 12) + the three callback-counted lists
 *      (RpClumpForAllAtomics / 003c0050 / 003bffc0 with the size callbacks 0x003bf360 / 3bf280 / 3bf2d0)
 *      + func_003e3370(registry) + 12.
 *   2. Chunk header, a 12-byte header chunk with the three list counts (callbacks 0x003be940 / 3be960 /
 *      3be980), the plugin table and array written, then the object's own list (obj+8) streamed through
 *      func_003bf3d0 with the shared context below, then the two keyed lists (obj+0x10 and obj+0x18; each
 *      entry gets a 4-byte key chunk and its body) and func_003e33f0.
 * The callbacks receive the whole stack frame in retail, so the context keeps retail's layout: stream,
 * the two-word plugin table, the array {pointer, count} and the success flag. */
extern void (*p4_ext_free[])(void *) __asm__("D_008873EC");
extern int p4_ext_table_open(void *table, int id) __asm__("func_003e5830");
extern int p4_ext_table_size(void *table) __asm__("func_003e5a30");
extern void p4_ext_table_close(void *table) __asm__("func_003e59e0");
extern int p4_ext_table_write(void *table, void *stream) __asm__("func_003e5ae0");
extern int p4_ext_table_key(void *table, int id, void *key) __asm__("func_003e5990");
extern int p4_ext_chunk_header(void *stream, int type, int size, int version, int build) __asm__("func_003deff0");
extern void *p4_ext_write_word(void *stream, void *data, int size) __asm__("func_003df240");
extern void *p4_ext_write_raw(void *stream, void *data, int size) __asm__("func_003e2ab0");
extern int p4_ext_array_build(void *array, void *obj) __asm__("func_003bebb0");
extern void p4_ext_array_free(void *array) __asm__("func_003beb10");
extern int p4_ext_array_write(void *array, void *stream) __asm__("func_003bed10");
extern int p4_ext_elem_size(void *elem) __asm__("func_003c2c90");
extern void p4_ext_elem_release(void *elem) __asm__("func_003c2a80");
extern void p4_ext_count_a(void *obj, void *callback, void *arg) __asm__("RpClumpForAllAtomics");
extern void p4_ext_count_b(void *obj, void *callback, void *arg) __asm__("func_003c0050");
extern void p4_ext_count_c(void *obj, void *callback, void *arg) __asm__("func_003bffc0");
extern int p4_ext_walk_entry(void *entry, void *context) __asm__("func_003bf3d0");
extern int p4_ext_write_b_entry(void *entry, void *stream) __asm__("func_003c39c0");
extern int p4_ext_write_c_entry(void *entry, void *stream) __asm__("func_003e5290");
extern int p4_ext_registry_size(void *registry, void *obj) __asm__("func_003e3370");
extern int p4_ext_registry_write(void *registry, void *stream, void *obj) __asm__("func_003e33f0");
extern char p4_ext_registry __asm__("D_0070AF90");
extern int p4_ext_bias_b __asm__("D_007647A4");
extern int p4_ext_bias_c __asm__("D_007647A0");
extern char p4_ext_size_cb_a __asm__("func_003bf360");
extern char p4_ext_size_cb_b __asm__("func_003bf280");
extern char p4_ext_size_cb_c __asm__("func_003bf2d0");
extern char p4_ext_count_cb_a __asm__("func_003be940");
extern char p4_ext_count_cb_b __asm__("func_003be960");
extern char p4_ext_count_cb_c __asm__("func_003be980");

struct p4_ext_array {
    unsigned int *items;
    int count;
};

struct p4_ext_context {
    void *stream;
    unsigned int table[2];
    struct p4_ext_array array;
    int ok;
};

static void p4_ext_release_array(struct p4_ext_array *array)
{
    int i;

    for (i = 0; i < array->count; i++) {
        p4_ext_elem_release((void *)array->items[i]);
    }
    if (array->items != 0) {
        p4_ext_free[0](array->items);
    }
}

void *func_003c0960(unsigned int *obj, void *stream)
{
    unsigned int table[2];
    struct p4_ext_array sizing;
    struct p4_ext_context ctx;
    unsigned int counts[3];
    int size = 0x18;
    int total;
    unsigned int *node;

    if (!p4_ext_table_open(table, (int)obj[1])) {
        total = 0;
    } else {
        size += p4_ext_table_size(table) + 12;
        p4_ext_table_close(table);
        sizing.count = 0;
        if (!p4_ext_array_build(&sizing, obj)) {
            total = 0;
        } else {
            int elems = 0x10;
            int i;

            for (i = 0; i < sizing.count; i++) {
                elems += p4_ext_elem_size((void *)sizing.items[i]) + 12;
            }
            size += elems + 12;
            p4_ext_array_free(&sizing);
            p4_ext_count_a(obj, &p4_ext_size_cb_a, &size);
            p4_ext_count_b(obj, &p4_ext_size_cb_b, &size);
            p4_ext_count_c(obj, &p4_ext_size_cb_c, &size);
            total = size + p4_ext_registry_size(&p4_ext_registry, obj) + 12;
            size = total;
        }
    }
    if (!p4_ext_chunk_header(stream, 0x10, total, 0x37002, 0x37)) {
        return 0;
    }
    if (!p4_ext_chunk_header(stream, 1, 0xc, 0x37002, 0x37)) {
        return 0;
    }
    counts[0] = counts[1] = counts[2] = 0;
    p4_ext_count_a(obj, &p4_ext_count_cb_a, &counts[0]);
    p4_ext_count_b(obj, &p4_ext_count_cb_b, &counts[1]);
    p4_ext_count_c(obj, &p4_ext_count_cb_c, &counts[2]);
    if (!p4_ext_write_raw(stream, counts, 12)) {
        return 0;
    }
    if (!p4_ext_table_open(ctx.table, (int)obj[1])) {
        return 0;
    }
    if (!p4_ext_table_write(ctx.table, stream)) {
        p4_ext_table_close(ctx.table);
        return 0;
    }
    ctx.array.count = 0;
    if (!p4_ext_array_build(&ctx.array, obj)) {
        return 0;
    }
    if (!p4_ext_array_write(&ctx.array, stream)) {
        p4_ext_release_array(&ctx.array);
        return 0;
    }
    ctx.stream = stream;
    ctx.ok = 1;

    node = (unsigned int *)obj[2];
    while (node != &obj[2]) {
        unsigned int *next = (unsigned int *)*node;
        int more = p4_ext_walk_entry(node - 0x10, &ctx);

        node = next;
        if (!more) {
            break;
        }
    }
    if ((int)counts[1] > 0 && ctx.ok) {
        node = (unsigned int *)obj[4];
        while (node != &obj[4]) {
            unsigned int *next = (unsigned int *)*node;
            unsigned char *entry = (unsigned char *)node - 4 - p4_ext_bias_b;
            unsigned int key[1];

            if (!p4_ext_table_key(ctx.table, *(int *)(entry + 4), key)
                || !p4_ext_chunk_header(ctx.stream, 1, 4, 0x37002, 0x37)
                || !p4_ext_write_word(ctx.stream, key, 4)
                || !p4_ext_write_b_entry(entry, ctx.stream)) {
                ctx.ok = 0;
                break;
            }
            node = next;
        }
    }
    if ((int)counts[2] > 0 && ctx.ok) {
        node = (unsigned int *)obj[6];
        while (node != &obj[6]) {
            unsigned int *next = (unsigned int *)*node;
            unsigned char *entry = (unsigned char *)node - 4 - p4_ext_bias_c;
            unsigned int key[1];

            if (!p4_ext_table_key(ctx.table, *(int *)(entry + 4), key)
                || !p4_ext_chunk_header(ctx.stream, 1, 4, 0x37002, 0x37)
                || !p4_ext_write_word(ctx.stream, key, 4)
                || !p4_ext_write_c_entry(entry, ctx.stream)) {
                ctx.ok = 0;
                break;
            }
            node = next;
        }
    }
    p4_ext_release_array(&ctx.array);
    ctx.array.items = 0;
    p4_ext_table_close(ctx.table);
    if (!ctx.ok) {
        return 0;
    }
    if (!p4_ext_registry_write(&p4_ext_registry, stream, obj)) {
        return 0;
    }
    return obj;
}
