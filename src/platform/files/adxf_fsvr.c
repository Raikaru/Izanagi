#include "adxf_host.h"
/*
 * ADXF file-server device wrappers (retail libadxe code1_004c, 0x004c6eb0..0x004c71b8). kwlnInitSubsystems
 * calls func_004c6eb0 to register the "ROFS" device with the CRI virtual file system (CVFS), then
 * func_004c6f50 four times, once per disc volume (VOL, BGM, BTL, ENV), in a loop that repeats until it
 * returns 0, and func_004c7188 once to detach the first.
 *
 * Retail opens the volume's CVM image ("CDV:/data.cvm") through the CDV CVFS device (a CD read), then adds
 * it to the ROFS device as a volume (CVFS option calls into the ROFS driver). The host has no CD or ROFS
 * driver: the VFS (vfs.c) already indexes the disc's five CVM containers, so attaching a volume reduces to
 * checking that its image is on the disc (func_004c6fa8) and detaching to nothing (func_004c71b8). The
 * device registration is kept: it goes through the CVFS add-device function (func_004e17a0, the CRI lane's)
 * with the host ROFS interface of rofs_host.c.
 *
 * Each retail public function is the same shape: take the SVM lock (func_004c7228, a no-op in retail),
 * call the unlocked body, release (func_004c7230, a no-op).
 */
#include <stdio.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "vfs.h"

extern void func_004c7228(void);
extern void func_004c7230(void);
extern int func_004e17a0(const char *name, void *interface, int arg);
extern int func_004e1a00(const char *name);
#define p4_rofs_name ((char *)P4_G2H(0x00757EF8u)) /* D_00757EF8: "ROFS", retail rodata */

#define ADXF_ERR_PRM (-3)

/* 0x004c6ee0: registers the "ROFS" device with its interface. */
int func_004c6ee0(void)
{
    return func_004e17a0(p4_rofs_name, (void *)func_004f8518, 0);
}

/* 0x004c6f30: removes the "ROFS" device. */
int func_004c6f30(void)
{
    return func_004e1a00(p4_rofs_name);
}

/* 0x004c6eb0: registers the device under the lock. */
int func_004c6eb0(void)
{
    int result;

    func_004c7228();
    result = func_004c6ee0();
    func_004c7230();
    return result;
}

/* 0x004c6f08: removes the device under the lock. */
int func_004c6f08(void)
{
    int result;

    func_004c7228();
    result = func_004c6f30();
    func_004c7230();
    return result;
}

/* 0x004c6fa8: attaches volume `name` (VOL, BGM, BTL, ENV) from the CVM image path `image` ("CDV:/data.cvm") to the
 * ROFS device. -3 for a null argument or the ROFS device itself (retail's checks); the host returns 0 when
 * the image is on the disc and -1 when it is not (kwlnInitSubsystems then asks again). */
int func_004c6fa8(const char *name, const char *image, void *arg)
{
    const char *base;
    long long size;
    int isDir;
    static char reported[16];

    (void)arg;
    if (name == NULL || image == NULL) {
        return ADXF_ERR_PRM;
    }
    if (strcmp(name, p4_rofs_name) == 0) {
        return ADXF_ERR_PRM;
    }
    base = strrchr(image, '/');
    base = base != NULL ? base + 1 : (strchr(image, ':') != NULL ? strchr(image, ':') + 1 : image);
    if (vfs_stat(base, &isDir, &size) != 0 || isDir) {
        if (reported[(unsigned char)name[0] & 15] == 0) {
            reported[(unsigned char)name[0] & 15] = 1;
            fprintf(stderr, "p4pc: ADXF volume %s: image %s is not on the disc\n", name, image);
        }
        return -1;
    }
    /* The mount (retail 0x004f8740): the volume becomes a CVFS device of its own name, so "VOL:NAME" is
     * served by the ROFS interface; a name over 8 characters is refused. */
    if (strlen(name) > 8) {
        return -1;
    }
    func_004e17a0(name, (void *)func_004f8548, 0);
    return 0;
}

/* 0x004c6f50: func_004c6fa8 under the lock. */
int func_004c6f50(const char *name, const char *dir, void *arg)
{
    int result;

    func_004c7228();
    result = func_004c6fa8(name, dir, arg);
    func_004c7230();
    return result;
}

/* 0x004c71b8: detaches a volume: the VFS keeps all volumes, so nothing is released. */
void func_004c71b8(const char *name)
{
    (void)name;
}

/* 0x004c7188: func_004c71b8 under the lock. */
int func_004c7188(const char *name)
{
    func_004c7228();
    func_004c71b8(name);
    func_004c7230();
    return 0;
}
