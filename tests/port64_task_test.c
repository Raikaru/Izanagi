/* port64 prototype: the kernel task list (decomp/src/Kernel/sdkTask.c) on a 32-bit and a 64-bit host.
 *
 * The same scenario runs against the same unit, compiled two ways: as the i386 build compiles it (guest
 * address == host pointer), and rewritten by tools/port64/gptr_rewrite.py for x86_64 (guest memory is one
 * block, pointers in guest structs are u32 guest addresses, callbacks are table entries). The test writes a
 * trace of every task field and list state as guest addresses and callback names; tests/port64_task.sh
 * compares the two traces byte for byte.
 *
 * The scenario creates a tree of tasks (names, delays, update and destroy callbacks, children), runs frames
 * (callbacks return 0, -1, or the address of the next update function), destroys by name and by
 * hierarchy, and looks tasks up by name. Retail data is optional: with an ELF path in argv[1] the image is
 * loaded into guest memory first (the assert strings and the allocator slots then hold retail words, which
 * the test overwrites where it provides the function). Nothing here assumes sizeof(void *) == 4: guest
 * memory is reached through P4_G2H / P4_H2G (include/p4pc/guest.h). */
#include <elf.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/os.h"

/* The unit reads gp-relative globals, the allocator slots and strings as linker symbols at retail
 * addresses on the i386 build (the 64-bit build rewrites them to guest-block accesses). */
#ifndef P4_GUEST64
#define ABS(n, a) P4_ABSOLUTE_SYMBOL(n, a)
ABS(D_00710550, 0x00710550); ABS(D_00710568, 0x00710568); ABS(D_00710580, 0x00710580);
ABS(D_00710598, 0x00710598); ABS(D_007105B0, 0x007105B0); ABS(D_007105D0, 0x007105D0);
ABS(D_008873F8, 0x008873F8); ABS(jtbl_008873FC, 0x008873FC);
ABS(D_008BFF70, 0x008BFF70); ABS(D_008BFF74, 0x008BFF74); ABS(D_008BFFB0, 0x008BFFB0);
ABS(D_008C0240, 0x008C0240); ABS(D_008C02E0, 0x008C02E0);
ABS(iGpffffac5c, 0x00763D4C); ABS(iGpffffac60, 0x00763D50); ABS(iGpffffac68, 0x00763D58); ABS(iGpffffac70, 0x00763D60);
ABS(iGpffffb9ec, 0x00764ADC); ABS(iGpffffb9f0, 0x00764AE0); ABS(iGpffffb9f4, 0x00764AE4);
ABS(iGpffffb9f8, 0x00764AE8); ABS(iGpffffb9fc, 0x00764AEC); ABS(iGpffffba00, 0x00764AF0);
ABS(iGpffffba04, 0x00764AF4); ABS(iGpffffba08, 0x00764AF8); ABS(iGpffffba0c, 0x00764AFC);
ABS(iGpffffba10, 0x00764B00); ABS(iGpffffba14, 0x00764B04); ABS(iGpffffba18, 0x00764B08);
#endif

/* retail addresses the scenario reads */
enum {
    GP_LIST1 = 0x00764ADC, GP_LIST2 = 0x00764AF4, GP_LIST3 = 0x00764AE8,   /* list heads */
    GP_CUR = 0x00764B04, GP_PROC = 0x00764B00, GP_POOL = 0x00764B08,
    SLOT_ALLOC = 0x008873F8, SLOT_FREE = 0x008873FC,
    HEAP = 0x01000000                                                      /* the test's bump allocator */
};

/* the task unit: compiled (the i386 mirror unit or its x86_64 rewrite), or with P4_RUN_RETAIL the retail
 * instructions run by the R5900 interpreter (tools/mipsdiff/r5900.c) over the same guest memory */
#ifndef P4_RUN_RETAIL
void *kwlnTaskCreate(const void *name, int32_t prio, int32_t delay, int32_t freeDelay, void *update, void *destroy,
                     uint8_t *work);
void *kwlnTaskCreateChild(void *parent, const void *name, int32_t prio, int32_t delay, int32_t freeDelay, void *update,
                          void *destroy, uint8_t *work);
int32_t kwlnTaskRunFrame(void);
void kwlnTaskInitPool(int32_t n);
uint8_t kwlnTaskDestroyWithHierarchyByName(const char *name);
uint8_t kwlnTaskDestroyWithHierarchy(void *task);
void *kwlnTaskGetTaskByName(const char *name);
int32_t kwlnTaskGetState(void *task);
uint32_t kwlnTaskGetWork(void *task);
int32_t kwlnTaskIsLinked(void *task);
#endif

