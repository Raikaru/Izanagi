/* ADXT start entry points and key helpers (src/cri/adxt_start.c): what each start records in the
 * handle, the lock wrappers, the key derivation and the sample adjust, over memory mapped at the
 * retail globals. Offsets, texts and orders are read from the retail asm (0x004d6ec0, 0x004d6d68,
 * 0x004d7080, 0x004d71b0, 0x004c31b8, 0x004cecc8, 0x004d5370).
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/cri/adxt_start.c"
#include "guest_test.h"

static int failures;
static char log_buf[2048];
static int log_len;

static void rec(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_len += vsnprintf(log_buf + log_len, sizeof log_buf - (size_t)log_len, fmt, ap);
    va_end(ap);
    if (log_len < (int)sizeof log_buf - 2) {
        log_buf[log_len++] = ' ';
        log_buf[log_len] = 0;
    }
}

static void clear_log(void)
{
    log_len = 0;
    log_buf[0] = 0;
}

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
#define CHECK_LOG(s) do { if (strcmp(log_buf, (s)) != 0) { printf("FAIL %s:%d log\n  got  %s\n  want %s\n", __FILE__, __LINE__, log_buf, (s)); failures++; } } while (0)

static int32_t lookup_rc;
static int32_t lookup_dir = 7, lookup_start = 100, lookup_len = 200;
static void *sj_ret = (void *)0x1234;
static int32_t ainf_len;

void func_004c54d8(void) { rec("lock"); }
void func_004c54e0(void) { rec("unlock"); }
void func_004c54a8(void) { rec("svmlock"); }
void func_004c54c0(void) { rec("svmunlock"); }
void func_004c6a98(const char *m) { rec("err(0x%x)", (unsigned)P4_H2G(m)); }
void func_004c6af0(const char *m, const char *a) { rec("err2(0x%x,%s)", (unsigned)P4_H2G(m), a); }
void func_004c6c68(int32_t a, int32_t b, char *buf, int32_t len) { snprintf(buf, (size_t)len, "%d %d", a, b); }
int32_t func_004c9a28(int32_t ptid, int32_t member, char *name, int32_t *dir, int32_t *start, int32_t *length)
{
    rec("lookup(%d,%d)", ptid, member);
    strcpy(name, "FILE.ADX");
    *dir = lookup_dir;
    *start = lookup_start;
    *length = lookup_len;
    return lookup_rc;
}
void func_004c4088(uint32_t adxb, uint32_t fn, uint32_t obj) { rec("cbdec(0x%x,%u,%u)", (unsigned)adxb, fn, obj); }
void func_004d3fa8(void *adxt, void *sj) { (void)adxt; rec("startsjd(0x%x)", (unsigned)(uintptr_t)sj); }
void func_004d65b8(void *adxt, int32_t sw) { (void)adxt; rec("lnksw(%d)", sw); }
void *func_004ed600(void *data, int32_t len) { rec("sjmem(%u,%d)", (unsigned)(uintptr_t)data, len); return sj_ret; }
int32_t func_004c5dc8(const uint8_t *ibuf, int32_t len, int32_t *ainflen, uint8_t *id, int16_t *vol, int16_t *pan)
{
    (void)ibuf; (void)vol; (void)pan;
    rec("ainf(%d)", len);
    *ainflen = ainf_len;
    memset(id, 0x5a, 16);
    return 0;
}
void ADXT_Stop(void *adxt) { (void)adxt; rec("stop"); }

static void test_starts(void)
{
    static uint8_t adxt[0xc8];
    char *scratch = p4t_galloc(64, 16);   /* the work name buffer the object points at: guest storage */

    memset(adxt, 0xaa, sizeof adxt);
    U32(adxt, 0xb0) = P4_H2G(scratch);
    clear_log();
    func_004d6ec0(0, "x", 0, 1);
    func_004d6ec0(adxt, 0, 0, 1);
    CHECK_LOG("err(0x759370) err(0x759370) ");

    clear_log();
    func_004d6f78(adxt, "BGM.ADX");
    CHECK_LOG("lock stop lnksw(0) unlock ");
    CHECK(strcmp(scratch, "BGM.ADX") == 0 && W32(adxt, 0xb4) == W32(adxt, 0xb0) && W32(adxt, 0xbc) == 0 && W32(adxt, 0xc0) == 0xfffff);
    CHECK(adxt[0xac] == 1 && W32(adxt, 0xb8) == 0 && adxt[1] == 1 && adxt[2] == 0);

    /* partition file: found with a stream controller */
    memset(adxt, 0, sizeof adxt);
    U32(adxt, 0xb0) = P4_H2G(scratch);
    W32(adxt, 8) = 0x55;
    clear_log();
    func_004d6d18(adxt, 3, 9);
    CHECK_LOG("lock stop lock lookup(3,9) unlock lnksw(0) unlock ");
    CHECK(W32(adxt, 0xb8) == 7 && W32(adxt, 0xbc) == 100 && W32(adxt, 0xc0) == 200 && adxt[2] == 1 && adxt[1] == 1 && adxt[0xac] == 1);
    CHECK(W32(adxt, 0xb4) == W32(adxt, 0xb0) && strcmp(scratch, "FILE.ADX") == 0);
    /* found without a stream controller: error text, code, status 6 */
    W32(adxt, 8) = 0;
    adxt[1] = 0;
    clear_log();
    func_004d6d68(adxt, 3, 9);
    CHECK(strstr(log_buf, "err2(0x759348,3 9)") != 0);
    CHECK(*(uint16_t *)(adxt + 0x60) == 0xffff && adxt[1] == 6);
    /* not found: nothing recorded */
    lookup_rc = -1;
    adxt[1] = 0;
    clear_log();
    func_004d6d68(adxt, 3, 9);
    CHECK_LOG("stop lock lookup(3,9) unlock ");
    CHECK(adxt[1] == 0);
    lookup_rc = 0;
    clear_log();
    func_004d6d68(0, 1, 2);
    CHECK_LOG("err(0x759318) ");

    /* memory */
    memset(adxt, 0, sizeof adxt);
    clear_log();
    func_004d7030(adxt, (void *)0x4000, 100);
    CHECK_LOG("lock stop svmlock sjmem(16384,100) startsjd(0x1234) lnksw(0) svmunlock unlock ");
    CHECK(adxt[2] == 2);
    sj_ret = 0;
    clear_log();
    func_004d7080(adxt, (void *)0x4000, 100);
    CHECK_LOG("stop svmlock sjmem(16384,100) svmunlock err(0x7593d0) ");
    clear_log();
    func_004d7080(adxt, 0, 100);
    func_004d7080(adxt, (void *)0x4000, -1);
    CHECK_LOG("err(0x7593a0) err(0x7593a0) ");
    sj_ret = (void *)0x1234;

    /* memory table entry: big-endian count at +4, offset of entry n at +8 + 8n */
    {
        static uint8_t acx[64];

        memset(acx, 0, sizeof acx);
        acx[4] = 0; acx[5] = 0; acx[6] = 0; acx[7] = 2;           /* two entries */
        acx[8 + 8 + 0] = 0; acx[8 + 8 + 1] = 0; acx[8 + 8 + 2] = 0; acx[8 + 8 + 3] = 0x30; /* entry 1 at +0x30 */
        memset(adxt, 0, sizeof adxt);
        clear_log();
        func_004d7160(adxt, acx, 1);
        {
            char want[160];

            snprintf(want, sizeof want, "lock stop svmlock sjmem(%u,1073741824) startsjd(0x1234) lnksw(0) svmunlock unlock ", (unsigned)(uintptr_t)(acx + 0x30));
            CHECK_LOG(want);
        }
        CHECK(adxt[2] == 2);
        clear_log();
        func_004d71b0(adxt, acx, 2);
        CHECK_LOG("stop ");
        func_004d71b0(adxt, 0, 0);
        clear_log();
        func_004d71b0(adxt, acx, -1);
        CHECK_LOG("err(0x759400) ");
        sj_ret = 0;
        clear_log();
        func_004d71b0(adxt, acx, 0);
        CHECK(strstr(log_buf, "err(0x759430)") != 0);
        sj_ret = (void *)0x1234;
    }
}

