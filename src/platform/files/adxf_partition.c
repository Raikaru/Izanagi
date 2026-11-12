/*
 * ADXF partition loading (AFS archives) over the host VFS.
 *
 * Retail (ADXF/PS2EE 7.30, libadxe code1_004c) loads an AFS archive's table of contents into a
 * caller-supplied work area (the "ptinfo") with ADXF_LoadPartitionNw and polls it with
 * ADXF_GetPtStat; later ADXF_OpenAfs(ptid, file) opens a member of the archive. The game starts the
 * three sound banks this way in btlSoundLoadBanks (H_Cdvd_StartStreamLoad / func_004c7ef8).
 *
 * This file translates the retail partition state machine from the asm (0x004c7260..0x004c7f80, the
 * lock wrappers around it, and the ADXF handle primitives it calls: 0x004c8770 open by name,
 * 0x004c8950/0x004c8830 open by archive member, 0x004c8a90 close, 0x004c9068 read start,
 * 0x004c9858 state, 0x004c9a28 member lookup). The retail handle layer sits on ADXSTM and the ROFS CD
 * driver, which the host replaces. Host I/O fills the destination synchronously, but the public ADXF
 * and ROFS states stay busy until the next scheduler frame boundary, preserving asynchronous disc
 * ordering without restoring PS2 seek/transfer delays. The version-6 reference C is
 * decomp/src/cri/libadxe/adx_fs.c; the retail layout and extra request history come from the asm.
 *
 * Retail state kept in the retail data image (so translated retail code that reads it agrees):
 *   0x00716728 load state (1 started, 2 loading, 3 done, 4 error)   0x00716724 partition being loaded
 *   0x00716720 handle of the archive being loaded                    0x0071671c entries parsed so far
 *   0x00716730 work buffer, 0x00716734 sectors per chunk             0x007161f0 ptinfo pointer per partition
 *   0x00716718/0x007165f8/0x007166f8 the command history ring and per-command call counters
 *
 * ptinfo layout (retail offsets): +0 next, +4 size, +8 file count, +12 count (u16), +14 zero, +15 type
 * (0: 16-bit sizes, 1: 32-bit), +16 archive name (256), +272 directory list pointer, +276 first sector
 * of the archive inside its container, +280 first member offset in sectors (u16, or u32 for type 1),
 * +282 (type 0) / +284 (type 1) the table of member sizes in sectors.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/lockstep_disc.h"
#include "p4pc/platform.h"
#include "vfs.h"

#define ADXF_SECTOR 2048
#define ADXF_STAT_PREP 1
#define ADXF_STAT_READING 2
#define ADXF_STAT_READEND 3
#define ADXF_STAT_ERROR 4
#define ADXF_ERR_PRM (-3)
#define ADXF_ERR_FATAL (-1)
#define ADXF_FILE_MAX 0x1000
#define ADXF_NAME_MAX 256

extern void func_004c9d18(void);
extern void func_004c9d30(void);
extern void func_004c6a98(const char *message);

/* Retail data (guest memory): scalars and the history ring are used in place; a pointer word holds a guest
 * address (read as p4_adxf_handle / p4_adxf_work, written with the setters below). */
#define p4_adxf_state P4_GDATA(int, 0x00716728u)
#define p4_adxf_ptid P4_GDATA(int, 0x00716724u)
#define p4_adxf_count P4_GDATA(int, 0x0071671Cu)
#define p4_adxf_chunk P4_GDATA(int, 0x00716734u)
#define p4_adxf_history_next P4_GDATA(int, 0x00716718u)
#define p4_adxf_history ((unsigned char (*)[16])P4_G2H(0x007165F8u))
#define p4_adxf_ncall ((unsigned short *)P4_G2H(0x007166F8u))
#define ADXF_HANDLE_WORD P4_GDATA(uint32_t, 0x00716720u)
#define ADXF_WORK_WORD P4_GDATA(uint32_t, 0x00716730u)
#define ADXF_PTINFO_WORD(ptid) (((uint32_t *)P4_G2H(0x007161F0u))[ptid])
#define p4_adxf_handle ((void *)P4_G2H(ADXF_HANDLE_WORD))
#define p4_adxf_work ((unsigned char *)P4_G2H(ADXF_WORK_WORD))
#define adxf_ptinfo(ptid) ((unsigned char *)P4_G2H(ADXF_PTINFO_WORD(ptid)))

