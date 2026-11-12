/* func_00415070 (RxHeapCreate): allocates a 28-byte heap header and a data
 * block of at least 1024 bytes, rounded up to 32, plus 139 bytes of slack.
 * The data pointer is aligned up to 128 inside the block and stored in
 * data[0]. The heap header is then initialised and passed to func_00414ef0.
 * On success returns the header. On failure frees whatever was allocated
 * and returns NULL.
 *
 * Direct transliteration of the retail code. The "size < 128" clamp follows the rounding (unsigned
 * sltiu), so it is dead after the 1024 floor except when the rounding wraps (a request above 0xFFFFFFE0
 * rounds to 0 and ends up as 128 bytes). */

typedef void *(*P4HeapAllocFn)(u32 size, u32 flags);
typedef void (*P4HeapFreeFn)(void *p);

u32 *func_00415070(u32 arg0)
{
    extern s32 func_00414ef0(u32 *heap);
    P4HeapAllocFn alloc = *(P4HeapAllocFn *)0x008873E8;
    P4HeapFreeFn free_fn = *(P4HeapFreeFn *)0x008873EC;
    u32 *work;
    u32 *data;
    u32 size;
    s32 result;

    size = arg0;
    if (size < 1024)
        size = 1024;
    work = (u32 *)alloc(28, 0x00040409);
    if (work == NULL)
        return NULL;
    size = (size + 31) & ~31u;
    if (size < 128)
        size = 128;
    data = (u32 *)alloc(size + 139, 0x01040409);
    if (data != NULL) {
        data[0] = ((u32)data + 12 + 127) & ~127u;
        data[1] = size;
        data[2] = 0;
        work[0] = size;
        work[1] = (u32)data;
        work[3] = 0;
        work[4] = 0;
        work[5] = 0;
        work[6] = 1;
        result = func_00414ef0(work);
        if (result != 0)
            return work;
        free_fn(data);
    }
    free_fn(work);
    return NULL;
}
