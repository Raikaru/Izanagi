/* Mod artifact files (include/p4pc/mod_artifacts.h) on the real OS file layer:
 * path rules and device names, case collisions, atomic replace, confined
 * reads, quotas checked before writing, no persistence without a settings
 * directory, and no link following where the platform has links. */
#include "p4pc/mod_artifacts.h"
#include "p4pc/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_artifacts: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)

static char root[1024];

static void path_of(char out[1400], const char *mod, const char *leaf)
{
    snprintf(out, 1400, "%s/mod-artifacts/%s%s%s", root, mod, leaf ? "/" : "", leaf ? leaf : "");
}
/* Removes the fixture's previous tree (depth two is all this test creates). */
static void visit_remove(const char *name, void *user)
{
    if (!strcmp(name, ".") || !strcmp(name, "..")) return;
    char path[1400];
    snprintf(path, sizeof path, "%s/%s", (const char *)user, name);
    if (p4_os_is_dir_nofollow(path)) {
        p4_os_list_dir(path, visit_remove, path);
        p4_os_rmdir(path);
    } else remove(path);
}
static int read_text(const char *mod, const char *path, char *out, size_t capacity)
{
    char reason[256];
    int64_t size = p4_mod_artifact_read(mod, path, out, capacity - 1, reason, sizeof reason);
    if (size < 0) return -1;
    out[size] = 0;
    return (int)size;
}

