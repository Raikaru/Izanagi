#include "p4pc/game_skills.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/game_text_native.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include <stdio.h>
#include <stdlib.h>

enum { SKILL_ELEMENT_SLOT = 0x007644a4u, SKILL_METADATA_SLOT = 0x007644a8u,
    SKILL_NAME_SLOT = 0x00764538u };
enum { SKILL_PRESENTATION_SLOT = 0x007644acu };
typedef struct SkillStage {
    P4GameSkills *skills;
    uint32_t prepared;
} SkillStage;
static SkillStage staged;
static P4GameSkills *active;
static uint32_t original_metadata, original_elements, original_presentation;
static uint32_t original_metadata_bytes, original_element_bytes, original_presentation_bytes;
static int prepare_complete(void *user);
static void commit_complete(void *user);
static int pending(void) { return staged.skills != NULL; }
static _Noreturn void refuse(int result)
{
    fprintf(stderr, "p4pc: skill table publication refused (%d): %s\n", result, p4_game_tables_error(result));
    abort();
}
static _Noreturn void source_refuse(uint32_t id, const char *kind, int result)
{
    char reason[160];
    snprintf(reason, sizeof reason, "Skill %u %s source authorization failed (%d)",
        (unsigned)id, kind, result);
    p4_package_boot_runtime_failed(reason);
}
static void use_owned(uint32_t id)
{
    int result = p4_game_skills_runtime_use(active, id);
    if (result) source_refuse(id, "catalog", result);
}
static void use_text(uint32_t id, uint32_t role)
{
    const P4GameText *text = p4_game_skills_text_view(active, role);
    int result = p4_game_tables_text_runtime_use(text, P4_TEXT_SKILL, id, role);
    if (result) source_refuse(id, "text", result);
}
int p4_game_skills_boot_stage(P4GameSkills *skills)
{
    if (!skills) return P4_TABLE_INVALID;
    if (!p4_game_skills_semantic_revision(skills)) return P4_TABLE_NOT_READY;
    if (active || p4_game_skills_revision(skills)) return P4_TABLE_FROZEN;
    int admission = p4_game_skills_native_admit(skills);
    if (admission) return admission;
    int sources = p4_game_skills_validate_runtime_sources(skills);
    if (sources) return sources;
    if (staged.skills) return staged.skills == skills ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (!p4_game_skills_count(skills)) {
        P4GameSkillDefinition definition;
        int result = p4_game_skills_definition_find(skills, 0, &definition);
        if (result == P4_TABLE_NOT_FOUND) return P4_TABLE_OK; /* Genuinely empty, not a fabricated retail view. */
        if (result) return result;
    }
    int result = p4_game_tables_battle_complete_register("skill", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (SkillStage){skills, 0};
    return P4_TABLE_OK;
}
int p4_game_skills_boot_unstage(P4GameSkills *exact)
{
    if (!exact) return P4_TABLE_INVALID;
    if (active || staged.prepared || p4_game_skills_revision(exact)) return P4_TABLE_FROZEN;
    if (!staged.skills) return P4_TABLE_OK;
    if (staged.skills != exact) return P4_TABLE_CONFLICT;
    int result = p4_game_tables_battle_complete_unregister("skill", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (SkillStage){0};
    return P4_TABLE_OK;
}
P4GameSkills *p4_game_skills_active(void) { return active; }
uint16_t p4_game_skill_endurance_sentinel(void)
{
    return active ? (uint16_t)(P4_SKILL_MAX_ID + 1u) : P4_SKILL_RETAIL_COUNT;
}
int p4_game_skills_expanded(void) { return active != NULL; }
int32_t p4_game_skill_inheritance_rate(uint32_t id)
{
    if (id < P4_SKILL_RETAIL_COUNT) return (int8_t)P4_G8(0x00641e60u + id);
    const P4GameSkillView *row = p4_game_skills_find(active, id);
    if (!row) source_refuse(id, "catalog", P4_TABLE_NOT_FOUND);
    use_owned(id);
    return row->desc.inheritance_rate;
}
int32_t p4_game_skill_inheritance_group(uint32_t id, int32_t retail_group)
{
    if (id < P4_SKILL_RETAIL_COUNT) return retail_group;
    const P4GameSkillView *row = p4_game_skills_find(active, id);
    if (!row) source_refuse(id, "catalog", P4_TABLE_NOT_FOUND);
    use_owned(id);
    return row->desc.inheritance_group;
}
static uint32_t mutation_id(uint32_t ordinal)
{
    if (ordinal < P4_SKILL_RETAIL_COUNT) return ordinal;
    const P4GameSkillView *row = p4_game_skills_at(active, ordinal - P4_SKILL_RETAIL_COUNT);
    if (!row) refuse(P4_TABLE_NOT_FOUND);
    return row->desc.id;
}
uint16_t p4_game_skill_pick_mutation(const uint16_t *known_skills, uint32_t excluded_id)
{
    extern uint32_t RpRandom(void);
    int32_t grade = p4_game_skill_inheritance_rate(excluded_id);
    uint32_t rows = P4_SKILL_RETAIL_COUNT + (uint32_t)p4_game_skills_count(active);
    uint32_t count = 0;
    for (uint32_t i = 0; i < rows; ++i) {
        uint32_t id = mutation_id(i);
        if (id != excluded_id && p4_game_skill_inheritance_rate(id) == grade) ++count;
    }
    if (!count) refuse(P4_TABLE_NOT_FOUND);
    /* Same rejection sampling and one random draw per attempt as00304410.
     * Enumerating the chosen ordinal avoids a new per-call scratch allocation. */
    for (;;) {
        uint32_t pick = RpRandom() % count;
        for (uint32_t i = 0; i < rows; ++i) {
            uint32_t id = mutation_id(i);
            if (id == excluded_id || p4_game_skill_inheritance_rate(id) != grade) continue;
            if (pick) { --pick; continue; }
            uint32_t known = 0;
            while (known < 8 && known_skills[known] != id) ++known;
            if (known == 8) return (uint16_t)id;
            break;
        }
    }
}
int p4_game_skill_registered(uint32_t id)
{
    if (id < P4_SKILL_RETAIL_COUNT) return 1;
    if (!p4_game_skills_find(active, id)) return 0;
    use_owned(id);
    return 1;
}
int p4_game_skill_active(uint32_t id)
{
    if (id < P4_SKILL_RETAIL_COUNT) return id < P4_SKILL_RETAIL_ACTIVE_COUNT;
    const P4GameSkillView *row = p4_game_skills_find(active, id);
    if (row) use_owned(id);
    return row && row->desc.active;
}
uint32_t p4_game_skill_name(uint32_t id)
{
    uint32_t stream = p4_game_skills_name(active, id);
    if (stream) {
        use_owned(id);
        use_text(id, P4_TEXT_NAME);
    }
    return stream;
}
uint32_t p4_game_skill_description(uint32_t id)
{
    uint32_t stream = p4_game_skills_description(active, id);
    if (stream) {
        use_owned(id);
        use_text(id, P4_TEXT_DESCRIPTION);
    }
    return stream;
}
int32_t p4_game_skill_element(uint32_t id, int32_t retail_element)
{
    const P4GameSkillView *row = p4_game_skills_find(active, id);
    if (row) use_owned(id);
    return row ? (int32_t)row->desc.element_id : retail_element;
}
int p4_game_skill_draw_description(uint32_t id, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags)
{
    if (id < P4_SKILL_RETAIL_COUNT) return 0;
    uint32_t stream = p4_game_skill_description(id);
    if (!stream) source_refuse(id, "text", P4_TABLE_NOT_FOUND);
    p4_game_tables_text_draw(stream, x, y, scale, color, style, effect, flags, 1);
    return 1;
}
int p4_game_skills_packed_pending(void) { return pending(); }
uint32_t p4_game_skills_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count, uint32_t *rows)
{
    uint32_t mask = 0, found = 0;
    if (!pending()) return 0;
    for (unsigned block = 0; block < count; ++block) {
        const uint32_t slot = blocks[block].slot, size = blocks[block].size;
        uint32_t stride = 0, bit = 0;
        if (slot == SKILL_METADATA_SLOT) { stride = P4_SKILL_ROW_SIZE; bit = 1; }
        else if (slot == SKILL_ELEMENT_SLOT) { stride = P4_SKILL_ELEMENT_SIZE; bit = 2; }
        else if (slot == SKILL_NAME_SLOT) { stride = 19; bit = 4; }
        else if (slot == SKILL_PRESENTATION_SLOT) { stride = 4; bit = 8; }
        if (!stride) continue;
        if (size % stride) refuse(P4_TABLE_INVALID);
        /* Metadata takes precedence if one resource supplies several banks. */
        if (!found || bit == 1) found = size / stride;
        mask |= bit;
    }
    if (rows) *rows = found;
    return mask;
}
void p4_game_skills_packed_accept(uint32_t mask)
{
    if (mask && !pending()) refuse(P4_TABLE_NOT_READY);
    staged.prepared |= mask;
}
void p4_game_skills_load_begin(void)
{
    if (pending()) staged.prepared = 0;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (!pending() || staged.prepared != 15) return P4_TABLE_NOT_READY;
    uint32_t metadata_base, metadata_rows, element_base, element_rows, name_base, name_rows;
    uint32_t presentation_base, presentation_rows;
    if (!p4_game_table_extent_rows(SKILL_METADATA_SLOT, P4_SKILL_ROW_SIZE, &metadata_base, &metadata_rows) ||
        !p4_game_table_extent_rows(SKILL_ELEMENT_SLOT, P4_SKILL_ELEMENT_SIZE, &element_base, &element_rows) ||
        !p4_game_table_extent_rows(SKILL_PRESENTATION_SLOT, 4, &presentation_base, &presentation_rows) ||
        !p4_game_table_extent_rows(SKILL_NAME_SLOT, 19, &name_base, &name_rows)) return P4_TABLE_INVALID;
    /* Semantic/source/profile checks were completed by the early CPU seal.
     * Only actual copied-byte verification, allocation and binding remain. */
    int result = p4_game_skills_materialize(staged.skills,
        P4_GPTR(const uint8_t, metadata_base), (size_t)metadata_rows * P4_SKILL_ROW_SIZE,
        P4_GPTR(const uint8_t, element_base), (size_t)element_rows * P4_SKILL_ELEMENT_SIZE,
        P4_GPTR(const uint8_t, presentation_base), (size_t)presentation_rows * 4u,
        P4_GPTR(const uint8_t, name_base), (size_t)name_rows * 19u);
    if (result) return result;
    original_metadata = metadata_base; original_metadata_bytes = metadata_rows * P4_SKILL_ROW_SIZE;
    original_elements = element_base; original_element_bytes = element_rows * P4_SKILL_ELEMENT_SIZE;
    original_presentation = presentation_base; original_presentation_bytes = presentation_rows * 4u;
    return P4_TABLE_OK;
}
static void commit_complete(void *user)
{
    (void)user;
    /* A real prefix-only CPU view still verifies copied inputs, but does not
     * relocate banks or turn on expanded-skill/sentinel behavior. */
    if (!p4_game_skills_count(staged.skills)) return;
    /* One non-yielding game-thread boundary; all allocations/text/references
     * succeeded before either pointer becomes visible to native consumers.
     * Each store publishes its extent in the same step. */
    const uint32_t rows = p4_game_skills_limit(staged.skills);
    p4_game_table_slot_publish(SKILL_METADATA_SLOT, p4_game_skills_metadata(staged.skills), rows * P4_SKILL_ROW_SIZE);
    p4_game_table_slot_publish(SKILL_ELEMENT_SLOT, p4_game_skills_elements(staged.skills), rows * P4_SKILL_ELEMENT_SIZE);
    p4_game_table_slot_publish(SKILL_PRESENTATION_SLOT, p4_game_skills_presentation(staged.skills), rows * 4u);
    active = staged.skills;
}
int p4_game_skills_session_end(int image_live)
{
    if (active && image_live) {
        if (P4_G32(SKILL_METADATA_SLOT) != p4_game_skills_metadata(active) ||
            P4_G32(SKILL_ELEMENT_SLOT) != p4_game_skills_elements(active) ||
            P4_G32(SKILL_PRESENTATION_SLOT) != p4_game_skills_presentation(active)) return P4_TABLE_CONFLICT;
        p4_game_table_slot_publish(SKILL_METADATA_SLOT, original_metadata, original_metadata_bytes);
        p4_game_table_slot_publish(SKILL_ELEMENT_SLOT, original_elements, original_element_bytes);
        p4_game_table_slot_publish(SKILL_PRESENTATION_SLOT, original_presentation, original_presentation_bytes);
    }
    active = NULL;
    staged = (SkillStage){0};
    original_metadata = original_elements = original_presentation = 0;
    original_metadata_bytes = original_element_bytes = original_presentation_bytes = 0;
    return P4_TABLE_OK;
}
