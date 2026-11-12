/* Real native skill lifecycle over actual prepared synthetic source inputs.
 * Direct dispatch uses a probe; the checked path uses actual selected bytes.
 * No native image, battle or gameplay admission is claimed. */
#include "p4pc/game_skills.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/package_memory.h"
#include "element_skill_inputs.h"
#include "p4pc/guest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4TestElementSkillInputs inputs;
static unsigned observed;
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected skill lifecycle runtime recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected skill lifecycle source use: %s\n", reason); abort(); }
/* The native lifecycle witness must never enter unrelated game/renderer code.
 * Fail-stop definitions permit COFF linkers to retain those unused sections
 * without supplying a fake successful gameplay or rendering fallback. */
uint32_t RpRandom(void) { abort(); }
void p4_game_tables_text_draw(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline)
{
    (void)stream; (void)x; (void)y; (void)depth; (void)color; (void)style;
    (void)effect; (void)flags; (void)multiline; abort();
}
static int authored_use(const P4GameSkills *view, uint32_t id, void *context)
{
    (void)view; (void)id; (void)context;
    fputs("Prefix-only lifecycle consumed an authored row\n", stderr); abort();
}
static int source_probe(void *context, const char *table, const char *path,
    uint32_t rows, const uint8_t *bytes, size_t size)
{
    assert(context == &inputs && !strcmp(table, "skill") && !strcmp(path, "battle/SKILL.TBL") && rows == 440);
    assert(bytes == p4_game_table_input_bytes(inputs.skill) && size == p4_game_table_input_size(inputs.skill));
    ++observed;
    return P4_TABLE_OK; /* Dispatch facts only, never loaded-range authority. */
}
static void no_guest(void)
{
#ifdef P4_GUEST64
    assert(!p4_guest_base); /* Any accidental GP read/write would fault. */
#endif
}
static P4GameSkills *new_view(void)
{
    P4GameSkills *view = p4_game_skills_create(NULL); assert(view);
    assert(!p4_game_skills_bind_runtime_use(view, p4_game_table_input_semantic_use, authored_use, &inputs));
    return view;
}
static void seal_view(P4GameSkills *view)
{
    assert(!p4_game_skills_seal(view, p4_game_table_input_span(inputs.skill, 1),
        p4_game_table_input_span(inputs.skill, 0), p4_game_table_input_span(inputs.effect, 0),
        p4_game_table_input_span(inputs.message, 1), inputs.tables, NULL, NULL, 1));
}
static void word(uint8_t *out, uint32_t value)
{ for (unsigned i = 0; i < 4; ++i) out[i] = (uint8_t)(value >> (i * 8u)); }
static void detach(void)
{
    /* Fixture owns no tasks/VM/image. No active pointer was published; its
     * real native session reset does not read/write any guest word. */
    assert(!p4_game_skills_active());
    assert(!p4_game_skills_session_end(0));
    assert(!p4_game_tables_battle_complete_clear());
    no_guest();
}
int main(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    assert(!p4_game_tables_freeze(tables, 1));
    inputs.tables = tables;
    inputs.skills = new_view();
    p4_test_element_inputs_begin(&inputs);
    P4GameSkills *wrong = new_view(); seal_view(wrong);
    no_guest();
    assert(p4_game_skills_boot_unstage(NULL) == P4_TABLE_INVALID);
    assert(!p4_game_skills_boot_unstage(inputs.skills));
    assert(!p4_game_skills_boot_stage(inputs.skills));
    assert(p4_game_skills_boot_unstage(wrong) == P4_TABLE_CONFLICT);
    assert(!p4_game_skills_boot_unstage(inputs.skills));
    assert(!p4_game_skills_boot_unstage(inputs.skills));
    assert(!p4_game_skills_boot_stage(inputs.skills)); /* Real early retry. */
    assert(p4_game_tables_battle_prepare() == P4_TABLE_NOT_READY); /* No original bank events. */
    assert(p4_game_skills_boot_unstage(inputs.skills) == P4_TABLE_FROZEN);
    assert(!p4_game_skills_revision(inputs.skills) && !p4_game_skills_active());
    detach();
    assert(!p4_game_skills_boot_stage(inputs.skills));
    assert(!p4_game_tables_prepare_register(source_probe, &inputs));
    uint8_t descriptor[0xa4] = {0};
    word(descriptor + 4, 0x007644a4u); word(descriptor + 12, 0x007644a8u);
    p4_game_tables_observe_packed(descriptor, "battle/SKILL.TBL",
        p4_game_table_input_bytes(inputs.skill), (uint32_t)p4_game_table_input_size(inputs.skill), NULL);
    assert(observed == 1 && p4_game_skills_boot_unstage(inputs.skills) == P4_TABLE_FROZEN);
    assert(!p4_game_skills_revision(inputs.skills) && !p4_game_skills_active());
    detach();
    assert(!p4_game_tables_prepare_clear());
    P4GameTableInput *owned[] = {inputs.skill, inputs.effect, inputs.message};
    assert(!p4_game_table_admission_stage(owned, 3));
    assert(!p4_game_skills_boot_stage(inputs.skills));
    /* The fixture's sealed input stands in for the loader's writable buffer. */
    p4_game_table_admission_packed(descriptor, "battle/SKILL.TBL",
        (uint8_t *)p4_game_table_input_bytes(inputs.skill), (uint32_t)p4_game_table_input_size(inputs.skill),
        p4_game_tables_observe_packed);
    assert(observed == 1 && p4_game_skills_boot_unstage(inputs.skills) == P4_TABLE_FROZEN);
    assert(!p4_game_skills_revision(inputs.skills) && !p4_game_skills_active());
    detach();
    assert(!p4_game_table_admission_unstage(owned, 3));
    assert(!p4_game_skills_boot_stage(inputs.skills));
    p4_test_element_inputs_materialize(&inputs); /* Real zero-added prefix stamp; no clone. */
    assert(p4_game_skills_revision(inputs.skills) == 1 && !p4_game_skills_metadata(inputs.skills) &&
        !p4_game_skills_elements(inputs.skills) && !p4_game_skills_presentation(inputs.skills));
    assert(p4_game_skills_boot_unstage(inputs.skills) == P4_TABLE_FROZEN);
    assert(p4_game_skills_boot_stage(inputs.skills) == P4_TABLE_FROZEN);
    detach();
    assert(!p4_game_tables_prepare_clear());
    p4_game_skills_destroy(wrong); p4_game_skills_destroy(inputs.skills);
    p4_test_element_inputs_release(&inputs);
    p4_game_tables_destroy(tables);
    p4_package_registry_retire();
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    no_guest();
    puts("game_skills_native: direct dispatch and selected-source immediate handoff, scoped rollback and no-guest prefix passed; no native image/gameplay admission");
    return 0;
}
