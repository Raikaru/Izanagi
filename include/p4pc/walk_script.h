/* Waypoint scripts for scripted play: one text file drives retail (the PCSX2 hook's --walk FILE) and the port
 * (P4PC_WALK_SCRIPT=FILE, src/platform/pad/pad_sdl.c) with the same meaning, because both read the same guest memory.
 *
 * One command per line, `#` starts a comment, blank lines are ignored. A command runs until it is done, then the next starts (on the
 * same frame); the script is done after the last one.
 * Directives retain up to 511 bytes, including multi-character battle plans and their trailing options.
 *
 *   walk:X,Z[,R]       steer the field player to the world point (X, Z) with the d-pad until it is within R units (default 40).
 *                      The camera is read each frame, so the direction pressed is whatever moves the player toward the point on
 *                      screen. Fails when the distance has not shrunk by 3 units in 150 frames (stuck) or after the timeout.
 *   face:DEG           turn the player toward the world yaw DEG (degrees, atan2(at.x, at.z): 0 is +Z, 90 is +X) by pressing the
 *                      matching direction until the facing is within 12 degrees (a few frames of movement).
 *   press:BUTTON[:N]   hold BUTTON (up down left right cross circle triangle square start select l1 r1 l2 r2 l3 r3) for N frames
 *                      (default 3, at least 3), then release for 3 frames.
 *   wait:N             do nothing for N frames.
 *   waitfield:F:R      wait until the field root's ids are major F, minor R (decimal).
 *   waitstep:N         wait for the live dungeon-step table index (FldStepIndex), independent of field/room (Castle 3F is step 8).
 *   waitstate:ADDR==V  wait until the u32 at guest address ADDR equals V (ADDR and V decimal or 0x hex; `!=` waits for a change).
 *   waitstate16:ADDR==V the same predicate for a u16 at an even guest address; adjacent HP/SP or roster fields are not part of the comparison.
 *   waitcontrol:on|off wait until the player is under control (the task `rotate pc` exists) or is not (an event, a movie, a load).
 *   waitmsg:open|closed wait for the event flag at 0x00764604 (gp-0x4AEC): 1 while a message window or an event holds the player's input
 *                      (every event scene, "Help" boxes, NPC talk), 0 in free roam.
 *   skipmsg            press Cross every 24 frames (a message, an event, a prompt's default answer) until the event flag is clear and
 *                      the player is under control for 12 frames in a row.
 *   skip:N             press Cross every 24 frames for N frames whether or not the player is free (to stop at a story point by time).
 *   skipfield:F:R      press Cross every 24 frames (events, messages, a shop's first screen) until the field root's ids are major F, minor R.
 *   skipevtmsg:ID:ROW  Cross every 24 frames only through the active EVT's non-choice text until its zero-based message ID and row match.
 *                      Then emit no input, even while that row is still typing. Reads the EVT's own message handle/window; no age or game-state writes.
 *   waitchoice         advance only actual non-choice text until a native selection is ready, without navigating or confirming it.
 *                      Release input while a choice prepares, even if another caption is live; reads the shared BF/EVT message-window list.
 *                      Field BF text requires event input ownership and no battle task; battle Help keeps its strict text predicate.
 *   choose:N           Cross through dialogue until a message-window choice is ready, move to its Nth visible enabled answer (1-based),
 *                      and confirm it. Waits for the selection to close before continuing; fails if N exceeds that menu's answer count.
 *                      Reads the game's message-window list and cursor, so non-default answers do not depend on scene/frame timing.
 *   walk:shadow[,R]    walk toward the nearest shadow or treasure unit (scene list 0x1, id 0x460 and up), re-aimed every frame;
 *   battle:first=a,b;plan=a,b;skip=N;delay=N|off   a battle driver, the logic of the port's P4PC_BATTLE_DRIVER: while the `battle` task exists the
 *                      script's own pad output is replaced by it. Buttons (up down left right cross circle triangle square l1 r1 ...) are pressed 14
 *                      frames apart when a party member's command menu has been up `delay` (20) frames (battle state 8, action state 5 COMMAND,
 *                      the actor in the party group, no action running, after `skip` frames); `first` is a character's first command in a fight, `plan` every later
 *                      one (default: plan); first1=..., through first8=..., override `first` for that character ID (1 = Yu, 2 = Yosuke),
 *                      independently of the party-list order. plan1=..., through plan8=..., override later `plan` commands for that native character ID;
 *                      `first`/`firstN` precedence is unchanged, and charge Guard takes priority over both first and repeat inputs. Unlisted IDs use `plan`.
 *                      Cross every 24 frames at any other moment, except in Shuffle Time (the task `battle_shuffle`):
 *                      a flipped card cannot be flipped again, so every 45 frames the driver alternates a cursor move (right along the row, down at
 *                      its end) with Cross. Units are walker frames. In the results phase (battle state 16) a level-up that offers a ninth skill
 *                      asks which to forget, and its Yes/No defaults to No, so Cross alone would loop: while that task (shdPersonaSkillSelectSeq)
 *                      reads input the driver declines the offer instead, Left to the offered skill (list slot 8), Cross, then Yes. A known skill
 *                      is never forgotten (the battle simulator's result_prompt.c makes the same choice).
 *                      A key may be skill:ID (native skill ID, decimal or hex) with the acting character's Skill list open, or item:ID (native
 *                      item ID 0x301..0x3FF) with the Item list open: it presses Down or Up (the shorter way around the wrapping list; Down on a
 *                      tie) from the row the list opens on to that entry, 14 frames apart, then the next key follows (e.g.
 *                      circle,down,cross,skill:195,cross). btlSelectionRefreshList (00207140) builds the lists: the character's current persona
 *                      skills in record order that are active (ID below 0x1B8 with skill-table flags01 bit 2), and the held items (D_0079757A
 *                      count) in ID order whose granted skill (item metadata +0x24) is active. With the cursor-memory option (flag 60) a list
 *                      opens on its remembered row (action +0x3F0: pair 1 Skill, pair 2 Item), row 0 when that is past the list's end
 *                      (btlSelectionUpdateWindow 00202D20). An entry missing from the list fails the walk. Reads only; the cursor still moves by pad input.
 *                      Optional persona=modal gives shdPersonaStatusDraw Cross priority over its underlying card grid; the legacy cadence is the default.
 *                      Optional guard=ID (native enemy ID, decimal or hex) replaces a fresh command with Circle/Up/Cross while a living matching enemy
 *                      has native Power/Mind Charge active. It never interrupts an armed macro or consumes a character's first plan.
 *                      Optional guard=ID:PC,PC limits that override to the listed native party IDs 1..8; guard=259:2,3,4 leaves Yu's normal heal plan active.
 *                      Without a party list, guard=ID still applies to every party member. The filter does not depend on any companion remaining alive.
 *                      Guard changes the remembered wheel: use an explicit repeat such as circle,down,cross,cross,cross to reopen the selected Skill.
 *                      An armed plan continues through TARGET state 6 for the same actor; confirming to action 15 ends it.
 *                      Optional target1=ID through target8=ID bind that native party ID's TARGET confirmations to a living native enemy ID (decimal/hex).
 *                      Fresh Right input follows the game's target list until it matches; all-target skills may confirm when the enemy is included.
 *                      Dead/absent/unreachable targets fail the walk instead of confirming another enemy. No game state is written; omitted bindings are unchanged.
 *                      Remove/rebind the option before ally-targeted healing or after that enemy dies. Guard overrides do not use target bindings.
 *   battlerule:COND[@ID,ID]=KEYS  appends an ordered conditional rule to the active battle: driver (up to 16; the next battle: line or
 *                      battle:off clears them). At each fresh command after a charge Guard (guard=ID) has not claimed it, the first rule whose
 *                      condition holds for the acting character supplies the command's buttons instead of first/plan; otherwise those stay
 *                      the character's default. Like Guard, a rule does not consume a character's first plan. @ID,ID limits the rule
 *                      to those native character IDs 1..8. Conditions read native unit records only (DatUnit HP +8, SP +0xA, ailments `bad`
 *                      +0xC (datCalcChkBadStatus), charge flags +0x14, signed buff-level nibbles +0x1C):
 *                        dead        a party member is down (HP 0): the first in party order is the bound ally (a Revival Bead, a revive skill)
 *                        hp<N        a living party member has HP below N: the lowest (party order breaks ties) is the bound ally
 *                        sp<N        the acting character's own SP is below N (an SP item before a skill macro would fail)
 *                        charge:ID   a living enemy with that native ID has Power/Mind Charge (DatUnit +0x14 & 0xC0000)
 *                        status:M    the acting character's own ailments have a bit of mask M (0x8: silence after Foolish Whisper)
 *                        buff:M      the acting character's own buff levels match a datCalcHasBuffMask bit of M: 0x1/0x2 attack up/down,
 *                                    0x4/0x8, 0x10/0x20 the next two stats, 0x40/0x80 defense up/down (0x80: Marakunda), 0x100/0x200
 *                                    the fifth; any selected bit matches (stat levels only: 0x1..0x200)
 *                        nobuff:M    no bit of M matches the acting character's own buff levels (re-apply a -kaja once it lapses)
 *                        foebuff:M   a living enemy's buff levels match a bit of M (Dekaja after its Heat Riser): the first is the bound enemy
 *                        afflicted:M a living party member's ailments have a bit of mask M (0x4: fear after Evil Smile): the first in party
 *                                    order is the bound ally (Me Patra or Amrita cure every ally at once)
 *                        enemy:ID    a living enemy with that native ID is in the fight: it is the bound enemy (focus a healer; Escape from 138)
 *                        weak:E      a living enemy is weak to element E (0 physical, 1 fire, 2 ice, 3 elec, 4 wind, 5 almighty, 6 light,
 *                                    7 dark): the first in the enemy list is the bound enemy
 *                        hits:E      a living enemy takes element E at full strength (not resisted, nulled, repelled or absorbed): the
 *                                    first such enemy is the bound enemy
 *                      Element affinities are datCalcGetAffinity's (0023A6B0) for enemies: the affinity row (iGpffffb3c8, by DatUnit
 *                      id2), the enemy's resist/null/repel/absorb passives (its skill row), Guard removing a weakness, and the Break and
 *                      Wall buff levels, so a Wall or a Break changes the answer the turn it lands.
 *                      A bound ally steers the rule's TARGET confirmations like targetN= does for enemies: fresh Right input until the cursor
 *                      is on that ally (an all-ally skill confirms at once); an ally missing from a party target list fails the walk. A pane
 *                      without party candidates (the macro fell back to Attack) confirms as usual. A bound enemy steers them the same way.
 *                      Rules without a bound unit keep
 *                      targetN= enemy bindings. The walker notes each rule it applies. A rule whose skill:/item: key names an entry
 *                      missing from the acting character's list (a spent Revival Bead, a skill not learned) does not apply, so a later
 *                      rule can name a fallback (dead@1 with item:0x30F, then dead@1 with item:0x310).
 *   battleturn:KEYS | battleturn:COND=KEYS  appends one command to the active battle: driver's tape (up to 256; the next battle: line or
 *                      battle:off clears it). The Nth line supplies the Nth fresh party command of the fight, whoever acts, ahead of guard=,
 *                      battlerule:, first and plan. COND binds the command's TARGET confirmations like a rule's bound unit: enemy:ID (a
 *                      living enemy with that native ID), ally:ID (a living party member) or dead:ID (a downed one, for a revive); an
 *                      absent bound target fails the walk. A command that does not take (the 600-frame re-arm) retries the same line. Past
 *                      the last line the host's battle_turn callback may supply more (a search driver), else the driver's normal input
 *                      continues. A fixed tape found offline replays the same presses at the same frames in a gate.
 *   waittask:NAME[:off] wait until the engine task NAME exists (:off: is gone), e.g. Y_FclShopMainProc for the shop menu (the same in retail and the port)
 *   skiptask:NAME[:off][:BUTTON] press BUTTON (default Cross) every 24 frames until task NAME exists (:off: is gone);
 *                      stops before pressing through its menu. E.g. skiptask:cmpMain:triangle opens camp after party-gather finishes.
 *   waitbtl:menu[:N]   wait for 40 ready command-menu frames; optional character ID N is 1–8 (1 = Yu, 2 = Yosuke), not a party-list slot.
 *                      Omit N to accept any party member. Advance actual non-choice Help text and decline an earlier All-Out.
 *                      Each wait counts fresh action-state-5 COMMAND frames without a text overlay, not AI/STANDBY; never confirms COMMAND/TARGET.
 *                      After issuing an action, let it leave state 5 before waiting again.
 *   waitbtl:dead:ID    wait until matching native enemy records exist and all have zero u16 HP (ID decimal or hex).
 *                      An absent/unlinked enemy is not evidence of defeat. Observes duplicate IDs, sends no input, and binds no heap address.
 *   walk:goal[,R]      the live 0x37FF upstairs-quad centre on a generated floor; fixed fields without it use their placed goal marker
 *                      (scene list 0xE). Routed like walk:unit; a generated-floor caller can press Cross without a seed-specific push.
 *                      Bound to the original field/room and dungeon step: a stair transition completes it even before control returns;
 *                      it never reacquires the next floor's goal. Ordinary battles keep the same field/step and only pause the walk.
 *   walk:entry[,R]     the live 0x37FE downstairs-quad centre; no fixed-field marker fallback.
 *                      Routed and bound to the current field/step like walk:goal, so on-foot returns do not depend on the board seed.
 *   walk:unit:ID[,R]   the unit with that id (0x464 ... on a dungeon floor; the dump's 'CHECK unit' lines list them).
 *   plan:on|off        route walk:X,Z over the floor's cell grid as well (default off: walk:shadow/unit always are, other fields' data
 *                      must not steer a plain walk)
 *                      On a dungeon floor a planned walk: is routed over the 1200-unit cell grid (exits and pieces), a standstill presses Cross
 *                      (gates, curtains) up to three times, then fails.
 *                      In the port (P4WalkHost.nav_blocked / nav_ground = the game's own FldFrame_CollideSphereSelected and FldFrame_RaycastField) a planned
 *                      walk is routed over the field's real collision world instead: the floor is sampled on a 100-unit grid (a node is walkable when the
 *                      ground ray finds a floor near the player's height and a 62-unit sphere there touches nothing), A* over it with a surcharge next
 *                      to walls and near other Shadows, the path pulled straight over line of sight; a closed gate or curtain (a collidable list 0xA
 *                      object) is crossable at a cost and is faced and Crossed when the way reaches it; a spot where the walk stalls is remembered
 *                      as shut. Works on any floor (generated boards, Heaven's bridges, Yukiko's Castle). Retail's host has no such tests: it keeps the
 *                      cell grid, or walks the waypoint list the port logged (`NAVWP x z`, `NAVGATE x z` notes).
 *   walk:obj:ID[,R]    the field object (scene list 0xA: gates, chests) with that id (CHECK lists them), planned like walk:unit
 *   navprobe           log how the game's collision tests see the player's spot (sphere heights and radii, eight directions)
 *   navdump:FILE       write the sampled floor around the player as a PGM (255 walkable, 140 near a wall, 70 wall, 0 no floor, 200 the player)
 *   push:X,Z,N         steer at the point for N frames, no stuck check (to face a wall or a door before Cross).
 *                      The duration expires even if an automatic event takes player control during the push.
 *   log                one line with position, event flag, field and room ids
 *   battleskip:on|off  a walk: whose player is not under control (a battle, a scene, a load) presses Cross every 24 frames instead of waiting (default off)
 *   autoskip:on|off    while an event or message holds the input during a walk or face, press Cross every 24 frames (default on) and do not
 *                      count those frames toward stuck or timeout.
 *   timeout:N          the timeout for the following commands in frames (default 3000); a wait that times out fails the script.
 *   exit               stop (done).
 *
 * The player is the matrix pointed at by the work block of the task `rotate pc` (word +0x48): position at +0x30 (x) / +0x38 (z), facing
 * (the matrix's "at") at +0x20 / +0x28. The camera is the object at the word 0x00764B70 (its frame at +4, matrix at frame + 0x10: right at
 * +0x00 / +0x08, at at +0x20 / +0x28); the stick moves the player along `at` for up and along -right for right (RenderWare cameras'
 * right points to the screen's left).
 *
 * The caller steps it once per frame: p4walk_frame(w, host, frame) returns the buttons held (a mask of P4W_* bits) and the status. */
