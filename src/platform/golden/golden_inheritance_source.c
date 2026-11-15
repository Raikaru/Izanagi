/* Pinned Steam build's inheritance rates, read from the owner's archive. */
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_source.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char supported[] = "aa8dc493be29eeef2bef69c4ba8486a70e5e695d0d6f880311cd30f47b4ddeef";
const char *p4_golden_inheritance_source_sha256(void) { return supported; }

int p4_golden_inheritance_load(const char *directory, const P4GoldenDomainMap *map,
                              uint8_t *out, size_t capacity, size_t *written,
                              char *reason, size_t reason_capacity)
{
    P4GoldenCpk cpk = {0};
    P4GoldenCpkEntry entry;
    const uint8_t *fcl, *ftd;
    uint8_t *bin = NULL;
    size_t bin_size, fcl_size, ftd_size;
    char path[1100], digest[65], scratch[160];
    char *why = reason && reason_capacity ? reason : scratch;
    size_t room = reason && reason_capacity ? reason_capacity : sizeof scratch;
    int result = -1;
    if (!directory || !map || !out || !written ||
        (size_t)snprintf(path, sizeof path, "%s/data_e.cpk", directory) >= sizeof path) {
        snprintf(why, room, "invalid inheritance source path or output");
        return -1;
    }
    if (p4_golden_cpk_open(&cpk, path, why, room)) return -1;
    if (p4_golden_cpk_find(&cpk, "init_free.bin", &entry) ||
        p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, why, room)) {
        snprintf(why, room, "cannot read data_e.cpk::init_free.bin");
        goto done;
    }
    if (p4_golden_bin_find(bin, bin_size, "init/fclTable.bin", &fcl, &fcl_size) ||
        p4_golden_pak2_find(fcl, fcl_size, "fclCombineSkillTargetLv.ftd", &ftd, &ftd_size)) {
        snprintf(why, room, "cannot read init/fclTable.bin::fclCombineSkillTargetLv.ftd");
        goto done;
    }
    p4_content_digest(ftd, ftd_size, digest);
    if (strcmp(digest, supported)) {
        snprintf(why, room, "unsupported fclCombineSkillTargetLv.ftd digest");
        goto done;
    }
    result = p4_golden_inheritance_convert(map, ftd, ftd_size, out, capacity, written, why, room);
done:
    free(bin);
    p4_golden_cpk_close(&cpk);
    return result;
}
