#ifndef P4PC_GAME_KNOWLEDGE_H
#define P4PC_GAME_KNOWLEDGE_H
#include "p4pc/game_tables.h"

/* Expanded enemy knowledge (Analyze / known affinities) for authored aliases.
 * The enemy's logical id, native id2 selector, semantic profile and knowledge
 * identity are not interchangeable. Aliases may deliberately share a knowledge
 * identity, but only within the same profile. Every key comes from an actual
 * enemy alias and either its side affinity row or, for a status-channel side
 * ailment that declares default-from and has no row, that side ailment; there
 * is no other profile/element fallback. Unmodified retail pairs keep the
 * retail bitmap (game_knowledge_native.h), except what it cannot hold (below). */
typedef struct P4KnowledgeKey {
    uint32_t knowledge_id, profile_id, element_id, channel_id;
} P4KnowledgeKey;
typedef struct P4GameKnowledge P4GameKnowledge;

/* Knowledge with no side key, for retail enemies (below 336): natural status
 * pairs (retail leaves 8..15 on the status channel) that the original resolver
 * answers without a side row, one bit per slot (bit slot - 8), the counterpart
 * of the retail Analyze bitmap's eight element bits that end at leaf 7; and,
 * with no alias for the enemy's native profile, each defaulted side ailment's
 * status pair, one bit per side-ailment index (p4_game_tables_side_ailment_at). */
#define P4_KNOWLEDGE_RETAIL_ENEMIES 336u
#define P4_KNOWLEDGE_STATUS_FIRST 8u
#define P4_KNOWLEDGE_STATUS_SLOTS 8u
typedef struct P4KnowledgeRetail {
    uint8_t status[P4_KNOWLEDGE_RETAIL_ENEMIES];
    uint16_t side[P4_KNOWLEDGE_RETAIL_ENEMIES];
} P4KnowledgeRetail;

/* Build from a frozen snapshot that must outlive this state; a snapshot
 * without enemy side rows still holds the retail status bits. Queries and
 * observations never allocate. */
int p4_game_knowledge_create(const P4GameTables *tables, P4GameKnowledge **state);
void p4_game_knowledge_destroy(P4GameKnowledge *state);
void p4_game_knowledge_clear(P4GameKnowledge *state);
size_t p4_game_knowledge_count(const P4GameKnowledge *state);
const P4GameTables *p4_game_knowledge_snapshot(const P4GameKnowledge *state);
int p4_game_knowledge_test(const P4GameKnowledge *state, const P4KnowledgeKey *key, int *known);
int p4_game_knowledge_observe(P4GameKnowledge *state, const P4KnowledgeKey *key);
/* A retail enemy below 336 and a status slot 8..15, else INVALID. */
int p4_game_knowledge_status_test(const P4GameKnowledge *state, uint32_t enemy_id, uint32_t slot, int *known);
int p4_game_knowledge_status_observe(P4GameKnowledge *state, uint32_t enemy_id, uint32_t slot);
/* A retail enemy below 336 and a side ailment leaf with default-from, else
 * INVALID (NOT_FOUND for a leaf that is no defaulted side ailment). */
int p4_game_knowledge_side_test(const P4GameKnowledge *state, uint32_t enemy_id, uint32_t leaf, int *known);
int p4_game_knowledge_side_observe(P4GameKnowledge *state, uint32_t enemy_id, uint32_t leaf);
/* The current retail bits, for save codecs. */
const P4KnowledgeRetail *p4_game_knowledge_retail(const P4GameKnowledge *state);
/* Whether any side key of one knowledge identity/profile is known; a profile
 * without side rows simply has none. */
int p4_game_knowledge_any(const P4GameKnowledge *state, uint32_t knowledge_id, uint32_t profile_id, int *known);
/* Resolve a live logical enemy/native-profile pair and element frame to its
 * key in the same snapshot. A missing alias or side pair is NOT_FOUND (another
 * alias sharing the knowledge identity does not qualify it); UINT32_MAX is a
 * legal id in every key field, not a sentinel. */
int p4_game_knowledge_resolve(const P4GameKnowledge *state,
    uint32_t enemy_id, uint32_t native_profile, const P4GameActionElement *element,
    P4KnowledgeKey *key);

/* Stable persistence identity of one compiled key, borrowing snapshot strings:
 * its canonical (lowest-index) alias and side row, or its side ailment when
 * defaulted (no row). Save codecs serialize fields, never pointers; owner is
 * NULL for an unowned retail descriptor. */
typedef struct P4KnowledgeIdentity {
    P4KnowledgeKey key;
    const char *owner, *alias_key, *element_key, *channel_key;
    uint32_t alias_schema, alias_revision, element_revision, channel_revision;
    const char *element_owner, *channel_owner;
    uint32_t defaulted;
} P4KnowledgeIdentity;
int p4_game_knowledge_identity_at(const P4GameKnowledge *state, size_t index,
    P4KnowledgeIdentity *identity, int *known);
/* The canonical alias of one compiled key, borrowed from its snapshot, for
 * source-ledger references rather than owner-string lookup. */
const P4AffinityProfileAlias *p4_game_knowledge_identity_alias_at(
    const P4GameKnowledge *state, size_t index);
int p4_game_knowledge_identity_find(const P4GameKnowledge *state,
    const P4KnowledgeIdentity *identity, size_t *index);
/* Atomic replacement at the shared sidecar's non-failing commit: validates all
 * strictly increasing indices and every side bit (a defaulted side ailment's
 * index) before touching the current observations; NULL retail clears it. */
int p4_game_knowledge_replace(P4GameKnowledge *state, const size_t *known_indices, size_t count,
    const P4KnowledgeRetail *retail);

/* Package handler "game-tables.enemy-knowledge" (expanded element cohorts
 * only): its CPU pass registers the REQUIRED p4pc/enemy-knowledge save block,
 * its runtime pass stages the shared "element-knowledge" completion
 * participant. Core-only session end after every save/battle user quiesced. */
int p4_game_knowledge_boot_session_end(void);
#endif
