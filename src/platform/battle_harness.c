/* Battle harness: P4PC_START_BATTLE=<encounter id> (decimal or 0x hex) boots the engine as the real boot does and,
 * once the boot chain has reached gameSequence (sequence 27: the data tables are loaded, the logo task is about to
 * start), enters the game's own battle path instead of the logo, opening movie and title.
 *
 * What it runs is the body of the AI script command btlScriptStartEncounter (retail 0x001932f0, AI_FUNCTION_00DC),
 * the command the field and the events use to start a battle: a fade task, the player party's DatUnits
 * (datUnitCreatePc), the encounter's enemy group (datUnitCreateEncounterGroup), the encounter record's field/BGM
 * pair, then a sequence change to 6 with the 0x3C-byte packet, which btlTaskCreate (0x00192e90) copies to +0xC70
 * of the battle context. The script command only differs in where the encounter id comes from (scrGetParamInt(0)).
 *
 * P4PC_START_BATTLE_AT=<frames> delays the start by that many presented frames after sequence 27 begins (default 10).
 * The frame counter is the flip count: p4pc_battle_harness_frame is called from RwCameraShowRaster. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/gmem.h"
#include "p4pc/capture_state.h"
#include "p4pc/battle_sim.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif

#define iGpffffb1a8 P4_GPDATA(int32_t, 0xb1a8)   /* the sequence manager's task handle (kwlnTaskCreate in sequenceCreate) */
#define iGpffffb3ac P4_GPDATA(int32_t, 0xb3ac)   /* the battle context (btlTaskCreate), 0xE88 bytes */
#define D_008C024E P4_GARRAY(uint16_t, 0x008C024Eu)   /* the pad edge word the game polls (0x008C024E) */
#define iGpffffb468 P4_GPDATA(uint8_t, 0xb468)   /* the hFade work: state byte, u16 timer at +2, u16 delay at +4 (btlFade.c D_00764558) */
extern uint32_t kwlnGetCamera(void);   /* the scene camera: a guest address (the mirror's `int`) */
extern uint32_t p4gl_frame_mask_get(void);
extern double p4gl_context_capture_nonblack(void);
#define fGpffff8120 P4_GPDATA(float, 0x8120)
#define fGpffff8124 P4_GPDATA(float, 0x8124)
#define fGpffff8100 P4_GPDATA(float, 0x8100)
#define fGpffff811c P4_GPDATA(float, 0x811c)
#define fGpffff8118 P4_GPDATA(float, 0x8118)
#define fGpffff8104 P4_GPDATA(float, 0x8104)
#define fGpffff8054 P4_GPDATA(float, 0x8054)
#define fGpffff8058 P4_GPDATA(float, 0x8058)
#define fGpffff805c P4_GPDATA(float, 0x805c)
#define fGpffff8060 P4_GPDATA(float, 0x8060)
#define fGpffff8108 P4_GPDATA(float, 0x8108)
#define fGpffff80bc P4_GPDATA(float, 0x80bc)
#define fGpffff8110 P4_GPDATA(float, 0x8110)
extern uint8_t *K_Clump_GetLightA(void);
extern uint8_t *K_Clump_GetLightB(void);
extern uint8_t *K_Clump_GetLightC(void);
#define iGpffffb1cc P4_GPDATA(int32_t, 0xb1cc)   /* the global fade object (fadeCreateController): state, mode, duration, counter words */
#define D_005E57A8 P4_GARRAY(char, 0x005E57A8u)   /* the boot logo task's name (logoDraw.c) */
extern void *kwlnTaskGetTaskByName(const void *name);
extern int32_t kwlnTaskDestroyWithHierarchy(void *task);
extern uint32_t datCalcMaxHpById(int32_t id);
extern void datSetUnitHp(int32_t id, int32_t hp);
extern void datSetHeroLevel(int32_t id, int32_t level);   /* the character's level (datInitDefaultGameState passes (1, 1)) */
extern void datSetUnitSp(int32_t id, int32_t sp);
extern uint32_t datCalcMaxSpById(int32_t id);
extern void datInitDefaultGameState(void);   /* what New Game runs: the party's levels, HP/SP, items, flags */
extern void hFadeTaskStart(int16_t mode); /* Also used by the optional live fade-in diagnostic. */

enum { SEQ_GAME = 27 };

static int32_t sEncounter = -2;   /* -2 not read yet, -1 off */
static int sDelay = 10;
static int sSeen27;
static int sDone;

