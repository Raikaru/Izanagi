/* Mod lifecycle records (include/p4pc/mod_lifecycle.h) on the real OS file layer:
 * session allocation, the crash handler's marker step, reconciliation reads,
 * per-mod consumption and the bounded pruning of other sessions' markers. */
#include "p4pc/mod_lifecycle.h"
#include "p4pc/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_lifecycle: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)

static char root[1024];

static long file_size(const char *leaf)
{
    char path[1200];
    snprintf(path, sizeof path, "%s/mod-lifecycle/%s", root, leaf);
    FILE *stream = fopen(path, "rb");
    if (!stream) return -1;
    fseek(stream, 0, SEEK_END);
    long size = ftell(stream);
    fclose(stream);
    return size;
}
static void write_leaf(const char *leaf, const char *text)
{
    char path[1200];
    snprintf(path, sizeof path, "%s/mod-lifecycle/%s", root, leaf);
    FILE *stream = fopen(path, "wb");
    if (stream) {
        fputs(text, stream);
        fclose(stream);
    }
}
/* A new launch: the previous process's marker handle is gone. */
static void relaunch(void)
{
    CHECK(p4_mod_lifecycle_root(NULL) == 0);
    CHECK(p4_mod_lifecycle_root(root) == 0);
}

int main(int argc, char **argv)
{
    char reason[192], tag[64];
    if (argc != 2 || strlen(argv[1]) >= sizeof root - 32) return 2;
    snprintf(root, sizeof root, "%s", argv[1]);
    p4_os_mkdir(root);
    char directory[1200];
    snprintf(directory, sizeof directory, "%s/mod-lifecycle", root);
    /* A fresh tree: the fixture data directory persists between runs. */
    for (unsigned i = 0; i < 40; ++i) {
        char path[1200];
        snprintf(path, sizeof path, "%s/crash-%u.marker", directory, i);
        remove(path);
    }
    snprintf(directory + strlen(directory), sizeof directory - strlen(directory), "/session");
    remove(directory);

    /* No settings directory: no session, no writes. */
    CHECK(p4_mod_lifecycle_root(NULL) == 0);
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 0 && strstr(reason, "No settings directory"));
    CHECK(p4_mod_lifecycle_tag("test.mod", "R1", reason, sizeof reason) == -1);

    /* Session 1: an empty marker opened before any fault. */
    CHECK(p4_mod_lifecycle_root(root) == 0);
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 1);
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 1);
    CHECK(file_size("crash-1.marker") == 0 && file_size("session") == 2);
    CHECK(p4_mod_lifecycle_tag("test.mod", "", reason, sizeof reason) == -1);
    CHECK(p4_mod_lifecycle_tag("test.mod", "bad\ttag", reason, sizeof reason) == -1);
    CHECK(p4_mod_lifecycle_tag("test.mod", "R1:B1", reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_tag("other.mod", "X", reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_tag("test.mod", "R1:B2", reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_crashed(1, "test.mod", tag, sizeof tag, reason, sizeof reason) == 0);
    /* What the crash handler does, first: the staged bytes, whole. */
    p4_os_crash_marker_write();
    CHECK(file_size("crash-1.marker") == (long)strlen("P4PC-CRASH 1 session=1\ntest.mod\tR1:B2\nother.mod\tX\n"));

    /* Session 2 reads session 1's crash; its own marker is never "crashed". */
    relaunch();
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 2);
    CHECK(p4_mod_lifecycle_crashed(2, "test.mod", tag, sizeof tag, reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_crashed(1, "test.mod", tag, sizeof tag, reason, sizeof reason) == 1 && !strcmp(tag, "R1:B2"));
    CHECK(p4_mod_lifecycle_crashed(1, "untagged.mod", tag, sizeof tag, reason, sizeof reason) == 1 && !tag[0]);
    CHECK(p4_mod_lifecycle_crashed(9, "test.mod", tag, sizeof tag, reason, sizeof reason) == 0);
    /* Consumption is per mod; the marker goes with its last record. */
    CHECK(p4_mod_lifecycle_consume(1, "test.mod", reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_crashed(1, "test.mod", tag, sizeof tag, reason, sizeof reason) == 1 && !tag[0]);
    CHECK(p4_mod_lifecycle_crashed(1, "other.mod", tag, sizeof tag, reason, sizeof reason) == 1 && !strcmp(tag, "X"));
    CHECK(p4_mod_lifecycle_consume(1, "other.mod", reason, sizeof reason) == 0);
    CHECK(file_size("crash-1.marker") == -1);
    CHECK(p4_mod_lifecycle_crashed(1, "other.mod", tag, sizeof tag, reason, sizeof reason) == 0);

    /* Session 2 ends without a crash (empty marker): session 3 prunes it. A
     * corrupt or foreign marker is an error, never a verified crash. */
    write_leaf("crash-5.marker", "P4PC-CRASH 1 session=6\n");
    relaunch();
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 3);
    CHECK(file_size("crash-2.marker") == -1 && file_size("crash-3.marker") == 0);
    CHECK(p4_mod_lifecycle_crashed(2, "test.mod", tag, sizeof tag, reason, sizeof reason) == 0);
    CHECK(p4_mod_lifecycle_crashed(5, "test.mod", tag, sizeof tag, reason, sizeof reason) == -1 && strstr(reason, "corrupt"));

    /* At most P4_MOD_LIFECYCLE_KEPT recorded crashes stay; the oldest go. */
    for (unsigned i = 10; i < 10 + P4_MOD_LIFECYCLE_KEPT + 2; ++i) {
        char leaf[32], text[64];
        snprintf(leaf, sizeof leaf, "crash-%u.marker", i);
        snprintf(text, sizeof text, "P4PC-CRASH 1 session=%u\n", i);
        write_leaf(leaf, text);
    }
    relaunch();
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 4);
    CHECK(file_size("crash-5.marker") == -1 && file_size("crash-10.marker") == -1 && file_size("crash-11.marker") == -1);
    CHECK(file_size("crash-12.marker") > 0 && file_size("crash-27.marker") > 0);

    /* A corrupt counter blocks sessions rather than reusing an identity. */
    write_leaf("session", "four\n");
    relaunch();
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 0 && strstr(reason, "corrupt"));
    write_leaf("session", "4\n");
    relaunch();
    CHECK(p4_mod_lifecycle_session(reason, sizeof reason) == 5);
    CHECK(p4_mod_lifecycle_root(NULL) == 0);
    if (!failures) puts("mod_lifecycle: sessions, crash marker step, per-mod reconciliation and bounded pruning PASS");
    return failures ? 1 : 0;
}
