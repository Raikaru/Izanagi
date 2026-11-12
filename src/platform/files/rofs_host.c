/*
 * The ROFS CVFS device on the host VFS.
 *
 * Retail registers a CVFS device named "ROFS" (func_004c6ee0 -> CVFS AddDev with the interface getter
 * func_004f8518). Its interface is the table at 0x00743b60, a CVFS_IF (decomp/src/cri/re4/cvfs.h) whose
 * slots are thin wrappers 0x004f8558..0x004f8848 (most are native C in the decomp, src/sce/rofs_forward_*.c)
 * over the ROFS CD driver core (rofs_*.c, 0x004f1a08..0x004f8f60), which reads the disc's CVM volumes through
 * the CD hardware. The host has no ROFS driver; the VFS (vfs.c) already serves the disc and override tree.
 * The core functions the forwarders call, and the slots that are still asm (seek, state, change directory,
 * option function, the getters), are defined here under their retail names and implemented over the VFS:
 * files are windows of a VFS file read sequentially in 2048-byte sectors (adxf_host.h), sizes are bytes
 * for the by-name queries, positions are sectors, and reads complete at the next scheduler frame boundary.
 * Names arrive in "VOL:NAME" form (option 100 asks for it) and resolve by base name (adxf_partition.c).
 *
 * Volume management (option ids 1, 2, 3, 5, 6) succeeds without effect: the VFS holds all volumes.
 */
#include <stdlib.h>
#include <string.h>

#include "adxf_host.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "vfs.h"

extern void *func_004c8770(const char *fname, void *dir, int start, int length);
extern void func_004c8a90(void *handle);
#define P4_ROFS_IF_TABLE 0x00743B60u /* D_00743B60: the ROFS interface table, retail data (function words) */

#define SECTOR 2048

/* 0x004f8518: the interface getter registered with the device. */
void *func_004f8518(void)
{
    return P4_G2H(P4_ROFS_IF_TABLE);
}

/* 0x004f8548: the interface getter of a ROFS volume device (retail 0x004f8740 registers each mounted volume
 * under its own name, "VOL", "BGM", "BTL", "ENV", with this getter). Its table at 0x00743BC8 is the ROFS
 * table without the server slot, so the same host functions serve it. */
#define P4_ROFS_VOL_IF_TABLE 0x00743BC8u

void *func_004f8548(void)
{
    return P4_G2H(P4_ROFS_VOL_IF_TABLE);
}

/* Slot 0x00 (core 0x004f2408): the kernel's frame alarm publishes pending host read completions. */
void func_004f2408(void)
{
}

/* Slot 0x08: size in bytes of a file, or -1. */
int func_004f2460(const char *fname)
{
    char path[512];
    int isDir = 0;
    long long size = 0;

    if (fname == NULL || p4pc_adxf_resolve_name(fname, NULL, path, sizeof path) != 0
        || vfs_stat(path, &isDir, &size) != 0 || isDir) {
        return -1;
    }
    return (int)size;
}

/* Slot 0x5c: the size through a pointer; 0 or -1. */
int func_004f2548(const char *fname, int *size)
{
    int bytes = func_004f2460(fname);

    if (bytes < 0 || size == NULL) {
        return -1;
    }
    *size = bytes;
    return 0;
}

/* Slot 0x40: 1 when the file exists. */
int func_004f2948(const char *fname)
{
    return func_004f2460(fname) >= 0;
}

/* Slot 0x10: opens a file for reading (the whole file); NULL when it is not found. */
void *func_004f2260(const char *fname, void *dir, int rw)
{
    (void)rw;
    return func_004c8770(fname, dir, 0, 0xfffff);
}

/* Slot 0x14. */
void func_004f2370(void *handle)
{
    func_004c8a90(handle);
}

/* Slot 0x18: seek in sectors (type 0 from the start, 1 from the position, 2 from the end); returns the new
 * sector, or -205 for another type. */
int func_004f85c0(void *handle, int pos, int type)
{
    P4AdxfHandle *h = handle;
    long long target;

    if (h == NULL) {
        return -1;
    }
    if (type == 0) {
        target = (long long)pos * SECTOR;
    } else if (type == 1) {
        target = h->pos + (long long)pos * SECTOR;
    } else if (type == 2) {
        target = h->size + (long long)pos * SECTOR;
    } else {
        return -205;
    }
    if (target < 0) {
        target = 0;
    }
    if (target > h->size) {
        target = h->size;
    }
    p4pc_adxf_cancel(h);
    h->pos = target;
    h->stat = 1;
    return (int)((target + SECTOR - 1) / SECTOR);
}

/* Slot 0x1c: the position in sectors. */
int func_004f1a08(void *handle)
{
    P4AdxfHandle *h = handle;

    return h != NULL ? (int)((h->pos + SECTOR - 1) / SECTOR) : -1;
}

/* Slot 0x20: reads `sectors` sectors at the position into `buffer`; returns the sectors requested or -1. */
int func_004f2398(void *handle, int sectors, void *buffer)
{
    P4AdxfHandle *h = handle;

    if (h == NULL || buffer == NULL || sectors < 0) {
        return -1;
    }
    return p4pc_adxf_read(h, sectors, buffer);
}

/* Slot 0x28: stops a transfer and cancels its completion alarm. */
void func_004f23e0(void *handle)
{
    if (handle != NULL) {
        p4pc_adxf_cancel(handle);
    }
}

/* Slot 0x2c: CVFS states are 0 idle, 1 complete, 2 in transfer, 3 error, distinct from ADXF's
 * 1 prepared, 2 reading, 3 read end, 4 error. Polling must preserve the busy state until the frame alarm fires. */
int func_004f8648(void *handle)
{
    P4AdxfHandle *h = handle;

    if (h == NULL || h->stat == 4) {
        return 3;
    }
    if (h->stat == 2) {
        p4pc_poll_devices();
    }
    return h->stat == 2 ? 2 : (h->stat == 3 ? 1 : 0);
}

/* Slot 0x38: sectors transferred so far. */
int func_004f1ce0(void *handle)
{
    return func_004f1a08(handle);
}

/* Slot 0x3c: the directory is implicit in the names; 0 for success. */
int func_004f86c8(const char *dirname)
{
    (void)dirname;
    return 0;
}

/* Slot 0x48: directory information is not kept (no directory lists); 0 entries. */
int func_004f28b0(const char *name, void *info, int count)
{
    (void)name;
    (void)info;
    (void)count;
    return 0;
}

/* Slot 0x60: option function. Option 100 asks whether the device wants the "DEV:path" form (it does);
 * the volume options (1 set default, 2 add, 3 delete, 5 query, 6 delete) succeed. */
int func_004f8848(void *handle, int option, int a, int b)
{
    (void)handle;
    (void)a;
    (void)b;
    return option == 100 ? 1 : 0;
}
