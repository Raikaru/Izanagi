/* ovdiff --replay: run the port's native on the inputs the retail game really passed, and compare with what
 * retail returned and wrote.
 *
 * Included by ovdiff.c (it uses that file's statics) and entered from main() through `replay_function`.
 *
 *   ovdiff -R golden.jsonl [-W windows.jsonl] [-G] [-o report.tsv] <SLUS_217.82> <addr|all|@file>...
 *
 * golden.jsonl is written by the PCSX2 hook of the private trace tooling (--hooks tools/mipsdiff/sigs.txt) from headless retail runs:
 * one line per recorded call of a hooked override, at the function's entry and again at its return.
 *
 *   {"addr":"0x00143ba0","name":"gsUploadRasterTiles","call":N,"frame":F,"sp":"0x01f...",
 *    "args":[a0,a1,a2,a3,t0,t1,t2,t3], "fargs":[f12..f19 as bits], "v0":u32,"v0h":u32,"f0":bits,"win":[pre,post],
 *    "mem":[{"a":addr,"k":"p"|"s","pre":"hex","post":"hex"}, ...]}
 *
 * mem: bounded snapshots, 256 bytes at every argument that points into the data range, 128 bytes at each pointer found
 * inside those (one level), and 64 bytes at sp (k "s": the caller's outgoing stack words, not a result). A replay writes
 * every "p" region's pre bytes at its real address (the harness maps guest RAM identity), calls the native with the
 * arguments laid out as callee_slots() does (non-float arguments from a0.., 'f' arguments from f12.., positionally
 * as sigs.txt gives them), then compares the return value by the signature's type and every "p" region against its post
 * bytes (fcmp/wcmp: pointers and small values exactly, other words as floats within the host tolerance).
 *
 * windows.jsonl (with -G) records its two explicit address/length ranges and concatenated bytes at entry:
 * {"id":n,"w":[[addr,len],[addr,len]],"hex":"..."}. Captures include both the full RenderWare page and a narrower
 * engine-global range; the ranges, not a hardcoded 8192-byte layout, determine where each byte belongs.
 * A replay restores those globals except the pinned allocator slots 0x008873E8..0x008873F7.
 * Without -G only argument regions (including caller-stack inputs) are restored; replays are normally run with -G.
 *
 * What it cannot see: globals outside the windows, memory reached through pointers deeper than the second level or beyond
 * the snapshot sizes, and writes outside the recorded regions. A replay verdict is evidence for the recorded inputs, not
 * a proof; a mismatch is always worth reading (-v prints the first differing words).
 * A 64-bit result needs v0h (bits 32..63 of the EE's $v0). A legacy recording without it can expose a low-word or memory
 * mismatch, but cannot prove a complete match and is otherwise inconclusive; $v1 is not a substitute.
 */

typedef struct {
    uint32_t addr, size;
    int stack;
    uint8_t *pre, *post;
} RMem;

typedef struct {
    uint32_t addr, call, frame, sp;
    uint32_t args[8], fargs[8], v0, v0h, f0;
    int has_v0h;                     /* bits 32..63 of $v0, recorded since the 64-bit returns were found to sit there */
    int win_pre;
    uint32_t vf[33][4];              /* VU0 vf0..vf31 and the accumulator at entry (zero when not recorded) */
    int has_vf;
    unsigned nmem;
    RMem *mem;
} RRec;

static RRec *g_rrec;
static size_t g_nrrec;
typedef struct {
    uint8_t *bytes;
    uint32_t address[2], length[2];
} RWindow;
static RWindow *g_win;            /* recorded address ranges and concatenated bytes, by id */
static size_t g_nwin;
static int g_use_globals;
static const char *g_peek;       /* -P: addresses to print */
static int g_replay_interp;      /* -E: replay through the interpreter (see replay_interp) */

static int hexval(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }

/* Decodes the hex string at `p` (ended by a quote) into a fresh buffer; *n gets the byte count. */
static uint8_t *hex_dec(const char *p, uint32_t *n) {
    const char *e = strchr(p, '"');
    if (!e) return NULL;
    uint32_t len = (uint32_t)(e - p) / 2;
    uint8_t *b = malloc(len ? len : 1);
    for (uint32_t i = 0; i < len; i++) b[i] = (uint8_t)(hexval(p[2 * i]) << 4 | hexval(p[2 * i + 1]));
    *n = len;
    return b;
}

static const char *jkey(const char *line, const char *key) {
    char pat[40];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return NULL;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

static void jnums(const char *p, uint32_t *out, int n) {
    for (int i = 0; i < n; i++) {
        while (*p == '[' || *p == ',' || *p == ' ') p++;
        out[i] = (uint32_t)strtoull(p, (char **)&p, 10);
    }
}

/* replay_expected.txt: `<addr8> reason` for functions whose replay result is a documented harness limit, not a native bug
 * (kernel syscall wrappers return kernel state; a function that reads the IOP/INTC; retail heap layout). A mismatch there
 * is reported as "documented difference: reason" and does not fail the run. */
static Impure g_rexp[1024];
static size_t g_nrexp;

static void load_replay_expected(void) {
    char path[600];
    snprintf(path, sizeof path, "%s/replay_expected.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff");
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f) && g_nrexp < 1024) {
        if (line[0] == '#' || line[0] == '\n') continue;
        Impure *im = &g_rexp[g_nrexp];
        memset(im, 0, sizeof *im);
        char tok[64], why[128] = "";
        if (sscanf(line, "%63s %127[^\n]", tok, why) < 1) continue;
        im->addr = (uint32_t)strtoul(tok, NULL, 16);
        snprintf(im->why, sizeof im->why, "%s", why);
        g_nrexp++;
    }
    fclose(f);
}

