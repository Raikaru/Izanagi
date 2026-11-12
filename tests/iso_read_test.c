/* Exercise the real disc/container read loop (VFS slots, sectors, CVM headers) with short
 * reads, interrupts, errors and a premature end of file. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static const unsigned char input[] = "0123456789abcdef";
static size_t input_size, position, calls;
static int fake_error, fake_eof, interrupt_seek, interrupt_read, error_read, no_progress;

static int fixture_fseeko(FILE *fp, off_t off, int whence) {
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
    if (n > 3) n = 3;   /* every host transfer is short */
    memcpy(dst, input + position, n);
    position += n;
    if (interrupt_read) { interrupt_read = 0; fake_error = 1; errno = EINTR; }
    return n;
}

static int fixture_ferror(FILE *fp) { (void)fp; return fake_error; }
static int fixture_feof(FILE *fp) { (void)fp; return fake_eof; }
static void fixture_clearerr(FILE *fp) { (void)fp; fake_error = fake_eof = 0; }

/* A C library may already map these names (mingw: fseeko -> fseeko64). */
#undef fseeko
#undef fread
#undef ferror
#undef feof
#undef clearerr
#define fseeko fixture_fseeko
#define fread fixture_fread
#define ferror fixture_ferror
#define feof fixture_feof
#define clearerr fixture_clearerr
#include "../src/platform/files/iso9660.c"
#undef fseeko
#undef fread
#undef ferror
#undef feof
#undef clearerr

int main(void) {
    unsigned char result[sizeof input];
    FILE *stream = NULL; /* All stream operations above are fixture-owned. */
    input_size = sizeof input - 1;
    interrupt_seek = 1;
    interrupt_read = 1;
    if (iso_read_at(stream, 2, result, input_size - 2) != 0 ||
        memcmp(result, input + 2, input_size - 2) != 0 || calls < 5) return 1;

    input_size = 4;   /* the requested range runs past the end of the file */
    errno = EINVAL;
    if (iso_read_at(stream, 0, result, 5) == 0 || !fake_eof || errno != 0) return 2;

    input_size = sizeof input - 1;
    error_read = 1;
    if (iso_read_at(stream, 0, result, 1) == 0 || errno != EACCES) return 3;
    error_read = 0;
    no_progress = 1;
    if (iso_read_at(stream, 0, result, 1) == 0 || errno != EIO) return 4;
    return 0;
}
