#ifndef P4PC_GAME_SKILLS_H
#define P4PC_GAME_SKILLS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/game_text.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_table_use.h"
/* Stable keys use the shared owner:local-name convention. Retail IDs cannot be
 * registered as new records. Eight equipped slots are not a catalog limit. */
typedef struct P4GameSkillDesc {
    uint32_t id, active, element_id, schema, revision;
    char key[P4_TABLE_KEY_SIZE];
    uint8_t metadata[P4_SKILL_ROW_SIZE];
    uint8_t presentation[4]; /* BtlSkillFlags: info u16, flags u16, little endian. */
    uint8_t element_aux;
    uint8_t inheritance_group; /* Legacy19-column matrix selector, independent of element_id. */
    int8_t inheritance_rate; /* Signed inheritance divisor/mutation grade; zero gives no inheritance weight. */
    /* Retail active skill (1..439) whose battle effect and sound bank files a
     * cast of this skill loads (p4_game_skill_effect); 0 = none, and casting
     * the skill refuses the session. */
    uint32_t effect_source;
} P4GameSkillDesc;
typedef struct P4GameSkillView {
    P4GameSkillDesc desc;
    char owner[64];
} P4GameSkillView;
/* Real CPU row proof for AI/source consumers, including the retail prefix.
 * metadata is NULL if that original row is absent (not a synthesized passive
 * row). Exactly one of retail_element/authored identifies the element source:
 * raw retail bytes retain native sentinel semantics; authored keeps fullu32.
 * All pointers borrow sealed definitions/original inputs, never guest storage. */
typedef struct P4GameSkillDefinition {
    uint32_t id, active;
    const uint8_t *metadata, *retail_element;
    const P4GameSkillDesc *authored;
} P4GameSkillDefinition;
typedef struct P4GameSkills P4GameSkills;
/* Root attests this exact sealed view/id against its captured ledger entry.
 * It must refuse unknown rows/origins, never rebuild authority from owner text. */
typedef int (*P4GameSkillRowUseFn)(const P4GameSkills *exact_view,
    uint32_t skill_id, void *immutable_source_context);
/* Inject real source transports before sealing. Original input context is
 * taken from the actual sealed SKILL span owner, never supplied/reconstructed.
 * All callbacks/context outlive this snapshot and native readers. */
int p4_game_skills_bind_runtime_use(P4GameSkills *skills,
    P4GameTableSourceUseFn original_use, P4GameSkillRowUseFn authored_use,
    void *immutable_source_context);
/* Pure declaration prerequisite for boot_stage, NOT origin/native authority.
 * No callbacks, provider touches or guest work occur here. */
int p4_game_skills_validate_runtime_sources(const P4GameSkills *skills);
/* Actual runtime consumption only, after materialization/binding. Original
 * active rows use issued SKILL1; genuine inactive rows use issued SKILL0.
 * Authored rows use Root's exact-view/id ledger attestation. Only0 permits
 * consumption; positive callback statuses are errors, never fallback/omit.
 * Never call from CPU definitions/sealing/materialization/preparation/binding. */
int p4_game_skills_runtime_use(const P4GameSkills *skills, uint32_t id);
P4GameSkills *p4_game_skills_create(const P4GameTableAllocator *allocator);
/* Destroy only after publication is detached and native queues/readers drain.
 * CPU-sealed text/tables and the original input owners outlive this snapshot. */
void p4_game_skills_destroy(P4GameSkills *skills);
int p4_game_skills_register(P4GameSkills *skills, const char *owner, const P4GameSkillDesc *desc);
/* Native allocator declaration admission, not a semantic registry limit or
 * gameplay certificate: added rows require an actual allocator callback
 * (NOT_FOUND if absent). A full-width element leaf is admitted; activating
 * it still requires the element owner's complete participant cohort
 * (table-source, route, AI, knowledge and UI) at battle completion. No work. */