static const char *rexpected_reason(uint32_t addr) {
    for (size_t i = 0; i < g_nrexp; i++) if (g_rexp[i].addr == addr) return g_rexp[i].why;
    return NULL;
}

/* The golden file can be gigabytes (whole pages), more than a 32-bit process can hold: every function's child reads only
 * its own lines, and the parent never parses the memory snapshots. */
static const char *g_golden_file;
_Static_assert(sizeof(off_t) >= 8, "golden recording offsets require large-file support");
typedef struct { uint32_t addr; off_t off; } GoldIndex;
static GoldIndex *g_gindex;
static size_t g_ngindex;

/* One pass over the file at start: the address and offset of every line (the parent keeps nothing else). */
static void load_golden(const char *path) {
    load_replay_expected();
    g_golden_file = path;
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char *line = NULL;
    size_t cap = 0, icap = 0;
    off_t off = ftello(f);
    while (getline(&line, &cap, f) > 0) {
        const char *p = strstr(line, "\"addr\":\"0x");
        if (p) {
            if (g_ngindex == icap) { icap = icap ? icap * 2 : 8192; g_gindex = realloc(g_gindex, icap * sizeof *g_gindex); }
            g_gindex[g_ngindex].addr = (uint32_t)strtoul(p + 10, NULL, 16);
            g_gindex[g_ngindex++].off = off;
        }
        off = ftello(f);
        if (off < 0) { perror("golden recording offset"); exit(2); }
    }
    free(line);
    fclose(f);
}

static void load_records(uint32_t only) {
    FILE *f = fopen(g_golden_file, "r");
    if (!f) { perror(g_golden_file); exit(2); }
    char *line = NULL;
    size_t cap = 0, ncap = 0;
    for (size_t gi = 0; gi < g_ngindex; gi++) {
        if (g_gindex[gi].addr != only) continue;
        if (fseeko(f, g_gindex[gi].off, SEEK_SET) != 0) {
            perror("golden recording seek");
            exit(2);
        }
        if (getline(&line, &cap, f) <= 0) break;
        const char *p;
        if (!(p = jkey(line, "addr"))) continue;
        RRec r;
        memset(&r, 0, sizeof r);
        r.addr = (uint32_t)strtoul(p + 3, NULL, 16);          /* "0x%08x" */
        if ((p = jkey(line, "call"))) r.call = (uint32_t)strtoul(p, NULL, 10);
        if ((p = jkey(line, "frame"))) r.frame = (uint32_t)strtoul(p, NULL, 10);
        if ((p = jkey(line, "sp"))) r.sp = (uint32_t)strtoul(p + 3, NULL, 16);        /* "0x%08x" */
        if ((p = jkey(line, "args"))) jnums(p, r.args, 8);
        if ((p = jkey(line, "fargs"))) jnums(p, r.fargs, 8);
        if ((p = jkey(line, "v0"))) r.v0 = (uint32_t)strtoul(p, NULL, 10);
        if ((p = jkey(line, "v0h"))) { r.v0h = (uint32_t)strtoul(p, NULL, 10); r.has_v0h = 1; }
        if ((p = jkey(line, "f0"))) r.f0 = (uint32_t)strtoul(p, NULL, 10);
        r.win_pre = -1;
        if ((p = jkey(line, "vf")) && *p == '"') {
            uint32_t n = 0;
            uint8_t *v = hex_dec(p + 1, &n);
            if (v && n == sizeof r.vf) { memcpy(r.vf, v, n); r.has_vf = 1; }
            free(v);
        }
        if ((p = jkey(line, "win"))) r.win_pre = (int)strtol(p + 1, NULL, 10);
        const char *m = jkey(line, "mem");
        unsigned nm = 0, mcap = 0;
        while (m && (m = strstr(m, "{\"a\":"))) {
            RMem x;
            memset(&x, 0, sizeof x);
            x.addr = (uint32_t)strtoul(m + 5, NULL, 10);
            const char *k = strstr(m, "\"k\":\"");
            const char *pre = strstr(m, "\"pre\":\"");
            const char *post = strstr(m, "\"post\":\"");
            if (!k || !pre || !post) break;
            x.stack = k[5] == 's';
            uint32_t n1 = 0, n2 = 0;
            x.pre = hex_dec(pre + 7, &n1);
            x.post = hex_dec(post + 8, &n2);
            x.size = n1 < n2 ? n1 : n2;
            if (nm == mcap) { mcap = mcap ? mcap * 2 : 8; r.mem = realloc(r.mem, mcap * sizeof *r.mem); }
            r.mem[nm++] = x;
            m = post + 8;
        }
        r.nmem = nm;
        if (g_nrrec == ncap) { ncap = ncap ? ncap * 2 : 4096; g_rrec = realloc(g_rrec, ncap * sizeof *g_rrec); }
        g_rrec[g_nrrec++] = r;
    }
    free(line);
    fclose(f);
}

