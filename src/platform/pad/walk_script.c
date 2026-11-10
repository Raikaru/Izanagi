/* Waypoint scripts (include/p4pc/walk_script.h): the parser and the per-frame state machine. Plain C with a read32 callback, so retail's
 * p4hook (guest memory through pcsx2-hook) and the port (guest memory is the host's) use the same file. */
#include "p4pc/walk_script.h"
#include "p4pc/guest.h"
#include "p4pc/hostmath.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TASK_LISTS 3
static const uint32_t kTaskLists[TASK_LISTS] = {0x00764adcu, 0x00764af4u, 0x00764ae8u};
#define TASK_WORK 0x38u
#define TASK_NEXT 0x3cu
#define FIELD_ROOT_HANDLE 0x007D2510u
#define CAMERA_POINTER 0x00764B70u
#define SCENE_OBJECT 0x00762EA0u  /* pointer to the scene object; +8 is the resource manager (list heads by type) */
#define RNG_SLOT_INDEX 0x00764708u   /* iGpffffb618: the generator's slot in the engine block at 0x008872E0 (base, front, back, end) */
#define EVENT_FLAG 0x00764604u   /* gp-0x4AEC: 1 while a message window / event holds the player's input (0 in free roam; found by diffing RAM) */
#define MAX_COMMANDS 1024
#define DEFAULT_TIMEOUT 3000
#define STUCK_FRAMES 150
#define STUCK_PROGRESS 3.0f

typedef enum { C_WALK, C_FACE, C_PRESS, C_WAIT, C_WAITFIELD, C_WAITSTEP, C_WAITSTATE, C_WAITCONTROL, C_WAITMSG, C_SKIPMSG, C_SKIPFOR, C_SKIPFIELD, C_SKIPEVTMSG, C_WAITCHOICE, C_CHOOSE, C_AUTOSKIP, C_BATTLESKIP, C_PLAN, C_BATTLE, C_BATTLERULE, C_BATTLETURN, C_WAITBTL, C_WAITTASK, C_SKIPTASK, C_NAVPROBE, C_NAVDUMP, C_LOG, C_PUSH, C_EXIT } Kind;

typedef struct {
    Kind kind;
    float a, b, c;          /* walk: x, z, radius; face: degrees; press: hold; wait: frames */
    int button;
    uint32_t addr, value;
    int ne;                 /* waitstate: != */
    int on;                 /* waitcontrol */
    uint32_t timeout;
    char text[512];         /* retain item-selection macros, per-character roles and full-HP cancellation keys */
} Cmd;

#define BTL_PLAN_MAX 32
#define BTL_PARTY_MAX 8
#define BATTLE_CONTEXT 0x0076449Cu         /* gp-0x4C54: iGpffffb3ac, the battle context (btlTaskCreate) */
#define BTL_CMD_STATE 8
#define BTL_RESULT_STATE 16       /* battle phase 16: the result task, which waits for every persona level-up */
#define BTL_ACTION_COMMAND 5u     /* retail action table: COMMAND (001a1450 / 001a17d0), not AI/STANDBY */
#define BTL_ACTION_TARGET 6u      /* TARGET (001a1c80 / 001a1ea0) confirms to action 15, cancels to COMMAND */
#define BTL_ACTION_ALL_OUT_PROMPT 30u /* Yes/No before a requested COMMAND turn: Circle declines */
#define BTL_ACTION_RUNNING 0x400000u

/* battle:first=a,b,..;plan=a,b,..;skip=N;delay=N (see the header): the same logic as the port's P4PC_BATTLE_DRIVER (src/platform/battle_driver.c), in the
 * walker's frames so retail plays the fights the port plays. */
typedef struct {
    int on;
    unsigned plan[BTL_PLAN_MAX], first[BTL_PLAN_MAX];
    int plan_len, first_len;
    unsigned first_pc[BTL_PARTY_MAX][BTL_PLAN_MAX];
    int first_pc_len[BTL_PARTY_MAX];
    unsigned plan_pc[BTL_PARTY_MAX][BTL_PLAN_MAX];
    int plan_pc_len[BTL_PARTY_MAX];
    unsigned skip, delay;
    unsigned guard_enemy;
    unsigned guard_party;                  /* 0: all party units; otherwise bits indexed by native unit ID - 1 */
    unsigned target_pc[BTL_PARTY_MAX];      /* optional native enemy ID, enforced before TARGET confirmation */
    unsigned target_moves;
    int owns;                              /* a battle task exists and the driver has the pad */
    int32_t seen;
    uint32_t state_start, wait_since, start_at, next_skip, hold_until;
    uint32_t plan_actor;
    unsigned hold_mask;
    int acted, last_waiting, step, cur_first, cur_guard;
    unsigned cur_pc;
    uint32_t shuffle_at;                   /* Shuffle Time: when the next cursor move or flip is due */
    int shuffle_step;
    int persona_modal;                    /* explicit result-input profile; legacy card cadence stays unchanged */
    uint8_t commanded[BTL_PARTY_MAX];      /* the wheel remembers a character's last command inside a fight */
    P4WalkBattleRule rules[P4WALK_BATTLE_RULE_MAX]; /* battlerule: lines, in order; cleared with the driver */
    int nrules;
    int cur_rule;                          /* the rule supplying the armed command's buttons, -1 for first/plan/guard */
    uint32_t cur_ally;                     /* its bound ally's native ID, 0 for none */
    uint32_t cur_enemy;                    /* its bound enemy's native ID (weak:/hits:), 0 for none */
    int nav_left;                          /* skill:ID presses still due at the current step, -1 before they are counted */
    unsigned nav_key;                      /* their button: 1 << P4W_DOWN or 1 << P4W_UP */
    uint32_t prompt_at;                    /* the next results-screen skill-offer press is due */
    P4WalkBattleRule turns[P4WALK_BATTLE_TURN_MAX]; /* battleturn: lines: the Nth supplies the Nth fresh command */
    int nturns, turn_next;                 /* lines given, and the next one to arm */
    int cur_turn;                          /* the armed command's line, -1 for none, -2 for the host's line in host_turn */
    int turn_retry;                        /* the line to retry after a command that did not take, -1 for none */
    unsigned host_turns;                   /* commands the host's battle_turn callback has supplied */
    P4WalkBattleRule host_turn;
} BattleDriver;

struct NavMap;

struct P4Walk {
    Cmd cmd[MAX_COMMANDS];
    int n, at;
    uint32_t start;         /* frame the current command began */
    int started;
    float best;             /* walk: best distance so far */
    uint32_t best_frame;
    uint32_t control_frames; /* skipmsg: consecutive frames under control */
    int plan;                /* plan:on: walk:X,Z routes over the floor's cell grid too (default off; walk:shadow/unit always do) */
    int battleskip;          /* battleskip:on: a walk presses Cross through battles/scenes instead of waiting for control */
    int autoskip;            /* walk / face press Cross every 24 frames while an event holds the input */
    uint32_t event_frames;   /* frames the current walk spent in an event (they do not count toward stuck or timeout) */
    uint32_t last_game_frame; /* p4walk_tick: the game-frame counter value of the last step (0 = none yet) */
    uint32_t mask;            /* the buttons held since that step */
    int door_tries;          /* Cross presses at a standstill (a gate or curtain) before the walk gives up */
    uint32_t cross_until;
    int gate_phase;           /* 0 none, 1 steering at the gate, 2 Cross, 3 waiting for it to open */
    uint32_t gate_until;
    float gate_x, gate_z;
    int resets;               /* times the list of shut edges was dropped because it left no route */
    int nblocked;             /* edges (cell column, row, direction) the walk found shut: the planner routes around them */
    int blocked[16][3];
    int step_c, step_r, step_d;   /* the edge the planner last chose */
    float aim_x, aim_z;       /* the point the planner last steered at (for the failure message) */
    uint32_t goal_field, goal_step; /* walk:goal belongs to the field/floor where it first resolved */
    int goal_scoped;
    BattleDriver bd;
    uint32_t btl_menu_frames;
    uint32_t choice_window, choice_next, choice_hold_until, choice_mask;
    int choice_commit;
    struct NavMap *nav;       /* nav_*: the sampled collision world of the current floor */
    int nav_ready, nav_active;
    uint32_t nav_replan;
    float nav_wx, nav_wz, nav_remain, nav_tx, nav_tz, nav_lx, nav_lz;
    int nav_forced;
    int gate_cycles;          /* gates the nav walk faced and Crossed in this command */
    int nav_gate;             /* the planned way crosses a closed gate near (nav_gx, nav_gz) */
    float nav_gx, nav_gz;
    int no_detour;            /* a walk that stalled while detouring around a room's middle (a bridge, not a stage) goes straight through */
    int detoured;             /* the last planned aim was such a detour */
    int status;
};

static const char *kButtons[P4W_BUTTON_COUNT] = {"up", "down", "left", "right", "cross", "circle", "triangle", "square",
                                                 "start", "select", "l1", "r1", "l2", "r2", "l3", "r3"};

static uint32_t parse_num(const char *s, char **end) { return (uint32_t)strtoul(s, end, 0); }
/* Host strtof rounds by the thread's float mode, and the Windows CRT follows MXCSR's round-toward-zero where glibc rounds to nearest (about half of the
 * decimal fractions then parse one ulp apart): a script's numbers parse in round-to-nearest, the same value on every target. */
static float parse_float(const char *s, char **end) {
    const unsigned csr = p4pc_fpu_host_enter();
    const float v = strtof(s, end);
    p4pc_fpu_host_leave(csr);
    return v;
}

static int battle_plan(const char *s, size_t len, unsigned *out, int *n) {
    *n = 0;
    while (len > 0) {
        const char *c = memchr(s, ',', len);
        size_t k = c ? (size_t)(c - s) : len;
        int i;
        int item = k > 5 && !strncmp(s, "item:", 5);
        if (item || (k > 6 && !strncmp(s, "skill:", 6))) {   /* skill:ID / item:ID: navigate the open Skill or Item list to that entry */
            size_t at = item ? 5 : 6;
            char num[16], *end;
            if (k - at >= sizeof num || *n >= BTL_PLAN_MAX) return 0;
            memcpy(num, s + at, k - at);
            num[k - at] = 0;
            unsigned long id = strtoul(num, &end, 0);
            /* Skills: a retail active ID below the 0x1B8 split, or an added one
             * (576..4095, P4_SKILL_MAX_ID); retail passives are never listed. */
            if (*end || (item ? id <= 0x300u || id >= 0x400u :
                !id || (id >= 0x1B8u && id < 0x240u) || id > 0xFFFu)) return 0;
            out[(*n)++] = (item ? P4WALK_KEY_ITEM : P4WALK_KEY_SKILL) | (unsigned)id;
            s += k + (c ? 1 : 0);
            len -= k + (c ? 1 : 0);
            continue;
        }
        for (i = 0; i < P4W_BUTTON_COUNT; i++)
            if (strlen(kButtons[i]) == k && !strncmp(kButtons[i], s, k)) break;
        if (i == P4W_BUTTON_COUNT || *n >= BTL_PLAN_MAX) return 0;
        out[(*n)++] = 1u << i;
        s += k + (c ? 1 : 0);
        len -= k + (c ? 1 : 0);
    }
    return 1;
}

/* "first=down,cross;plan=cross;skip=140;delay=20" or "off" */
static int battle_parse(const char *s, BattleDriver *d) {
    memset(d, 0, sizeof *d);
    d->delay = 20;
    d->seen = -2;
    d->cur_rule = -1;
    d->cur_turn = -1;
    d->turn_retry = -1;
    if (!strcmp(s, "off")) return 1;
    d->on = 1;
    d->first_len = -1;
    while (*s) {
        const char *e = strchr(s, ';');
        size_t len = e ? (size_t)(e - s) : strlen(s);
        if (len > 5 && !strncmp(s, "plan=", 5)) { if (!battle_plan(s + 5, len - 5, d->plan, &d->plan_len)) return 0; }
        else if (len > 6 && !strncmp(s, "first=", 6)) { if (!battle_plan(s + 6, len - 6, d->first, &d->first_len)) return 0; }
        else if (len > 7 && !strncmp(s, "first", 5) && s[5] >= '1' && s[5] < '1' + BTL_PARTY_MAX && s[6] == '=') {
            unsigned pc = (unsigned)(s[5] - '1');
            if (!battle_plan(s + 7, len - 7, d->first_pc[pc], &d->first_pc_len[pc])) return 0;
        }
        else if (len > 6 && !strncmp(s, "plan", 4) && s[4] >= '1' && s[4] < '1' + BTL_PARTY_MAX && s[5] == '=') {
            unsigned pc = (unsigned)(s[4] - '1');
            if (!battle_plan(s + 6, len - 6, d->plan_pc[pc], &d->plan_pc_len[pc])) return 0;
        }
        else if (len > 8 && !strncmp(s, "target", 6) && s[6] >= '1' && s[6] < '1' + BTL_PARTY_MAX && s[7] == '=') {
            char *end;
            unsigned long id = strtoul(s + 8, &end, 0);
            if (end != s + len || id == 0 || id > 0xFFFFu) return 0;
            d->target_pc[s[6] - '1'] = (unsigned)id;
        }
        else if (len > 5 && !strncmp(s, "skip=", 5)) d->skip = (unsigned)strtoul(s + 5, NULL, 10);
        else if (len > 6 && !strncmp(s, "delay=", 6)) d->delay = (unsigned)strtoul(s + 6, NULL, 10);
        else if (len == 13 && !strncmp(s, "persona=modal", 13)) d->persona_modal = 1;
        else if (len > 6 && !strncmp(s, "guard=", 6)) {
            char *end;
            unsigned long id = strtoul(s + 6, &end, 0);
            if (id == 0 || id > 0xFFFFu) return 0;
            d->guard_enemy = (unsigned)id;
            d->guard_party = 0;
            if (end != s + len) {
                if (*end++ != ':') return 0;
                for (;;) {
                    char *start = end;
                    unsigned long pc = strtoul(start, &end, 10);
                    if (end == start || end > s + len || pc < 1 || pc > BTL_PARTY_MAX) return 0;
                    d->guard_party |= 1u << (pc - 1u);
                    if (end == s + len) break;
                    if (*end++ != ',' || end == s + len) return 0;
                }
            }
        }
        else return 0;
        s += len + (e ? 1 : 0);
    }
    if (d->plan_len <= 0) return 0;
    if (d->first_len <= 0) { memcpy(d->first, d->plan, sizeof d->first); d->first_len = d->plan_len; }
    return 1;
}

/* A rule's numeric field: digits from the first character (no sign or space), within [lo, hi], ending exactly at `end`. */
static int rule_number(const char *s, const char *end, int base, unsigned long lo, unsigned long hi, uint32_t *out) {
    char *stop;
    if (s >= end || *s < '0' || *s > '9') return 0;
    unsigned long v = strtoul(s, &stop, base);
    if (stop != end || v < lo || v > hi) return 0;
    *out = (uint32_t)v;
    return 1;
}