/* A pointer as the word the command history keeps (diagnostic only: a host string is truncated). */
#define ADXF_W(p) ((int)(uint32_t)(uintptr_t)(p))

/* The 64-byte aligned 2048-byte area retail takes from its own bss at 0x00716780: guest memory, since its address is
 * stored in the work word. */
static unsigned char *adxf_tmpbuf(void)
{
    static uint32_t tmp;

    if (tmp == 0) {
        tmp = p4_gport_alloc(ADXF_SECTOR, 64);
    }
    return (unsigned char *)P4_G2H(tmp);
}

#include "adxf_host.h"
typedef P4AdxfHandle PartHandle;

static uint32_t adxf_read_complete(int32_t id, void *arg)
{
    P4AdxfHandle *h = arg;

    (void)id;
    h->completion_alarm = 0;
    h->stat = ADXF_STAT_READEND;
    return 0;
}

void p4pc_adxf_cancel(P4AdxfHandle *h)
{
    if (h->completion_alarm != 0) {
        p4pc_cancel_alarm(h->completion_alarm);
        h->completion_alarm = 0;
    }
    if (h->stat == ADXF_STAT_READING) {
        h->stat = ADXF_STAT_READEND;
    }
}

/* 0x004c7260: records one ADXF command in the history ring: {command, phase, call count, 3 words}. */
void func_004c7260(int command, int phase, int a, int b, int c)
{
    int index = p4_adxf_history_next;
    unsigned char *entry;

    index -= (index < 0 ? (index + 15) : index) >> 4 << 4;
    entry = p4_adxf_history[index];
    p4_adxf_history_next = index;
    if (phase == 0) {
        p4_adxf_ncall[command] = (unsigned short)(p4_adxf_ncall[command] + 1);
    }
    entry[0] = (unsigned char)command;
    entry[1] = (unsigned char)phase;
    *(int *)(entry + 12) = c;
    *(int *)(entry + 4) = a;
    *(int *)(entry + 8) = b;
    p4_adxf_history_next = index + 1;
    *(unsigned short *)(entry + 2) = p4_adxf_ncall[command];
}

/* 0x004c7238: bytes to 2048-byte sectors, rounding up. */
int func_004c7238(int bytes)
{
    int sectors = (bytes < 0 ? bytes + 2047 : bytes) >> 11;

    return sectors + (bytes - (sectors << 11) > 0);
}

/* 0x004c7340: validates the partition id and the work area given to a load. */
int func_004c7340(unsigned int ptid, void *ptinfo)
{
    if (ptid >= 256) {
        func_004c6a98("E9040801:partition ID is range outside.(adxf_ChkPrmPt)");
        return ADXF_ERR_PRM;
    }
    if (ptinfo == NULL) {
        func_004c6a98("E9040802:'ptinfo' is NULL.(adxf_ChkPrmPt)");
        return ADXF_ERR_PRM;
    }
    return 0;
}

/* Resolves an ADXF / ROFS file name to a VFS path the way the ROFS volumes do. "VOL:NAME" is a file of the main
 * volume's selected directory (the DATA container), "BTL:NAME" of the BTL volume's, "BGM:NAME" and "ENV:NAME" of the
 * root of their containers, a name without a volume is the main volume's. Retail searches the one directory
 * list its volume was handed (H_Cdvd_SelectDirectory, ROFS_SetDir) and finds nothing else, or the list the
 * caller passes as the open's `dir` argument (`blob`): a name that is not in that directory is not found.
 * Returns 0 and fills `out`, or -1. */
