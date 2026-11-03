/* Lockstep-only disc replay (include/p4pc/lockstep_disc.h, docs/lockstep.md).
 *
 * Retail p4hook --lockstep-disc-trace records each top-level disc read with the game frame of
 * its request and of the first status poll that saw it complete; tools/lockstep_disc.py turns
 * an earlier session's trace into anchor-relative rows classed game (H_Cdvd_UpdateRequests),
 * stream (ADX stream server) or other. Once the qualified anchor is applied, the frame-alarm
 * seam stands in for the next-frame completion fence of each read (disc reads are its only
 * users). The read is keyed from the VFS read observer: file base name, handle-relative offset
 * and sectors (raw CDVD: LSN and sectors), and classified by its owner. A game-level request
 * takes the next unused game row with that key, must start at the same anchor-relative frame,
 * and completes no earlier than the normal frame boundary and not before the anchor-relative
 * frame retail first observed completion; a device poll in that frame releases it. An ADX
 * stream server read completes normally and is only recorded. Unmatched or unowned reads fail
 * the run. Host bytes are still copied at submission: retail consumers read a destination only
 * after observing completion (H_Cdvd_UpdateRequests state 3). port-disc-holds.tsv records every
 * held and stream read. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/lockstep_disc.h"
#include "p4pc/frame_alarm_seam.h"
#include "p4pc/lockstep_format.h"
#include "p4pc/platform.h"
#include "p4pc/vfs_origin.h"
#include "files/adxf_host.h"

#define DISC_NAME 64
#define DISC_HOLDS 32
#define DISC_SECTOR 2048
#define DISC_RECHECK_US (1000000u / 60u)
#define HCDVD_REQUESTS 0x008D1B84u

/* Held by ADXSTM_ExecServer's test-and-set for its whole pass (src/cri/re4/adx_stmc.c). */
extern int32_t adxstmf_execsvr_flg;

enum { DISC_FILE, DISC_CDVD };

typedef struct {
    int game, used; /* only game rows (retail H_Cdvd requests) are matched */
    char name[DISC_NAME];
    uint32_t offset, sectors;
    int32_t request, complete; /* anchor-relative retail frames; complete -1: never observed */
} DiscRow;

typedef struct {
    int used, boundary;
    int32_t id;
    uint32_t (*handler)(int32_t, void *);
    void *arg;
    uint32_t request, target;
    size_t row;
} DiscHold;

static DiscRow *sRows;
static size_t sRowCount;
static uint32_t sRetailAnchor, sHorizon, sAnchor;
static FILE *sReceipt;
static int sConfigured;
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
    if (fprintf(sReceipt, "%lld\tgame\t%s\t%u\t%u\t%u\t%d\t%d\t%u\t%u\t%s\n",
                (long long)hold->row, row->name, row->offset, row->sectors, hold->request,
                row->request, row->complete, frame, frame - hold->request, outcome) < 0 ||
        fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
}

/* ADX stream server reads complete at the normal frame boundary; only the request is recorded. */
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
 * request. A read issued while the ADXSTM server pass holds its test-and-set flag belongs to
 * the ADX stream server (BGM .ADX, voice .AFS). Anything else refuses the run. */
static int hcdvd_request(uint32_t handle) {
    uint32_t node = guest_word(HCDVD_REQUESTS);
    for (unsigned count = 0; node; count++) {
        if (count == 256) disc_fail("the H_Cdvd request list does not terminate");
        if (guest_word(node + 0x154u) == handle) return 1;
        node = guest_word(node + 0x4u);
    }
    return 0;
}

static void release(DiscHold *hold, int from_alarm) {
    const DiscHold held = *hold;
    hold->used = 0;
    receipt(&held, game_frame(), "released");
    if (!from_alarm) p4pc_cancel_alarm(held.id);
    held.handler(held.id, held.arg);
}