int p4walk_battle_rule_parse(const char *text, P4WalkBattleRule *out) {
    memset(out, 0, sizeof *out);
    const char *eq = strchr(text, '=');
    if (!eq || !eq[1]) return 0;
    const char *at = memchr(text, '@', (size_t)(eq - text));
    const char *cond = at ? at : eq;
    size_t n = (size_t)(cond - text);
    if (n == 4 && !strncmp(text, "dead", 4)) out->kind = P4WALK_RULE_DEAD;
    else if (n > 3 && !strncmp(text, "hp<", 3)) {
        if (!rule_number(text + 3, cond, 10, 1, 0xFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_HP_BELOW;
    } else if (n > 3 && !strncmp(text, "sp<", 3)) {
        if (!rule_number(text + 3, cond, 10, 1, 0xFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_SP_BELOW;
    } else if (n > 7 && !strncmp(text, "charge:", 7)) {
        if (!rule_number(text + 7, cond, 0, 1, 0xFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_CHARGE;
    } else if (n > 7 && !strncmp(text, "status:", 7)) {
        if (!rule_number(text + 7, cond, 0, 1, 0xFFFFFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_STATUS;
    } else if (n > 5 && !strncmp(text, "buff:", 5)) {
        if (!rule_number(text + 5, cond, 0, 1, 0x3FFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_BUFF;
    } else if (n > 7 && !strncmp(text, "nobuff:", 7)) {
        if (!rule_number(text + 7, cond, 0, 1, 0x3FFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_NOBUFF;
    } else if (n > 8 && !strncmp(text, "foebuff:", 8)) {
        if (!rule_number(text + 8, cond, 0, 1, 0x3FFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_FOEBUFF;
    } else if (n > 10 && !strncmp(text, "afflicted:", 10)) {
        if (!rule_number(text + 10, cond, 0, 1, 0xFFFFFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_AFFLICTED;
    } else if (n > 6 && !strncmp(text, "enemy:", 6)) {
        if (!rule_number(text + 6, cond, 0, 1, 0xFFFFu, &out->value)) return 0;
        out->kind = P4WALK_RULE_ENEMY;
    } else if (n > 5 && (!strncmp(text, "weak:", 5) || !strncmp(text, "hits:", 5))) {
        if (!rule_number(text + 5, cond, 10, 0, 7, &out->value)) return 0;
        out->kind = text[0] == 'w' ? P4WALK_RULE_WEAK : P4WALK_RULE_HITS;
    } else return 0;
    if (at) {
        const char *p = at + 1;
        for (;;) {
            const char *comma = memchr(p, ',', (size_t)(eq - p));
            const char *end = comma ? comma : eq;
            uint32_t id;
            if (!rule_number(p, end, 10, 1, BTL_PARTY_MAX, &id)) return 0;
            out->actors |= 1u << (id - 1u);
            if (!comma) break;
            p = comma + 1;
        }
    }
    return battle_plan(eq + 1, strlen(eq + 1), out->keys, &out->key_count) && out->key_count > 0;
}

int p4walk_battle_turn_parse(const char *text, P4WalkBattleRule *out) {
    memset(out, 0, sizeof *out);
    const char *eq = strchr(text, '='), *keys = text;
    if (eq) {
        static const struct { const char *name; int kind; } conds[] = {
            {"enemy:", P4WALK_RULE_ENEMY}, {"ally:", P4WALK_RULE_ALLY}, {"dead:", P4WALK_RULE_DEAD}};
        size_t i;
        for (i = 0; i < sizeof conds / sizeof conds[0]; i++) {
            size_t n = strlen(conds[i].name);
            if (!strncmp(text, conds[i].name, n) && (size_t)(eq - text) > n) break;
        }
        if (i == sizeof conds / sizeof conds[0]) return 0;
        out->kind = conds[i].kind;
        if (!rule_number(text + strlen(conds[i].name), eq, 0, 1, out->kind == P4WALK_RULE_ENEMY ? 0xFFFFu : BTL_PARTY_MAX, &out->value)) return 0;
        keys = eq + 1;
    }
    return battle_plan(keys, strlen(keys), out->keys, &out->key_count) && out->key_count > 0;
}

/* datCalcGetBuffLevel: index i's signed nibble at DatUnit +0x1C + i/2 (odd indices high), values above 7 minus 15. */
static int rule_buff_level(const P4WalkBattleUnit *u, unsigned index) {
    int v = (u->buffs[index >> 1] >> ((index & 1u) * 4u)) & 0xF;
    return v > 7 ? v - 15 : v;
}

/* datCalcHasBuffMask's stat bits: 2i selects stat i raised, 2i+1 lowered (0x80: defense down); any selected bit matches. */
static int rule_buff_match(const P4WalkBattleUnit *u, uint32_t mask) {
    for (unsigned stat = 0; stat < 5; stat++) {
        int level = rule_buff_level(u, stat);
        if (((mask >> (2u * stat)) & 1u) && level > 0) return 1;
        if (((mask >> (2u * stat + 1u)) & 1u) && level < 0) return 1;
    }
    return 0;
}

int p4walk_battle_rule_select(const P4WalkBattleRule *rules, int count, const P4WalkBattleView *v, uint32_t *ally, uint32_t *enemy) {
    *ally = 0;
    *enemy = 0;
    if (v->actor < 1 || v->actor > BTL_PARTY_MAX) return -1;
    const P4WalkBattleUnit *self = NULL;
    for (int i = 0; i < v->party_count; i++)
        if (v->party[i].id == v->actor) { self = &v->party[i]; break; }
    for (int r = 0; r < count; r++) {
        const P4WalkBattleRule *rule = &rules[r];
        if (rule->actors && !(rule->actors & (1u << (v->actor - 1u)))) continue;
        if (v->unusable & (1u << r)) continue;
        int best = -1;
        switch (rule->kind) {
        case P4WALK_RULE_DEAD:
            for (int i = 0; i < v->party_count && best < 0; i++)
                if (!v->party[i].hp) best = i;
            break;
        case P4WALK_RULE_HP_BELOW:
            for (int i = 0; i < v->party_count; i++)
                if (v->party[i].hp && v->party[i].hp < rule->value && (best < 0 || v->party[i].hp < v->party[best].hp)) best = i;
            break;
        case P4WALK_RULE_AFFLICTED:
            for (int i = 0; i < v->party_count && best < 0; i++)
                if (v->party[i].hp && (v->party[i].bad & rule->value)) best = i;
            break;
        case P4WALK_RULE_SP_BELOW:
            if (self && self->sp < rule->value) return r;
            continue;
        case P4WALK_RULE_CHARGE:
            for (int i = 0; i < v->enemy_count; i++)
                if (v->enemies[i].id == rule->value && v->enemies[i].hp && (v->enemies[i].status & 0xC0000u)) return r;
            continue;
        case P4WALK_RULE_ENEMY:
            for (int i = 0; i < v->enemy_count; i++)
                if (v->enemies[i].id == rule->value && v->enemies[i].hp) { *enemy = v->enemies[i].id; return r; }
            continue;
        case P4WALK_RULE_STATUS:
            if (self && (self->bad & rule->value)) return r;
            continue;
        case P4WALK_RULE_BUFF:
            if (self && rule_buff_match(self, rule->value)) return r;
            continue;
        case P4WALK_RULE_NOBUFF:
            if (self && !rule_buff_match(self, rule->value)) return r;
            continue;
        case P4WALK_RULE_FOEBUFF:
            for (int i = 0; i < v->enemy_count; i++)
                if (v->enemies[i].hp && rule_buff_match(&v->enemies[i], rule->value)) { *enemy = v->enemies[i].id; return r; }
            continue;
        case P4WALK_RULE_WEAK:
        case P4WALK_RULE_HITS:
            for (int i = 0; i < v->enemy_count; i++) {
                const P4WalkBattleUnit *e = &v->enemies[i];
                unsigned bit = 1u << rule->value;
                if (!e->hp) continue;
                if (rule->kind == P4WALK_RULE_WEAK ? (e->weak & bit) != 0 : !((e->block | e->resist) & bit)) { *enemy = e->id; return r; }
            }
            continue;
        default:
            continue;
        }
        if (best >= 0) { *ally = v->party[best].id; return r; }
    }
    return -1;
}

uint32_t p4walk_enemy_affinity(unsigned raw, unsigned passives, int guarding, int broken, int walled) {
    uint32_t affinity = ((uint32_t)(raw & 0xFF00u) << 16) | ((raw & 0xFFu) * 5u);
    if (!(affinity & 0xFFFFu)) {
        if (affinity & 0x8000000u) affinity |= 0x7Du;
        else if (affinity & 0x27000000u) affinity |= 100u;
        else if (affinity & 0x10000000u) affinity |= 0x32u;
    }
    if (guarding && (affinity & 0x8000000u)) affinity = (affinity & 0xF7FF0000u) | 100u;
    if ((passives & 1u) && !(affinity & 0x7000000u)) affinity = (((affinity & 0xF7FFFFFFu) | 0x10000000u) & 0xFFFF0000u) | 0x32u;
    if ((passives & 2u) && !(affinity & 0x6000000u)) affinity = (((affinity & 0xE7FFFFFFu) | 0x1000000u) & 0xFFFF0000u) | 0x64u;
    if ((passives & 4u) && !(affinity & 0x4000000u)) affinity = (((affinity & 0xE6FFFFFFu) | 0x2000000u) & 0xFFFF0000u) | 0x64u;
    if (passives & 8u) affinity = (((affinity & 0xE4FFFFFFu) | 0x4000000u) & 0xFFFF0000u) | 0x64u;
    if (broken && (affinity & 0x17000000u)) affinity = (affinity & 0xE8FF0000u) | 100u;
    if (walled && ((affinity & 0x8000000u) || !(affinity & 0xFF000000u)))
        affinity = (((affinity & 0xFFFFFFu) | 0x10000000u) & 0xFFFF0000u) | 0x32u;
    return affinity;
}

int p4walk_battle_list_moves(const uint16_t *list, int count, int cursor, unsigned id, int *moves) {
    if (count <= 0 || cursor < 0 || cursor >= count) return 0;
    for (int i = 0; i < count; i++) {
        if (list[i] != id) continue;
        int down = (i - cursor + count) % count;
        *moves = down <= count - down ? down : down - count;
        return 1;
    }
    return 0;
}

P4Walk *p4walk_parse(const char *text, char *err, size_t errsz) {
    P4Walk *w = calloc(1, sizeof *w);
    w->autoskip = 1;
    uint32_t timeout = DEFAULT_TIMEOUT;
    int lineno = 0;
    const char *p = text;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        char line[sizeof w->cmd[0].text];
        lineno++;
        if (len >= sizeof line) len = sizeof line - 1;
        memcpy(line, p, len);
        line[len] = 0;
        p = eol ? eol + 1 : p + len;
        char *h = strchr(line, '#');
        if (h) *h = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t' || *s == '\r') s++;
        size_t sl = strlen(s);
        while (sl && (s[sl - 1] == ' ' || s[sl - 1] == '\t' || s[sl - 1] == '\r')) s[--sl] = 0;
        if (!*s) continue;
        if (w->n >= MAX_COMMANDS) { snprintf(err, errsz, "too many commands"); goto fail; }
        Cmd *c = &w->cmd[w->n];
        memset(c, 0, sizeof *c);
        snprintf(c->text, sizeof c->text, "%s", s);
        c->timeout = timeout;
        char *e;
        if (!strncmp(s, "walk:unit", 9) || !strncmp(s, "walk:shadow", 11) || !strncmp(s, "walk:goal", 9) || !strncmp(s, "walk:entry", 10) || !strncmp(s, "walk:obj:", 9)) {   /* walk:obj:ID: a list 0xA object; unit/shadow, or a live floor stair quad */
            int unit = s[5] == 'u';
            c->kind = C_WALK;
            c->on = 1;
            c->c = 150.0f;
            e = s + (s[5] == 's' ? 11 : s[5] == 'e' ? 10 : 9);
            if (s[5] == 'o') c->value = parse_num(s + 9, &e) | 0x20000u;
            if (s[5] == 'g') c->value = 0x10000u;                    /* walk:goal: live upstairs quad, or a fixed field's placed marker */
            if (s[5] == 'e') c->value = 0x10001u;                    /* walk:entry: live downstairs quad */
            if (unit && *e == ':') c->value = parse_num(e + 1, &e);
            if (*e == ',') c->c = parse_float(e + 1, &e);
            if (*e || c->c <= 0) goto bad;
        } else if (!strncmp(s, "walk:", 5)) {
            c->kind = C_WALK;
            c->a = parse_float(s + 5, &e);
            if (*e != ',') goto bad;
            c->b = parse_float(e + 1, &e);
            c->c = 40.0f;
            if (*e == ',') c->c = parse_float(e + 1, &e);
            if (*e || c->c <= 0) goto bad;
        } else if (!strncmp(s, "face:", 5)) {
            c->kind = C_FACE;
            c->a = parse_float(s + 5, &e);
            if (*e) goto bad;
        } else if (!strncmp(s, "press:", 6)) {
            c->kind = C_PRESS;
            char *colon = strchr(s + 6, ':');
            size_t bl = colon ? (size_t)(colon - (s + 6)) : strlen(s + 6);
            c->button = -1;
            for (int i = 0; i < P4W_BUTTON_COUNT; i++)
                if (strlen(kButtons[i]) == bl && !strncmp(kButtons[i], s + 6, bl)) c->button = i;
            if (c->button < 0) goto bad;
            c->a = 3;
            if (colon) { c->a = parse_float(colon + 1, &e); if (*e) goto bad; }
            if (c->a < 3) c->a = 3;
        } else if (!strncmp(s, "wait:", 5)) {
            c->kind = C_WAIT;
            c->a = parse_float(s + 5, &e);
            if (*e) goto bad;
        } else if (!strncmp(s, "waitfield:", 10)) {
            c->kind = C_WAITFIELD;
            c->addr = parse_num(s + 10, &e);
            if (*e != ':') goto bad;
            c->value = parse_num(e + 1, &e);
            if (*e) goto bad;
        } else if (!strncmp(s, "waitstep:", 9)) {
            c->kind = C_WAITSTEP;
            c->value = parse_num(s + 9, &e);
            if (e == s + 9 || *e) goto bad;
        } else if (!strncmp(s, "skipfield:", 10)) {
            c->kind = C_SKIPFIELD;
            c->addr = parse_num(s + 10, &e);
            if (*e != ':') goto bad;
            c->value = parse_num(e + 1, &e);
            if (*e) goto bad;
        } else if (!strncmp(s, "skipevtmsg:", 11)) {
            c->kind = C_SKIPEVTMSG;
            c->addr = parse_num(s + 11, &e);
            if (e == s + 11 || *e != ':') goto bad;
            const char *row = e + 1;
            c->value = parse_num(row, &e);
            if (e == row || *e) goto bad;
        } else if (!strcmp(s, "waitchoice")) {
            c->kind = C_WAITCHOICE;
        } else if (!strncmp(s, "choose:", 7)) {
            c->kind = C_CHOOSE;
            c->value = parse_num(s + 7, &e);
            if (*e || c->value < 1 || c->value > 32) goto bad;
        } else if (!strncmp(s, "waitstate:", 10) || !strncmp(s, "waitstate16:", 12)) {
            c->kind = C_WAITSTATE;
            c->on = s[9] == '1';
            c->addr = parse_num(s + (c->on ? 12 : 10), &e);
            if (e[0] == '=' && e[1] == '=') c->ne = 0;
            else if (e[0] == '!' && e[1] == '=') c->ne = 1;
            else goto bad;
            c->value = parse_num(e + 2, &e);
            if (*e || (c->on && ((c->addr & 1u) || c->value > 0xFFFFu))) goto bad;
        } else if (!strncmp(s, "waitcontrol:", 12)) {
            c->kind = C_WAITCONTROL;
            if (!strcmp(s + 12, "on")) c->on = 1;
            else if (!strcmp(s + 12, "off")) c->on = 0;
            else goto bad;
        } else if (!strncmp(s, "waitmsg:", 8)) {
            c->kind = C_WAITMSG;
            if (!strcmp(s + 8, "closed")) c->on = 1;
            else if (!strcmp(s + 8, "open")) c->on = 0;
            else goto bad;
        } else if (!strncmp(s, "skip:", 5)) {
            c->kind = C_SKIPFOR;
            c->a = parse_float(s + 5, &e);
            if (*e || c->a <= 0) goto bad;
        } else if (!strncmp(s, "push:", 5)) {
            c->kind = C_PUSH;
            c->a = parse_float(s + 5, &e);
            if (*e != ',') goto bad;
            c->b = parse_float(e + 1, &e);
            if (*e != ',') goto bad;
            c->c = parse_float(e + 1, &e);                      /* frames */
            if (*e || c->c <= 0) goto bad;
        } else if (!strcmp(s, "log")) {
            c->kind = C_LOG;
        } else if (!strncmp(s, "plan:", 5)) {
            c->kind = C_PLAN;
            if (!strcmp(s + 5, "on")) c->on = 1;
            else if (!strcmp(s + 5, "off")) c->on = 0;
            else goto bad;
        } else if (!strcmp(s, "navprobe")) {
            c->kind = C_NAVPROBE;
        } else if (!strncmp(s, "navdump:", 8)) {
            c->kind = C_NAVDUMP;
            if (!s[8]) goto bad;
        } else if (!strncmp(s, "waittask:", 9) || !strncmp(s, "skiptask:", 9)) {
            c->kind = s[0] == 's' ? C_SKIPTASK : C_WAITTASK;
            c->on = 1;
            c->button = P4W_CROSS;
            const char *arg = strchr(s + 9, ':');
            size_t n = arg ? (size_t)(arg - (s + 9)) : strlen(s + 9);
            if (n == 0 || n > 0x17) goto bad;
            if (arg && !strncmp(arg, ":off", 4) && (!arg[4] || arg[4] == ':')) {
                c->on = 0;
                arg += 4;
            }
            if (arg && *arg) {
                if (c->kind != C_SKIPTASK || !arg[1]) goto bad;
                int button;
                for (button = 0; button < P4W_BUTTON_COUNT; button++)
                    if (!strcmp(arg + 1, kButtons[button])) break;
                if (button == P4W_BUTTON_COUNT) goto bad;
                c->button = button;
            }
        } else if (!strcmp(s, "waitbtl:menu")) {
            c->kind = C_WAITBTL;
        } else if (!strncmp(s, "waitbtl:menu:", 13)) {
            unsigned long character = strtoul(s + 13, &e, 10);
            if (*e || character < 1 || character > BTL_PARTY_MAX) goto bad;
            c->kind = C_WAITBTL;
            c->value = (uint32_t)character;
        } else if (!strncmp(s, "waitbtl:dead:", 13)) {
            unsigned long enemy = strtoul(s + 13, &e, 0);
            if (*e || enemy < 1 || enemy > 0xFFFFu) goto bad;
            c->kind = C_WAITBTL;
            c->on = 2;
            c->value = (uint32_t)enemy;
        } else if (!strncmp(s, "battle:", 7)) {
            BattleDriver probe;
            c->kind = C_BATTLE;
            if (!battle_parse(s + 7, &probe)) goto bad;
        } else if (!strncmp(s, "battlerule:", 11)) {
            P4WalkBattleRule probe;
            c->kind = C_BATTLERULE;
            if (!p4walk_battle_rule_parse(s + 11, &probe)) goto bad;
        } else if (!strncmp(s, "battleturn:", 11)) {
            P4WalkBattleRule probe;
            c->kind = C_BATTLETURN;
            if (!p4walk_battle_turn_parse(s + 11, &probe)) goto bad;
        } else if (!strncmp(s, "battleskip:", 11)) {
            c->kind = C_BATTLESKIP;
            if (!strcmp(s + 11, "on")) c->on = 1;
            else if (!strcmp(s + 11, "off")) c->on = 0;
            else goto bad;
        } else if (!strncmp(s, "autoskip:", 9)) {
            c->kind = C_AUTOSKIP;
            if (!strcmp(s + 9, "on")) c->on = 1;
            else if (!strcmp(s + 9, "off")) c->on = 0;
            else goto bad;
        } else if (!strcmp(s, "skipmsg")) {
            c->kind = C_SKIPMSG;
        } else if (!strncmp(s, "timeout:", 8)) {
            timeout = (uint32_t)strtoul(s + 8, &e, 10);
            if (*e || !timeout) goto bad;
            continue;
        } else if (!strcmp(s, "exit")) {
            c->kind = C_EXIT;
        } else {
            goto bad;
        }
        w->n++;
        continue;
    bad:
        snprintf(err, errsz, "line %d: bad command '%s'", lineno, s);
        goto fail;
    }
    return w;
fail:
    free(w);
    return NULL;
}

void p4walk_free(P4Walk *w) { free(w); }
int p4walk_status(const P4Walk *w) { return w->status; }

static float f32_of(uint32_t u) {
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static int name_is(const P4WalkHost *h, uint32_t node, const char *name) {
    char buf[0x19];
    for (int i = 0; i < 0x18; i += 4) {
        uint32_t v = h->read32(h->user, node + (uint32_t)i);
        memcpy(buf + i, &v, 4);
    }
    buf[0x18] = 0;
    return strcmp(buf, name) == 0;
}

static uint32_t task_node(const P4WalkHost *h, const char *name) {
    for (int l = 0; l < TASK_LISTS; l++) {
        uint32_t node = h->read32(h->user, kTaskLists[l]);
        for (int i = 0; node && i < 256; i++) {
            if (name_is(h, node, name)) return node;
            node = h->read32(h->user, node + TASK_NEXT);
        }
    }
    return 0;
}

static uint32_t task_work(const P4WalkHost *h, const char *name) {
    uint32_t node = task_node(h, name);
    return node ? h->read32(h->user, node + TASK_WORK) : 0;
}

/* itfMesWindowTaskUpdate (0027b500) walks this same list. Selection readiness and
 * visible cursor/count are the fields read by itfMesWindowCommitSelection /
 * itfMesWindowPollSelection (0027ac50 / 0027ae90), not the live-script count. */
static uint32_t choice_window(const P4WalkHost *h, unsigned *cursor, unsigned *count, int *opening) {
    uint32_t node = h->read32(h->user, 0x008817ECu);
    *opening = 0;
    for (int i = 0; node && i < 64; i++, node = h->read32(h->user, node + 4)) {
        uint32_t win = h->read32(h->user, node + 0xC);
        if (!win) continue;
        uint32_t flags = h->read32(h->user, win);
        uint32_t selection = h->read32(h->user, win + 0x48);
        unsigned state = selection & 0xFFFFu;
        if (state != 1 && state != 2) continue;
        *opening = 1;
        if (state != 2 || (flags & 0x38u) != 0x20u) continue;
        *cursor = selection >> 16;
        *count = h->read32(h->user, win + 0x4C) >> 16;
        return win;
    }
    return 0;
}

/* EVT/battle Help types in phase 3 and accepts page input in phase 4.
 * Field BF also has phase-2 text and phase-3/4 boxes with idle selection 0;
 * accept those only while a field event owns input, never as battle Help. */
static int text_window_active(const P4WalkHost *h, uint32_t win, int field_bf) {
    if (!win) return 0;
    uint32_t flags = h->read32(h->user, win), phase = flags & 7u;
    uint32_t mode = flags & 0x300u;
    if ((mode != 0x200u || (phase != 3 && phase != 4)) &&
        (!field_bf || mode != 0x100u || phase != 2)) return 0;
    if ((h->read32(h->user, win + 0x24) & 0xFFu) != 1) return 0;
    uint32_t selection = h->read32(h->user, win + 0x48) & 0xFFFFu;
    if (mode == 0x200u && selection == 0xFFFFu) return 1;
    return field_bf && selection == (mode == 0x100u ? 0xFFFFu : 0) &&
        h->read32(h->user, EVENT_FLAG) != 0 && !task_node(h, "battle");
}

static int text_window_open(const P4WalkHost *h, int field_bf) {
    uint32_t node = h->read32(h->user, 0x008817ECu);
    for (int i = 0; node && i < 64; i++, node = h->read32(h->user, node + 4)) {
        uint32_t win = h->read32(h->user, node + 0xC);
        if (text_window_active(h, win, field_bf)) return 1;
    }
    return 0;
}

/* evtEvent's active message handle selects its own window, not another live
 * caption. These are the matched EVT fields observed by event_scene_state.py. */
static uint32_t evt_text_window(const P4WalkHost *h, uint32_t *message, uint32_t *row) {
    uint32_t driver = task_work(h, "evtEvent");
    if (!driver || h->read32(h->user, driver) != 4) return 0;
    uint32_t child = h->read32(h->user, driver + 0xC);
    uint32_t work = child ? h->read32(h->user, child + TASK_WORK) : 0;
    if (!work || h->read32(h->user, work + 0x678) != 1 ||
        h->read32(h->user, work + 0x684) != 2) return 0;
    uint32_t handle = h->read32(h->user, work + 0x680);
    uint32_t node = h->read32(h->user, 0x008817ECu);
    for (int i = 0; node && i < 64; i++, node = h->read32(h->user, node + 4)) {
        if (h->read32(h->user, node + 8) != handle) continue;
        uint32_t win = h->read32(h->user, node + 0xC);
        if (!text_window_active(h, win, 0)) return 0;
        *message = h->read32(h->user, work + 0x688);
        *row = h->read32(h->user, win + 0x2C) & 0xFFFFu;
        return win;
    }
    return 0;
}

uint32_t p4walk_player_matrix(const P4WalkHost *h) {
    uint32_t work = task_work(h, "rotate pc");
    if (!work) return 0;
    uint32_t m = h->read32(h->user, work + 0x48);
#ifdef P4_GUEST64
    return m < 0x00100000u || m >= P4_GUEST_SIZE ? 0 : m;       /* the game's heap reaches the port half of the block on a 64-bit host */
#else
    return m < 0x00100000u || m >= 0x02000000u ? 0 : m;
#endif
}

int p4walk_player(const P4WalkHost *h, float *x, float *z, float *fx, float *fz) {
    uint32_t m = p4walk_player_matrix(h);
    if (!m) return 0;
    *x = f32_of(h->read32(h->user, m + 0x30));
    *z = f32_of(h->read32(h->user, m + 0x38));
    float ax = f32_of(h->read32(h->user, m + 0x20)), az = f32_of(h->read32(h->user, m + 0x28));
    float l = sqrtf(ax * ax + az * az);
    if (l > 1e-4f) { ax /= l; az /= l; }
    *fx = ax;
    *fz = az;
    return 1;
}

/* ---- Cross-target dungeon digest (include/p4pc/walk_script.h) -------------------------------------------------------------------------------------------
 * Guest data only: link and matrix words are followed, never hashed, because a 64-bit host's heap blocks need not sit at the same guest addresses on
 * every target. A matrix is its rows, position and flags, not RwMatrix's three pad words: the game leaves those uninitialised (btlUnitUpdateAll,
 * 0019c0d0, builds each unit's matrix on the stack and sets only rows, position and flags, as retail does, then copies all 64 bytes), so they hold
 * each host's stack garbage, and the VU0 routines carry such w lanes on. The grid is the dungeon floor's generated layout: the placed cells of
 * read_grid below (outside a generated floor that area holds other data, which is left out). The additive generator is the one LOG reports (its slot
 * in the engine block at 0x008872E0: base, front, back; 31 state words), RpRandom's Mersenne Twister is the array the walk digest hashes. */
static uint32_t fnv_word(uint32_t hash, uint32_t v) {
    for (int b = 0; b < 4; b++) hash = (hash ^ ((v >> (8 * b)) & 0xFFu)) * 16777619u;
    return hash;
}

static uint32_t fnv_words(const P4WalkHost *h, uint32_t hash, uint32_t addr, uint32_t words) {
    for (uint32_t k = 0; addr && k < words; k++) hash = fnv_word(hash, h->read32(h->user, addr + 4u * k));
    return hash;
}

static uint32_t fnv_matrix(const P4WalkHost *h, uint32_t hash, uint32_t m) {
    for (uint32_t k = 0; m && k < 16; k++)
        if (k != 7 && k != 11 && k != 15) hash = fnv_word(hash, h->read32(h->user, m + 4u * k));
    return hash;
}

static uint32_t digest_rng(const P4WalkHost *h, uint32_t hash) {
    uint32_t blk = 0x008872E0u + h->read32(h->user, RNG_SLOT_INDEX), base = h->read32(h->user, blk);
    if (!base) return fnv_word(hash, 0);
    hash = fnv_word(hash, h->read32(h->user, blk + 4) - base);
    hash = fnv_word(hash, h->read32(h->user, blk + 8) - base);
    return fnv_words(h, hash, base, 31);
}

int p4walk_dungeon_digest(const P4WalkHost *h, uint32_t frame, uint32_t every, uint32_t *roll, char *line, size_t n) {
    uint32_t player = p4walk_player_matrix(h);
    *roll = digest_rng(h, fnv_matrix(h, *roll, player));
    if (!every || frame % every != 0) return 0;
    const uint32_t seed0 = 2166136261u;
    uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE), work = root ? h->read32(h->user, root + TASK_WORK) : 0;
    uint32_t ids = work ? h->read32(h->user, work + 0x18) : 0xFFFFFFFFu;
    uint32_t step_task = h->read32(h->user, 0x00764334u), step_work = step_task ? h->read32(h->user, step_task + TASK_WORK) : 0;
    uint32_t grid = seed0;
    for (uint32_t r = 0; r < 24; r++)
        for (uint32_t c = 0; c < 16; c++) {
            uint32_t a = FIELD_ROOT_HANDLE + 0x54u + r * 0x100u + c * 0x10u, w0 = h->read32(h->user, a);
            if ((w0 & 0xFFu) != 1u || (w0 >> 28) != 3u) continue;          /* read_grid's placed piece */
            grid = fnv_word(fnv_word(grid, r << 8 | c), w0);
            grid = fnv_words(h, grid, a + 4u, 2);
        }
    uint32_t units = seed0, nunits = 0;
    uint32_t obj = h->read32(h->user, SCENE_OBJECT), mgr = obj ? h->read32(h->user, obj + 8) : 0;
    for (uint32_t p = mgr ? h->read32(h->user, mgr + 4u) : 0; p && nunits < 64; p = h->read32(h->user, p + 0x138), nunits++) {
        units = fnv_word(units, h->read32(h->user, p) & 0xFFFFu);
        units = fnv_matrix(h, units, h->read32(h->user, p + 0x164));
    }
    uint32_t ctx = task_node(h, "battle") ? h->read32(h->user, BATTLE_CONTEXT) : 0;
    snprintf(line, n, "dungeon digest flip %u field %u:%u step %d seed %08x grid %08x rng %08x mt %08x player %08x units %u:%08x battle %d roll %08x",
             frame, ids & 0xFFFFu, ids >> 16, step_work ? (int32_t)h->read32(h->user, step_work + 4) : -1, h->read32(h->user, 0x00764330u), grid,
             digest_rng(h, seed0), fnv_words(h, seed0, 0x00885A90u, 0x270u), fnv_matrix(h, seed0, player), nunits, units,
             ctx ? (int32_t)h->read32(h->user, ctx + 0x2FC) : -1, *roll);
    return 1;
}

/* printf("%.0f") rounds with the FPU's current mode, and the port runs in the EE's round-toward-zero mode (a retail 392.886 prints 393, the port's same value 392):
 * round half up by hand. */
static long round_half_up(float v) { return (long)floorf(v + 0.5f); }
static uint32_t bits_of(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

/* The field's interaction data, one note line each (see p4walk_checks_frame). Lists are the scene resource manager's, entries as the
 * FldEvent_* trigger tests read them: list 0xD = area quads (u16 id at +0, low 10 bits = index into the frame state's 0x2C-byte event table
 * at 0x007D2510+0x18DC, 0x3FF/0x3FE = the field script's procedure 0/1; quad corners at +0x15C/+0x168/+0x174/+0x180), list 0xA = field objects
 * (flags at +0x28, position at +4), list 0x1 = units (model matrix at +0x164). */
/* the floor's cell grid: exits, type and piece id per occupied cell (one note line per row that has any) */
static void note_grid(const P4WalkHost *h) {
    for (int r = 0; r < 24; r++) {
        char row[16 * 8 + 32];
        int k = snprintf(row, sizeof row, "CHECK grid row %2d:", r);
        int occ = 0;
        for (int c = 0; c < 16; c++) {
            uint32_t a = FIELD_ROOT_HANDLE + 0x54u + (uint32_t)r * 0x100u + (uint32_t)c * 0x10u;
            uint32_t w0 = h->read32(h->user, a), w1 = h->read32(h->user, a + 4), w2 = h->read32(h->user, a + 8);
            if ((w0 & 0xFF) == 1) { k += snprintf(row + k, sizeof row - (size_t)k, " %X%X%04x", (w2 >> 16) & 0xF, w1 & 0xF, w0 >> 16); occ++; }
            else k += snprintf(row + k, sizeof row - (size_t)k, " ......");
        }
        if (occ) h->note(h->user, row);
    }
}

static void dump_checks(const P4WalkHost *h, uint32_t field, uint32_t room) {
    char m[300];
    uint32_t obj = h->read32(h->user, SCENE_OBJECT);
    uint32_t mgr = obj ? h->read32(h->user, obj + 8) : 0;
    uint32_t count = h->read32(h->user, FIELD_ROOT_HANDLE + 0x18D8), tab = h->read32(h->user, FIELD_ROOT_HANDLE + 0x18DC);
    snprintf(m, sizeof m, "CHECKS field %u room %u: %u event table entries at %08x", field, room, count, tab);
    h->note(h->user, m);
    if (!mgr) return;
    uint32_t p = h->read32(h->user, mgr + 4u * 0xDu);
    for (int i = 0; p && i < 400; i++, p = h->read32(h->user, p + 0x138)) {
        uint32_t id = h->read32(h->user, p) & 0xFFFFu, idx = id & 0x3FFu;
        float xs[4], zs[4], lx = 1e30f, hx = -1e30f, lz = 1e30f, hz = -1e30f;
        for (int k = 0; k < 4; k++) {
            xs[k] = f32_of(h->read32(h->user, p + 0x15C + 12u * (uint32_t)k));
            zs[k] = f32_of(h->read32(h->user, p + 0x15C + 12u * (uint32_t)k + 8));
            if (xs[k] < lx) lx = xs[k];
            if (xs[k] > hx) hx = xs[k];
            if (zs[k] < lz) lz = zs[k];
            if (zs[k] > hz) hz = zs[k];
        }
        int n = snprintf(m, sizeof m, "CHECK area %08x id %04x x %ld..%ld z %ld..%ld bits %08x %08x %08x %08x", p, id, round_half_up(lx), round_half_up(hx),
                         round_half_up(lz), round_half_up(hz), bits_of(xs[0]), bits_of(zs[0]), bits_of(xs[3]), bits_of(zs[3]));
        if (idx == 0x3FF || idx == 0x3FE) {
            snprintf(m + n, sizeof m - (size_t)n, " field-script procedure %d", idx == 0x3FF ? 0 : 1);
        } else if (idx < count && tab) {
            uint32_t e = tab + idx * 0x2C, k = h->read32(h->user, e + 0xC), s = h->read32(h->user, e + 0x10);
            snprintf(m + n, sizeof m - (size_t)n, " event %u kind %02x type %02x script %u flags %x %x %x", idx, k & 0xFF, (k >> 8) & 0xFF, s,
                     h->read32(h->user, e), h->read32(h->user, e + 4), h->read32(h->user, e + 8));
        }
        h->note(h->user, m);
    }
    p = h->read32(h->user, mgr + 4u * 0xAu);
    for (int i = 0; p && i < 200; i++, p = h->read32(h->user, p + 0x138)) {
        uint32_t mh = h->read32(h->user, p + 0x144);
        snprintf(m, sizeof m, "CHECK object %08x id %04x flags %x pos %ld %ld %ld model pos %ld %ld", p, h->read32(h->user, p) & 0xFFFFu,
                 h->read32(h->user, p + 0x28), round_half_up(f32_of(h->read32(h->user, p + 4))), round_half_up(f32_of(h->read32(h->user, p + 8))),
                 round_half_up(f32_of(h->read32(h->user, p + 12))), mh ? round_half_up(f32_of(h->read32(h->user, mh + 0x30))) : 0L,
                 mh ? round_half_up(f32_of(h->read32(h->user, mh + 0x38))) : 0L);
        h->note(h->user, m);
    }
    p = h->read32(h->user, mgr + 4u);                                    /* list 0x1: units; 0x400..0x403 the party, 0x460.. shadows and treasure */
    for (int i = 0; p && i < 64; i++, p = h->read32(h->user, p + 0x138)) {
        uint32_t mh = h->read32(h->user, p + 0x164);
        snprintf(m, sizeof m, "CHECK unit %08x id %04x flags %x pos %ld %ld", p, h->read32(h->user, p) & 0xFFFFu, h->read32(h->user, p + 0x28),
                 mh ? round_half_up(f32_of(h->read32(h->user, mh + 0x30))) : 0L, mh ? round_half_up(f32_of(h->read32(h->user, mh + 0x38))) : 0L);
        h->note(h->user, m);
    }
    static const struct { uint32_t type; const char *name; } kLists[] = {
        { 0xC, "list 0xC (scene objects: chests, props)" }, { 0xF, "list 0xF (encounter spawn points)" },
        { 0x10, "list 0x10 (encounter slots)" }, { 0x11, "list 0x11 (slot models)" }, { 0xE, "list 0xE (floor goal)" },
    };
    for (size_t l = 0; l < sizeof kLists / sizeof kLists[0]; l++) {
        p = h->read32(h->user, mgr + 4u * kLists[l].type);
        for (int i = 0; p && i < 128; i++, p = h->read32(h->user, p + 0x138)) {
            snprintf(m, sizeof m, "CHECK %s %08x id %04x flags %x pos %ld %ld %ld", kLists[l].name, p, h->read32(h->user, p) & 0xFFFFu,
                     h->read32(h->user, p + 0x28), round_half_up(f32_of(h->read32(h->user, p + 0x140))), round_half_up(f32_of(h->read32(h->user, p + 0x144))),
                     round_half_up(f32_of(h->read32(h->user, p + 0x148))));
            h->note(h->user, m);
        }
    }
    note_grid(h);
}

/* One call per frame (p4hook --probe-checks, port P4PC_DUMP_CHECKS=1): dumps the interaction data 60 and 900 frames after the field or room id changed (dungeon floors fill their lists late). */
void p4walk_checks_frame(const P4WalkHost *h, uint32_t frame) {
    static uint32_t last_ids = 0xFFFFFFFEu, since;
    uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE);
    uint32_t work = root ? h->read32(h->user, root + TASK_WORK) : 0;
    uint32_t ids = work ? h->read32(h->user, work + 0x18) : 0xFFFFFFFFu;
    if (ids != last_ids) { last_ids = ids; since = frame; return; }
    if (ids != 0xFFFFFFFFu && (frame - since == 60 || frame - since == 900)) dump_checks(h, ids & 0xFFFF, ids >> 16);
}

/* The nearest field unit of the scene resource manager's list 0x1 whose id is `id`, or any id of 0x460 and up when id is 0 (the party is 0x400..0x403;
 * the others are the floor's shadows and treasure): position from the model matrix at +0x164. */
static int nearest_unit(const P4WalkHost *h, float px, float pz, uint32_t id, float *tx, float *tz) {
    uint32_t obj = h->read32(h->user, SCENE_OBJECT);
    uint32_t mgr = obj ? h->read32(h->user, obj + 8) : 0;
    uint32_t p;
    float best = 1e30f;
    int found = 0;
    if (id & 0x20000u) {                                                 /* walk:obj:ID: a list 0xA object, the position of its model (+0x144) */
        for (p = mgr ? h->read32(h->user, mgr + 4u * 0xA) : 0; p; p = h->read32(h->user, p + 0x138)) {
            if ((h->read32(h->user, p) & 0xFFFFu) != (id & 0xFFFFu)) continue;
            uint32_t mh = h->read32(h->user, p + 0x144);
            if (!mh) return 0;
            *tx = f32_of(h->read32(h->user, mh + 0x30));
            *tz = f32_of(h->read32(h->user, mh + 0x38));
            return 1;
        }
        return 0;
    }
    if (id == 0x10000u || id == 0x10001u) {
        /* Resolve the requested live stair quad, independent of board seed. */
        uint32_t quad_id = id == 0x10001u ? 0x37FEu : 0x37FFu;
        p = mgr ? h->read32(h->user, mgr + 4u * 0xD) : 0;
        for (int i = 0; p && i < 400; i++, p = h->read32(h->user, p + 0x138)) {
            if ((h->read32(h->user, p) & 0xFFFFu) != quad_id) continue;
            float x = 0.0f, z = 0.0f;
            for (uint32_t k = 0; k < 4; k++) {
                x += f32_of(h->read32(h->user, p + 0x15C + 12u * k));
                z += f32_of(h->read32(h->user, p + 0x164 + 12u * k));
            }
            *tx = x * 0.25f;
            *tz = z * 0.25f;
            return 1;
        }
        if (id == 0x10001u) return 0;                                /* no downstairs marker fallback */
        /* Fixed fields without an advance quad retain their placed marker. */
        p = mgr ? h->read32(h->user, mgr + 4u * 0xE) : 0;
        if (!p) return 0;
        *tx = f32_of(h->read32(h->user, p + 0x140));
        *tz = f32_of(h->read32(h->user, p + 0x148));
        return 1;
    }
    p = mgr ? h->read32(h->user, mgr + 4u) : 0;
    for (int i = 0; p && i < 64; i++, p = h->read32(h->user, p + 0x138)) {
        uint32_t uid = h->read32(h->user, p) & 0xFFFFu;
        if (id ? uid != id : uid < 0x460u) continue;
        uint32_t m = h->read32(h->user, p + 0x164);
        if (!m) continue;
        float sx = f32_of(h->read32(h->user, m + 0x30)), sz = f32_of(h->read32(h->user, m + 0x38));
        float d = (sx - px) * (sx - px) + (sz - pz) * (sz - pz);
        if (d < best) { best = d; *tx = sx; *tz = sz; found = 1; }
    }
    return found;
}

/* The nearest heart-arch gate or curtain of the floor (scene list 0xA, position from the model matrix at +0x144) within `range` of the player. */
static int nearest_gate(const P4WalkHost *h, float px, float pz, float range, float *gx, float *gz) {
    uint32_t obj = h->read32(h->user, SCENE_OBJECT);
    uint32_t mgr = obj ? h->read32(h->user, obj + 8) : 0;
    uint32_t p = mgr ? h->read32(h->user, mgr + 4u * 0xAu) : 0;
    float best = range * range;
    int found = 0;
    for (int i = 0; p && i < 64; i++, p = h->read32(h->user, p + 0x138)) {
        uint32_t mh = h->read32(h->user, p + 0x144);
        if (!mh) continue;
        float sx = f32_of(h->read32(h->user, mh + 0x30)), sz = f32_of(h->read32(h->user, mh + 0x38));
        float d = (sx - px) * (sx - px) + (sz - pz) * (sz - pz);
        if (d < best && (sx != 0.0f || sz != 0.0f)) { best = d; *gx = sx; *gz = sz; found = 1; }
    }
    return found;
}

/* Dungeon floors are a grid of 1200-unit cells (24 rows x 16 columns of 0x10-byte entries at frame state + 0x54; cell (c, r) covers
 * x in [1200c-600, 1200c+600), z likewise). Entry: +0 occupied, +2 u16 piece (resource) id, +4 type, +0xA exit mask (bit0 north = -z, bit1 west,
 * bit2 south, bit3 east). Cells of one piece (a room) are open to each other; pieces join where both edge bits are set. */
#define GRID_COLS 16
#define GRID_ROWS 24
typedef struct { uint8_t occ, type, exits, shape; uint16_t piece; } GridCell;

static int read_grid(const P4WalkHost *h, GridCell g[GRID_ROWS][GRID_COLS]) {
    int n = 0;
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++) {
            uint32_t a = FIELD_ROOT_HANDLE + 0x54u + (uint32_t)r * 0x100u + (uint32_t)c * 0x10u;
            uint32_t w0 = h->read32(h->user, a), w1 = h->read32(h->user, a + 4), w2 = h->read32(h->user, a + 8);
            g[r][c].occ = (uint8_t)(w0 & 0xFF) == 1 && (w0 >> 28) == 3;   /* a placed piece: its resource id is 0x3xxx (other fields leave other data there) */
            g[r][c].piece = (uint16_t)(w0 >> 16);
            g[r][c].type = (uint8_t)(w1 & 0xFF);
            g[r][c].shape = (uint8_t)((w1 >> 16) & 0xFF);          /* the piece's size in cells: 1 corridor, 2 bend or small room, 3 big room */
            g[r][c].exits = (uint8_t)((w2 >> 16) & 0xF);
            n += g[r][c].occ;
        }
    return n;
}

static int g_nblocked, g_blocked[16][3], g_permissive;                                 /* set by the caller for the duration of a plan_step */

static int cells_joined(GridCell g[GRID_ROWS][GRID_COLS], int c, int r, int d) {
    static const int ddc[4] = { 0, -1, 0, 1 }, ddr[4] = { -1, 0, 1, 0 };
    for (int i = 0; i < g_nblocked; i++)
        if ((g_blocked[i][0] == c && g_blocked[i][1] == r && g_blocked[i][2] == d) ||
            (g_blocked[i][0] == c + ddc[d] && g_blocked[i][1] == r + ddr[d] && g_blocked[i][2] == ((d + 2) & 3))) return 0;
    static const int dc[4] = { 0, -1, 0, 1 }, dr[4] = { -1, 0, 1, 0 };   /* north, west, south, east */
    int c2 = c + dc[d], r2 = r + dr[d];
    if (c2 < 0 || c2 >= GRID_COLS || r2 < 0 || r2 >= GRID_ROWS || !g[r2][c2].occ) return 0;
    if (g[r][c].piece == g[r2][c2].piece && g[r][c].type == g[r2][c2].type && 1) return 1;   /* a big room is open inside; smaller pieces have walls: their exit masks say */
    return (g[r][c].exits >> d & 1) && (g[r2][c2].exits >> ((d + 2) & 3) & 1);
}

/* Next point to steer at on the way to (tx, tz) and a progress metric that falls as the walk advances (cells left * 1200 + distance to the next point).
 * 0 when there is no grid or no route (the caller steers straight). */
static int g_detoured;   /* set by plan_step when its aim point is a detour around a room's middle */

static int plan_step(const P4WalkHost *h, float px, float pz, float tx, float tz, float *wx, float *wz, float *metric, int *edge, int detour) {
    GridCell g[GRID_ROWS][GRID_COLS];
    g_detoured = 0;
    if (read_grid(h, g) == 0) return 0;
    int pc = (int)floorf((px + 600.0f) / 1200.0f), pr = (int)floorf((pz + 600.0f) / 1200.0f);
    int tc = (int)floorf((tx + 600.0f) / 1200.0f), tr = (int)floorf((tz + 600.0f) / 1200.0f);
    if (pc < 0 || pc >= GRID_COLS || pr < 0 || pr >= GRID_ROWS || tc < 0 || tc >= GRID_COLS || tr < 0 || tr >= GRID_ROWS) return 0;
    if (!g[pr][pc].occ || !g[tr][tc].occ) return 0;
    if (pc == tc && pr == tr) { *wx = tx; *wz = tz; *metric = hypotf(tx - px, tz - pz); return 1; }
    int dist[GRID_ROWS][GRID_COLS], from[GRID_ROWS][GRID_COLS];
    static const int dc[4] = { 0, -1, 0, 1 }, dr[4] = { -1, 0, 1, 0 };
    for (g_permissive = 0; g_permissive < 2; g_permissive++) {
        for (int r = 0; r < GRID_ROWS; r++) for (int c = 0; c < GRID_COLS; c++) dist[r][c] = -1;
        int qc[GRID_ROWS * GRID_COLS], qr[GRID_ROWS * GRID_COLS], qh = 0, qt = 0;
        dist[tr][tc] = 0;                                           /* search from the target so every cell knows its way down */
        qc[qt] = tc; qr[qt++] = tr;
        while (qh < qt) {
            int c = qc[qh], r = qr[qh++];
            for (int d = 0; d < 4; d++) {
                int c2 = c + dc[d], r2 = r + dr[d];
                if (c2 < 0 || c2 >= GRID_COLS || r2 < 0 || r2 >= GRID_ROWS || dist[r2][c2] >= 0) continue;
                if (!cells_joined(g, c, r, d)) continue;
                dist[r2][c2] = dist[r][c] + 1;
                from[r2][c2] = (d + 2) & 3;                          /* from (c2, r2) step in this direction to get nearer */
                qc[qt] = c2; qr[qt++] = r2;
            }
        }

        if (dist[pr][pc] >= 0) break;
    }
    g_permissive = 0;
    if (dist[pr][pc] < 0) return 0;
    int d = from[pr][pc], c2 = pc + dc[d], r2 = pr + dr[d];
    edge[0] = pc;
    edge[1] = pr;
    edge[2] = d;
    float cx = 1200.0f * (float)c2, cz = 1200.0f * (float)r2;
    /* in a one-cell corridor first get onto the centre line, so a turn does not clip the wall */
    if ((d & 1) == 0 && fabsf(px - 1200.0f * (float)pc) > 250.0f && g[pr][pc].piece != g[r2][c2].piece) { cx = 1200.0f * (float)pc; cz = pz; }
    else if ((d & 1) == 1 && fabsf(pz - 1200.0f * (float)pr) > 250.0f && g[pr][pc].piece != g[r2][c2].piece) { cz = 1200.0f * (float)pr; cx = px; }
    float metric0 = (float)dist[pr][pc] * 1200.0f + hypotf(cx - px, cz - pz);              /* progress is measured to the plain point, not the detour */
    /* a room (a piece of 2x2 or 3x3 cells) has a stage or a pillar in the middle: when the straight way to the next point passes it, aim at a point beside it */
    {
        int c0 = pc, c1 = pc, r0 = pr, r1 = pr;
        for (int r = 0; r < GRID_ROWS; r++)
            for (int c = 0; c < GRID_COLS; c++)
                if (g[r][c].occ && g[r][c].piece == g[pr][pc].piece && g[r][c].type == g[pr][pc].type && abs(c - pc) < 4 && abs(r - pr) < 4) {
                    if (c < c0) c0 = c;
                    if (c > c1) c1 = c;
                    if (r < r0) r0 = r;
                    if (r > r1) r1 = r;
                }
        if (detour && (c1 > c0 || r1 > r0)) {
            float mx = 600.0f * (float)(c0 + c1), mz = 600.0f * (float)(r0 + r1);       /* centre of the room */
            float ax = cx - px, az = cz - pz, al = hypotf(ax, az), toc_x = mx - px, toc_z = mz - pz, tl = hypotf(toc_x, toc_z);
            if (al > 1.0f && tl > 450.0f) {
                float t = (toc_x * ax + toc_z * az) / (al * al);                             /* where along the way the centre is closest */
                float qx = px + ax * (t < 0 ? 0 : t > 1 ? 1 : t) - mx, qz = pz + az * (t < 0 ? 0 : t > 1 ? 1 : t) - mz;
                if (t > 0 && hypotf(qx, qz) < 650.0f) {
                    float nx = -az / al, nz = ax / al, side = (px - mx) * nx + (pz - mz) * nz < 0 ? -1.0f : 1.0f;
                    {
                        float off = c1 - c0 >= 2 || r1 - r0 >= 2 ? 1300.0f : 1000.0f;   /* beside the stage, inside the walls */
                        cx = mx + nx * side * off;
                        cz = mz + nz * side * off;
                        g_detoured = 1;
                    }
                }
            }
        }
    }
    *wx = cx;
    *wz = cz;
    *metric = metric0;
    return 1;
}

/* face:DEG's direction (sin, cos) from basic IEEE operations only: the hosts' sinf/cosf differ in the last place between glibc and the Windows CRT for
 * about one input in eighty (whole degrees -360..360: 16 in the EE mode, face:11 and face:101 among them, 14 others in round-to-nearest), and a script
 * must turn the player the same way on every target. fmod is exact; the quadrant's remainder is at most 45 degrees, where the series to x^11 is far
 * below a float ulp. */
static void face_dir(float deg, float *s, float *c) {
    double d = fmod((double)deg, 360.0);
    if (d < 0.0) d += 360.0;
    int q = (int)((d + 45.0) / 90.0);
    double x = (d - 90.0 * (double)q) * (3.14159265358979323846 / 180.0), x2 = x * x;
    double sn = x * (1.0 - x2 / 6.0 * (1.0 - x2 / 20.0 * (1.0 - x2 / 42.0 * (1.0 - x2 / 72.0 * (1.0 - x2 / 110.0)))));
    double cs = 1.0 - x2 / 2.0 * (1.0 - x2 / 12.0 * (1.0 - x2 / 30.0 * (1.0 - x2 / 56.0 * (1.0 - x2 / 90.0))));
    switch (q & 3) {
    case 0: *s = (float)sn; *c = (float)cs; break;
    case 1: *s = (float)cs; *c = (float)-sn; break;
    case 2: *s = (float)-sn; *c = (float)-cs; break;
    default: *s = (float)-cs; *c = (float)sn; break;
    }
}

/* The d-pad directions that move the player along the world vector (dx, dz) as seen by the camera. */
static uint32_t steer(const P4WalkHost *h, float dx, float dz) {
    uint32_t cam = h->read32(h->user, CAMERA_POINTER);
    uint32_t frame = cam ? h->read32(h->user, cam + 4) : 0;
    if (!frame) return 0;
    float fx = f32_of(h->read32(h->user, frame + 0x30)), fz = f32_of(h->read32(h->user, frame + 0x38));
    float rx = -f32_of(h->read32(h->user, frame + 0x10)), rz = -f32_of(h->read32(h->user, frame + 0x18));
    float lf = sqrtf(fx * fx + fz * fz), lr = sqrtf(rx * rx + rz * rz), ld = sqrtf(dx * dx + dz * dz);
    if (lf < 1e-4f || lr < 1e-4f || ld < 1e-4f) return 0;
    float sy = (dx * fx + dz * fz) / (ld * lf), sx = (dx * rx + dz * rz) / (ld * lr);
    uint32_t m = 0;
    if (fabsf(sy) > 0.414f * fabsf(sx)) m |= 1u << (sy > 0 ? P4W_UP : P4W_DOWN);
    if (fabsf(sx) > 0.414f * fabsf(sy)) m |= 1u << (sx > 0 ? P4W_RIGHT : P4W_LEFT);
    return m;
}

/* ---- Navigation over the field's own collision world (the port: P4WalkHost.nav_*) --------------------------------------------------------------
 * The floor is sampled lazily on a 100-unit grid: a node is walkable when the game's ground ray finds a floor near the player's height and the
 * player's sphere (radius NAV_R) at that spot touches no collision. A* over the nodes (8 neighbours, no corner cutting, a surcharge next to walls),
 * the path pulled straight (line of sight over the nodes) and the walker steers at the farthest visible point of it. */
#define NAV_STEP 100.0f
#define NAV_NX 192
#define NAV_NZ 288
#define NAV_X0 (-600.0f)
#define NAV_Z0 (-600.0f)
#define NAV_N (NAV_NX * NAV_NZ)
#define NAV_R 62.0f
#define NAV_R_WIDE 150.0f
#define NAV_HI 400.0f
#define NAV_LO 600.0f
#define NAV_DY 200.0f
#define NAV_HEAP (NAV_N * 4)
enum { NAV_UNKNOWN = 0, NAV_FREE = 1, NAV_WALL = 2, NAV_VOID = 3, NAV_GATE = 4 };   /* GATE: a wall near a closed gate or curtain, which Cross opens */

typedef struct { float f; int32_t node; } NavEntry;

typedef struct NavMap {
    uint8_t st[NAV_N];
    uint8_t tight[NAV_N];                /* 0 unknown, 1 clear, 2 a wider sphere touches the walls */
    uint8_t forced[NAV_N];               /* nodes the walk found shut (a closed gate, an invisible wall) */
    float gy[NAV_N];
    float g[NAV_N];
    int32_t from[NAV_N];
    uint8_t closed[NAV_N];
    NavEntry heap[NAV_HEAP];
    float ref_y;
    float lift;
    int gate_n;                          /* the closed gates (collidable field objects, list 0xA) and their positions */
    float gate_x[16], gate_z[16];
    uint32_t gate_sig;
    int navoid;                          /* positions to keep away from (the other Shadows: touching one starts a fight) */
    float avoid_x[8], avoid_z[8];
    int path_len;                        /* nodes of the last path, start first */
    int32_t path[NAV_N];
} NavMap;

static int nav_idx(int ix, int iz) { return iz * NAV_NX + ix; }
static float nav_x(int ix) { return NAV_X0 + ((float)ix + 0.5f) * NAV_STEP; }
static float nav_z(int iz) { return NAV_Z0 + ((float)iz + 0.5f) * NAV_STEP; }
static int nav_ix(float x) { return (int)floorf((x - NAV_X0) / NAV_STEP); }
static int nav_iz(float z) { return (int)floorf((z - NAV_Z0) / NAV_STEP); }

static int nav_state(const P4WalkHost *h, NavMap *m, int ix, int iz) {
    if (ix < 0 || iz < 0 || ix >= NAV_NX || iz >= NAV_NZ) return NAV_WALL;
    int i = nav_idx(ix, iz);
    if (m->st[i] == NAV_UNKNOWN) {
        float x = nav_x(ix), z = nav_z(iz), gy;
        if (!h->nav_ground(h->user, x, z, m->ref_y + NAV_HI, m->ref_y - NAV_LO, &gy) || fabsf(gy - m->ref_y) > NAV_DY) m->st[i] = NAV_VOID;
        else {
            m->gy[i] = gy;
            m->st[i] = h->nav_blocked(h->user, x, gy + m->lift, z, NAV_R) ? NAV_WALL : NAV_FREE;
            for (int g = 0; g < m->gate_n && m->st[i] == NAV_WALL; g++) {
                float ex = x - m->gate_x[g], ez = z - m->gate_z[g];
                if (ex * ex + ez * ez < 380.0f * 380.0f) m->st[i] = NAV_GATE;
            }
        }
    }
    return m->forced[i] ? NAV_WALL : m->st[i];
}

static int nav_tight(const P4WalkHost *h, NavMap *m, int ix, int iz) {
    int i = nav_idx(ix, iz);
    if (!m->tight[i]) m->tight[i] = h->nav_blocked(h->user, nav_x(ix), m->gy[i] + m->lift, nav_z(iz), NAV_R_WIDE) ? 2 : 1;
    return m->tight[i] == 2;
}

static void heap_push(NavMap *m, int *n, float f, int node) {
    if (*n >= NAV_HEAP) return;
    int i = (*n)++;
    m->heap[i].f = f;
    m->heap[i].node = node;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (m->heap[p].f <= m->heap[i].f) break;
        NavEntry t = m->heap[p]; m->heap[p] = m->heap[i]; m->heap[i] = t;
        i = p;
    }
}

static NavEntry heap_pop(NavMap *m, int *n) {
    NavEntry top = m->heap[0];
    m->heap[0] = m->heap[--(*n)];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *n && m->heap[l].f < m->heap[s].f) s = l;
        if (r < *n && m->heap[r].f < m->heap[s].f) s = r;
        if (s == i) break;
        NavEntry t = m->heap[s]; m->heap[s] = m->heap[i]; m->heap[i] = t;
        i = s;
    }
    return top;
}

/* The walkable node nearest to (x, z) within `maxr` nodes (the node itself when it is walkable); -1 when none. */
static int nav_nearest_free(const P4WalkHost *h, NavMap *m, float x, float z, int maxr) {
    int cx = nav_ix(x), cz = nav_iz(z);
    float best = 1e30f;
    int bi = -1;
    for (int r = 0; r <= maxr; r++) {
        for (int dz = -r; dz <= r; dz++)
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int ix = cx + dx, iz = cz + dz;
                if (nav_state(h, m, ix, iz) != NAV_FREE) continue;
                float ex = nav_x(ix) - x, ez = nav_z(iz) - z, d = ex * ex + ez * ez;
                if (d < best) { best = d; bi = nav_idx(ix, iz); }
            }
        if (bi >= 0 && (float)r * NAV_STEP > sqrtf(best) + NAV_STEP) break;
    }
    return bi;
}

/* A* from (sx, sz) to (tx, tz); fills m->path (start first) and returns its length in units, or -1 when there is no way. */
static float nav_astar(const P4WalkHost *h, NavMap *m, float sx, float sz, float tx, float tz) {
    int s = nav_nearest_free(h, m, sx, sz, 3), t = nav_nearest_free(h, m, tx, tz, 10);
    m->path_len = 0;
    if (s < 0 || t < 0) return -1.0f;
    int tix = t % NAV_NX, tiz = t / NAV_NX, n = 0;
    for (int i = 0; i < NAV_N; i++) { m->g[i] = 1e30f; m->closed[i] = 0; }
    m->g[s] = 0;
    m->from[s] = -1;
    heap_push(m, &n, 0, s);
    static const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, dz8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    while (n > 0) {
        NavEntry e = heap_pop(m, &n);
        int cur = e.node;
        if (m->closed[cur]) continue;
        m->closed[cur] = 1;
        if (cur == t) break;
        int cx = cur % NAV_NX, cz = cur / NAV_NX;
        for (int d = 0; d < 8; d++) {
            int nx = cx + dx8[d], nz = cz + dz8[d];
            int ns = nav_state(h, m, nx, nz);
            if (ns != NAV_FREE && ns != NAV_GATE) continue;
            if (d >= 4) {
                int s1 = nav_state(h, m, cx + dx8[d], cz), s2 = nav_state(h, m, cx, cz + dz8[d]);
                if ((s1 != NAV_FREE && s1 != NAV_GATE) || (s2 != NAV_FREE && s2 != NAV_GATE)) continue;
            }
            int ni = nav_idx(nx, nz);
            if (m->closed[ni]) continue;
            float step = (d < 4 ? 1.0f : 1.41421356f) * NAV_STEP * (ns == NAV_GATE ? 5.0f : nav_tight(h, m, nx, nz) ? 1.8f : 1.0f);
            for (int a = 0; a < m->navoid; a++) {
                float ax = nav_x(nx) - m->avoid_x[a], az = nav_z(nz) - m->avoid_z[a];
                if (ax * ax + az * az < 450.0f * 450.0f) { step *= 8.0f; break; }
            }
            float g2 = m->g[cur] + step;
            if (g2 >= m->g[ni]) continue;
            m->g[ni] = g2;
            m->from[ni] = cur;
            float ex = (float)abs(nx - tix), ez = (float)abs(nz - tiz), mn = ex < ez ? ex : ez, mx = ex < ez ? ez : ex;
            heap_push(m, &n, g2 + NAV_STEP * (mx + 0.41421356f * mn), ni);
        }
    }
    if (!m->closed[t]) return -1.0f;
    int len = 0;
    for (int i = t; i >= 0; i = m->from[i]) m->path[len++] = i;
    for (int i = 0; i < len / 2; i++) { int a = m->path[i]; m->path[i] = m->path[len - 1 - i]; m->path[len - 1 - i] = a; }
    m->path_len = len;
    return m->g[t];
}

/* a straight line over walkable nodes (checked every 40 units; a node on the line that is wide-tight is fine, a wall or void is not) */
static int nav_los(const P4WalkHost *h, NavMap *m, float x0, float z0, float x1, float z1) {
    float dx = x1 - x0, dz = z1 - z0, l = sqrtf(dx * dx + dz * dz);
    int n = (int)(l / 40.0f) + 1;
    for (int i = 1; i <= n; i++) {
        float f = (float)i / (float)n;
        if (nav_state(h, m, nav_ix(x0 + dx * f), nav_iz(z0 + dz * f)) != NAV_FREE) return 0;
    }
    return 1;
}


#define NAV_LIFT 30.0f

/* the closed gates: list 0xA objects the collision world counts (flags & 2, +0x150 == 1); the sampled floor is dropped when the set changes (a gate opened) */
static void nav_refresh_gates(const P4WalkHost *h, NavMap *m) {
    uint32_t obj = h->read32(h->user, SCENE_OBJECT), mgr = obj ? h->read32(h->user, obj + 8) : 0, sig = 0;
    int n = 0;
    float gx[16], gz[16];
    for (uint32_t p = mgr ? h->read32(h->user, mgr + 4u * 0xA) : 0; p && n < 16; p = h->read32(h->user, p + 0x138)) {
        if (!(h->read32(h->user, p + 0x28) & 2) || h->read32(h->user, p + 0x150) != 1) continue;
        uint32_t mh = h->read32(h->user, p + 0x144);
        if (!mh) continue;
        gx[n] = f32_of(h->read32(h->user, mh + 0x30));
        gz[n] = f32_of(h->read32(h->user, mh + 0x38));
        sig = sig * 31u + (h->read32(h->user, p) & 0xFFFFu) + 1u;
        n++;
    }
    if (sig != m->gate_sig || n != m->gate_n) {
        memset(m->st, 0, sizeof m->st);
        memset(m->tight, 0, sizeof m->tight);
        m->gate_sig = sig;
    }
    m->gate_n = n;
    memcpy(m->gate_x, gx, sizeof gx);
    memcpy(m->gate_z, gz, sizeof gz);
}

/* One planning step: re-plans every 10 frames (and when the target moves or the waypoint is reached) and returns the waypoint to steer at and the
 * way left in `metric`; 0 when there is no nav or no way over the sampled floor. */
static int nav_step(P4Walk *w, const P4WalkHost *h, float x, float z, float py, float tx, float tz, float *wx, float *wz, float *metric, uint32_t frame) {
    if (!h->nav_blocked || !h->nav_ground) return 0;
    if (!w->nav) {
        w->nav = calloc(1, sizeof(NavMap));
        if (!w->nav) return 0;
    }
    NavMap *m = w->nav;
    if (!w->nav_ready) {
        memset(m->st, 0, sizeof m->st);
        memset(m->tight, 0, sizeof m->tight);
        memset(m->forced, 0, sizeof m->forced);
        m->ref_y = py;
        m->lift = NAV_LIFT;
        m->gate_sig = 0xFFFFFFFFu;
        w->nav_ready = 1;
        w->nav_replan = 0;
    }
    float moved = hypotf(tx - w->nav_tx, tz - w->nav_tz), toward = hypotf(w->nav_wx - x, w->nav_wz - z);
    if (frame >= w->nav_replan || moved > 150.0f || toward < 120.0f) {
        nav_refresh_gates(h, m);
        m->navoid = 0;
        {
            uint32_t obj = h->read32(h->user, SCENE_OBJECT), mgr = obj ? h->read32(h->user, obj + 8) : 0;
            uint32_t p = mgr ? h->read32(h->user, mgr + 4u) : 0;
            for (int i = 0; p && i < 64 && m->navoid < 8; i++, p = h->read32(h->user, p + 0x138)) {
                if ((h->read32(h->user, p) & 0xFFFFu) < 0x460u) continue;
                uint32_t mm = h->read32(h->user, p + 0x164);
                if (!mm) continue;
                float sx = f32_of(h->read32(h->user, mm + 0x30)), sz = f32_of(h->read32(h->user, mm + 0x38));
                if (hypotf(sx - tx, sz - tz) < 300.0f) continue;                         /* the target itself */
                m->avoid_x[m->navoid] = sx;
                m->avoid_z[m->navoid++] = sz;
            }
        }
        float remain = nav_astar(h, m, x, z, tx, tz);
        if (remain < 0) {
            if (h->note && frame % 120 == 0) {
                char nm[160];
                snprintf(nm, sizeof nm, "NAV no way from (%.0f, %.0f) to (%.0f, %.0f): start node %d target node %d (states %d %d)", x, z, tx, tz,
                         nav_nearest_free(h, m, x, z, 3), nav_nearest_free(h, m, tx, tz, 10), nav_state(h, m, nav_ix(x), nav_iz(z)), nav_state(h, m, nav_ix(tx), nav_iz(tz)));
                h->note(h->user, nm);
            }
            return 0;
        }
        w->nav_replan = frame + 10;
        w->nav_tx = tx;
        w->nav_tz = tz;
        w->nav_remain = remain;
        int k = 1;
        float travelled = 0.0f;
        for (int i = 1; i < m->path_len; i++) {
            int a = m->path[i - 1], b = m->path[i];
            travelled += NAV_STEP * ((a % NAV_NX != b % NAV_NX && a / NAV_NX != b / NAV_NX) ? 1.41421356f : 1.0f);
            if (travelled > 700.0f) break;
            if (nav_los(h, m, x, z, nav_x(b % NAV_NX), nav_z(b / NAV_NX))) k = i;
        }
        w->nav_gate = 0;
        for (int i = 0; i < m->path_len && i < 14; i++) {
            int ix = m->path[i] % NAV_NX, iz = m->path[i] / NAV_NX;
            if (nav_state(h, m, ix, iz) != NAV_GATE) continue;
            float best = 1e30f;
            for (int g = 0; g < m->gate_n; g++) {
                float ex = nav_x(ix) - m->gate_x[g], ez = nav_z(iz) - m->gate_z[g], d2 = ex * ex + ez * ez;
                if (d2 < best) { best = d2; w->nav_gx = m->gate_x[g]; w->nav_gz = m->gate_z[g]; w->nav_gate = 1; }
            }
            break;
        }
        if (m->path_len < 2) { w->nav_wx = tx; w->nav_wz = tz; }
        else if (k == m->path_len - 1 && nav_los(h, m, x, z, tx, tz)) { w->nav_wx = tx; w->nav_wz = tz; }
        else { w->nav_wx = nav_x(m->path[k] % NAV_NX); w->nav_wz = nav_z(m->path[k] / NAV_NX); }
    }
    if (h->note && hypotf(w->nav_wx - w->nav_lx, w->nav_wz - w->nav_lz) > 60.0f) {   /* the route as a script: one line per new waypoint */
        char nm[96];
        snprintf(nm, sizeof nm, "NAVWP %.0f %.0f from %.0f %.0f remaining %.0f", w->nav_wx, w->nav_wz, x, z, w->nav_remain);
        h->note(h->user, nm);
        w->nav_lx = w->nav_wx;
        w->nav_lz = w->nav_wz;
    }
    *wx = w->nav_wx;
    *wz = w->nav_wz;
    *metric = w->nav_remain + hypotf(w->nav_wx - x, w->nav_wz - z) * 0.0f;
    return 1;
}

/* navprobe: how the game's tests see the player's own spot (one log line each), to calibrate the sphere's height. */
static void nav_probe(const P4WalkHost *h) {
    float x, z, fx, fz;
    uint32_t mtx = p4walk_player_matrix(h);
    char msg[400];
    if (!h->note || !h->nav_blocked || !h->nav_ground || !mtx || !p4walk_player(h, &x, &z, &fx, &fz)) return;
    float py = f32_of(h->read32(h->user, mtx + 0x34)), gy = 0.0f;
    int g = h->nav_ground(h->user, x, z, py + NAV_HI, py - NAV_LO, &gy);
    snprintf(msg, sizeof msg, "NAV player (%.1f, %.1f, %.1f) ground %d y %.2f", x, py, z, g, gy);
    h->note(h->user, msg);
    static const float lifts[] = { -40.0f, -10.0f, 0.0f, 10.0f, 30.0f, 55.0f, 90.0f, 140.0f };
    static const float radii[] = { 40.0f, 55.0f, 62.0f, 80.0f, 110.0f, 150.0f };
    for (unsigned r = 0; r < sizeof radii / sizeof radii[0]; r++) {
        int k = snprintf(msg, sizeof msg, "NAV sphere r %.0f at y+lift:", radii[r]);
        for (unsigned l = 0; l < sizeof lifts / sizeof lifts[0]; l++)
            k += snprintf(msg + k, sizeof msg - (size_t)k, " %+.0f=%d", lifts[l], h->nav_blocked(h->user, x, py + lifts[l], z, radii[r]));
        h->note(h->user, msg);
    }
    for (int d = 0; d < 8; d++) {
        static const float ex[8] = { 1, 0.7071f, 0, -0.7071f, -1, -0.7071f, 0, 0.7071f }, ez[8] = { 0, 0.7071f, 1, 0.7071f, 0, -0.7071f, -1, -0.7071f };
        int k = snprintf(msg, sizeof msg, "NAV dir %d(%.2f,%.2f) y+30 r62 blocked at:", d, ex[d], ez[d]);
        for (int dist = 50; dist <= 400; dist += 50)
            k += snprintf(msg + k, sizeof msg - (size_t)k, " %d=%d", dist, h->nav_blocked(h->user, x + ex[d] * (float)dist, py + 30.0f, z + ez[d] * (float)dist, 62.0f));
        h->note(h->user, msg);
    }
}

/* navdump:FILE: the sampled floor as a PGM (255 walkable, 140 next to a wall, 70 wall, 0 no floor, 200 the player) */
static void nav_dump(P4Walk *w, const P4WalkHost *h, const char *path) {
    float x, z, fx, fz;
    uint32_t mtx = p4walk_player_matrix(h);
    if (!h->nav_blocked || !h->nav_ground || !mtx || !p4walk_player(h, &x, &z, &fx, &fz)) return;
    if (!w->nav) w->nav = calloc(1, sizeof(NavMap));
    NavMap *m = w->nav;
    if (!m) return;
    memset(m->st, 0, sizeof m->st);
    memset(m->tight, 0, sizeof m->tight);
    memset(m->forced, 0, sizeof m->forced);
    m->ref_y = f32_of(h->read32(h->user, mtx + 0x34));
    m->lift = NAV_LIFT;
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P5\n%d %d\n255\n", NAV_NX, NAV_NZ);
    int px = nav_ix(x), pz = nav_iz(z);
    for (int iz = 0; iz < NAV_NZ; iz++)
        for (int ix = 0; ix < NAV_NX; ix++) {
            int st = nav_state(h, m, ix, iz);
            unsigned char v = st == NAV_FREE ? (nav_tight(h, m, ix, iz) ? 140 : 255) : st == NAV_WALL ? 70 : 0;
            if (abs(ix - px) <= 1 && abs(iz - pz) <= 1) v = 200;
            fputc(v, f);
        }
    fclose(f);
    char msg[300];
    snprintf(msg, sizeof msg, "NAV dump %s (%d x %d, 100 units a pixel, origin %.0f %.0f, player pixel %d %d)", path, NAV_NX, NAV_NZ, NAV_X0, NAV_Z0, px, pz);
    if (h->note) h->note(h->user, msg);
    w->nav_ready = 0;
}


static void fail(P4Walk *w, const P4WalkHost *h, const char *why) {
    char m[512];
    float x = 0, z = 0, fx = 0, fz = 0;
    int ok = p4walk_player(h, &x, &z, &fx, &fz);
    snprintf(m, sizeof m, "WALK SCRIPT FAILED at command %d '%s': %s; player %s x=%.1f z=%.1f facing=(%.2f,%.2f) aim=(%.0f,%.0f) planned=%d shut=%d", w->at + 1,
             w->cmd[w->at].text, why, ok ? "under control" : "NOT under control", x, z, fx, fz, w->aim_x, w->aim_z, w->step_d, w->nblocked);
    if (h->note) h->note(h->user, m);
    if (h->note && w->step_d >= 0) note_grid(h);                  /* a planned walk: the floor it failed on (the grid dump of the field change may be of an earlier floor) */
    w->status = P4WALK_FAILED;
}

/* The game's own frame counter (iGpffffb1a0, +1 per kwlnRunFrame): every timed command counts these, so retail (two vsyncs per game frame) and the port
 * (one flip per game frame) leave the game at the same counter for the same script. */
#define GAME_FRAME_COUNTER 0x00764290u

uint32_t p4walk_tick(P4Walk *w, const P4WalkHost *h) {
    uint32_t gf = h->read32(h->user, GAME_FRAME_COUNTER);
    if (gf == 0 || gf == 0xFFFFFFFFu || gf == w->last_game_frame || w->status != P4WALK_RUNNING) return w->mask;
    if (w->last_game_frame == 0 || gf < w->last_game_frame || gf - w->last_game_frame > 4) w->last_game_frame = gf - 1;
    uint32_t any = 0;                                   /* several game frames in one flip (turbo): a button held in any of them is down for the flip */
    while (w->last_game_frame < gf && w->status == P4WALK_RUNNING) {
        w->mask = p4walk_frame(w, h, ++w->last_game_frame);
        any |= w->mask;
    }
    return w->status == P4WALK_RUNNING ? any : 0;
}

static uint32_t walk_step(P4Walk *w, const P4WalkHost *h, uint32_t frame);

/* A telegraphed charge is native battle state, not a turn-count guess. Snapshot
 * it only when beginning a new command so an in-flight target pane stays owned. */
static int battle_enemy_charged(const P4WalkHost *h, uint32_t context, unsigned id) {
    uint32_t action = context ? h->read32(h->user, context + 0x174) : 0;
    for (unsigned n = 0; n < 32 && action > 0x100000u && action < 0x02000000u;
         n++, action = h->read32(h->user, action + 0x450)) {
        uint32_t unit = h->read32(h->user, action + 0x30);
        if (unit <= 0x100000u || unit >= 0x02000000u) continue;
        uint32_t data = h->read32(h->user, unit + 0xA64);
        if (data <= 0x100000u || data >= 0x02000000u) continue;
        uint32_t identity = h->read32(h->user, data);
        if ((identity & 4u) && (identity >> 16) == id &&
            (h->read32(h->user, data + 8) & 0xFFFFu) &&
            (h->read32(h->user, data + 0x14) & 0xC0000u)) return 1;
    }
    return 0;
}

/* TARGET owns a list of action pointers, not unit pointers. Use its live
 * candidates and cursor; formation/screen order is not the array's order.
 * An enemy or a living ally must have HP; a revive's ally must be down (HP 0).
 * A rule's ally binding does not apply to a pane without party candidates
 * (its macro fell back to an enemy-targeted command): that pane confirms as is. */
static int battle_bound_target(const P4WalkHost *h, uint32_t actor, unsigned id, int ally, int down) {
    uint32_t cursor = h->read32(h->user, actor + 0xD0);
    unsigned count = cursor & 0xFFFFu, selected = cursor >> 16;
    int found = 0, selected_match = 0, allies = 0;
    if (count > 12 || selected >= count) return -1;
    for (unsigned n = 0; n < count; n++) {
        uint32_t a = h->read32(h->user, actor + 0x98 + n * 4);
        if (a <= 0x100000u || a >= 0x02000000u) continue;
        uint32_t u = h->read32(h->user, a + 0x30);
        if (u <= 0x100000u || u >= 0x02000000u) continue;
        uint32_t data = h->read32(h->user, u + 0xA64);
        if (data <= 0x100000u || data >= 0x02000000u) continue;
        uint32_t identity = h->read32(h->user, data);
        int hp = (h->read32(h->user, data + 8) & 0xFFFFu) != 0, enemy = (identity & 4u) != 0;
        allies += !enemy;
        if (enemy == (ally != 0) || (identity >> 16) != id) continue;
        if (down ? hp : (!hp || (h->read32(h->user, data + 0xC) & 0x80000u))) continue;
        found = 1;
        if (n == selected) selected_match = 1;
    }
    if (ally && !allies) return 1;
    if (!found) return -1;
    /* Retail TARGET copies every candidate for target classes 1 and 2. */
    uint32_t table = h->read32(h->user, 0x007644A8u);
    unsigned skill = h->read32(h->user, actor + 0x6C) >> 16;
    unsigned mode = table > 0x100000u && table < 0x02000000u ?
        h->read32(h->user, table + skill * 0x28u + 8) & 0xFFu : 0;
    return selected_match || mode == 1 || mode == 2;
}

#define BTL_ENEMY_MAX 32
static uint32_t read8(const P4WalkHost *h, uint32_t a) { return (h->read32(h->user, a & ~3u) >> ((a & 3u) * 8u)) & 0xFFu; }
static uint32_t read16(const P4WalkHost *h, uint32_t a) { return (h->read32(h->user, a & ~3u) >> ((a & 2u) * 8u)) & 0xFFFFu; }
#define ENEMY_TABLE_PTR 0x007644B4u        /* iGpffffb3c4: 0x3C-byte enemy rows by DatUnit id, skills u16[8] at +0xE */
#define AFFINITY_TABLE_PTR 0x007644B8u     /* iGpffffb3c8: 0x20-byte affinity rows (u16 per element) by DatUnit id2 */

/* datCalcGetAffinity's inputs per element 0..7 (physical, fire, ice, elec, wind, almighty, light, dark): the passive skill IDs for resist,
 * null, repel and absorb (0: none), and the Break and Wall buff indices (-1: none). */
static const uint16_t kAffinityPassives[8][4] = {
    {0x1B8, 0x1B9, 0x1BA, 0x1BB}, {0x1BC, 0x1BD, 0x1BE, 0x1BF}, {0x1C0, 0x1C1, 0x1C2, 0x1C3}, {0x1C4, 0x1C5, 0x1C6, 0x1C7},
    {0x1C8, 0x1C9, 0x1CA, 0x1CB}, {0, 0, 0, 0}, {0x1CC, 0x1CD, 0x1CE, 0}, {0x1CF, 0x1D0, 0x1D1, 0}};
static const int8_t kAffinityBreak[8] = {-1, 8, 9, 11, 10, -1, -1, -1};
static const int8_t kAffinityWall[8] = {-1, 12, 13, 14, 15, -1, -1, -1};

/* One unit record (DatUnit: identity +0, id2 +4, HP +8, ailments +0xC, charge flags +0x14, buff levels +0x1C) into a rule snapshot, with an
 * enemy's element affinities; 0 if the pointer is not one. */
static int battle_unit(const P4WalkHost *h, uint32_t unit, int want_enemy, P4WalkBattleUnit *out) {
    if (unit <= 0x100000u || unit >= 0x02000000u) return 0;
    uint32_t data = h->read32(h->user, unit + 0xA64);
    if (data <= 0x100000u || data >= 0x02000000u) return 0;
    uint32_t identity = h->read32(h->user, data);
    if (((identity & 4u) != 0) != (want_enemy != 0)) return 0;
    memset(out, 0, sizeof *out);
    out->id = identity >> 16;
    out->hp = h->read32(h->user, data + 8) & 0xFFFFu;
    out->sp = h->read32(h->user, data + 8) >> 16;
    out->bad = h->read32(h->user, data + 0xC);
    out->status = h->read32(h->user, data + 0x14);
    for (unsigned k = 0; k < 2; k++) {
        uint32_t word = h->read32(h->user, data + 0x1C + k * 4u);
        for (unsigned b = 0; b < 4; b++) out->buffs[k * 4u + b] = (uint8_t)(word >> (8u * b));
    }
    uint32_t rows = h->read32(h->user, AFFINITY_TABLE_PTR), meta = h->read32(h->user, ENEMY_TABLE_PTR);
    if (want_enemy && rows && meta) {
        unsigned id2 = h->read32(h->user, data + 4) & 0xFFFFu, skills[8];
        for (unsigned s = 0; s < 8; s++) skills[s] = read16(h, meta + out->id * 0x3Cu + 0xEu + s * 2u);
        int guarding = (identity & 0x200u) != 0;
        for (unsigned e = 0; e < 8; e++) {
            unsigned passives = 0;
            for (unsigned s = 0; s < 8; s++) {
                for (unsigned p = 0; p < 4; p++)
                    if (kAffinityPassives[e][p] && skills[s] == kAffinityPassives[e][p]) passives |= 1u << p;
                if (e != 5 && skills[s] == 0x1DB) passives |= 2u;   /* nulls every element but almighty */
            }
            int broken = kAffinityBreak[e] >= 0 && rule_buff_level(out, (unsigned)kAffinityBreak[e]) > 0;
            int walled = kAffinityWall[e] >= 0 && rule_buff_level(out, (unsigned)kAffinityWall[e]) > 0;
            uint32_t a = p4walk_enemy_affinity(read16(h, rows + id2 * 0x20u + e * 2u), passives, guarding, broken, walled);
            if (a & 0x8000000u) out->weak |= (uint16_t)(1u << e);
            if (a & 0x7000000u) out->block |= (uint16_t)(1u << e);
            if (a & 0x10000000u) out->resist |= (uint16_t)(1u << e);
        }
    }
    return 1;
}

/* The battlerule: snapshot: the party chain (context +0x178, next +0xA6C) and the enemy action list (+0x174, next +0x450). */
static void battle_view(const P4WalkHost *h, uint32_t context, P4WalkBattleUnit *party, int *np, P4WalkBattleUnit *enemies, int *ne) {
    *np = *ne = 0;
    uint32_t u = h->read32(h->user, context + 0x178);
    for (int n = 0; n < BTL_PARTY_MAX && u > 0x100000u && u < 0x02000000u; n++, u = h->read32(h->user, u + 0xA6C))
        if (battle_unit(h, u, 0, &party[*np])) ++*np;
    uint32_t a = h->read32(h->user, context + 0x174);
    for (int n = 0; n < BTL_ENEMY_MAX && a > 0x100000u && a < 0x02000000u; n++, a = h->read32(h->user, a + 0x450))
        if (*ne < BTL_ENEMY_MAX && battle_unit(h, h->read32(h->user, a + 0x30), 1, &enemies[*ne])) ++*ne;
}

#define SKILL_TABLE_PTR 0x007644A8u        /* gp-0x4C48: iGpffffb3b8, 0x28-byte DatSkillEntry rows (flags01 at +1) */
#define ITEM_TABLE_PTR 0x007642A0u         /* the item metadata pointer slot (datEntryAt): 0x44-byte rows, granted skill at +0x24 */
#define ITEM_COUNTS 0x0079757Au            /* D_0079757A: datItemCount's byte per item ID */
#define HERO_PERSONA_SLOT 0x00797F88u      /* D_00797F88: the hero's active roster slot (datPersonaGetRecord) */
#define HERO_ROSTER 0x00797F8Cu            /* datPersonaRosterSlot: 0x30-byte records */
#define PARTY_SLOTS 0x00796E50u            /* DatPartySlot[ID - 2], 0x88 bytes, persona record at +0x54 */
#define FLAG_WORDS 0x0079B1CCu             /* D_00797560 +0x3C6C: datGetFlag's words */

static int skill_active(const P4WalkHost *h, uint32_t table, unsigned id) {
    const int active = h->skill_active ? h->skill_active(h->user, id) : id < 0x1B8u;
    return id && active && ((h->read32(h->user, table + id * 0x28u) >> 8) & 2u);
}

/* The actor's Skill or Item list as btlSelectionRefreshList (00207140) builds it: its count, with the character ID in *pc. */
static int battle_list(const P4WalkHost *h, uint32_t actor, int items, uint16_t list[256], unsigned *pc) {
    uint32_t unit = h->read32(h->user, actor + 0x30);
    int count = 0;
    uint32_t table = h->read32(h->user, SKILL_TABLE_PTR);
    *pc = h->read32(h->user, unit + 0xA4) & 0xFFFFu;
    if (items) {
        uint32_t meta = h->read32(h->user, ITEM_TABLE_PTR);
        for (unsigned id = 0x301; meta && table && id < 0x400; id++) {
            if (!((h->read32(h->user, (ITEM_COUNTS + id) & ~3u) >> (((ITEM_COUNTS + id) & 3u) * 8u)) & 0xFFu)) continue;
            if (skill_active(h, table, h->read32(h->user, meta + id * 0x44u + 0x24u) & 0xFFFFu)) list[count++] = (uint16_t)id;
        }
    } else {
        uint32_t record = 0;
        if (*pc == 1) record = HERO_ROSTER + (h->read32(h->user, HERO_PERSONA_SLOT) & 0xFFFFu) * 0x30u;
        else if (*pc >= 2 && *pc <= 10) record = PARTY_SLOTS + (*pc - 2u) * 0x88u + 0x54u;
        if (record && table && !((h->read32(h->user, unit + 0xA0) >> 16) & 0xFFu))   /* BtlUnit +0xA2 nonzero lists nothing */
            for (unsigned i = 0; i < 8; i++) {
                unsigned id = (h->read32(h->user, record + 0xC + (i & ~1u) * 2u) >> ((i & 1u) * 16u)) & 0xFFFFu;
                if (skill_active(h, table, id)) list[count++] = (uint16_t)id;
            }
    }
    return count;
}

/* A rule whose skill:/item: entry is missing from the actor's list (a spent Revival Bead, a skill not learned) cannot apply. */
static int battle_rule_usable(const P4WalkHost *h, uint32_t actor, const P4WalkBattleRule *rule) {
    uint16_t list[256];
    unsigned pc;
    for (int k = 0; k < rule->key_count; k++) {
        unsigned key = rule->keys[k];
        if (!(key & (P4WALK_KEY_SKILL | P4WALK_KEY_ITEM))) continue;
        int count = battle_list(h, actor, (key & P4WALK_KEY_ITEM) != 0, list, &pc), held = 0;
        for (int i = 0; i < count && !held; i++) held = list[i] == (key & 0xFFFFu);
        if (!held) return 0;
    }
    return 1;
}

/* skill:ID / item:ID: the signed Down/Up presses from the row the actor's Skill or Item list opens on to that entry (see the header).
 * The opening row is btlSelectionUpdateWindow's (00202D20). 0 with `why` set on failure. */
static int battle_list_key_moves(const P4WalkHost *h, uint32_t actor, unsigned key, int *moves, char *why, size_t whysz) {
    unsigned pc, want = key & 0xFFFFu;
    int items = (key & P4WALK_KEY_ITEM) != 0;
    uint16_t list[256];
    int count = battle_list(h, actor, items, list, &pc);
    int cursor = 0;                                         /* pair 1 (Skill) or 2 (Item) of the remembered {first, selected} rows */
    uint32_t saved = h->read32(h->user, actor + 0x3F0);
    if (((h->read32(h->user, FLAG_WORDS + 4u) >> 28) & 1u) && saved > 0x100000u && saved < 0x02000000u)
        cursor = (int16_t)(h->read32(h->user, saved + (items ? 8u : 4u)) >> 16);
    if (cursor < 0 || cursor >= count) cursor = 0;          /* a remembered row past the list's end opens on row 0 */
    if (p4walk_battle_list_moves(list, count, cursor, want, moves)) return 1;
    int at = snprintf(why, whysz, "%s:%#x is not in character %u's %s list (%d entries:", items ? "item" : "skill", want, pc,
                      items ? "Item" : "Skill", count);
    for (int i = 0; i < count && i < 8 && at > 0 && (size_t)at < whysz; i++)
        at += snprintf(why + at, whysz - (size_t)at, " %#x", list[i]);
    if (at > 0 && (size_t)at < whysz) snprintf(why + at, whysz - (size_t)at, "%s)", count > 8 ? " ..." : "");
    return 0;
}

#define MESSAGE_WINDOW_CURRENT 0x00882F60u /* NmMesWindowRecord returned by nmMesWindowCurrent: handle +0, state byte +4 */
#define MESSAGE_WINDOW_SLOTS 0x00881808u   /* ItfMesWindowSlot[], 0x20 stride, window at +0 */

unsigned p4walk_skill_prompt_buttons(const P4WalkSkillPrompt *p) {
    switch (p->state) {
    case 0: case 1: case 6:                                 /* opening and "did not learn" messages: text only */
        return p->row_waiting ? 1u << P4W_CROSS : 0;
    case 2:                                                 /* the list: walk to the offered skill (slot 8) and pick it */
        if (!p->list_active || p->list_cursor < 0 || p->pending <= 0) return 0;
        return 1u << (p->list_cursor == 8 ? P4W_CROSS : P4W_LEFT);
    case 3:                                                 /* Yes only for the offered skill itself, otherwise No */
        if (p->choice_active) {
            int wanted = p->replacement_pending ? 0 : 1;
            if (p->choice_cursor == wanted) return 1u << P4W_CROSS;
            return 1u << (wanted ? P4W_DOWN : P4W_UP);
        }
        return p->row_waiting ? 1u << P4W_CROSS : 0;
    default:
        return 0;
    }
}

/* The skill-replace task (shdPersonaSkillSelectSeq 0011f5a0, stored under the 23-character name) while it reads input
 * (states 0-3 and 6): its persona panel and the current message window, as the battle simulator reads them. */
static int battle_skill_prompt(const P4WalkHost *h, P4WalkSkillPrompt *p) {
    uint32_t node = task_node(h, "shdPersonaSkillSelectSe");
    uint32_t work = node ? h->read32(h->user, node + TASK_WORK) : 0;
    uint32_t parent = work ? h->read32(h->user, work + 0xC) : 0;
    uint32_t panel = parent ? h->read32(h->user, parent + TASK_WORK) : 0;
    if (!panel) return 0;
    memset(p, 0, sizeof *p);
    p->state = (int32_t)h->read32(h->user, work + 8);
    if (p->state < 0 || (p->state > 3 && p->state != 6)) return 0;
    p->list_active = (h->read32(h->user, panel + 0x534) & 0x800u) != 0;
    p->list_cursor = (int8_t)read8(h, panel + 0x88);
    p->pending = (int32_t)read16(h, panel + 0x8C + 0x224);
    p->replacement_pending = read16(h, work + 6) == read16(h, panel + 0x8C + 0x66);
    int32_t handle = (int32_t)h->read32(h->user, MESSAGE_WINDOW_CURRENT);
    int8_t window_state = (int8_t)read8(h, MESSAGE_WINDOW_CURRENT + 4);     /* 1 text, 2 choice/close */
    if ((window_state == 1 || window_state == 2) && handle >= 0 && handle < 0x100) {
        uint32_t window = h->read32(h->user, MESSAGE_WINDOW_SLOTS + (uint32_t)handle * 0x20u);
        if (window) {
            p->row_waiting = (int8_t)read8(h, window + 0x24) == 1;
            p->choice_active = (int16_t)read16(h, window + 0x48) == 2 && (int16_t)read16(h, window + 0x4E) == 2;
            p->choice_cursor = (int16_t)read16(h, window + 0x4A);
        }
    }
    return 1;
}

/* The battle driver: in a fight it owns the pad (a held-back walk script would press Attack every 24 frames); outside one it returns 0 and the
 * script's own mask stands. */
static int battle_drive(P4Walk *w, const P4WalkHost *h, uint32_t frame, uint32_t *mask) {
    BattleDriver *d = &w->bd;
    if (!task_node(h, "battle")) {
        d->owns = 0;
        d->seen = -2;
        d->last_waiting = 0;
        memset(d->commanded, 0, sizeof d->commanded);       /* the wheel starts on Attack again in the next fight */
        return 0;
    }
    d->owns = 1;
    uint32_t c = h->read32(h->user, BATTLE_CONTEXT);
    uint32_t state = c ? h->read32(h->user, c + 0x2FC) : 0;
    if ((int32_t)state != d->seen) { d->seen = (int32_t)state; d->state_start = frame; }
    uint32_t actor = c ? h->read32(h->user, c + 0x104) : 0, flags = c ? h->read32(h->user, c + 0xC) : 0;
    uint32_t unit = 0, action_state = 0;
    if (actor > 0x100000u && actor < 0x02000000u) {
        unit = h->read32(h->user, actor + 0x30);
        action_state = h->read32(h->user, actor + 0xC) & 0xFFFFu;
    }
    int party = -1;
    uint32_t u = c ? h->read32(h->user, c + 0x178) : 0;
    for (int n = 0; n < BTL_PARTY_MAX && u > 0x100000u && u < 0x02000000u; n++, u = h->read32(h->user, u + 0xA6C))
        if (u == unit) { party = n; break; }
    if (d->acted && actor != d->plan_actor) {
        d->acted = 0;
        d->last_waiting = 0;
        d->hold_until = 0;
    }
    /* TARGET is still player input, but may only continue a plan armed in COMMAND
     * for this same action. A fresh plan must not start in an unrelated target pane. */
    int input_state = action_state == BTL_ACTION_COMMAND ||
        (action_state == BTL_ACTION_TARGET && d->acted && actor == d->plan_actor);
    int waiting = state == BTL_CMD_STATE && input_state && party >= 0 && !(flags & BTL_ACTION_RUNNING) && frame - d->state_start >= d->skip;
    if (waiting != d->last_waiting) {
        d->last_waiting = waiting;
        d->acted = 0;
        d->wait_since = frame;
        d->hold_until = 0;
    }
    /* A results-screen level-up with eight skills asks which to forget; its Yes/No defaults to No, so the periodic Cross
     * below would loop forever. Decline the offer through the real list and Yes/No: one press, then a release. */
    P4WalkSkillPrompt prompt;
    if (!waiting && state == BTL_RESULT_STATE && battle_skill_prompt(h, &prompt)) {
        if (frame >= d->prompt_at) {
            unsigned bits = p4walk_skill_prompt_buttons(&prompt);
            if (bits) { d->hold_mask = bits; d->hold_until = frame + 5; d->prompt_at = frame + 10; }
        }
    /* Explicit result profile: an optional Persona decision owns Cross, not its
     * underlying card grid. Ordinary plans retain their legacy input cadence. */
    } else if (!waiting && task_node(h, "battle_shuffle") &&
        (!d->persona_modal || !task_node(h, "shdPersonaStatusDraw"))) {
        if (frame >= d->shuffle_at) {
            static const int move[] = { P4W_RIGHT, P4W_RIGHT, P4W_RIGHT, P4W_DOWN, P4W_LEFT, P4W_LEFT, P4W_LEFT, P4W_DOWN };
            int s = d->shuffle_step++;
            d->hold_mask = 1u << ((s & 1) ? P4W_CROSS : move[(s >> 1) & 7]);
            d->hold_until = frame + 5;
            d->shuffle_at = frame + 45;
        }
    } else if (!waiting) {                                    /* Help windows, enemy turns, results: Cross every 24 frames */
        d->shuffle_step = 0;
        if (frame >= d->next_skip) { d->hold_mask = 1u << P4W_CROSS; d->hold_until = frame + 5; d->next_skip = frame + 24; }
    } else {
        if (!d->acted && frame - d->wait_since >= d->delay) {
            d->acted = 1;
            d->start_at = frame;
            d->plan_actor = actor;
            d->step = 0;
            d->target_moves = 0;
            d->nav_left = -1;
            d->hold_until = 0;
            d->cur_first = party < BTL_PARTY_MAX && !d->commanded[party];
            d->cur_pc = (h->read32(h->user, unit + 0xA4) & 0xFFFFu) - 1u;
            d->cur_guard = d->guard_enemy &&
                (!d->guard_party || (d->cur_pc < BTL_PARTY_MAX && (d->guard_party & (1u << d->cur_pc)))) &&
                battle_enemy_charged(h, c, d->guard_enemy);
            d->cur_rule = -1;
            d->cur_ally = 0;
            d->cur_enemy = 0;
            /* battleturn: the Nth fresh command takes the Nth line (a retry keeps its line), then the host's, ahead of everything else. */
            d->cur_turn = -1;
            if (d->turn_retry != -1) { d->cur_turn = d->turn_retry; d->turn_retry = -1; }
            else if (d->turn_next < d->nturns) d->cur_turn = d->turn_next++;
            else if (h->battle_turn) {
                char line[512];
                if (h->battle_turn(h->user, (unsigned)d->nturns + d->host_turns, line, sizeof line)) {
                    if (!p4walk_battle_turn_parse(line, &d->host_turn)) {
                        fail(w, h, "the host's battle turn is not a battleturn: line");
                        *mask = 0;
                        return 1;
                    }
                    d->cur_turn = -2;
                    d->host_turns++;
                }
            }
            if (d->cur_turn != -1) {
                const P4WalkBattleRule *turn = d->cur_turn == -2 ? &d->host_turn : &d->turns[d->cur_turn];
                d->cur_guard = 0;
                d->cur_ally = turn->kind == P4WALK_RULE_ALLY || turn->kind == P4WALK_RULE_DEAD ? turn->value : 0;
                d->cur_enemy = turn->kind == P4WALK_RULE_ENEMY ? turn->value : 0;
                if (h->note) {
                    char m[96];
                    snprintf(m, sizeof m, "battle turn %u for character %u",
                             d->cur_turn == -2 ? (unsigned)d->nturns + d->host_turns : (unsigned)d->cur_turn + 1u, d->cur_pc + 1);
                    h->note(h->user, m);
                }
            } else if (!d->cur_guard && d->nrules > 0) {
                P4WalkBattleUnit allies[BTL_PARTY_MAX], enemies[BTL_ENEMY_MAX];
                P4WalkBattleView view = {d->cur_pc + 1u, allies, 0, enemies, 0, 0};
                battle_view(h, c, allies, &view.party_count, enemies, &view.enemy_count);
                for (int r = 0; r < d->nrules; r++)
                    if (!battle_rule_usable(h, actor, &d->rules[r])) view.unusable |= 1u << r;
                d->cur_rule = p4walk_battle_rule_select(d->rules, d->nrules, &view, &d->cur_ally, &d->cur_enemy);
                if (d->cur_rule >= 0 && h->note) {
                    char m[112];
                    snprintf(m, sizeof m, "battle rule %d for character %u (ally %u, enemy %u)", d->cur_rule + 1, d->cur_pc + 1,
                             (unsigned)d->cur_ally, (unsigned)d->cur_enemy);
                    h->note(h->user, m);
                }
            }
            if (!d->cur_guard && d->cur_rule < 0 && party < BTL_PARTY_MAX) d->commanded[party] = 1;
        }
        if (d->acted) {
            const unsigned *bits = d->cur_first ? d->first : d->plan;
            int len = d->cur_first ? d->first_len : d->plan_len;
            if (d->cur_guard) {
                static const unsigned guard[] = {1u << P4W_CIRCLE, 1u << P4W_UP, 1u << P4W_CROSS};
                bits = guard;
                len = sizeof guard / sizeof guard[0];
            } else if (d->cur_turn != -1) {
                const P4WalkBattleRule *turn = d->cur_turn == -2 ? &d->host_turn : &d->turns[d->cur_turn];
                bits = turn->keys;
                len = turn->key_count;
            } else if (d->cur_rule >= 0) {
                bits = d->rules[d->cur_rule].keys;
                len = d->rules[d->cur_rule].key_count;
            } else if (d->cur_first && d->cur_pc < BTL_PARTY_MAX && d->first_pc_len[d->cur_pc] > 0) {
                bits = d->first_pc[d->cur_pc];
                len = d->first_pc_len[d->cur_pc];
            } else if (!d->cur_first && d->cur_pc < BTL_PARTY_MAX && d->plan_pc_len[d->cur_pc] > 0) {
                bits = d->plan_pc[d->cur_pc];
                len = d->plan_pc_len[d->cur_pc];
            }
            if (d->step < len && (bits[d->step] & (P4WALK_KEY_SKILL | P4WALK_KEY_ITEM)) && frame >= d->start_at + (uint32_t)d->step * 14u) {
                /* skill:ID / item:ID: count the presses once, then one Down/Up per 14-frame slot (later keys wait), then move on. */
                if (d->nav_left < 0) {
                    int moves;
                    char why[192];
                    if (!battle_list_key_moves(h, actor, bits[d->step], &moves, why, sizeof why)) {
                        fail(w, h, why);
                        *mask = 0;
                        return 1;
                    }
                    d->nav_key = 1u << (moves < 0 ? P4W_UP : P4W_DOWN);
                    d->nav_left = moves < 0 ? -moves : moves;
                }
                if (d->nav_left > 0) {
                    d->hold_mask = d->nav_key;
                    d->nav_left--;
                    d->start_at += 14u;
                } else {
                    d->hold_mask = 0;
                    d->step++;
                    d->nav_left = -1;
                }
                d->hold_until = frame + 5;
            } else if (d->step < len && frame >= d->start_at + (uint32_t)d->step * 14u) {
                const P4WalkBattleRule *bound = d->cur_turn == -2 ? &d->host_turn : d->cur_turn >= 0 ? &d->turns[d->cur_turn] :
                                                d->cur_rule >= 0 ? &d->rules[d->cur_rule] : NULL;
                int ally = bound && d->cur_ally, foe = bound && d->cur_enemy;
                unsigned id = ally ? d->cur_ally : foe ? d->cur_enemy : d->cur_pc < BTL_PARTY_MAX ? d->target_pc[d->cur_pc] : 0;
                int target = 1;
                if (id && !d->cur_guard && action_state == BTL_ACTION_TARGET && (bits[d->step] & (1u << P4W_CROSS)))
                    target = battle_bound_target(h, actor, id, ally, ally && bound->kind == P4WALK_RULE_DEAD);
                if (target < 0 || (!target && d->target_moves >= 12)) {
                    char why[128];
                    if ((ally || foe) && d->cur_turn != -1)
                        snprintf(why, sizeof why, "battle turn %u's %s %u is %s",
                                 d->cur_turn == -2 ? (unsigned)d->nturns + d->host_turns : (unsigned)d->cur_turn + 1u, ally ? "ally" : "enemy", id,
                                 target < 0 ? "absent from TARGET" : "not reachable with TARGET input");
                    else if (ally || foe)
                        snprintf(why, sizeof why, "battle rule %d's %s %u is %s", d->cur_rule + 1, ally ? "ally" : "enemy", id,
                                 target < 0 ? "absent from TARGET" : "not reachable with TARGET input");
                    else
                        snprintf(why, sizeof why, "battle target%u=%u is %s", d->cur_pc + 1, id,
                                 target < 0 ? "dead or absent from TARGET" : "not reachable with TARGET input");
                    fail(w, h, why);
                    *mask = 0;
                    return 1;
                }
                if (!target) {
                    /* Fresh Right edges wrap the native screen-sorted enemy list.
                     * Defer this Cross (and later keys), never confirm a fallback. */
                    d->hold_mask = 1u << P4W_RIGHT;
                    d->target_moves++;
                    d->start_at = frame + 14u - (uint32_t)d->step * 14u;
                } else {
                    d->hold_mask = bits[d->step++];
                    d->target_moves = 0;
                }
                d->hold_until = frame + 5;
            }
            else if (d->step >= len && frame - d->start_at > 600) {                     /* the plan did not take: again (a turn keeps its line) */
                d->acted = 0;
                d->wait_since = frame - d->delay;
                if (d->cur_turn != -1) d->turn_retry = d->cur_turn;
            }
        }
    }
    *mask = frame < d->hold_until ? d->hold_mask : 0;
    return 1;
}

uint32_t p4walk_frame(P4Walk *w, const P4WalkHost *h, uint32_t frame) {
    uint32_t held = walk_step(w, h, frame), m;
    if (w->bd.on && w->status == P4WALK_RUNNING && battle_drive(w, h, frame, &m)) return m;
    return held;
}

static uint32_t walk_step(P4Walk *w, const P4WalkHost *h, uint32_t frame) {
    if (w->status != P4WALK_RUNNING) return 0;
    for (int guard = 0; guard < 8; guard++) {
        if (w->at >= w->n) { w->status = P4WALK_DONE; if (h->note) h->note(h->user, "walk script done"); return 0; }
        Cmd *c = &w->cmd[w->at];
        if (!w->started) {
            w->started = 1;
            w->start = frame;
            w->best = 1e30f;
            w->best_frame = frame;
            w->event_frames = 0;
            w->door_tries = 0;
            w->cross_until = 0;
            w->nblocked = 0;
            w->gate_phase = 0;
            w->resets = 0;
            w->btl_menu_frames = 0;
            w->step_d = -1;
            w->no_detour = 0;
            w->detoured = 0;
            w->nav_ready = 0;
            w->nav_active = 0;
            w->nav_forced = 0;
            w->gate_cycles = 0;
            w->goal_scoped = 0;
            w->choice_window = w->choice_next = w->choice_hold_until = w->choice_mask = 0;
            w->choice_commit = 0;
            w->nav_tx = w->nav_tz = w->nav_wx = w->nav_wz = w->nav_lx = w->nav_lz = 0.0f;
            if (h->note) {
                char m[160];
                snprintf(m, sizeof m, "walk script %d/%d: %s (frame %u)", w->at + 1, w->n, c->text, frame);
                h->note(h->user, m);
            }
        }
        uint32_t el = frame - w->start;
        int done = 0;
        uint32_t held = 0;
        float x, z, fx, fz;
        switch (c->kind) {
        case C_EXIT:
            w->status = P4WALK_DONE;
            return 0;
        case C_PUSH:                                      /* steer at the point for N frames without a stuck check: to face a wall or a door */
            done = el >= (uint32_t)c->c;
            if (!done && p4walk_player(h, &x, &z, &fx, &fz))
                held = steer(h, c->a - x, c->b - z);
            break;
        case C_LOG: {                                     /* one line: position, event flag, field and room ids */
            uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE);
            uint32_t work = root ? h->read32(h->user, root + TASK_WORK) : 0;
            uint32_t ids = work ? h->read32(h->user, work + 0x18) : 0xFFFFFFFFu;
            char m[256];
            int ok = p4walk_player(h, &x, &z, &fx, &fz);
            uint32_t blk = 0x008872E0u + h->read32(h->user, RNG_SLOT_INDEX), base = h->read32(h->user, blk), front = h->read32(h->user, blk + 4);
            uint32_t hash = 0;
            for (uint32_t k = 0; base && k < 31; k++) hash = (hash << 5 | hash >> 27) ^ h->read32(h->user, base + 4 * k);
            uint32_t cam = h->read32(h->user, CAMERA_POINTER), cf = cam ? h->read32(h->user, cam + 4) : 0;
            uint32_t step_task = h->read32(h->user, 0x00764334u), step_work = step_task ? h->read32(h->user, step_task + TASK_WORK) : 0;
            snprintf(m, sizeof m, "LOG frame %u pos %s %.1f %.1f event %u field %u room %u rng front %u state %08x cam %.1f %.1f %.1f at %.2f %.2f %.2f step %d seed %08x", frame, ok ? "ok" : "none", x, z,
                     h->read32(h->user, EVENT_FLAG), ids & 0xFFFF, ids >> 16, base ? (front - base) / 4 : 0u, hash,
                     cf ? f32_of(h->read32(h->user, cf + 0x40)) : 0.f, cf ? f32_of(h->read32(h->user, cf + 0x44)) : 0.f, cf ? f32_of(h->read32(h->user, cf + 0x48)) : 0.f,
                     cf ? f32_of(h->read32(h->user, cf + 0x30)) : 0.f, cf ? f32_of(h->read32(h->user, cf + 0x34)) : 0.f, cf ? f32_of(h->read32(h->user, cf + 0x38)) : 0.f,
                     step_work ? (int32_t)h->read32(h->user, step_work + 4) : -1, h->read32(h->user, 0x00764330u));
            if (h->note) h->note(h->user, m);
            done = 1;
            break;
        }
        case C_WAIT:
            done = el >= (uint32_t)c->a;
            break;
        case C_PRESS:
            if (el < (uint32_t)c->a) held = 1u << c->button;
            done = el >= (uint32_t)c->a + 3u;           /* then 3 frames released */
            break;
        case C_SKIPEVTMSG: {
            uint32_t message = 0, row = 0;
            uint32_t win = evt_text_window(h, &message, &row);
            done = win && message == c->addr && row == c->value;
            if (!done && win && el % 24 < 3) held = 1u << P4W_CROSS;
            break;
        }
        case C_WAITCHOICE: {
            unsigned cursor = 0, count = 0;
            int opening;
            done = choice_window(h, &cursor, &count, &opening) != 0;
            if (!done && !opening && text_window_open(h, 1) && el % 24 < 3)
                held = 1u << P4W_CROSS;
            break;
        }
        case C_CHOOSE: {
            unsigned cursor = 0, count = 0;
            int opening;
            uint32_t win = choice_window(h, &cursor, &count, &opening);
            if (w->choice_commit && win != w->choice_window) {
                done = frame >= w->choice_hold_until + 3;
            } else if (win) {
                if (c->value > count) { fail(w, h, "choice is outside the visible menu"); return 0; }
                if (win != w->choice_window) {
                    w->choice_window = win;
                    w->choice_next = frame + 6;          /* release the preceding text-skip Cross */
                    w->choice_hold_until = 0;
                    w->choice_mask = 0;
                }
                if (frame >= w->choice_next) {
                    unsigned target = c->value - 1;
                    w->choice_commit = cursor == target;
                    w->choice_mask = 1u << (w->choice_commit ? P4W_CROSS : cursor < target ? P4W_DOWN : P4W_UP);
                    w->choice_hold_until = frame + 3;
                    w->choice_next = frame + 14;
                }
                if (frame < w->choice_hold_until) held = w->choice_mask;
            } else if (!opening) {
                held = el % 24 < 3 ? 1u << P4W_CROSS : 0;
            }
            break;
        }
        case C_WAITFIELD: {
            uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE);
            uint32_t work = root ? h->read32(h->user, root + TASK_WORK) : 0;
            uint32_t ids = work ? h->read32(h->user, work + 0x18) : 0xFFFFFFFFu;
            done = work && (ids & 0xFFFF) == c->addr && (ids >> 16) == c->value;
            break;
        }
        case C_WAITSTEP: {                                /* FldStepIndex (0015a160): generated floors all use the same field/room */
            uint32_t task = h->read32(h->user, 0x00764334u);
            uint32_t work = task ? h->read32(h->user, task + TASK_WORK) : 0;
            done = work && h->read32(h->user, work + 4) == c->value;
            break;
        }
        case C_WAITSTATE: {
            uint32_t v = h->read32(h->user, c->on ? c->addr & ~3u : c->addr);
            if (c->on) v = (v >> ((c->addr & 2u) * 8u)) & 0xFFFFu;
            done = c->ne ? v != c->value : v == c->value;
            break;
        }
        case C_WAITMSG:
            done = (h->read32(h->user, EVENT_FLAG) == 0) == (c->on != 0);
            break;
        case C_WAITCONTROL:
            done = (task_work(h, "rotate pc") != 0) == (c->on != 0);
            break;
        case C_SKIPFIELD: {                               /* Cross every 24 frames until the field root's ids are F:R (events and shops on the way) */
            uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE);
            uint32_t work = root ? h->read32(h->user, root + TASK_WORK) : 0;
            uint32_t ids = work ? h->read32(h->user, work + 0x18) : 0xFFFFFFFFu;
            done = work && (ids & 0xFFFF) == c->addr && (ids >> 16) == c->value;
            if (!done && el % 24 < 3) held = 1u << P4W_CROSS;
            break;
        }
        case C_SKIPFOR:                                   /* Cross every 24 frames for N frames, control or not */
            done = el >= (uint32_t)c->a;
            if (!done && el % 24 < 3) held = 1u << P4W_CROSS;
            break;
        case C_AUTOSKIP:
            w->autoskip = c->on;
            done = 1;
            break;
        case C_BATTLESKIP:
            w->battleskip = c->on;
            done = 1;
            break;
        case C_NAVPROBE:
            nav_probe(h);
            done = 1;
            break;
        case C_NAVDUMP:
            nav_dump(w, h, c->text + 8);
            done = 1;
            break;
        case C_WAITTASK:
        case C_SKIPTASK: {
            char nm[0x18];
            snprintf(nm, sizeof nm, "%s", c->text + 9);
            char *arg = strchr(nm, ':');
            if (arg) *arg = 0;
            done = (task_node(h, nm) != 0) == (c->on != 0);
            if (!done && c->kind == C_SKIPTASK && el % 24 < 3) held = 1u << c->button;
            break;
        }
        case C_WAITBTL: {                                 /* real COMMAND readiness, or an enemy's defeat within the active encounter */
            uint32_t bc = task_node(h, "battle") ? h->read32(h->user, BATTLE_CONTEXT) : 0;
            if (c->on == 2) {
                uint32_t action = bc ? h->read32(h->user, bc + 0x174) : 0;
                if (action <= 0x100000u || action >= 0x02000000u) break;
                int found = 0;
                done = 1;
                for (unsigned n = 0; action && n < 64; n++) {
                    if (action <= 0x100000u || action >= 0x02000000u) { done = 0; break; }
                    uint32_t unit = h->read32(h->user, action + 0x30);
                    uint32_t data = unit > 0x100000u && unit < 0x02000000u ?
                        h->read32(h->user, unit + 0xA64) : 0;
                    if (data > 0x100000u && data < 0x02000000u) {
                        uint32_t identity = h->read32(h->user, data);
                        if ((identity & 4u) && (identity >> 16) == c->value) {
                            found = 1;
                            if (h->read32(h->user, data + 8) & 0xFFFFu) {
                                done = 0;
                                break;
                            }
                        }
                    }
                    action = h->read32(h->user, action + 0x450);
                }
                if (action || !found) done = 0; /* Missing enemies (including defeat cleanup) do not establish a kill. */
                break;
            }
            uint32_t st = bc ? h->read32(h->user, bc + 0x2FC) : 0, act = bc ? h->read32(h->user, bc + 0x104) : 0, fl = bc ? h->read32(h->user, bc + 0xC) : 0;
            uint32_t un = 0, action_state = 0, u = bc ? h->read32(h->user, bc + 0x178) : 0;
            if (act > 0x100000u && act < 0x02000000u) {
                un = h->read32(h->user, act + 0x30);
                action_state = h->read32(h->user, act + 0xC) & 0xFFFFu;
            }
            int inparty = 0;
            for (int n = 0; n < BTL_PARTY_MAX && u > 0x100000u && u < 0x02000000u; n++, u = h->read32(h->user, u + 0xA6C)) if (u == un) inparty = 1;
            int message_open = st == BTL_CMD_STATE && text_window_open(h, 0);
            w->btl_menu_frames = st == BTL_CMD_STATE && action_state == BTL_ACTION_COMMAND && inparty && !(fl & BTL_ACTION_RUNNING) && !message_open &&
                (!c->value || (h->read32(h->user, un + 0xA4) & 0xFFFFu) == c->value) ? w->btl_menu_frames + 1 : 0;
            done = w->btl_menu_frames >= 40;
            /* A scripted Help page can still own a state-8 turn. Advance only
             * its real text window, never a COMMAND/TARGET pane or an All-Out. */
            if (!done && el % 24 < 3) {
                if (st != BTL_CMD_STATE) held = 1u << P4W_CROSS;
                else if (action_state == BTL_ACTION_ALL_OUT_PROMPT) held = 1u << P4W_CIRCLE;
                else if (message_open && action_state != BTL_ACTION_COMMAND && action_state != BTL_ACTION_TARGET) held = 1u << P4W_CROSS;
            }
            break;
        }
        case C_BATTLE:
            battle_parse(c->text + 7, &w->bd);
            done = 1;
            break;
        case C_BATTLERULE:
            if (!w->bd.on) { fail(w, h, "battlerule: needs an active battle: driver"); return 0; }
            if (w->bd.nrules >= P4WALK_BATTLE_RULE_MAX) { fail(w, h, "more than 16 battlerule: lines"); return 0; }
            p4walk_battle_rule_parse(c->text + 11, &w->bd.rules[w->bd.nrules++]);
            done = 1;
            break;
        case C_BATTLETURN:
            if (!w->bd.on) { fail(w, h, "battleturn: needs an active battle: driver"); return 0; }
            if (w->bd.nturns >= P4WALK_BATTLE_TURN_MAX) { fail(w, h, "more than 256 battleturn: lines"); return 0; }
            p4walk_battle_turn_parse(c->text + 11, &w->bd.turns[w->bd.nturns++]);
            done = 1;
            break;
        case C_PLAN:
            w->plan = c->on;
            done = 1;
            break;
        case C_SKIPMSG: {
            /* an event / message: Cross every 24 frames until the player has been under control for 12 frames in a row */
            if (task_work(h, "rotate pc") && h->read32(h->user, EVENT_FLAG) == 0) w->control_frames++; else w->control_frames = 0;
            if (el == 0) w->control_frames = 0;
            done = w->control_frames >= 12;
            if (!done && el % 24 < 3) held = 1u << P4W_CROSS;
            break;
        }
        case C_WALK: {
            int floor_goal = c->on == 1 && (c->value == 0x10000u || c->value == 0x10001u);
            uint32_t goal_field = 0xFFFFFFFFu, goal_step = 0xFFFFFFFFu;
            if (floor_goal) {
                uint32_t root = h->read32(h->user, FIELD_ROOT_HANDLE);
                uint32_t work = root ? h->read32(h->user, root + TASK_WORK) : 0;
                if (work) goal_field = h->read32(h->user, work + 0x18);
                uint32_t task = h->read32(h->user, 0x00764334u);
                work = task ? h->read32(h->user, task + TASK_WORK) : 0;
                if (work) goal_step = h->read32(h->user, work + 4);
                /* Crossing the stair can finish the old goal before we reach its
                 * centre. Never reacquire the next floor's goal, even during its
                 * loading/entry event; ordinary battles keep these IDs unchanged. */
                if (w->goal_scoped &&
                    ((goal_field != 0xFFFFFFFFu && goal_field != w->goal_field) ||
                     (goal_step != 0xFFFFFFFFu && w->goal_step != 0xFFFFFFFFu && goal_step != w->goal_step))) {
                    done = 1;
                    break;
                }
            }
            if (!p4walk_player(h, &x, &z, &fx, &fz)) {
                if (w->battleskip) {                                   /* a battle, a scene or a load on the way: Cross through it, the clocks stop */
                    w->event_frames++;
                    w->best_frame++;
                    if (w->autoskip && w->event_frames % 24 < 3) held = 1u << P4W_CROSS;
                    if (w->event_frames > 20000) { fail(w, h, "the player is not under control"); return 0; }
                    break;
                }
                if (el > c->timeout) { fail(w, h, "the player is not under control"); return 0; }
                break;
            }
            float tx = c->a, tz = c->b;
            if (c->on == 1 && !nearest_unit(h, x, z, c->value, &tx, &tz)) {   /* none yet (after a battle the floor's units come back later) */
                if (el - w->event_frames > c->timeout) { fail(w, h, "no such field unit in the scene"); return 0; }
                w->best_frame = frame;
                break;
            }
            if (floor_goal && !w->goal_scoped && goal_field != 0xFFFFFFFFu) {
                w->goal_field = goal_field;
                w->goal_step = goal_step;
                w->goal_scoped = 1;
            }
            float dx = tx - x, dz = tz - z, d = sqrtf(dx * dx + dz * dz);
            if (d <= c->c) { done = 1; break; }
            if (task_node(h, "battle")) { w->best_frame++; w->event_frames++; break; }   /* a battle is starting: no game calls from here until it is over */
            if (h->read32(h->user, EVENT_FLAG) != 0) {          /* a message or an event: the player cannot move, the clocks stop */
                w->event_frames++;
                w->best_frame++;
                if (w->autoskip && w->event_frames % 24 < 3) held = 1u << P4W_CROSS;
                break;
            }
            float wx = tx, wz = tz, metric = d;
            g_nblocked = w->nblocked;
            memcpy(g_blocked, w->blocked, sizeof g_blocked);
            int edge[3] = { -1, -1, -1 };
            w->nav_active = 0;
            if (c->on || w->plan) {                              /* the game's own collision world first (the port), else the cell grid */
                uint32_t pm = p4walk_player_matrix(h);
                if (pm && nav_step(w, h, x, z, f32_of(h->read32(h->user, pm + 0x34)), tx, tz, &wx, &wz, &metric, frame)) { w->nav_active = 1; w->step_d = 4; }
            }
            if (w->nav_active) {
                if (w->nav_gate && w->gate_phase == 0 && w->gate_cycles < 12 && hypotf(w->nav_gx - x, w->nav_gz - z) < 280.0f) {   /* at the gate on the way: face it, Cross */
                    w->gate_cycles++;
                    if (h->note) {
                        char gm[96];
                        snprintf(gm, sizeof gm, "NAVGATE %.0f %.0f from %.0f %.0f", w->nav_gx, w->nav_gz, x, z);
                        h->note(h->user, gm);
                    }
                    w->gate_x = w->nav_gx;
                    w->gate_z = w->nav_gz;
                    w->gate_phase = 1;
                    w->gate_until = frame + 40;
                    w->best_frame = frame;
                    w->nav_replan = 0;
                }
            } else if (!(c->on || w->plan) || !plan_step(h, x, z, tx, tz, &wx, &wz, &metric, edge, !w->no_detour)) {
                metric = d;
                if (w->nblocked > 0 && w->resets < 3 && (c->on || w->plan)) { w->nblocked = 0; w->resets++; w->best = 1e30f; w->best_frame = frame; }   /* the shut edges cut the floor in two: forget them */
            } else { w->step_c = edge[0]; w->step_r = edge[1]; w->step_d = edge[2]; }
            w->detoured = w->nav_active ? 0 : g_detoured;
            w->aim_x = wx;
            w->aim_z = wz;
            if (w->gate_phase == 1) {                           /* face the gate: steer at it */
                held = steer(h, w->gate_x - x, w->gate_z - z);
                if (frame >= w->gate_until) { w->gate_phase = 2; w->gate_until = frame + 4; }
                break;
            }
            if (w->gate_phase == 2) {
                held = 1u << P4W_CROSS;
                if (frame >= w->gate_until) { w->gate_phase = 3; w->gate_until = frame + 200; }
                break;
            }
            if (w->gate_phase == 3) {                           /* the curtain opens */
                if (frame >= w->gate_until) { w->gate_phase = 0; w->best = 1e30f; w->best_frame = frame; }
                break;
            }
            if (frame < w->cross_until) { held = 1u << P4W_CROSS; break; }
            if (metric < w->best - STUCK_PROGRESS) { w->best = metric; w->best_frame = frame; }
            if (frame - w->best_frame > STUCK_FRAMES) {
                if (w->detoured && !w->no_detour) {             /* the middle of the room is no obstacle: aim at the exit through it */
                    w->no_detour = 1;
                    w->best = 1e30f;
                    w->best_frame = frame;
                    break;
                }
                if (w->door_tries < 3) {                        /* a gate or a curtain: face it, Cross opens it */
                    w->door_tries++;
                    if (w->step_d >= 0 && nearest_gate(h, x, z, 900.0f, &w->gate_x, &w->gate_z)) {   /* only where the planner walks: a generated floor */
                        w->gate_phase = 1;
                        w->gate_until = frame + 40;
                        w->best_frame = frame;
                        break;
                    }
                    w->cross_until = frame + 3;
                    w->best_frame = frame;
                    held = 1u << P4W_CROSS;
                    break;
                }
                if (w->nav_active && w->nav_forced < 60 && w->nav) {   /* a spot the sampled floor calls open is shut: remember it and plan another way */
                    float ax = w->nav_wx - x, az = w->nav_wz - z, al = hypotf(ax, az);
                    if (al > 1.0f) {
                        int cx = nav_ix(x + ax / al * 130.0f), cz = nav_iz(z + az / al * 130.0f);
                        for (int ddz = -1; ddz <= 1; ddz++)
                            for (int ddx = -1; ddx <= 1; ddx++)
                                if (cx + ddx >= 0 && cx + ddx < NAV_NX && cz + ddz >= 0 && cz + ddz < NAV_NZ) w->nav->forced[nav_idx(cx + ddx, cz + ddz)] = 1;
                    }
                    w->nav_forced++;
                    w->nav_replan = 0;
                    w->door_tries = 0;
                    w->best = 1e30f;
                    w->best_frame = frame;
                    break;
                }
                if (w->nblocked < 15 && w->step_d >= 0) {            /* the way the exit masks promise is shut: try another route */
                    w->blocked[w->nblocked][0] = w->step_c;
                    w->blocked[w->nblocked][1] = w->step_r;
                    w->blocked[w->nblocked][2] = w->step_d;
                    w->nblocked++;
                    w->door_tries = 0;
                    w->best = 1e30f;
                    w->best_frame = frame;
                    break;
                }
                fail(w, h, "stuck (no progress toward the point)");
                return 0;
            }
            if (el - w->event_frames > c->timeout) { fail(w, h, "timeout"); return 0; }
            held = steer(h, wx - x, wz - z);
            break;
        }
        case C_FACE: {
            if (!p4walk_player(h, &x, &z, &fx, &fz)) { if (el > c->timeout) { fail(w, h, "the player is not under control"); return 0; } break; }
            float tx, tz;
            face_dir(c->a, &tx, &tz);
            if (fx * tx + fz * tz > 0.978f) { done = 1; break; }
            if (el > 60) { fail(w, h, "could not turn to the angle"); return 0; }
            held = steer(h, tx, tz);
            break;
        }
        }
        if (c->kind >= C_WAITFIELD && c->kind != C_BATTLESKIP && c->kind != C_PLAN && c->kind != C_BATTLE && c->kind != C_EXIT && c->kind != C_LOG && c->kind != C_PUSH && !done && el > c->timeout) { fail(w, h, "timeout"); return 0; }
        if (!done) return held;
        w->at++;
        w->started = 0;
    }
    return 0;
}