int p4pc_adxf_resolve_name(const char *name, const void *blob, char *out, size_t cap)
{
    const char *label = "DATA";
    const char *dir = NULL;
    const char *base = name;
    long long size = 0;
    int isDir = 0;

    if (strncmp(name, "VOL:", 4) == 0) {
        base += 4;
    } else if (strncmp(name, "BTL:", 4) == 0) {
        label = "BTL";
        base += 4;
        dir = p4pc_cdvd_dir(1);
    } else if (strncmp(name, "BGM:", 4) == 0 || strncmp(name, "ENV:", 4) == 0) {
        label = name[0] == 'B' ? "BGM" : "ENV";
        base += 4;
        dir = "\\";
    }
    if (blob != NULL && (label[0] == 'D' || label[0] == 'B') && label[1] != 'G') {
        int volume = 0;
        const char *named = p4pc_cdvd_dir_of_blob(blob, &volume);

        if (named != NULL && volume == (label[0] == 'B')) {
            dir = named;
        }
    }
    if (dir == NULL) {
        dir = label[0] == 'D' ? p4pc_cdvd_dir(0) : "\\";
    }
    snprintf(out, cap, "%s%s%s", label, dir, base);
    return vfs_stat(out, &isDir, &size) == 0 && !isDir ? 0 : -1;
}

/* 0x004c8770: opens `fname` for a range of `length` sectors from sector `start`. Returns a handle or 0. */
void *func_004c8770(const char *fname, void *dir, int start, int length)
{
    PartHandle *h = NULL;
    char path[512];
    long long fsize = 0;
    int isDir = 0;
    int fd;

    func_004c7260(1, 0, ADXF_W(fname), ADXF_W(dir), -1);
    if (fname != NULL && p4pc_adxf_resolve_name(fname, dir, path, sizeof path) == 0 && vfs_stat(path, &isDir, &fsize) == 0
        && !isDir && (fd = vfs_open(path, 0)) >= 0) {
        h = p4pc_heap_calloc(1, sizeof *h); /* the game holds the handle: guest memory */
        if (h == NULL) {
            vfs_close(fd);
        } else {
            h->used = 1;
            h->stat = ADXF_STAT_PREP;
            h->fd = fd;
            h->base = (long long)start * ADXF_SECTOR;
            h->size = fsize - h->base;
            if ((long long)length * ADXF_SECTOR < h->size) {
                h->size = (long long)length * ADXF_SECTOR;
            }
            if (h->size < 0) {
                h->size = 0;
            }
        }
    } else if (fname != NULL) {
        fprintf(stderr, "p4pc: ADXF partition open: %s not found\n", fname);
    }
    func_004c7260(1, 1, ADXF_W(fname), ADXF_W(dir), -1);
    return h;
}

/* 0x004c8a90: closes a handle, cancelling any unpublished completion first. */
void func_004c8a90(void *handle)
{
    PartHandle *h = handle;

    func_004c7260(3, 0, ADXF_W(handle), -1, -1);
    if (h != NULL) {
        p4pc_adxf_cancel(h);
        vfs_close(h->fd);
        memset(h, 0, sizeof *h);
        p4pc_heap_free(h);
    }
    func_004c7260(3, 1, ADXF_W(handle), -1, -1);
}

/* 0x004c9888: checks a partition id and member index against the loaded table. */
int func_004c9888(unsigned int ptid, int member)
{
    unsigned char *pt;

    if (ptid >= 256 || (pt = adxf_ptinfo(ptid)) == NULL) {
        func_004c6a98("E9040828:'ptid' is range outside.");
        return ADXF_ERR_PRM;
    }
    if (member < 0 || member >= *(int *)(pt + 8)) {
        func_004c6a98("E9040828:'flid' is range outside.");
        return ADXF_ERR_PRM;
    }
    return 0;
}

