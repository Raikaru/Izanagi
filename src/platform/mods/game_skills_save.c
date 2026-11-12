#include "p4pc/game_skills_save.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_table_refs.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_save.h"
#include <stdio.h>
#include <string.h>

#define SKILL_SAVE_SCHEMA 1u
#define SKILL_SAVE_MAGIC 0x4B533450u /* P4SK, little endian */
/* DatPersonaRecord (0x30 bytes): flags u16 (bit 0 active), ID u16 at +2, skill
 * slots u16[8] at +0x0C. */
#define PERSONA_RECORD 0x30u
#define RECORD_SKILLS 0x0Cu
#define RECORD_SKILL_SLOTS 8u
#define STOCK_BASE 0x00797F8Cu /* datPersonaRosterSlot 0x0010ACE0 */
#define STOCK_SLOTS 12u
#define STAGE_RECORD 0x0079B6A8u /* datStageRecord 0x00110270 */
#define PARTY_SLOTS 0x00796E50u /* DatPartySlot[party ID - 2], datPersonaGetRecord 0x0010A900 */
#define PARTY_STRIDE 0x88u
#define PARTY_PERSONA 0x54u
#define PARTY_COUNT 9u

typedef struct SkillSaveContext {
    const P4GameSkills *skills;
    const P4GameTableSources *sources;
} SkillSaveContext;
static P4ModSaveRegistration *registration;
static SkillSaveContext save_context;
static uint8_t used[(P4_SKILL_MAX_ID + 8u) / 8u];

