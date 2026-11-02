/* func_003e9d50 (retail 0x003E9D50) is RwFrameRotate(frame, axis, angle, combineOp): it calls 0x003E0870
 * (RwMatrixRotate) on the frame's modelling matrix (frame + 0x10) with its own arguments untouched, then
 * marks the hierarchy dirty. The decomp attributes this address to RwFrameTranslate (baframe.c, with
 * `#define RwMatrixTranslate func_003e0870`), a function with no angle: built for x86 it dropped the
 * float angle that MIPS passes through $f12 and rotated by whatever was left, so every one of the
 * game's callers (36) rotated by garbage. RwFrameTranslate is 0x003E9C10 (src/rw/baframe.c,
 * func_003e9c10: RwMatrixTranslate 0x003E0C90). */
extern void *p4c_003e0870_003e9d50(void *matrix, const void *axis, float angle, int combineOp) __asm__("func_003e0870");

void *func_003e9d50(void *frame, const void *axis, float angle, int combineOp)
{
    p4c_003e0870_003e9d50(&((RwFrame *)frame)->modelling, axis, angle, combineOp);
    RwFrameUpdateObjects((RwFrame *)frame);
    return frame;
}