/* 0x004c9a28: archive name, directory list, first sector and size in sectors of member `member`. */
int func_004c9a28(int ptid, int member, char *name, int *dir, int *start, int *length)
{
    int result = func_004c9888((unsigned int)ptid, member);
    unsigned char *pt;
    int sector, i;

    if (result < 0) {
        *dir = 0;
        *start = -1;
        *length = -1;
        return result;
    }
    pt = adxf_ptinfo(ptid);
    if (pt[15] == 1) {
        const unsigned int *sizes = (const unsigned int *)(pt + 284);

        sector = *(int *)(pt + 280);
        for (i = 0; i < member; i++) {
            sector += (int)sizes[i];
        }
        *length = (int)sizes[member];
    } else {
        const unsigned short *sizes = (const unsigned short *)(pt + 282);

        sector = *(unsigned short *)(pt + 280);
        for (i = 0; i < member; i++) {
            sector += sizes[i];
        }
        *length = sizes[member];
    }
    strncpy(name, (const char *)(pt + 16), ADXF_NAME_MAX);
    *dir = *(int *)(pt + 272);
    *start = *(int *)(pt + 276) + sector;
    return result;
}

/* 0x004c9bb8: the archive-name field of a partition (ptinfo + 16). */
void *func_004c9bb8(int ptid)
{
    return adxf_ptinfo(ptid) + 16;
}

/* 0x004c8830: opens the handle for member `member` of partition `ptid`; -3 when the lookup fails. */
static int open_member(PartHandle **out, int ptid, int member)
{
    char name[ADXF_NAME_MAX + 4];
    int dir, start, length;

    if (func_004c9a28(ptid, member, name, &dir, &start, &length) < 0) {
        return ADXF_ERR_PRM;
    }
    *out = func_004c8770(name, P4_G2H((uint32_t)dir), start, length);
    return *out != NULL ? 0 : -1;
}

/* 0x004c8950: the handle for member `member` of partition `ptid`, or 0. */
void *func_004c8950(int ptid, int member)
{
    PartHandle *h = NULL;

    func_004c7260(2, 0, ptid, member, -1);
    if (open_member(&h, ptid, member) < 0) {
        h = NULL;
    }
    func_004c7260(2, 1, ptid, member, -1);
    return h;
}

/* 0x004c9858: status polling does not itself complete an asynchronous transfer. */
int func_004c9858(void *handle)
{
    PartHandle *h = handle;

    if (h == NULL) {
        func_004c6a98("E9040832:'adxf' is NULL.(adxf_GetStat)");
        return ADXF_ERR_PRM;
    }
    if (h->stat == ADXF_STAT_READING) {
        p4pc_lockstep_disc_poll(h); /* an online lockstep replay answers it with retail's poll */
        p4pc_poll_devices();
    }
    return h->stat;
}

/* Sequential read of `sectors` sectors from the handle's position into `buffer` (no alignment rule); the
 * short tail of a member is zero filled. Returns the sectors, or -1 on a read error (state 4). */
int p4pc_adxf_read(P4AdxfHandle *h, int sectors, void *buffer)
{
    long long want = (long long)sectors * ADXF_SECTOR;
    long long avail = h->size - h->pos;
    long long got = 0;

    if (h->stat == ADXF_STAT_READING) {
        p4pc_poll_devices();
    }
    if (h->stat == ADXF_STAT_READING) {
        return 0;
    }
    if (avail < 0) {
        avail = 0;
    }
    if (want < avail) {
        avail = want;
    }
    vfs_seek(h->fd, h->base + h->pos, 0);
    while (got < avail) {
        int n = vfs_read(h->fd, (unsigned char *)buffer + got, (int)(avail - got));

        if (n <= 0) {
            break;
        }
        got += n;
    }
    if (got < avail) {
        h->stat = ADXF_STAT_ERROR;
        return ADXF_ERR_FATAL;
    }
    memset((unsigned char *)buffer + got, 0, (size_t)(want - got));
    h->pos += want;
    h->stat = ADXF_STAT_READING;
    h->completion_alarm = p4pc_set_next_frame_alarm(adxf_read_complete, h);
    if (h->completion_alarm < 0) {
        h->completion_alarm = 0;
        h->stat = ADXF_STAT_ERROR;
        return ADXF_ERR_FATAL;
    }
    return sectors;
}

