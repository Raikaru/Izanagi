/* cri_adxi_rpc_test: the host side of the CRI_ADXI.IRX RPC server (src/cri/adxi_rpc.c) behind
 * func_004258b0 = sceSifCallRpc, one case per command the EE sends (ids 0..3, 8..18, 32..42), driven the
 * way func_004e58a0 drives it: words in, words out, fno = 0x400 + id.
 *
 *   - the remote joint commands against the retail joint library called directly on the same
 *     object (the reply must be what the library returns), with chunk data in IOP RAM;
 *   - the IOPRNA state machine: core flags after init, core mode, start/stop, sync start/stop,
 *     pause, finish, against the values the IOP handlers compute (IRX 0x1258..0x2494);
 *   - a PSM's PCM reaching the mixer voice through the joints, on the right core and channel;
 *   - the server buffer persisting between calls, and unknown commands and clients.
 * Boots the image like axrna_test (CRI's joints are compiled decomp). Exits 77 (CTest skip) when
 * P4PC_ISO or P4PC_ELF is unset. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/image.h"
#include "p4pc/os.h"
#include "adxi_rpc.h"
#include "cri_mem.h"
#include "guest_test.h"
#include "sdr_mem.h"

#define SKIP_RETURN 77

void SJRBF_Init(void);
void SJUNI_Init(void);
int32_t func_004258b0(uint32_t client, uint32_t fno, uint32_t mode, uint32_t send, uint32_t ssize, uint32_t recv,
                      uint32_t rsize, uint32_t endfunc, uint32_t efarg);
void *kwlnProbeHeapBudget(void);

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "cri_adxi_rpc_test: FAIL %s\n", what);
    failures++;
}

static void check(const char *what, long long got, long long want)
{
    if (got != want) {
        fprintf(stderr, "cri_adxi_rpc_test: FAIL %s: got 0x%llx want 0x%llx\n", what, got, want);
        failures++;
    }
}

/* The RPC reads its send words from, and writes its reply to, guest memory by address: one scratch block of guest
 * storage, the caller's words copied in and the reply words copied out. */
static uint32_t *gsend, *grecv;

/* One command: ns words sent, nr reply words read back (the rest of recv is left alone). */
static int32_t rpc(uint32_t id, const uint32_t *send, int ns, uint32_t *recv, int nr)
{
    int32_t rc;

    if (ns > 0) {
        memcpy(gsend, send, (size_t)ns * 4u);
    }
    if (nr > 0) {
        memcpy(grecv, recv, (size_t)nr * 4u);
    }
    rc = func_004258b0(P4ADXI_CLIENT, 0x400u + id, 0, ns > 0 ? P4_H2G(gsend) : 0, (uint32_t)ns * 4u,
                       nr > 0 ? P4_H2G(grecv) : 0, (uint32_t)nr * 4u, 0, 0);
    if (nr > 0) {
        memcpy(recv, grecv, (size_t)nr * 4u);
    }
    return rc;
}

static uint32_t rpc1(uint32_t id, uint32_t a)
{
    uint32_t s[1] = {a};
    uint32_t r[1] = {0};

    rpc(id, s, 1, r, 1);
    return r[0];
}

static int16_t pattern(int ch, int i)
{
    return (int16_t)(ch == 0 ? (i * 7) % 20000 - 10000 : 9000 - (i * 5) % 18000);
}

/* Appends n samples of channel ch (pattern index from..) to a remote joint through GetChunk and
 * PutChunk of the RPC (the EE writes into the chunk over DMA between the two). */