/* ---- guest memory helpers ---------------------------------------------------------------------------- */
static uint32_t rd32(uint32_t a) { uint32_t v; memcpy(&v, P4_G2H(a), 4); return v; }
static uint16_t rd16(uint32_t a) { uint16_t v; memcpy(&v, P4_G2H(a), 2); return v; }
static void wr32(uint32_t a, uint32_t v) { memcpy(P4_G2H(a), &v, 4); }

static int guest_map(void)
{
#ifdef P4_GUEST64
    return p4_guest_init();
#else
    const char *why;
    return p4_os_map_fixed(0x00100000u, 0x01F00000u, &why);
#endif
}

static int load_elf(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    Elf32_Ehdr eh;
    if (fread(&eh, sizeof eh, 1, f) != 1) return -1;
    for (int i = 0; i < eh.e_phnum; i++) {
        Elf32_Phdr ph;
        fseek(f, (long)(eh.e_phoff + (uint32_t)i * eh.e_phentsize), SEEK_SET);
        if (fread(&ph, sizeof ph, 1, f) != 1) return -1;
        if (ph.p_type != PT_LOAD || ph.p_vaddr < 0x100000u || ph.p_vaddr + ph.p_memsz > 0x02000000u) continue;
        fseek(f, (long)ph.p_offset, SEEK_SET);
        if (fread(P4_G2H(ph.p_vaddr), 1, ph.p_filesz, f) != ph.p_filesz) return -1;
    }
    fclose(f);
    return 0;
}

#ifdef P4_RUN_RETAIL
#include "r5900.h"

#define RT_GP 0x007690F0u
#define RT_STACK 0x01FF0000u
static R5900 cpu[8];
static int depth;
static int32_t func_003e1220_(uint32_t, uint32_t, uint32_t, uint32_t, void *, uint32_t);
static void kAssertFail_(const void *, uint32_t);
static void kAssertFailAt_(const void *, const void *, uint32_t);
typedef struct { const void *fn; char kind; } CbKind;   /* u: int (*)(u8 *), d: void (*)(u8 *), a: void *(*)(int, int), f: void (*)(int, void *) */
static CbKind cbs[16];
static int ncbs;
static void cb_kind(const void *fn, char kind) { cbs[ncbs].fn = fn; cbs[ncbs++].kind = kind; }

static int rt_hook(R5900 *c, uint32_t t, void *user)
{
    (void)user;
    uint32_t a0 = (uint32_t)c->r[4], a1 = (uint32_t)c->r[5], a2 = (uint32_t)c->r[6], a3 = (uint32_t)c->r[7];
    uint64_t ret = 0;
    switch (t) {
    case 0x003e1220: ret = (uint64_t)(int64_t)func_003e1220_(a0, a1, a2, a3, P4_G2H((uint32_t)c->r[8]), (uint32_t)c->r[9]); break;
    case 0x0046d730: kAssertFail_(P4_G2H(a0), a1); break;
    case 0x0046d740: kAssertFailAt_(P4_G2H(a0), P4_G2H(a1), a2); break;
    case 0x00440b68: break;                                                  /* the debug printf */
    case 0x0043f9c8: memset(P4_G2H(a0), (int)a1, a2); ret = a0; break;       /* memset, memcpy */
    case 0x0043f810: memcpy(P4_G2H(a0), P4_G2H(a1), a2); ret = a0; break;
    default: {
        const void *fn = P4_FN_G2H(t);
        for (int i = 0; i < ncbs; i++) {
            if (cbs[i].fn != fn) continue;
            if (cbs[i].kind == 'u') ret = (uint64_t)(int64_t)((int32_t (*)(uint8_t *))fn)(P4_G2H(a0));
            else if (cbs[i].kind == 'd') ((void (*)(uint8_t *))fn)(P4_G2H(a0));
            else if (cbs[i].kind == 'a') ret = P4_H2G(((void *(*)(int32_t, int32_t))fn)((int32_t)a0, (int32_t)a1));
            else ((void (*)(int32_t, void *))fn)((int32_t)a0, P4_G2H(a1));
            c->r[2] = ret;
            return 1;
        }
        fprintf(stderr, "retail code called %08x\n", t);
        exit(3);
    }
    }
    c->r[2] = ret;
    return 1;
}

