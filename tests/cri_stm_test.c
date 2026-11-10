/* ADXSTM / ADX server entry points (src/cri/adxstm_api.c): the field each inner reads or writes
 * and that every wrapper brackets its inner with the ADX lock pair. Offsets and the order are read from
 * the retail asm (0x004d20f0, 0x004d2160, 0x004d2bd8, 0x004d2ce0, 0x004d2d30, 0x004d3220, 0x004c6a70).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/cri/adxstm_api.c"
#include "guest_test.h"

static int failures;
static int locks, unlocks, svm_locks, svm_unlocks;
static void (*err_func)(void *, const char *);
static void *err_obj;

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

void func_004d18c8(void) { locks++; }
void func_004d18d0(void) { unlocks++; }
void func_004c54a8(void) { svm_locks++; }
void func_004c54c0(void) { svm_unlocks++; }
int32_t func_004e2360(const char *name) { return (int32_t)strlen(name); }
int32_t func_004d3220(uint8_t *adxt) { return (int8_t)adxt[2]; }
uint32_t func_004d2d70(uint32_t stm) { return U32(P4_G2H(stm), 0x14); }
void func_004f0e80(void (*f)(void *, const char *), void *o) { err_func = f; err_obj = o; }

int32_t adxstmf_execsvr_flg;
static uint8_t *binding;
static int fs_passes, handle_passes;
void ADXSTM_BindFileNw(void *stm, const char *name, void *dir, int32_t start, int32_t length)
{
    binding = stm;
    SETPP(stm, 0x50, name);
    SETPP(stm, 0x54, dir);
    W32(stm, 0x0c) = start;
    W32(stm, 0x14) = length;
    P8(stm, 0x45) = 1;
}
void ADXT_ExecFsSvr(void)
{
    if (++fs_passes == 2) {
        binding[0x45] = 0;
    }
}
int32_t func_004f1278(int32_t *flag)
{
    if (*flag != 0) return 0;
    *flag = 1;
    return 1;
}
void ADXSTMF_ExecHndl(void *stm)
{
    CHECK(adxstmf_execsvr_flg == 1);
    handle_passes++;
    P8(stm, 0x45) = 0;
}
static int32_t joint_num(void *sj, int32_t line)
{
    (void)sj;
    return line == 0 ? 1000 : 200;
}

int main(void)
{
    uint8_t *stm = p4t_galloc(0x60, 16);
    uint32_t a = 0, b = 0;

    /* start: empty range -> status 3, else 2; counters cleared, request flag set; limit lifted */
    memset(stm, 0xaa, 0x60);
    W32(stm, 0x14) = 0;
    CHECK(func_004d2128(stm) == 1);
    CHECK(stm[1] == 3 && stm[2] == 0 && W32(stm, 0x34) == 0 && W32(stm, 0x4c) == 0 && W32(stm, 0x24) == 0 && W32(stm, 0x28) == 0);
    CHECK(stm[0x47] == 1 && W32(stm, 0x5c) == 0xfffff);
    CHECK(locks == 1 && unlocks == 1 && svm_locks == 1 && svm_unlocks == 1);
    W32(stm, 0x14) = 9;
    func_004d2160(stm);
    CHECK(stm[1] == 2);
    CHECK(stm[3] == 0xaa && stm[0x46] == 0xaa);

    /* chunk limits: +0x1c first, +0x18 second */
    W32(stm, 0x1c) = 111;
    W32(stm, 0x18) = 222;
    CHECK(func_004d2b80(stm, &a, &b) == 1 && a == 111 && b == 222);
    CHECK(locks == 2 && unlocks == 2);

    CHECK(func_004d2c98(stm, 77) == 1 && W32(stm, 0x2c) == 77);
    W32(stm, 0x10) = 123456;
    CHECK(func_004d2cf8(stm) == 123456);
    CHECK(func_004d2d78("abcd") == 4);
    stm[2] = 0xfe;
    CHECK(func_004d31e8(stm) == -2);
    CHECK(locks == 6 && unlocks == 6);

    if (p4t_guest_map(0x00715000, 0x1000) != 0) {
        printf("cri_stm: cannot map the retail globals\n");
        return failures ? 1 : 77;
    }
    func_004c6a70(0x1234, 0x5678);
    CHECK(*(volatile uint32_t *)P4_G2H(0x00715ca0) == 0x1234 && *(volatile uint32_t *)P4_G2H(0x00715ca4) == 0x5678);
    CHECK((void *)err_func == P4_FN_G2H(0x1234) && err_obj == P4_G2H(0x5678));

    /* Blocking bind must actually pump until the bind flag clears. */
    memset(stm, 0, 0x60);
    func_004d1e58(stm, "VOL:FILE.ADX", P4_G2H(0x1234u), 7, 90);
    CHECK(fs_passes == 2 && stm[0x45] == 0);
    CHECK(P32(stm, 0x54) == 0x1234u && W32(stm, 0x0c) == 7);
    CHECK(func_004d2d38(stm) == 90);
    CHECK(func_004d21a0(stm, 5) == 1 && W32(stm, 0x5c) == 5 && stm[1] == 2);
    func_004d2dc8(stm, 0x123);
    CHECK(stm[0x44] == 0x23);
    {
        uint32_t *vt = p4t_galloc(10 * sizeof(uint32_t), 16);
        uint32_t *sj = p4t_galloc(sizeof(uint32_t), 16);
        vt[9] = p4_fn_h2g_sig((void *)joint_num, "ipi");
        sj[0] = P4_H2G(vt);
        CHECK(func_004d2fa8(stm, sj) == 1200);
        CHECK(P32(stm, 4) == P4_H2G(sj));
        CHECK(W32(stm, 0x18) == 1200 && W32(stm, 0x1c) == 1200 && W32(stm, 0x40) == 1200);
    }
    /* Lazy-open wait uses the same re-entry flag as the controller server. */
    P32(stm, 8) = 0;
    func_004d3228(stm);
    CHECK(handle_passes == 1 && adxstmf_execsvr_flg == 0 && stm[0x45] == 0);

    if (failures) {
        return 1;
    }
    printf("cri_stm: ok\n");
    return 0;
}