#ifndef P4PC_WALK_SCRIPT_H
#define P4PC_WALK_SCRIPT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    P4W_UP, P4W_DOWN, P4W_LEFT, P4W_RIGHT, P4W_CROSS, P4W_CIRCLE, P4W_TRIANGLE, P4W_SQUARE,
    P4W_START, P4W_SELECT, P4W_L1, P4W_R1, P4W_L2, P4W_R2, P4W_L3, P4W_R3, P4W_BUTTON_COUNT
} P4WalkButton;

typedef struct P4WalkHost {
    uint32_t (*read32)(void *user, uint32_t guest_addr);
    void (*note)(void *user, const char *message);
    void *user;
    /* Optional (the port only; retail's host leaves them NULL and the planner keeps to the cell grid): the game's own collision tests. blocked: 1 when a
     * sphere of `radius` at (x, y, z) touches the field's collision world. ground: 1 and the floor height when the vertical line from y_hi down to y_lo
     * meets the field's ground. */
    int (*nav_blocked)(void *user, float x, float y, float z, float radius);
    int (*nav_ground)(void *user, float x, float z, float y_hi, float y_lo, float *y);
    /* Optional: the battle driver's command N (0-based) once its battleturn: lines are spent. Write `KEYS` or `COND=KEYS` (battleturn: syntax) to
     * `line` and return 1, or return 0 for the driver's normal input. Called synchronously when that command is armed. */
    int (*battle_turn)(void *user, unsigned n, char *line, size_t size);
    /* Optional (the port only): the game's active-skill predicate (p4_game_skill_active), so the driver's Skill and Item lists keep
     * an added skill exactly as btlSelectionRefreshList (00207140) does. NULL keeps the retail split: an ID below 0x1B8. */
    int (*skill_active)(void *user, unsigned id);
    /* Optional (the port only): whether an enemy kind has a passive skill granted (p4_game_passive_enemy_has), so the planner's
     * affinity model sees a granted resist/null/repel/absorb passive as datCalcGetAffinity does. NULL: nothing granted. */
    int (*passive_granted)(void *user, unsigned enemy, unsigned skill);
} P4WalkHost;

