/* Game-unit mod seams: scalar state only, no native descriptor pointers.
 * Kept separate from mod_hooks.h so the guest rewriter never sees host records. */
#ifndef P4PC_MOD_ENTRY_H
#define P4PC_MOD_ENTRY_H
#include <stdint.h>
#include <limits.h>

typedef struct P4ModEntry {
    volatile uint8_t enabled;
    uint8_t restore;
} P4ModEntry;
enum { P4_MOD_INACTIVE, P4_MOD_ACTIVE, P4_MOD_BYPASS };

/* Scheduler seam; no event records/callback pointers enter guest ABI census. */
#define P4_MOD_FRAME_START_BIT 1u
#define P4_MOD_EXIT_BIT (1u << 10)
extern uint32_t p4_mod_event_mask;
/* Queued game-thread work for the next frame boundary; zero without mods. */
enum { P4_MOD_SAFE_PUBLISHED = 1u, P4_MOD_SAFE_EXIT = 2u, P4_MOD_SAFE_SERVICES = 4u, P4_MOD_SAFE_PARTY = 8u,
    P4_MOD_SAFE_SEMANTIC = 16u, P4_MOD_SAFE_PRESENT = 32u,
    P4_MOD_SAFE_FORGE = 64u, P4_MOD_SAFE_EQUIP = 128u };
extern uint32_t p4_mod_safe_point;
void p4_mod_event_frame(uint32_t frame);
/* Window-close request: 1 when an exit subscriber defers the quit to the next
 * frame boundary (best effort; finish runs after the event, then exit(0)),
 * 0 when the caller must exit as before. */
int p4_mod_event_quit_request(void (*finish)(void));

/* Operation authorizer seams (include/p4pc/mod_authorize.h). A seam tests the
 * mask first; zero without authorizers. The call returns 1 allow, 0 deny. */
#define P4_MOD_AUTH_FUSION_BIT 1u
#define P4_MOD_AUTH_ENCOUNTER_BIT 2u
#define P4_MOD_AUTH_BATTLE_PROTECTION_ITEM_BIT 4u
int p4_mod_authorize_battle_protection_item(int character, int item, int element);
#define P4_MOD_AUTH_WEAPON_FUSION_BIT 8u
extern uint32_t p4_mod_authorize_mask;
int p4_mod_authorize_fusion(int kind, int materials);
/* Battle formation seam: the lineup's IDs one per byte in battle order. Returns
 * 0xFFFFFFFF to keep it, else (count << 24) | projected IDs, one per nibble. */
extern uint32_t p4_mod_formation_mask;
uint32_t p4_mod_formation_project(uint32_t packedIds, uint32_t count);
/* A projected lineup without Yu handed his leader role to this fighter (logged). */
void p4_mod_formation_leader(uint32_t unitId);
/* Velvet Room command seam (include/p4pc/mod_velvet.h), for the menu
 * dispatcher at the idle root (f0 0, f1 0x18). The mask is nonzero while a
 * command is registered and enabled. offer is 1 when its hint and Square entry
 * show (asked once per idle streak; rearm, called while the root is not idle,
 * makes the next idle frame ask again); hint_text is the guest address of the
 * hint's RAW text. open_page creates the Config child under the Velvet task
 * (its guest address) and hosts the command's page in it (0 on success);
 * page_step is 1 while that page runs, then destroys the child and returns 0. */
extern uint32_t p4_mod_velvet_mask;
int p4_mod_velvet_offer(uint32_t task);
void p4_mod_velvet_rearm(void);
uint32_t p4_mod_velvet_hint_text(void);
int p4_mod_velvet_open_page(uint32_t task);
int p4_mod_velvet_page_step(void);

/* Compile-time ABI checks only. A typedef collision must not silently turn a
 * pointer/aggregate into an integer bridge. No expression is evaluated. */
#define P4_MOD_INTEGER_ABI(value, bytes) \
    (sizeof(value) == (bytes) && \
     (__builtin_classify_type(value) == 1 || __builtin_classify_type(value) == 3))
#define P4_MOD_SIGNED_ABI(value) _Generic((value), char: (CHAR_MIN < 0), \
    signed char: 1, short: 1, int: 1, long: 1, long long: 1, default: 0)
#endif
