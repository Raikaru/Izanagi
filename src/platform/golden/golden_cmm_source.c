/* Pinned English Social Link rank-up table from the owner's archive. */
#include "p4pc/golden_cmm.h"
#include "p4pc/golden_source.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char supported[] = "b986579f82a66280d5fbbe05873c7ad617e962b85edac3617a5bbff19e815983";
const char *p4_golden_cmm_rankup_source_sha256(void) { return supported; }

int p4_golden_cmm_rankup_load(const char *directory, const P4GoldenDomainMap *map,
                               uint8_t out[480], char *reason, size_t capacity)
{
    P4GoldenCpk cpk = {0};
    P4GoldenCpkEntry entry;
    const uint8_t *cmm, *dat;
    uint8_t *bin = NULL;
    size_t bin_size, cmm_size, dat_size;
    char path[1100], digest[65], scratch[160];
    char *why = reason && capacity ? reason : scratch;
    size_t room = reason && capacity ? capacity : sizeof scratch;
    int result = -1;
    if (!directory || !map || !out ||
        (size_t)snprintf(path, sizeof path, "%s/data_e.cpk", directory) >= sizeof path) {
        snprintf(why, room, "invalid rank-up source path or output");
        return -1;
    }
    if (p4_golden_cpk_open(&cpk, path, why, room)) return -1;
    if (p4_golden_cpk_find(&cpk, "init.bin", &entry) ||
        p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, why, room)) {
        snprintf(why, room, "cannot read data_e.cpk::init.bin");
        goto done;
    }
    if (p4_golden_bin_find(bin, bin_size, "event/cmm.bin", &cmm, &cmm_size) ||
        p4_golden_pak2_find(cmm, cmm_size, "cmmRankUpSkillTable.dat", &dat, &dat_size)) {
        snprintf(why, room, "cannot read event/cmm.bin::cmmRankUpSkillTable.dat");
        goto done;
    }
    p4_content_digest(dat, dat_size, digest);
    if (strcmp(digest, supported)) {
        snprintf(why, room, "unsupported cmmRankUpSkillTable.dat digest");
        goto done;
    }
    result = p4_golden_cmm_rankup_convert(map, dat, dat_size, out, why, room);
done:
    free(bin);
    p4_golden_cpk_close(&cpk);
    return result;
}
