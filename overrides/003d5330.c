/* func_003d5330 (136 bytes): opens arg0 as a stream, finds its 0x1B chunk and reads the animation object
 * in it. Returns the object or 0.
 *
 * func_003e2f60(2, 1, arg0) opens the stream (0 on failure); func_003df050(stream, 0x1B, 0, 0) finds the
 * chunk (0 on failure; the stream is not closed then, as in retail). func_003d53c0 reads the object,
 * func_003e2e40(stream, 0) closes the stream and the object is returned. */

extern u8 *p4c_003d53c0_003d5330(s32 stream) __asm__("func_003d53c0");
extern s32 p4c_003df050_003d5330(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003e2e40_003d5330(s32 stream, s32 flags) __asm__("func_003e2e40");
extern s32 p4c_003e2f60_003d5330(s32 a, s32 b, s32 c) __asm__("func_003e2f60");

s32 func_003d5330(s32 arg0)
{
    s32 stream;
    s32 result;

    stream = p4c_003e2f60_003d5330(2, 1, arg0);
    if (stream == 0) {
        return 0;
    }
    if (p4c_003df050_003d5330(stream, 0x1B, 0, 0) == 0) {
        return 0;
    }
    result = p4c_003d53c0_003d5330(stream);
    p4c_003e2e40_003d5330(stream, 0);
    return result;
}
