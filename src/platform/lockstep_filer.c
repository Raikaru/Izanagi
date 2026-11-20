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
    int used, pre;
    uint32_t index; /* retail's request index: since the start of the run (pre-anchor) or since the anchor (online) */
} FilerHold;

int p4pc_filer_replay_enabled;
static FilerRow *sRows;
static size_t sRowCount;
static uint32_t sRetailAnchor, sHorizon, sAnchor;
static FILE *sReceipt;
static int sConfigured, sOnline;
static uint32_t sOnlineIndex;
/* Before the anchor (NG-7j): from the pairing's begin each FilerRequest is retail's n-th, taken as retail takes it. */
static int sPreOn, sPreActive, sAnchored;
static uint32_t sPreNext;
static FILE *sPreReceipt, *sPreTakes;
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

static void pre_enqueue(uint32_t address, const char *key, uint32_t frame) {
    char retail[P4LF_KEY], why[P4LF_KEY + 256];
    int32_t enqueue;
    FilerRow *grown;
    size_t slot = FILER_HOLDS;
    if (!sPreActive) {   /* booted through: counted only */
        sPreNext++;
        return;
    }
    p4pc_online_pre_filer(sPreNext, retail, &enqueue);
    if (strcmp(retail, key)) {
        snprintf(why, sizeof why, "pre-anchor filer request %u is %s at port frame %u, retail's is %s", sPreNext, key,
                 frame, retail);
        filer_fail(why);
    }
    for (size_t i = 0; i < FILER_HOLDS; i++) {
        if (sHolds[i].used && sHolds[i].request == address)
            filer_fail("a filer request slot was queued twice without a take");
        if (!sHolds[i].used && slot == FILER_HOLDS) slot = i;
    }
    if (slot == FILER_HOLDS) filer_fail("more filer requests in flight than the request pool");
    grown = realloc(sRows, (sRowCount + 1) * sizeof *sRows);
    if (!grown || !(grown[sRowCount].key = malloc(strlen(key) + 1))) filer_fail("allocating the rows");
    sRows = grown;
    strcpy(sRows[sRowCount].key, key);
    sRows[sRowCount].enqueue = enqueue;
    sRows[sRowCount].complete = -1;
    sRows[sRowCount].used = 1;
    sHolds[slot] = (FilerHold){address, frame, sRowCount++, 1, 1, sPreNext++};
}

static int pre_take(uint32_t address, uint32_t frame) {
    char why[P4LF_KEY + 256];
    int32_t relative;
    int complete;
    size_t slot;
    for (slot = 0; slot < FILER_HOLDS; slot++)
        if (sHolds[slot].used && sHolds[slot].pre && sHolds[slot].request == address) break;
    if (slot == FILER_HOLDS) return 1;   /* queued before the pairing began: natural */
    FilerHold *hold = &sHolds[slot];
    /* Evidence, written before the wait so a stuck pairing shows which poll it is waiting for. */
    if (sPreTakes && (fprintf(sPreTakes, "take\t%u\t%u\t%u\n", hold->index, frame, frame - hold->enqueued) < 0 ||
                      fflush(sPreTakes)))
        filer_fail("writing port-pre-filer-takes.tsv");
    p4pc_online_pre_filer_take(hold->index, &relative, &complete);
    if (relative != (int32_t)(frame - hold->enqueued)) {
        snprintf(why, sizeof why, "pre-anchor filer request %s taken at port update +%u, where retail's take of it is "
                 "at +%d", sRows[hold->row].key, frame - hold->enqueued, relative);
        filer_fail(why);
    }
    if (!complete) return 0;
    if (!p4lf_complete(&sHost, address)) {
        snprintf(why, sizeof why, "pre-anchor filer request %s is not finished at port update +%u, where retail took "
                 "it complete", sRows[hold->row].key, frame - hold->enqueued);
        filer_fail(why);
    }
    sRows[hold->row].complete = (int32_t)(frame - hold->enqueued);
    if (!sPreReceipt || fprintf(sPreReceipt, "%u\t%s\t%u\t%u\t%d\n", hold->index, sRows[hold->row].key, hold->enqueued, frame,
                                sRows[hold->row].complete) < 0 || fflush(sPreReceipt))
        filer_fail("writing port-pre-filer.tsv");
    hold->used = 0;
    return 1;
}

void p4pc_filer_replay_enqueue(const void *request) {
    char key[P4LF_KEY], why[P4LF_KEY + 256];
    const uint32_t address = P4_H2G(request), frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    size_t index, slot;
    uint32_t online_ordinal = 0;
    if (!p4lf_key(&sHost, address, key)) filer_fail("a queued filer request lies outside guest memory");
    if (sPreOn && !sAnchored) {
        pre_enqueue(address, key, frame);
        return;
    }
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
        online_ordinal = sOnlineIndex++;
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
    sHolds[slot] = (FilerHold){address, frame, index, 1, 0, online_ordinal};
}

int p4pc_filer_replay_take(const void *request) {
    char key[P4LF_KEY], why[P4LF_KEY + 256];
    const uint32_t address = P4_H2G(request), frame = game_frame();
    const int32_t relative = (int32_t)(frame - sAnchor);
    size_t slot;
    if (sPreOn && !sAnchored) return pre_take(address, frame);
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
        /* retail's index, not the row: the rows also hold the pre-anchor requests */
        p4pc_online_filer_take(hold->index, &update, &complete);
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
    if (sOnline) {   /* the pre-anchor online pairing (NG-7j) */
        snprintf(path, sizeof path, "%s/port-pre-filer.tsv", directory);
        sPreReceipt = fopen(path, "w");
        if (!sPreReceipt || fprintf(sPreReceipt, "index\tkey\tenqueue_frame\trelease_frame\tupdates\n") < 0 ||
            fflush(sPreReceipt))
            filer_fail("writing port-pre-filer.tsv");
        snprintf(path, sizeof path, "%s/port-pre-filer-takes.tsv", directory);
        sPreTakes = fopen(path, "w");
        if (!sPreTakes || fprintf(sPreTakes, "event\tindex\tframe\tupdate\n") < 0 || fflush(sPreTakes))
            filer_fail("writing port-pre-filer-takes.tsv");
        sPreOn = 1;
        p4pc_filer_replay_enabled = 1;
    }
    return 1;
}

void p4pc_lockstep_filer_pre_begin(void) {
    if (sPreOn && !sAnchored) sPreActive = 1;
}

void p4pc_lockstep_filer_activate(uint32_t anchor_frame) {
    if (!sConfigured || sAnchored) return;
    for (size_t i = 0; i < FILER_HOLDS; i++)
        if (sHolds[i].used && sHolds[i].pre) filer_fail("a pre-anchor filer request is still held at the anchor");
    sAnchored = 1;
    sAnchor = anchor_frame;
    if (fprintf(sReceipt, "# anchor %u\n", anchor_frame) < 0 || fflush(sReceipt))
        filer_fail("writing port-filer-holds.tsv");
    p4pc_filer_replay_enabled = 1;
}
