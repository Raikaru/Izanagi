/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI ADXM (thread server) functions the decomp has no C for, translated from the retail asm
 * (0x004cb510..0x004cc218) and checked instruction by instruction against it.
 *
 * ADXM_Setup (0x004cc218) turns the SVM into a threaded server: it saves the caller's thread
 * priority, creates four kernel threads (an idle counter thread, a file-system/main server thread,
 * a vsync-type thread and a reserve thread), installs the SVM lock callbacks and the border hook.
 * ADXM data is read and written at its retail addresses; the priorities live at 0x007199a0.. (see
 * ADXM_Setup for the defaults). The kernel calls are the retail libkernl entry points
 * (func_004215e0 CreateThread, func_00422920 StartThread, func_00421670 ChangeThreadPriority,
 * func_004216d0 GetThreadId, func_004216e0 ReferThreadStatus, func_00421750 SuspendThread,
 * func_00421770 ResumeThread, func_00421620 ExitThread, ...), which src/platform/kernel_thread.c
 * provides.
 *
 * The thread bodies end each pass with ADXM_WaitVsync (SleepThread). The two debug writes to the GS
 * background colour (0x120000e0, guarded by the profile word 0x00719ab4) are left out: that
 * register is not mapped on the host.
 */
#include <stdint.h>

#define M32(a) (*(volatile int32_t *)P4_G2H(a))
#define M16(a) (*(volatile uint16_t *)P4_G2H(a))
#define M64(a) (*(volatile uint64_t *)P4_G2H(a))

/* ThreadParam (guest layout, as kernel_thread.c reads it): the function is a guest code address, the stack and gp guest addresses. */
typedef struct {
    int32_t status;
    uint32_t function;
    uint32_t stack;
    int32_t stack_size;
    uint32_t gp;
    int32_t init_priority;
    int32_t cur_priority;
    uint32_t attr;
    uint32_t option;
} AdxmThreadParam;

_Static_assert(sizeof(AdxmThreadParam) == 36, "ThreadParam layout");

extern int32_t func_004215e0(void *param);                  /* CreateThread */
extern int32_t func_00421620(void);                          /* ExitThread */
extern int32_t func_00421670(int32_t id, int32_t prio);      /* ChangeThreadPriority */
extern int32_t func_004216d0(void);                          /* GetThreadId */
extern int32_t func_004216e0(int32_t id, void *status);      /* ReferThreadStatus */
extern int32_t func_004216f0(int32_t id, void *status);      /* iReferThreadStatus */
extern int32_t func_00421710(int32_t id);                    /* WakeupThread */
extern int32_t func_00421750(int32_t id);                    /* SuspendThread */
extern int32_t func_00421770(int32_t id);                    /* ResumeThread */
extern int32_t func_004226a8(int32_t id);
extern int32_t func_00422920(int32_t id, uint32_t arg);         /* StartThread */
extern int32_t func_00421700(void);                          /* SleepThread */
extern int32_t func_004f1068(void);                          /* SVM_ExecSvrFunc, vsync-type thread */
extern int32_t func_004f1080(void);                          /* SVM_ExecSvrFunc, fs/main server thread */
extern int32_t func_004f10b0(void);
extern int32_t func_004f10e0(void);
extern void func_004f0de8(int32_t svtype, void *func, void *obj); /* SVM_SetCbGotoSvrBorder */
extern void func_004f0ec0(void *func, void *obj);            /* SVM lock callback */
extern void func_004f0ed8(void *func, void *obj);            /* SVM unlock callback */
extern void func_004f07e0(const char *msg);                  /* SVM error report */
extern void func_004f11e0(void);                             /* SVM_Init */
extern int32_t func_004cc1e0(void);                          /* ADXM_IsSetupThrd */
void func_004cbd28(void);
void func_004cd1a0(void);
void func_004ccbf0(void);

