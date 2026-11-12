/* What the RwMatrix overrides (003df9f0, 003e0680, 003e0e20) call, declared the way the decomp unit they are spliced into declares it: tests/matrix_override_test.c
 * defines these callees. Forced into the rewritten overrides of the 64-bit twin (unit64.py build --src); the i386 test declares them in its own text. */
u8 *RwMatrixMultiply(u8 *out, u8 *a, u8 *b);
s32 func_003df590(s32 code, ...);
void func_003df4d0(void *err);
