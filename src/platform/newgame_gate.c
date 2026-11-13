/* New-game gate: P4PC_NEWGAME_GATE=<flip>[,<field>:<room>[,<month>/<day>]] makes the process exit at that presented frame (the flip count of
 * RwCameraShowRaster, the one P4PC_CAPTURE_FRAMES and P4PC_AUTOPRESS_FRAMES use) with 0 when the game is in that field and
 * room (the field scene's id words, iGpffff9db0 +0 / +4: the ones fieldSceneLoad compares) and, when P4PC_REF_RGB names a raw
 * 640x448 RGB reference and P4PC_REF_MAX a limit in percent, the frame captured one flip earlier (P4PC_CAPTURE_FRAMES=<flip - 1>)
 * is within that mean absolute difference of it; with 1 otherwise. A third field month/day also requires the calendar to be on that date
 * (datGetDateIndex() == calDayOfYearFromMonthDay(month, day)): story progress, not just a room. tests/newgame_gate.sh drives it through the New Game script.
 *
 * P4PC_FACING_GATE=1 adds the walkers' facing check (state words, no frames): every few flips the scene resources of list 3 (the event and field characters, the
 * game's scene manager at the word 0x00762EA0: manager + 8, list heads at +4 * list, next node at +0x138, the model's matrix at +0x164) are sampled; a model that
 * moved in x/z must have its matrix's `at` column along the way it moved (retail: at is the direction of travel, right = (at.z, 0, -at.x)). Any sample against
 * the travel (dot < -0.5 on two samples in a row) fails the gate, as does a model of the Inaba greeting or the 04/12 breakfast (retail poses below) with another facing, as does a run in which fewer than 40 such samples were seen (the events never moved anyone). The event walkers' yaw
 * comes from MT_Scene_GetDirectionYaw's acos of a soft double: with a 32-bit declaration of it every walker faced +Z. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "p4pc/crt.h"
#include "p4pc/gmem.h"
#include "p4pc/capture_state.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif

/* The loaded field scene's record (retail data word iGpffff9db0, a guest address): field id +0, room +4, flags +0xC. */
#define iGpffff9db0 ((uint8_t *)P4_G2H(P4_GPDATA(uint32_t, 0x9db0)))
extern double p4gl_context_capture_mad(void);
extern const char *p4gl_gate_limit(const char *envName);
extern int16_t datGetDateIndex(void);
extern int32_t calDayOfYearFromMonthDay(int32_t month, int32_t day);
extern int p4pc_title_ready_state, p4pc_title_confirmed; /* title_driver.c */
extern unsigned p4pc_title_ready_flip, p4pc_title_confirmed_flip;

/* P4PC_GATE_RESULT=<path>: the gate's outcome as a small file the engine writes at the
 * gate's exit, for a caller (p4mod test) that must not trust the mixed engine and mod
 * log. One "key value" per line after the "p4pc-gate-result 1" header. 0 written. */
static int write_gate_result(const char *path, int ok, unsigned frame, int field, int room)
{
    FILE *out = fopen(path, "wb");
    if (out == NULL) return -1;
    int bad = fprintf(out, "p4pc-gate-result 1\nok %d\nflip %u\nfield %d\nroom %d\ntitle_ready_state %d\n"
                           "title_ready_flip %u\ntitle_confirmed %d\ntitle_confirmed_flip %u\n",
                      ok, frame, field, room, p4pc_title_ready_state, p4pc_title_ready_flip, p4pc_title_confirmed,
                      p4pc_title_confirmed_flip) < 0;
    bad |= fclose(out) != 0;
    return bad ? -1 : 0;
}


/* The facing check of P4PC_FACING_GATE (above). */
typedef struct { uint32_t node; float x, z; int bad; } FacingTrack;
static FacingTrack sTrack[24];
static unsigned sFacingSamples, sFacingBad;

/* Poses of the Inaba greeting read from retail (PortTrace3's ng-handsome state, scene list 3, the matrices at node + 0x164): where a model stands and the `at`
 * column it has there. Yu faces -Z, Dojima and Nanako +Z (toward each other). Each must be seen with that `at` at least once in the run. */