static int put_pcm(uint32_t h, int ch, int from, int n)
{
    int done = 0;

    while (done < n) {
        uint32_t s[4] = {h, 0, (uint32_t)(n - done) * 2u, 0};
        uint32_t r[2] = {0, 0};
        int got, k;
        int16_t *p;

        rpc(P4ADXI_SJ_GETCHUNK, s, 3, r, 2);
        got = (int)r[1] / 2;
        if (got <= 0) {
            return done;
        }
        p = (int16_t *)p4sdr_iop_ptr(r[0], r[1]);
        if (p == NULL) {
            fail("GetChunk address is not in IOP RAM");
            return done;
        }
        for (k = 0; k < got; k++) {
            p[k] = pattern(ch, from + done + k);
        }
        s[1] = 1;
        s[2] = r[0];
        s[3] = r[1];
        rpc(P4ADXI_SJ_PUTCHUNK, s, 4, NULL, 0);
        done += got;
    }
    return done;
}

static uint32_t sj_create_rbf(uint32_t addr, uint32_t size, uint32_t extra)
{
    uint32_t s[3] = {addr, size, extra};
    uint32_t r[1] = {0};

    rpc(P4ADXI_SJ_CREATE_RBF, s, 3, r, 1);
    return r[0];
}

static uint32_t num_data(uint32_t h, uint32_t line)
{
    uint32_t s[2] = {h, line};
    uint32_t r[1] = {0xdeadbeef};

    rpc(P4ADXI_SJ_GETNUMDATA, s, 2, r, 1);
    return r[0];
}

static void render(float *l, float *r, int frames)
{
    static float blk[240 * 2];
    int done = 0;

    while (done < frames) {
        int c = frames - done > 240 ? 240 : frames - done;
        int i;

        p4snd_render(blk, c);
        for (i = 0; i < c; i++) {
            l[done + i] = blk[i * 2];
            r[done + i] = blk[i * 2 + 1];
        }
        done += c;
    }
}

static int silent(const float *l, const float *r, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        if (l[i] != 0.0f || r[i] != 0.0f) {
            return 0;
        }
    }
    return 1;
}

/* ------------------------------------------------------------------ the groups */

static void test_buffer_and_unknown(void)
{
    uint32_t s[3] = {0x11111111, 0x22222222, 0x33333333};
    uint32_t r[3] = {0, 0, 0};
    uint32_t none[1] = {0};

    /* DTX open (id 3) changes nothing in the buffer, and the buffer outlives the call */
    check("id 3 returns 0", rpc(P4ADXI_DTX_OPEN, s, 3, NULL, 0), 0);
    check("id 3 with a reply of 3 words", rpc(P4ADXI_DTX_OPEN, NULL, 0, r, 3), 0);
    check("buffer word 0 persists", r[0], 0x11111111);
    check("buffer word 1 persists", r[1], 0x22222222);
    check("buffer word 2 persists", r[2], 0x33333333);

    check("unknown id answers 0", rpc(5, none, 1, NULL, 0), 0);
    check("unknown id twice", rpc(5, none, 1, NULL, 0), 0);
    check("fno 0 is empty", func_004258b0(P4ADXI_CLIENT, 0, 0, 0, 0, 0, 0, 0, 0), 0);
    check("a client that is not the server", func_004258b0(0x00732D40u, 0x401, 0, 0, 0, 0, 0, 0, 0), -1);
    check("oversized send refused", func_004258b0(P4ADXI_CLIENT, 0x403, 0, P4_H2G(gsend), 0x1000, 0, 0, 0, 0), -1);
}

