/* Synthetic CBS fixture encoder: RFC 1950 zlib + RFC 1951 stored blocks,
 * followed by the format's published RC4 permutation. No retail fixture data. */
#include "../src/platform/memcard/import_cbs.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char initial_s[256] = {
0x5f, 0x1f, 0x85, 0x6f, 0x31, 0xaa, 0x3b, 0x18,
		    0x21, 0xb9, 0xce, 0x1c, 0x07, 0x4c, 0x9c, 0xb4,
		    0x81, 0xb8, 0xef, 0x98, 0x59, 0xae, 0xf9, 0x26,
		    0xe3, 0x80, 0xa3, 0x29, 0x2d, 0x73, 0x51, 0x62,
		    0x7c, 0x64, 0x46, 0xf4, 0x34, 0x1a, 0xf6, 0xe1,
		    0xba, 0x3a, 0x0d, 0x82, 0x79, 0x0a, 0x5c, 0x16,
		    0x71, 0x49, 0x8e, 0xac, 0x8c, 0x9f, 0x35, 0x19,
		    0x45, 0x94, 0x3f, 0x56, 0x0c, 0x91, 0x00, 0x0b,
		    0xd7, 0xb0, 0xdd, 0x39, 0x66, 0xa1, 0x76, 0x52,
		    0x13, 0x57, 0xf3, 0xbb, 0x4e, 0xe5, 0xdc, 0xf0,
		    0x65, 0x84, 0xb2, 0xd6, 0xdf, 0x15, 0x3c, 0x63,
		    0x1d, 0x89, 0x14, 0xbd, 0xd2, 0x36, 0xfe, 0xb1,
		    0xca, 0x8b, 0xa4, 0xc6, 0x9e, 0x67, 0x47, 0x37,
		    0x42, 0x6d, 0x6a, 0x03, 0x92, 0x70, 0x05, 0x7d,
		    0x96, 0x2f, 0x40, 0x90, 0xc4, 0xf1, 0x3e, 0x3d,
		    0x01, 0xf7, 0x68, 0x1e, 0xc3, 0xfc, 0x72, 0xb5,
		    0x54, 0xcf, 0xe7, 0x41, 0xe4, 0x4d, 0x83, 0x55,
		    0x12, 0x22, 0x09, 0x78, 0xfa, 0xde, 0xa7, 0x06,
		    0x08, 0x23, 0xbf, 0x0f, 0xcc, 0xc1, 0x97, 0x61,
		    0xc5, 0x4a, 0xe6, 0xa0, 0x11, 0xc2, 0xea, 0x74,
		    0x02, 0x87, 0xd5, 0xd1, 0x9d, 0xb7, 0x7e, 0x38,
		    0x60, 0x53, 0x95, 0x8d, 0x25, 0x77, 0x10, 0x5e,
		    0x9b, 0x7f, 0xd8, 0x6e, 0xda, 0xa2, 0x2e, 0x20,
		    0x4f, 0xcd, 0x8f, 0xcb, 0xbe, 0x5a, 0xe0, 0xed,
		    0x2c, 0x9a, 0xd4, 0xe2, 0xaf, 0xd0, 0xa9, 0xe8,
		    0xad, 0x7a, 0xbc, 0xa8, 0xf2, 0xee, 0xeb, 0xf5,
		    0xa6, 0x99, 0x28, 0x24, 0x6c, 0x2b, 0x75, 0x5d,
		    0xf8, 0xd3, 0x86, 0x17, 0xfb, 0xc0, 0x7b, 0xb3,
		    0x58, 0xdb, 0xc7, 0x4b, 0xff, 0x04, 0x50, 0xe9,
		    0x88, 0x69, 0xc9, 0x2a, 0xab, 0xfd, 0x5b, 0x1b,
		    0x8a, 0xd9, 0xec, 0x27, 0x44, 0x0e, 0x33, 0xc8,
		    0x6b, 0x93, 0x32, 0x48, 0xb6, 0x30, 0x43, 0xa5
};
static void put32(unsigned char *p, uint32_t n)
{
    p[0] = (unsigned char)n; p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16); p[3] = (unsigned char)(n >> 24);
}
static void crypt(unsigned char *p, size_t n)
{
    unsigned char s[256];
    unsigned i = 0, j = 0;
    memcpy(s, initial_s, sizeof(s));
    while (n--) {
        unsigned char t;
        i = (i + 1) & 255; j = (j + s[i]) & 255;
        t = s[j]; s[j] = s[i]; s[i] = t;
        *p++ ^= s[(s[i] + s[j]) & 255];
    }
}
static unsigned char *encode(const unsigned char *data, size_t n, size_t *size)
{
    size_t pos = 124, off = 0, k;
    uint32_t a = 1, b = 0;
    unsigned char *p = calloc(1, 124 + 6 + n + (n / 65535 + 1) * 5);
    assert(p);
    memcpy(p, "CFU", 3); put32(p + 8, 124); put32(p + 12, (uint32_t)n);
    memcpy(p + 20, "BASLUS-21782Save1", 17);
    for (k = 0; k < 16; ++k) p[52 + k] = (unsigned char)(k + 1);
    put32(p + 76, 0x8427);
    p[pos++] = 0x78; p[pos++] = 0x01;
    do {
        size_t len = n - off;
        unsigned inv;
        if (len > 65535) len = 65535;
        p[pos++] = (unsigned char)(off + len == n);
        p[pos++] = (unsigned char)len; p[pos++] = (unsigned char)(len >> 8);
        inv = (unsigned)len ^ 65535u;
        p[pos++] = (unsigned char)inv; p[pos++] = (unsigned char)(inv >> 8);
        memcpy(p + pos, data + off, len); pos += len; off += len;
    } while (off < n);
    for (k = 0; k < n; ++k) { a = (a + data[k]) % 65521; b = (b + a) % 65521; }
    p[pos++] = (unsigned char)(b >> 8); p[pos++] = (unsigned char)b;
    p[pos++] = (unsigned char)(a >> 8); p[pos++] = (unsigned char)a;
    put32(p + 16, (uint32_t)(pos - 124));
    crypt(p + 124, pos - 124);
    *size = pos;
    return p;
}
static void clear(P4SaveArchive *a)
{
    size_t i;
    for (i = 0; i < a->count; ++i) free(a->files[i].data);
    memset(a, 0, sizeof(*a));
}
static void reject(const unsigned char *p, size_t n)
{
    P4SaveArchive a = {0}; char error[128];
    assert(!p4save_decode_cbs(p, n, &a, error, sizeof(error)));
    assert(error[0] && !a.count);
    clear(&a);
}
static void synthetic(void)
{
    unsigned char body[140] = {0};
    unsigned char *p, *bomb;
    size_t n, i;
    P4SaveArchive a = {0}; char error[128];
    for (i = 0; i < 16; ++i) body[i] = (unsigned char)(31 + i);
    put32(body + 16, 5); put32(body + 20, 0x8497);
    memcpy(body + 32, "save", 5); memcpy(body + 64, "hello", 5);
    put32(body + 69 + 16, 7); put32(body + 69 + 20, 0x8497);
    memcpy(body + 69 + 32, "icon.sys", 9); memcpy(body + 69 + 64, "fixture", 7);
    p = encode(body, sizeof(body), &n);
    assert(p4save_decode_cbs(p, n, &a, error, sizeof(error)));
    assert(!strcmp(a.name, "BASLUS-21782Save1") && a.count == 2);
    assert(!memcmp(a.created, p + 52, 8) && !memcmp(a.modified, p + 60, 8));
    assert(!strcmp(a.files[0].name, "save") && a.files[0].size == 5);
    assert(!memcmp(a.files[0].data, "hello", 5));
    assert(!memcmp(a.files[0].created, body, 8));
    assert(!memcmp(a.files[0].modified, body + 8, 8));
    assert(!strcmp(a.files[1].name, "icon.sys") && a.files[1].size == 7);
    assert(!memcmp(a.files[1].data, "fixture", 7));
    clear(&a);
    /* Alternate compressed-size convention used by real CBS writers. */
    put32(p + 16, (uint32_t)n);
    assert(p4save_decode_cbs(p, n, &a, error, sizeof(error))); clear(&a);
    for (i = 0; i < n; ++i) reject(p, i);
    p[0] ^= 1; reject(p, n); p[0] ^= 1;
    p[n - 1] ^= 1; reject(p, n); p[n - 1] ^= 1; /* Adler-32 */
    put32(p + 12, 8u * 1024u * 1024u + 4097u); reject(p, n);
    put32(p + 12, 1); reject(p, n); /* dishonest expansion size */
    free(p);
    /* Larger stream claiming only 64 output bytes: inflate must stop at 64,
     * not grow a buffer to satisfy the stream. Also exercise the hard cap. */
    bomb = calloc(1, 1024 * 1024); assert(bomb);
    p = encode(bomb, 1024 * 1024, &n); free(bomb);
    put32(p + 12, 64); reject(p, n); free(p);
    /* Per-record truncation, file type, unsafe name, and duplicate name. */
    put32(body + 16, 141); p = encode(body, sizeof(body), &n); reject(p, n); free(p);
    put32(body + 16, 5); put32(body + 20, 0x8427);
    p = encode(body, sizeof(body), &n); reject(p, n); free(p);
    put32(body + 20, 0x8497); memcpy(body + 32, "../x", 5);
    p = encode(body, sizeof(body), &n); reject(p, n); free(p);
    memcpy(body + 32, "save", 5); memset(body + 101, 0, 32); memcpy(body + 101, "save", 5);
    p = encode(body, sizeof(body), &n); reject(p, n); free(p);
}
static int private_file(const char *name)
{
    FILE *f = fopen(name, "rb"); long length;
    unsigned char *p;
    P4SaveArchive a = {0}; char error[160]; size_t i;
    if (!f) { perror(name); return 0; }
    assert(!fseek(f, 0, SEEK_END)); length = ftell(f);
    assert(length > 0 && length < 16 * 1024 * 1024);
    rewind(f); p = malloc((size_t)length); assert(p);
    assert(fread(p, 1, (size_t)length, f) == (size_t)length); fclose(f);
    if (!p4save_decode_cbs(p, (size_t)length, &a, error, sizeof(error))) {
        fprintf(stderr, "%s: %s\n", name, error); free(p); return 0;
    }
    printf("%s: %s (%zu files; zlib Adler-32 verified)\n", name, a.name, a.count);
    for (i = 0; i < a.count; ++i) printf("  %s: %zu bytes\n", a.files[i].name, a.files[i].size);
    clear(&a); free(p); return 1;
}
int main(int argc, char **argv)
{
    int i;
    synthetic();
    for (i = 1; i < argc; ++i) if (!private_file(argv[i])) return 1;
    puts("CBS decoder tests passed");
    return 0;
}