typedef struct P4Walk P4Walk;

enum { P4WALK_RUNNING = 0, P4WALK_DONE = 1, P4WALK_FAILED = -1 };

/* Parses the script text; NULL (and a message in err) on a malformed line. */
P4Walk *p4walk_parse(const char *text, char *err, size_t errsz);
void p4walk_free(P4Walk *w);
/* One frame; `frame` must increase by one between calls. Returns the held buttons (bit 1 << P4WalkButton). */
uint32_t p4walk_frame(P4Walk *w, const P4WalkHost *host, uint32_t frame);
/* The same, driven by the game's frame counter (guest 0x00764290, +1 per game frame): call it every vsync/flip; it steps once per counter change and
 * returns the held buttons. All wait:/press:N/skip:/timeout: counts are game frames, identical in retail and the port. */
uint32_t p4walk_tick(P4Walk *w, const P4WalkHost *host);
int p4walk_status(const P4Walk *w);
/* One call per frame: 60 frames after the field/room ids change, one note line per interaction trigger of the loaded field (area quads with their
 * event-table entries, field objects, shadows). Same code for p4hook --probe-checks and the port's P4PC_DUMP_CHECKS=1. */
void p4walk_checks_frame(const P4WalkHost *host, uint32_t frame);
/* Reads the player's position into x/z and facing (unit vector) into fx/fz; 0 when there is no player under control. */
int p4walk_player(const P4WalkHost *host, float *x, float *z, float *fx, float *fz);
/* The guest address of the matrix p4walk_player reads (the `rotate pc` task's model), 0 when there is none. */
uint32_t p4walk_player_matrix(const P4WalkHost *host);
/* Cross-target dungeon digest (the port's P4PC_DUNGEON_DIGEST, tests/dungeon_digest_gate.sh). Call once per frame with a caller-owned rolling hash
 * (start 2166136261): every frame it folds the player's matrix and the game's additive random generator into *roll; when frame % every == 0 it
 * writes one row "dungeon digest flip N ..." into line and returns 1. Each component is a separate FNV-1a hash of guest data (never of a pointer
 * word): field ids, dungeon step and floor seed, the floor's cell grid, the additive generator, RpRandom's Mersenne Twister, the player's
 * matrix, the floor's units (ids and model matrices) and the battle state, so the first differing row of two targets names the flip and the
 * kind of state that split. */
