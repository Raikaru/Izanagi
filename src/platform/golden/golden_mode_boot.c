/* Core-only Golden table adapter admission. No package can supply adapters. */
#include "golden_mode_boot.h"
#include <stdio.h>
#include <string.h>

static const char *const required[] = {
    "verified sources and projections", "EFFECT0 conversion (472x12 to 440x4)",
    "MODEL0 conversion (11x382 to 11x332)",
    "AICALC9/10 compatibility", "expanded enemy model sources", "expanded item icon sources",
    "expanded item categories, actions and models",
    "expanded entity names and descriptions",
    "passive skill semantic elements", "widened skill flags", "skill and persona name widths",
    "image-table binding"
};
enum { REQUIRED_COUNT = sizeof required / sizeof required[0] };
static P4GoldenModeAdapter registered_items[REQUIRED_COUNT];
static P4GoldenModeAdapters registered = {registered_items, 0, NULL, NULL, NULL};
static int frozen;
int p4_golden_mode_adapter_register(const char *name, P4GoldenModeConvert convert)
{
    if (frozen || !name || !convert || registered.count == REQUIRED_COUNT) return -1;
    size_t i = 0;
    while (i < REQUIRED_COUNT && strcmp(required[i], name)) ++i;
    if (i == REQUIRED_COUNT) return -1;
    for (size_t j = 0; j < registered.count; ++j)
        if (!strcmp(registered_items[j].name, name)) return -1;
    registered_items[registered.count++] = (P4GoldenModeAdapter){required[i], convert};
    return 0;
}
int p4_golden_mode_publisher_register(P4GoldenModePublish publish, void (*rollback)(void *), void *context)
{
    if (frozen || !publish || !rollback || registered.publish) return -1;
    registered.publish = publish; registered.rollback = rollback; registered.context = context; return 0;
}
const P4GoldenModeAdapters *p4_golden_mode_registered(void)
{
    frozen = 1;
    return &registered;
}
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_mode_boot_prepare(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                 const P4GoldenModeAdapters *adapters, char *reason, size_t capacity)
{
    const P4GoldenModeAdapter *resolved[sizeof required / sizeof required[0]] = {0};
    size_t missing = 0;
    if (mode == P4_GAME_MODE_PS2) return 0;
    if (mode != P4_GAME_MODE_GOLDEN) return fail(reason, capacity, "invalid game mode");
    if (adapters && adapters->count && !adapters->items) return fail(reason, capacity, "invalid Golden adapter registry");
    for (size_t i = 0; adapters && i < adapters->count; ++i) {
        const P4GoldenModeAdapter *a = &adapters->items[i];
        if (!a->name || !a->convert) return fail(reason, capacity, "invalid Golden adapter");
        size_t j = 0;
        while (j < sizeof required / sizeof required[0] && strcmp(required[j], a->name)) ++j;
        if (j == sizeof required / sizeof required[0] || resolved[j])
            return fail(reason, capacity, "unknown or duplicate Golden adapter");
        resolved[j] = a;
    }
    for (size_t i = 0; i < sizeof required / sizeof required[0]; ++i) {
        if (!resolved[i]) {
            fprintf(stderr, "p4pc: Golden mode refuses unavailable adapter: %s\n", required[i]);
            ++missing;
        }
    }
    if (!adapters || !adapters->publish || !adapters->rollback) {
        fprintf(stderr, "p4pc: Golden mode refuses unavailable adapter: cache publication and declarations\n");
        ++missing;
    }
    if (missing) return fail(reason, capacity, "required Golden table adapters are unavailable");
    if (!builder) return fail(reason, capacity, "no Golden table boot builder");
    for (size_t i = 0; i < sizeof required / sizeof required[0]; ++i) {
        if (resolved[i]->convert(adapters->context, reason, capacity)) {
            adapters->rollback(adapters->context);
            return -1;
        }
    }
    int result = adapters->publish(adapters->context, builder, reason, capacity);
    if (result) adapters->rollback(adapters->context);
    return result;
}