static const struct { const char *who; float x, z, atx, atz; int fromDate; } sPoses[] = {
    { "Yu", -293.0f, 674.9f, 0.194f, -0.981f, 10 },
    { "Dojima", -282.8f, 572.1f, -0.105f, 0.995f, 10 },
    { "Nanako", -328.0f, 535.8f, -0.139f, 0.990f, 10 },
    /* the 04/12 breakfast (PortTrace3's retail frame f10921, list 3): Yu standing at the table, three of the dishes */
    { "Yu at breakfast", 214.0f, -95.9f, -0.387f, 0.922f, 12 },
    { "dish 0c35", 219.1f, 137.1f, 0.940f, 0.342f, 12 },
    { "dish 0c33", 218.6f, 160.8f, 0.978f, 0.208f, 12 },
    { "dish 0c32", 160.3f, 168.8f, -0.951f, -0.309f, 12 },
};
enum { P4_POSES = sizeof sPoses / sizeof sPoses[0] };
static int sPoseSeen[P4_POSES];
static float sFacingWorst = 1.0f;

static int guest_ok(uint32_t a)
{
    return a > 0x100000u && a < 0x02000000u;
}

static void facing_sample(unsigned frame)
{
    uint32_t g, mgr, node;
    int n = 0;

    if (frame % 6 != 0 || datGetDateIndex() < 10) {
        return; /* from 04/11 (the Inaba arrival): the Velvet Room's props slide without a heading */
    }
    memcpy(&g, (const uint8_t *)P4_G2H(0x00762EA0u), 4);
    if (!guest_ok(g)) {
        return;
    }
    memcpy(&mgr, (const uint8_t *)P4_G2H(g) + 8, 4);
    if (!guest_ok(mgr)) {
        return;
    }
    memcpy(&node, (const uint8_t *)P4_G2H(mgr) + 4 * 3, 4);
    while (guest_ok(node) && n < 24) {
        uint32_t mp, nx;
        float m[16];
        int slot = -1, i;

        memcpy(&mp, (const uint8_t *)P4_G2H(node) + 0x164, 4);
        memcpy(&nx, (const uint8_t *)P4_G2H(node) + 0x138, 4);
        if (guest_ok(mp)) {
            memcpy(m, (const uint8_t *)P4_G2H(mp), 64);
            for (i = 0; i < 24; i++) {
                if (sTrack[i].node == node) {
                    slot = i;
                    break;
                }
                if (slot < 0 && sTrack[i].node == 0) {
                    slot = i;
                }
            }
            for (i = 0; i < P4_POSES; i++) {
                if (fabsf(m[12] - sPoses[i].x) < 1.5f && fabsf(m[14] - sPoses[i].z) < 1.5f) {
                    if (fabsf(m[8] - sPoses[i].atx) <= 0.02f && fabsf(m[10] - sPoses[i].atz) <= 0.02f) {
                        sPoseSeen[i] = 1; /* the model also stands here turning, before and after the greeting: one matching sample is the greeting */
                    }
                }
            }
            if (slot >= 0) {
                FacingTrack *tr = &sTrack[slot];
                const float x = m[12], z = m[14];

                if (tr->node == node) {
                    const float dx = x - tr->x, dz = z - tr->z;
                    const float d = sqrtf(dx * dx + dz * dz);

                    if (d > 4.0f && d < 80.0f) { /* a larger step is a cut, not a walk */
                        const float dot = (m[8] * dx + m[10] * dz) / d;

                        sFacingSamples++;
                        if (dot < sFacingWorst) {
                            sFacingWorst = dot;
                        }
                        if (dot < -0.5f) {
                            /* one sample can be a turn on the spot or a reused node; two in a row are a model walking backwards */
                            if (++tr->bad >= 2) {
                                fprintf(stderr, "p4pc: facing gate: flip %u node %x moved (%.1f, %.1f) per 6 flips, at (%.3f, %.3f), alignment %.3f\n", frame, node, dx, dz, m[8], m[10], dot);
                                sFacingBad++;
                            }
                        } else {
                            tr->bad = 0;
                        }
                    }
                }
                if (tr->node != node) {
                    tr->bad = 0;
                }
                tr->node = node;
                tr->x = x;
                tr->z = z;
            }
        }
        node = nx;
        n++;
    }
}

/* A card continuation owns only its explicit poses; the boot component retains
 * the original sample minimum. Every component still rejects backwards motion.
 * With no component mask, the ordinary whole-game guard is unchanged. */
