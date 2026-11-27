#ifndef P4_GOLDEN_AI_PRODUCT_H
#define P4_GOLDEN_AI_PRODUCT_H
#include "golden_vm.h"
#include "p4pc/golden_ai_boot.h"
#include "p4pc/golden_source.h"
#include "p4pc/golden_domains.h"
#include "p4pc/golden_levels.h"
enum P4GoldenAiRaw { P4_GAI_ITEM, P4_GAI_LEVELS, P4_GAI_PERSONA,
    P4_GAI_PROGRAMS, P4_GAI_SKILL, P4_GAI_UNIT, P4_GAI_RAW_COUNT };
typedef struct P4GoldenAiProduct {
    const P4GameTablesDefinitionBuilder *builder;
    P4GoldenDomainMap map;
    P4GoldenCacheTable raw[P4_GAI_RAW_COUNT];
    P4GoldenSpan skill[12], unit[12], persona[12], programs[12];
    P4GoldenVmProgram scripts[2];
    uint32_t cache_origin;
    unsigned consumers;
} P4GoldenAiProduct;
int p4_golden_ai_product_acquire(const P4GoldenAiProduct *product);
void p4_golden_ai_product_detach(const P4GoldenAiProduct *product);
/* Source-owner teardown, after all runtime consumers detach. */
int p4_golden_ai_boot_forget(const P4GoldenAiProduct *product);
#endif