static void invalid_window(const char *path, size_t line, const char *why) {
    fprintf(stderr, "ovdiff: %s:%zu: invalid recorded window: %s\n", path, line, why);
    exit(2);
}

static void load_windows(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char *line = NULL;
    size_t cap = 0, wcap = 0, lineno = 0;
    while (getline(&line, &cap, f) > 0) {
        lineno++;
        const char *id = jkey(line, "id"), *hex = jkey(line, "hex"), *ranges = jkey(line, "w");
        RWindow w = {0};
        int consumed = 0;
        if (!id || !hex || *hex != '"' || !ranges ||
            sscanf(ranges, " [ [ %u , %u ] , [ %u , %u ] ] %n",
                   &w.address[0], &w.length[0], &w.address[1], &w.length[1], &consumed) != 4 || !consumed)
            invalid_window(path, lineno, "expected two explicit [address,length] ranges and hex bytes");
        for (unsigned i = 0; i < 2; i++)
            if (!w.length[i] || ((w.address[i] | w.length[i]) & 3) ||
                w.address[i] < RAM_LO || w.address[i] >= RAM_HI || w.length[i] > RAM_HI - w.address[i])
                invalid_window(path, lineno, "range is not aligned guest memory");
        size_t n = (size_t)strtoul(id, NULL, 10);
        if (n < wcap && g_win[n].bytes) invalid_window(path, lineno, "duplicate id");
        uint32_t len = 0;
        w.bytes = hex_dec(hex + 1, &len);
        if (!w.bytes || len != w.length[0] + w.length[1])
            invalid_window(path, lineno, "byte count does not match recorded ranges");
        if (n >= wcap) { size_t nc = n + 256; g_win = realloc(g_win, nc * sizeof *g_win); memset(g_win + wcap, 0, (nc - wcap) * sizeof *g_win); wcap = nc; }
        g_win[n] = w;
        if (n + 1 > g_nwin) g_nwin = n + 1;
    }
    free(line);
    fclose(f);
}

static int pinned(uint32_t a) {
    return a >= SLOT_ALLOC && a < SLOT_ALLOC + 16;
}

static void apply_window(const RRec *r);

/* Guest ranges the harness uses itself (the arena with its data and the allocator pool; the libc heap sits above guest RAM
 * in a replay): a recorded region there would overwrite harness state, so it is neither written nor compared. */
static int harness_owned(uint32_t a) {
    return a >= ARENA && a < POOL_END;
}

/* What fill_arena pins that a replay keeps: the engine allocator slots (the camera pointer is retail's own: the camera object
 * is one of the pages the discovery records). */
static void repin(void) {
    ov_install_slots((uint32_t *)GH(SLOT_ALLOC));
}

/* gen_port patches function-pointer words in the data to host pointers (above guest RAM). A recorded word holding the retail
 * address of code there is the same pointer: loading it would make the native call guest code, comparing it would flag
 * the host pointer the native stores. */
static int retail_code_ptr(uint32_t w) { return w >= RAM_LO && w < 0x00530000u && !(w & 3); }
#ifdef P4_GUEST64
/* On a 64-bit host a function word the port stores is the guest code address of a function of the table (a retail address, or a handle above
 * RAM): what i386 compares as a host pointer. */
static int host_ptr_word(uint32_t w) { return w >= RAM_HI || (retail_code_ptr(w) && p4_fn_known(w)); }
#else
static int host_ptr_word(uint32_t w) { return w >= RAM_HI; }
#endif

static void copy_words(uint32_t addr, const uint8_t *src, uint32_t size) {
    uint32_t i = 0;
    if (addr & 3) { memcpy(GH(addr), src, size); return; }
    for (; i + 4 <= size; i += 4) {
        uint32_t w, cur;
        memcpy(&w, src + i, 4);
        memcpy(&cur, GH(addr + i), 4);
        if (retail_code_ptr(w) && host_ptr_word(cur)) continue;         /* keep the patched host pointer */
        if (retail_code_ptr(w)) {                                        /* a function pointer in recorded data (engine vtables): the port's function at that address */
            const p4pc_native_t *fn = find_native(w);
            if (fn) w = impure_reason(fn->addr, fn->name) ? ov_word(ov_cb, "iiiii") : P4_FN_H2G(fn->fn);   /* an impure native (OpenGL draw) is the logging callback, as in the case runs */
        }
        memcpy(GH(addr + i), &w, 4);
    }
    if (i < size) memcpy(GH(addr + i), src + i, size - i);
}

/* Writes the call's regions and, with -G, its global windows into guest memory, then repins the harness's own words. */
/* The VU0 registers retail held at the call (the decomp's COP2 natives read and leave their w lanes in them). */
static void vu_from_record(const RRec *r) {
    if (!r->has_vf) return;
    for (int i = 1; i < 32; i++) memcpy(p4pc_vu0.vf[i].u, r->vf[i], 16);
    memcpy(p4pc_vu0.acc.u, r->vf[32], 16);
}

static void load_call_memory(const RRec *r) {
    apply_window(r);
    for (unsigned i = 0; i < r->nmem; i++) {
        const RMem *m = &r->mem[i];
        /* An argument may point into the caller's recorded stack region
         * (for example __pack_d's unpacked value at entry $sp). It is input,
         * even though unlabelled outgoing stack words are not compared as output. */
        if (harness_owned(m->addr) || harness_owned(m->addr + m->size - 1)) continue;
        copy_words(m->addr, m->pre, m->size);
    }
    repin();
}

