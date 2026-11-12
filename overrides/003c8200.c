/* Retail 003c8200 (code1_003c.c): builds the triangle-strip mesh header of an RpMeshHeader-style
 * mesh from its list of 0x14-byte mesh records (count at mesh+4, records at mesh+8).
 *  1. The records are sorted through pointers (qsort with func_003c6080 as the comparison, key = word 2,
 *     the material id) and grouped by that key; each group gets {_, count, key}.
 *  2. For every group the records are copied into a scratch array, func_003c70e0 (stripifier, args: records,
 *     count, &list, arg1) and func_003c7d90 (&list, arg2) turn them into a linked list of index runs
 *     {indices, count, ..., next}; each run becomes a freshly allocated {indices*, count, key,
 *     indices...}. The list nodes come from a free list at [globals + offset + 4] created for the call.
 *  3. The runs are packed into one allocation from func_003c54a0: a 16-byte header (1, run count, total
 *     indices, 0, the engine's mesh serial in halfword +6 which is then incremented), 12 bytes per run
 *     {index pointer, count, key} and the 16-bit indices. Returns the header, or 0 when the first
 *     allocation fails. Same shapes as retail, including the 16-bit run counter. */
extern void *(*p4_sm_malloc[])(int size, int flags) __asm__("D_008873E8");
extern void (*p4_sm_free[])(void *) __asm__("D_008873EC");
extern void (*p4_sm_node_free[])(void *freelist, void *node) __asm__("D_008873FC");
extern void p4_sm_qsort(void *base, unsigned int n, unsigned int size, void *compare) __asm__("func_00440bb8");
extern void *p4_sm_compare __asm__("func_003c6080");
extern void *p4_sm_freelist_create(int size, int count, int align, int flags) __asm__("func_003e1210");
extern void p4_sm_freelist_destroy(void *freelist) __asm__("func_003e12f0");
extern void p4_sm_strip(void *records, unsigned int count, void *list, unsigned int arg) __asm__("func_003c70e0");
extern void p4_sm_strip_pack(void *list, unsigned int arg) __asm__("func_003c7d90");
extern void *p4_sm_header_alloc(int size) __asm__("func_003c54a0");
extern void *p4_sm_memcpy(void *dst, const void *src, unsigned int n) __asm__("func_0043f810");
extern unsigned char p4_sm_globals __asm__("D_008872E0");
extern int p4_sm_globals_offset __asm__("D_007647E0");

void *func_003c8200(unsigned char *mesh, unsigned int arg1, unsigned int arg2)
{
    unsigned int count = *(unsigned int *)(mesh + 4);
    unsigned char *records = *(unsigned char **)(mesh + 8);
    unsigned int **sorted;
    unsigned int **runs;
    unsigned int *groups;
    unsigned int *list = 0;
    unsigned int **cursor;
    unsigned int run_count = 0;
    unsigned int group_count = 1;
    unsigned int i, g;
    unsigned char *globals = &p4_sm_globals + p4_sm_globals_offset;
    unsigned char *header;
    unsigned int *entry;
    unsigned char *indices;
    int total_indices = 0;
    int header_size = 0x10;

    sorted = (unsigned int **)p4_sm_malloc[0]((int)(count << 2), 0x10502);
    if (sorted == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        sorted[i] = (unsigned int *)(records + i * 0x14);
    }
    p4_sm_qsort(sorted, count, 4, &p4_sm_compare);

    if (count > 1) {
        int key = (int)sorted[0][2];

        for (i = 1; i < count; i++) {
            if ((int)sorted[i][2] != key) {
                group_count++;
                key = (int)sorted[i][2];
            }
        }
    }
    runs = (unsigned int **)p4_sm_malloc[0]((int)(group_count << 2), 0x1010502);
    groups = (unsigned int *)p4_sm_malloc[0]((int)(group_count * 12), 0x10502);
    groups[2] = sorted[0][2];
    groups[1] = 0;
    groups[0] = 0;
    group_count = 1;
    if (count > 1) {
        unsigned int *slot = groups + 3;

        for (i = 0; i < count - 1; i++) {
            if (sorted[i][2] != sorted[i + 1][2]) {
                slot[2] = sorted[i + 1][2];
                group_count++;
                slot[1] = i + 1;
                slot[0] = 0;
                slot[-2] = (i + 1) - slot[-2];
                slot += 3;
            }
        }
    }
    groups[group_count * 3 - 2] = count - groups[group_count * 3 - 2];

    *(void **)(globals + 4) = p4_sm_freelist_create(0x10, (int)(count / 10 + 5), 4, 0x10502);
    cursor = sorted;
    for (g = 0; g < group_count; g++) {
        unsigned int *group = groups + g * 3;
        unsigned int *scratch = (unsigned int *)p4_sm_malloc[0]((int)(group[1] * 0x14), 0x10502);
        unsigned int *dst = scratch;
        unsigned int *node;

        for (i = 0; i < group[1]; i++) {
            unsigned int *record = *cursor++;

            dst[0] = record[0];
            dst[1] = record[1];
            dst[2] = record[2];
            dst[3] = record[3];
            dst[4] = record[4];
            dst += 5;
        }
        p4_sm_strip(scratch, group[1], &list, arg1);
        p4_sm_strip_pack(&list, arg2);
        for (node = list; node != 0; node = (unsigned int *)node[3]) {
            unsigned int *run = (unsigned int *)p4_sm_malloc[0]((int)(node[1] * 2 + 0xc), 0x10502);
            unsigned short *to;
            const unsigned short *from = (const unsigned short *)node[0];

            run[2] = group[2];
            run[1] = node[1];
            run[0] = (unsigned int)(run + 3);
            to = (unsigned short *)run[0];
            for (i = 0; i < run[1]; i++) {
                to[i] = from[i];
            }
            runs[run_count] = run;
            run_count = (run_count + 1) & 0xffff;
        }
        while (list != 0) {
            unsigned int *node_free = list;

            p4_sm_free[0]((void *)node_free[0]);
            list = (unsigned int *)node_free[3];
            node_free[0] = 0;
            p4_sm_node_free[0](*(void **)(globals + 4), node_free);
        }
        p4_sm_free[0](scratch);
    }
    p4_sm_freelist_destroy(*(void **)(globals + 4));
    *(void **)(globals + 4) = 0;

    for (i = 0; i < run_count; i++) {
        total_indices += (int)runs[i][1];
        header_size += (int)runs[i][1] * 2 + 0xc;
    }
    header = (unsigned char *)p4_sm_header_alloc(header_size);
    *(unsigned int *)header = 1;
    *(unsigned short *)(header + 4) = (unsigned short)run_count;
    entry = (unsigned int *)(header + 16);
    indices = (unsigned char *)(entry + run_count * 3);
    *(unsigned short *)(header + 6) = *(unsigned short *)globals;
    *(unsigned int *)(header + 12) = 0;
    *(unsigned int *)(header + 8) = (unsigned int)total_indices;
    *(unsigned short *)globals = (unsigned short)(*(unsigned short *)globals + 1);
    for (i = 0; i < run_count; i++) {
        entry[0] = (unsigned int)indices;
        entry[1] = runs[i][1];
        entry[2] = runs[i][2];
        p4_sm_memcpy(indices, (const void *)runs[i][0], runs[i][1] << 1);
        indices += runs[i][1] * 2;
        entry += 3;
        p4_sm_free[0](runs[i]);
        runs[i] = 0;
    }
    p4_sm_free[0](sorted);
    p4_sm_free[0](runs);
    p4_sm_free[0](groups);
    return header;
}