#define ADXM_PRIO_LOCK 0x007199a0
#define ADXM_PRIO_IDLE 0x007199a4
#define ADXM_PRIO_FS 0x007199ac
#define ADXM_PRIO_T3 0x007199b0
#define ADXM_PRIO_MAIN 0x007199b4
#define ADXM_PRIO_VSYNC 0x007199b8
#define ADXM_INIT_CNT 0x00719994
#define ADXM_LOCK_CNT 0x007199d0
#define ADXM_TID_IDLE 0x00719a28
#define ADXM_TID_FS 0x00719a30
#define ADXM_TID_T3 0x00719a34
#define ADXM_TID_VSYNC 0x00719a3c
#define ADXM_TID_RESERVE 0x00719a40
#define ADXM_MAIN_TID 0x00719a38
#define ADXM_SAVED_PRIO 0x007212c0
#define ADXM_SAVED_TID 0x007212c4
#define ADXM_MAIN_PRIO 0x007212c8
#define ADXM_GP 0x007690f0

/* 0x004cc1f0: the ADXM is set up (init count > 0). */
int32_t func_004cc1f0(void)
{
    return M32(ADXM_INIT_CNT) != 0;
}

/* 0x004cb510: -1 selects a server type from the setup state: 2 when set up, else 1. */
int32_t func_004cb510(int32_t type)
{
    if (type == -1) {
        type = (func_004cc1f0() ^ 1) ? 1 : 2;
    }
    return type;
}

/* 0x004cbda8 suspend a thread unless it is already suspended (status 8) or waiting and suspended
 * (12). Thread status values are the libkernl ones: 1 run, 2 ready, 4 wait, 8 suspend, 12 both,
 * 16 dormant. */
int32_t func_004cbda8(int32_t tid)
{
    int32_t st[16];
    int32_t ret = 0;

    if (tid != 0) {
        func_004216e0(tid, st);
        if (st[0] != 8 && st[0] != 12) {
            ret = func_00421750(tid);
        }
    }
    return ret;
}

/* 0x004cbd40 resume a thread that is suspended (status 8 or 12). */
int32_t func_004cbd40(int32_t tid)
{
    int32_t st[16];
    int32_t ret = 0;

    if (tid != 0) {
        func_004216e0(tid, st);
        if (st[0] == 8 || st[0] == 12) {
            ret = func_00421770(tid);
        }
    }
    return ret;
}

/* 0x004cbc88 interrupt-context wake of a waiting thread (status 4 or 12) through func_004226a8,
 * after iReferThreadStatus. */
void func_004cbc88(int32_t tid)
{
    int32_t st[16];

    if (tid != 0) {
        func_004216f0(tid, st);
        if (st[0] == 4 || st[0] == 12) {
            func_004226a8(tid);
        }
    }
}

/* 0x004cbcd8 wake a waiting thread (status 4 or 12) with WakeupThread. */
void func_004cbcd8(int32_t tid)
{
    int32_t st[16];

    if (tid != 0) {
        func_004216e0(tid, st);
        if (st[0] == 4 || st[0] == 12) {
            func_00421710(tid);
        }
    }
}

/* 0x004cb548 SVM lock callback: the outermost lock raises the caller to the ADXM lock priority
 * (remembering the old one and its thread) and resumes the idle thread. */
void func_004cb548(void)
{
    if (M32(ADXM_LOCK_CNT) == 0) {
        int32_t tid = func_004216d0();

        M32(ADXM_SAVED_PRIO) = func_00421670(tid, M32(ADXM_PRIO_LOCK));
        M32(ADXM_SAVED_TID) = tid;
        func_004cbd40(M32(ADXM_TID_IDLE));
    }
    M32(ADXM_LOCK_CNT) = M32(ADXM_LOCK_CNT) + 1;
}

/* 0x004cb5d0 SVM unlock callback: the outermost unlock suspends the idle thread and restores the
 * caller's priority. */