static void apply_window(const RRec *r) {
    if (!g_use_globals || r->win_pre < 0) return;
    if ((size_t)r->win_pre >= g_nwin || !g_win[r->win_pre].bytes) {
        fprintf(stderr, "ovdiff: missing global window %d for %08x call %u\n", r->win_pre, r->addr, r->call);
        exit(2);
    }
    const RWindow *w = &g_win[r->win_pre];
    uint32_t offset = 0;
    for (unsigned i = 0; i < 2; i++) {
        uint32_t lo = w->address[i], hi = lo + w->length[i];
        /* Restore each contiguous part once; never replace the harness's four
         * pinned allocator slots with the recorded retail function pointers. */
        uint32_t prefix_end = hi < SLOT_ALLOC ? hi : SLOT_ALLOC;
        if (lo < prefix_end) copy_words(lo, w->bytes + offset, prefix_end - lo);
        uint32_t suffix_start = lo > SLOT_ALLOC + 16 ? lo : SLOT_ALLOC + 16;
        if (suffix_start < hi) copy_words(suffix_start, w->bytes + offset + suffix_start - lo, hi - suffix_start);
        offset += w->length[i];
    }
}

typedef struct {
    unsigned recorded, valid, exact, fp, special, bad, crashed;
    char first[300];
} RReport;

/* Allocator results: retail's heap and the harness's pool hand out different addresses for the same allocation, so a pair
 * (retail word, native word) with the native in the harness pool and the retail value a data/heap pointer counts as equal
 * when the mapping stays a bijection within the call. */
#define PMAP_MAX 64
static uint32_t g_pm_r[PMAP_MAX], g_pm_n[PMAP_MAX];
static unsigned g_pm_cnt;

static int ptr_equiv(uint32_t retail, uint32_t native) {
    if (native < POOL_BASE || native >= POOL_END || retail < 0x00700000u || retail >= RAM_HI) return 0;
    for (unsigned i = 0; i < g_pm_cnt; i++) {
        if (g_pm_r[i] == retail) return g_pm_n[i] == native;
        if (g_pm_n[i] == native) return 0;
    }
    if (g_pm_cnt == PMAP_MAX) return 0;
    g_pm_r[g_pm_cnt] = retail;
    g_pm_n[g_pm_cnt++] = native;
    return 1;
}

/* A native that called a function the port has no native for ran a logged stub (fake result, no effects): where retail's
 * callee really worked, the comparison says nothing. */
static uint32_t g_stub_addr;
static int native_hit_alloc(void) {
    for (unsigned i = 0; i < g_nlog && i < 256; i++)
        if (g_log[i].addr >= 0xFFFF0001u && g_log[i].addr <= 0xFFFF0004u) return 1;
    return 0;
}
static int native_hit_stub(void) {
    for (unsigned i = 0; i < g_nlog && i < 256; i++)
        if (g_log[i].addr < 0xFFFF0000u) { g_stub_addr = g_log[i].addr; return 1; }
    return 0;
}

/* One recorded call: pre regions in, native out, verdict (0 exact, 1 float tolerance, 2 special, 3 mismatch, 4 crash). */
/* -E interprets the selected retail body, using native callees by default.
 * -E -F interprets its retail callees too, with the recorded entry stack.
 * A match is a reference on these recorded inputs; a native difference still needs a cause. */
static void replay_interp(const Sig *s, const Case *cs, NRet *nr) {
    Info *in = analyze(s->addr);
    R5900 *c = calloc(1, sizeof *c);
    load_interp_regs(c, s, cs);
    vu_from_host(c);   /* the recorded VU0 registers replay_one loaded (vu_from_record): the paired run seeds its EE side the same way */
    c->lo_pc = g_full ? RAM_LO : s->addr;
    c->hi_pc = g_full ? RAM_HI : in->end;
    c->hook = interp_hook;
    int rc = r5900_call(c, s->addr);
    if (rc != R5900_OK) { free(c); g_sig = 0; g_sig_addr = rc; siglongjmp(g_recover, 1); }
    nr->eax = (uint32_t)c->r[2];
    nr->edx = ee_result_hi(c, s->ret);
    nr->st0 = f_of(c->f[0]);
    nr->has_st0 = 1;
    free(c);
}

