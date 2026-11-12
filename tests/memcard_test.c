/* Host round-trip for the sceMc2 folder shim (src/platform/memcard/ps2_memcard.c).
 * Drives the entry points the way the game does: start an async op, then poll
 * sceMc2CheckAsync for the completion. Usage: memcard_test <save-root>. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "p4pc/os.h"

int32_t func_004316a8(int32_t flag);
int32_t func_00431928(void *params, void *extra);
int32_t func_00431b58(int32_t socket);
int32_t func_00431cb8(int32_t socket, int32_t *status);
int32_t func_00431d78(int32_t socket, const char *name, void *result);
int32_t func_00431f28(int32_t socket, const char *name, void *buf, uint32_t offset, uint32_t size);
int32_t func_00432288(int32_t socket, const char *name, const void *data, uint32_t offset, uint32_t size);
int32_t func_004323a8(int32_t socket, const char *name);
int32_t func_004326b8(int32_t socket, const char *name);
int32_t func_004327b0(int32_t socket, const char *name);
int32_t func_00432ec0(int32_t *cmd, int32_t *result);
int32_t func_00432ee8(int32_t *cmd, int32_t *result);

#define KIND_GETINFO 2
#define KIND_FORMAT 3
#define ERR_FORMAT_BUSY 0x81010010
#define MC_FAIL(e) ((int32_t)(0x81010000u | (unsigned)(e)))   /* a failed op: 0x8101 and the errno value */
#define KIND_READFILE 5
#define KIND_WRITEFILE 6
#define KIND_CREATEFILE 7
#define KIND_SEARCHFILE 14
#define KIND_MKDIR 11
#define KIND_CHDIR 12
#define ERR_NOT_INIT 0x81018001
#define ERR_BUSY 0x81018010
#define ERR_PARAM 0x81018016

static int failures;

#define CHECK(c)                                                          \
    do {                                                                  \
        if (!(c)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);  \
            failures++;                                                   \
        }                                                                 \
    } while (0)

/* Collect the completed host command once, checking its kind and the idle slot. */
static int32_t finish(int32_t kind)
{
    int32_t cmd = -1, result = 0x7fffffff;
    CHECK(func_00432ec0(&cmd, &result) == 1);
    CHECK(cmd == kind);
    CHECK(func_00432ec0(&cmd, &result) == -1);
    return result;
}

static void removeIfPresent(const char *path)
{
    char host[512];
    CHECK(snprintf(host, sizeof host, "%s", path) < (int)sizeof host);
    if (p4_os_extended_path(host, sizeof host) != 0) {
        CHECK(0);
        return;
    }
    remove(host);
    rmdir(host);
}

static void countEntry(const char *name, void *user)
{
    if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        ++*(unsigned *)user;
}