static uint32_t hold_alarm(int32_t id, void *arg) {
    DiscHold *hold = arg;
    (void)id;
    if (!hold->used) return 0;
    hold->boundary = 1;
    if ((int32_t)(game_frame() - hold->target) >= 0) {
        release(hold, 1);
        return 0;
    }
    return DISC_RECHECK_US; /* polls in the target frame release it; this covers callers that only wait */
}

static int32_t seam_set(uint32_t (*handler)(int32_t, void *), void *arg) {
    char why[384];
    const uint32_t frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    uint32_t offset, sectors;
    size_t index, slot;
    if (!sRead.valid) disc_fail("a disc completion has no preceding VFS read");
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
    if (sRead.family != DISC_FILE || !hcdvd_request(P4_H2G(arg))) {
        if (adxstmf_execsvr_flg) {
            stream_receipt(sRead.name, offset, sectors, frame);
            return p4pc_set_frame_boundary_alarm(handler, arg);
        }
        snprintf(why, sizeof why, "%s read %s offset %u sectors %u at port frame %u (anchor+%d) has neither "
                 "an H_Cdvd request node nor the ADX stream server as owner",
                 sRead.family == DISC_CDVD ? "raw sector" : "file", sRead.name[0] ? sRead.name : "-",
                 offset, sectors, frame, relative);
        disc_fail(why);
    }
    for (index = 0; index < sRowCount; index++) {
        const DiscRow *row = &sRows[index];
        if (!row->used && row->game && !strcmp(row->name, sRead.name) &&
            row->offset == offset && row->sectors == sectors)
            break;
    }
    if (index == sRowCount) {
        snprintf(why, sizeof why, "unmatched game read %s offset %u sectors %u at port frame %u "
                 "(anchor+%d; the retail trace ends at anchor+%d)", sRead.name, offset, sectors, frame,
                 relative, (int32_t)(sHorizon - sRetailAnchor));
        disc_fail(why);
    }
    if (sRows[index].request != relative || sRows[index].complete < 0) {
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
    sHolds[slot] = (DiscHold){1, 0, 0, handler, arg, frame, sAnchor + (uint32_t)sRows[index].complete, index};
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
    const uint32_t frame = game_frame();
    for (size_t slot = 0; slot < DISC_HOLDS; slot++)
        if (sHolds[slot].used && sHolds[slot].boundary && (int32_t)(frame - sHolds[slot].target) >= 0)
            release(&sHolds[slot], 0);
}

static const P4FrameAlarmSeam sSeam = {seam_set, seam_cancel, seam_poll};

int p4pc_lockstep_disc_open(const char *directory) {
    const char *source = getenv("P4PC_LOCKSTEP_DISC_REPLAY");
    char line[512], path[1200];
    FILE *table;
    if (!source || !*source) return 0;
    table = fopen(source, "r");
    if (!table) disc_fail("cannot open P4PC_LOCKSTEP_DISC_REPLAY");
    if (!fgets(line, sizeof line, table) ||
        sscanf(line, "# p4pc-disc-replay 1 retail_anchor %u horizon %u", &sRetailAnchor, &sHorizon) != 2)
        disc_fail("the table header is not p4pc-disc-replay 1");
    while (fgets(line, sizeof line, table)) {
        DiscRow row = {0};
        char family[16], name[128];
        DiscRow *grown;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%15s %127s %u %u %d %d", family, name, &row.offset, &row.sectors,
                   &row.request, &row.complete) != 6)
            disc_fail("malformed table row");
        if (!strcmp(family, "game")) row.game = 1;
        else if (strcmp(family, "stream") && strcmp(family, "substrate") && strcmp(family, "other"))
            disc_fail("unknown table row class");
        base_name(strcmp(name, "-") ? name : "", row.name);
        grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
        if (!grown) disc_fail("allocating the table");
        sRows = grown;
        sRows[sRowCount++] = row;
    }
    if (ferror(table) || fclose(table)) disc_fail("reading the table");
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
