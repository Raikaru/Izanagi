/*
 * ADXF file handles over the host VFS, for the game's disc request path (H_Cdvd_UpdateRequests,
 * decomp/src/Kernel/sdkCdvd.c, and H_Cdvd_FileExists). Retail opens "VOL:NAME" or "BTL:NAME"
 * through the CRI ADXF layer (libadxe, code1_004c) after H_Cdvd_SelectDirectory has installed the
 * directory's file list in the ROFS driver; reads are asynchronous and polled. The host has the
 * whole tree in vfs.c, so the directory is remembered (overrides/00455230.c calls
 * p4pc_cdvd_set_dir) and an accepted read stays busy until the next scheduler frame boundary.
 *
 * Retail functions replaced (all asm-only in promoted code1_004c.c):
 *   0x004c85a0 open(name, mode)        returns a handle or 0
 *   0x004c8a60 close(handle)
 *   0x004c9670 size in bytes
 *   0x004c95f8 size in 2048-byte sectors
 *   0x004c9010 read(handle, sectors, buffer): returns the sectors read
 *   0x004c9820 state: 3 (ADXF_STAT_READEND) once the read is done, 4 (error) on a failed read
 * The call pattern is in sdkCdvd.c H_Cdvd_UpdateRequests: the buffer holds (sectors << 11) + 0x40
 * bytes, a short read is an error, and the state is polled until 3.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adxf_host.h"
#include "p4pc/gmem.h"
#include "p4pc/lockstep_disc.h"
#include "p4pc/platform.h"
#include "vfs.h"

#define ADXF_SECTOR 2048
#define ADXF_STAT_READEND 3
#define ADXF_STAT_ERROR 4
#define DIR_MAX 256

/* The handle is the shared window handle of adxf_host.h (also what the ADXF partition code and the ROFS device
 * return), so a handle from func_004c8950 (ADXF_OpenAfs) works with the functions below. Opened by name
 * it covers the whole file. */
typedef P4AdxfHandle AdxfFile;

/* The directory each ROFS volume has selected: [0] the main volume ("VOL:"), [1] "BTL:". The BGM and ENV
 * volumes have no directory list, their files are at the container root. */
static char sCurrentDir[2][DIR_MAX] = {"\\", "\\"};

/* Records the directory H_Cdvd_SelectDirectory chose on a volume (0 main, 1 BTL): a normalized path such
 * as "\\FIELD\\RMD\\". */
void p4pc_cdvd_set_dir(const char *dir, int volume)
{
    snprintf(sCurrentDir[volume != 0], DIR_MAX, "%s", dir);
}

const char *p4pc_cdvd_dir(int volume)
{
    return sCurrentDir[volume != 0];
}

/* The game's directory tables (decomp h_cdvd.c): names and "#DirLst#" blobs side by side, each table ended by
 * an empty name. BTL: names 0x007107b0, blobs 0x00710800; main volume: names 0x00711190, blobs 0x007113e0. */
const char *p4pc_cdvd_dir_of_blob(const void *blob, int *volume)
{
    static const uint32_t names[2] = {0x00711190u, 0x007107b0u};
    static const uint32_t blobs[2] = {0x007113e0u, 0x00710800u};
    int v;
    int i;

    for (v = 0; v < 2; v++) {
        for (i = 0; i < 300; i++) {
            const char *name = (const char *)P4_G2H(P4_G32(names[v] + 4u * (uint32_t)i));

            if (name[0] == '\0') {
                break;
            }
            if (P4_G2H(P4_G32(blobs[v] + 4u * (uint32_t)i)) == blob) {
                *volume = v;
                return name;
            }
        }
    }
    return NULL;
}

/* Opens "VOL:NAME" or "BTL:NAME" in the current directory. A BTL: name is looked up only in the
 * BTL container; a VOL: name in the container search order of vfs.c. Returns 0 when not found. */
void *func_004c85a0(const char *name, int mode)
{
    char path[DIR_MAX + 128];
    AdxfFile *file;
    int isDir = 0;
    long long size = 0;
    int fd;

    (void)mode;
    if (name == NULL) {
        return NULL;
    }
    if (p4pc_adxf_resolve_name(name, NULL, path, sizeof path) != 0 || vfs_stat(path, &isDir, &size) != 0 || isDir) {
        fprintf(stderr, "p4pc: ADXF open: %s not found (dir %s)\n", name, sCurrentDir[0]);
        return NULL;
    }
    fd = vfs_open(path, 0);
    if (fd < 0) {
        return NULL;
    }
    file = p4pc_heap_calloc(1, sizeof *file); /* the game keeps the handle: guest memory */
    if (file == NULL) {
        vfs_close(fd);
        return NULL;
    }
    file->used = 1;
    file->fd = fd;
    file->base = 0;
    file->size = size;
    file->pos = 0;
    file->stat = ADXF_STAT_READEND;
    return file;
}

void func_004c8a60(void *handle)
{
    AdxfFile *file = handle;

    if (file != NULL) {
        p4pc_adxf_cancel(file);
        vfs_close(file->fd);
        p4pc_heap_free(file);
    }
}

int func_004c9670(void *handle)
{
    return handle != NULL ? (int)((AdxfFile *)handle)->size : 0;
}

int func_004c95f8(void *handle)
{
    return handle != NULL ? (int)((((AdxfFile *)handle)->size + ADXF_SECTOR - 1) / ADXF_SECTOR) : 0;
}

int func_004c9010(void *handle, int sectors, void *buffer)
{
    AdxfFile *file = handle;
    int done;

    if (file == NULL || buffer == NULL || sectors <= 0) {
        return 0;
    }
    done = p4pc_adxf_read(file, sectors, buffer);
    if (done < 0) {
        return 0;
    }
    return done;
}

int func_004c9820(void *handle)
{
    AdxfFile *file = handle;

    if (file == NULL) {
        return ADXF_STAT_ERROR;
    }
    if (file->stat == 2) {
        p4pc_lockstep_disc_poll(file); /* an online lockstep replay answers it with retail's poll */
        p4pc_poll_devices();
    }
    return file->stat;
}