/* 0x004c9068 (ADXF_ReadNw): reads `sectors` sectors sequentially into `buffer` (64-byte aligned). Returns the
 * sectors requested, or an error code. */
int func_004c9068(void *handle, int sectors, void *buffer)
{
    PartHandle *h = handle;
    int result;

    if ((P4_H2G(buffer) & 0x3f) != 0) {
        func_004c6a98("E0120401:'buf' isn't 64byte alignment.(adxf_ReadNw)");
        return ADXF_ERR_PRM;
    }
    func_004c7260(4, 0, ADXF_W(handle), sectors, ADXF_W(buffer));
    if (h == NULL) {
        func_004c6a98("E9040811:'adxf' is NULL.(adxf_ReadSj32)");
        return ADXF_ERR_PRM;
    }
    if (sectors < 0) {
        func_004c6a98("E9040812:'nsct'is negative.(adxf_ReadSj32)");
        return ADXF_ERR_PRM;
    }
    if (buffer == NULL) {
        func_004c6a98("E9040813:'sj'is NULL.(adxf_ReadSj32)");
        return ADXF_ERR_PRM;
    }
    result = p4pc_adxf_read(h, sectors, buffer);
    func_004c7260(4, 1, ADXF_W(handle), sectors, ADXF_W(buffer));
    return result;
}

/* 0x004c7e28: abandons the partition load: closes its handle and clears the progress. */
int func_004c7e28(void)
{
    func_004c8a90(p4_adxf_handle);
    ADXF_HANDLE_WORD = 0;
    p4_adxf_count = 0;
    p4_adxf_chunk = 0;
    return 0;
}

/* 0x004c7b80: starts a partition load. Arguments (retail passes 11): the partition id, a source selector
 * (`parent`/`member` name a member of an already loaded partition when `fname` is NULL), the archive
 * name and the directory list and first/length sectors to open it with, the ptinfo area, the work
 * buffer and its size, and the table type (0: 16-bit sizes, 1: 32-bit). Returns 0 or a negative error;
 * the state is read through func_004c7f80. */
