/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI SVM (server manager) functions the decomp has no C for, translated from the retail asm.
 *
 * The decomp's src/cri/re4/svm.c owns the callback table (svm_svr_ftbl), the pass counters
 * (svm_exec_cnt) and the lock bookkeeping (svm_lock_level, svm_locking_type) as globals; these
 * functions use those globals by name so the natives and the translated asm see one state. The
 * retail table keeps a third word per slot (a name string, "Unknown" by default) that the decomp
 * model drops; nothing reads it.
 *
 * The lock and unlock callbacks are port state here: retail installs them from the ADXM thread
 * setup (func_004f0ec0 and func_004f0ed8), and the decomp's svm_lock_func is a file-local static.
 */
#include <stdint.h>

/* A slot of the decomp's table (a global of the decomp's svm.c, guest data): the callback is a guest code address and the
 * argument a guest address. */
typedef struct {
    uint32_t func;
    uint32_t obj;
} P4SvmSvrFunc;

_Static_assert(sizeof(P4SvmSvrFunc) == 8, "SVM server slot layout");

typedef struct {
    void (*func)(void *obj);
    void *obj;
} P4SvmVoidFunc;

#define P4_SVM_NUM_SVTYPE 8
#define P4_SVM_NUM_SVRFUNC 6

extern P4SvmSvrFunc svm_svr_ftbl[P4_SVM_NUM_SVTYPE][P4_SVM_NUM_SVRFUNC];
extern int32_t svm_exec_cnt[P4_SVM_NUM_SVTYPE];
extern volatile int32_t svm_lock_level;
extern volatile int32_t svm_locking_type;
extern void SVM_CallErr(const char *fmt, ...);

/* retail 0x00925e60 / 0x00925e68: {callback, argument} pairs */
P4SvmVoidFunc p4_svm_lock_cb;
P4SvmVoidFunc p4_svm_unlock_cb;

/* 0x004f0518: take the SVM lock of the given type through the installed callback. The type of
 * the outermost lock is remembered for the unlock check. Nothing happens with no callback. */
void func_004f0518(int32_t type)
{
    if (p4_svm_lock_cb.func != 0) {
        p4_svm_lock_cb.func(p4_svm_lock_cb.obj);
        if (svm_lock_level == 0) {
            svm_locking_type = type;
        }
        svm_lock_level++;
    }
}

/* 0x004f0578: leave the lock. On the outermost unlock the type must equal the one taken. */
void func_004f0578(int32_t type)
{
    if (p4_svm_unlock_cb.func != 0) {
        svm_lock_level--;
        if (svm_lock_level == 0) {
            if (svm_locking_type != type) {
                SVM_CallErr("2103102:SVM:svm_unlock:lock type miss match.(type org=%d, type now=%d)",
                            svm_locking_type, type);
            }
            svm_locking_type = 0;
        }
        p4_svm_unlock_cb.func(p4_svm_unlock_cb.obj);
    }
}

/* 0x004f0ef0: run the six callbacks of one server type in order and count the pass. Returns the
 * OR of their results. Retail also flags the type as executing around each call (a word per type
 * at 0x007439a8 that nothing in the decomp reads). */
int32_t SVM_ExecSvrFunc(int32_t svtype)
{
    const P4SvmSvrFunc *p = svm_svr_ftbl[svtype];
    int32_t ret = 0;
    int i;

    for (i = 0; i < P4_SVM_NUM_SVRFUNC; i++, p++) {
        if (p->func != 0) {
            ret |= ((int32_t (*)(void *))p4_fn_g2h_sig(p->func, P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(P4_G2H(p->obj));
        }
    }
    svm_exec_cnt[svtype]++;
    return ret;
}

/* 0x004f0ec0 and 0x004f0ed8: install the lock and the unlock callback (function, argument). The
 * callbacks are the port-side words that func_004f0518 and func_004f0578 call. */
void func_004f0ec0(void (*func)(void *obj), void *obj)
{
    p4_svm_lock_cb.func = func;
    p4_svm_lock_cb.obj = obj;
}

void func_004f0ed8(void (*func)(void *obj), void *obj)
{
    p4_svm_unlock_cb.func = func;
    p4_svm_unlock_cb.obj = obj;
}
