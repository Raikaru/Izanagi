/* _rpPTankAtomicCreateCustom (retail 0x003a2390): builds a PTank atomic.
 *
 * RpAtomicCreate (func_003c00e0), then a 0x140-byte tank record from the RW allocator (slot 0x008873E8,
 * flags 0x3012F). The record: +0 maxPCount, +4 actPCount, +0x0C raw data block, +0x10 the atomic's
 * previous render callback, +0x14 the four callbacks copied from `callbacks` (alloc, create, instance,
 * render), +0x40 instFlags, +0x44 platFlags, +0x50 the public data (see src/render/p4gl_ptank.c).
 * The atomic's render callback becomes func_003a2290 and its plugin slot (atomic + iGpffffb610) the
 * record. With an alloc callback it returns the raw block (record +0x0C); without one the array
 * (func_003a1730) or structure (func_003a1c40) layout builders run. The create callback then finishes
 * the platform setup; on success the constants are set: size 1,1, centre 0, rotation 0, identity
 * matrix flags, colour and the four corner colours (0,0,0,255 from 0x00763B68), the unit-square
 * texture coordinates (0x00707E50), the material texture cleared, and the render-state defaults
 * (src blend, dest blend, vertex alpha) read through fpRenderStateGet (0x00887304, ids 10, 11, 12).
 * Any failure destroys the atomic (func_003c02e0) and returns 0. */
extern void *p4c_003c00e0_003a2390(void) __asm__("func_003c00e0");
extern s32 p4c_003c02e0_003a2390(void *atomic) __asm__("func_003c02e0");
extern void p4c_003c42b0_003a2390(void *material, void *texture) __asm__("func_003c42b0");
extern void *p4c_003a2290_003a2390(void *atomic) __asm__("func_003a2290");
extern s32 p4c_003a1730_003a2390(u8 *rec, u32 dataFlags, u32 platFlags) __asm__("func_003a1730");
extern s32 p4c_003a1c40_003a2390(u8 *rec, u32 dataFlags, u32 platFlags) __asm__("func_003a1c40");

typedef void *(*P4PtankAlloc)(u32 size, u32 flags);
typedef s32 (*P4PtankAllocCb)(u8 *pd, u32 maxP, u32 dataFlags, u32 platFlags);
typedef s32 (*P4PtankCreateCb)(void *atomic, u8 *pd, u32 maxP, u32 dataFlags, u32 platFlags);
typedef s32 (*P4RenderStateGet)(s32 state, void *out);

void *func_003a2390(u32 maxP, u32 dataFlags, u32 platFlags, void *callbacks)
{
    u8 *atomic = (u8 *)p4c_003c00e0_003a2390();
    u8 *rec;
    s32 laidOut = 0;

    if (atomic != (u8 *)0) {
        rec = (u8 *)(*(P4PtankAlloc *)0x008873E8)(0x140, 0x3012F);
        if (rec != (u8 *)0) {
            u32 *w = (u32 *)rec;
            u8 *pd = rec + 0x50;
            u32 *uv = (u32 *)0x00707E50;
            u8 *colour = (u8 *)0x00763B68;
            f32 *unit = (f32 *)0x00763B60;
            s32 i;

            w[4] = *(u32 *)(atomic + 0x48);
            *(void **)(atomic + 0x48) = (void *)p4c_003a2290_003a2390;
            w[0] = maxP;
            w[1] = 0;
            w[0x2A] = dataFlags;
            w[0x11] = platFlags;
            w[0x10] = 0xFFFFFFFFu;
            w[0xF] = 0;
            w[3] = 0;
            *(u8 **)(atomic + iGpffffb610) = rec;
            __builtin_memcpy(rec + 0x14, callbacks, 0x10);
            if (w[5] != 0) {
                w[3] = (u32)(*(P4PtankAllocCb *)(rec + 0x14))(pd, maxP, dataFlags, platFlags);
            } else if ((dataFlags & 0x20000000) == 0x20000000) {
                laidOut = p4c_003a1c40_003a2390(rec, dataFlags, platFlags);
            } else {
                laidOut = p4c_003a1730_003a2390(rec, dataFlags, platFlags);
            }
            if (laidOut != 1 && w[3] == 0) {
                p4c_003c02e0_003a2390(atomic);
                return (void *)0;
            }
            if ((*(P4PtankCreateCb *)(rec + 0x18))(atomic, pd, w[0], dataFlags, platFlags) == 1) {
                p4c_003c42b0_003a2390(**(void ***)(*(u8 **)(atomic + 0x18) + 0x20), (void *)0);
                w[0x30] = *(u32 *)(unit + 0);
                w[0x31] = *(u32 *)(unit + 1);
                w[0x32] = 0;
                w[0x2E] = 0;
                w[0x2F] = 0;
                w[0x4A] = 0x3F800000;
                w[0x45] = 0x3F800000;
                w[0x40] = 0x3F800000;
                w[0x44] = 0;
                w[0x42] = 0;
                w[0x41] = 0;
                w[0x49] = 0;
                w[0x48] = 0;
                w[0x46] = 0;
                w[0x4E] = 0;
                w[0x4D] = 0;
                w[0x4C] = 0;
                w[0x43] |= 0x20003;
                for (i = 0; i < 5; i++) {
                    rec[0xCC + 4 * i] = colour[0];
                    rec[0xCD + 4 * i] = colour[1];
                    rec[0xCE + 4 * i] = colour[2];
                    rec[0xCF + 4 * i] = colour[3];
                }
                for (i = 0; i < 8; i++) {
                    w[0x38 + i] = uv[i];
                }
                (*(P4RenderStateGet *)0x00887304)(10, rec + 0xAC);
                (*(P4RenderStateGet *)0x00887304)(11, rec + 0xB0);
                (*(P4RenderStateGet *)0x00887304)(12, rec + 0xB4);
                return atomic;
            }
        }
    }
    p4c_003c02e0_003a2390(atomic);
    return (void *)0;
}
