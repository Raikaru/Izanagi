/* File-layer checks against the retail disc named by P4PC_ISO. Exits 77 (CTest skip) when
 * P4PC_ISO is unset. Built with src/platform/files/vfs.c and iso9660.c only. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "iso9660.h"
#include "p4pc/os.h"
#include "vfs.h"

#define SKIP_RETURN 77
#define SLUS_SIZE 8621084LL
#define DATA_CVM_SIZE 3001540608LL
/* Each CVM container is an ISO 9660 volume at this offset (see vfs.c, CVM_HEADER_BYTES). */
#define CVM_HEADER_BYTES 0x1800
#define CHUNK 64
#define SECTOR 2048
#define OVERRIDE_DIR "p4pc_files_test_overrides"

static FILE *disc;
static int failures;

static void fail(const char *what, const char *name)
{
    fprintf(stderr, "files_test: FAIL %s: %s\n", what, name);
    failures++;
}

/* Opens a PS2 path and reads up to n bytes from its start. Returns the byte count, or -1
 * if the path does not open. *size receives the file size. */
static int read_head(const char *path, unsigned char *buf, int n, long long *size)
{
    int fd = vfs_open(path, 0);
    int got;

    if (fd < 0) {
        return -1;
    }
    *size = vfs_seek(fd, 0, SEEK_END);
    vfs_seek(fd, 0, SEEK_SET);
    got = vfs_read(fd, buf, n);
    vfs_close(fd);
    return got;
}

static void check_containers(void)
{
    static const char *const names[] = {"DATA.CVM", "BTL.CVM", "BGM.CVM", "ENV.CVM", "ZZZZZ.BIN"};
    unsigned char head[4];
    long long size = 0;

    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (read_head(names[i], head, sizeof head, &size) != 4) {
            fail("container missing or unreadable", names[i]);
            continue;
        }
        if (size <= 0) {
            fail("container is empty", names[i]);
        }
        if (i < 4 && memcmp(head, "CVMH", 4) != 0) {
            fail("container lacks CVMH magic", names[i]);
        }
        if (i == 0 && size != DATA_CVM_SIZE) {
            fail("DATA.CVM size is not 3001540608", names[i]);
        }
    }
}

static void check_slus(void)
{
    unsigned char head[4];
    unsigned char sector[SECTOR];
    long long size = 0;
    long long off = 0;
    long long sz = 0;

    if (read_head("SLUS_217.82", head, sizeof head, &size) != 4 || memcmp(head, "\x7f" "ELF", 4) != 0) {
        fail("SLUS_217.82 does not open with ELF magic", "SLUS_217.82");
    }
    if (size != SLUS_SIZE) {
        fail("SLUS_217.82 size is not 8621084", "SLUS_217.82");
    }
    /* The same bytes through the CD sector path: LSN = byte offset / 2048. */
    if (vfs_disc_locate("SLUS_217.82", &off, &sz) != 0 || off % SECTOR != 0) {
        fail("SLUS_217.82 has no sector-aligned disc location", "SLUS_217.82");
    } else if (vfs_disc_read_sectors((unsigned int)(off / SECTOR), 1, sector) != 0 ||
               memcmp(sector, "\x7f" "ELF", 4) != 0) {
        fail("sector read of SLUS_217.82 lacks ELF magic", "SLUS_217.82");
    }
}

struct root_pick {
    long long offset;
    int found;
};

struct first_file {
    char path[256];
    long long offset;
    long long size;
    int found;
};

static void pick_data_cvm(void *ctx, const char *path, long long offset, long long size)
{
    struct root_pick *p = ctx;

    (void)size;
    if (strcmp(path, "DATA.CVM") == 0) {
        p->offset = offset;
        p->found = 1;
    }
}

static void pick_first(void *ctx, const char *path, long long offset, long long size)
{
    struct first_file *p = ctx;

    if (!p->found && size > 0) {
        snprintf(p->path, sizeof p->path, "%s", path);
        p->offset = offset;
        p->size = size;
        p->found = 1;
    }
}

/* A file inside DATA.CVM must open through the VFS, its disc location must agree with the
 * container walk, and its first bytes must equal the raw disc bytes. */