static void test_sjx(void)
{
    uint32_t h[32];
    uint32_t s[4], r[1];
    int i;

    /* id 0: free slots in order, handle = 0x15c40 + 20 i; id 1 frees; id 2 sets the id half */
    for (i = 0; i < 32; i++) {
        s[0] = 0x100 + (uint32_t)i;
        s[1] = 2;
        s[2] = 1;
        s[3] = 0x7000 + (uint32_t)i;
        r[0] = 0xffffffff;
        rpc(P4ADXI_SJX_CREATE, s, 4, r, 1);
        h[i] = r[0];
        if (r[0] != 0x15c40u + 20u * (uint32_t)i) {
            check("SJX create handle", r[0], 0x15c40u + 20u * (uint32_t)i);
            break;
        }
    }
    r[0] = 0x1234;
    rpc(P4ADXI_SJX_CREATE, s, 4, r, 1);
    check("33rd SJX create: no slot", r[0], 0);

    s[0] = h[3];
    rpc(P4ADXI_SJX_DESTROY, s, 1, NULL, 0);
    s[0] = h[7];
    rpc(P4ADXI_SJX_DESTROY, s, 1, NULL, 0);
    r[0] = 0;
    rpc(P4ADXI_SJX_CREATE, s, 4, r, 1);
    check("SJX create reuses the lowest free slot", r[0], h[3]);
    r[0] = 0;
    rpc(P4ADXI_SJX_CREATE, s, 4, r, 1);
    check("SJX create reuses the next free slot", r[0], h[7]);

    s[0] = h[0];
    s[1] = 0xabcd1234;
    rpc(P4ADXI_SJX_SETID, s, 2, NULL, 0);
    s[0] = 0xbad;
    rpc(P4ADXI_SJX_DESTROY, s, 1, NULL, 0); /* a bad handle is logged and ignored */
    for (i = 0; i < 32; i++) {
        s[0] = h[i];
        rpc(P4ADXI_SJX_DESTROY, s, 1, NULL, 0);
    }
}