int p4pc_facing_gate_check(int date)
{
    const char *component = getenv("P4PC_FACING_COMPONENT_MASK");
    unsigned long mask = 0;
    int k, seen = 0, needed = 0;
    if (component != NULL) {
        char *end;
        mask = strtoul(component, &end, 0);
        if (*component == '\0' || *component == '-' || *end != '\0' ||
            mask >= (1ul << P4_POSES)) {
            fprintf(stderr, "p4pc: facing gate: invalid component pose mask\n");
            return 0;
        }
        fprintf(stderr, "p4pc: facing gate: component pose mask 0x%02lx (sample minimum belongs to boot)\n", mask);
    }
    fprintf(stderr, "p4pc: facing gate: %u samples of a scene model moving, %u against its heading, worst alignment %.3f\n", sFacingSamples, sFacingBad, sFacingWorst);
    for (k = 0; k < P4_POSES; k++) {
        if (component != NULL ? !(mask & (1ul << k)) : date < sPoses[k].fromDate) {
            continue;
        }
        needed++;
        seen += sPoseSeen[k];
        if (!sPoseSeen[k]) {
            fprintf(stderr, "p4pc: facing gate: the retail pose of %s was never seen\n", sPoses[k].who);
        }
    }
    fprintf(stderr, "p4pc: facing gate: retail poses seen (Inaba greeting, 04/12 breakfast): %d of %d\n", seen, needed);
    return sFacingBad == 0 && (component != NULL || sFacingSamples >= 40) && seen == needed;
}

void p4pc_newgame_gate_frame(unsigned frame)
{
    static long gate = -2;
    static int wantField = -1, wantRoom = -1, wantMonth = -1, wantDay = -1;

    if (gate == -2) {
        const char *g = getenv("P4PC_NEWGAME_GATE");
        char *end = NULL;

        #ifdef P4_MOD_GAME_BINDINGS
        if ((g && *g) || getenv("P4PC_FACING_GATE")) p4_mod_fidelity_check_start();
        #endif
        gate = -1;
        if (g != NULL && *g != '\0') {
            gate = strtol(g, &end, 0);
            if (end != NULL && *end == ',') {
                wantField = (int)strtol(end + 1, &end, 0);
                if (end != NULL && *end == ':') {
                    wantRoom = (int)strtol(end + 1, &end, 0);
                    if (end != NULL && *end == ',') {
                        wantMonth = (int)strtol(end + 1, &end, 0);
                        if (end != NULL && *end == '/') {
                            wantDay = (int)strtol(end + 1, NULL, 0);
                        }
                    }
                }
            }
        }
    }
    {
        static int facing = -1;

        if (facing < 0) {
            facing = getenv("P4PC_FACING_GATE") != NULL;
        }
        if (facing) {
            facing_sample(frame);
        }
    }
    if (gate < 0) return;
    {
        const int stateGate = p4pc_capture_state_gate(frame);
        if (stateGate == 0 || (stateGate < 0 && frame < (unsigned long)gate)) return;
    }
    {
        const uint8_t *rec = iGpffff9db0;
        const int field = rec != NULL ? *(const int32_t *)rec : -1;
        const int room = rec != NULL ? *(const int32_t *)(rec + 4) : -1;
        const char *max = p4gl_gate_limit("P4PC_REF_MAX");
        const double mad = p4gl_context_capture_mad();
        const int date = datGetDateIndex();
        int ok = (wantField < 0 || (field == wantField && room == wantRoom)) && (wantMonth < 0 || date == calDayOfYearFromMonthDay(wantMonth, wantDay));

        if (getenv("P4PC_FACING_GATE") != NULL) {
            const int facingOk = p4pc_facing_gate_check(date);
            ok = ok && facingOk;
        }
        if (getenv("P4PC_REF_RGB") != NULL) {
            ok = ok && mad >= 0.0 && (max == NULL || mad <= atof(max));
        }
        fprintf(stderr, "p4pc: new-game gate %s at flip %u: field %d room %d (want %d:%d), date index %d (want %d/%d = %d), frame MAD %.4f%% (limit %s)\n", ok ? "PASS" : "FAIL",
                frame, field, room, wantField, wantRoom, date, wantMonth, wantDay, wantMonth < 0 ? -1 : calDayOfYearFromMonthDay(wantMonth, wantDay), mad, max != NULL ? max : "none");
        fflush(stderr);
        {
            const char *result = getenv("P4PC_GATE_RESULT");
            if (result != NULL && *result != '\0' && write_gate_result(result, ok, frame, field, room)) {
                fprintf(stderr, "p4pc: new-game gate: cannot write its result to %s\n", result);
                ok = 0;
            }
        }
        p4pc_exit(ok ? 0 : 1);
    }
}
