/* Exercise the real loader read loop with short reads, interrupts and errors. */
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const unsigned char input[] = "0123456789abcdef";
static size_t input_size, position, calls;
static int fake_error, fake_eof, interrupt_seek, interrupt_read, error_read, no_progress;

static int fixture_fseek(FILE *fp, long off, int whence) {
    (void)fp;
    if (interrupt_seek) { interrupt_seek = 0; errno = EINTR; return -1; }
    if (whence != SEEK_SET || off < 0) { errno = EINVAL; return -1; }
    position = (size_t)off;
    fake_eof = 0;
    return 0;
}

static size_t fixture_fread(void *dst, size_t size, size_t count, FILE *fp) {
    (void)fp;
    ++calls;
    if (size != 1) { fake_error = 1; errno = EINVAL; return 0; }
    if (error_read) { fake_error = 1; errno = EACCES; return 0; }
    if (no_progress) return 0;
    if (position >= input_size) { fake_eof = 1; return 0; }
    size_t available = input_size - position;
    size_t n = count < available ? count : available;
    if (n > 3) n = 3;
    memcpy(dst, input + position, n);
    position += n;
    if (interrupt_read) { interrupt_read = 0; fake_error = 1; errno = EINTR; }
    return n;
}

static int fixture_ferror(FILE *fp) { (void)fp; return fake_error; }
static int fixture_feof(FILE *fp) { (void)fp; return fake_eof; }
static void fixture_clearerr(FILE *fp) { (void)fp; fake_error = fake_eof = 0; }

#define fseek fixture_fseek
#define fread fixture_fread
#define ferror fixture_ferror
#define feof fixture_feof
#define clearerr fixture_clearerr
#include "../src/image.c"
#undef fseek
#undef fread
#undef ferror
#undef feof
#undef clearerr

const p4pc_native_t p4pc_native_table[] = {{0, NULL, NULL, NULL}};
const size_t p4pc_native_count = 0;
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_alias_t p4pc_alias_table[] = {{0, NULL}};
const size_t p4pc_alias_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;

int main(void) {
    unsigned char result[sizeof input];
    FILE *stream = NULL; /* All stream operations above are fixture-owned. */
    input_size = sizeof input - 1;
    interrupt_seek = 1;
    interrupt_read = 1;
    if (read_at(stream, 0, result, input_size) != 0 ||
        memcmp(result, input, input_size) != 0 || calls < 3) return 1;

    input_size = 4;
    errno = EINVAL;
    if (read_at(stream, 0, result, 5) == 0 || !fake_eof || errno != 0) return 2;

    input_size = sizeof input - 1;
    error_read = 1;
    if (read_at(stream, 0, result, 1) == 0 || errno != EACCES) return 3;
    error_read = 0;
    no_progress = 1;
    if (read_at(stream, 0, result, 1) == 0 || errno != EIO) return 4;
    return 0;
}
