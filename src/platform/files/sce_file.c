/*
 * PS2 file API (libkernl filestub) over the virtual file tree in vfs.c. Names and open
 * flags follow the SDK. The tree is read-only, so writes fail; memory-card saves go
 * through the h_memcard wrappers, not here.
 */
#include <stdint.h>
#include <string.h>

#include "vfs.h"
#include "p4pc/profile_trace.h"

#define SCE_WRONLY 0x0002
#define SCE_CREAT 0x0200
#define SCE_NAME_BYTES 256

int sceOpen(const char *name, int flags)
{
    /* RDWR includes WRONLY's bit; including RDWR itself would also reject RDONLY (1). */
    int writable = (flags & (SCE_WRONLY | SCE_CREAT)) != 0;
    return vfs_open(name, writable);
}

int sceClose(int fd)
{
    return vfs_close(fd);
}

int sceRead(int fd, void *buf, int n)
{
    uint64_t start = p4pc_prof_span_begin();
    int result = vfs_read(fd, buf, n);
    if (start) {
        uint64_t end = p4pc_prof_span_stop(start);
        P4VfsOrigin origin;
        if (!vfs_origin(fd, &origin))
            p4pc_prof_span_record(start, end, "sce.read.sync", n, result, fd, origin.key);
    }
    return result;
}

int sceWrite(int fd, const void *buf, int n)
{
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

int sceLseek(int fd, int offset, int whence)
{
    return (int)vfs_seek(fd, offset, whence);
}

/* Nothing is buffered in the host layer, so there is nothing to flush. */
int sceSync(const char *name, int mode)
{
    (void)name;
    (void)mode;
    return 0;
}

/* Directory and stat natives. The layouts are the SDK's: sceIoStat is 64 bytes with the
 * mode word first and the low size word at +8 (hisize at +0x24); sceIoDirent is that stat,
 * then a 256-byte NUL-terminated name at +0x40, then an unused word and a private pointer
 * (0x148 bytes). Mode type bits are the SDK's FIO_S_IFDIR 0x1000 and FIO_S_IFREG 0x2000.
 * The stat timestamps are left zero: the host layer has no SDK dates. */
#define SCE_IFDIR 0x1000u
#define SCE_IFREG 0x2000u
#define SCE_STAT_BYTES 0x40
#define SCE_DIRENT_BYTES 0x148

static void put_u32(unsigned char *at, unsigned int v)
{
    at[0] = (unsigned char)v;
    at[1] = (unsigned char)(v >> 8);
    at[2] = (unsigned char)(v >> 16);
    at[3] = (unsigned char)(v >> 24);
}

static void fill_stat(unsigned char *out, int is_dir, long long size)
{
    memset(out, 0, SCE_STAT_BYTES);
    put_u32(out, is_dir ? SCE_IFDIR : SCE_IFREG);
    put_u32(out + 8, (unsigned int)size);
    put_u32(out + 0x24, (unsigned int)((unsigned long long)size >> 32));
}

int sceDopen(const char *dirname)
{
    return vfs_dir_open(dirname);
}

int sceDread(int fd, void *dirent)
{
    unsigned char *out = dirent;
    char name[SCE_NAME_BYTES];
    int is_dir;
    long long size;
    int r = vfs_dir_next(fd, name, sizeof name, &is_dir, &size);

    if (r != 1) {
        return r;
    }
    memset(out, 0, SCE_DIRENT_BYTES);
    fill_stat(out, is_dir, size);
    memcpy(out + SCE_STAT_BYTES, name, strlen(name) + 1);
    return 1;
}

int sceDclose(int fd)
{
    return vfs_dir_close(fd);
}

int sceGetstat(const char *name, void *stat)
{
    int is_dir;
    long long size;

    if (vfs_stat(name, &is_dir, &size) != 0) {
        return -1;
    }
    fill_stat(stat, is_dir, size);
    return 0;
}

/* The decomp calls the libkernl file entry points by their retail addresses, under m2c names;
 * each one forwards to the host implementation above. Retail serialises them with a semaphore
 * (func_00426670 creates it lazily, func_004266c8/func_004266f8 take and release it); the host
 * calls are synchronous on one thread, so func_00426670 has nothing to create.
 *   0x00426cf0 sceOpen(path, flags[, mode])    0x00426f80 sceClose(fd)
 *   0x004270f8 sceLseek(fd, offset, whence)    0x00427338 sceRead(fd, buf, n)
 *   0x004275a8 sceWrite(fd, buf, n)            0x00428f08 sceSync(name, mode)
 *   0x00428550 sceDopen  0x00428618 sceDclose  0x00428780 sceDread  0x004288d8 sceGetstat */
void func_00426670(void)
{
}

int func_00426cf0(const char *name, int flags, int mode)
{
    (void)mode;
    return sceOpen(name, flags);
}

int func_00426f80(int fd)
{
    return sceClose(fd);
}

int func_004270f8(int fd, int offset, int whence)
{
    return sceLseek(fd, offset, whence);
}

int func_00427338(int fd, void *buf, int n)
{
    return sceRead(fd, buf, n);
}

int func_004275a8(int fd, const void *buf, int n)
{
    return sceWrite(fd, buf, n);
}

int func_00428f08(const char *name, int mode)
{
    return sceSync(name, mode);
}

int func_00428550(const char *dirname)
{
    return sceDopen(dirname);
}

int func_00428618(int fd)
{
    return sceDclose(fd);
}

int func_00428780(int fd, void *dirent)
{
    return sceDread(fd, dirent);
}

int func_004288d8(const char *name, void *stat)
{
    return sceGetstat(name, stat);
}

/* func_00427dd8 is the shared path-command call behind sceRemove (6), sceRmdir (8), sceDelDrv
 * (0x10) and sceChdir (0x12). The tree is read-only, so every command that would change it
 * fails with -1; sceChdir succeeds when the path names a directory (the host layer keeps no
 * working directory, so relative paths are not resolved against it). */
int func_00427dd8(const char *name, int command)
{
    int is_dir;
    long long size;

    if (command == 0x12 && vfs_stat(name, &is_dir, &size) == 0 && is_dir) {
        return 0;
    }
    return -1;
}

/* func_00427868 is the fio status call (retail sceIoctl-style: fd, command, out). Retail
 * answers commands 1, 2 and 3 locally from its handle table and the last IOP reply; the host
 * layer completes every operation before it returns, so nothing is ever pending: command 1
 * stores 0 (no operation outstanding in any of the 32 handle slots), command 2 stores a 0
 * word and command 3 a 0 doubleword, and each returns 0. Other commands go to the IOP on
 * retail and are not available: -1. */
int func_00427868(int fd, int command, void *out)
{
    (void)fd;
    if (out == 0) {
        return -1;
    }
    switch (command) {
    case 1:
    case 2:
        memset(out, 0, 4);
        return 0;
    case 3:
        memset(out, 0, 8);
        return 0;
    default:
        return -1;
    }
}

/* sceLseek64 (libkernl filestub.o, retail 0x00429358): preserve the full EE offset and
 * v0 result when CRI selects its 64-bit file mode. The VFS already keeps 64-bit positions. */
int64_t func_00429358(int fd, int64_t offset, int whence)
{
    return vfs_seek(fd, offset, whence);
}
