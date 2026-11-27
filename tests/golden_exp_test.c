/* Synthetic EXP section; no executable or game data. */
#include "p4pc/golden_exp.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <string.h>

int p4_golden_exp_test_read(const char *path, uint8_t out[396], char exe_digest[65],
                            const char *expected_exe, const char *expected_exp, char *reason, size_t capacity);

static void put32(uint8_t *p, unsigned value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static int fixture_write(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(bytes, 1, size, f);
    return fclose(f) || written != size ? -1 : 0;
}

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(int argc, char **argv)
{
    uint8_t section[800] = {0}, out[396];
    char digest[65], reason[128];
    if (argc == 2) {
        FILE *probe = fopen(argv[1], "rb");
        if (!probe) return 77;
        fclose(probe);
        CHECK(!p4_golden_exp_read(argv[1], out, digest, reason, sizeof reason));
        puts("golden_exp: pinned Steam executable accepted");
        return 0;
    }
    for (unsigned i = 0; i < 99; ++i) {
        unsigned value = i * 100;
        section[4 + i * 4] = (uint8_t)value;
        section[5 + i * 4] = (uint8_t)(value >> 8);
    }
    p4_content_digest(section + 4, 396, digest);
    CHECK(!p4_golden_exp_section(section, sizeof section, digest, out, reason, sizeof reason));
    CHECK(!memcmp(out, section + 4, sizeof out));
    CHECK(p4_golden_exp_section(section, sizeof section, "bad", out, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "digest"));
    {
        const char *path = "golden_exp_synthetic_fixture.tmp";
        uint8_t pe[1536] = {0};
        char exe_hash[65], actual_hash[65];
        pe[0] = 'M'; pe[1] = 'Z'; put32(pe + 60, 64);
        memcpy(pe + 64, "PE\0\0", 4);
        pe[68] = 0x64; pe[69] = 0x86; pe[70] = 1;
        memcpy(pe + 88, ".sdata", 6);
        put32(pe + 104, 512); put32(pe + 108, 512);
        put32(pe + 124, 0x40000040);
        memcpy(pe + 512, section, 400);
        p4_content_digest(pe, sizeof pe, exe_hash);
        CHECK(!fixture_write(path, pe, sizeof pe));
        CHECK(!p4_golden_exp_test_read(path, out, actual_hash, exe_hash, digest, reason, sizeof reason));
        CHECK(!strcmp(actual_hash, exe_hash) && !memcmp(out, section + 4, sizeof out));
        CHECK(p4_golden_exp_read(path, out, actual_hash, reason, sizeof reason) == -1);
        CHECK(strstr(reason, "digest"));
        CHECK(p4_golden_exp_test_read(path, out, actual_hash, exe_hash, "wrong", reason, sizeof reason) == -1);
        CHECK(strstr(reason, "digest"));
        /* Synthetic, independently hashed spans in both allowed data sections.
         * Raw bytes beyond a section's virtual extent are never a supplier. */
        pe[70] = 2;
        put32(pe + 96, 512);
        memcpy(pe + 128, ".xpdata", 7);
        put32(pe + 136, 256); put32(pe + 144, 256); put32(pe + 148, 1024);
        put32(pe + 164, 0x40000040);
        for (unsigned i = 0; i < 8; ++i) pe[512 + 420 + i] = (uint8_t)(i + 71);
        for (unsigned i = 0; i < 12; ++i) pe[1024 + 12 + i] = (uint8_t)(i + 91);
        char first_hash[65], second_hash[65], wrong_hash[65];
        uint8_t literal_bytes[20];
        p4_content_digest(pe + 512 + 420, 8, first_hash);
        p4_content_digest(pe + 1024 + 12, 12, second_hash);
        P4GoldenPEDataSpan spans[] = {
            {".sdata", first_hash, 420, 8, 0}, {".xpdata", second_hash, 12, 12, 8}
        };
        p4_content_digest(pe, sizeof pe, exe_hash);
        CHECK(!fixture_write(path, pe, sizeof pe));
        CHECK(!p4_golden_exp_test_image_read(path, out, actual_hash, exe_hash, digest,
            spans, 2, literal_bytes, sizeof literal_bytes, reason, sizeof reason));
        CHECK(!strcmp(actual_hash, exe_hash) && !memcmp(out, section + 4, sizeof out));
        CHECK(!memcmp(literal_bytes, pe + 512 + 420, 8));
        CHECK(!memcmp(literal_bytes + 8, pe + 1024 + 12, 12));
        memset(wrong_hash, 'f', 64); wrong_hash[64] = 0;
        spans[1].sha256 = wrong_hash;
        CHECK(p4_golden_exp_test_image_read(path, out, actual_hash, exe_hash, digest,
            spans, 2, literal_bytes, sizeof literal_bytes, reason, sizeof reason) == -1);
        CHECK(strstr(reason, "digest")); spans[1].sha256 = second_hash;
        spans[1].section = ".arch";
        CHECK(p4_golden_exp_test_image_read(path, out, actual_hash, exe_hash, digest,
            spans, 2, literal_bytes, sizeof literal_bytes, reason, sizeof reason) == -1);
        spans[1].section = ".xpdata";
        spans[1].output_offset = 4;
        CHECK(p4_golden_exp_test_image_read(path, out, actual_hash, exe_hash, digest,
            spans, 2, literal_bytes, sizeof literal_bytes, reason, sizeof reason) == -1);
        spans[1].output_offset = 8;
        put32(pe + 136, 16);
        p4_content_digest(pe, sizeof pe, exe_hash);
        CHECK(!fixture_write(path, pe, sizeof pe));
        CHECK(p4_golden_exp_test_image_read(path, out, actual_hash, exe_hash, digest,
            spans, 2, literal_bytes, sizeof literal_bytes, reason, sizeof reason) == -1);
        CHECK(strstr(reason, "file-backed"));
        put32(pe + 124, 0x60000040);
        p4_content_digest(pe, sizeof pe, exe_hash);
        CHECK(!fixture_write(path, pe, sizeof pe));
        CHECK(p4_golden_exp_test_read(path, out, actual_hash, exe_hash, digest, reason, sizeof reason) == -1);
        CHECK(strstr(reason, "non-executable"));
        CHECK(!remove(path));
    }
    memcpy(section + 404, section + 4, 396);
    CHECK(p4_golden_exp_section(section, sizeof section, digest, out, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "unique"));
    CHECK(p4_golden_exp_section(NULL, 0, digest, out, reason, sizeof reason) == -1);
    CHECK(p4_golden_exp_read("this-file-does-not-exist", out, digest, reason, sizeof reason) == -1);
    puts("golden_exp: ok");
    return 0;
}
