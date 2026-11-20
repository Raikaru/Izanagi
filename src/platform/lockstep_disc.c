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
#include "../movie/mwply.h"
#include "p4pc/lockstep_filer_replay.h"

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
    int pre;               /* a pre-anchor game read (row indexes sPre): held for retail's recorded duration */
} DiscHold;

/* A game-level read in the retail prefix before the initial anchor: only its identity and
 * request-to-completion duration are environment input (NG-7h). */
typedef struct {
    uint32_t sequence, offset, sectors, duration;
    char name[DISC_NAME];
} DiscPreRead;

typedef struct {
    uint32_t sequence, event[3], duration;
} DiscEventRow;

static DiscRow *sRows;
static size_t sRowCount;
static DiscEventRow *sEvents;
static size_t sEventCount, sEventNext, sEventCapacity;
static const void *sEventWork;
static uint32_t sEventStart;
int p4pc_lockstep_disc_event_enabled;
static int sPreAnchorOnly;
static uint32_t sEventActualReady;
static FILE *sEventReceipt;
static DiscPreRead *sPre;
static size_t sPreCount, sPreNext, sPreCapacity;
static FILE *sPreReceipt;
static int sActivated, sAnchored, sObserving;
/* Before the initial anchor (NG-7j) retail's disc latency, event load time and movie schedule follow retail's own
 * frame history, so nothing recorded reproduces them. With the online channel (include/p4pc/lockstep_online.h) the
 * port completes each of these waits exactly when the live retail run does; a table (when given) is only the
 * identity and order contract: file, offset, sectors for reads, event IDs for loads, file name for movies. */
typedef struct {
    uint32_t sequence;
    char name[DISC_NAME];
} DiscMovie;
static DiscMovie *sMovies;
static size_t sMovieCount, sMovieNext, sMovieCapacity;
static int sTable, sPreOnline, sPreActive;
typedef struct {
    uint32_t offset, sectors, request;
    char name[DISC_NAME];
} DiscPreRec;
static DiscPreRec *sPreRecs;
static size_t sPreRecCount, sPreRecCapacity;
static int sMovieCurrent = -1;
static FILE *sMovieReceipt, *sMoviePolls;
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

void p4pc_lockstep_disc_event_request(const void *work) {
    const uint8_t *bytes = work;
    uint32_t live[3], id[3];
    int32_t retail_request;
    char why[384];
    if (!p4pc_lockstep_disc_event_enabled) return;
    if (!bytes || sEventWork) disc_fail("overlapping or invalid pre-anchor event request");
    id[0] = p4ls_get(bytes + 0x78c); id[1] = p4ls_get(bytes + 0x790); id[2] = p4ls_get(bytes + 0x794);
    if (sTable) {
        const DiscEventRow *row = sEventNext < sEventCount ? &sEvents[sEventNext] : NULL;
        if (!row) disc_fail("missing pre-anchor event request record");
        if (row->sequence != sEventNext || id[0] != row->event[0] || id[1] != row->event[1] || id[2] != row->event[2])
            disc_fail("pre-anchor event request sequence or file ID differs");
    }
    if (!sPreActive) {   /* before the pairing begins: counted and checked against the table only */
        sEventNext++;
        return;
    }
    p4pc_online_pre_event(sEventNext, live, &retail_request);
    if (live[0] != id[0] || live[1] != id[1] || live[2] != id[2]) {
        snprintf(why, sizeof why, "pre-anchor event load %llu is %u/%u/%u at port frame %u, retail's is %u/%u/%u",
                 (unsigned long long)sEventNext, id[0], id[1], id[2], game_frame(), live[0], live[1], live[2]);
        disc_fail(why);
    }
    sEventWork = work;
    sEventStart = game_frame();
    sEventActualReady = UINT32_MAX;
}