int p4walk_dungeon_digest(const P4WalkHost *host, uint32_t frame, uint32_t every, uint32_t *roll, char *line, size_t n);

/* battlerule: (above), exposed so its selection is a testable pure function of a battle snapshot. */
#define P4WALK_BATTLE_RULE_MAX 16
#define P4WALK_BATTLE_TURN_MAX 256
#define P4WALK_BATTLE_KEYS_MAX 32
#define P4WALK_KEY_SKILL 0x80000000u               /* key flag: skill:ID, the native skill ID in the low 16 bits */
#define P4WALK_KEY_ITEM 0x40000000u                /* key flag: item:ID, the native item ID in the low 16 bits */
enum { P4WALK_RULE_DEAD = 1, P4WALK_RULE_HP_BELOW, P4WALK_RULE_SP_BELOW, P4WALK_RULE_CHARGE, P4WALK_RULE_STATUS, P4WALK_RULE_BUFF,
       P4WALK_RULE_AFFLICTED, P4WALK_RULE_ENEMY, P4WALK_RULE_WEAK, P4WALK_RULE_HITS, P4WALK_RULE_NOBUFF, P4WALK_RULE_FOEBUFF,
       P4WALK_RULE_ALLY };
typedef struct {
    int kind;                                   /* P4WALK_RULE_* */
    uint32_t value;                             /* hp<N/sp<N: N; charge/enemy: the enemy ID; status/afflicted/buff: the mask; weak/hits: E */
    uint32_t actors;                            /* bit ID-1 per native character that may use the rule; 0: every one */
    unsigned keys[P4WALK_BATTLE_KEYS_MAX];      /* 1 << P4WalkButton per press */
    int key_count;
} P4WalkBattleRule;
/* Parses a battleturn: line, "KEYS" or "COND=KEYS": kind 0 (no binding), P4WALK_RULE_ENEMY, P4WALK_RULE_ALLY or P4WALK_RULE_DEAD with the
 * native ID in value. 1 on success, 0 on a malformed line. */
