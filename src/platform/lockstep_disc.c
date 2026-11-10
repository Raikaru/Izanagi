/* Lockstep-only disc replay (include/p4pc/lockstep_disc.h, docs/lockstep.md).
 *
 * Retail p4hook --lockstep-disc-trace records each top-level disc read with the game frame of
 * its request and of the first status poll that saw it complete, and the VBLANK count of both;
 * tools/lockstep_disc.py turns an earlier session's trace into anchor-relative rows classed game
 * (H_Cdvd_UpdateRequests), stream (ADX stream server) or other. Once the replay anchor is
 * applied (the accepted initial LOAD anchor, or the qualified battle anchor), the frame-alarm
 * seam stands in for the next-frame completion fence of each read (disc reads are its only
 * users). The read is keyed from the VFS read observer: file base name, handle-relative offset
 * and sectors (raw CDVD: LSN and sectors), and classified by its owner. A game-level request
 * takes the next unused game row with that key, must start at the same anchor-relative frame,
 * and completes no earlier than the normal frame boundary and not before the anchor-relative
 * frame retail first observed completion; a device poll in that frame releases it. An ADX
 * stream server read is held only by an unused stream row with its key that retail requested in
 * the same anchor-relative frame (a voice that starts where retail started it), and completes
 * retail's VBLANK count after its request: the stream server polls on VBLANKs, and retail's
 * update counter often steps once more than its server passes between request and completion
 * (collision 900's voice: 5 updates, 8 VBLANKs, 4 passes). Any other
 * stream server read completes normally and is only recorded. Unmatched or unowned reads fail
 * the run. Host bytes are still copied at submission: retail consumers read a destination only
 * after observing completion (H_Cdvd_UpdateRequests state 3). With P4PC_LOCKSTEP_DISC_ONLINE
 * (include/p4pc/lockstep_online.h) game reads come live from retail instead of from the table:
 * the port's n-th game read after the anchor is retail's n-th, and each status poll of it
 * (p4pc_lockstep_disc_poll) is answered by retail's poll in the same update, releasing the read
 * exactly where retail's poll saw it complete. port-disc-holds.tsv records every held and
 * stream read. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/lockstep_disc.h"
#include "p4pc/frame_alarm_seam.h"
#include "p4pc/lockstep_format.h"
#include "p4pc/lockstep_online.h"
#include "p4pc/platform.h"
#include "p4pc/vfs_origin.h"
#include "files/adxf_host.h"

#define DISC_NAME 64
#define DISC_HOLDS 32
#define DISC_SECTOR 2048
#define DISC_RECHECK_US (1000000u / 60u)
#define DISC_VBLANK_NS (1000000000ull / 60u) /* the kernel's VBLANK period (kernel_thread.c KT_VSYNC_NS) */
#define HCDVD_REQUESTS 0x008D1B84u
#define ADXF_PARTITION_HANDLE 0x00716720u /* the archive the partition loader is reading */

/* Held by ADXSTM_ExecServer's test-and-set for its whole pass (src/cri/re4/adx_stmc.c). */
extern int32_t adxstmf_execsvr_flg;

enum { DISC_FILE, DISC_CDVD };

typedef struct {
    int game, stream, used; /* game rows (retail H_Cdvd requests) and stream rows (ADX stream server) are matched */
    char name[DISC_NAME];
    uint32_t offset, sectors;
    int32_t request, complete; /* anchor-relative retail frames; complete -1: never observed */
    int32_t vblanks;           /* retail VBLANKs from the request to the poll that saw completion; -1: never */
} DiscRow;

typedef struct {
    int used, boundary;
    int32_t id;
    uint32_t (*handler)(int32_t, void *);
    void *arg;
    uint32_t request, target;
    uint64_t due_ns; /* a stream hold: released once the scheduler reaches it, not by frame */
    size_t row;
    int online;            /* released only by p4pc_lockstep_disc_poll, when retail's poll saw it complete */
    uint32_t online_index; /* retail's game read index */
} DiscHold;

static DiscRow *sRows;
static size_t sRowCount;
static uint32_t sRetailAnchor, sHorizon, sAnchor;
static FILE *sReceipt;
static int sConfigured, sOnline;
static uint32_t sOnlineReads;
static DiscHold sHolds[DISC_HOLDS];
static struct {
    int valid, family;
    char name[DISC_NAME];
    int64_t first, bytes;
} sRead;

static void disc_fail(const char *why) {
    fprintf(stderr, "p4pc: state-aligned lockstep: disc replay: %s\n", why);
    exit(4);
}