/* evtLoaderIsIdle: answered with the live retail run's own answer to its same poll of this request. */
int p4pc_lockstep_disc_event_idle(const void *work, int actual_idle) {
    int32_t relative;
    int complete;
    char why[384];
    if (!p4pc_lockstep_disc_event_enabled || !sPreActive) return actual_idle;
    if (!sEventWork || sEventWork != work) disc_fail("event idle poll lacks its matching request");
    const uint32_t frame = game_frame(), elapsed = frame - sEventStart;
    if (actual_idle && sEventActualReady == UINT32_MAX) sEventActualReady = frame;
    p4pc_online_pre_event_poll(sEventNext, &relative, &complete);
    if (relative != (int32_t)elapsed) {
        snprintf(why, sizeof why, "pre-anchor event load %llu is polled at port update +%u, where retail's poll is at +%d",
                 (unsigned long long)sEventNext, elapsed, relative);
        disc_fail(why);
    }
    if (!complete) return 0;
    if (!actual_idle) disc_fail("retail's event loader completed, the port's loader is not idle");
    if (!sEventReceipt ||
        fprintf(sEventReceipt, "%llu\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n", (unsigned long long)sEventNext,
                p4ls_get((const uint8_t *)work + 0x78c), p4ls_get((const uint8_t *)work + 0x790),
                p4ls_get((const uint8_t *)work + 0x794), sEventStart, sEventActualReady, frame, elapsed,
                frame - sEventActualReady) < 0 || fflush(sEventReceipt))
        disc_fail("writing pre-anchor event load receipt");
    sEventWork = NULL;
    sEventNext++;
    return 1;
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
    if (!origin || origin->kind != P4_VFS_DISC || disc_offset < 0) {
        /* Before the anchor only game-level reads are replayed; a host or mod read is not one. */
        if (!sAnchored) { sRead.valid = 0; return; }
        disc_fail("a read after the anchor comes from a host or mod override");
    }
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
    const DiscRow *row;
    if (hold->pre) {
        const DiscPreRec *pre = &sPreRecs[hold->row];
        if (!sPreReceipt ||
            fprintf(sPreReceipt, "%llu\t%s\t%u\t%u\t%u\t%u\t%u\t%s\n", (unsigned long long)hold->online_index, pre->name,
                    pre->offset, pre->sectors, hold->request, frame, frame - hold->request, outcome) < 0 ||
            fflush(sPreReceipt))
            disc_fail("writing pre-anchor game read receipt");
        return;
    }
    row = &sRows[hold->row];
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

/* A movie's start, GetCurFrm answers and status reads follow the live retail run (src/movie/mwply.c). The id is
 * the movie's index plus one. */
static int movie_replay_start(const char *path) {
    char name[DISC_NAME], live[P4ON_NAME], upper[DISC_NAME], why[384];
    int32_t retail_request;
    if (sActivated || !sMovieReceipt) return 0;
    base_name(path, name);
    if (sTable) {
        if (sMovieNext >= sMovieCount) {
            snprintf(why, sizeof why, "pre-anchor movie %s at port frame %u has no recorded retail movie (the table has %llu)",
                     name, game_frame(), (unsigned long long)sMovieCount);
            disc_fail(why);
        }
        if (strcmp(sMovies[sMovieNext].name, name)) {
            snprintf(why, sizeof why, "pre-anchor movie %u is %s at port frame %u, the table's is %s",
                     sMovies[sMovieNext].sequence, name, game_frame(), sMovies[sMovieNext].name);
            disc_fail(why);
        }
    }
    if (!sPreActive) {   /* before the pairing begins: counted and checked against the table only */
        sMovieNext++;
        return 0;
    }
    p4pc_online_pre_movie(sMovieNext, live, &retail_request);
    base_name(live, upper);
    if (strcmp(upper, name)) {
        snprintf(why, sizeof why, "pre-anchor movie %llu is %s at port frame %u, retail's is %s",
                 (unsigned long long)sMovieNext, name, game_frame(), upper);
        disc_fail(why);
    }
    if (fprintf(sMovieReceipt, "%llu\t%s\t%u\t%d\n", (unsigned long long)sMovieNext, name, game_frame(), retail_request) < 0 ||
        fflush(sMovieReceipt) || (sMoviePolls && fprintf(sMoviePolls, "start\t%u\t0\t0\t-1\n", game_frame()) < 0))
        disc_fail("writing pre-anchor movie receipt");
    return (int)++sMovieNext;
}

static int32_t movie_replay_poll(int id) { return p4pc_online_pre_movie_poll((uint32_t)id - 1u); }

static int32_t movie_replay_status(int id) { return p4pc_online_pre_movie_status((uint32_t)id - 1u); }

/* Evidence, and the exactness check: the frame the port delivered must be the frame retail delivered. */
static void movie_replay_observe_poll(uint32_t calls, int32_t stat, int32_t retail, int32_t delivered) {
    if (delivered != retail) {
        char why[256];
        snprintf(why, sizeof why, "pre-anchor movie poll %u delivered frame %d where retail delivered %d", calls, delivered, retail);
        disc_fail(why);
    }
    if (sMovieReceipt && sMoviePolls &&
        fprintf(sMoviePolls, "poll\t%u\t%u\t%d\t%d\n", game_frame(), calls, stat, delivered) < 0)
        disc_fail("writing pre-anchor movie poll evidence");
}

static void movie_replay_observe_status(uint32_t calls, int32_t stat) {
    if (sMovieReceipt && sMoviePolls &&
        fprintf(sMoviePolls, "status\t%u\t%u\t%d\t-1\n", game_frame(), calls, stat) < 0)
        disc_fail("writing pre-anchor movie status evidence");
}

static const P4MovieReplay sMovieReplay = {movie_replay_start, movie_replay_poll, movie_replay_status,
                                           movie_replay_observe_poll, movie_replay_observe_status};

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

/* Before the initial anchor: a game-level read (an H_Cdvd request node's or the ADXF partition
 * loader's handle) takes the next recorded retail read, whose identity must match exactly, and
 * completes retail's recorded number of updates after its request. Everything else (ADX stream
 * server, raw sector and substrate reads, host or mod files) completes at the normal boundary. */
static int32_t pre_seam_set(uint32_t (*handler)(int32_t, void *), void *arg) {
    char why[384], name[DISC_NAME], live_name[P4ON_NAME];
    const uint32_t frame = game_frame();
    const P4AdxfHandle *file = arg;
    P4VfsOrigin origin;
    int64_t before;
    uint32_t offset, sectors, live_offset, live_sectors;
    int32_t retail_request;
    size_t slot;
    if (!sRead.valid || sRead.family != DISC_FILE || file == NULL || vfs_origin(file->fd, &origin) ||
        !origin.key || !(hcdvd_request(P4_H2G(arg)) || partition_request(P4_H2G(arg)))) {
        sRead.valid = 0;
        return p4pc_set_frame_boundary_alarm(handler, arg);
    }
    base_name(origin.key, name);
    before = sRead.first - file->base;
    if (strcmp(name, sRead.name) || before < 0 || before % DISC_SECTOR || file->pos <= before ||
        (file->pos - before) % DISC_SECTOR ||
        sRead.bytes != (file->pos - before < file->size - before ? file->pos - before : file->size - before))
        disc_fail("the last observed VFS read is not exactly this game handle's read");
    offset = (uint32_t)(before / DISC_SECTOR);
    sectors = (uint32_t)((file->pos - before) / DISC_SECTOR);
    sRead.valid = 0;
    if (sTable) {
        const DiscPreRead *row = sPreNext < sPreCount ? &sPre[sPreNext] : NULL;
        if (!row) {
            snprintf(why, sizeof why, "pre-anchor game read %s offset %u sectors %u at port frame %u has no recorded "
                     "retail read (the table has %llu)", name, offset, sectors, frame, (unsigned long long)sPreCount);
            disc_fail(why);
        }
        if (strcmp(row->name, name) || row->offset != offset || row->sectors != sectors) {
            snprintf(why, sizeof why, "pre-anchor game read %u is %s offset %u sectors %u at port frame %u, the table's is "
                     "%s offset %u sectors %u", row->sequence, name, offset, sectors, frame, row->name, row->offset,
                     row->sectors);
            disc_fail(why);
        }
    }
    if (!sPreActive) {   /* before the pairing begins: counted and checked against the table only */
        sPreNext++;
        return p4pc_set_frame_boundary_alarm(handler, arg);
    }
    p4pc_online_pre_read(sPreNext, live_name, &live_offset, &live_sectors, &retail_request);
    {
        char upper[DISC_NAME];
        base_name(live_name, upper);
        if (strcmp(upper, name) || live_offset != offset || live_sectors != sectors) {
            snprintf(why, sizeof why, "pre-anchor game read %llu is %s offset %u sectors %u at port frame %u, retail's is "
                     "%s offset %u sectors %u", (unsigned long long)sPreNext, name, offset, sectors, frame, upper,
                     live_offset, live_sectors);
            disc_fail(why);
        }
    }
    if (sPreRecCount == sPreRecCapacity) {
        const size_t capacity = sPreRecCapacity ? sPreRecCapacity * 2 : 128;
        DiscPreRec *grown = realloc(sPreRecs, capacity * sizeof *sPreRecs);
        if (!grown) disc_fail("allocating pre-anchor read records");
        sPreRecs = grown; sPreRecCapacity = capacity;
    }
    sPreRecs[sPreRecCount] = (DiscPreRec){offset, sectors, frame, {0}};
    memcpy(sPreRecs[sPreRecCount].name, name, sizeof name);
    for (slot = 0; slot < DISC_HOLDS && sHolds[slot].used; slot++) {
    }
    if (slot == DISC_HOLDS) disc_fail("more than 32 disc reads held at once");
    sHolds[slot] = (DiscHold){1, 0, 0, handler, arg, frame, 0, 0, sPreRecCount++, 1, (uint32_t)sPreNext, 1};
    sPreNext++;
    sHolds[slot].id = p4pc_set_frame_boundary_alarm(hold_alarm, &sHolds[slot]);
    if (sHolds[slot].id < 0) disc_fail("no kernel alarm for a held pre-anchor read");
    return sHolds[slot].id;
}

static int32_t seam_set(uint32_t (*handler)(int32_t, void *), void *arg) {
    char why[384];
    const uint32_t frame = game_frame();
    if (!sAnchored) return pre_seam_set(handler, arg);
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
    sHolds[slot] = (DiscHold){1, 0, 0, handler, arg, frame, sAnchor + (uint32_t)sRows[index].complete, 0, index, 0, 0, 0};
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
            if (held.pre) disc_fail("a recorded pre-anchor retail read was stopped or closed by the port");
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
        if (hold->pre) {
            p4pc_online_pre_read_poll(hold->online_index, &update, &complete);
            if (update != (int32_t)(frame - hold->request)) {
                snprintf(why, sizeof why, "pre-anchor game read %s polled at port update +%u, where retail's poll of it is "
                         "at +%d", sPreRecs[hold->row].name, frame - hold->request, update);
                disc_fail(why);
            }
            if (complete) release(hold, 0);
            return;
        }
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
        goto pre_online; /* online without a table: no identity contract, stream reads are not held */
    }
    sTable = 1;
    table = fopen(source, "r");
    if (!table) disc_fail("cannot open P4PC_LOCKSTEP_DISC_REPLAY");
    if (!fgets(line, sizeof line, table) ||
        sscanf(line, "# p4pc-disc-replay 5 retail_anchor %u horizon %u pre_anchor_only %d",
               &sRetailAnchor, &sHorizon, &sPreAnchorOnly) != 3 ||
        (sPreAnchorOnly != 0 && sPreAnchorOnly != 1))
        disc_fail("the table header is not p4pc-disc-replay 5");
    while (fgets(line, sizeof line, table)) {
        DiscRow row = {0};
        char family[16], name[128];
        DiscRow *grown;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (!strncmp(line, "event\t", 6)) {
            DiscEventRow event, *events;
            if (sscanf(line, "event %u %u %u %u %u", &event.sequence, &event.event[0],
                       &event.event[1], &event.event[2], &event.duration) != 5 ||
                event.sequence != sEventCount || event.duration > sRetailAnchor)
                disc_fail("malformed or reordered pre-anchor event row");
            if (sEventCount==sEventCapacity) {
                const size_t capacity=sEventCapacity ? sEventCapacity*2 : 16;
                if (capacity<sEventCapacity || capacity>SIZE_MAX/sizeof *sEvents)
                    disc_fail("pre-anchor event table is too large");
                events = realloc(sEvents, capacity * sizeof *sEvents);
                if (!events) disc_fail("allocating pre-anchor event table");
                sEvents = events; sEventCapacity = capacity;
            }
            sEvents[sEventCount++] = event;
            continue;
        }
        if (!strncmp(line, "movie\t", 6)) {
            DiscMovie movie = {0}, *grown_movie;
            char movie_name[128];
            if (sscanf(line, "movie %u %127s", &movie.sequence, movie_name) != 2 ||
                movie.sequence != sMovieCount || !strcmp(movie_name, "-"))
                disc_fail("malformed or reordered pre-anchor movie row");
            base_name(movie_name, movie.name);
            if (sMovieCount == sMovieCapacity) {
                const size_t capacity = sMovieCapacity ? sMovieCapacity * 2 : 8;
                if (capacity < sMovieCapacity || capacity > SIZE_MAX / sizeof *sMovies)
                    disc_fail("pre-anchor movie table is too large");
                grown_movie = realloc(sMovies, capacity * sizeof *sMovies);
                if (!grown_movie) disc_fail("allocating pre-anchor movie table");
                sMovies = grown_movie; sMovieCapacity = capacity;
            }
            sMovies[sMovieCount++] = movie;
            continue;
        }
        if (!strncmp(line, "pre\t", 4)) {
            DiscPreRead pre = {0}, *grown_pre;
            char pre_name[128];
            if (sscanf(line, "pre %u %127s %u %u %u", &pre.sequence, pre_name, &pre.offset, &pre.sectors,
                       &pre.duration) != 5 || pre.sequence != sPreCount || pre.duration > sRetailAnchor ||
                !strcmp(pre_name, "-"))
                disc_fail("malformed or reordered pre-anchor game read row");
            base_name(pre_name, pre.name);
            if (sPreCount == sPreCapacity) {
                const size_t capacity = sPreCapacity ? sPreCapacity * 2 : 64;
                if (capacity < sPreCapacity || capacity > SIZE_MAX / sizeof *sPre)
                    disc_fail("pre-anchor game read table is too large");
                grown_pre = realloc(sPre, capacity * sizeof *sPre);
                if (!grown_pre) disc_fail("allocating pre-anchor game read table");
                sPre = grown_pre; sPreCapacity = capacity;
            }
            sPre[sPreCount++] = pre;
            continue;
        }
        if (sPreAnchorOnly) disc_fail("pre-anchor-only table contains a post-anchor read");
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
pre_online:
    if (!sOnline) goto receipt; /* a table alone replays nothing before the anchor */
    sPreOnline = 1;
    p4pc_lockstep_disc_event_enabled = 1;
    snprintf(path, sizeof path, "%s/port-event-loads.tsv", directory);
    sEventReceipt = fopen(path, "w");
    if (!sEventReceipt ||
        fprintf(sEventReceipt, "sequence\tevent0\tevent1\tevent2\trequest_frame\tactual_ready_frame\t"
                               "release_frame\tretail_duration\theld_frames\n") < 0 ||
        fflush(sEventReceipt)) disc_fail("writing pre-anchor event receipt header");
    snprintf(path, sizeof path, "%s/port-game-reads.tsv", directory);
    sPreReceipt = fopen(path, "w");
    if (!sPreReceipt ||
        fprintf(sPreReceipt, "sequence\tfile\toffset\tsectors\trequest_frame\trelease_frame\tretail_duration\t"
                             "outcome\n") < 0 || fflush(sPreReceipt))
        disc_fail("writing pre-anchor game read receipt header");
    snprintf(path, sizeof path, "%s/port-movies.tsv", directory);
    sMovieReceipt = fopen(path, "w");
    if (!sMovieReceipt ||
        fprintf(sMovieReceipt, "sequence\tfile\tstart_frame\tretail_start_frame\n") < 0 ||
        fflush(sMovieReceipt))
        disc_fail("writing pre-anchor movie receipt header");
    snprintf(path, sizeof path, "%s/port-movie-polls.tsv", directory);
    sMoviePolls = fopen(path, "w");
    if (!sMoviePolls || fprintf(sMoviePolls, "event\tframe\tcalls\tvalue\tnumber\n") < 0 || fflush(sMoviePolls))
        disc_fail("writing pre-anchor movie poll evidence header");
    p4pc_movie_replay = &sMovieReplay;
    if (vfs_read_observer_register(observe_read, NULL) != 0)
        disc_fail("the VFS read observer is already in use");
    sObserving = 1;
    p4pc_frame_alarm_seam = &sSeam;
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

void p4pc_lockstep_disc_pre_begin(void) {
    if (sPreOnline && !sActivated) sPreActive = 1;
    p4pc_lockstep_filer_pre_begin();
}

void p4pc_lockstep_disc_activate(uint32_t anchor_frame) {
    if (!sConfigured || sActivated) return;
    if (p4pc_lockstep_disc_event_enabled) {
        if (sEventWork || (sTable && sEventNext != sEventCount)) disc_fail("pre-anchor event records were not consumed");
        p4pc_lockstep_disc_event_enabled = 0;
        if (fclose(sEventReceipt)) disc_fail("closing pre-anchor event receipt");
        sEventReceipt = NULL;
    }
    if (sPreReceipt) {
        if (sTable && sPreNext != sPreCount) disc_fail("pre-anchor game read records were not consumed");
        for (size_t slot = 0; slot < DISC_HOLDS; slot++)
            if (sHolds[slot].used && sHolds[slot].pre) disc_fail("a pre-anchor game read is still held at the anchor");
        if (fclose(sPreReceipt)) disc_fail("closing pre-anchor game read receipt");
        sPreReceipt = NULL;
    }
    if (sMovieReceipt) {
        if (sTable && sMovieNext != sMovieCount) disc_fail("pre-anchor movie records were not consumed");
        if (fclose(sMovieReceipt)) disc_fail("closing pre-anchor movie receipt");
        sMovieReceipt = NULL;
        if (sMoviePolls && fclose(sMoviePolls)) disc_fail("closing pre-anchor movie poll evidence");
        sMoviePolls = NULL;
        p4pc_movie_replay = NULL;
    }
    sActivated = 1;
    /* A table that ends at the anchor (its source trace did not run on past it) replays nothing after it; the online
     * channel is the live retail trace and does: its n-th post-anchor read is retail's n-th (NG-7m). */
    if (sPreAnchorOnly && !sOnline) {
        if (sObserving) { vfs_read_observer_remove(observe_read, NULL); sObserving = 0; }
        if (p4pc_frame_alarm_seam == &sSeam) p4pc_frame_alarm_seam = NULL;
        return;
    }
    sAnchor = anchor_frame;
    sAnchored = 1;
    if (!sObserving) {
        if (vfs_read_observer_register(observe_read, NULL) != 0)
            disc_fail("the VFS read observer is already in use");
        sObserving = 1;
    }
    if (fprintf(sReceipt, "# anchor %u\n", anchor_frame) < 0 || fflush(sReceipt))
        disc_fail("writing port-disc-holds.tsv");
    p4pc_frame_alarm_seam = &sSeam;
}