static void check_cvm_file(void)
{
    struct root_pick root = {0, 0};
    struct first_file first = {"", 0, 0, 0};
    unsigned char raw[CHUNK];
    unsigned char got[CHUNK];
    char key[300];
    long long off = 0;
    long long sz = 0;
    size_t want;

    if (iso_walk(disc, 0, pick_data_cvm, &root) != 0 || !root.found) {
        fail("DATA.CVM not on the disc root", "DATA.CVM");
        return;
    }
    if (iso_walk(disc, root.offset + CVM_HEADER_BYTES, pick_first, &first) != 0 || !first.found) {
        fail("DATA.CVM holds no files", "DATA.CVM");
        return;
    }
    snprintf(key, sizeof key, "DATA/%s", first.path);
    if (vfs_disc_locate(key, &off, &sz) != 0 || off != first.offset || sz != first.size) {
        fail("vfs_disc_locate disagrees with the container walk", key);
    }
    {
        int container = -1;
        int fd = vfs_open(key, 0);
        long long extent = -1;
        if (fd < 0 || vfs_disc_extent(fd, &extent, &container) != 0 ||
            extent != first.offset || container != 0)
            fail("open CVM extent/container disagrees with the disc", key);
        if (vfs_is_override(fd))
            fail("disc file reported as a host override", key);
        if (fd >= 0)
            vfs_close(fd);
    }
    want = first.size < CHUNK ? (size_t)first.size : CHUNK;
    if (read_head(key, got, CHUNK, &sz) != (int)want || sz != first.size) {
        fail("file inside DATA.CVM does not open", key);
        return;
    }
    if (fseeko(disc, (off_t)first.offset, SEEK_SET) != 0 || fread(raw, 1, want, disc) != want ||
        memcmp(raw, got, want) != 0) {
        fail("file inside DATA.CVM differs from the raw disc bytes", key);
    }
}

/* An override file in P4PC_OVERRIDE_DIR wins for opens; the disc copy stays visible to
 * the CD drive model (vfs_disc_locate ignores overrides). */
static void check_override(void)
{
    unsigned char head[8];
    unsigned char sector[SECTOR];
    long long size = 0;
    long long off = 0;
    long long sz = 0;

    if (read_head("SYSTEM.CNF", head, sizeof head, &size) != 8 || memcmp(head, "OVERRIDE", 8) != 0 ||
        size != 8) {
        fail("override SYSTEM.CNF does not win", "SYSTEM.CNF");
    }
    if (vfs_disc_locate("SYSTEM.CNF", &off, &sz) != 0 || off % SECTOR != 0 ||
        vfs_disc_read_sectors((unsigned int)(off / SECTOR), 1, sector) != 0 ||
        memcmp(sector, "OVERRIDE", 8) == 0) {
        fail("disc SYSTEM.CNF is hidden by the override", "SYSTEM.CNF");
    }
    {
        int container = 0;
        int fd = vfs_open("SYSTEM.CNF", 0);
        long long extent = -1;
        if (fd < 0 || vfs_disc_extent(fd, &extent, &container) != 0 ||
            extent != off || container != -1)
            fail("override lost its original physical extent", "SYSTEM.CNF");
        if (!vfs_is_override(fd))
            fail("override lost its host backing flag", "SYSTEM.CNF");
        if (fd >= 0) {
            vfs_close(fd);
            if (vfs_disc_extent(fd, &extent, &container) != -1)
                fail("closed descriptor retained a physical extent", "SYSTEM.CNF");
            if (vfs_is_override(fd))
                fail("closed descriptor retained a host backing flag", "SYSTEM.CNF");
        }
    }
}

int main(void)
{
    const char *iso = getenv("P4PC_ISO");
    char override_file[256];

    if (!iso || !*iso) {
        fprintf(stderr, "files_test: skipped, P4PC_ISO is unset\n");
        return SKIP_RETURN;
    }
    disc = fopen(iso, "rb");
    if (!disc) {
        fprintf(stderr, "files_test: cannot open P4PC_ISO=%s\n", iso);
        return 1;
    }
    p4_os_mkdir(OVERRIDE_DIR);
    snprintf(override_file, sizeof override_file, "%s/SYSTEM.CNF", OVERRIDE_DIR);
    {
        FILE *f = fopen(override_file, "wb");

        if (!f) {
            fprintf(stderr, "files_test: cannot write %s\n", override_file);
            fclose(disc);
            return 1;
        }
        fputs("OVERRIDE", f);
        fclose(f);
    }
    if (vfs_mount(iso, OVERRIDE_DIR) != 0) {
        fail("vfs_mount failed", iso);
    } else {
        check_containers();
        check_slus();
        check_cvm_file();
        check_override();
    }
    remove(override_file);
    rmdir(OVERRIDE_DIR);
    fclose(disc);
    if (failures) {
        fprintf(stderr, "files_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("files_test: all checks passed\n");
    return 0;
}