static int replay_one(const Sig *s, const RRec *r, const p4pc_native_t *nat, char *why, size_t whysz) {
    Case cs;
    memset(&cs, 0, sizeof cs);
    cs.entry_sp = r->sp;
    size_t ni = 0, nf = 0;
    for (size_t i = 0; i < strlen(s->args) && i < 8; i++) cs.slot[i] = s->args[i] == 'f' ? r->fargs[nf++] : r->args[ni++];
    NRet nr;
    memset(&nr, 0, sizeof nr);
    uint8_t *now[64] = { 0 };
    Writes *w = writes_new();
    vu_reset_host();
    vu_from_record(r);
    g_nlog = 0;
    g_stub_seq = 0;
    g_pool = POOL_BASE;
    g_sig = 0;
    int rc;
    struct itimerval it = { { 0, 0 }, { 1, 0 } };
    track_begin();
    if (sigsetjmp(g_recover, 1) == 0) {
        g_recover_armed = 1;
        setitimer(ITIMER_REAL, &it, NULL);
        /* the allocator slots the harness pins stay as fill_arena set them */
        load_call_memory(r);
        if (g_peek) {                                      /* -P ADDR[,ADDR...]: guest words after the call's memory is loaded */
            fprintf(stderr, "peek %08x call %u:", s->addr, r->call);
            for (const char *q = g_peek; *q;) {
                uint32_t pa = (uint32_t)strtoul(q, (char **)&q, 16);
                fprintf(stderr, " [%08x]=%08x", pa, *(uint32_t *)GH(pa));
                if (*q == ',') q++;
            }
            fprintf(stderr, "\n");
        }
        if (g_replay_interp) replay_interp(s, &cs, &nr);
        else ovd_call8(nat->fn, s, nat, cs.slot, &nr);
        rc = 0;
        for (unsigned i = 0; i < r->nmem && i < 64; i++) {
            if (r->mem[i].stack || harness_owned(r->mem[i].addr) || harness_owned(r->mem[i].addr + r->mem[i].size - 1)) continue;
            now[i] = malloc(r->mem[i].size);
            memcpy(now[i], GH(r->mem[i].addr), r->mem[i].size);
        }
    } else {
        rc = -1;
    }
    struct itimerval off = { { 0, 0 }, { 0, 0 } };
    setitimer(ITIMER_REAL, &off, NULL);
    g_recover_armed = 0;
    track_end(w);
    if (rc == -1) OVD_FPU_RESET();
    free(w->orig); free(w->now); free(w);

    int worst = 0;
    g_pm_cnt = 0;
    char *m = why, *end = why + whysz - 1;
    why[0] = 0;
#define RNOTE(...) do { if (m < end - 100) m += snprintf(m, (size_t)(end - m), __VA_ARGS__); } while (0)
    if (rc == -1) {
        RNOTE("native crashed (signal %d at %p)", g_sig, (void *)g_sig_addr);
        worst = 4;
    } else {
        switch (s->ret) {
        case 'v': break;
        case 'f': {
            if (!nr.has_st0) { worst = 3; RNOTE("native returned no float; "); break; }
            uint32_t got;
            memcpy(&got, &nr.st0, 4);
            int k = fcmp(r->f0, got);
            if (k > worst) worst = k;
            if (k) RNOTE("ret f0 retail=%g (%08x) native=%g (%08x); ", f_of(r->f0), r->f0, f_of(got), got);
            break;
        }
        case 'q':
            {
                /* EE returns all 64 bits in $v0. Old recordings contain $v1
                 * instead of its high word: still check their low word and memory,
                 * but never invent the missing oracle from an unrelated register. */
                if (r->v0 != nr.eax || (r->has_v0h && r->v0h != nr.edx)) {
                    worst = 3;
                    if (r->has_v0h) RNOTE("ret retail=%08x:%08x native=%08x:%08x; ", r->v0h, r->v0, nr.edx, nr.eax);
                    else RNOTE("ret low word retail=%08x native=%08x (high word not recorded); ", r->v0, nr.eax);
                }
            }
            break;
        default: {
            uint32_t msk = 0xFFFFFFFFu, x = r->v0, y = nr.eax;
            if (s->ret == 'b' || s->ret == 'B') msk = 0xFF;
            else if (s->ret == 'h' || s->ret == 'H') msk = 0xFFFF;
            if ((x & msk) != (y & msk) && !(msk == 0xFFFFFFFFu && (ptr_equiv(x, y) || (retail_code_ptr(x) && host_ptr_word(y))))) { worst = 3; RNOTE("ret retail=0x%08x native=0x%08x; ", x & msk, y & msk); }
            break;
        }
        }
        int shown = 0;
        for (unsigned i = 0; i < r->nmem && i < 64; i++) {
            if (r->mem[i].stack || !now[i]) continue;
            uint32_t n = r->mem[i].size & ~3u;
            for (uint32_t off2 = 0; off2 < n; off2 += 4) {
                uint32_t wa, wb;
                /* below the entry sp is the retail callee's own frame (and its callees'): scratch the native never has */
                uint32_t wad = r->mem[i].addr + off2;
                if (r->sp && wad < r->sp && wad + 0x4000u >= r->sp) continue;
                if (pinned(wad)) continue;                      /* the allocator slots and camera pointer the harness pins */
                memcpy(&wa, r->mem[i].post + off2, 4);
                memcpy(&wb, now[i] + off2, 4);
                if (wa == wb || words_match_native_ptr(wa, wb) || ptr_equiv(wa, wb) || (retail_code_ptr(wa) && host_ptr_word(wb))) continue;
                int k = wcmp(wa, wb);
                if (k > worst) worst = k;
                if (k >= 2 && shown < 4) {
                    RNOTE("mem[0x%08x] retail=%08x native=%08x%s; ", r->mem[i].addr + off2, wa, wb, k == 1 ? " (float tolerance)" : "");
                    shown++;
                }
            }
        }
    }
#undef RNOTE
    for (unsigned i = 0; i < 64; i++) free(now[i]);
    if (worst >= 3 && worst != 4 && native_hit_alloc() && !native_hit_stub()) {
        snprintf(why + strlen(why), whysz - strlen(why) > 40 ? 40 : whysz - strlen(why), " [native used the allocator]");
        return 6;
    }
    if (worst >= 3 && worst != 4 && native_hit_stub()) {
        snprintf(why + strlen(why), whysz - strlen(why) > 40 ? 40 : whysz - strlen(why), " [native called stub 0x%08x]", g_stub_addr);
        return 5;
    }
    if (worst < 3 && s->ret == 'q' && !r->has_v0h) {
        snprintf(why, whysz, "recording omits bits 32..63 of the EE $v0 result");
        return 7;
    }
    return worst;
}


