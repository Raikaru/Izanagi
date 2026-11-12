/* func_003de2d0 (sdkCdvd device method at vtable offset 0x18): releases the device's channel records,
 * the +0x58 array allocated by func_003dea20, through the RenderWare free slot at 0x008873EC (a tail
 * call in retail, so the slot function's result is returned).
 *
 * Direct transliteration of the retail code. */

typedef void (*P4CdFreeFn)(void *p);

void func_003de2d0(unsigned char *dev)
{
    (*(P4CdFreeFn *)0x008873EC)(*(void **)(dev + 0x58));
}
