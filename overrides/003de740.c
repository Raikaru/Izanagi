/* Retail 003de740 (code1_003d.c): seek on a buffered stream object. The 16-byte position record at
 * stream+0x10 (64-bit position at +0x10, two words after it) is returned through `out`.
 *   stream+0x00  64-bit stream size
 *   stream+0x10  64-bit current position
 *   stream+0x30  nonzero while the device handle is open; +0x38 state (3 = repositioning, 2 = closed)
 *   stream+0x60  device handle; +0x64 bytes consumed in the buffer; +0x6c bytes buffered
 * `whence` 1 = absolute, 2 = relative to the position, 3 = back from the end; anything else returns
 * the 64-bit value -1 (the upper half of retail's record is whatever was on the stack, 0 here). The
 * target is clamped to [0, size]. A target inside the buffered window only moves the buffer cursor
 * (func_003dd6d0); anything else repositions the device handle (func_004270f8). */
extern void p4_stream_refill(unsigned char *stream) __asm__("func_003dd6d0");
extern int p4_device_seek(int handle, int offset, int whence) __asm__("func_004270f8");

void func_003de740(unsigned char *out, unsigned char *stream, int offset, int whence)
{
    long long size = *(long long *)(stream + 0x00);
    long long position = *(long long *)(stream + 0x10);
    long long target;
    int used, buffered;
    unsigned int i;

    if (whence == 3) {
        target = size - (long long)offset;
    } else if (whence == 2) {
        target = position + (long long)offset;
    } else if (whence == 1) {
        target = (long long)offset;
    } else {
        long long failure = -1;

        for (i = 0; i < 8; i++) {
            out[i] = ((unsigned char *)&failure)[i];
        }
        for (i = 8; i < 16; i++) {
            out[i] = 0;
        }
        return;
    }
    if (size < target) {
        target = size;
    } else if (target < 0) {
        target = 0;
    }

    used = *(int *)(stream + 0x64);
    buffered = *(int *)(stream + 0x6c);
    if (used < buffered) {
        long long low = position - (long long)used;
        long long high = position + (long long)(buffered - used);

        if (target >= low && target <= high) {
            *(int *)(stream + 0x64) = (int)(target - low);
            *(long long *)(stream + 0x10) = target;
            p4_stream_refill(stream);
            for (i = 0; i < 16; i++) {
                out[i] = stream[0x10 + i];
            }
            return;
        }
    }
    *(int *)(stream + 0x38) = 3;
    *(int *)(stream + 0x64) = buffered;
    *(long long *)(stream + 0x10) = target;
    p4_device_seek(*(int *)(stream + 0x60), (int)target, 0);
    if (*(int *)(stream + 0x30) == 0) {
        *(int *)(stream + 0x38) = 2;
    }
    for (i = 0; i < 16; i++) {
        out[i] = stream[0x10 + i];
    }
}