int p4walk_battle_turn_parse(const char *text, P4WalkBattleRule *out);
typedef struct {
    uint32_t id;                                /* native character or enemy ID */
    uint32_t hp;                                /* DatUnit +8, u16 */
    uint32_t sp;                                /* DatUnit +0xA, u16 */
    uint32_t bad;                               /* DatUnit +0xC: ailments (datCalcChkBadStatus: 0x4 fear, 0x8 silence) */
    uint32_t status;                            /* DatUnit +0x14: charge flags (0xC0000 Power/Mind Charge) */
    uint8_t buffs[8];                           /* DatUnit +0x1C: signed nibbles, index 2k low and 2k+1 high, values above 7 minus 15 */
    uint16_t weak, block, resist;               /* enemies: bit E per element 0..7 of datCalcGetAffinity's weak, null/repel/absorb
                                                 * and resist flags (p4walk_enemy_affinity) */
} P4WalkBattleUnit;
typedef struct {
    uint32_t actor;                             /* the acting character's native ID */
    const P4WalkBattleUnit *party;              /* the party chain, in its order */
    int party_count;
    const P4WalkBattleUnit *enemies;
    int enemy_count;
    uint32_t unusable;                          /* bit r: rule r names a skill:/item: entry missing from the actor's list (skipped) */
} P4WalkBattleView;
/* Parses "COND[@ID,ID]=KEYS"; 1 on success, 0 on a malformed rule. */
int p4walk_battle_rule_parse(const char *text, P4WalkBattleRule *out);
/* The index of the first rule that applies to view->actor, or -1. *ally gets the bound ally's native ID and *enemy the bound enemy's
 * (0: none). */
