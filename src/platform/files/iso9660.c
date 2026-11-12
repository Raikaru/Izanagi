/*
 * Read-only ISO 9660 walker for the disc image and the CVM containers.
 *
 * Only the primary volume descriptor and directory records are used. Each volume
 * is walked from its own base offset, so extent LBAs are relative to that base.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iso9660.h"

#define ISO_SECTOR 2048LL
#define ISO_PVD_SECTOR 16LL
#define ISO_MAX_DEPTH 16
#define ISO_PATH_MAX 512

static unsigned le32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

int iso_read_at(FILE *fp, long long offset, void *dst, size_t n)
{
    unsigned char *out = dst;

    clearerr(fp);
    for (;;) {
        errno = 0;
        if (fseeko(fp, (off_t)offset, SEEK_SET) == 0) {
            break;
        }
        if (errno != EINTR) {
            if (errno == 0) {
                errno = EIO;
            }
            return -1;
        }
        clearerr(fp);
    }
    while (n != 0) {
        size_t got;

        errno = 0;
        got = fread(out, 1, n, fp);
        out += got;
        n -= got;
        if (n == 0) {
            return 0;
        }
        if (ferror(fp)) {
            if (errno == EINTR) {
                clearerr(fp);
                continue;
            }
            if (errno == 0) {
                errno = EIO;
            }
            return -1;
        }
        if (feof(fp)) {
            return -1;
        }
        if (got == 0) {
            errno = EIO;
            return -1;
        }
    }
    return 0;
}

/* Copies an ISO name into `out` in upper case, without the ";N" version suffix or a
 * trailing dot. Returns the length of `out`. */
static size_t clean_name(const unsigned char *name, unsigned len, char *out)
{
    size_t n = 0;
    unsigned i;

    for (i = 0; i < len && name[i] != ';'; i++) {
        unsigned char c = name[i];

        if (c >= 'a' && c <= 'z') {
            c = (unsigned char)(c - 'a' + 'A');
        }
        out[n++] = (char)c;
    }
    if (n > 0 && out[n - 1] == '.') {
        n--;
    }
    out[n] = '\0';
    return n;
}

/* Visits the directory whose data is `length` bytes at absolute offset `extent`.
 * Subdirectories are walked recursively; `prefix` is the path of this directory. */
static int walk_dir(FILE *fp, long long base, long long extent, unsigned length, const char *prefix,
                    int depth, iso_visit_fn visit, void *ctx)
{
    unsigned char *dir;
    unsigned pos = 0;
    int rc = 0;

    if (depth > ISO_MAX_DEPTH) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    dir = malloc(length);
    if (dir == NULL) {
        return -1;
    }
    if (iso_read_at(fp, extent, dir, length) != 0) {
        free(dir);
        return -1;
    }

    while (pos < length) {
        const unsigned char *rec = dir + pos;
        unsigned rec_len = rec[0];
        unsigned name_len;
        char leaf[256];
        char path[ISO_PATH_MAX];
        size_t leaf_len;

        /* Records never span a sector boundary; zero padding fills the rest. */
        if (rec_len == 0) {
            pos = (pos / 2048u + 1) * 2048u;
            continue;
        }
        if (rec_len < 33 || pos + rec_len > length) {
            break;
        }
        name_len = rec[32];
        if (33 + name_len > rec_len) {
            break;
        }
        if (name_len == 1 && (rec[33] == 0 || rec[33] == 1)) {
            /* "." and ".." */
            pos += rec_len;
            continue;
        }

        leaf_len = clean_name(rec + 33, name_len, leaf);
        if (leaf_len == 0) {
            pos += rec_len;
            continue;
        }
        if (prefix[0] != '\0') {
            if (snprintf(path, sizeof path, "%s/%s", prefix, leaf) >= (int)sizeof path) {
                rc = -1;
                break;
            }
        } else {
            if (snprintf(path, sizeof path, "%s", leaf) >= (int)sizeof path) {
                rc = -1;
                break;
            }
        }

        if (rec[25] & 0x02) {
            rc = walk_dir(fp, base, base + (long long)le32(rec + 2) * ISO_SECTOR, le32(rec + 10), path,
                          depth + 1, visit, ctx);
            if (rc != 0) {
                break;
            }
        } else {
            visit(ctx, path, base + (long long)le32(rec + 2) * ISO_SECTOR, le32(rec + 10));
        }
        pos += rec_len;
    }

    free(dir);
    return rc;
}

int iso_walk(FILE *fp, long long base, iso_visit_fn visit, void *ctx)
{
    unsigned char pvd[2048];
    const unsigned char *root;

    if (iso_read_at(fp, base + ISO_PVD_SECTOR * ISO_SECTOR, pvd, sizeof pvd) != 0) {
        return -1;
    }
    if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0) {
        return -1;
    }
    root = pvd + 156;
    if ((root[25] & 0x02) == 0) {
        return -1;
    }
    return walk_dir(fp, base, base + (long long)le32(root + 2) * ISO_SECTOR, le32(root + 10), "", 0,
                    visit, ctx);
}