static int32_t currentSequence(void)
{
    uint8_t *task;
    uint8_t *work;

    if (iGpffffb1a8 == 0) {
        return -1;
    }
    task = (uint8_t *)P4_G2H((uint32_t)iGpffffb1a8);
    work = (uint8_t *)P4_G2H(*(uint32_t *)(task + 0x38));
    if (work == NULL) {
        return -1;
    }
    return *(int32_t *)(work + 4);
}

static void startEncounter(int32_t id)
{
    if (getenv("P4PC_START_BATTLE_KEEPSAVE") == NULL) {
        datInitDefaultGameState();   /* a battle needs a party with HP: the boot has no save loaded */
        fprintf(stderr, "p4pc: harness: default game state: hero max hp %u\n", (unsigned)datCalcMaxHpById(1));
        {   /* P4PC_BATTLE_HERO_LEVEL=<n>: raise the hero's level (max HP and SP follow) and fill both, so encounters beyond level 1 can be won */
            const char *lv = getenv("P4PC_BATTLE_HERO_LEVEL");
            if (lv != NULL && *lv != '\0') {
                datSetHeroLevel(1, atoi(lv));
                datSetUnitHp(1, (int32_t)datCalcMaxHpById(1));
                datSetUnitSp(1, (int32_t)datCalcMaxSpById(1));
                fprintf(stderr, "p4pc: harness: hero level %d: max hp %u max sp %u\n", atoi(lv), (unsigned)datCalcMaxHpById(1), (unsigned)datCalcMaxSpById(1));
            }
        }
        {   /* P4PC_BATTLE_HERO_HP=<hp>: a hero that survives encounters it cannot win at level 1 (the win path, the result screen) */
            const char *h = getenv("P4PC_BATTLE_HERO_HP");
            if (h != NULL && *h != '\0') {
                datSetUnitHp(1, atoi(h));
                fprintf(stderr, "p4pc: harness: hero hp set to %d\n", atoi(h));
            }
        }
    }
    {   /* The boot jumps in while the logo task is still alive: it fades the screen out and holds black (the global fade object
         * stays in state 3) once its timeline ends, which hid every battle frame. A real battle starts from the game, not from the logos. */
        void *logo = kwlnTaskGetTaskByName(D_005E57A8);
        if (logo != NULL) {
            fprintf(stderr, "p4pc: harness: destroying the boot logo task %08x\n", (unsigned)P4_H2G(logo));
            kwlnTaskDestroyWithHierarchy(logo);
        }
    }
    if (!p4_battle_encounter_start((uint32_t)id)) {
        fprintf(stderr, "p4pc: harness: invalid/unavailable encounter %d\n", (int)id);
        exit(2);
    }
}

/* The number of x87 registers holding a value (the tag word of FNSTENV): an i386 float return arrives in st(0) and the caller pops it;
 * a call through a prototype that says void (an asm alias, a retail tail jump) leaves it there, and once eight have leaked every x87
 * load (glibc's acosf, sinf, ...) produces the real indefinite NaN 0xFFC00000. */
int p4pc_x87_depth(void)
{
#if defined(__i386__)
    struct { uint16_t cw, p0, sw, p1, tw, p2; uint32_t rest[5]; } env;
    int n = 0, i;

    __asm__ volatile("fnstenv %0\n\tfldenv %0" : "=m"(env));
    for (i = 0; i < 8; i++) {
        if (((env.tw >> (2 * i)) & 3) != 3) {
            n++;
        }
    }
    return n;
#else
    return 0;
#endif
}