/* ---------------------------------------------------------------- discovery (-D)
 *
 * A native reads memory the snapshots do not hold: globals, allocator state, deeper pointers. Discovery finds out which:
 * each recorded call runs once in a forked child with the data pages unmapped (PROT_NONE) after its snapshot regions are
 * loaded; the first access to a page records it and maps it (the harness's content). The pages are
 * written to the -D file as one line per recorded call, `<addr8> <call> <name> <page> <page> ...` (page-aligned addresses). Re-recording the same
 * deterministic run with `p4hook --wanted-calls FILE` stores those whole pages at entry and return, and the next replay loads them, so the native sees
 * what retail saw. */
static const char *g_wanted_path;
#define DISC_MAX_PAGES 128

static uint32_t g_dpages[DISC_MAX_PAGES];
static unsigned g_ndpages;
static int g_dpipe = -1;

/* a: a host address (the faulting one); the guest address is its offset in the block (identity on i386) */
#define GBASE ((uintptr_t)GH(1) - 1)
static int disc_protected(uintptr_t a) {
    a -= GBASE;
    if (a < 0x00530000u || a >= RAM_HI) return 0;
    if (harness_owned((uint32_t)a)) return 0;
    return 1;
}

static void on_discover(int sig, siginfo_t *si, void *uc) {
    uintptr_t a = (uintptr_t)si->si_addr;
    if (sig == SIGSEGV && disc_protected(a)) {
        uintptr_t pg = a & ~(uintptr_t)4095;
        mprotect((void *)pg, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (g_ndpages < DISC_MAX_PAGES) g_dpages[g_ndpages++] = (uint32_t)(pg - GBASE);
        return;
    }
    on_signal(sig, si, uc);
}

/* One call under discovery, in a forked child; the pages it touched come back through a pipe. */
static void discover_call(const Sig *s, const RRec *r, const p4pc_native_t *nat, uint32_t *pages, unsigned *npages) {
    int fd[2];
    if (pipe(fd) != 0) return;
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        Case cs;
        memset(&cs, 0, sizeof cs);
        cs.entry_sp = r->sp;
        size_t ni = 0, nf = 0;
        for (size_t i = 0; i < strlen(s->args) && i < 8; i++) cs.slot[i] = s->args[i] == 'f' ? r->fargs[nf++] : r->args[ni++];
        seed_rng((uint64_t)s->addr << 20 | r->call);
        fill_arena(0, 0);
        load_call_memory(r);
        struct sigaction sa;
        memset(&sa, 0, sizeof sa);
        sa.sa_sigaction = on_discover;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
        sigaction(SIGSEGV, &sa, NULL);
        g_tracking = 0;
        vu_reset_host();
        vu_from_record(r);
        g_nlog = 0; g_stub_seq = 0; g_pool = POOL_BASE; g_sig = 0;
        /* unmap the data in the gaps between the harness's own ranges */
        mprotect(GH(0x00530000u), ARENA - 0x00530000u, PROT_NONE);
        mprotect(GH(POOL_END), RAM_HI - POOL_END, PROT_NONE);
        NRet nr;
        memset(&nr, 0, sizeof nr);
        struct itimerval it = { { 0, 0 }, { 2, 0 } };
        if (sigsetjmp(g_recover, 1) == 0) {
            g_recover_armed = 1;
            setitimer(ITIMER_REAL, &it, NULL);
            ovd_call8(nat->fn, s, nat, cs.slot, &nr);
        }
        g_recover_armed = 0;
        uint32_t n = g_ndpages;
        if (write(fd[1], &n, 4) < 0) { }
        if (n && write(fd[1], g_dpages, n * 4) < 0) { }
        _exit(0);
    }
    close(fd[1]);
    uint32_t n = 0;
    if (read(fd[0], &n, 4) == 4 && n <= DISC_MAX_PAGES) {
        uint32_t buf[DISC_MAX_PAGES];
        size_t want = n * 4, got = 0;
        while (got < want) { ssize_t k = read(fd[0], (char *)buf + got, want - got); if (k <= 0) break; got += (size_t)k; }
        for (unsigned i = 0; i < got / 4; i++) {
            unsigned j;
            for (j = 0; j < *npages; j++) if (pages[j] == buf[i]) break;
            if (j == *npages && *npages < DISC_MAX_PAGES) pages[(*npages)++] = buf[i];
        }
    }
    close(fd[0]);
    int st;
    waitpid(pid, &st, 0);
}

