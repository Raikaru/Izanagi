/* Synthetic fusion FTDs, no game bytes. */
#include "p4pc/golden_fusion_convert.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); return 1; } } while (0)
static void put32(uint8_t *p, unsigned n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24); }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    const unsigned count[] = {9, 14, 26, 28}, stride[] = {2, 38, 8, 28};
    P4GoldenDomainMap map;
    uint8_t in[800], out[784];
    char reason[128] = "";
    size_t written;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    for (unsigned k = 0; k < 4; ++k) {
        unsigned n = count[k] * stride[k], total = (16 + n + 31) & ~31u;
        memset(in, 0, sizeof in); put32(in, n); put32(in + 4, count[k]);
        CHECK(!p4_golden_fusion_convert(&map, (P4GoldenFusionKind)k, in, total, out, sizeof out, &written, reason, sizeof reason));
        CHECK(written == n);
        if (k == 2) {
            /* Fictional gate: an arbitrary rule targets Persona 7. */
            in[20] = 7;
            CHECK(!p4_golden_fusion_convert(&map, (P4GoldenFusionKind)k, in, total, out, sizeof out, &written, reason, sizeof reason));
            CHECK(out[4] == 7 && out[5] == 0);
            in[20] = 0; in[21] = 1;
            CHECK(p4_golden_fusion_convert(&map, (P4GoldenFusionKind)k, in, total, out, sizeof out, &written, reason, sizeof reason) == -1);
        }
        if (k == 3) {
            /* Fictional chart: arcana 1 combined with arcana 2 yields 3. */
            in[16 + 28 + 2] = 3;
            CHECK(!p4_golden_fusion_convert(&map, (P4GoldenFusionKind)k, in, total, out, sizeof out, &written, reason, sizeof reason));
            CHECK(out[28 + 2] == 3);
            in[16] = 28;
            CHECK(p4_golden_fusion_convert(&map, (P4GoldenFusionKind)k, in, total, out, sizeof out, &written, reason, sizeof reason) == -1);
        }
    }
    puts("golden_fusion_convert: ok");
    return 0;
}