int p4walk_battle_rule_select(const P4WalkBattleRule *rules, int count, const P4WalkBattleView *view, uint32_t *ally, uint32_t *enemy);
/* datCalcGetAffinity (0023A6B0) for an enemy element: `raw` is its affinity-row halfword, `passives` the resist (1), null (2), repel (4)
 * and absorb (8) passives it holds for that element, then Guard, a Break and a Wall. Flags: 0x8000000 weak, 0x1000000 null, 0x2000000
 * repel, 0x4000000 absorb, 0x10000000 resist; the low halfword is the damage percent. */
uint32_t p4walk_enemy_affinity(unsigned raw, unsigned passives, int guarding, int broken, int walled);
/* skill:ID / item:ID navigation: the signed Down (+) / Up (-) presses from row `cursor` of a wrapping list of `count` IDs to `id`, the shorter way
 * (Down on a tie). 1 on success; 0 when `id` is not listed or `cursor` is outside the list. */
int p4walk_battle_list_moves(const uint16_t *list, int count, int cursor, unsigned id, int *moves);
/* The results screen's skill-offer prompt (shdPersonaSkillSelectSeq), as the battle driver reads it. */
typedef struct {
    int32_t state;                              /* skill-replace task state (work +8) */
    int32_t list_active;                        /* panel accepts list input (flags & 0x800) */
    int32_t list_cursor;                        /* -1 inactive, 0..7 known skills, 8 the offered skill */
    int32_t pending;                            /* offered skills still queued */
    int32_t replacement_pending;                /* the chosen row is the offered skill itself */
    int32_t row_waiting;                        /* a message text row waits for input */
    int32_t choice_active;                      /* the Yes/No choice polls input */
    int32_t choice_cursor;                      /* 0 Yes, 1 No */
} P4WalkSkillPrompt;
/* The P4W_* buttons that decline the offer (pick the offered skill, answer Yes), 0 to wait. A known skill is never forgotten. */
unsigned p4walk_skill_prompt_buttons(const P4WalkSkillPrompt *prompt);

#endif