static void discover_function(const Sig *s, const p4pc_native_t *nat, FILE *out) {
    for (size_t i = 0; i < g_nrrec; i++) {
        if (g_rrec[i].addr != s->addr) continue;
        uint32_t pages[DISC_MAX_PAGES];
        unsigned npages = 0;
        discover_call(s, &g_rrec[i], nat, pages, &npages);
        fprintf(out, "%08x %u %s", s->addr, g_rrec[i].call, s->name);
        for (unsigned j = 0; j < npages; j++) fprintf(out, " %08x", pages[j]);
        fprintf(out, "\n");
    }
    fflush(out);
}

static void dump_call(const RRec *r) {
    if (!getenv("P4_REPLAY_DEBUG")) return;
    fprintf(stderr, "  args");
    for (int a = 0; a < 8; a++) fprintf(stderr, " %08x", r->args[a]);
    fprintf(stderr, "  sp %08x\n  regions", r->sp);
    for (unsigned m = 0; m < r->nmem; m++)
        fprintf(stderr, " %c%08x+%x%s", r->mem[m].stack ? 's' : 'p', r->mem[m].addr, r->mem[m].size,
                harness_owned(r->mem[m].addr) ? "(owned)" : "");
    fprintf(stderr, "\n");
}

/* Child process body for one function: every recorded call, one row in the report (the format test_function writes). */
/* A different oracle from strict golden replay: compare the selected EE body
 * with its native on identical recorded inputs and identical native callees.
 * Callback differences and every non-local guest byte are exact, not tolerant.
 * A fault, truncated observation or incomplete pair can never pass this mode. */
static void replay_paired_function(const Sig *s, const p4pc_native_t *nat, Report *rep) {
    Info *in = analyze(s->addr);
    if (!in->ok || in->hw) {
        snprintf(rep->status, sizeof rep->status, "paired unavailable: unsupported function boundary or hardware");
        return;
    }
    /* Context creation must precede both entry-snapshot loads. */
    if (!paired_device_init()) {
        snprintf(rep->status, sizeof rep->status, "paired unavailable: production RW raster device initialization failed");
        return;
    }
    Result sides[2] = {0};
    for (int side = 0; side < 2; ++side) sides[side].w = writes_new();
    for (size_t i = 0; i < g_nrrec; ++i) {
        const RRec *r = &g_rrec[i];
        if (r->addr != s->addr) continue;
        Case cs = {0};
        cs.entry_sp = r->sp;
        for (size_t slot = 0, ni = 0, nf = 0; slot < strlen(s->args) && slot < 8; ++slot)
            cs.slot[slot] = s->args[slot] == 'f' ? r->fargs[nf++] : r->args[ni++];
        rep->attempts++;
        int complete = 1;
        char why[512] = {0};
        g_callback_entry_sp = r->sp;
        for (int side = 0; side < 2; ++side) {
            /* track_end restores every written page, including pages absent
             * from the recording. Re-seed arena/allocator and input snapshots
             * too, so the second caller cannot inherit the first's effects. */
            seed_rng((uint64_t)s->addr << 20 | r->call);
            fill_arena(0, 0);
            load_call_memory(r);
            if (!paired_device_begin()) {
                snprintf(why, sizeof why, "production RW raster device initialization failed");
                complete = 0;
                break;
            }
            if (!side) run_interp(s, &cs, &sides[side], in, r->has_vf ? r->vf : NULL);
            else run_native(s, &cs, &sides[side], nat, r->has_vf ? r->vf : NULL);
            Result *result = &sides[side];
            if (result->rc != 0) {
                snprintf(why, sizeof why, "%s failed: rc %d, signal %d at %p, pc %08x",
                    side ? "native" : "EE caller", result->rc, g_sig, (void *)g_sig_addr, result->bad_pc);
                complete = 0;
            } else if (result->log_overflow || result->w->total > 512) {
                snprintf(why, sizeof why, "%s observation overflow: callbacks %s, touched pages %u",
                    side ? "native" : "EE caller", result->log_overflow ? "truncated" : "complete", result->w->total);
                complete = 0;
            }
            if (!complete) break;
        }
        g_callback_entry_sp = 0;
        if (!complete) {
            rep->nfault++;
        } else {
            Verdict verdict;
            judge(s, &sides[0], &sides[1], &verdict, 1, r->sp);
            rep->valid++;
            if (!verdict.worst) rep->exact++;
            else {
                rep->bad++;
                snprintf(why, sizeof why, "%s", verdict.msg);
            }
        }
        if (why[0]) {
            if (!rep->repro[0]) snprintf(rep->repro, sizeof rep->repro,
                "call %u (frame %u): %s", r->call, r->frame, why);
            if (g_verbose) {
                fprintf(stderr, "paired %08x %s call %u: %s\n", s->addr, s->name, r->call, why);
                dump_call(r);
            }
        }
    }
    for (int side = 0; side < 2; ++side) {
        free(sides[side].w->orig);
        free(sides[side].w->now);
        free(sides[side].w);
    }
    snprintf(rep->status, sizeof rep->status, "%s",
        rep->bad ? "paired MISMATCH" : rep->nfault ? "paired incomplete" : "paired exact");
}

