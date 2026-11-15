/* Exercise the native Analyze column selector with and without its published
 * side-ailment gate. The presenter itself is the production implementation. */
#include <assert.h>
#include "p4pc/game_side_ailment.h"
#include "../src/platform/mods/game_elements_ui_native.c"

static int side_enabled;
static int side_registered = 1;
uint32_t p4_game_elements_ui_count(const P4GameElementUi *ui) { (void)ui; return 1; }
int p4_game_elements_ui_header_art_find(const P4GameElementUi *ui, uint32_t leaf, uint32_t *index)
{ (void)ui; *index = leaf; return P4_TABLE_OK; }
int p4_game_knowledge_native_ailment(void *unit, uint32_t leaf, uint32_t *state, uint32_t *observed)
{ (void)unit; (void)leaf; *state = 0; *observed = 1; return P4_TABLE_OK; }
uint32_t datGetFlag(int32_t bit) { (void)bit; return 0; }
void *datPersonaGetRecord(uint16_t id) { (void)id; return NULL; }
int32_t datPersonaFindSkillSlot(uint8_t *persona, uint16_t skill) { (void)persona; (void)skill; return -1; }
const P4SideAilmentDef *p4_side_ailment_find_leaf(uint32_t leaf)
{ return side_registered && (leaf == 24u || leaf == 25u) ? (const P4SideAilmentDef *)(uintptr_t)1 : NULL; }
int p4_side_ailment_active(void) { return side_enabled; }

/* The rest of the presenter pulls game tables and the element owner; the
 * column selector never reaches them here. Inert stubs keep the 64w link
 * (lld) as linkable as the host one (--gc-sections dropped these on Linux). */