void func_004cb5d0(void)
{
    M32(ADXM_LOCK_CNT) = M32(ADXM_LOCK_CNT) - 1;
    if (M32(ADXM_LOCK_CNT) == 0) {
        func_004cbda8(M32(ADXM_TID_IDLE));
        func_00421670(func_004216d0(), M32(ADXM_SAVED_PRIO));
    }
    if (M32(ADXM_LOCK_CNT) < 0) {
        M32(ADXM_LOCK_CNT) = 0;
    }
}

/* 0x004cb640 ADXM goto-border: with the server thread `tid` at the ADXM lock priority, wake it and
 * resume it until the border flag (0x007199d4) clears; about 200 million tries, then the
 * error text at 0x758910. The thread's priority is set to `prio` afterwards. */
void func_004cb640(int32_t tid, int32_t prio)
{
    int32_t n;

    if (tid == 0) {
        return;
    }
    M32(0x007199d4) = 1;
    func_00421670(tid, M32(ADXM_PRIO_LOCK));
    for (n = 0;;) {
        int32_t over;
        int32_t flag;

        func_004cbcd8(tid);
        func_004cbd40(tid);
        over = (0xbebc1ff < n);
        flag = M32(0x007199d4);
        n++;
        if (flag == 0) {
            break;
        }
        if (!over) {
            continue;
        }
        func_004f07e0((const char *)P4_G2H(0x00758910u));
        break;
    }
    func_00421670(tid, prio);
}

/* 0x004cb728, 0x004cb750: goto-border for the vsync-type and the reserve thread. */
void func_004cb728(void)
{
    func_004cb640(M32(ADXM_TID_VSYNC), M32(ADXM_PRIO_VSYNC));
}

void func_004cb750(void)
{
    func_004cb640(M32(ADXM_TID_RESERVE), M32(0x007199bc));
}

/* 0x004cb778: idle counter. */
void func_004cb778(void)
{
    M64(0x007199f0) = M64(0x007199f0) + 1;
}

/* 0x004cb790 idle thread: count until its exit word (0x00719a48) is set, then flag it done
 * (0x00719a50) and exit. The host scheduler switches threads only at kernel calls, so each count
 * goes through GetThreadId; retail relied on preemption. */
void func_004cb790(void)
{
    if (M64(0x00719a48) == 0) {
        do {
            func_004cb778();
            func_004216d0();
        } while (M64(0x00719a48) == 0);
    }
    M64(0x00719a50) = 1;
    func_00421620();
}

/* 0x004cb880 server thread (file system and main servers): each pass suspends the vsync-type
 * thread, runs the file-system server once per frame, resumes the reserve thread when threaded,
 * then runs the SVM server functions (flag 0x007199d8 set around them) and waits for vsync. */
void func_004cb880(void)
{
    if (M64(0x00719a68) == 0) {
        do {
            M64(0x00719a00) = M64(0x00719a00) + 1;
            func_004cbda8(M32(0x00719a3c));
            if (M32(0x00719ab8) == 0) {
                func_004cd1a0();
            }
            M32(0x00719ab8) = 0;
            if (func_004cc1e0() == 1) {
                func_004cbda8(M32(0x00719a40));
            }
            func_004ccbf0();
            M32(0x007199d8) = 1;
            func_004f1080();
            M32(0x007199d8) = 0;
            func_004cbd28();
        } while (M64(0x00719a68) == 0);
    }
    M64(0x00719a70) = 1;
    func_00421620();
}

/* 0x004cb988 third server thread: runs the SVM server functions of its type each pass (flag
 * 0x007199e0 set around them), then waits for vsync. */
void func_004cb988(void)
{
    if (M64(0x00719a78) == 0) {
        do {
            M64(0x00719a08) = M64(0x00719a08) + 1;
            M32(0x007199e0) = 1;
            func_004f10b0();
            M32(0x007199e0) = 0;
            if (M64(0x00719a78) != 0) {
                break;
            }
            func_004cbd28();
        } while (M64(0x00719a78) == 0);
    }
    M64(0x00719a80) = 1;
    func_00421620();
}