int main(int argc, char **argv)
{
    static const char *root;
    char path[512];
    char longRoot[512];
    uint8_t data[256], buf[256], small[16];
    int32_t status[4] = { 0, 0, 0, 0 };
    int32_t busyStatus[3] = { 7, 8, 9 };
    int32_t params[3] = { 0, 2, 0 };
    int32_t bad[3] = { 0, 4, 0 };
    int32_t bad2[3] = { 0, 2, 1 };
    int32_t sock, res, i;
    FILE *f;

    root = argc > 1 ? argv[1] : "p4pc_memcard_test";
    if (argc > 2 && strcmp(argv[2], "--long-path") == 0) {
        static const char component[] = "/long-memory-card-directory-component-00000000000000000000000000000";
        CHECK(snprintf(longRoot, sizeof longRoot, "%s", root) < (int)sizeof longRoot);
        for (i = 0; i <= 4; i++) {
            CHECK(snprintf(path, sizeof path, "%s", longRoot) < (int)sizeof path);
            CHECK(p4_os_extended_path(path, sizeof path) == 0);
            CHECK(p4_os_mkdir(path) == 0 || errno == EEXIST);
            if (i < 4) {
                size_t used = strlen(longRoot);
                if (used + sizeof component > sizeof longRoot) return 1;
                memcpy(longRoot + used, component, sizeof component);
            }
        }
        CHECK(strlen(longRoot) > 260);
        root = longRoot; /* Leave the SDK input unprefixed: it must normalize its own paths. */
    }
    p4_os_setenv("P4PC_SAVE_DIR", root, 1);

    /* Clean any state a previous run left in port 0. */
    snprintf(path, sizeof path, "%s/mc0/ROUND.BIN", root);
    removeIfPresent(path);
    snprintf(path, sizeof path, "%s/mc0/BUSY.BIN", root);
    removeIfPresent(path);
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR/IN.BIN", root);
    removeIfPresent(path);
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR", root);
    removeIfPresent(path);

    /* Before Init, CreateSocket refuses. */
    CHECK(func_00431928(params, NULL) == ERR_NOT_INIT);

    CHECK(func_004316a8(0) == 0);
    sock = func_00431928(params, NULL);
    CHECK(sock == 0);
    CHECK(func_00431928(bad, NULL) == ERR_PARAM);
    CHECK(func_00431928(bad2, NULL) == ERR_PARAM);

    /* GetInfo succeeds immediately. Busy admission must have no side effects. */
    CHECK(func_00431cb8(sock, status) > 0);
    CHECK(status[0] == 2);
    CHECK(status[2] == 0x1000);
    CHECK(func_00431cb8(sock, busyStatus) == ERR_BUSY);
    CHECK(busyStatus[0] == 7 && busyStatus[1] == 8 && busyStatus[2] == 9);
    CHECK(func_00432288(sock, "BUSY.BIN", "bad", 0, 3) == ERR_BUSY);
    CHECK(finish(KIND_GETINFO) == 0);
    CHECK(func_00431cb8(sock, status) > 0);
    CHECK(finish(KIND_GETINFO) == 0);
    CHECK(func_00431d78(sock, "BUSY.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == MC_FAIL(ENOENT));

    /* Create, then create again: the second one reports EEXIST. */
    CHECK(func_004323a8(sock, "ROUND.BIN") > 0);
    CHECK(finish(KIND_CREATEFILE) == 0);
    CHECK(func_004323a8(sock, "ROUND.BIN") > 0);
    CHECK(finish(KIND_CREATEFILE) == MC_FAIL(EEXIST));

    /* Write 256 bytes, then search finds it and does not find a missing name. */
    for (i = 0; i < 256; i++) {
        data[i] = (uint8_t)(i * 7 + 3);
    }
    CHECK(func_00432288(sock, "ROUND.BIN", data, 0, 256) > 0);
    CHECK(finish(KIND_WRITEFILE) == 256);

    CHECK(func_00431d78(sock, "ROUND.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == 0);
    CHECK(func_00431d78(sock, "NOPE.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == MC_FAIL(ENOENT));

    /* Read back the whole file. */
    memset(buf, 0, sizeof buf);
    CHECK(func_00431f28(sock, "ROUND.BIN", buf, 0, 256) > 0);
    CHECK(finish(KIND_READFILE) == 256);
    CHECK(memcmp(buf, data, 256) == 0);

    /* Overwrite 4 bytes at offset 10, then read a window that spans the change. */
    memset(small, 0xAA, 4);
    CHECK(func_00432288(sock, "ROUND.BIN", small, 10, 4) > 0);
    CHECK(finish(KIND_WRITEFILE) == 4);
    memset(buf, 0, sizeof buf);
    CHECK(func_00431f28(sock, "ROUND.BIN", buf, 8, 16) > 0);
    CHECK(finish(KIND_READFILE) == 16);
    CHECK(buf[0] == data[8] && buf[1] == data[9]);
    CHECK(buf[2] == 0xAA && buf[3] == 0xAA && buf[4] == 0xAA && buf[5] == 0xAA);
    CHECK(memcmp(buf + 6, data + 14, 10) == 0);

    /* Directory, chdir into it, file inside it, chdir back to root. */
    CHECK(func_004326b8(sock, "SAVEDIR") > 0);
    CHECK(finish(KIND_MKDIR) == 0);
    CHECK(func_004327b0(sock, "SAVEDIR") > 0);
    CHECK(finish(KIND_CHDIR) == 0);
    CHECK(func_004323a8(sock, "IN.BIN") > 0);
    CHECK(finish(KIND_CREATEFILE) == 0);
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR/IN.BIN", root);
    CHECK(p4_os_extended_path(path, sizeof path) == 0);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f) {
        fclose(f);
    }
    CHECK(func_004327b0(sock, "/") > 0);
    CHECK(finish(KIND_CHDIR) == 0);
    CHECK(func_00431d78(sock, "SAVEDIR/IN.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == 0);

    /* A name that climbs out of the port directory is refused. */
    CHECK(func_00431d78(sock, "../mc1/ROUND.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == MC_FAIL(ENOENT));

    /* Metadata, icon and full-save sizes retain their complete byte round-trips. */
    {
        static uint8_t bulk[171112];
        for (i = 0; i < (int32_t)sizeof bulk; i++) bulk[i] = (uint8_t)(i * 7 + 3);
        CHECK(func_00432288(sock, "ROUND.BIN", bulk, 0, 964) > 0);
        CHECK(finish(KIND_WRITEFILE) == 964);
        CHECK(func_00432288(sock, "ROUND.BIN", bulk, 0, 53542) > 0);
        CHECK(finish(KIND_WRITEFILE) == 53542);
        CHECK(func_00432288(sock, "ROUND.BIN", bulk, 0, sizeof bulk) > 0);
        CHECK(finish(KIND_WRITEFILE) == sizeof bulk);
        memset(bulk, 0, sizeof bulk);
        CHECK(func_00431f28(sock, "ROUND.BIN", bulk, 0, sizeof bulk) > 0);
        CHECK(finish(KIND_READFILE) == sizeof bulk);
        for (i = 0; i < (int32_t)sizeof bulk; i++) CHECK(bulk[i] == (uint8_t)(i * 7 + 3));
    }
    /* Wait mode collects the same completed host result without scheduler waits. */
    CHECK(func_00431cb8(sock, status) > 0);
    CHECK(func_00432ee8(NULL, &res) == 1);
    CHECK(res == 0);

    /* Format: refused for a bad socket, refused while another command is parked (with the code the
     * retail routine uses), then empties the card and returns to the root directory. */
    CHECK(func_00431b58(5) == 0x81018002);
    CHECK(func_00431cb8(sock, status) > 0);
    CHECK(func_00431b58(sock) == ERR_FORMAT_BUSY);
    CHECK(finish(KIND_GETINFO) == 0);
    {
        unsigned entries = 0;
        /* A positive control and an empty root prove SAVEDIR was removed before
         * fixture cleanup, even where CRT access() rejects a long host path. */
        CHECK(snprintf(path, sizeof path, "%s/mc0", root) < (int)sizeof path);
        CHECK(p4_os_extended_path(path, sizeof path) == 0);
        CHECK(p4_os_list_dir(path, countEntry, &entries) == 0);
        CHECK(entries == 2); /* ROUND.BIN and SAVEDIR. */
        CHECK(func_00431b58(sock) == 0);
        CHECK(finish(KIND_FORMAT) == 0);
        entries = 0;
        CHECK(p4_os_list_dir(path, countEntry, &entries) == 0);
        CHECK(entries == 0);
    }
    CHECK(func_00431d78(sock, "ROUND.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == MC_FAIL(ENOENT));
    CHECK(func_00431d78(sock, "SAVEDIR/IN.BIN", NULL) > 0);
    CHECK(finish(KIND_SEARCHFILE) == MC_FAIL(ENOENT));
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR", root);
    CHECK(p4_os_extended_path(path, sizeof path) == 0);
    CHECK(access(path, F_OK) != 0);

    res = func_00432ee8(NULL, NULL);
    CHECK(res == -1); /* nothing in flight */

    /* Tidy the files this run created. */
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR/IN.BIN", root);
    removeIfPresent(path);
    snprintf(path, sizeof path, "%s/mc0/SAVEDIR", root);
    removeIfPresent(path);
    snprintf(path, sizeof path, "%s/mc0/ROUND.BIN", root);
    removeIfPresent(path);

    if (failures) {
        fprintf(stderr, "memcard_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("memcard_test: ok\n");
    return 0;
}
