/* rofs_open_test: how a CRI file name finds its file, against the opens of a retail run.
 *
 * Retail opens every stream and archive of the CRI layer as "VOL:NAME", "BGM:NAME", "ENV:NAME" or "BTL:NAME" through
 * cvFsOpen. The prefix names a ROFS device (one per mounted CVM volume, registered when kwlnInitSubsystems attaches
 * it), and the ROFS driver looks the bare name up in a directory list: the one the caller passes as the open's `dir`
 * (the partition loads of the voice archives pass the list of \SOUND\) or else the one H_Cdvd_SelectDirectory last
 * installed on that volume (ROFS_SetDir). BGM and ENV hold their files at the root. There is no search by base name.
 *
 * tests/data/rofs_retail_opens.txt holds the distinct opens of 30000 vsync frames of retail P4 under PCSX2 (PortTrace3's
 * call trace of 0x004e1cd8 cvFsOpen and 0x004f2798 ROFS_SetDir, the directory lists mapped to their directories with the
 * game's own tables): `<volume> <cur|blob> <directory> <name>`. All 358 opens of that run are on the disc where this
 * rule puts them; the test checks that each of the 131 distinct ones resolves to an existing disc file through
 * H_Cdvd_SelectDirectory / the directory list and p4pc_adxf_resolve_name, and through cvFsOpen on the ROFS devices
 * that the attach registers. It also checks what the rule refuses: a name in another directory, in another volume, and a
 * volume that is not attached. Skipped (exit 77) without P4PC_ISO and P4PC_ELF. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "adxf_host.h"
#include "guest_test.h"
#include "vfs.h"

#define SKIP_RETURN 77

void ADXT_Init(void);
void func_004d32f8(int32_t *prm);
int32_t func_004f34b0(void *config);
int32_t func_004f5bf0(int32_t handle, int32_t option, int32_t value);
int32_t func_004c6f50(const char *name, const char *image, void *arg);
void H_Cdvd_SelectDirectory(const char *dir);
void *cvFsOpen(const char *fname, void *dir, int rw);
void cvFsClose(void *cvfs);
int32_t cvFsReqRd(void *cvfs, int32_t sectors, void *buffer);
int32_t cvFsGetStat(void *cvfs);
int32_t cvFsSeek(void *cvfs, int32_t sectors, int32_t mode);
int32_t cvFsTell(void *cvfs);
void cvFsExecServer(void);
void cvFsStopTr(void *cvfs);
int sceCdRead(unsigned int lsn, unsigned int sectors, void *buffer, const void *mode);
int sceCdSync(int mode);
int sceCdBreak(void);
void *kwlnProbeHeapBudget(void);

static int checks, failures;

#define CHECK(cond)                                                              \
    do {                                                                         \
        checks++;                                                                \
        if (!(cond)) {                                                           \
            failures++;                                                          \
            fprintf(stderr, "rofs_open_test: FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                        \
    } while (0)

/* H_Cdvd_SelectDirectory as the game calls it: the directory with its volume prefix. */
static void select_dir(const char *vol, const char *dir)
{
    char arg[300];

    snprintf(arg, sizeof arg, "%s:%s", vol, dir);
    H_Cdvd_SelectDirectory(arg);
}

/* The blob of a directory in the game's table (names at `names`, lists beside them at `blobs`), or NULL. */
static const void *blob_of(uint32_t names, uint32_t blobs, const char *dir)
{
    int i;

    for (i = 0; i < 300; i++) {
        const char *name = (const char *)P4_G2H(P4_G32(names + 4u * (uint32_t)i));

        if (name[0] == '\0') {
            return NULL;
        }
        if (strcmp(name, dir) == 0) {
            return P4_G2H(P4_G32(blobs + 4u * (uint32_t)i));
        }
    }
    return NULL;
}

static int exists(const char *path)
{
    long long size = 0;
    int isDir = 0;

    return vfs_stat(path, &isDir, &size) == 0 && !isDir;
}

static void attach_volumes(void)
{
    static int32_t prm[5];
    int32_t cfg[4] = {6, 4, 0x3D2, 0};
    static const uint32_t names[4] = {0x00761698u, 0x0076169cu, 0x007616a0u, 0x007616a4u};
    static const uint32_t images[4] = {0x005dc788u, 0x005dc798u, 0x005dc7a8u, 0x005dc7b8u};
    int v;

    prm[0] = (int32_t)0x00761694u;
    func_004d32f8(prm);
    ADXT_Init();
    func_004f34b0(cfg);
    func_004f5bf0(0, 0, 1);
    func_004c6eb0();
    for (v = 0; v < 4; v++) {
        CHECK(func_004c6f50((const char *)P4_G2H(names[v]), (const char *)P4_G2H(images[v]), 0) == 0);
    }
}

/* Host reads remain busy throughout their submission update; neither polling
 * nor a CRI server pass can bypass the next-frame completion fence. */
