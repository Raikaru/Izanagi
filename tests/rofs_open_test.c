/* ROFS lookup and transfer behavior against a hand-written synthetic disc.
 *
 * The fixture covers current-directory and explicit-blob lookup, all four
 * attached volumes, wrong-directory and wrong-volume rejection, and deferred
 * transfers. The executable under test still needs the user's ELF for boot.
 * The Python wrapper supplies the synthetic ISO; no game files are in it. */
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

/* Install synthetic directory names and blob identities into the guest tables
 * that both H_Cdvd_SelectDirectory and the host ROFS resolver consult. */
static int synthetic_directories(void)
{
    static const char *const names[] = {"\\ALPHA\\", "\\BETA\\", ""};
    int i;

    for (i = 0; ; i++) {
        const char *value = names[i];
        size_t size = strlen(value) + 1;
        char *name = p4pc_heap_malloc(size);
        void *blob = p4pc_heap_calloc(1, 16);
        if (name == NULL || blob == NULL) return -1;
        memcpy(name, value, size);
        P4_G32(0x00711190u + 4u * (uint32_t)i) = P4_H2G(name);
        P4_G32(0x007113e0u + 4u * (uint32_t)i) = P4_H2G(blob);
        if (*value == '\0') break;
    }
    return 0;
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
    const char *data = argc > 1 ? argv[1] : "rofs_synthetic_opens.txt";
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
    if (synthetic_directories() != 0) {
        fprintf(stderr, "rofs_open_test: cannot allocate synthetic directories\n");
        return 1;
    }

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
        } else if (strcmp(vol, "VOL") == 0) {
            select_dir(vol, dir);
            CHECK(strcmp(p4pc_cdvd_dir(0), dir) == 0);
        } else if (strcmp(vol, "BTL") == 0) {
            /* The battle directory table is native storage, unlike VOL's
             * guest table; select an authored directory through the host API. */
            p4pc_cdvd_set_dir(dir, 1);
            CHECK(strcmp(p4pc_cdvd_dir(1), dir) == 0);
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
    CHECK(rows == 5);

    /* what the rule refuses */
    select_dir("VOL", "\\ALPHA\\");
    CHECK(p4pc_adxf_resolve_name("VOL:SAMPLE.DAT", NULL, path, sizeof path) == 0);
    CHECK(strcmp(path, "DATA\\ALPHA\\SAMPLE.DAT") == 0);
    CHECK(p4pc_adxf_resolve_name("VOL:SECOND.DAT", NULL, path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("VOL:SECOND.DAT", blob_of(0x00711190u, 0x007113e0u, "\\BETA\\"), path, sizeof path) == 0);
    CHECK(strcmp(path, "DATA\\BETA\\SECOND.DAT") == 0);
    CHECK(p4pc_adxf_resolve_name("VOL:SAMPLE.DAT", blob_of(0x00711190u, 0x007113e0u, "\\BETA\\"), path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("BTL:SAMPLE.DAT", NULL, path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("ENV:SAMPLE.DAT", NULL, path, sizeof path) != 0);
    CHECK(p4pc_adxf_resolve_name("ENV:AMBIENT.DAT", NULL, path, sizeof path) == 0);
    CHECK(p4pc_adxf_resolve_name("BGM:AMBIENT.DAT", NULL, path, sizeof path) != 0);
    select_dir("VOL", "\\BETA\\");
    CHECK(p4pc_adxf_resolve_name("VOL:SAMPLE.DAT", NULL, path, sizeof path) != 0);
    CHECK(cvFsOpen("VOL:SAMPLE.DAT", NULL, 0) == NULL);

    if (failures != 0) {
        fprintf(stderr, "rofs_open_test: %d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("rofs_open_test: ok, %d synthetic opens, %d checks\n", rows, checks);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