/* 0x004cba78 vsync-type thread: runs the SVM vsync server functions (flag 0x007199e4), then, unless
 * they asked to stop, services the border handshake with the server thread. */
void func_004cba78(void)
{
    if (M64(0x00719a88) == 0) {
        do {
            int32_t stop;

            M64(0x00719a10) = M64(0x00719a10) + 1;
            M32(0x007199e4) = 1;
            stop = func_004f10e0();
            M32(0x007199e4) = 0;
            if (M32(0x007199d4) == 1) {
                M32(0x007199d4) = 0;
                func_004cbd28();
            }
            if (stop == 0) {
                if (func_004cc1e0() == 1) {
                    int32_t tid = M32(0x00719a40);

                    if (func_004cbd40(tid) == M32(0x00719a40)) {
                        func_004cbcd8(M32(0x00719a40));
                    }
                }
                if (M32(0x007199d4) != 1) {
                    func_004cd1a0();
                    M32(0x00719ab8) = 1;
                }
                func_004cbd28();
            }
        } while (M64(0x00719a88) == 0);
    }
    M64(0x00719a90) = 1;
    func_00421620();
}

/* One ADXM thread: parameters as the retail code fills them (entry, stack, size, gp, initial
 * priority = the caller's priority saved by ADXM_Setup). Returns the thread id. */
static int32_t adxm_create(void *entry, uint32_t stack, int32_t size, int32_t *id_slot)
{
    AdxmThreadParam p = {0};

    p.function = p4_fn_h2g_sig(entry, "vi");   /* a thread entry: its argument is an integer word */
    p.stack = stack;
    p.stack_size = size;
    p.gp = ADXM_GP;
    p.init_priority = M32(ADXM_MAIN_PRIO);
    *id_slot = func_004215e0(&p);
    func_00422920(*id_slot, 0);
    return *id_slot;
}

/* 0x004cbe08: idle thread (2 KiB stack), started then suspended. */
void func_004cbe08(void)
{
    int32_t *slot = (int32_t *)P4_G2H(ADXM_TID_IDLE);

    adxm_create((void *)func_004cb790, 0x00719ac0, 2048, slot);
    if (*slot != 0) {
        func_004cbda8(*slot);
    }
    func_00421670(*slot, M32(ADXM_PRIO_IDLE));
}

/* 0x004cbf68: server thread (4 KiB stack). */
void func_004cbf68(void)
{
    int32_t *slot = (int32_t *)P4_G2H(ADXM_TID_FS);

    adxm_create((void *)func_004cb880, 0x0071b2c0, 4096, slot);
    func_00421670(*slot, M32(ADXM_PRIO_FS));
}

/* 0x004cbff0: third server thread (4 KiB stack). */
void func_004cbff0(void)
{
    int32_t *slot = (int32_t *)P4_G2H(ADXM_TID_T3);

    adxm_create((void *)func_004cb988, 0x0071c2c0, 4096, slot);
    func_00421670(*slot, M32(ADXM_PRIO_T3));
}

/* 0x004cc078: vsync-type thread (8 KiB stack), started then suspended. */
void func_004cc078(void)
{
    int32_t *slot = (int32_t *)P4_G2H(ADXM_TID_VSYNC);

    adxm_create((void *)func_004cba78, 0x0071d2c0, 8192, slot);
    func_00421670(*slot, M32(ADXM_PRIO_VSYNC));
    func_004cbda8(*slot);
}

/* 0x004cc218 ADXM_Setup(prm): first call initialises the SVM, installs the ADXM lock/unlock
 * callbacks, clears two word pairs at 0x00922e20/0x00922e28, loads the priorities (defaults 1, 8,
 * 16, 18, 24, 25 at 0x007199a0, +4, +0xc, +0x10, +0x14, +0x18, or from prm: +0, +4, +8, +0xc,
 * +0x10 for [0x14], +0x14 for [0x18]), saves the caller's priority, creates the four threads and
 * hooks the border handler on server type 6. Every call counts. */