static uint32_t game_frame(void) {
    return p4ls_get((const uint8_t *)P4_G2H(P4LS_GAME_FRAME));
}

/* Upper-case final component after any '/', '\\' or volume ':'. */
static void base_name(const char *path, char out[DISC_NAME]) {
    const char *start = path;
    size_t i;
    for (const char *p = path; *p; p++)
        if (*p == '/' || *p == '\\' || *p == ':') start = p + 1;
    for (i = 0; start[i] && i + 1 < DISC_NAME; i++) out[i] = (char)toupper((unsigned char)start[i]);
    out[i] = '\0';
}

/* Keys only: contiguous chunks of one file accumulate into the read the next alarm completes.
 * file_offset is relative to the VFS file; an ADXF member window's base is removed in
 * seam_set. Retail has no read for a host or mod override, so those fail. */
static void observe_read(const P4VfsOrigin *origin, const void *bytes, size_t size,
                         int64_t file_offset, int64_t disc_offset, void *user) {
    char name[DISC_NAME];
    (void)bytes;
    (void)user;
    if (!origin || origin->kind != P4_VFS_DISC || disc_offset < 0)
        disc_fail("a read after the anchor comes from a host or mod override");
    if (file_offset < 0) {
        sRead.valid = 1;
        sRead.family = DISC_CDVD;
        sRead.name[0] = '\0';
        sRead.first = disc_offset;
        sRead.bytes = (int64_t)size;
        return;
    }
    base_name(origin->key ? origin->key : "", name);
    if (sRead.valid && sRead.family == DISC_FILE && !strcmp(sRead.name, name) &&
        file_offset == sRead.first + sRead.bytes) {
        sRead.bytes += (int64_t)size;
        return;
    }
    sRead.valid = 1;
    sRead.family = DISC_FILE;
    memcpy(sRead.name, name, sizeof name);
    sRead.first = file_offset;
    sRead.bytes = (int64_t)size;
}