static int fail(const P4ModSaveContext *context, const char *message)
{
    if (context && context->error && context->error_capacity && !context->error[0])
        snprintf(context->error, context->error_capacity, "%s", message);
    return -1;
}
static int ready(const SkillSaveContext *source)
{
    return p4_game_skills_revision(source->skills) &&
        p4_game_table_sources_validate_view(source->sources, source->skills) == P4_TABLE_OK;
}
static void mark(uint32_t record)
{
    for (uint32_t slot = 0; slot < RECORD_SKILL_SLOTS; ++slot) {
        const uint32_t id = P4_G16(record + RECORD_SKILLS + slot * 2u);
        if (id >= P4_SKILL_RETAIL_COUNT && id <= P4_SKILL_MAX_ID) used[id >> 3] |= (uint8_t)(1u << (id & 7u));
    }
}
/* Every saved persona record; an ID nothing registers stays unpinned. */
static void scan(void)
{
    memset(used, 0, sizeof used);
    for (uint32_t slot = 0; slot < STOCK_SLOTS; ++slot) {
        const uint32_t record = STOCK_BASE + slot * PERSONA_RECORD;
        if (P4_G16(record) & 1u) mark(record);
    }
    if (P4_G16(STAGE_RECORD + 2u)) mark(STAGE_RECORD);
    for (uint32_t slot = 0; slot < PARTY_COUNT; ++slot) {
        const uint32_t record = PARTY_SLOTS + slot * PARTY_STRIDE + PARTY_PERSONA;
        if (P4_G16(record + 2u)) mark(record);
    }
    const uint32_t limit = p4_game_personas_native_limit();
    for (uint32_t id = 0; id < limit; ++id) {
        const uint32_t record = p4_game_personas_native_compendium(id);
        if (P4_G16(record) & 1u) mark(record);
    }
}
static int is_used(uint32_t id) { return id <= P4_SKILL_MAX_ID && (used[id >> 3] & (1u << (id & 7u))); }
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    const SkillSaveContext *source = user;
    if (!ready(source)) return fail(context, "Added skill state is not native-ready");
    scan();
    uint32_t count = 0;
    const size_t rows = p4_game_skills_count(source->skills);
    for (size_t i = 0; i < rows; ++i)
        if (is_used(p4_game_skills_at(source->skills, i)->desc.id)) ++count;
    if (!count) return 1; /* Omit: no saved record holds an added skill. */
    int result = p4_mod_save_write_u32(writer, SKILL_SAVE_MAGIC);
    if (!result) result = p4_mod_save_write_u32(writer, SKILL_SAVE_SCHEMA);
    if (!result) result = p4_mod_save_write_u32(writer, count);
    if (result) return result;
    P4GameTableRefWriter *refs = NULL;
    result = p4_game_table_refs_writer_begin(source->sources, context, writer, &refs);
    if (result) return result;
    /* A reference resolves by ID and must match the entry's owner, key, schema
     * and revision: a load whose package moved or lacks the skill fails there. */
    for (size_t i = 0; !result && i < rows; ++i) {
        const uint32_t id = p4_game_skills_at(source->skills, i)->desc.id;
        if (!is_used(id)) continue;
        const uint32_t ids[5] = {id};
        result = p4_game_table_refs_write(refs, source->skills, P4_TABLE_SOURCE_SKILL, ids);
    }
    p4_game_table_refs_writer_end(refs);
    return result;
}
static int decode(const P4ModSaveContext *context, uint32_t schema,
    P4ModSaveReader *reader, void **staged, void *user)
{
    const SkillSaveContext *source = user;
    *staged = NULL;
    if (context && context->image.data && !ready(source))
        return fail(context, "Added skill state is not native-ready");
    if (!reader) return schema == 0 ? P4_TABLE_OK : P4_TABLE_INVALID;
    if (schema != SKILL_SAVE_SCHEMA) return P4_TABLE_INVALID;
    uint32_t magic, version, count;
    int result = p4_mod_save_read_u32(reader, &magic);
    if (!result) result = p4_mod_save_read_u32(reader, &version);
    if (!result) result = p4_mod_save_read_u32(reader, &count);
    if (result) return result;
    if (magic != SKILL_SAVE_MAGIC || version != SKILL_SAVE_SCHEMA || !count ||
        count > p4_game_skills_count(source->skills)) return P4_TABLE_INVALID;
    P4GameTableRefReader *refs = NULL;
    result = p4_game_table_refs_reader_begin(source->sources, context, reader, &refs);
    if (result) return result;
    uint32_t previous = 0;
    for (uint32_t i = 0; !result && i < count; ++i) {
        const P4TableSourceEntry *current;
        result = p4_game_table_refs_read(refs, source->skills, P4_TABLE_SOURCE_SKILL, &current);
        if (result) break;
        const uint32_t id = current->ids[0];
        if (id < P4_SKILL_RETAIL_COUNT || id <= previous || !p4_game_skills_find(source->skills, id))
            result = P4_TABLE_INVALID;
        previous = id;
    }
    p4_game_table_refs_reader_end(refs);
    if (!result && p4_mod_save_remaining(reader)) result = P4_TABLE_INVALID;
    return result;
}
static int validate(const P4ModSaveContext *context, void *staged, void *user)
{
    (void)context; (void)staged;
    return ready(user) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
/* The block only pins identities; the records themselves load with the image. */
static void commit(void *staged, void *user) { (void)staged; (void)user; }
static void discard(void *staged, void *user) { (void)staged; (void)user; }
int p4_game_skills_save_register(const P4GameSkills *skills, const P4GameTableSources *sources)
{
    if (!skills || !sources) return P4_TABLE_INVALID;
    if (!p4_game_skills_count(skills)) return P4_TABLE_OK;
    if (registration)
        return save_context.skills == skills && save_context.sources == sources ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    save_context = (SkillSaveContext){skills, sources};
    const P4ModSaveProvider provider = {
        .struct_size = sizeof provider, .abi_version = P4_MOD_SAVE_ABI_VERSION,
        .owner = "p4pc", .key = "expanded-skills", .schema_version = SKILL_SAVE_SCHEMA,
        .flags = P4_MOD_SAVE_REQUIRED, .max_bytes = 1024u * 1024u, .user = &save_context,
        .encode = encode, .decode = decode, .validate = validate, .commit = commit, .discard = discard
    };
    if (p4_mod_save_register(&provider, &registration)) {
        save_context = (SkillSaveContext){0};
        return P4_TABLE_CONFLICT;
    }
    return P4_TABLE_OK;
}
int p4_game_skills_save_unregister(void)
{
    if (!registration) return P4_TABLE_OK;
    if (p4_mod_save_unregister(registration)) return P4_TABLE_CONFLICT;
    registration = NULL;
    save_context = (SkillSaveContext){0};
    return P4_TABLE_OK;
}
