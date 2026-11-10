/* func_0039bbd0 (PDS registry: build the pipe of a type-1 entry from its spec). The spec
 * is a 33-word block of PS2 pipeline configuration (the registry specs at D_0064F3C0 and
 * following):
 *   word 0, 1        RxPipeline pluginId and pluginData
 *   words 2..21      up to ten {RxClusterDefinition *, cluster type} pairs, ended by a NULL
 *                    cluster definition; each is passed to func_003d3130
 *   word 22          kind 1..6; selects the PS2AllMat generator that takes words 23..25:
 *                    kind 1, 2 -> func_003d35f0 (word 23 vertices, 24 stride, 25 batch),
 *                    kind 3..5 -> func_003d3460 (same arguments),
 *                    kind 6 -> func_003d3780 (words 23, 24); anything else reports error 0
 *   word 26          func_003d38e0 argument
 *   word 27          pointer to {pointer, size} for func_003d3920
 *   words 28..32     the five node callbacks (func_003d39a0 slots 1..5)
 *
 * The pipe is a real RxPipeline: created, locked, given one PS2AllMat node (func_003d3a80),
 * configured, then unlocked. The node body is a retail no-op (func_003d3120) and the
 * configuration stays data in the node's private/init data; the GL renderer never runs the
 * VU1 packets these specs describe (see func_003d30b0). Returns the pipe.
 *
 * Direct transliteration of the retail code. */

extern s32 *func_003d3a80(void);
extern u8 *func_003d3130(u8 *node, u32 *cluster, u32 type);
extern u8 *func_003d35f0(u8 *node, s32 a, s32 b, s32 c);
extern u8 *func_003d3460(u8 *node, s32 a, s32 b, s32 c);
extern u8 *func_003d3780(u8 *node, s32 a, s32 b);
extern u8 *func_003d38e0(u8 *node, s32 arg);
extern u8 *func_003d3920(u8 *node, u8 *ptr, s32 size);
extern u8 *func_003d39a0(u8 *node, s32 slot, s32 fn);

void *func_0039bbd0(u32 spec_arg)
{
    u32 *spec = (u32 *)spec_arg;
    u32 *pipe;
    s32 *locked;
    s32 *def;
    u8 *frag;
    u8 *node;
    u8 *chain;
    u32 i;

    pipe = (u32 *)func_004115d0();
    pipe[11] = spec[0];
    pipe[12] = spec[1];
    locked = func_00412ca0((s32 *)pipe);
    def = func_003d3a80();
    frag = func_00412fb0((u8 *)locked, NULL, def, 0);
    node = (u8 *)func_00412e90(frag, def[0], 0, NULL);

    for (i = 0; i < 10; i++) {
        if (spec[2 + i * 2] == 0)
            break;
        func_003d3130(node, (u32 *)spec[2 + i * 2], spec[3 + i * 2]);
    }

    switch (spec[22]) {
    case 1:
    case 2:
        func_003d35f0(node, spec[23], spec[24], spec[25]);
        break;
    case 3:
    case 4:
    case 5:
        func_003d3460(node, spec[23], spec[24], spec[25]);
        break;
    case 6:
        func_003d3780(node, spec[23], spec[24]);
        break;
    default: {
        s32 err[2];

        err[0] = 305;
        err[1] = func_003df590(0);
        func_003df4d0(err);
        break;
    }
    }

    node = (u8 *)func_00412e90((u8 *)func_004125e0((s32)frag), def[0], 0, NULL);
    node = func_003d38e0(node, spec[26]);
    func_003d3920(node, *(u8 **)spec[27], ((s32 *)spec[27])[1]);
    chain = func_003d39a0(node, 1, spec[28]);
    chain = func_003d39a0(chain, 2, spec[29]);
    chain = func_003d39a0(chain, 3, spec[30]);
    chain = func_003d39a0(chain, 4, spec[31]);
    func_003d39a0(chain, 5, spec[32]);
    return pipe;
}