/* one retail call; the callbacks the scenario provides re-enter, so each depth has its own CPU and stack */
static uint32_t rt(uint32_t entry, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    R5900 *c = &cpu[depth];
    memset(c, 0, sizeof *c);
    c->r[28] = RT_GP;
    c->r[29] = RT_STACK - (uint32_t)depth * 0x4000u;
    c->r[4] = a0; c->r[5] = a1; c->r[6] = a2; c->r[7] = a3; c->r[8] = a4; c->r[9] = a5; c->r[10] = a6; c->r[11] = a7;
    c->max_steps = 4000000;
    c->lo_pc = 0x00450F20u;
    c->hi_pc = 0x00452760u;
    c->hook = rt_hook;
    depth++;
    int rc = r5900_call(c, entry);
    depth--;
    if (rc != R5900_OK) {
        fprintf(stderr, "retail %08x: rc=%d pc=%08x fault=%s addr=%08x bad=%08x\n", entry, rc, c->pc_now, c->fault_why ? c->fault_why : "-",
                c->fault_addr, c->bad_word);
        exit(4);
    }
    return (uint32_t)c->r[2];
}

void kwlnTaskInitPool(int32_t n) { rt(0x00451CA0, (uint32_t)n, 0, 0, 0, 0, 0, 0, 0); }
int32_t kwlnTaskRunFrame(void) { return (int32_t)rt(0x00451CF0, 0, 0, 0, 0, 0, 0, 0, 0); }
void *kwlnTaskCreate(const void *name, int32_t prio, int32_t delay, int32_t freeDelay, void *update, void *destroy, uint8_t *work)
{
    return P4_G2H(rt(0x00451DE0, P4_H2G(name), (uint32_t)prio, (uint32_t)delay, (uint32_t)freeDelay, P4_FN_H2G(update), P4_FN_H2G(destroy),
                     P4_H2G(work), 0));
}
void *kwlnTaskCreateChild(void *parent, const void *name, int32_t prio, int32_t delay, int32_t freeDelay, void *update, void *destroy,
                          uint8_t *work)
{
    return P4_G2H(rt(0x00451FC0, P4_H2G(parent), P4_H2G(name), (uint32_t)prio, (uint32_t)delay, (uint32_t)freeDelay, P4_FN_H2G(update),
                     P4_FN_H2G(destroy), P4_H2G(work)));
}
uint8_t kwlnTaskDestroyWithHierarchyByName(const char *name) { return (uint8_t)rt(0x00452040, P4_H2G(name), 0, 0, 0, 0, 0, 0, 0); }
uint8_t kwlnTaskDestroyWithHierarchy(void *task) { return (uint8_t)rt(0x00452080, P4_H2G(task), 0, 0, 0, 0, 0, 0, 0); }
void *kwlnTaskGetTaskByName(const char *name) { return P4_G2H(rt(0x00452380, P4_H2G(name), 0, 0, 0, 0, 0, 0, 0)); }
int32_t kwlnTaskGetState(void *task) { return (int32_t)rt(0x004522D0, P4_H2G(task), 0, 0, 0, 0, 0, 0, 0); }
uint32_t kwlnTaskGetWork(void *task) { return rt(0x00452560, P4_H2G(task), 0, 0, 0, 0, 0, 0, 0); }
int32_t kwlnTaskIsLinked(void *task) { return (int32_t)rt(0x00452490, P4_H2G(task), 0, 0, 0, 0, 0, 0, 0); }

#define func_003e1220 func_003e1220_
#define kAssertFail kAssertFail_
#define kAssertFailAt kAssertFailAt_
#endif