static void replay_function(const Sig *s, Report *rep) {
    memset(rep, 0, sizeof *rep);
    load_records(s->addr);
    const p4pc_native_t *nat = find_native(s->addr);
    if (!nat) { snprintf(rep->status, sizeof rep->status, "skipped: no native at this address"); return; }
    if (strpbrk(s->args, "qd?") || s->ret == 'd' || strlen(s->args) > 8) { snprintf(rep->status, sizeof rep->status, "skipped: signature not supported"); return; }
    const char *why_impure = impure_reason(s->addr, nat->name);
    if (why_impure) { snprintf(rep->status, sizeof rep->status, "skipped: impure (%s)", why_impure); return; }
    unsigned recorded = 0;
    for (size_t i = 0; i < g_nrrec; i++) if (g_rrec[i].addr == s->addr) recorded++;
    if (!recorded) { snprintf(rep->status, sizeof rep->status, "no recorded calls"); return; }
    if (g_replay_paired) {
        replay_paired_function(s, nat, rep);
        return;
    }
    if (g_wanted_path) {
        FILE *wf = fopen(g_wanted_path, "a");
        if (wf) { discover_function(s, nat, wf); fclose(wf); }
        snprintf(rep->status, sizeof rep->status, "discovery");
        return;
    }
    char why[512];
    for (size_t i = 0; i < g_nrrec; i++) {
        const RRec *r = &g_rrec[i];
        if (r->addr != s->addr) continue;
        rep->attempts++;
        /* fresh guest state per call, as ovdiff does per case */
        seed_rng((uint64_t)s->addr << 20 | r->call);
        fill_arena(0, 0);
        int k = replay_one(s, r, nat, why, sizeof why);
        if (k == 7) {
            rep->inconclusive++;
            if (!rep->invalid_why[0]) snprintf(rep->invalid_why, sizeof rep->invalid_why, "recording omits bits 32..63 of the EE $v0 result");
            if (g_verbose) fprintf(stderr, "replay-missing-v0h %08x %s call %u: %s\n", s->addr, s->name, r->call, why);
            continue;
        }
        if (k == 6) {
            rep->inconclusive++;
            if (!rep->invalid_why[0]) snprintf(rep->invalid_why, sizeof rep->invalid_why, "allocator results differ (harness pool vs retail heap)");
            if (g_verbose) fprintf(stderr, "replay-alloc %08x %s call %u: %s\n", s->addr, s->name, r->call, why);
            continue;
        }
        if (k == 5) {
            rep->inconclusive++;
            if (!rep->invalid_why[0]) snprintf(rep->invalid_why, sizeof rep->invalid_why, "calls a function with no native (stub)");
            if (g_verbose) fprintf(stderr, "replay-stub %08x %s call %u: %s\n", s->addr, s->name, r->call, why);
            continue;
        }
        if (k == 4) { rep->nfault++; if (g_verbose) { fprintf(stderr, "replay-crash %08x %s call %u (frame %u): %s\n", s->addr, s->name, r->call, r->frame, why); dump_call(r); } if (!rep->repro[0]) snprintf(rep->repro, sizeof rep->repro, "call %u (frame %u): %s", r->call, r->frame, why); continue; }
        rep->valid++;
        switch (k) {
        case 0: rep->exact++; break;
        case 1: rep->fp++; break;
        case 2: rep->special++; break;
        default:
            if (!g_replay_interp) {
                /* The same call through the retail code in the interpreter: when it gives the native's answer, the recording
                 * lacks state (or retail depended on something unrecorded), so the call says nothing about the native. */
                char why2[512];
                seed_rng((uint64_t)s->addr << 20 | r->call);
                fill_arena(0, 0);
                g_replay_interp = 1;
                int k2 = replay_one(s, r, nat, why2, sizeof why2);
                g_replay_interp = 0;
                if (k2 == 3 && !strcmp(why, why2)) {
                    rep->valid--;
                    rep->inconclusive++;
                    if (!rep->invalid_why[0]) snprintf(rep->invalid_why, sizeof rep->invalid_why, "the interpreted retail code gives the native's answer, not the recording");
                    if (g_verbose) fprintf(stderr, "replay-interp-agrees %08x %s call %u: %s\n", s->addr, s->name, r->call, why);
                    continue;
                }
            }
            rep->bad++;
            if (g_verbose) {
                fprintf(stderr, "replay %08x %s call %u (frame %u, run %s): %s\n", s->addr, s->name, r->call, r->frame, "-", why);
                dump_call(r);
            }
            if (!rep->repro[0]) snprintf(rep->repro, sizeof rep->repro, "call %u (frame %u): %s", r->call, r->frame, why);
            break;
        }
    }
    if (rep->valid == 0 && rep->inconclusive) snprintf(rep->status, sizeof rep->status, "inconclusive (%u calls: %s)", rep->inconclusive, rep->invalid_why[0] ? rep->invalid_why : "stub callee or allocator results");
    else if (rep->valid == 0) snprintf(rep->status, sizeof rep->status, "no valid calls (%u crashed)", rep->nfault);
    else if (rep->bad && (rexpected_reason(s->addr) || expected_reason(s->addr)))
        snprintf(rep->status, sizeof rep->status, "documented difference: %s", rexpected_reason(s->addr) ? rexpected_reason(s->addr) : expected_reason(s->addr));
    else if (rep->bad) snprintf(rep->status, sizeof rep->status, "MISMATCH");
    else if (rep->special) snprintf(rep->status, sizeof rep->status, "ok (special-float host differences)");
    else if (rep->fp) snprintf(rep->status, sizeof rep->status, "ok (float tolerance)");
    else snprintf(rep->status, sizeof rep->status, "ok");
}