void p4pc_battle_harness_frame(unsigned frame)
{
    if (sEncounter == -2) {
        const char *e = getenv("P4PC_START_BATTLE");
        const char *d = getenv("P4PC_START_BATTLE_AT");
        sEncounter = (e != NULL && *e != '\0') ? (int32_t)strtol(e, NULL, 0) : -1;
        if (d != NULL && *d != '\0') {
            sDelay = atoi(d);
        }
        if (sEncounter >= 0) {
            #ifdef P4_MOD_GAME_BINDINGS
            p4_mod_fidelity_check_start();
            #endif
            fprintf(stderr, "p4pc: harness: P4PC_START_BATTLE=%d, starting %d frames after sequence %d begins\n",
                    (int)sEncounter, sDelay, SEQ_GAME);
        }
    }
    if (sEncounter < 0) {
        return;
    }
    if (sDone) {
        {   /* A registered capture has its own state predicate and hard deadline;
             * the fixed-flip fallback keeps its intro-state/nonblack witness. */
            const char *g = getenv("P4PC_BATTLE_GATE");
            const int state_gate = g != NULL ? p4pc_capture_state_gate(frame) : -1;
            if (g != NULL && *g != '\0' &&
                (state_gate > 0 || (state_gate < 0 && frame >= (unsigned)atoi(g)))) {
                uint8_t *c = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
                int32_t st = c != NULL ? *(int32_t *)(c + 0x2FC) : -1;
                double lit = p4gl_context_capture_nonblack();
                int ok = st >= 7 && lit >= 0.5;
                fprintf(stderr, "p4pc: battle gate %s: btlMain state %d (need %s), %.3f of the captured frame lit (need 0.5+)\n", ok ? "PASS" : "FAIL", (int)st, "7+", lit);
                fflush(stderr);
                exit(ok ? 0 : 1);
            }
        }
        {   /* every change of the pad edge word the game sees, so a scripted press can be told from one the game ignored */
            static uint16_t lastEdge;
            if (D_008C024E[0] != lastEdge) {
                lastEdge = D_008C024E[0];
                fprintf(stderr, "p4pc: harness: frame %u pad edge word %04x\n", frame, (unsigned)lastEdge);
            }
        }
        /* a status line a second: the sequence, and the battle context's btlMain state (+0x2FC, next +0x300), its
         * frame counter (+0x304) and the phase btlMainSetPhase wrote (+0x1C) */
        {   /* every change of a packet list's head or tail, with the node's id (+0x40), flags (+0x47), step (+0x45) and
             * callbacks (+0x68 init, +0x6C update, +0x70 destroy) while it is still a node */
            static uint32_t seen[8];
            uint8_t *c = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
            int i;
            for (i = 0; c != NULL && i < 8; i++) {
                uint32_t v = *(uint32_t *)(c + 0x198 + (i / 2) * 8 + (i % 2) * 4);
                if (v != seen[i]) {
                    uint8_t *n = (uint8_t *)P4_G2H(v);
                    seen[i] = v;
                    if (v != 0 && v < 0x02000000u && (v & 3) == 0) {
                        fprintf(stderr, "p4pc: harness: frame %u list %d %s = %08x (id %08x flags %02x step %u init %08x update %08x destroy %08x)\n",
                                frame, i / 2, i % 2 ? "head" : "tail", (unsigned)v, (unsigned)*(uint32_t *)(n + 0x40), n[0x47], n[0x45],
                                (unsigned)*(uint32_t *)(n + 0x68), (unsigned)*(uint32_t *)(n + 0x6C), (unsigned)*(uint32_t *)(n + 0x70));
                    } else {
                        fprintf(stderr, "p4pc: harness: frame %u list %d %s = %08x\n", frame, i / 2, i % 2 ? "head" : "tail", (unsigned)v);
                    }
                }
            }
        }
        {   /* every HP change of any unit (ctx+0x178+8*group list, DatUnit at +0xA64, hp u16 at +8) and every battle-state change */
            static int32_t hpSeen[4][8], spSeen[4][8];
            static int32_t stateSeen = -1;
            uint8_t *c = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
            int g;
            for (g = 0; c != NULL && g < 4; g++) {
                const uint8_t *u = (const uint8_t *)P4_G2H(* (const uint32_t *)(c + 0x178 + 8 * g));
                int n = 0;
                for (; u != NULL && n < 8; u = (const uint8_t *)P4_G2H(* (const uint32_t *)(u + 0xA6C)), n++) {
                    const uint8_t *rec = (const uint8_t *)P4_G2H(* (const uint32_t *)(u + 0xA64));
                    int32_t hp = rec != NULL ? (int32_t) * (const uint16_t *)(rec + 8) : -1;
                    if (hp != hpSeen[g][n]) {
                        if (hpSeen[g][n] != 0 || hp != 0) {
                            fprintf(stderr, "p4pc: harness: frame %u hp: group %d unit %d %d -> %d\n", frame, g, n, hpSeen[g][n], hp);
                        }
                        hpSeen[g][n] = hp;
                    }
                    {   /* SP (DatUnit +0xA): a skill or Persona command spends it */
                        int32_t sp = rec != NULL ? (int32_t) * (const uint16_t *)(rec + 10) : -1;
                        if (sp != spSeen[g][n]) {
                            if (spSeen[g][n] != 0 || sp != 0) {
                                fprintf(stderr, "p4pc: harness: frame %u sp: group %d unit %d %d -> %d\n", frame, g, n, spSeen[g][n], sp);
                            }
                            spSeen[g][n] = sp;
                        }
                    }
                }
            }
            if (c != NULL && *(int32_t *)(c + 0x2FC) != stateSeen) {
                stateSeen = *(int32_t *)(c + 0x2FC);
                fprintf(stderr, "p4pc: harness: frame %u btlMain state -> %d\n", frame, stateSeen);
            }
        }
        static int32_t lastState = -1;
        static unsigned stateSince;
        static int dumped;
        {
            uint8_t *c = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
            int32_t st = c != NULL ? *(int32_t *)(c + 0x2FC) : -1;
            if (st != lastState) {
                lastState = st;
                stateSince = frame;
                dumped = 0;
            } else if (c != NULL && !dumped && frame - stateSince >= 300) {
                int i;
                dumped = 1;
                /* the four packet lists at +0x19C (head, tail per list): each packet's id (+0x40), flags (+0x47), update
                 * (+0x6C) and destroy (+0x70) callbacks, next (+0x7C) */
                for (i = 0; i < 4; i++) {
                    fprintf(stderr, "p4pc: harness: list %d head %08x tail %08x\n", i, (unsigned)*(uint32_t *)(c + i * 8 + 0x19C),
                            (unsigned)*(uint32_t *)(c + i * 8 + 0x198));
                }
                for (i = 0; i < 4; i++) {
                    uint8_t *pk = (uint8_t *)P4_G2H(*(uint32_t *)(c + i * 8 + 0x19C));
                    int n = 0;
                    while (pk != NULL && n < 24) {
                        fprintf(stderr, "p4pc: harness: state %d stuck: list %d packet %p id %08x flags %02x init %08x update %08x destroy %08x\n",
                                (int)st, i, (void *)pk, (unsigned)*(uint32_t *)(pk + 0x40), (unsigned)pk[0x47],
                                (unsigned)*(uint32_t *)(pk + 0x68), (unsigned)*(uint32_t *)(pk + 0x6C), (unsigned)*(uint32_t *)(pk + 0x70));
                        /* the wait conditions (+0x00 and +0x10 before the update, +0x20 and +0x30 after: type byte, then the
                         * awaited UID or frame count at +8), the packet's own UID (+0x58) and its step (+0x45) */
                        fprintf(stderr, "p4pc: harness:    waits before %02x:%08x%08x %02x:%08x%08x after %02x:%08x%08x %02x:%08x%08x  uid %08x%08x step %u delay %d paused %u flags47 %02x steps %u updates %u\n",
                                pk[0x00], *(unsigned *)(pk + 0x0C), *(unsigned *)(pk + 0x08), pk[0x10], *(unsigned *)(pk + 0x1C), *(unsigned *)(pk + 0x18),
                                pk[0x20], *(unsigned *)(pk + 0x2C), *(unsigned *)(pk + 0x28), pk[0x30], *(unsigned *)(pk + 0x3C), *(unsigned *)(pk + 0x38),
                                *(unsigned *)(pk + 0x5C), *(unsigned *)(pk + 0x58), pk[0x45], (int)*(int16_t *)(pk + 0x48), pk[0x46], pk[0x47], *(unsigned *)(pk + 0x4C), *(unsigned *)(pk + 0x50));
                        {   /* the work word of a unit packet: the unit (+0) and its movement words */
                            const uint8_t *w = (const uint8_t *)P4_G2H(*(uint32_t *)(pk + 0x78));
                            const uint8_t *u = w != NULL ? (const uint8_t *)P4_G2H(*(uint32_t *)w) : NULL;

                            if (u != NULL && *(uint32_t *)w > 0x100000u && *(uint32_t *)w < 0x2000000u) {
                                fprintf(stderr, "p4pc: harness:    work %p words %08x %08x %08x %08x; unit %p c4 %08x c8 %04x 98 %08x 9c %08x pos %g %g %g tgt %g %g %g\n",
                                        (const void *)w, *(const unsigned *)w, *(const unsigned *)(w + 4), *(const unsigned *)(w + 8),
                                        *(const unsigned *)(w + 12), (const void *)u, *(const unsigned *)(u + 0xC4),
                                        (unsigned)*(const uint16_t *)(u + 0xC8), *(const unsigned *)(u + 0x98), *(const unsigned *)(u + 0x9C),
                                        *(const float *)(u + 4), *(const float *)(u + 8), *(const float *)(u + 0xC),
                                        *(const float *)(u + 0xD0), *(const float *)(u + 0xD4), *(const float *)(u + 0xD8));
                                fprintf(stderr, "p4pc: harness:    unit quat %.9g %.9g %.9g %.9g (%08x %08x %08x %08x) rot speed %g\n", *(const float *)(u + 0x1C), *(const float *)(u + 0x20),
                                        *(const float *)(u + 0x24), *(const float *)(u + 0x28), *(const unsigned *)(u + 0x1C), *(const unsigned *)(u + 0x20),
                                        *(const unsigned *)(u + 0x24), *(const unsigned *)(u + 0x28), *(const float *)(u + 0x4F4));
                            }
                        }
                        pk = (uint8_t *)P4_G2H(*(uint32_t *)(pk + 0x7C));
                        n++;
                    }
                }
            }
        }
        {   /* P4PC_BATTLE_FADEIN=<battle frames>: start a fade-in (hFadeTaskStart(0)) that many btlMain frames into state 8 */
            static int fadeInDone;
            const char *f = getenv("P4PC_BATTLE_FADEIN");
            uint8_t *c = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
            if (f != NULL && !fadeInDone && c != NULL && *(int32_t *)(c + 0x2FC) == 8 && *(int32_t *)(c + 0x304) >= atoi(f)) {
                fadeInDone = 1;
                fprintf(stderr, "p4pc: harness: frame %u: fade in\n", frame);
                hFadeTaskStart(0);
            }
        }
        if (frame % 60 == 0) {
            uint8_t *ctx = (uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
            if (ctx != NULL) {
                fprintf(stderr, "p4pc: harness: frame %u seq %d battle state %d next %d frames %d phase %d flags %08x\n", frame,
                        (int)currentSequence(), *(int32_t *)(ctx + 0x2FC), *(int32_t *)(ctx + 0x300), *(int32_t *)(ctx + 0x304),
                        (int)*(int16_t *)(ctx + 0x1C), (unsigned)*(uint32_t *)(ctx + 0xC));
                fprintf(stderr, "p4pc: harness: frame %u x87 registers in use: %d\n", frame, p4pc_x87_depth());
                fprintf(stderr, "p4pc: harness: frame %u gp floats 8120 %g 8124 %g 8100 %g 811c %g 8118 %g 8104 %g 8054 %g 8058 %g 805c %g 8060 %g 8108 %g 80bc %g 8110 %g\n", frame,
                        fGpffff8120, fGpffff8124, fGpffff8100, fGpffff811c, fGpffff8118, fGpffff8104, fGpffff8054, fGpffff8058, fGpffff805c, fGpffff8060, fGpffff8108,
                        fGpffff80bc, fGpffff8110);
                {   /* the camera pose the battle applies (ctx+0xCC quaternion, ctx+0xC0 position) and the curve's four keyframes at ctx+0x24
                     * (u16 flags at +0, count +0x76, next +0x78; key i: position at +4+0x1c*i, quaternion at +0x10+0x1c*i) */
                    const float *q = (const float *)(ctx + 0xCC);
                    int k;
                    fprintf(stderr, "p4pc: harness: frame %u ctx %p pose quat %g %g %g %g pos %g %g %g curve flags %04x count %u next %u\n", frame, (const void *)ctx, q[0], q[1], q[2], q[3],
                            *(const float *)(ctx + 0xC0), *(const float *)(ctx + 0xC4), *(const float *)(ctx + 0xC8),
                            (unsigned)*(const uint16_t *)(ctx + 0x24), (unsigned)*(const uint16_t *)(ctx + 0x24 + 0x76), (unsigned)*(const uint16_t *)(ctx + 0x24 + 0x78));
                    for (k = 0; k < 4; k++) {
                        const float *key = (const float *)(ctx + 0x24 + 4 + 0x1c * k);
                        fprintf(stderr, "p4pc: harness: frame %u   key %d pos %g %g %g quat %g %g %g %g\n", frame, k, key[0], key[1], key[2], key[3], key[4], key[5], key[6]);
                    }
                }
                {
                    const uint8_t *cam = (const uint8_t *)P4_G2H(kwlnGetCamera());
                    const uint8_t *fr = cam != NULL ? (const uint8_t *)P4_G2H(* (const uint32_t *)(cam + 4)) : NULL;
                    if (fr != NULL) {
                        const float *m = (const float *)(fr + 80);   /* RwFrame.ltm: right, up, at, pos (16 bytes each) */
                        fprintf(stderr, "p4pc: harness: frame %u camera %p frame %p view %g x %g near %g far %g pos %g %g %g at %g %g %g\n", frame,
                                (const void *)cam, (const void *)fr, *(const float *)(cam + 104), *(const float *)(cam + 108), *(const float *)(cam + 128),
                                *(const float *)(cam + 132), m[12], m[13], m[14], m[8], m[9], m[10]);
                    } else {
                        fprintf(stderr, "p4pc: harness: frame %u camera %p has no frame\n", frame, (const void *)cam);
                    }
                }
                {   /* the unit lists (ctx+0x178+8*group, next at +0xA6C): flags word at +0x9C (bit 8 = shown), position +4 */
                    int g;
                    for (g = 0; g < 4; g++) {
                        const uint8_t *u = (const uint8_t *)P4_G2H(* (const uint32_t *)(ctx + 0x178 + 8 * g));
                        int n = 0;
                        for (; u != NULL && n < 8; u = (const uint8_t *)P4_G2H(* (const uint32_t *)(u + 0xA6C)), n++) {
                            const uint8_t *rec = (const uint8_t *)P4_G2H(* (const uint32_t *)(u + 0xA64));   /* the DatUnit: hp u16 at +8 */
                            fprintf(stderr, "p4pc: harness: frame %u group %d unit %d at %p flags %08x pos %g %g %g scale %g hp %d\n", frame, g, n,
                                    (const void *)u, (unsigned)*(const uint32_t *)(u + 0x9C), *(const float *)(u + 4), *(const float *)(u + 8),
                                    *(const float *)(u + 12), *(const float *)(u + 0x2C), rec != NULL ? (int)*(const uint16_t *)(rec + 8) : -1);
                        }
                    }
                }
                {   /* the lights the atomic renderer gathers: RpLight type 3 at +0, subtype +1, flags +2, colour floats at +24 */
                    uint8_t *(*const get[3])(void) = {K_Clump_GetLightA, K_Clump_GetLightB, K_Clump_GetLightC};
                    int i;
                    for (i = 0; i < 3; i++) {
                        const uint8_t *l = get[i]();
                        if (l == NULL) {
                            fprintf(stderr, "p4pc: harness: frame %u light %c: none\n", frame, 'A' + i);
                        } else {
                            fprintf(stderr, "p4pc: harness: frame %u light %c %p: type %u subtype %u flags %02x colour %g %g %g\n", frame,
                                    'A' + i, (const void *)l, l[0], l[1], l[2], *(const float *)(l + 24), *(const float *)(l + 28),
                                    *(const float *)(l + 32));
                        }
                    }
                }
                {
                    const int32_t *fo = (const int32_t *)P4_G2H((uint32_t)iGpffffb1cc);
                    if (fo != NULL) {
                        fprintf(stderr, "p4pc: harness: frame %u global fade: state %d mode %d duration %d counter %d\n", frame, fo[0], fo[1], fo[2], fo[3]);
                    } else {
                        fprintf(stderr, "p4pc: harness: frame %u global fade: not created\n", frame);
                    }
                }
                fprintf(stderr, "p4pc: harness: frame %u frame mask %08x\n", frame, (unsigned)p4gl_frame_mask_get());
                fprintf(stderr, "p4pc: harness: frame %u fade state %u timer %u delay %u\n", frame, (&iGpffffb468)[0],
                        (unsigned)*(uint16_t *)(&iGpffffb468 + 2), (unsigned)*(uint16_t *)(&iGpffffb468 + 4));
            } else {
                fprintf(stderr, "p4pc: harness: frame %u seq %d, no battle context\n", frame, (int)currentSequence());
            }
        }
        return;
    }
    if (currentSequence() == SEQ_GAME) {
        if (!sSeen27) {
            sSeen27 = (int)frame ? (int)frame : 1;
        }
        /* The logo destructor requires both title-loader children to have finished.
         * A frame delay alone can expire while asynchronous logo.bin is still loading. */
        uint8_t *logo = (uint8_t *)kwlnTaskGetTaskByName(D_005E57A8);
        uint8_t *work = logo != NULL ? (uint8_t *)P4_G2H(*(uint32_t *)(logo + 0x38)) : NULL;

        if (work == NULL || *(int32_t *)work < 4) {
            return;
        }
        if ((int)frame - sSeen27 >= sDelay) {
            sDone = 1;
            startEncounter(sEncounter);
        }
    }
}