/* ---- the trace --------------------------------------------------------------------------------------- */
static uint32_t trace_hash = 2166136261u;
static void emit(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void emit(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    puts(buf);
    for (const char *p = buf; *p; p++) trace_hash = (trace_hash ^ (uint8_t)*p) * 16777619u;
}

/* ---- the test's side of the seams ------------------------------------------------------------------- */
static uint32_t bump = HEAP;
static int frame_no;

static void *tst_alloc(int32_t pool, int32_t flags)
{
    uint32_t a = bump;
    bump += 0x60;                                /* 0x54-byte tasks, 16-byte steps */
    memset(P4_G2H(a), 0xA5, 0x60);               /* poison: the unit must initialise every field */
    emit("alloc pool=%x flags=%x -> %x", (unsigned)pool, (unsigned)flags, (unsigned)a);
    return P4_G2H(a);
}

static void tst_free(int32_t pool, void *p)
{
    emit("free pool=%x -> %x", (unsigned)pool, (unsigned)P4_H2G(p));
}

#ifdef P4_RUN_RETAIL
static
#endif
int32_t func_003e1220(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, void *a4, uint32_t a5)
{
    emit("pool create size=%x n=%x align=%x %x mem=%x flags=%x", a0, a1, a2, a3, (unsigned)P4_H2G(a4), a5);
    return 0x1234;
}

#ifndef P4_RUN_RETAIL
void func_00440b68(void) {}                      /* the debug printf; the scenario does not call the dump paths */
void kwlnTaskDetachChildren(void) {}
#endif
#ifdef P4_RUN_RETAIL
static
#endif
void kAssertFail(const void *file, uint32_t line) { emit("assert %x line %x", (unsigned)P4_H2G(file), line); }
#ifdef P4_RUN_RETAIL
static
#endif
void kAssertFailAt(const void *f1, const void *f2, uint32_t line)
{
    emit("assert %x %x line %x", (unsigned)P4_H2G(f1), (unsigned)P4_H2G(f2), line);
}

/* update callbacks: 0 keeps the task as it is, -1 ends a running task, anything else becomes the task's next update function
 * (the unit stores the result in the task) */
static int32_t upd_count(uint8_t *t);
static int32_t upd_chain(uint8_t *t);
static int32_t upd_next(uint8_t *t);
static int32_t upd_once(uint8_t *t);
static void dtor_a(uint8_t *t);
static void dtor_b(uint8_t *t);

typedef struct { const char *name; void *fn; } Named;
static const Named fns[] = {
    {"upd_count", (void *)upd_count}, {"upd_chain", (void *)upd_chain}, {"upd_next", (void *)upd_next},
    {"upd_once", (void *)upd_once}, {"dtor_a", (void *)dtor_a}, {"dtor_b", (void *)dtor_b},
    {"tst_alloc", (void *)tst_alloc}, {"tst_free", (void *)tst_free},
};

static const char *fn_name(uint32_t v)
{
    if (!v) return "-";
    for (size_t i = 0; i < sizeof fns / sizeof *fns; i++)
        if (P4_FN_H2G(fns[i].fn) == v) return fns[i].name;
    return "?";
}

static const char *task_name(uint8_t *t) { return (const char *)t; }

static int32_t upd_count(uint8_t *t)
{
    emit("  f%d upd_count %s work=%x", frame_no, task_name(t), (unsigned)kwlnTaskGetWork(t));
    return 0;
}

static int32_t upd_chain(uint8_t *t)
{
    emit("  f%d upd_chain %s -> upd_next", frame_no, task_name(t));
    return (int32_t)P4_FN_H2G(upd_next);
}

static int32_t upd_next(uint8_t *t)
{
    emit("  f%d upd_next %s", frame_no, task_name(t));
    return 0;
}

static int32_t upd_once(uint8_t *t)
{
    emit("  f%d upd_once %s", frame_no, task_name(t));
    return -1;
}

static void dtor_a(uint8_t *t) { emit("  dtor_a %s", task_name(t)); }
static void dtor_b(uint8_t *t) { emit("  dtor_b %s", task_name(t)); }

/* ---- dump -------------------------------------------------------------------------------------------- */
static void dump_task(uint32_t t)
{
    char name[0x19];
    memcpy(name, P4_G2H(t), 0x18);
    name[0x18] = 0;
    emit("    %08x \"%s\" sum=%d flags=%x pri=%x t24=%x t28=%x t2c=%x t2e=%x upd=%s dtor=%s work=%x ln=%x lp=%x lp2=%x par=%x ch=%x nx=%x",
         (unsigned)t, name, (int)rd32(t + 0x18), (unsigned)rd32(t + 0x1C), (unsigned)rd32(t + 0x20), (unsigned)rd32(t + 0x24),
         (unsigned)rd32(t + 0x28), rd16(t + 0x2C), rd16(t + 0x2E), fn_name(rd32(t + 0x30)), fn_name(rd32(t + 0x34)),
         (unsigned)rd32(t + 0x38), (unsigned)rd32(t + 0x3C), (unsigned)rd32(t + 0x40), (unsigned)rd32(t + 0x44),
         (unsigned)rd32(t + 0x48), (unsigned)rd32(t + 0x4C), (unsigned)rd32(t + 0x50));
}

static void dump_state(const char *what)
{
    emit("== %s: heads %x %x %x cur=%x proc=%x counts %d %d %d", what, (unsigned)rd32(GP_LIST1), (unsigned)rd32(GP_LIST2),
         (unsigned)rd32(GP_LIST3), (unsigned)rd32(GP_CUR), (unsigned)rd32(GP_PROC), (int)rd32(0x00764AE4), (int)rd32(0x00764AFC),
         (int)rd32(0x00764AF0));
    uint32_t heads[3] = {GP_LIST1, GP_LIST2, GP_LIST3};
    for (int l = 0; l < 3; l++) {
        emit("  list %d", l + 1);
        for (uint32_t t = rd32(heads[l]), n = 0; t && n < 64; t = rd32(t + 0x3C), n++) dump_task(t);
    }
}

int main(int argc, char **argv)
{
    if (guest_map() != 0) {
        fprintf(stderr, "cannot map guest memory\n");
        return 1;
    }
    if (argc > 1 && load_elf(argv[1]) != 0) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    /* the engine allocator slots the unit calls through (retail data holds function words here) */
    wr32(SLOT_ALLOC, P4_FN_H2G(tst_alloc));
    wr32(SLOT_FREE, P4_FN_H2G(tst_free));
    wr32(GP_LIST1, 0); wr32(GP_LIST2, 0); wr32(GP_LIST3, 0); wr32(GP_CUR, 0); wr32(GP_PROC, 0);

#ifdef P4_RUN_RETAIL
    cb_kind((void *)upd_count, 'u'); cb_kind((void *)upd_chain, 'u'); cb_kind((void *)upd_next, 'u'); cb_kind((void *)upd_once, 'u');
    cb_kind((void *)dtor_a, 'd'); cb_kind((void *)dtor_b, 'd'); cb_kind((void *)tst_alloc, 'a'); cb_kind((void *)tst_free, 'f');
#endif
    uint8_t *names = P4_G2H(0x01800000u);        /* names the test passes live in guest memory */
    const char *nm[] = {"root", "ui", "bg", "fx0", "fx1", "chain", "once", "leaf"};
    for (int i = 0; i < 8; i++) {
        strcpy((char *)names + i * 32, nm[i]);
    }
#define NAME(i) ((const void *)(names + (i) * 32))

    kwlnTaskInitPool(64);
    emit("pool word %x", (unsigned)rd32(GP_POOL));

    void *root = kwlnTaskCreate(NAME(0), 0x10, 0, 3, (void *)upd_count, (void *)dtor_a, (uint8_t *)P4_G2H(0x01810000u));
    void *ui = kwlnTaskCreateChild(root, NAME(1), 0x20, 0, 3, (void *)upd_count, (void *)dtor_b, (uint8_t *)P4_G2H(0x01810100u));
    void *bg = kwlnTaskCreate(NAME(2), 0x11, 2, 4, (void *)upd_count, (void *)dtor_a, (uint8_t *)P4_G2H(0x01810200u));
    void *fx0 = kwlnTaskCreateChild(ui, NAME(3), 0x30, 0, 3, (void *)upd_count, (void *)dtor_b, 0);
    void *fx1 = kwlnTaskCreateChild(ui, NAME(4), 0x31, 1, 3, (void *)upd_count, (void *)dtor_b, 0);
    void *chain = kwlnTaskCreate(NAME(5), 0x12, 0, 3, (void *)upd_chain, (void *)dtor_a, (uint8_t *)P4_G2H(0x01810300u));
    void *once = kwlnTaskCreate(NAME(6), 0x13, 0, 3, (void *)upd_once, (void *)dtor_b, 0);
    void *leaf = kwlnTaskCreateChild(root, NAME(7), 0x40, 5, 3, (void *)upd_count, 0, 0);
    emit("created root=%x ui=%x bg=%x fx0=%x fx1=%x chain=%x once=%x leaf=%x", (unsigned)P4_H2G(root), (unsigned)P4_H2G(ui),
         (unsigned)P4_H2G(bg), (unsigned)P4_H2G(fx0), (unsigned)P4_H2G(fx1), (unsigned)P4_H2G(chain), (unsigned)P4_H2G(once),
         (unsigned)P4_H2G(leaf));
    dump_state("after create");

    for (frame_no = 1; frame_no <= 10; frame_no++) {
        emit("-- frame %d", frame_no);
        kwlnTaskRunFrame();
        dump_state("frame");
        if (frame_no == 4) {
            emit("lookup fx1 -> %x state %d linked %d", (unsigned)P4_H2G(kwlnTaskGetTaskByName(NAME(4))),
                 (int)kwlnTaskGetState(fx1), (int)kwlnTaskIsLinked(fx1));
            emit("destroy by name fx0 -> %d", (int)kwlnTaskDestroyWithHierarchyByName(NAME(3)));
            dump_state("after destroy fx0");
        }
        if (frame_no == 7) {
            emit("destroy hierarchy ui -> %d", (int)kwlnTaskDestroyWithHierarchy(ui));
            dump_state("after destroy ui");
        }
    }
    emit("lookup missing -> %x", (unsigned)P4_H2G(kwlnTaskGetTaskByName(NAME(3))));
    emit("TRACE_HASH %08x", trace_hash);
    return 0;
}
