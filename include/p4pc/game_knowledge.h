#ifndef P4PC_GAME_KNOWLEDGE_H
#define P4PC_GAME_KNOWLEDGE_H
#include "p4pc/game_tables.h"

/* Expanded enemy knowledge (Analyze / known affinities) for authored aliases.
 * The enemy's logical id, native id2 selector, semantic profile and knowledge
 * identity are not interchangeable. Aliases may deliberately share a knowledge
 * identity, but only within the same profile. Every key comes from an actual
 * enemy alias and its side affinity row; there is no profile/element fallback.
 * Unmodified retail pairs keep the retail bitmap (game_knowledge_native.h). */
typedef struct P4KnowledgeKey {
    uint32_t knowledge_id, profile_id, element_id, channel_id;
} P4KnowledgeKey;
typedef struct P4GameKnowledge P4GameKnowledge;

/* Build from a frozen snapshot that must outlive this state. No enemy side
 * rows means NULL without allocation. Queries and observations never allocate. */
int p4_game_knowledge_create(const P4GameTables *tables, P4GameKnowledge **state);
void p4_game_knowledge_destroy(P4GameKnowledge *state);
void p4_game_knowledge_clear(P4GameKnowledge *state);
size_t p4_game_knowledge_count(const P4GameKnowledge *state);
const P4GameTables *p4_game_knowledge_snapshot(const P4GameKnowledge *state);
int p4_game_knowledge_test(const P4GameKnowledge *state, const P4KnowledgeKey *key, int *known);
int p4_game_knowledge_observe(P4GameKnowledge *state, const P4KnowledgeKey *key);
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
 * its canonical (lowest-index) alias and side row. Save codecs serialize
 * fields, never pointers; owner is NULL for an unowned retail descriptor. */
typedef struct P4KnowledgeIdentity {
    P4KnowledgeKey key;
    const char *owner, *alias_key, *element_key, *channel_key;
    uint32_t alias_schema, alias_revision, element_revision, channel_revision;
    const char *element_owner, *channel_owner;
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
 * strictly increasing indices before touching the current observations. */
int p4_game_knowledge_replace(P4GameKnowledge *state, const size_t *known_indices, size_t count);

/* Package handler "game-tables.enemy-knowledge" (expanded element cohorts
 * only): its CPU pass registers the REQUIRED p4pc/enemy-knowledge save block,
 * its runtime pass stages the shared "element-knowledge" completion
 * participant. Core-only session end after every save/battle user quiesced. */
int p4_game_knowledge_boot_session_end(void);
#endif
