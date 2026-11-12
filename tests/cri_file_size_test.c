/* Real file-size and seek widths through CRI, including the mirrored high-word caller.
 * P4PC_ISO itself is exposed through the VFS override layer, without copying retail bytes.
 * Skipped (77) unless P4PC_ISO and P4PC_ELF are set. */
#ifndef _WIN32
#define _XOPEN_SOURCE 700
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "guest_test.h"
#include "vfs.h"

int sceOpen(const char *name, int flags);
int sceClose(int fd);
int sceRead(int fd, void *buffer, int bytes);
int64_t func_004e47f8(int32_t fd, int64_t offset, int32_t whence);
int64_t htCiGetFileSize(const char *name);
int64_t func_004e8058(const char *name);
int32_t func_004e7798(void *name);
int32_t htci_filesize_upper(const char *name);
int32_t func_004e80e0(uint8_t *name);

static int checks, failures;

#ifndef _WIN32
static char fixtureDirectory[] = "/tmp/p4pc-cri-size-XXXXXX";
static char fixtureLink[4096];

static void remove_fixture(void)
{
    if (fixtureLink[0] != '\0') {
        unlink(fixtureLink);
    }
    rmdir(fixtureDirectory);
}
#endif

#define CHECK(condition)                                                        \
    do {                                                                        \
        checks++;                                                               \
        if (!(condition)) {                                                     \
            failures++;                                                         \
            fprintf(stderr, "cri_file_size_test: FAIL line %d: %s\n",             \
                    __LINE__, #condition);                                      \
        }                                                                       \
    } while (0)

static int test_main(int argc, char **argv)
{
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *slash;
    const char *backslash;
    const char *name;
    char directory[4096];
    char path[256];
    long long bytes = 0;
    const int64_t boundary = INT64_C(0x100000000);
    const int64_t offsets[] = {INT64_C(0xffffffff), INT64_C(0x100000000), INT64_C(0x100000001)};
    uint32_t oldMode;
    int isDirectory = 0;
    int fd;
    int reference;
    int written;

    (void)argc;
    (void)argv;
    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "cri_file_size_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return 77;
    }
    slash = strrchr(iso, '/');
    backslash = strrchr(iso, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        slash = backslash;
    }
    name = slash == NULL ? iso : slash + 1;
    if (slash == NULL) {
        strcpy(directory, ".");
    } else {
        size_t length = (size_t)(slash - iso) + 1;
        if (length >= sizeof directory) {
            fprintf(stderr, "cri_file_size_test: ISO directory is too long\n");
            return 1;
        }
        memcpy(directory, iso, length);
        directory[length] = '\0';
    }
    written = snprintf(path, sizeof path, "host0:%s", name);
    if (written < 0 || (size_t)written >= sizeof path) {
        fprintf(stderr, "cri_file_size_test: ISO filename exceeds the CRI path buffer\n");
        return 1;
    }
#ifndef _WIN32
    /* VFS override keys are upper-case; the CI ISO is named p4.iso on a
     * case-sensitive filesystem. Expose the real file under its canonical key,
     * without copying it or requiring writes beside the read-only retail input. */
    {
        char *absolute = realpath(iso, NULL);
        size_t prefix;
        if (absolute == NULL || mkdtemp(fixtureDirectory) == NULL) {
            free(absolute);
            fprintf(stderr, "cri_file_size_test: cannot create ISO alias directory\n");
            return 1;
        }
        atexit(remove_fixture);
        written = snprintf(fixtureLink, sizeof fixtureLink, "%s/%s", fixtureDirectory, name);
        if (written < 0 || (size_t)written >= sizeof fixtureLink) {
            free(absolute);
            return 1;
        }
        prefix = strlen(fixtureDirectory) + 1;
        for (size_t i = prefix; fixtureLink[i] != '\0'; i++) {
            if (fixtureLink[i] >= 'a' && fixtureLink[i] <= 'z') {
                fixtureLink[i] -= 'a' - 'A';
            }
        }
        if (symlink(absolute, fixtureLink) != 0) {
            free(absolute);
            fprintf(stderr, "cri_file_size_test: cannot create ISO alias\n");
            return 1;
        }
        free(absolute);
        strcpy(directory, fixtureDirectory);
    }
#endif

    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    if (vfs_mount(iso, directory) != 0 || vfs_stat(path, &isDirectory, &bytes) != 0) {
        fprintf(stderr, "cri_file_size_test: cannot expose the actual ISO through VFS\n");
        return 1;
    }
    CHECK(!isDirectory);
    CHECK(bytes > boundary + 64);
    if (bytes <= boundary + 64) {
        return 1;
    }
    oldMode = P4_G32(0x00731c70u);
    P4_G32(0x00731c70u) = 1;
    fd = sceOpen(path, 1); /* SDK RDONLY: not a request to write. */
    reference = vfs_open(path, 0);
    CHECK(fd >= 0);
    CHECK(reference >= 0);
    CHECK(sceOpen(path, 2) < 0);
    CHECK(sceOpen(path, 3) < 0);
    CHECK(sceOpen(path, 0x201) < 0);
    CHECK(htCiGetFileSize(path) == bytes);
    CHECK(func_004e8058(path) == bytes);
    CHECK(func_004e7798((void *)path) == (int32_t)(uint32_t)bytes);
    CHECK(htci_filesize_upper(path) == (int32_t)((int64_t)bytes >> 32));
    CHECK(func_004e80e0((uint8_t *)path) == (int32_t)(uint32_t)bytes);
    if (fd >= 0 && reference >= 0) {
        for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; i++) {
            uint8_t actual[64];
            uint8_t expected[64];
            CHECK(func_004e47f8(fd, offsets[i], 0) == offsets[i]);
            CHECK(vfs_seek(reference, offsets[i], 0) == offsets[i]);
            CHECK(sceRead(fd, actual, sizeof actual) == sizeof actual &&
                  vfs_read(reference, expected, sizeof expected) == sizeof expected &&
                  memcmp(actual, expected, sizeof actual) == 0);
        }
        CHECK(func_004e47f8(fd, 0, 2) == bytes);
        /* Retail's legacy branch explicitly sign-extends the low offset word. */
        P4_G32(0x00731c70u) = 0;
        CHECK(func_004e47f8(fd, boundary + 0x1234, 0) == 0x1234);
    }
    if (fd >= 0) {
        CHECK(sceClose(fd) == 0);
    }
    if (reference >= 0) {
        CHECK(vfs_close(reference) == 0);
    }
    P4_G32(0x00731c70u) = oldMode;
    printf("cri_file_size_test: ISO=%lld bytes, upper=%d, checks=%d, failures=%d\n",
           bytes, (int32_t)((int64_t)bytes >> 32), checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
