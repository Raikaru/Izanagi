/* Synthetic adapter callbacks exercise ordering/refusal only. */
#include "../src/platform/golden/golden_mode_boot.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static unsigned converted, published, rolled_back;
static int convert(void *context, char *reason, size_t capacity)
{ (void)reason; (void)capacity; if (context != &converted) return -1; ++converted; return 0; }
static int reject(void *context, char *reason, size_t capacity)
{ (void)context; if (reason && capacity) snprintf(reason, capacity, "synthetic refusal"); return -1; }
static int publish(void *context, P4GameTablesDefinitionBuilder *builder, char *reason, size_t capacity)
{ (void)reason; (void)capacity; if (context != &converted || !builder) return -1; ++published; return 0; }
static void rollback(void *context) { if (context == &converted) ++rolled_back; }
int main(void)
{
    P4GameTablesDefinitionBuilder builder = {0}; char reason[160] = "";
    P4GoldenModeAdapter entries[] = {
        {"verified sources and projections", convert}, {"EFFECT0 Golden rows (472x12)", convert},
        {"MODEL0 Golden rows (11x382)", convert}, {"AICALC9/10 compatibility", convert},
        {"expanded enemy model sources", convert}, {"expanded item icon sources", convert},
        {"expanded item categories and actions", convert}, {"expanded item models", convert},
        {"expanded entity names and descriptions", convert}, {"passive skill semantic elements", convert},
        {"widened skill flags", convert}, {"skill and persona name widths", convert},
        {"R2 EFFECT0 packet timing", convert},
        {"M8f-res:battle-common-14A8-1490", convert},
        {"R7a-dep:M11-skill-card-UI-U1-U4", convert},
        {"R7a-dep:M11-return-field-flow", convert},
        {"R7a-dep:shop-permissions-context-flags-U2-U3-U5", convert},
        {"P10-dep:E2-forced-retry-liveness", convert},
        {"Golden BF command compatibility", convert},
        {"Golden script feature compatibility", convert},
        {"native Golden asset families", convert},
        {"image supplier ledger", convert},
        {"M6 image-table cache binder (chart, gate, recipe, EXP, inheritance)", convert}
    };
    P4GoldenModeAdapters ops = {entries, sizeof entries / sizeof entries[0], publish, rollback, &converted};
    CHECK(!p4_golden_mode_boot_prepare(P4_GAME_MODE_PS2, NULL, NULL, reason, sizeof reason));
    CHECK(!converted && !published);
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, NULL, reason, sizeof reason) == -1);
    CHECK(!converted && !published && strstr(reason, "M6 image-table cache binder"));
    --ops.count;
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
    CHECK(!converted && !published && strstr(reason, "M6 image-table cache binder") && strstr(reason, "inheritance"));
    ++ops.count;
    /* R12 cannot silently lift the independent undecided-literal ledger. */
    --ops.count;
    entries[ops.count-1].name = entries[ops.count].name;
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
    CHECK(!converted && !published && strstr(reason, "required Golden table adapters"));
    entries[ops.count-1].name = "image supplier ledger";
    ++ops.count;
    {
        const size_t last = ops.count - 1;
        P4GoldenModeAdapter timing = entries[12];
        entries[12] = entries[last]; --ops.count;
        CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
        CHECK(!converted && !published && strstr(reason, "R2 EFFECT0 packet timing"));
        entries[12] = timing; ++ops.count;
    }
    {
        const size_t last = ops.count - 1;
        P4GoldenModeAdapter resource = entries[13];
        entries[13] = entries[last]; --ops.count;
        CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
        CHECK(!converted && !published && strstr(reason, "M8f-res:battle-common-14A8-1490"));
        entries[13] = resource; ++ops.count;
    }
    for (size_t i = 18; i < 21; ++i) {
        const size_t last = ops.count - 1;
        P4GoldenModeAdapter missing = entries[i];
        entries[i] = entries[last]; --ops.count;
        CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
        CHECK(!converted && !published && strstr(reason, "required Golden table adapters"));
        entries[i] = missing; ++ops.count;
    }
    CHECK(!p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason));
    CHECK(converted == sizeof entries / sizeof entries[0] && published == 1);
    converted = published = 0; entries[2].convert = reject;
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
    CHECK(converted == 2 && !published && rolled_back == 1 && strstr(reason, "synthetic refusal"));
    entries[2].convert = convert; entries[1].name = entries[0].name; converted = 0;
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &ops, reason, sizeof reason) == -1);
    CHECK(!converted && strstr(reason, "duplicate"));
    CHECK(!p4_golden_mode_adapter_register("EFFECT0 Golden rows (472x12)", convert));
    CHECK(p4_golden_mode_adapter_register("EFFECT0 Golden rows (472x12)", convert) == -1);
    CHECK(p4_golden_mode_adapter_register("unknown", convert) == -1);
    CHECK(!p4_golden_mode_publisher_register(publish, rollback, &converted));
    CHECK(p4_golden_mode_registered()->count == 1);
    CHECK(p4_golden_mode_adapter_register("MODEL0 Golden rows (11x382)", convert) == -1);
    puts("golden_mode_boot: ok"); return 0;
}
