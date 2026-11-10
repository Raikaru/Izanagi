/* Shared retail/port controller: task readiness and character-specific battle plans. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/walk_script.h"

#define NODE 0x00120000u
#define CONTEXT 0x00130000u
#define ACTION 0x00140000u
#define YOSUKE 0x00150000u
#define HERO 0x00160000u
#define PLAYER_WORK 0x00170000u
#define PLAYER_MATRIX 0x00180000u
static int task_present;
static int battle_present, battle_waiting, actor_id;
static int player_present;
static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "walk_tasks: line %d: %s\n", __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static uint32_t read32(void *user, uint32_t address)
{
    static const char names[3][24] = {"Y_FclShopMainProc", "battle", "rotate pc"};
    uint32_t value = 0;

    (void)user;
    if (address == 0x00764ADCu) {
        return task_present || battle_present || player_present ? NODE : 0;
    }
    if (address >= NODE && address < NODE + sizeof names[0]) {
        memcpy(&value, names[player_present ? 2 : battle_present] + address - NODE, sizeof value);
    } else if (address == NODE + 0x38) {
        return player_present ? PLAYER_WORK : 0;
    } else if (address == PLAYER_WORK + 0x48) {
        return PLAYER_MATRIX;
    } else if (address == 0x0076449Cu) {
        return CONTEXT;
    } else if (address == CONTEXT + 0x2FC) {
        return 8;
    } else if (address == CONTEXT + 0xC) {
        return battle_waiting ? 0 : 0x400000u;
    } else if (address == CONTEXT + 0x104) {
        return ACTION;
    } else if (address == ACTION + 0xC) {
        return battle_waiting ? 5u : 1u; /* native COMMAND / STANDBY, independent of context state 8 */
    } else if (address == ACTION + 0x30) {
        return actor_id == 1 ? HERO : YOSUKE;
    } else if (address == CONTEXT + 0x178) {
        return YOSUKE; /* reverse character order: overrides must not use list indices */
    } else if (address == YOSUKE + 0xA6C) {
        return HERO;
    } else if (address == HERO + 0xA4) {
        return 1;
    } else if (address == YOSUKE + 0xA4) {
        return 2;
    }
    return value;
}

static void check_plan(P4Walk *walk, P4WalkHost *host, unsigned begin, unsigned end,
                       unsigned first_press, const unsigned *buttons, unsigned count)
{
    for (unsigned frame = begin; frame <= end; frame++) {
        unsigned elapsed = frame - first_press;
        uint32_t expected = frame >= first_press && elapsed / 14 < count && elapsed % 14 < 5
                                ? 1u << buttons[elapsed / 14] : 0;
        CHECK(p4walk_frame(walk, host, frame) == expected);
    }
}

int main(void)
{
    P4WalkHost host = {0};
    P4Walk *walk;
    char error[128];
    unsigned frame;

    host.read32 = read32;
    /* A finite push must release on schedule after an automatic event takes control. */
    player_present = 1;
    walk = p4walk_parse("push:0,1000,5\npress:cross\n", error, sizeof error);
    CHECK(walk != NULL);
    if (walk == NULL) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    CHECK(p4walk_frame(walk, &host, 1) == 0); /* camera data is intentionally absent */
    player_present = 0;
    for (frame = 2; frame <= 5; frame++) {
        CHECK(p4walk_frame(walk, &host, frame) == 0);
        CHECK(p4walk_status(walk) == P4WALK_RUNNING);
    }
    CHECK(p4walk_frame(walk, &host, 6) == (1u << P4W_CROSS));
    for (frame = 7; frame <= 12; frame++) (void)p4walk_frame(walk, &host, frame);
    CHECK(p4walk_status(walk) == P4WALK_DONE);
    p4walk_free(walk);

    walk = p4walk_parse("skiptask:Y_FclShopMainProc\nwaittask:Y_FclShopMainProc:off\n", error, sizeof error);
    CHECK(walk != NULL);
    if (walk == NULL) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    for (frame = 1; frame <= 25; frame++) {
        uint32_t expected = (frame - 1) % 24 < 3 ? 1u << P4W_CROSS : 0;
        CHECK(p4walk_frame(walk, &host, frame) == expected);
    }
    task_present = 1;
    CHECK(p4walk_frame(walk, &host, 26) == 0); /* do not buy from the newly opened menu */
    CHECK(p4walk_status(walk) == P4WALK_RUNNING);
    CHECK(p4walk_frame(walk, &host, 27) == 0); /* waiting for closure never presses Cross */
    task_present = 0;
    CHECK(p4walk_frame(walk, &host, 28) == 0);
    CHECK(p4walk_status(walk) == P4WALK_DONE);
    p4walk_free(walk);

    walk = p4walk_parse("waittask:Y_FclShopMainProc\npress:circle:3\n", error, sizeof error);
    CHECK(walk != NULL);
    if (walk == NULL) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    CHECK(p4walk_frame(walk, &host, 1) == 0);
    task_present = 1;
    for (frame = 2; frame <= 8; frame++) {
        uint32_t expected = frame < 5 ? 1u << P4W_CIRCLE : 0;
        CHECK(p4walk_frame(walk, &host, frame) == expected);
    }
    CHECK(p4walk_status(walk) == P4WALK_DONE);
    p4walk_free(walk);

    task_present = 0;
    battle_present = battle_waiting = actor_id = 1;
    walk = p4walk_parse("battle:first=cross,down,cross,cross;first2=cross,cross,cross;plan=cross,cross,cross\nwait:300\n",
                        error, sizeof error);
    CHECK(walk != NULL);
    if (walk == NULL) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    {
        const unsigned hero_first[] = {P4W_CROSS, P4W_DOWN, P4W_CROSS, P4W_CROSS};
        const unsigned repeat[] = {P4W_CROSS, P4W_CROSS, P4W_CROSS};
        check_plan(walk, &host, 1, 70, 21, hero_first, 4);
        battle_waiting = 0;
        (void)p4walk_frame(walk, &host, 71);
        battle_waiting = 1;
        actor_id = 2;
        check_plan(walk, &host, 72, 128, 92, repeat, 3); /* first2 overrides the generic first */
        battle_waiting = 0;
        (void)p4walk_frame(walk, &host, 129);
        battle_waiting = actor_id = 1;
        check_plan(walk, &host, 130, 186, 150, repeat, 3); /* later Yu turns use plan, not first */
        battle_present = 0;
        CHECK(p4walk_frame(walk, &host, 187) == 0);
        battle_present = 1;
        check_plan(walk, &host, 188, 257, 208, hero_first, 4); /* a new battle resets first turns */
    }
    p4walk_free(walk);
    {
        static const char *invalid[] = {"first0", "first9", "first10", "firstx"};
        for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
            char script[80];
            snprintf(script, sizeof script, "battle:%s=cross;plan=cross\n", invalid[i]);
            walk = p4walk_parse(script, error, sizeof error);
            CHECK(walk == NULL);
            p4walk_free(walk);
        }
    }
    if (failures == 0) {
        puts("walk_tasks: task readiness, Cross release, Circle exit and character plans passed");
    }
    return failures != 0;
}