void func_004cc218(int32_t *prm)
{
    int32_t st[16];
    int32_t tid;

    M32(0x007199ec) = 0;
    if (M32(ADXM_INIT_CNT) == 0) {

        func_004f11e0();
        func_004f0ec0((void *)func_004cb548, 0);
        func_004f0ed8((void *)func_004cb5d0, 0);
        M32(0x00922e24) = 0;
        M32(0x00922e20) = 0;
        M32(0x00922e2c) = 0;
        M32(0x00922e28) = 0;
        if (prm == 0) {
            M32(0x007199b8) = 25;
            M32(0x007199b4) = 24;
            M32(0x007199a0) = 1;
            M32(0x007199a4) = 8;
            M32(0x007199ac) = 16;
            M32(0x007199b0) = 18;
        } else {
            M32(0x007199b4) = prm[4];
            M32(0x007199a0) = prm[0];
            M32(0x007199a4) = prm[1];
            M32(0x007199ac) = prm[2];
            M32(0x007199b0) = prm[3];
            M32(0x007199b8) = prm[5];
        }
        tid = func_004216d0();
        M32(ADXM_MAIN_TID) = tid;
        func_004216e0(tid, st);
        M32(ADXM_MAIN_PRIO) = st[6];
        func_004cbe08();
        func_004cbf68();
        func_004cbff0();
        func_004cc078();
        func_00421670(M32(ADXM_MAIN_TID), M32(ADXM_PRIO_MAIN));
        func_004f0de8(6, (void *)func_004cb728, 0);
    }
    M32(ADXM_INIT_CNT) = M32(ADXM_INIT_CNT) + 1;
}

extern void func_004f0e80(void (*func)(void *obj, const char *msg), void *obj);   /* SVM_SetCbErr */

/* 0x004cc200: ADXM_SetCbErr, a tail call to the SVM error-callback setter. */
void func_004cc200(void (*func)(void *obj, const char *msg), void *obj)
{
    func_004f0e80(func, obj);
}

/* 0x004cbd28: sleep the calling thread until woken (the thread loops' vsync wait). */
void func_004cbd28(void)
{
    func_00421700();
}

/* 0x004cd1a0: call the registered server callback (function at 0x00922e28, argument at +4). */
void func_004cd1a0(void)
{
    void (*cb)(void *) = (void (*)(void *))p4_fn_g2h_sig(M32(0x00922e28), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0));

    if (cb != 0) {
        cb((void *)P4_G2H(M32(0x00922e2c)));
    }
}

/* 0x004ccbf0: complete the ADXM_WaitVsync handshake: when the caller armed 0x007199ec and its
 * thread (0x00719a38) is waiting, wake it and clear the word. */
void func_004ccbf0(void)
{
    int32_t st[16];

    if (M32(0x007199ec) == 1) {
        func_004216e0(M32(0x00719a38), st);
        if (st[0] == 4 || st[0] == 12) {
            if (func_00421710(M32(0x00719a38)) == M32(0x00719a38)) {
                M32(0x007199ec) = 0;
            }
        }
    }
}

extern int32_t func_004f1050(void);                          /* SVM_ExecSvrFunc, type 0..7 */
extern int32_t func_004f1098(void);
extern int32_t func_004f10c8(void);
extern int32_t func_004f10f8(void);
extern void func_004f1350(void);
extern void func_004f1380(void);
extern void ADXM_WaitVsync(void);

/* 0x004cb3f8: run the eight server types one after the other, in type order. */
int32_t func_004cb3f8(void)
{
    func_004f1050();
    func_004f1068();
    func_004f1080();
    func_004f1098();
    func_004f10b0();
    func_004f10c8();
    func_004f10e0();
    func_004f10f8();
    return 0;
}

