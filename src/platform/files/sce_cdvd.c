/*
 * libcdvd entry points on the boot path. sceCdInit (cdvd000.o, retail 0x0042eae0) mounts
 * the disc named by P4PC_ISO and the loose-file override directory named by
 * P4PC_OVERRIDE_DIR (see docs/platform/disc_cri.md). sceCdMmode (cdvd036.o, retail 0x0042f600) only selects
 * the drive's media mode, which the host does not have.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/platform.h"
#include "vfs.h"
#include "p4pc/vfs_source.h"
#include "p4pc/profile_trace.h"

/* Referenced by the decomp as func_0042eae0. Returns 1 when the tree is mounted, 0 on
 * failure. With P4PC_ISO unset the tree is mounted with overrides only and every disc
 * file is missing. */
int func_0042eae0(int mode)
{
    const char *iso = vfs_source_selected_iso();

    (void)mode;
    if (!iso) {
        fprintf(stderr, "p4pc: sceCdInit: P4PC_ISO is unset, no disc files will be found\n");
    }
    if (vfs_source_mount_selected() != 0) {
        fprintf(stderr, "p4pc: sceCdInit: mount failed (P4PC_ISO=%s)\n", iso);
        return 0;
    }
    return 1;
}

int sceCdMmode(int mode)
{
    (void)mode;
    return 1;
}

/* Layout of libcdvd's sceCdlFILE (the decomp's include/cri/ps2/libcdvd.h). */
typedef struct {
    unsigned int lsn;
    unsigned int size;
    char name[16];
    unsigned char date[8];
} sceCdlFILE;

/* Returns 1 and fills *fp when the file is on the disc, 0 otherwise. The LSN is the
 * file's byte offset in the disc image divided by the 2048-byte sector. A file that does
 * not start on a sector boundary has no LSN, so it is reported as not found. */
int sceCdSearchFile(sceCdlFILE *fp, const char *name)
{
    long long offset;
    long long size;
    const char *base = name;
    size_t n;

    if (!fp || !name || vfs_disc_locate(name, &offset, &size) != 0) {
        return 0;
    }
    if (offset % CD_SECTOR != 0 || offset / CD_SECTOR > 0xffffffffLL || size > 0xffffffffLL) {
        return 0;
    }
    fp->lsn = (unsigned int)(offset / CD_SECTOR);
    fp->size = (unsigned int)size;
    for (const char *p = name; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':') {
            base = p + 1;
        }
    }
    memset(fp->name, 0, sizeof fp->name);
    n = strlen(base);
    if (n > sizeof fp->name - 1) {
        n = sizeof fp->name - 1;
    }
    memcpy(fp->name, base, n);
    return 1;
}

/* Host bytes are available immediately, but an accepted read remains busy until
 * the next scheduler frame boundary. This is an ordering fence, not PS2 pacing. */
static int32_t sReadAlarm;

static uint32_t read_complete(int32_t id, void *arg)
{
    uint64_t trace_start = p4pc_prof_span_begin();
    (void)arg;
    sReadAlarm = 0;
    p4pc_prof_span_end(trace_start, "scecd.async.complete", 0, 0, (uint32_t)id, "disc");
    return 0;
}

int sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, const void *mode)
{
    (void)mode;
    uint64_t trace_start = p4pc_prof_span_begin();
    if (sReadAlarm != 0)
        p4pc_poll_devices();
    if (sReadAlarm != 0 || vfs_disc_read_sectors(lsn, sectors, buf) != 0) {
        p4pc_prof_span_end(trace_start, "scecd.async.submit", (uint64_t)sectors * CD_SECTOR, 0, (uint32_t)sReadAlarm, "disc");
        return 0;
    }
    sReadAlarm = p4pc_set_next_frame_alarm(read_complete, NULL);
    if (sReadAlarm < 0) {
        sReadAlarm = 0;
        p4pc_prof_span_end(trace_start, "scecd.async.submit", (uint64_t)sectors * CD_SECTOR, 0, 0, "disc");
        return 0;
    }
    p4pc_prof_span_end(trace_start, "scecd.async.submit", (uint64_t)sectors * CD_SECTOR, 1, (uint32_t)sReadAlarm, "disc");
    return 1;
}

/* Mode1 polls; mode0 waits through the deferred publication boundary. */
int sceCdSync(int mode)
{
    uint64_t trace_start = p4pc_prof_span_begin();
    if (sReadAlarm != 0)
        p4pc_poll_devices();
    while (sReadAlarm != 0 && mode == 0)
        p4pc_wait_frame_boundary();
    p4pc_prof_span_end(trace_start, mode == 0 ? "scecd.sync.wait" : "scecd.sync.poll", 0, sReadAlarm != 0, mode, "disc");
    return sReadAlarm != 0;
}

int sceCdGetError(void)
{
    return 0;
}

int sceCdBreak(void)
{
    if (sReadAlarm != 0) {
        p4pc_cancel_alarm(sReadAlarm);
        sReadAlarm = 0;
    }
    return 1;
}

/* The CRI SRD and H_Cdvd units call these retail addresses by their m2c names; the decomp gives
 * them no libcdvd name. 0x0042E7C8 is sceCdSync, 0x0042F200 is sceCdRead (lsn, sectors, buf,
 * mode) and 0x0042F548 is sceCdBreak. */
int func_0042e7c8(int mode)
{
    return sceCdSync(mode);
}

int func_0042f200(unsigned int lsn, unsigned int sectors, void *buf, const void *mode)
{
    return sceCdRead(lsn, sectors, buf, mode);
}

int func_0042f548(void)
{
    return sceCdBreak();
}

/* func_0042efe0 is sceCdDiskReady (H_Cdvd calls it with mode 0 and ignores the result). Retail
 * issues an IOP command and returns the drive status, SCECdComplete (2) when ready and
 * SCECdNotReady (6) otherwise. The host disc image is always ready. */
/* func_0042e2b8 is the layer-aware file search behind sceCdSearchFile (func_0042e5b8 calls it with
 * layer 0; the CRI file layer passes the disc layer from 0x007330CC). The disc image is one
 * tree whose LSNs already include the second layer, so the layer only selects a search on retail
 * and the port ignores it. */
int func_0042e2b8(sceCdlFILE *fp, const char *name, int layer)
{
    (void)layer;
    return sceCdSearchFile(fp, name);
}

int func_0042efe0(int mode)
{
    (void)mode;
    return 2;
}

/* func_004f2798 is ROFS_SetDir(volume, dirlist) (rofs_if.c). Retail hands the "#DirLst#"
 * directory blob to the ROFS driver (the function table at 0x00743A48, slot +0x40), which
 * installs it as that volume's file list, and returns 0 on success; with no driver it reports
 * error -203 through the ROFS error hook. The host VFS (vfs.c) already holds the complete disc
 * and override tree, so there is no per-volume list to install and the call always succeeds.
 * H_Cdvd_SelectDirectory spins until this returns 0, and the kwln init calls ignore the result. */
int func_004f2798(void *volume, const void *dirlist)
{
    (void)volume;
    (void)dirlist;
    return 0;
}


/* sceCdSeek (cdvd010.o, retail 0x0042F3E0): queues a seek to a sector and returns 1 when the drive took
 * the command. The host reads by LSN with no head to move, so the command always succeeds. */
int func_0042f3e0(unsigned int lsn)
{
    (void)lsn;
    return 1;
}

/* sceCdGetError (cdvd015.o, retail 0x0042F4B0): the same as sceCdGetError above, under the retail name
 * the CRI cd layer links to. */
int func_0042f4b0(void)
{
    return sceCdGetError();
}
