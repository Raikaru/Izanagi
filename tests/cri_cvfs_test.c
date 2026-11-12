/* cvFsDelDev (src/cri/cvfs_port.c, retail 0x004e1a00): the table scan over memory mapped at the device
 * table 0x00923440. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/cri/cvfs_port.c"
#include "guest_test.h"

static int failures;
static const char *last_msg;

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

void cvFsCallUsrErrFn(void *obj, const char *msg, void *hn)
{
    CHECK(obj == 0 && hn == 0);
    last_msg = msg;
}

int main(void)
{
    uint8_t *tbl;

    if (p4t_guest_map(0x00923000, 0x1000) != 0) {
        printf("cri_cvfs: cannot map the device table\n");
        return 77;
    }
    tbl = (uint8_t *)P4_G2H(0x00923440);
    memset(tbl, 0, 0x200);
    strcpy((char *)tbl + 4, "ROFS");
    strcpy((char *)tbl + 0x10 + 4, "CDV");
    strcpy((char *)tbl + 0x20 + 4, "MFS");
    *(uint32_t *)(tbl + 0x10) = 0xdead;

    func_004e1a00("CDV");
    CHECK(tbl[0x14] == 0 && *(uint32_t *)(tbl + 0x10) == 0xdead);
    CHECK(strcmp((char *)tbl + 4, "ROFS") == 0 && strcmp((char *)tbl + 0x24, "MFS") == 0);
    /* prefix match: the first entry whose name starts with the argument goes, and only that one */
    strcpy((char *)tbl + 0x30 + 4, "MFS2");
    func_004e1a00("MF");
    CHECK(tbl[0x24] == 0 && strcmp((char *)tbl + 0x34, "MFS2") == 0);
    /* an empty name matches the first entry, whatever it is */
    func_004e1a00("");
    CHECK(tbl[4] == 0);
    CHECK(last_msg == 0);
    func_004e1a00(0);
    CHECK(last_msg == (const char *)P4_G2H(0x00759e28));
    if (failures) {
        return 1;
    }
    printf("cri_cvfs: ok\n");
    return 0;
}