/* 0x004cb360: the vsync-side server pass. Server mode 2 (threaded) runs type 0; mode 3 runs types 0
 * to 3 and the main type (5); mode 1 and anything else run nothing. */
int32_t func_004cb360(void)
{
    int32_t mode = func_004cb510(M32(0x00719980));

    if (mode == 2) {
        func_004f1050();
    } else if (mode == 3) {
        func_004f1050();
        func_004f1068();
        func_004f1080();
        func_004f1098();
        func_004f10c8();
    }
    return 0;
}

/* 0x004cb2d0: the main-loop server pass by mode: 1 runs every type (0x004cb3f8), 2 the main type
 * (5), 3 types 6 and 7. */
int32_t func_004cb2d0(void)
{
    int32_t mode = func_004cb510(M32(0x00719980));

    if (mode == 1) {
        func_004cb3f8();
    } else if (mode == 2) {
        func_004f10c8();
    } else if (mode == 3) {
        func_004f10e0();
        func_004f10f8();
    }
    return 0;
}

/* 0x004ccb68: after a vsync, wake the vsync-type thread (0x00719a3c) and, when the ADXM setup is
 * threaded, the second one (0x00719a40): a thread func_004cbd40 reports as just resumed is woken. */
void func_004ccb68(void)
{
    if (func_004cbd40(M32(0x00719a3c)) == M32(0x00719a3c)) {
        func_004cbcd8(M32(0x00719a3c));
    }
    if (func_004cc1e0() == 1 && func_004cbd40(M32(0x00719a40)) == M32(0x00719a40)) {
        func_004cbcd8(M32(0x00719a40));
    }
}

/* 0x004cd170: call the registered vsync callback (function at 0x00922e20, argument at +4). */
void func_004cd170(void)
{
    void (*cb)(void *) = (void (*)(void *))p4_fn_g2h_sig(M32(0x00922e20), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0));

    if (cb != 0) {
        cb((void *)P4_G2H(M32(0x00922e24)));
    }
}

/* 0x004ccc98: the vsync server entry: the SVM lock pair brackets waking the threads, the vsync
 * callback and the WaitVsync handshake. */
void func_004ccc98(void)
{
    func_004f1350();
    func_004ccb68();
    func_004cd170();
    ADXM_WaitVsync();
    func_004f1380();
}

/* 0x004cca10: the ADXM vsync interrupt body. Counts the vsync (0x007188b8); unless it is already
 * running (0x00719aac), marks the pass, resumes the server threads whose loop is not already in
 * progress (0x007199d8, 0x007199e0, and with the threaded setup 0x007199dc) and counts the pass
 * (64-bit, 0x00719a20); runs the vsync-side server pass; then forwards `arg` to the user callback
 * at 0x00719ab0 and returns its result (0 with none). */
int32_t func_004cca10(int32_t arg)
{
    M32(0x007188b8) = M32(0x007188b8) + 1;
    if (M32(0x00719aac) == 0) {
        M32(0x00719aac) = 1;
        if (M32(0x00719994) > 0) {
            if (M32(0x007199d8) != 1) {
                func_004cbc88(M32(0x00719a30));
            }
            if (M32(0x007199e0) != 1) {
                func_004cbc88(M32(0x00719a34));
            }
            if (func_004cc1e0() == 1 && M32(0x007199dc) != 1) {
                func_004cbc88(M32(0x00719a2c));
            }
            M64(0x00719a20) = M64(0x00719a20) + 1;
        }
        func_004cb360();
        M32(0x00719aac) = 0;
    }
    if (M32(0x00719ab0) == 0) {
        return 0;
    }
    return ((int32_t (*)(int32_t))p4_fn_g2h_sig(M32(0x00719ab0), P4_SIG(1, 0, 0, 0, 0, 0, 0, 0)))(arg);
}