int p4_game_skills_native_admit(const P4GameSkills *skills);
struct P4GameTableInputSpan;
/* CPU-only immutable seal against actual producer-owned input spans:
 * SKILL1 metadata, SKILL0 elements (same owner), EFFECT0 presentation, MSG1 names.
 * All extents, element/text references and text profiles are checked here;
 * no guest allocation, address, RW dependency or publication is involved.
 * CPU-only snapshots may seal with no guest allocator; native_admit/boot_stage
 * reject added rows without that transport, and materialize checks it itself.
 * Same revision and exact dependency objects are idempotent; no registration
 * reopens after sealing. Empty snapshots require no original inputs. */
int p4_game_skills_seal(P4GameSkills *skills,
    const struct P4GameTableInputSpan *metadata,
    const struct P4GameTableInputSpan *elements,
    const struct P4GameTableInputSpan *presentation,
    const struct P4GameTableInputSpan *original_names,
    const P4GameTables *tables, const P4GameText *names,
    const P4GameText *descriptions, uint32_t revision);
uint32_t p4_game_skills_semantic_revision(const P4GameSkills *skills);
/* Actual runtime phase only: verify all four copied native banks against the
 * sealed source bytes, then allocate/copy/bind. Text must be materialized first.
 * Failure leaves CPU definitions sealed and all game pointers unchanged.
 * Repeated calls verify the same original bytes without another allocation. */
int p4_game_skills_materialize(P4GameSkills *skills,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *elements, size_t elements_size,
    const uint8_t *presentation, size_t presentation_size,
    const uint8_t *original_names, size_t original_names_size);
int p4_game_skills_validate_original(const P4GameSkills *skills,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *elements, size_t elements_size,
    const uint8_t *presentation, size_t presentation_size,
    const uint8_t *original_names, size_t original_names_size);
uint32_t p4_game_skills_revision(const P4GameSkills *skills);
size_t p4_game_skills_count(const P4GameSkills *skills);
uint32_t p4_game_skills_limit(const P4GameSkills *skills);
/* at/find expose immutable CPU definitions, never fabricated guest streams. */
const P4GameSkillView *p4_game_skills_at(const P4GameSkills *skills, size_t index);
const P4GameSkillView *p4_game_skills_find(const P4GameSkills *skills, uint32_t id);
/* OK/NOT_FOUND/NOT_READY/INVALID; writes out only on success. ID0 remains the
 * caller's empty/action sentinel decision, not an invented missing row. */
int p4_game_skills_definition_find(const P4GameSkills *skills, uint32_t id,
    P4GameSkillDefinition *definition);
/* Guest addresses for one atomic game-thread pointer publication. Metadata and
 * element addresses are zero for an empty snapshot. */
uint32_t p4_game_skills_metadata(const P4GameSkills *skills);
uint32_t p4_game_skills_elements(const P4GameSkills *skills);
uint32_t p4_game_skills_presentation(const P4GameSkills *skills);
/* Runtime bindings only; zero before successful materialization or if absent. */
uint32_t p4_game_skills_name(const P4GameSkills *skills, uint32_t id);
uint32_t p4_game_skills_description(const P4GameSkills *skills, uint32_t id);
/* Pure exact dependency lookup, not runtime/source authority. */
const P4GameText *p4_game_skills_text_view(const P4GameSkills *skills, uint32_t role);

/* Stage the CPU-sealed snapshot before native raw-table exposure. The
 * completion participant materializes only after the actual banks arrive. */
int p4_game_skills_boot_stage(P4GameSkills *skills);
/* Exact EARLY rollback only; raw observations/materialization lock ownership. */
int p4_game_skills_boot_unstage(P4GameSkills *exact);
P4GameSkills *p4_game_skills_active(void);
/* Caller supplies actual RAM lifetime after tasks and queues drain. */
int p4_game_skills_session_end(int image_live);
#endif