const char *p4_game_tables_error(int result) { (void)result; assert(!"p4_game_tables_error must not be called"); return ""; }
void p4_package_boot_runtime_failed(const char *reason) { (void)reason; assert(!"p4_package_boot_runtime_failed must not be called"); }
void *p4pc_heap_calloc(size_t count, size_t size) { (void)count; (void)size; assert(!"p4pc_heap_calloc must not be called"); return NULL; }
void p4pc_heap_free(void *p) { (void)p; assert(!"p4pc_heap_free must not be called"); }
int p4_game_tables_battle_complete_register(const char *table_key, P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user)
{ (void)table_key; (void)prepare; (void)commit; (void)user; assert(!"p4_game_tables_battle_complete_register must not be called"); return 0; }
int p4_game_tables_battle_complete_unregister(const char *table_key, P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user)
{ (void)table_key; (void)prepare; (void)commit; (void)user; assert(!"p4_game_tables_battle_complete_unregister must not be called"); return 0; }
int p4_game_tables_battle_complete_require(const char *owner_key, const char *required_key)
{ (void)owner_key; (void)required_key; assert(!"p4_game_tables_battle_complete_require must not be called"); return 0; }
const P4GameTables *p4_game_elements_ui_tables(const P4GameElementUi *ui) { (void)ui; assert(!"p4_game_elements_ui_tables must not be called"); return NULL; }
const P4GameTables *p4_game_elements_prepared_snapshot(void) { assert(!"p4_game_elements_prepared_snapshot must not be called"); return NULL; }
const P4GameElementUiPreparedColumn *p4_game_elements_ui_column(const P4GameElementUi *ui, uint32_t index)
{ (void)ui; (void)index; assert(!"p4_game_elements_ui_column must not be called"); return NULL; }
int p4_game_elements_prepared_required_pair(uint32_t leaf, uint32_t channel)
{ (void)leaf; (void)channel; assert(!"p4_game_elements_prepared_required_pair must not be called"); return 0; }
size_t p4_game_tables_affinity_alias_count(const P4GameTables *tables) { (void)tables; assert(!"p4_game_tables_affinity_alias_count must not be called"); return 0; }
const P4AffinityProfileAlias *p4_game_tables_affinity_alias_at(const P4GameTables *tables, size_t index)
{ (void)tables; (void)index; assert(!"p4_game_tables_affinity_alias_at must not be called"); return NULL; }
int p4_game_elements_enemy_metadata_flags(uint32_t enemy_id, uint32_t *flags)
{ (void)enemy_id; (void)flags; assert(!"p4_game_elements_enemy_metadata_flags must not be called"); return 0; }
int p4_game_elements_profile_natural_pair(const P4GameTables *tables, uint32_t leaf, uint32_t channel, uint32_t component)
{ (void)tables; (void)leaf; (void)channel; (void)component; assert(!"p4_game_elements_profile_natural_pair must not be called"); return 0; }
int p4_game_skill_active(uint32_t id) { (void)id; assert(!"p4_game_skill_active must not be called"); return 0; }
void p4_game_elements_native_applicability_frame(void *unit, uint32_t skill_id, P4GameActionFrame *frame)
{ (void)unit; (void)skill_id; (void)frame; assert(!"p4_game_elements_native_applicability_frame must not be called"); }
int p4_game_elements_ui_art_find(const P4GameElementUi *ui, uint32_t leaf, uint32_t *index)
{ (void)ui; (void)leaf; (void)index; assert(!"p4_game_elements_ui_art_find must not be called"); return 0; }
int p4_game_elements_ui_art_draw(const P4GameElementUi *ui, uint32_t index, float x, float y, float depth, uint32_t rgba, float scale_x, float scale_y)
{ (void)ui; (void)index; (void)x; (void)y; (void)depth; (void)rgba; (void)scale_x; (void)scale_y; assert(!"p4_game_elements_ui_art_draw must not be called"); return 0; }
int p4_game_elements_ui_prompt_find(const P4GameElementUi *ui, uint32_t prompt, uint32_t *index)
{ (void)ui; (void)prompt; (void)index; assert(!"p4_game_elements_ui_prompt_find must not be called"); return 0; }
int p4_game_elements_ui_persona_snapshot_use(const P4GameElementUi *ui, uint32_t persona_id,
    const uint32_t native_words[7], uint32_t column, uint32_t marker, uint32_t *value)
{ (void)ui; (void)persona_id; (void)native_words; (void)column; (void)marker; (void)value; assert(!"p4_game_elements_ui_persona_snapshot_use must not be called"); return 0; }
int p4_game_elements_ui_runtime_pair(const P4GameElementUi *ui, uint32_t index, uint32_t marker)
{ (void)ui; (void)index; (void)marker; assert(!"p4_game_elements_ui_runtime_pair must not be called"); return 0; }
uint32_t p4_game_elements_ui_ko_text(const P4GameElementUi *ui, uint32_t kind)
{ (void)ui; (void)kind; assert(!"p4_game_elements_ui_ko_text must not be called"); return 0; }
int p4_resources_draw(uint32_t token, float x, float y, float depth, uint32_t rgba, float scale_x, float scale_y)
{ (void)token; (void)x; (void)y; (void)depth; (void)rgba; (void)scale_x; (void)scale_y; assert(!"p4_resources_draw must not be called"); return 0; }
int32_t p4_game_tables_text_draw_geometry(uint32_t stream, float x, float y, float depth, uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline, uint32_t horizontal_128, uint32_t vertical_128)
{ (void)stream; (void)x; (void)y; (void)depth; (void)color; (void)style; (void)effect; (void)flags; (void)multiline; (void)horizontal_128; (void)vertical_128; assert(!"p4_game_tables_text_draw_geometry must not be called"); return 0; }
int p4_game_knowledge_native_analyze(void *defender, const P4GameActionElement *element, uint32_t *known)
{ (void)defender; (void)element; (void)known; assert(!"p4_game_knowledge_native_analyze must not be called"); return 0; }
int p4_game_elements_native_profile_base_affinity(void *unit, uint32_t leaf, uint32_t channel, uint32_t component, uint32_t *native_value)
{ (void)unit; (void)leaf; (void)channel; (void)component; (void)native_value; assert(!"p4_game_elements_native_profile_base_affinity must not be called"); return 0; }
void sdkSpriteDraw(uint8_t *sample, int32_t set_states) { (void)sample; (void)set_states; assert(!"sdkSpriteDraw must not be called"); }
void btlUiSetScale(uint8_t *work, float x, float y) { (void)work; (void)x; (void)y; assert(!"btlUiSetScale must not be called"); }
void btlUiDrawSpriteFaded(uint8_t *work, int32_t slot, int32_t record, float x, float y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{ (void)work; (void)slot; (void)record; (void)x; (void)y; (void)red; (void)green; (void)blue; (void)alpha; assert(!"btlUiDrawSpriteFaded must not be called"); }

int main(void)
{
    AilmentColumn columns[AILMENT_COLUMNS];
    active = (const P4GameElementUi *)(uintptr_t)1;
    assert(ailment_columns((void *)(uintptr_t)1, columns) == 7);
    side_enabled = 1;
    assert(ailment_columns((void *)(uintptr_t)1, columns) == 9);
    side_enabled = 0;
    side_registered = 0;
    assert(ailment_columns((void *)(uintptr_t)1, columns) == 9);
    return 0;
}