int main(int argc, char **argv)
{
    char reason[256], text[256], path[1400];
    if (argc != 2 || strlen(argv[1]) >= sizeof root - 64) return 2;
    snprintf(root, sizeof root, "%s", argv[1]);
    p4_os_mkdir(root);
    snprintf(path, sizeof path, "%s/mod-artifacts", root);
    p4_os_list_dir(path, visit_remove, path);

    /* Path rules. */
    CHECK(p4_mod_artifact_path_valid("spoiler.txt"));
    CHECK(p4_mod_artifact_path_valid("runs/ABCDEF0123/manifest.json"));
    CHECK(p4_mod_artifact_path_valid("a/b/c/d/e/f/g/h"));
    CHECK(!p4_mod_artifact_path_valid("a/b/c/d/e/f/g/h/i"));
    CHECK(!p4_mod_artifact_path_valid(""));
    CHECK(!p4_mod_artifact_path_valid("/abs"));
    CHECK(!p4_mod_artifact_path_valid("a//b"));
    CHECK(!p4_mod_artifact_path_valid("a/"));
    CHECK(!p4_mod_artifact_path_valid("../x"));
    CHECK(!p4_mod_artifact_path_valid("a/../x"));
    CHECK(!p4_mod_artifact_path_valid(".hidden"));
    CHECK(!p4_mod_artifact_path_valid("trailing."));
    CHECK(!p4_mod_artifact_path_valid("sp ace"));
    CHECK(!p4_mod_artifact_path_valid("back\\slash"));
    CHECK(!p4_mod_artifact_path_valid("colon:x"));
    CHECK(!p4_mod_artifact_path_valid("CON"));
    CHECK(!p4_mod_artifact_path_valid("nul.txt"));
    CHECK(!p4_mod_artifact_path_valid("dir/Com1.log"));
    CHECK(!p4_mod_artifact_path_valid("lpt9"));
    CHECK(p4_mod_artifact_path_valid("com0.txt"));
    CHECK(p4_mod_artifact_path_valid("console.txt"));
    char longest[200];
    memset(longest, 'a', 129);
    longest[129] = 0;
    CHECK(!p4_mod_artifact_path_valid(longest));

    /* No settings directory: writes are refused, reads find nothing. */
    CHECK(p4_mod_artifact_root(NULL) == 0);
    CHECK(p4_mod_artifact_write("p4pc.test", "a.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "not persistent") != NULL);
    CHECK(p4_mod_artifact_read("p4pc.test", "a.txt", text, sizeof text, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_folder("p4pc.test", text, sizeof text) == -1);

    CHECK(p4_mod_artifact_root(root) == 0);
    CHECK(p4_mod_artifact_folder("p4pc.test", text, sizeof text) == 0);
    path_of(path, "p4pc.test", NULL);
    CHECK(!strcmp(text, path));
    CHECK(p4_mod_artifact_write("Bad Id", "a.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_write("p4pc.test", "../escape.txt", "x", 1, reason, sizeof reason) == -1);

    /* Create, read back, atomically replace; an empty file is a file. */
    CHECK(p4_mod_artifact_write("p4pc.test", "runs/one/spoiler.txt", "first", 5, reason, sizeof reason) == 0);
    CHECK(read_text("p4pc.test", "runs/one/spoiler.txt", text, sizeof text) == 5 && !strcmp(text, "first"));
    CHECK(p4_mod_artifact_write("p4pc.test", "runs/one/spoiler.txt", "second!", 7, reason, sizeof reason) == 0);
    CHECK(read_text("p4pc.test", "runs/one/spoiler.txt", text, sizeof text) == 7 && !strcmp(text, "second!"));
    CHECK(p4_mod_artifact_read("p4pc.test", "runs/one/spoiler.txt", NULL, 64, reason, sizeof reason) == 7);
    CHECK(p4_mod_artifact_read("p4pc.test", "runs/one/spoiler.txt", text, 3, reason, sizeof reason) == -2);
    CHECK(p4_mod_artifact_write("p4pc.test", "empty.bin", NULL, 0, reason, sizeof reason) == 0);
    CHECK(p4_mod_artifact_read("p4pc.test", "empty.bin", text, sizeof text, reason, sizeof reason) == 0);
    path_of(path, "p4pc.test", "runs/one/.spoiler.txt.tmp");
    CHECK(fopen(path, "rb") == NULL);
    CHECK(p4_mod_artifact_read("p4pc.test", "runs/one/missing.txt", text, sizeof text, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_read("p4pc.other", "runs/one/spoiler.txt", text, sizeof text, reason, sizeof reason) == -1);

    /* A file where a directory is needed, a directory where a file is needed. */
    CHECK(p4_mod_artifact_write("p4pc.test", "empty.bin/inner.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_write("p4pc.test", "runs/one", "x", 1, reason, sizeof reason) == -1);

    /* Names that differ only by case from an existing entry. */
    CHECK(p4_mod_artifact_write("p4pc.test", "RUNS/two.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "case") != NULL);
    CHECK(p4_mod_artifact_write("p4pc.test", "runs/one/SPOILER.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_read("p4pc.test", "runs/one/Spoiler.txt", text, sizeof text, reason, sizeof reason) == -1);

    /* Quotas are checked before writing: oversized files and the file count. */
    char *big = calloc(1, P4_MOD_ARTIFACT_FILE_MAX + 1);
    CHECK(big != NULL);
    CHECK(p4_mod_artifact_write("p4pc.test", "big.bin", big, P4_MOD_ARTIFACT_FILE_MAX + 1, reason, sizeof reason) == -1);
    path_of(path, "p4pc.test", "big.bin");
    CHECK(fopen(path, "rb") == NULL);
    for (unsigned i = 0; i < P4_MOD_ARTIFACT_FILES_MAX; ++i) {
        char leaf[32];
        snprintf(leaf, sizeof leaf, "many/f%03u", i);
        if (p4_mod_artifact_write("p4pc.quota", leaf, "q", 1, reason, sizeof reason)) { CHECK(0); break; }
    }
    CHECK(p4_mod_artifact_write("p4pc.quota", "many/again", "q", 1, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "256") != NULL);
    /* Replacing an existing file does not add one. */
    CHECK(p4_mod_artifact_write("p4pc.quota", "many/f000", "replace", 7, reason, sizeof reason) == 0);
    /* Total bytes: sixteen 4 MiB files fill 64 MiB; the next byte is refused. */
    for (unsigned i = 0; i < 16; ++i) {
        char leaf[32];
        snprintf(leaf, sizeof leaf, "big/b%02u", i);
        CHECK(p4_mod_artifact_write("p4pc.bytes", leaf, big, P4_MOD_ARTIFACT_FILE_MAX, reason, sizeof reason) == 0);
    }
    CHECK(p4_mod_artifact_write("p4pc.bytes", "big/one", "x", 1, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "64 MiB") != NULL);
    CHECK(p4_mod_artifact_write("p4pc.bytes", "big/b00", big, P4_MOD_ARTIFACT_FILE_MAX, reason, sizeof reason) == 0);
    free(big);

#ifndef _WIN32
    /* A link is never followed, for directories or files. */
    char target[1400], link[1400];
    snprintf(target, sizeof target, "%s/outside", root);
    p4_os_mkdir(target);
    path_of(link, "p4pc.test", "linked");
    CHECK(symlink(target, link) == 0);
    CHECK(p4_mod_artifact_write("p4pc.test", "linked/x.txt", "x", 1, reason, sizeof reason) == -1);
    snprintf(target, sizeof target, "%s/outside/file.txt", root);
    FILE *stream = fopen(target, "wb");
    if (stream) fclose(stream);
    path_of(link, "p4pc.test", "file-link.txt");
    CHECK(symlink(target, link) == 0);
    CHECK(p4_mod_artifact_write("p4pc.test", "file-link.txt", "x", 1, reason, sizeof reason) == -1);
    CHECK(p4_mod_artifact_read("p4pc.test", "file-link.txt", text, sizeof text, reason, sizeof reason) == -1);
    stream = fopen(target, "rb");
    CHECK(stream != NULL);
    if (stream) {
        CHECK(fgetc(stream) == EOF);
        fclose(stream);
    }
#endif
    if (failures) return 1;
    puts("mod_artifacts: paths, case collisions, atomic replace, confined reads, quotas and no links PASS");
    return 0;
}