int func_004c7b80(int ptid, int parent, int member, const char *fname, void *dir, int start, int length,
                  unsigned char *ptinfo, unsigned char *work, int work_size, int type)
{
    int result;

    if (p4_adxf_state == 2) {
        func_004c6a98("E20040917 : It is impossible to load AFS files in parallel.");
        return -1;
    }
    if (p4_adxf_handle != NULL) {
        func_004c8a90(p4_adxf_handle);
        ADXF_HANDLE_WORD = 0;
    }
    p4_adxf_ptid = -1;
    p4_adxf_state = ADXF_STAT_PREP;
    if (work == NULL) {
        func_004c6a98("E1110701:'tmpbuf' is null.(load partition)");
        p4_adxf_state = ADXF_STAT_ERROR;
        return ADXF_ERR_PRM;
    }
    if (work_size <= 0) {
        func_004c6a98("E1110702:'tbsize' <= 0.(load partition)");
        p4_adxf_state = ADXF_STAT_ERROR;
        return ADXF_ERR_PRM;
    }
    result = func_004c7340((unsigned int)ptid, ptinfo);
    if (result < 0) {
        p4_adxf_state = ADXF_STAT_ERROR;
        return result;
    }
    if (p4_adxf_ptid >= 0) {
        func_004c6a98("E0042401:multi-load partition.(load partition)");
        p4_adxf_state = ADXF_STAT_ERROR;
        return -1;
    }
    p4_adxf_state = ADXF_STAT_READING;
    p4_adxf_ptid = ptid;
    p4_adxf_count = 0;
    memset(ptinfo, 0, 284);
    ADXF_PTINFO_WORD(ptid) = P4_H2G(ptinfo);
    *(unsigned int *)ptinfo = 0;
    if (fname == NULL) {
        int dir_out, first, count;

        ADXF_HANDLE_WORD = P4_H2G(func_004c8950(parent, member));
        if (p4_adxf_handle == NULL) {
            func_004c6a98("E9040804:can't open file.(load partition)");
            p4_adxf_state = ADXF_STAT_ERROR;
            return -1;
        }
        result = func_004c9a28(parent, member, (char *)ptinfo + 16, &dir_out, &first, &count);
        if (result < 0) {
            p4_adxf_state = ADXF_STAT_ERROR;
            return result;
        }
        *(int *)(ptinfo + 272) = dir_out;
        *(int *)(ptinfo + 276) = first;
        (void)count;
    } else {
        ADXF_HANDLE_WORD = P4_H2G(func_004c8770(fname, dir, start, length));
        if (p4_adxf_handle == NULL) {
            func_004c6a98("E9040804:can't open file.(load partition)");
            p4_adxf_state = ADXF_STAT_ERROR;
            return -1;
        }
        strncpy((char *)ptinfo + 16, fname, ADXF_NAME_MAX);
        *(int *)(ptinfo + 272) = (int)P4_H2G(dir);
        *(int *)(ptinfo + 276) = start;
    }
    ptinfo[14] = 0;
    ptinfo[15] = (unsigned char)type;
    p4_adxf_chunk = (work_size < 0 ? work_size + 2047 : work_size) >> 11;
    ADXF_WORK_WORD = P4_H2G(work);
    result = func_004c9068(p4_adxf_handle, p4_adxf_chunk, work);
    if (result >= 0) {
        *(int *)(ptinfo + 8) = 0;
        return 0;
    }
    func_004c8a90(p4_adxf_handle);
    p4_adxf_state = ADXF_STAT_ERROR;
    return result;
}

/* 0x004c78c8: partition load by name with the caller's work area: the whole archive (0xfffff sectors from
 * sector 0), 16-bit member sizes. */
int func_004c78c8(int ptid, const char *fname, void *dir, unsigned char *ptinfo, unsigned char *work, int work_size)
{
    return func_004c7b80(ptid, 0, 0, fname, dir, 0, 0xfffff, ptinfo, work, work_size, 0);
}

/* 0x004c7560: ADXF_LoadPartitionNw's body, without the lock: the default 2048-byte work area. */
int func_004c7560(int ptid, const char *fname, void *dir, unsigned char *ptinfo)
{
    return func_004c78c8(ptid, fname, dir, ptinfo, adxf_tmpbuf(), ADXF_SECTOR);
}

/* 0x004c74f8: ADXF_LoadPartitionNw(ptid, fname, dir, ptinfo). */
int func_004c74f8(int ptid, const char *fname, void *dir, unsigned char *ptinfo)
{
    int result;

    func_004c9d18();
    result = func_004c7560(ptid, fname, dir, ptinfo);
    func_004c9d30();
    return result;
}

/* 0x004c7f80: ADXF_GetPtStat: advances the table-of-contents load. When the current read has ended the
 * chunk in the work buffer is parsed (AFS magic and member count on the first chunk, then two words per
 * member: offset and size, kept as the size in sectors) and the next chunk is started; the result is the
 * load state (2 loading, 3 done, 4 error). */