static void test_sj(void)
{
    const uint32_t A = 0x100000, B = 0x110000, C = 0x120000;
    uint32_t s[4], r[4];
    uint32_t rbf, mem, uni;
    void *sj;
    int16_t *pcm;
    int i;

    /* 32: SJRBF over IOP RAM; the free line holds the ring, the data line is empty */
    rbf = sj_create_rbf(A, 0x1000, 0x200);
    check("SJRBF_Create handle is set", rbf != 0, 1);
    check("SJRBF_Create out of IOP RAM", sj_create_rbf(0x7fffff00u, 0x1000, 0x200), 0);
    sj = P4_G2H(rbf);
    check("free line holds the ring", num_data(rbf, 0), (uint32_t)sj_num(sj, 0));
    check("free line size", num_data(rbf, 0), 0x1000);
    check("data line empty", num_data(rbf, 1), 0);

    /* 38 GetChunk: data is an IOP address */
    s[0] = rbf;
    s[1] = 0;
    s[2] = 0x400;
    r[0] = r[1] = 0;
    rpc(P4ADXI_SJ_GETCHUNK, s, 3, r, 2);
    check("GetChunk address", r[0], A);
    check("GetChunk length", r[1], 0x400);
    pcm = (int16_t *)p4sdr_iop_ptr(A, 0x400);
    for (i = 0; i < 0x200; i++) {
        pcm[i] = (int16_t)(i * 3);
    }

    /* 40 PutChunk moves it to the data line */
    s[0] = rbf;
    s[1] = 1;
    s[2] = r[0];
    s[3] = r[1];
    rpc(P4ADXI_SJ_PUTCHUNK, s, 4, NULL, 0);
    check("data line after PutChunk", num_data(rbf, 1), 0x400);
    check("free line after PutChunk", num_data(rbf, 0), 0x1000 - 0x400);

    /* 42 IsGetChunk against the library called directly */
    {
        int32_t want_rbyte = -1;
        int32_t want = ((int32_t(*)(void *, int32_t, int32_t, int32_t *))P4_FN_G2H(sj_vt(sj)[10]))(sj, 1, 0x100, &want_rbyte);

        s[0] = rbf;
        s[1] = 1;
        s[2] = 0x100;
        r[0] = r[1] = 0xdead;
        rpc(P4ADXI_SJ_ISGETCHUNK, s, 3, r, 2);
        check("IsGetChunk result", r[0], (uint32_t)want);
        check("IsGetChunk rbyte", r[1], (uint32_t)want_rbyte);
    }

    /* 39 UngetChunk: take 0x100 from the data line and give it back */
    s[0] = rbf;
    s[1] = 1;
    s[2] = 0x100;
    rpc(P4ADXI_SJ_GETCHUNK, s, 3, r, 2);
    check("data line chunk", r[0], A);
    check("data line chunk length", r[1], 0x100);
    check("data line after take", num_data(rbf, 1), 0x300);
    s[0] = rbf;
    s[1] = 1;
    s[2] = r[0];
    s[3] = r[1];
    rpc(P4ADXI_SJ_UNGETCHUNK, s, 4, NULL, 0);
    check("data line after UngetChunk", num_data(rbf, 1), 0x400);
    check("chunk data intact", pcm[5], 15);

    /* 36 GetUuid against the library */
    {
        const uint8_t *u = ((const uint8_t *(*)(void *))P4_FN_G2H(sj_vt(sj)[4]))(sj);
        uint32_t got[4] = {0, 0, 0, 0};

        s[0] = rbf;
        rpc(P4ADXI_SJ_GETUUID, s, 1, got, 4);
        check("GetUuid is the library's uuid", u != NULL && memcmp(got, u, 16) == 0, 1);
    }

    /* 37 Reset empties the data line, refills the free line */
    rpc(P4ADXI_SJ_RESET, s, 1, NULL, 0);
    check("data line after Reset", num_data(rbf, 1), 0);
    check("free line after Reset", num_data(rbf, 0), 0x1000);

    /* 33 SJMEM over IOP RAM: the library's own counts */
    s[0] = B;
    s[1] = 0x400;
    r[0] = 0;
    rpc(P4ADXI_SJ_CREATE_MEM, s, 2, r, 1);
    mem = r[0];
    check("SJMEM_Create handle is set", mem != 0, 1);
    check("SJMEM line 1", num_data(mem, 1), (uint32_t)sj_num(P4_G2H(mem), 1));
    check("SJMEM line 0", num_data(mem, 0), (uint32_t)sj_num(P4_G2H(mem), 0));
    s[0] = B + 0x7fffff00u;
    s[1] = 0x400;
    rpc(P4ADXI_SJ_CREATE_MEM, s, 2, r, 1);
    check("SJMEM_Create out of IOP RAM", r[0], 0);

    /* 34 SJUNI: mode, work in IOP RAM, work size */
    s[0] = 0;
    s[1] = C;
    s[2] = 0x400;
    r[0] = 0;
    rpc(P4ADXI_SJ_CREATE_UNI, s, 3, r, 1);
    uni = r[0];
    check("SJUNI_Create handle is set", uni != 0, 1);
    check("SJUNI line 0", num_data(uni, 0), (uint32_t)sj_num(P4_G2H(uni), 0));
    check("SJUNI line 1", num_data(uni, 1), (uint32_t)sj_num(P4_G2H(uni), 1));

    /* 35 Destroy: the handle is gone, later calls on it answer zero */
    for (i = 0; i < 3; i++) {
        uint32_t d = i == 0 ? rbf : i == 1 ? mem : uni;

        s[0] = d;
        rpc(P4ADXI_SJ_DESTROY, s, 1, NULL, 0);
        check("destroyed joint answers 0", num_data(d, 1), 0);
    }
}

