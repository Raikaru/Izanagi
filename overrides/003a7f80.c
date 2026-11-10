/* func_003a7f80 (module 0x12F: builds the pipeline of its first record, called by func_003a8500):
 * creates a pipeline with one PS2AllMat node (func_003d3a80) and configures it with three
 * cluster definitions copied into the module's scratch block at 0x00885A60..0x00885A8F from
 * the retail cluster-definition table at 0x0070B520: a copy of 0x0070B550 with its attribute word
 * (copy + 8) = (attr & ~0x2000) | 0xD at 0x00885A70 (type 4), a copy of 0x0070B520 with attribute
 * 0x6C00000D at 0x00885A80 (type 6), and a copy of 0x0070B580 with attribute 0x6400000D at
 * 0x00885A60 (type 7); then the layout (func_003d3780: 3, 80), the flag 240, the buffer
 * {0x00708480, 4} and callback slots 2..4 (func_003d0850, func_003a7970, func_003a7a30).
 * Returns the unlocked pipeline, or 0 when the pipeline or the node definition is missing.
 *
 * Direct transliteration of the retail code. As in retail, the lock result is used unchecked. */

extern u32 *p4_pipe_create(void) __asm__("func_004115d0");
extern u32 *p4_pipe_lock(void *pipe) __asm__("func_00412ca0");
extern void *p4_pipe_add_fragment(void *lpipe, u32 *first, u32 *nodedef, u32 term, ...) __asm__("func_00412fb0");
extern void *p4_pipe_find_node(void *pipe, u32 name, void *start, s32 *index) __asm__("func_00412e90");
extern void *p4_pipe_unlock(void *lpipe) __asm__("func_004125e0");
extern u32 *p4_psmat_def(void) __asm__("func_003d3a80");
extern void *p4_psmat_cluster(void *node, void *cluster, u32 type) __asm__("func_003d3130");
extern void *p4_psmat_layout(void *node, s32 a, s32 b) __asm__("func_003d3780");
extern void *p4_psmat_flag(void *node, s32 flag) __asm__("func_003d38e0");
extern void *p4_psmat_buffer(void *node, void *buffer, s32 size) __asm__("func_003d3920");
extern void *p4_psmat_callback(void *node, s32 slot, void *fn) __asm__("func_003d39a0");
extern void *p4_memcpy(void *dst, const void *src, u32 n) __asm__("func_0043f810");
extern u8 p4cb_003d0850[] __asm__("func_003d0850");
extern u8 p4cb_003a7970[] __asm__("func_003a7970");
extern u8 p4cb_003a7a30[] __asm__("func_003a7a30");

u32 *func_003a7f80(void)
{
    u32 *pipe = p4_pipe_create();
    u32 *lpipe;
    u32 *def;
    void *node;
    void *unlocked;

    if (pipe == NULL)
        return NULL;
    lpipe = p4_pipe_lock(pipe);
    def = p4_psmat_def();
    if (def == NULL)
        return NULL;

    p4_pipe_add_fragment(lpipe, NULL, def, 0);
    node = p4_pipe_find_node(lpipe, def[0], NULL, NULL);

    p4_memcpy((void *)0x00885A80, (void *)0x0070B520, 16);
    *(u32 *)0x00885A88 = 0x6C00000D;
    p4_memcpy((void *)0x00885A70, (void *)0x0070B550, 16);
    *(u32 *)0x00885A78 = (*(u32 *)0x00885A78 & ~0x2000u) | 0xD;
    p4_memcpy((void *)0x00885A60, (void *)0x0070B580, 16);
    *(u32 *)0x00885A68 = 0x6400000D;
    p4_psmat_cluster(node, (void *)0x00885A70, 4);
    p4_psmat_cluster(node, (void *)0x00885A80, 6);
    p4_psmat_cluster(node, (void *)0x00885A60, 7);
    p4_psmat_layout(node, 3, 80);

    unlocked = p4_pipe_unlock(lpipe);
    node = p4_pipe_find_node(unlocked, def[0], NULL, NULL);
    p4_psmat_flag(node, 240);
    p4_psmat_buffer(node, (void *)0x00708480, 4);
    p4_psmat_callback(node, 2, p4cb_003d0850);
    p4_psmat_callback(node, 3, p4cb_003a7970);
    p4_psmat_callback(node, 4, p4cb_003a7a30);
    return (u32 *)unlocked;
}