static int32_t ref_key(const char *s, int32_t n, uint32_t seed_addr)
{
    const int16_t *tab = (const int16_t *)P4_G2H(0x007575d8);
    const int16_t *mult = (const int16_t *)P4_G2H(0x007576d8);
    int32_t key = *(const int16_t *)P4_G2H(seed_addr);
    int32_t i;

    for (i = 0; i < n; i++) {
        int32_t t = key * mult[(int8_t)s[i]];

        key = tab[t % 1024];
    }
    return (int16_t)key;
}

static void test_keys(void)
{
    int16_t k0 = 1, k1 = 1, k2 = 1;
    static uint8_t adxb[0xb0];
    int i;
    uint32_t x = 12345;
    const char *s = "ab\xe9 key";

    /* the tables overlap in retail (the character multipliers are entries 0..255 of the key table and the
     * seeds are entries 256, 512 and 768): fill the whole range with arbitrary words */
    for (i = -1100; i < 1100; i++) {
        x = x * 1103515245u + 12345u;
        M16(0x007575d8 + i * 2) = (int16_t)(x >> 16);
    }
    M32(0x00714c1c) = 0;
    clear_log();
    func_004c31b8("", 0, &k0, &k1, &k2);
    CHECK_LOG("");
    CHECK(M32(0x00714c1c) == 1);          /* the first use initialises (0x004c3178 counts it) */
    func_004c31b8("", 0, &k0, &k1, &k2);
    CHECK(M32(0x00714c1c) == 1);
    CHECK(func_004c3178() == 0 && M32(0x00714c1c) == 2);
    /* a non-NULL string of length 0 still yields the seeds, a NULL one with no length yields 0 */
    CHECK(k0 == M16(0x007577d8) && k1 == M16(0x007579d8) && k2 == M16(0x00757bd8));
    func_004c31b8(0, 0, &k0, &k1, &k2);
    CHECK(k0 == 0 && k1 == 0 && k2 == 0);

    clear_log();
    func_004c31b8(s, (int32_t)strlen(s), &k0, &k1, &k2);
    CHECK_LOG("");
    CHECK(k0 == ref_key(s, (int32_t)strlen(s), 0x007577d8));
    CHECK(k1 == ref_key(s, (int32_t)strlen(s), 0x007579d8));
    CHECK(k2 == ref_key(s, (int32_t)strlen(s), 0x00757bd8));

    memset(adxb, 0, sizeof adxb);
    ADXB_SetExtString(adxb, s);
    CHECK(*(uint16_t *)(adxb + 0xa0) == (uint16_t)k0 && *(uint16_t *)(adxb + 0xa2) == (uint16_t)k1 && *(uint16_t *)(adxb + 0xa4) == (uint16_t)k2);
    func_004c3dd8(s);
    CHECK(M16(0x00714c34) == k0 && M16(0x00714c36) == k1 && M16(0x00714c38) == k2);
}