static void test_rna(void)
{
    uint32_t s[8], r[4];
    uint32_t sjA, sjB, rna;
    P4SndVoice *v;
    static float l[2000], rr[2000];
    int i;
    double worst;

    /* 10 init: no reply, nothing running yet */
    s[0] = 2;
    s[1] = 0x130000;
    s[2] = 0x2000;
    check("IOPRNA init returns 0", rpc(P4ADXI_RNA_INIT, s, 3, NULL, 0), 0);
    check("cores idle after id 10", p4adxi_core_running(0) + p4adxi_core_running(1), 0);

    /* 12 base sfreq: the IOP reads words 1 and 2 as well, left over in the buffer (zero here).
     * First Init: core mode 0 starts core 0 only; the reply is [core mode, core 0, core 1]. */
    s[0] = 48000;
    r[0] = r[1] = r[2] = 0xdead;
    rpc(P4ADXI_RNA_SETSFREQ, s, 1, r, 3);
    check("id 12 reply: core mode", r[0], 0);
    check("id 12 reply: core 0 running", r[1], 1);
    check("id 12 reply: core 1 running", r[2], 0);

    /* 11 core mode 2: Finish (count 1 -> 0 tears down), Init again: both cores run */
    s[0] = 2;
    rpc(P4ADXI_RNA_SETCOREMODE, s, 1, NULL, 0);
    check("id 11: core 0 running", p4adxi_core_running(0), 1);
    check("id 11: core 1 running", p4adxi_core_running(1), 1);
    s[0] = 7; /* out of range: the mode stays 2, but Finish and Init still run (count 1 -> 0 -> 1) */
    rpc(P4ADXI_RNA_SETCOREMODE, s, 1, NULL, 0);
    s[0] = 48000;
    rpc(P4ADXI_RNA_SETSFREQ, s, 1, r, 3); /* count 1 -> 2, no second Init */
    check("id 12 reports core mode 2", r[0], 2);
    r[0] = r[1] = 0xdead;
    rpc(P4ADXI_RNA_FINISH, NULL, 0, r, 2); /* count 2 -> 1: nothing is torn down */
    check("id 13 with a second user: core 0 still running", r[0], 1);
    check("id 13 with a second user: core 1 still running", r[1], 1);

    /* 8 create: two channels on core 0 over two remote joints */
    sjA = sj_create_rbf(0x140000, 0x3000, 0x200);
    sjB = sj_create_rbf(0x150000, 0x3000, 0x200);
    s[0] = 2;
    s[1] = 0;
    s[2] = sjA;
    s[3] = sjB;
    r[0] = 0;
    rpc(P4ADXI_RNA_CREATE, s, 4, r, 1);
    rna = r[0];
    check("id 8 handle is the first slot", rna, 0xf060);
    check("PSM outputs to core 0", p4adxi_rna_core(rna), 0);
    v = p4adxi_rna_voice(rna);
    check("PSM has a mixer voice", v != NULL, 1);

    s[0] = 3; /* three channels: the object has room for two */
    r[0] = 0xdead;
    rpc(P4ADXI_RNA_CREATE, s, 4, r, 1);
    check("id 8 with 3 channels: no handle", r[0], 0);
    s[0] = 2;
    s[1] = 2; /* core 2 does not exist */
    r[0] = 0xdead;
    rpc(P4ADXI_RNA_CREATE, s, 4, r, 1);
    check("id 8 on core 2: no handle", r[0], 0);
    s[1] = 0;
    s[2] = 0x1234;
    r[0] = 0xdead;
    rpc(P4ADXI_RNA_CREATE, s, 4, r, 1);
    check("id 8 on an unknown joint: no handle", r[0], 0);

    /* PCM through the joints reaches the voice, channel 0 left and channel 1 right */
    check("ch0 samples", put_pcm(sjA, 0, 0, 1000), 1000);
    check("ch1 samples", put_pcm(sjB, 1, 0, 1000), 1000);
    p4adxi_pump();
    check("voice holds the PCM", p4snd_stream_data(v), 1000);
    check("joint data line drained", num_data(sjA, 1), 0);
    render(l, rr, 480);
    worst = 0.0;
    for (i = 240; i < 480; i++) { /* the first frames after a start are the mixer's fade-in */
        worst = fmax(worst, fmax(fabs(l[i] - (double)pattern(0, i) / 32768.0), fabs(rr[i] - (double)pattern(1, i) / 32768.0)));
    }
    if (worst > 2.0 / 32768.0) {
        fprintf(stderr, "cri_adxi_rpc_test: output deviates by %.1f LSB\n", worst * 32768.0);
        fail("PSM output is not the joint input");
    }

    /* 17 stop core 0: the voice pauses, data holds; the reply is the core's flag */
    check("id 17 reply", rpc1(P4ADXI_RNA_STOPCORE, 0), 0);
    check("core 0 stopped, core 1 running", p4adxi_core_running(0) * 2 + p4adxi_core_running(1), 1);
    {
        uint32_t held = p4snd_stream_data(v);

        render(l, rr, 480);
        check("stopped core: output silent", silent(l, rr, 480), 1);
        check("stopped core: data holds", p4snd_stream_data(v), held);
    }
    /* 16 start core 0 again */
    check("id 16 reply", rpc1(P4ADXI_RNA_STARTCORE, 0), 1);
    render(l, rr, 240);
    check("running again: output follows", fabs(l[0] - (double)pattern(0, 480) / 32768.0) < 2.0 / 32768.0, 1);
    check("id 16 on a bad core", rpc1(P4ADXI_RNA_STARTCORE, 5), 0);

    /* 14 sync start: PSMs on core 0 move to core 1, both cores run; 15 sync stop moves them back */
    r[0] = r[1] = 0;
    rpc(P4ADXI_RNA_SYNCSTART, NULL, 0, r, 2);
    check("id 14 reply core 0", r[0], 1);
    check("id 14 reply core 1", r[1], 1);
    check("PSM moved to core 1", p4adxi_rna_core(rna), 1);
    r[0] = r[1] = 0;
    rpc(P4ADXI_RNA_SYNCSTOP, NULL, 0, r, 2);
    check("id 15 reply core 0", r[0], 1);
    check("id 15 reply core 1", r[1], 1);
    check("PSM back on core 0", p4adxi_rna_core(rna), 0);

    /* 18 pause (1 stops both cores, 0 starts them) */
    s[0] = 1;
    rpc(P4ADXI_RNA_PAUSE, s, 1, r, 2);
    check("id 18 pause: core 0", r[0], 0);
    check("id 18 pause: core 1", r[1], 0);
    s[0] = 0;
    rpc(P4ADXI_RNA_PAUSE, s, 1, r, 2);
    check("id 18 resume: core 0", r[0], 1);
    check("id 18 resume: core 1", r[1], 1);

    /* 13 finish: PSMs are torn down, the cores stop */
    r[0] = r[1] = 0xdead;
    rpc(P4ADXI_RNA_FINISH, NULL, 0, r, 2);
    check("id 13 reply core 0", r[0], 0);
    check("id 13 reply core 1", r[1], 0);
    check("PSM torn down", p4adxi_rna_voice(rna) == NULL, 1);

    /* 9 destroy: the slot is free again */
    rpc1(P4ADXI_RNA_DESTROY, rna);
    s[0] = 48000;
    rpc(P4ADXI_RNA_SETSFREQ, s, 1, r, 3); /* Init again, count 0 -> 1 */
    s[0] = 1;
    s[1] = 1;
    s[2] = sjA;
    r[0] = 0;
    rpc(P4ADXI_RNA_CREATE, s, 3, r, 1);
    check("id 8 reuses the slot after id 9", r[0], 0xf060);
    check("mono PSM on core 1", p4adxi_rna_core(r[0]), 1);
    check("mono PSM has a voice", p4adxi_rna_voice(r[0]) != NULL, 1);
    rpc1(P4ADXI_RNA_DESTROY, r[0]);
    check("destroyed PSM has no voice", p4adxi_rna_voice(r[0]) == NULL, 1);
}

static int test_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "cri_adxi_rpc_test: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    p4snd_use_offline();

    gsend = p4t_galloc(0x1000, 16);
    grecv = p4t_galloc(0x1000, 16);
    SJRBF_Init();
    SJUNI_Init();
    p4adxi_reset();

    test_buffer_and_unknown();
    test_sjx();
    test_sj();
    test_rna();

    p4adxi_reset();
    if (failures != 0) {
        fprintf(stderr, "cri_adxi_rpc_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("cri_adxi_rpc_test: ok\n");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