int func_004c7f80(int ptid)
{
    unsigned char *pt;
    unsigned int *sizes32 = NULL;
    unsigned short *sizes16 = NULL;
    int stat, word, words;
    unsigned int *buf;

    if (ptid != p4_adxf_ptid) {
        func_004c6a98("E0041303:illigal parameter 'ptid'.(adxf_GetPtStat)");
        return ADXF_ERR_PRM;
    }
    if (p4_adxf_handle == NULL && ptid != -1) {
        return p4_adxf_state;
    }
    stat = func_004c9858(p4_adxf_handle);
    p4_adxf_state = stat;
    if (stat != ADXF_STAT_READEND) {
        return stat;
    }
    pt = adxf_ptinfo(ptid);
    if ((signed char)pt[15] == 1) {
        sizes32 = (unsigned int *)(pt + 284);
    } else {
        sizes16 = (unsigned short *)(pt + 282);
    }
    buf = (unsigned int *)p4_adxf_work;
    if (*(int *)(pt + 8) == 0) {
        unsigned int count;

        if (memcmp(buf, "AFS", 3) != 0) {
            func_004c6a98("E0040701:Illigal format(not AFS).(adxf_GetPtStat)");
            p4_adxf_state = ADXF_STAT_ERROR;
            func_004c7e28();
            return p4_adxf_state;
        }
        count = buf[1];
        if (count > ADXF_FILE_MAX) {
            func_004c6a98("E0040702:Illigal number of file.(adxf_GetPtStat)");
            p4_adxf_state = ADXF_STAT_ERROR;
            func_004c7e28();
            return p4_adxf_state;
        }
        *(int *)(pt + 8) = (int)count;
        *(unsigned short *)(pt + 12) = (unsigned short)count;
        *(unsigned int *)(pt + 4) = (signed char)pt[15] == 1 ? count * 4 + 288 : ((count * 2 + 284) >> 2) << 2;
        /* the first member's offset is truncated to whole sectors (not rounded up like the sizes) */
        {
            int first = ((int)buf[2] < 0 ? (int)buf[2] + 2047 : (int)buf[2]) >> 11;

            if ((signed char)pt[15] == 1) {
                *(int *)(pt + 280) = first;
            } else {
                *(unsigned short *)(pt + 280) = (unsigned short)first;
            }
        }
        word = 3;
    } else {
        word = 1;
    }
    words = p4_adxf_chunk << 9;
    while (word < words) {
        int sectors = func_004c7238((int)buf[word]);

        if (sizes32 != NULL) {
            sizes32[p4_adxf_count++] = (unsigned int)sectors;
        } else {
            if ((sectors & 0xffff0000) != 0) {
                func_004c6a98("E2122501:AFS file has 128MB or more of inside file.(adxf_GetPtStat)");
                p4_adxf_state = ADXF_STAT_ERROR;
                func_004c7e28();
                return p4_adxf_state;
            }
            sizes16[p4_adxf_count++] = (unsigned short)sectors;
        }
        if (p4_adxf_count >= *(int *)(pt + 8)) {
            p4_adxf_state = ADXF_STAT_READEND;
            func_004c7e28();
            return p4_adxf_state;
        }
        word += 2;
    }
    if (func_004c9068(p4_adxf_handle, p4_adxf_chunk, p4_adxf_work) >= 0) {
        p4_adxf_state = ((PartHandle *)p4_adxf_handle)->stat;
        return p4_adxf_state;
    }
    p4_adxf_state = ADXF_STAT_ERROR;
    func_004c7e28();
    return p4_adxf_state;
}

/* 0x004c7f30: the unlocked ADXF_GetPtStat. */
int func_004c7f30(int ptid)
{
    return func_004c7f80(ptid);
}

/* 0x004c7ef8: ADXF_GetPtStat(ptid) under the lock. */
int func_004c7ef8(int ptid)
{
    int result;

    func_004c9d18();
    result = func_004c7f30(ptid);
    func_004c9d30();
    return result;
}
