/* Lockstep-only filer replay (include/p4pc/lockstep_filer_replay.h, docs/lockstep.md).
 *
 * The background loader thread (sdkWrapWorkerThread) finishes FilerRequests on CPU time that
 * the port spends differently from the EE, so a consumer polling filerTakeCompletedRequest
 * (FldResource_PollLoads, the model manager) could see a request complete on another update than
 * retail. Retail's p4hook records, in the lockstep disc trace, each request's enqueue and the
 * update of the take that found it complete; tools/lockstep_disc.py turns them into rows keyed
 * by include/p4pc/lockstep_filer.h. After the replay anchor every port enqueue takes the next
 * unused row with its key and must happen at the same anchor-relative update; its take reports
 * "not complete" until retail's completion update, and must find the port's request finished
 * there. Unmatched, mistimed, late or past-the-horizon requests fail the run. With
 * P4PC_LOCKSTEP_DISC_ONLINE (include/p4pc/lockstep_online.h) the rows come live from retail
 * instead: the port's n-th request after the anchor is retail's n-th, and each take is answered by
 * retail's take in the same update. Nothing changes unless one of them is set.
 * port-filer-holds.tsv receipts every request. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/lockstep_filer.h"
#include "p4pc/lockstep_filer_replay.h"
#include "p4pc/lockstep_format.h"
#include "p4pc/lockstep_online.h"

#define FILER_HOLDS 256 /* the request pool (filerEnqueueRequest, 256 slots) */

typedef struct {
    char *key;
    int32_t enqueue, complete; /* anchor-relative retail updates; complete -1: never observed */
    int used;
} FilerRow;

typedef struct {
    uint32_t request, enqueued;
    size_t row;
    int used;
} FilerHold;

int p4pc_filer_replay_enabled;
static FilerRow *sRows;
static size_t sRowCount;
static uint32_t sRetailAnchor, sHorizon, sAnchor;
static FILE *sReceipt;
static int sConfigured, sOnline;
static uint32_t sOnlineIndex;
static FilerHold sHolds[FILER_HOLDS];

static void filer_fail(const char *why) {
    fprintf(stderr, "p4pc: state-aligned lockstep: filer replay: %s\n", why);
    exit(4);
}

static uint32_t game_frame(void) {
    return p4ls_get((const uint8_t *)P4_G2H(P4LS_GAME_FRAME));
}

static uint32_t guest_read(void *user, uint32_t address) {
    (void)user;
    return p4ls_get((const uint8_t *)P4_G2H(address));
}

static const P4WalkHost sHost = {.read32 = guest_read};

static void receipt(const FilerHold *hold, uint32_t frame, const char *outcome) {
    const FilerRow *row = &sRows[hold->row];
    if (fprintf(sReceipt, "%llu\t%s\t%u\t%d\t%d\t%u\t%d\t%s\n", (unsigned long long)hold->row, row->key,
                hold->enqueued, row->enqueue, row->complete, frame, (int32_t)(frame - hold->enqueued), outcome) < 0 ||
        fflush(sReceipt))
        filer_fail("writing port-filer-holds.tsv");
}

void p4pc_filer_replay_enqueue(const void *request) {
    char key[P4LF_KEY], why[P4LF_KEY + 256];
    const uint32_t address = P4_H2G(request), frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    size_t index, slot;
    if (!p4lf_key(&sHost, address, key)) filer_fail("a queued filer request lies outside guest memory");
    if (sOnline) {
        char retail[P4LF_KEY];
        int32_t enqueue;
        FilerRow *grown;
        p4pc_online_filer(sOnlineIndex, retail, &enqueue);
        if (strcmp(retail, key) || enqueue != relative) {
            snprintf(why, sizeof why, "filer request %s queued at port frame %u (anchor+%d) is retail's request %u, "
                     "%s queued at anchor+%d", key, frame, relative, sOnlineIndex, retail, enqueue);
            filer_fail(why);
        }
        grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
        if (!grown || !(grown[sRowCount].key = malloc(strlen(key) + 1))) filer_fail("allocating the rows");
        sRows = grown;
        strcpy(sRows[sRowCount].key, key);
        sRows[sRowCount].enqueue = enqueue;
        sRows[sRowCount].complete = -1;
        sRows[sRowCount].used = 0;
        index = sRowCount++;
        sOnlineIndex++;
    } else if (relative > (int32_t)(sHorizon - sRetailAnchor)) {
        snprintf(why, sizeof why, "filer request %s queued at port frame %u (anchor+%d) is past the retail trace "
                 "(anchor+%d)", key, frame, relative, (int32_t)(sHorizon - sRetailAnchor));
        filer_fail(why);
    }
    if (!sOnline) {
        for (index = 0; index < sRowCount; index++)
            if (!sRows[index].used && !strcmp(sRows[index].key, key)) break;
        if (index == sRowCount) {
            snprintf(why, sizeof why, "unmatched filer request %s queued at port frame %u (anchor+%d)", key, frame,
                     relative);
            filer_fail(why);
        }
    }
    if (sRows[index].enqueue != relative) {
        snprintf(why, sizeof why, "filer request %s queued at port frame %u (anchor+%d) matches retail row %llu "
                 "queued at anchor+%d", key, frame, relative, (unsigned long long)index, sRows[index].enqueue);
        filer_fail(why);
    }
    slot = FILER_HOLDS;
    for (size_t i = 0; i < FILER_HOLDS; i++) {
        if (sHolds[i].used && sHolds[i].request == address)
            filer_fail("a filer request slot was queued twice without a take");
        if (!sHolds[i].used && slot == FILER_HOLDS) slot = i;
    }
    if (slot == FILER_HOLDS) filer_fail("more filer requests in flight than the request pool");
    sRows[index].used = 1;
    sHolds[slot] = (FilerHold){address, frame, index, 1};
}