static void receipt(const DiscHold *hold, uint32_t frame, const char *outcome) {
    const DiscRow *row = &sRows[hold->row];
    if (fprintf(sReceipt, "%lld\t%s\t%s\t%u\t%u\t%u\t%d\t%d\t%u\t%u\t%s\n",
                (long long)hold->row, row->stream ? "stream" : "game", row->name, row->offset, row->sectors,
                hold->request, row->request, row->complete, frame, frame - hold->request, outcome) < 0 ||
        fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
}

/* An ADX stream server read without a retail stream row of the same key and request frame
 * completes at the normal frame boundary; only the request is recorded. */
static void stream_receipt(const char *name, uint32_t offset, uint32_t sectors, uint32_t frame) {
    if (fprintf(sReceipt, "-1\tstream\t%s\t%u\t%u\t%u\t-1\t-1\t-1\t-1\tstream-not-held\n",
                name[0] ? name : "-", offset, sectors, frame) < 0 || fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
}

static uint32_t guest_word(uint32_t address) {
    if (address < 0x100000u || address > 0x2000000u - 4u || (address & 3u))
        disc_fail("the H_Cdvd request list leaves guest RAM");
    return p4ls_get((const uint8_t *)P4_G2H(address));
}

/* Classification is by ownership, never by file type. A handle held by an H_Cdvd request node
 * (decomp/src/Kernel/sdkCdvd.c: list head 0x008D1B84, next +0x4, adxf +0x154) is a game-level
 * request, and so is the archive handle the ADXF partition loader is reading a table of contents
 * through (func_004c7b80/adxf_GetPtStat, src/platform/files/adxf_partition.c: retail word
 * 0x00716720), such as an event's voice .AFS. A read issued while the ADXSTM server pass holds
 * its test-and-set flag belongs to the ADX stream server (BGM .ADX, voice .AFS members).
 * Anything else refuses the run. */
static int hcdvd_request(uint32_t handle) {
    uint32_t node = guest_word(HCDVD_REQUESTS);
    for (unsigned count = 0; node; count++) {
        if (count == 256) disc_fail("the H_Cdvd request list does not terminate");
        if (guest_word(node + 0x154u) == handle) return 1;
        node = guest_word(node + 0x4u);
    }
    return 0;
}

static int partition_request(uint32_t handle) {
    return guest_word(ADXF_PARTITION_HANDLE) == handle;
}

static void release(DiscHold *hold, int from_alarm) {
    const DiscHold held = *hold;
    hold->used = 0;
    receipt(&held, game_frame(), "released");
    if (!from_alarm) p4pc_cancel_alarm(held.id);
    held.handler(held.id, held.arg);
}

/* A game read is due in retail's completion frame. A stream read is due at retail's VBLANK count
 * after its request: the stream server polls on VBLANKs, and retail's update counter steps between
 * its request and its completing poll once more than its passes do as often as not. */
static int hold_due(const DiscHold *hold) {
    if (hold->online) return 0;
    if (hold->due_ns != 0) return p4pc_scheduler_time_ns() >= hold->due_ns;
    return (int32_t)(game_frame() - hold->target) >= 0;
}

static uint32_t hold_alarm(int32_t id, void *arg) {
    DiscHold *hold = arg;
    (void)id;
    if (!hold->used) return 0;
    hold->boundary = 1;
    if (hold_due(hold)) {
        release(hold, 1);
        return 0;
    }
    return DISC_RECHECK_US; /* polls when it is due release it; this covers callers that only wait */
}

static int32_t seam_set(uint32_t (*handler)(int32_t, void *), void *arg) {
    char why[384];
    const uint32_t frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    uint32_t offset, sectors;
    size_t index, slot;
    if (!sRead.valid) {
        /* No bytes were read: a zero-sector read (or one at the end of its window). The ADX stream
         * server issues these (retail's ADXSTM does too, through rofs_read); it is not held, so it
         * is receipted with no sectors. Any other unobserved read refuses the run. */
        const P4AdxfHandle *file = arg;
        P4VfsOrigin origin;
        char name[DISC_NAME] = "-";
        if (file != NULL && !vfs_origin(file->fd, &origin) && origin.key) base_name(origin.key, name);
        if (file != NULL && adxstmf_execsvr_flg && file->pos >= 0 && file->pos % DISC_SECTOR == 0) {
            stream_receipt(name, (uint32_t)(file->pos / DISC_SECTOR), 0, frame);
            return p4pc_set_frame_boundary_alarm(handler, arg);
        }
        if (file == NULL)
            snprintf(why, sizeof why, "a raw sector read at port frame %u (anchor+%d) has no preceding VFS read",
                     frame, relative);
        else
            snprintf(why, sizeof why, "a file read of %s at port frame %u (anchor+%d) has no preceding VFS read "
                     "(handle position %lld of %lld bytes) and is not the ADX stream server's", name, frame,
                     relative, (long long)file->pos, (long long)file->size);
        disc_fail(why);
    }
    if (sRead.family == DISC_CDVD) {
        if (arg != NULL) disc_fail("a raw sector read completes through a file handle");
        offset = (uint32_t)(sRead.first / DISC_SECTOR);
        sectors = (uint32_t)((sRead.bytes + DISC_SECTOR - 1) / DISC_SECTOR);
    } else {
        const P4AdxfHandle *file = arg;
        P4VfsOrigin origin;
        char name[DISC_NAME];
        int64_t before;
        if (file == NULL) disc_fail("a file read completes without its handle");
        /* One row per read: the first observed chunk gives the position, the handle's
         * advance (the sectors argument) the size. An unobserved read (at or past the end
         * of its window) or a record from another file fails rather than being reused. */
        if (vfs_origin(file->fd, &origin) || !origin.key)
            disc_fail("a file read completes on a handle without a disc origin");
        base_name(origin.key, name);
        before = sRead.first - file->base;
        if (strcmp(name, sRead.name) || before < 0 || before % DISC_SECTOR || file->pos <= before ||
            (file->pos - before) % DISC_SECTOR ||
            sRead.bytes != (file->pos - before < file->size - before ? file->pos - before : file->size - before))
            disc_fail("the last observed VFS read is not exactly this handle's read");
        offset = (uint32_t)(before / DISC_SECTOR);
        sectors = (uint32_t)((file->pos - before) / DISC_SECTOR);
    }
    sRead.valid = 0;
    if (sRead.family != DISC_FILE || !(hcdvd_request(P4_H2G(arg)) || partition_request(P4_H2G(arg)))) {
        if (!adxstmf_execsvr_flg) {
            snprintf(why, sizeof why, "%s read %s offset %u sectors %u at port frame %u (anchor+%d) has neither "
                     "an H_Cdvd request node, the ADXF partition loader nor the ADX stream server as owner",
                     sRead.family == DISC_CDVD ? "raw sector" : "file", sRead.name[0] ? sRead.name : "-",
                     offset, sectors, frame, relative);
            disc_fail(why);
        }
        /* A stream server read is held only by a retail stream row of the same key that started in
         * the same anchor-relative frame and completed: a new stream (a voice) starts where retail
         * started it, and its data arrives as many VBLANKs later as on retail. A read without one
         * (BGM, whose position follows audio playback) is recorded and completes at the normal
         * frame boundary. */
        for (index = 0; index < sRowCount; index++) {
            const DiscRow *row = &sRows[index];
            if (!row->used && row->stream && !strcmp(row->name, sRead.name) && row->offset == offset &&
                row->sectors == sectors && row->request == relative && row->complete >= 0 && row->vblanks > 0)
                break;
        }
        if (index == sRowCount) {
            stream_receipt(sRead.name, offset, sectors, frame);
            return p4pc_set_frame_boundary_alarm(handler, arg);
        }
    } else if (sOnline) {
        char name[P4ON_NAME];
        uint32_t retail_offset, retail_sectors;
        int32_t request;
        DiscRow *grown;
        p4pc_online_read(sOnlineReads, name, &retail_offset, &retail_sectors, &request);
        if (strcmp(name, sRead.name) || retail_offset != offset || retail_sectors != sectors || request != relative) {
            snprintf(why, sizeof why, "game read %s offset %u sectors %u at port frame %u (anchor+%d) is retail's "
                     "game read %u, %s offset %u sectors %u at anchor+%d", sRead.name, offset, sectors, frame,
                     relative, sOnlineReads, name, retail_offset, retail_sectors, request);
            disc_fail(why);
        }
        grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
        if (!grown) disc_fail("allocating the rows");
        sRows = grown;
        memset(&sRows[sRowCount], 0, sizeof *sRows);
        sRows[sRowCount].game = 1;
        strcpy(sRows[sRowCount].name, name);
        sRows[sRowCount].offset = offset;
        sRows[sRowCount].sectors = sectors;
        sRows[sRowCount].request = request;
        sRows[sRowCount].complete = -1;
        sRows[sRowCount].vblanks = -1;
        index = sRowCount++;
    } else {
        for (index = 0; index < sRowCount; index++) {
            const DiscRow *row = &sRows[index];
            if (!row->used && row->game && !strcmp(row->name, sRead.name) &&
                row->offset == offset && row->sectors == sectors)
                break;
        }
    }
    if (index == sRowCount) {
        snprintf(why, sizeof why, "unmatched game read %s offset %u sectors %u at port frame %u "
                 "(anchor+%d; the retail trace ends at anchor+%d)", sRead.name, offset, sectors, frame,
                 relative, (int32_t)(sHorizon - sRetailAnchor));
        disc_fail(why);
    }
    if (sRows[index].request != relative || (sRows[index].complete < 0 && !(sOnline && sRows[index].game))) {
        snprintf(why, sizeof why, "read %s offset %u sectors %u at port frame %u (anchor+%d) matches retail "
                 "row %llu at anchor+%d with completion %d", sRead.name[0] ? sRead.name : "-", offset,
                 sectors, frame, relative, (unsigned long long)index, sRows[index].request,
                 sRows[index].complete);
        disc_fail(why);
    }
    for (slot = 0; slot < DISC_HOLDS && sHolds[slot].used; slot++) {
    }
    if (slot == DISC_HOLDS) disc_fail("more than 32 disc reads held at once");
    sRows[index].used = 1;
    sHolds[slot] = (DiscHold){1, 0, 0, handler, arg, frame, sAnchor + (uint32_t)sRows[index].complete, 0, index, 0, 0};
    if (sOnline && sRows[index].game) {
        sHolds[slot].online = 1;
        sHolds[slot].online_index = sOnlineReads++;
    }
    /* Due half a VBLANK early, so the stream server's poll at retail's VBLANK sees it complete. */
    if (sRows[index].stream)
        sHolds[slot].due_ns = p4pc_scheduler_time_ns() + (uint64_t)sRows[index].vblanks * DISC_VBLANK_NS -
                              DISC_VBLANK_NS / 2;
    sHolds[slot].id = p4pc_set_frame_boundary_alarm(hold_alarm, &sHolds[slot]);
    if (sHolds[slot].id < 0) disc_fail("no kernel alarm for a held read");
    return sHolds[slot].id;
}

/* A stopped or closed read drops its hold before the kernel cancels the alarm. */
static void seam_cancel(int32_t id) {
    for (size_t slot = 0; slot < DISC_HOLDS; slot++) {
        if (sHolds[slot].used && sHolds[slot].id == id) {
            const DiscHold held = sHolds[slot];
            sHolds[slot].used = 0;
            receipt(&held, game_frame(), "cancelled");
        }
    }
}

static void seam_poll(void) {
    for (size_t slot = 0; slot < DISC_HOLDS; slot++)
        if (sHolds[slot].used && (sHolds[slot].boundary || sHolds[slot].due_ns != 0) && hold_due(&sHolds[slot]))
            release(&sHolds[slot], 0);
}

static const P4FrameAlarmSeam sSeam = {seam_set, seam_cancel, seam_poll};

void p4pc_lockstep_disc_poll(const void *handle) {
    char why[384];
    if (!sOnline || p4pc_frame_alarm_seam != &sSeam) return;
    for (size_t slot = 0; slot < DISC_HOLDS; slot++) {
        DiscHold *hold = &sHolds[slot];
        int32_t update;
        int complete;
        if (!hold->used || !hold->online || hold->arg != handle) continue;
        const uint32_t frame = game_frame();
        const int32_t relative = (int32_t)(frame - sAnchor);
        p4pc_online_read_poll(hold->online_index, &update, &complete);
        if (update != relative) {
            snprintf(why, sizeof why, "game read %s offset %u sectors %u polled at port frame %u (anchor+%d), where "
                     "retail's poll of it is at anchor+%d", sRows[hold->row].name, sRows[hold->row].offset,
                     sRows[hold->row].sectors, frame, relative, update);
            disc_fail(why);
        }
        if (complete) {
            sRows[hold->row].complete = relative;
            release(hold, 0);
        }
        return;
    }
}

int p4pc_lockstep_disc_open(const char *directory) {
    const char *source = getenv("P4PC_LOCKSTEP_DISC_REPLAY");
    char line[512], path[1200];
    FILE *table;
    sOnline = p4pc_online_configured();
    if (!source || !*source) {
        if (!sOnline) return 0;
        goto receipt; /* online without a table: stream reads are not held */
    }
    table = fopen(source, "r");
    if (!table) disc_fail("cannot open P4PC_LOCKSTEP_DISC_REPLAY");
    if (!fgets(line, sizeof line, table) ||
        sscanf(line, "# p4pc-disc-replay 2 retail_anchor %u horizon %u", &sRetailAnchor, &sHorizon) != 2)
        disc_fail("the table header is not p4pc-disc-replay 2");
    while (fgets(line, sizeof line, table)) {
        DiscRow row = {0};
        char family[16], name[128];
        DiscRow *grown;
        if (line[0] == '#' || line[0] == '\n') continue;
        /* family file offset sectors request_rel complete_rel layer order vblanks */
        if (sscanf(line, "%15s %127s %u %u %d %d %*s %*u %d", family, name, &row.offset, &row.sectors,
                   &row.request, &row.complete, &row.vblanks) != 7)
            disc_fail("malformed table row");
        if (!strcmp(family, "game")) row.game = 1;
        else if (!strcmp(family, "stream")) row.stream = 1;
        else if (strcmp(family, "substrate") && strcmp(family, "other"))
            disc_fail("unknown table row class");
        base_name(strcmp(name, "-") ? name : "", row.name);
        grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
        if (!grown) disc_fail("allocating the table");
        sRows = grown;
        sRows[sRowCount++] = row;
    }
    if (ferror(table) || fclose(table)) disc_fail("reading the table");
receipt:
    snprintf(path, sizeof path, "%s/port-disc-holds.tsv", directory);
    sReceipt = fopen(path, "w");
    if (!sReceipt ||
        fprintf(sReceipt, "# p4pc-disc-holds 1 retail_anchor %u horizon %u rows %llu\n"
                "row\tfamily\tfile\toffset\tsectors\trequest_frame\tretail_request\tretail_complete\t"
                "release_frame\tupdates\toutcome\n",
                sRetailAnchor, sHorizon, (unsigned long long)sRowCount) < 0 || fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
    sConfigured = 1;
    return 1;
}

void p4pc_lockstep_disc_activate(uint32_t anchor_frame) {
    if (!sConfigured || p4pc_frame_alarm_seam) return;
    sAnchor = anchor_frame;
    if (vfs_read_observer_register(observe_read, NULL) != 0)
        disc_fail("the VFS read observer is already in use");
    if (fprintf(sReceipt, "# anchor %u\n", anchor_frame) < 0 || fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
    p4pc_frame_alarm_seam = &sSeam;
}
