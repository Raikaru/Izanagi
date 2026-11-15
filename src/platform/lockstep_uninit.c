/* Lockstep-only sync of words retail reads before it writes them (include/p4pc/lockstep_uninit.h,
 * docs/lockstep.md "Uninitialised words"). port-uninit.tsv receipts every word of every synced
 * list: the port's own value, retail's, and whether it was compared or copied. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/lockstep_format.h"
#include "p4pc/lockstep_online.h"
#include "p4pc/lockstep_uninit.h"

#define UNINIT_WORDS 1024u

static FILE *sReceipt, *sLog;
static int sOnline, sActive, sLogChecked;
static uint32_t sAnchor, sIndex, sLogged;

static void uninit_fail(const char *why) {
    fprintf(stderr, "p4pc: state-aligned lockstep: uninit replay: %s\n", why);
    exit(4);
}

static uint32_t game_frame(void) {
    return p4ls_get((const uint8_t *)P4_G2H(P4LS_GAME_FRAME));
}

int p4pc_lockstep_uninit_open(const char *directory) {
    char path[1200];
    sOnline = p4pc_online_configured();
    if (!sOnline) return 0;
    snprintf(path, sizeof path, "%s/port-uninit.tsv", directory);
    sReceipt = fopen(path, "w");
    if (!sReceipt || fprintf(sReceipt, "# p4pc-uninit-holds 1\nlist\tcreator\tframe\tretail_update\tcopies\t"
                             "word\tport\tretail\taction\n") < 0 || fflush(sReceipt))
        uninit_fail("writing port-uninit.tsv");
    return 1;
}

void p4pc_lockstep_uninit_activate(uint32_t anchor_frame) {
    if (!sOnline || sActive) return;
    sAnchor = anchor_frame;
    if (fprintf(sReceipt, "# anchor %u\n", anchor_frame) < 0 || fflush(sReceipt))
        uninit_fail("writing port-uninit.tsv");
    sActive = 1;
}

void p4pc_lockstep_thunder_list(uint32_t creator, void *list, uint32_t copies) {
    static uint32_t port[UNINIT_WORDS], retail[UNINIT_WORDS];
    const uint32_t frame = game_frame(), count = p4lu_words(creator, copies);
    char why[512];
    uint8_t *bytes = list;
    if (!sLogChecked) {
        const char *log = getenv("P4PC_LOCKSTEP_UNINIT_LOG");
        sLogChecked = 1;
        if (log && *log && (!(sLog = fopen(log, "w")) ||
                            fprintf(sLog, "# p4pc-uninit-log 1\nlist\tframe\tcreator\tcopies\twords\n") < 0))
            uninit_fail("cannot write P4PC_LOCKSTEP_UNINIT_LOG");
    }
    if (!list || (!sLog && !sActive)) return;
    if (!count || count > UNINIT_WORDS) uninit_fail("a thunder bolt list has an unknown creator or size");
    for (uint32_t i = 0; i < count; i++) port[i] = p4ls_get(bytes + i * 4u);
    if (sLog) {
        if (fprintf(sLog, "%u\t%u\t%08x\t%u\t", sLogged++, frame, creator, copies) < 0) uninit_fail("writing the log");
        for (uint32_t i = 0; i < count; i++)
            if (fprintf(sLog, i ? ",%08x" : "%08x", port[i]) < 0)
                uninit_fail("writing the log");
        if (fputc('\n', sLog) == EOF || fflush(sLog)) uninit_fail("writing the log");
    }
    if (!sActive) return;
    const int32_t relative = (int32_t)(frame - sAnchor);
    uint32_t retail_creator, retail_copies;
    int32_t update;
    const size_t words = p4pc_online_uninit(sIndex, &retail_creator, &update, &retail_copies, retail, UNINIT_WORDS);
    if (retail_creator != creator || retail_copies != copies || update != relative || words != count) {
        snprintf(why, sizeof why, "thunder bolt list %u from %08x with %u copies at port frame %u (anchor+%d) is "
                 "retail's list from %08x with %u copies at anchor+%d", sIndex, creator, copies, frame, relative,
                 retail_creator, retail_copies, update);
        uninit_fail(why);
    }
    for (uint32_t i = 0; i < count; i++) {
        if (p4lu_word_class(creator, i) == 2 && port[i] != retail[i]) {
            snprintf(why, sizeof why, "thunder bolt list %u word %u, written by its creator, is %08x at port frame %u "
                     "(anchor+%d) and %08x on retail", sIndex, i, port[i], frame, relative, retail[i]);
            uninit_fail(why);
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        const int kind = p4lu_word_class(creator, i);
        const char *action = kind == 1 ? "pointer" : kind == 2 ? "equal" : "copied";
        if (kind == 0) p4ls_put(bytes + i * 4u, retail[i]);
        if (fprintf(sReceipt, "%u\t%08x\t%u\t%d\t%u\t%u\t%08x\t%08x\t%s\n", sIndex, creator, frame, update, copies, i,
                    kind == 1 ? 0u : port[i], kind == 1 ? 0u : retail[i], action) < 0)
            uninit_fail("writing port-uninit.tsv");
    }
    if (fflush(sReceipt)) uninit_fail("writing port-uninit.tsv");
    sIndex++;
}
