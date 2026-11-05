/* Opt-in native battle test session. No host pointer is stored in guest data. */
#ifndef P4PC_BATTLE_SIM_H
#define P4PC_BATTLE_SIM_H
#include <stdint.h>
#include <stddef.h>

/* Set only by the explicit loader CLI before the game starts. Ordinary play
 * has inactive update guards and performs no simulator observations. */
extern int p4_battle_sim_active;
extern int p4_battle_sim_renderless;
/* Return 0 for an ordinary launch, 1 for a configured session, -1 for an
 * invalid explicit request. elf receives the unchanged positional ELF path. */
int p4_battle_sim_cli(int argc, char **argv, const char **elf);
/* 1 when the configured session is a declared mod cohort (P4PC_BATTLE_SIM_MODS):
 * the loader then admits the selected packages exactly as an ordinary launch does. */
int p4_battle_sim_mod_cohort(void);
void p4_battle_sim_before_update(void);
void p4_battle_sim_after_update(int completed);
uint32_t p4_battle_sim_update_count(void);
/* Complete canonical NTSC-U native image, not an arbitrary tagged patch.
 * Accepts the native compact variant only with its three field sections absent. */
int p4_battle_save_valid(const uint8_t *image, uint32_t size);
/* Shared real encounter construction, used by the existing harness too.
 * Caller owns preparation, boot/field readiness and immutable entry state. */
int p4_battle_encounter_start(uint32_t encounter);
/* Rendering sink initialization: leaves game-side draw/effect callbacks intact. */
int p4_battle_sim_graphics_open(void);
/* Native UI policy adapter. Negative input result preserves the original
 * handler; otherwise the real UI consumes its normal input-code enum.
 * These are tool entrypoints, not a guest-memory/script-write interface. */
int p4_battle_policy_begin(uint32_t mode, uint32_t latency);
int p4_battle_policy_input(uint32_t selection);
void p4_battle_policy_target(uint32_t action);
void p4_battle_policy_before_update(void);
const char *p4_battle_policy_error(void);
void p4_battle_sim_command_ready(uint32_t action);
/* Post-battle skill-offer prompt, read from the live shdPersonaSkillSelectSeq
 * task, its persona panel and the current message window. */
typedef struct P4BattleSkillPrompt {
    int32_t state;               /* skill-replace task state (work +0x08) */
    int32_t list_active;         /* panel accepts list input (flags & 0x800) */
    int32_t list_cursor;         /* -1 inactive, 0..7 known skills, 8 the pending skill */
    int32_t pending;             /* offered skills still queued */
    int32_t replacement_pending; /* the chosen row is the offered skill itself */
    int32_t row_waiting;         /* a message text row waits for input */
    int32_t choice_active;       /* the two-entry Yes/No choice polls input */
    int32_t choice_cursor;       /* 0 Yes, 1 No */
} P4BattleSkillPrompt;
/* P4PC_PAD_* bits that decline the offer through the native UI, or 0 to wait. */
unsigned p4_battle_skill_prompt_buttons(const P4BattleSkillPrompt *prompt);
#endif