static void test_transfers(void *h, const char *name, void *dir)
{
    void *buffer = p4pc_heap_malloc(2048);
    void *same = cvFsOpen(name, dir, 0);

    CHECK(buffer != NULL && same != NULL);
    if (buffer == NULL || same == NULL) {
        if (same != NULL) cvFsClose(same);
        p4pc_heap_free(buffer);
        cvFsClose(h);
        return;
    }
    CHECK(cvFsReqRd(h, 1, buffer) == 1);
    CHECK(cvFsGetStat(h) == 2 && cvFsGetStat(h) == 2);
    CHECK(cvFsReqRd(same, 1, buffer) == 1);
    CHECK(cvFsGetStat(same) == 2);
    cvFsExecServer();
    CHECK(cvFsGetStat(h) == 2 && cvFsGetStat(same) == 2);
    p4pc_wait_frame_boundary();
    CHECK(cvFsGetStat(h) == 1 && cvFsGetStat(same) == 1);
    CHECK(cvFsTell(h) == 1 && cvFsTell(same) == 1);

    CHECK(cvFsSeek(h, 0, 0) == 0);
    CHECK(cvFsReqRd(h, 1, buffer) == 1);
    cvFsStopTr(h);
    CHECK(cvFsGetStat(h) == 1);
    CHECK(cvFsReqRd(same, 1, buffer) == 1);
    cvFsClose(same); /* A closed handle's deferred callback must be cancelled. */
    p4pc_wait_frame_boundary();
    CHECK(cvFsGetStat(h) == 1);
    cvFsClose(h);

    CHECK(sceCdRead(0, 1, buffer, NULL) == 1);
    CHECK(sceCdSync(1) == 1 && sceCdSync(1) == 1);
    CHECK(sceCdRead(0, 1, buffer, NULL) == 0);
    p4pc_wait_frame_boundary();
    CHECK(sceCdSync(1) == 0);
    CHECK(sceCdRead(0, 1, buffer, NULL) == 1);
    CHECK(sceCdSync(0) == 0);
    CHECK(sceCdRead(0, 1, buffer, NULL) == 1);
    CHECK(sceCdBreak() == 1 && sceCdSync(1) == 0);
    p4pc_wait_frame_boundary();
    p4pc_heap_free(buffer);
}

static int test_main(int argc, char **argv)
{
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *data = argc > 1 ? argv[1] : "rofs_retail_opens.txt";
    FILE *f;
    char line[512];
    int rows = 0;
    char path[512];

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "rofs_open_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "rofs_open_test: cannot mount %s\n", iso);
        return 1;
    }
    attach_volumes();

    f = fopen(data, "r");
    if (f == NULL) {
        fprintf(stderr, "rofs_open_test: cannot read %s\n", data);
        return 1;
    }
    while (fgets(line, sizeof line, f) != NULL) {
        char vol[8], how[8], dir[256], name[128];
        char cvname[160];
        const void *blob = NULL;
        void *h;

        if (sscanf(line, "%7s %7s %255s %127s", vol, how, dir, name) != 4) {
            continue;
        }
        rows++;
        snprintf(cvname, sizeof cvname, "%s:%s", vol, name);
        if (strcmp(how, "blob") == 0) {
            blob = blob_of(0x00711190u, 0x007113e0u, dir);
            CHECK(blob != NULL);
        } else if (strcmp(vol, "VOL") == 0 || strcmp(vol, "BTL") == 0) {
            select_dir(vol, dir);
            CHECK(strcmp(p4pc_cdvd_dir(strcmp(vol, "BTL") == 0), dir) == 0);
        }
        CHECK(p4pc_adxf_resolve_name(cvname, blob, path, sizeof path) == 0);
        CHECK(exists(path));
        /* through CVFS: the prefix is a device, the ROFS interface opens it with the same lookup */
        h = cvFsOpen(cvname, (void *)blob, 0);
        CHECK(h != NULL);
        if (h != NULL && rows == 1) {
            test_transfers(h, cvname, (void *)blob);
        } else if (h != NULL) {
            cvFsClose(h);
        }
    }
    fclose(f);
    CHECK(rows == 131);

    /* what the rule refuses */
    select_dir("VOL", "\\SOUND\\EVENT\\");
    CHECK(p4pc_adxf_resolve_name("VOL:EVE_056.ADX", NULL, path, sizeof path) == 0);
    CHECK(strcmp(path, "DATA\\SOUND\\EVENT\\EVE_056.ADX") == 0);
    CHECK(p4pc_adxf_resolve_name("VOL:V162001.AFS", NULL, path, sizeof path) != 0);    /* in \SOUND\, not \SOUND\EVENT\ */
    CHECK(p4pc_adxf_resolve_name("VOL:V162001.AFS", blob_of(0x00711190u, 0x007113e0u, "\\SOUND\\"), path, sizeof path) == 0);
    CHECK(strcmp(path, "DATA\\SOUND\\V162001.AFS") == 0);
    CHECK(p4pc_adxf_resolve_name("VOL:EVE_056.ADX", blob_of(0x00711190u, 0x007113e0u, "\\SOUND\\"), path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("BTL:EVE_056.ADX", NULL, path, sizeof path) != 0);     /* another volume */
    CHECK(p4pc_adxf_resolve_name("ENV:EVE_056.ADX", NULL, path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("ENV:ENV_346.ADX", NULL, path, sizeof path) == 0);
    CHECK(p4pc_adxf_resolve_name("BGM:ENV_346.ADX", NULL, path, sizeof path) != 0);
    select_dir("VOL", "\\FIELD\\");
    CHECK(p4pc_adxf_resolve_name("VOL:EVE_056.ADX", NULL, path, sizeof path) != 0);     /* another directory is selected */
    CHECK(cvFsOpen("VOL:EVE_056.ADX", NULL, 0) == NULL);

    if (failures != 0) {
        fprintf(stderr, "rofs_open_test: %d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("rofs_open_test: ok, %d retail opens, %d checks\n", rows, checks);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
