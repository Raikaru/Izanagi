#ifndef P4_GOLDEN_MODE_DECLARATIONS_H
#define P4_GOLDEN_MODE_DECLARATIONS_H
#include "p4pc/golden_domains.h"

/* All rows are converted and indexed by port ID. Enemy model_sources and
 * item_icons must be independently approved resources, never guessed. */
typedef struct P4GoldenModeDeclarationInputs {
    const uint32_t *skill_element_ids;
    const uint8_t *skill_element_aux, *skill_metadata, *skill_presentation;
    const int8_t *skill_rates;
    const uint8_t *skill_inheritance_groups;
    size_t skill_count, active_count;
    const uint8_t *enemy_metadata, *enemy_affinity, *enemy_models, *enemy_ai;
    const uint32_t *enemy_model_sources;
    size_t enemy_count;
    const uint8_t *item_metadata;
    const P4ResourceRef *item_icons;
    size_t item_count;
    const P4GamePersonaDesc *personas;
    size_t persona_count;
} P4GoldenModeDeclarationInputs;

int p4_golden_mode_declare(P4GameMode mode, const P4GoldenDomainMap *map, const P4GoldenModeDeclarationInputs *inputs,
                           const P4GoldenDeclareSink *sink, char *reason, size_t capacity);
#endif