static void test_misc(void)
{
    static uint8_t sjd[0x110];

    memset(sjd, 0, sizeof sjd);
    clear_log();
    CHECK(ADXSJD_AdjustSmpl(sjd, 5) == 5 && W32(sjd, 0xa0) == 5 && W32(sjd, 0xa4) == 0);
    CHECK(ADXSJD_AdjustSmpl(sjd, -3) == -3 && W32(sjd, 0xa4) == 3 && W32(sjd, 0xa0) == 5);
    CHECK(ADXSJD_AdjustSmpl(sjd, 0) == 0 && W32(sjd, 0xa4) == 3);
    CHECK_LOG("svmlock svmunlock svmlock svmunlock svmlock svmunlock ");

    U32(sjd, 4) = 0x777;
    clear_log();
    func_004cec00(sjd, 5, 6);
    CHECK_LOG("cbdec(0x777,5,6) ");

    ainf_len = 0;
    clear_log();
    CHECK(adxt_GetDataIdFromMem((const uint8_t *)0x5000) == 0);
    CHECK_LOG("ainf(2048) ");
    ainf_len = 4;
    CHECK(adxt_GetDataIdFromMem((const uint8_t *)0x5000) == (uint8_t *)P4_G2H(0x00922e30));
    CHECK(((uint8_t *)P4_G2H(0x00922e30))[0] == 0x5a);
}

int main(void)
{
    if (p4t_guest_map(0x00714000, 0x4000) != 0 || p4t_guest_map(0x00756000, 0x3000) != 0 ||
        p4t_guest_map(0x00922000, 0x1000) != 0) {
        printf("cri_adxt_start: cannot map the retail globals\n");
        return 77;
    }
    test_starts();
    test_keys();
    test_misc();
    if (failures) {
        return 1;
    }
    printf("cri_adxt_start: ok\n");
    return 0;
}