int p4pc_filer_replay_take(const void *request) {
    char key[P4LF_KEY], why[P4LF_KEY + 256];
    const uint32_t address = P4_H2G(request), frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    size_t slot;
    for (slot = 0; slot < FILER_HOLDS; slot++)
        if (sHolds[slot].used && sHolds[slot].request == address) break;
    if (slot == FILER_HOLDS) {
        /* Queued before the anchor: retail's table refuses requests in flight across it, so this
         * one may be polled but never found complete. */
        if (p4lf_complete(&sHost, address)) {
            if (!p4lf_key(&sHost, address, key)) snprintf(key, sizeof key, "?");
            snprintf(why, sizeof why, "filer request %s queued before the replay anchor completes at port frame %u "
                     "(anchor+%d)", key, frame, relative);
            filer_fail(why);
        }
        return 1;
    }
    FilerHold *hold = &sHolds[slot];
    FilerRow *row = &sRows[hold->row];
    if (sOnline) {
        int32_t update;
        int complete;
        p4pc_online_filer_take((uint32_t)hold->row, &update, &complete);
        if (update != relative) {
            snprintf(why, sizeof why, "filer request %s taken at port frame %u (anchor+%d), where retail's take of "
                     "it is at anchor+%d", row->key, frame, relative, update);
            filer_fail(why);
        }
        if (!complete) return 0;
        row->complete = relative;
    }
    if (row->complete < 0 || relative < row->complete) {
        if (relative > (int32_t)(sHorizon - sRetailAnchor)) {
            snprintf(why, sizeof why, "held filer request %s reached the end of the retail trace at port frame %u "
                     "(anchor+%d)", row->key, frame, relative);
            filer_fail(why);
        }
        return 0;
    }
    if (relative > row->complete) {
        snprintf(why, sizeof why, "filer request %s first polled at port frame %u (anchor+%d), after retail took it "
                 "complete at anchor+%d", row->key, frame, relative, row->complete);
        filer_fail(why);
    }
    if (!p4lf_complete(&sHost, address)) {
        snprintf(why, sizeof why, "filer request %s is not finished at port frame %u (anchor+%d), where retail took "
                 "it complete", row->key, frame, relative);
        filer_fail(why);
    }
    receipt(hold, frame, "released");
    hold->used = 0;
    return 1;
}

int p4pc_lockstep_filer_open(const char *directory) {
    const char *source = getenv("P4PC_LOCKSTEP_FILER_REPLAY");
    char line[P4LF_KEY + 128], path[1200];
    FILE *table;
    sOnline = p4pc_online_configured();
    if (sOnline) goto receipt;
    if (!source || !*source) return 0;
    table = fopen(source, "r");
    if (!table) filer_fail("cannot open P4PC_LOCKSTEP_FILER_REPLAY");
    if (!fgets(line, sizeof line, table) || sscanf(line, P4LF_HEADER, &sRetailAnchor, &sHorizon) != 2)
        filer_fail("the table header is not p4pc-filer-replay 1");
    while (fgets(line, sizeof line, table)) {
        char key[P4LF_KEY];
        FilerRow row = {0}, *grown;
        unsigned long long order;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%1023s %d %d %llu", key, &row.enqueue, &row.complete, &order) != 4)
            filer_fail("malformed table row");
        row.key = malloc(strlen(key) + 1);
        grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
        if (!row.key || !grown) filer_fail("allocating the table");
        strcpy(row.key, key);
        sRows = grown;
        sRows[sRowCount++] = row;
    }
    if (ferror(table) || fclose(table)) filer_fail("reading the table");
receipt:
    snprintf(path, sizeof path, "%s/port-filer-holds.tsv", directory);
    sReceipt = fopen(path, "w");
    if (!sReceipt || fprintf(sReceipt, "# p4pc-filer-holds 1 retail_anchor %u horizon %u rows %llu\n"
                             "row\tkey\tenqueue_frame\tretail_enqueue\tretail_complete\trelease_frame\tupdates\toutcome\n",
                             sRetailAnchor, sHorizon, (unsigned long long)sRowCount) < 0 || fflush(sReceipt))
        filer_fail("writing port-filer-holds.tsv");
    sConfigured = 1;
    return 1;
}

void p4pc_lockstep_filer_activate(uint32_t anchor_frame) {
    if (!sConfigured || p4pc_filer_replay_enabled) return;
    sAnchor = anchor_frame;
    if (fprintf(sReceipt, "# anchor %u\n", anchor_frame) < 0 || fflush(sReceipt))
        filer_fail("writing port-filer-holds.tsv");
    p4pc_filer_replay_enabled = 1;
}
