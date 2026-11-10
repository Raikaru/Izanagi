/* func_003a8500 (module 0x12F record set-up, called by func_003a2ce0): stores the result of
 * func_003a7f80 in record word 3 and, when it is non-zero, builds a one-node PS2All pipeline
 * for it: RxPipelineCreate, lock, add the PS2All node (func_003cfa70), unlock, then on that
 * node set callback slot 1 to func_003a8180 and the node's data to the func_003a7f80 result.
 * The pipeline goes into record word 4. Returns 1 when the pipeline exists, else 0 (a failed
 * step destroys the half-built pipeline). A zero func_003a7f80 result returns 0 and leaves
 * word 4 untouched.
 *
 * Direct transliteration of the retail code. The pipeline helpers are the RenderWare
 * pipeline functions at their retail addresses. */

extern u32 *p4_pipe_create(void) __asm__("func_004115d0");
extern u32 *p4_pipe_lock(void *pipe) __asm__("func_00412ca0");
extern u32 *p4_pipe_nodedef(void) __asm__("func_003cfa70");
extern void *p4_pipe_add_fragment(void *lpipe, u32 *first, u32 *nodedef, u32 term, ...) __asm__("func_00412fb0");
extern void *p4_pipe_find_node(void *pipe, u32 name, void *start, s32 *index) __asm__("func_00412e90");
extern void *p4_pipe_unlock(void *lpipe) __asm__("func_004125e0");
extern void p4_pipe_destroy(void *pipe) __asm__("func_00411670");
extern void *p4_ps2all_set_callback(void *node, s32 slot, void *fn) __asm__("func_003cf9b0");
extern void *p4_ps2all_set_data(void *node, s32 data) __asm__("func_003cfa30");
extern s32 p4_ps2all_data_003a7f80(void) __asm__("func_003a7f80");
extern u8 p4cb_003a8180[] __asm__("func_003a8180");

s32 func_003a8500(u8 *record_arg)
{
    u32 *record = (u32 *)record_arg;
    s32 data;
    u32 *pipe;
    u32 *lpipe;
    u32 *def;
    void *frag;
    void *unlocked;

    record[3] = (u32)p4_ps2all_data_003a7f80();
    data = (s32)record[3];
    if (data == 0)
        return 0;

    pipe = p4_pipe_create();
    if (pipe != NULL) {
        lpipe = p4_pipe_lock(pipe);
        if (lpipe != NULL) {
            def = p4_pipe_nodedef();
            frag = p4_pipe_add_fragment(lpipe, NULL, def, 0);
            p4_pipe_find_node(frag, def[0], NULL, NULL);
            unlocked = p4_pipe_unlock(frag);
            if (unlocked != NULL) {
                void *node = p4_pipe_find_node(unlocked, def[0], NULL, NULL);

                p4_ps2all_set_callback(node, 1, p4cb_003a8180);
                p4_ps2all_set_data(node, data);
                record[4] = (u32)pipe;
                return record[4] != 0;
            }
        }
        p4_pipe_destroy(pipe);
    }
    record[4] = 0;
    return 0;
}
