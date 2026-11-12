/* func_003dea20 (sdkCdvd device create): builds a CD device object for H_Cdvd_InitDevices.
 *   arg0 = number of channel records, arg1 = base of the shared buffer area, arg2 = bytes
 *   per record's slice of it, arg3 / arg4 = the two device-name/descriptor words the caller
 *   prepared (iGpffffba40, iGpffffba38).
 * A first check, func_003dd900(arg4), must return 0 or the call returns NULL. The object is
 * 0x5C bytes from the RenderWare malloc (slot 0x008873E8, hint 0x000401BE). Its method table at
 * +0x14.. holds the sdkCdvd callbacks (offsets 0x14, 0x18, 0x1C, 0x28..0x4C); +0x58 points at
 * `arg0` zeroed 0x70-byte channel records from the RenderWare calloc (slot 0x008873F4), each
 * with its owner (+0x50), its slice of the buffer area (+0x68 = arg1 + i * arg2) and the slice
 * size (+0x6C = arg2). func_003dd290(obj, arg0, arg4, arg3) finishes the set-up; any result other
 * than 1 frees the records and the object and returns NULL.
 *
 * Direct transliteration of the retail code. The callbacks are passed by symbol: they run on the
 * host. */

extern u8 p4cd_003dd900[] __asm__("func_003dd900");
extern u8 p4cd_003dd290[] __asm__("func_003dd290");
extern u8 p4cd_003de280[] __asm__("func_003de280");
extern u8 p4cd_003de2d0[] __asm__("func_003de2d0");
extern u8 p4cd_003de2c0[] __asm__("func_003de2c0");
extern u8 p4cd_003de2f0[] __asm__("func_003de2f0");
extern u8 p4cd_003de4c0[] __asm__("func_003de4c0");
extern u8 p4cd_003de4d0[] __asm__("func_003de4d0");
extern u8 p4cd_003de6a0[] __asm__("func_003de6a0");
extern u8 p4cd_003de740[] __asm__("func_003de740");
extern u8 p4cd_003de8c0[] __asm__("func_003de8c0");
extern u8 p4cd_003de9b0[] __asm__("func_003de9b0");
extern u8 p4cd_003dd590[] __asm__("func_003dd590");
extern u8 p4cd_003de9c0[] __asm__("func_003de9c0");
extern u8 p4cd_003dea10[] __asm__("func_003dea10");
extern int p4cd_check(unsigned a) __asm__("func_003dd900");
extern int p4cd_finish(void *dev, int count, unsigned a4, unsigned a3) __asm__("func_003dd290");

typedef void *(*P4CdMallocFn)(unsigned size, unsigned flags);
typedef void *(*P4CdCallocFn)(unsigned count, unsigned size, unsigned flags);
typedef void (*P4CdFreeFn)(void *p);

void *func_003dea20(int arg0, unsigned char *arg1, int arg2, unsigned arg3, unsigned arg4)
{
    unsigned *dev;
    unsigned char *rec;
    int i;

    if (p4cd_check(arg4) != 0)
        return 0;

    dev = (unsigned *)(*(P4CdMallocFn *)0x008873E8)(0x5C, 0x401BE);
    dev[5] = (unsigned)p4cd_003de280;
    dev[6] = (unsigned)p4cd_003de2d0;
    dev[7] = (unsigned)p4cd_003de2c0;
    dev[10] = (unsigned)p4cd_003de2f0;
    dev[11] = (unsigned)p4cd_003de4c0;
    dev[12] = (unsigned)p4cd_003de4d0;
    dev[13] = (unsigned)p4cd_003de6a0;
    dev[14] = (unsigned)p4cd_003de740;
    dev[15] = (unsigned)p4cd_003de8c0;
    dev[16] = (unsigned)p4cd_003de9b0;
    dev[17] = (unsigned)p4cd_003dd590;
    dev[18] = (unsigned)p4cd_003de9c0;
    dev[19] = (unsigned)p4cd_003dea10;
    dev[22] = (unsigned)(*(P4CdCallocFn *)0x008873F4)(arg0, 0x70, 0x401BE);
    rec = (unsigned char *)dev[22];
    for (i = 0; i < arg0; i++) {
        *(unsigned *)(rec + i * 0x70 + 0x50) = (unsigned)dev;
        *(unsigned *)(rec + i * 0x70 + 0x68) = (unsigned)arg1 + (unsigned)(i * arg2);
        *(unsigned *)(rec + i * 0x70 + 0x6C) = (unsigned)arg2;
    }
    if (p4cd_finish(dev, arg0, arg4, arg3) != 1) {
        (*(P4CdFreeFn *)0x008873EC)((void *)dev[22]);
        (*(P4CdFreeFn *)0x008873EC)(dev);
        return 0;
    }
    return dev;
}
