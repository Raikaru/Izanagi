#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "guest_test.h"
#include "../src/platform/capture_state.c"

static int failures;
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); failures++; } } while (0)

static void word(uint32_t address, uint32_t value) { P4_G32(address) = value; }

static void procedure_tests(void)
{
    CaptureState state;
    const uint32_t script = 0x01000000u;
    const char *spec = "proc,sdl04_14_PM_C,105,20323,118,900,1200";
    word(0x00764608u, script);
    strcpy((char *)P4_G2H(script), "sdl04_14_PM_C");
    word(script + 0x12c, 105);
    word(script + 0x150, 0);
    /* The same procedure resets cmdTimer at many earlier instructions. */
    word(script + 0x18, 20200);
    word(script + 0x138, 118);
    CHECK(captureParse(&state, spec, 30));
    CHECK(captureObserve(&state, 800) == 0 && !state.draw && !state.capture);
    word(script + 0x18, 20323);
    CHECK(captureParse(&state, spec, 30));
    word(script + 0x138, 86);
    CHECK(captureObserve(&state, 1000) == 0 && !state.draw && !state.capture);
    word(script + 0x138, 87);
    CHECK(captureObserve(&state, 1001) == 0 && state.draw && !state.capture);
    word(script + 0x12c, 104);
    word(script + 0x138, 118);
    CHECK(captureObserve(&state, 1002) == 0 && !state.capture);
    word(script + 0x12c, 105);
    word(script + 0x18, 20322);
    CHECK(captureObserve(&state, 1003) == 0 && !state.capture);
    word(script + 0x18, 20323);
    CHECK(captureObserve(&state, 1004) == 0 && state.capture == 1004);
    CHECK(captureObserve(&state, 1005) == 0 && state.capture == 1004);

    /* A legitimate one-flip route change selects the same procedure phase, not the old absolute flip. */
    CHECK(captureParse(&state, spec, 30));
    CHECK(captureObserve(&state, 1006) == 0 && state.capture == 1006);
    sCapture = state;
    CHECK(p4pc_capture_state_wanted(1006));
    CHECK(!p4pc_capture_state_gate(1006));
    CHECK(p4pc_capture_state_gate(1007) == 1);

    CHECK(captureParse(&state, spec, 30));
    CHECK(captureObserve(&state, 899) == -1);
    CHECK(captureParse(&state, spec, 30));
    word(script + 0x138, 117);
    CHECK(captureObserve(&state, 1201) == -1);
}

static void event_tests(void)
{
    CaptureState state;
    const char *spec = "event,upstair_myroom,6,9068,154,1,0,190,16,0,10,2162944,900,1200";
    const uint32_t script = 0x01000000u, task = 0x01001000u, driver = 0x01001100u;
    const uint32_t child = 0x01001200u, work = 0x01001300u;
    const uint32_t entry = 0x01002000u, window = 0x01002100u;
    const uint32_t manager = 0x01002200u, banks = 0x01002300u;
    strcpy((char *)P4_G2H(script), "upstair_myroom");
    word(script + 0x12c, 6); word(script + 0x18, 9068);
    word(script + 0x138, 9999); /* Asset waits do not stop the enclosing BF timer. */
    word(0x00764adcu, task);
    strcpy((char *)P4_G2H(task), "evtEvent");
    word(task + 0x38, driver); word(task + 0x3c, 0);
    word(driver, 4); word(driver + 0x0c, child); word(child + 0x38, work);
    word(work + 0x78c, 154); word(work + 0x790, 1); word(work + 0x794, 0);
    word(work + 0x678, 1); word(work + 0x680, 17);
    word(work + 0x684, 2); word(work + 0x688, 16);
    word(0x008817ecu, entry); word(entry + 4, 0);
    word(entry + 8, 17); word(entry + 0x0c, window);
    word(0x008817e0u, 1); /* Active-window count is not an upper handle bound. */
    word(window, 0x00200204); word(window + 0x2c, 0); word(window + 0x30, 10);
    word(0x00764678u, manager); word(manager + 0x24, banks);
    word(banks + 4, 0x0003000d); word(banks + 8, 0x00210100);
    CHECK(captureParse(&state, spec, 150));
    word(work + 0x14, 38);
    CHECK(captureObserve(&state, 1000) == 0 && !state.draw && !state.capture);
    word(work + 0x14, 39);
    CHECK(captureObserve(&state, 1001) == 0 && state.draw && !state.capture);
    word(work + 0x14, 190); word(work + 0x78c, 155);
    CHECK(captureObserve(&state, 1002) == 0 && !state.capture);
    word(work + 0x78c, 154); word(work + 0x688, 15);
    CHECK(captureObserve(&state, 1003) == 0 && !state.capture);
    word(work + 0x688, 16); word(window, 0x00200102);
    CHECK(captureObserve(&state, 1004) == 0 && !state.capture);
    word(window, 0x00200204); word(window + 0x2c, 1);
    CHECK(captureObserve(&state, 1005) == 0 && !state.capture);
    word(window + 0x2c, 0); word(window + 0x30, 9);
    CHECK(captureObserve(&state, 1006) == 0 && !state.capture);
    word(window + 0x30, 10); word(banks + 4, 0x00000203);
    CHECK(captureObserve(&state, 1007) == 0 && !state.capture);
    word(banks + 4, 0x0003000d); word(banks + 8, 0x00230100);
    CHECK(captureObserve(&state, 1008) == 0 && !state.capture);
    word(banks + 8, 0x00210100);
    CHECK(captureObserve(&state, 1009) == 0 && state.capture == 1009);
    CHECK(captureParse(&state, spec, 150));
    CHECK(captureObserve(&state, 1010) == 0 && state.capture == 1010);
}

static void camera_fixture(void)
{
    const uint32_t scene = 0x01000200u, manager = 0x01000300u, resource = 0x01000400u;
    const uint32_t matrix = 0x01000800u, camera = 0x01000900u, frame = 0x01000a00u;
    unsigned i;
    word(0x00762ea0u, scene);
    word(scene, 68); word(scene + 4, 1); word(scene + 8, manager);
    word(manager + 4, resource);
    word(resource, 0x400); word(resource + 0x138, 0); word(resource + 0x164, matrix);
    P4_GF32(matrix + 48) = 8640.0f;
    P4_GF32(matrix + 52) = 0.0f;
    P4_GF32(matrix + 56) = 3930.0f;
    word(0x00764b70u, camera); word(camera + 4, frame);
    for (i = 0; i < 16; i++) P4_GF32(frame + 80 + 4 * i) = 0.0f;
    P4_GF32(frame + 80) = P4_GF32(frame + 100) = P4_GF32(frame + 120) = 1.0f;
}

static void camera_tests(void)
{
    CaptureState state;
    const char *spec = "camera,68,1,8640,0,3930,1,1,0,0,0,1,0,0,0,1,0.0001,206,1000,2000";
    camera_fixture();
    CHECK(captureParse(&state, spec, 150));
    word(0x01000200u, 7);
    CHECK(captureObserve(&state, 1100) == 0 && !state.entered);
    word(0x01000200u, 68);
    P4_GF32(0x01000800u + 48) = 8642.0f;
    CHECK(captureObserve(&state, 1101) == 0 && !state.entered);
    P4_GF32(0x01000800u + 48) = 8640.0f;
    P4_GF32(0x01000a00u + 80) = 0.9f;
    CHECK(captureObserve(&state, 1102) == 0 && !state.entered);
    P4_GF32(0x01000a00u + 80) = 1.0f;
    CHECK(captureObserve(&state, 1200) == 0 && state.entered == 1200 && !state.draw);
    CHECK(captureObserve(&state, 1254) == 0 && !state.draw);
    CHECK(captureObserve(&state, 1255) == 0 && state.draw);
    CHECK(captureObserve(&state, 1406) == 0 && state.capture == 1406);
    CHECK(captureParse(&state, spec, 150));
    CHECK(captureObserve(&state, 1200) == 0);
    P4_GF32(0x01000800u + 48) = 8642.0f;
    CHECK(captureObserve(&state, 1201) == -1); /* It must stay inside the registered state. */
}

static void battle_menu_tests(void)
{
    CaptureState state;
    const char *spec = "battle-menu,40,1,4000";
    const uint32_t context = 0x01004000u, actor = 0x01005000u, party = 0x01006000u;
    const uint32_t entry = 0x01007000u, window = 0x01008000u;
    word(0x0076449cu, context);
    word(context + 0x2fc, 6);
    word(context + 0x104, actor); word(context + 0x178, party);
    word(actor + 0x0c, 5); word(actor + 0x30, party); word(party + 0xa6c, 0);
    word(0x008817ecu, 0);
    CHECK(captureParse(&state, spec, 4));
    CHECK(captureObserve(&state, 899) == 0 && !state.entered);
    word(context + 0x2fc, 8);
    CHECK(captureObserve(&state, 900) == 0 && state.entered == 900 && !state.draw);
    /* A state-8 Help page is not the command menu. */
    word(0x008817ecu, entry); word(entry + 4, 0); word(entry + 0x0c, window);
    word(window, 0x00200204); word(window + 0x24, 1); word(window + 0x48, 0xffff);
    CHECK(captureObserve(&state, 901) == 0 && !state.entered && !state.capture);
    /* Native text can retain inactive selection state0 with no choice rows. */
    word(window + 0x48, 0xffff0000u); word(window + 0x4c, 0);
    CHECK(captureObserve(&state, 901) == 0 && !state.entered && !state.capture);
    word(window + 0x4c, 2u << 16); /* Preparing a choice is not COMMAND readiness. */
    CHECK(captureObserve(&state, 901) == 0 && !state.entered && !state.capture);
    word(window + 0x48, 1); word(window, 0);
    CHECK(captureObserve(&state, 901) == 0 && !state.entered && !state.capture);
    word(window + 0x48, 2);
    CHECK(captureObserve(&state, 901) == 0 && !state.entered && !state.capture);
    word(0x008817ecu, 0);
    word(context + 0x0c, 0x400000);
    CHECK(captureObserve(&state, 902) == 0 && !state.entered);
    word(context + 0x0c, 0); word(actor + 0x30, 0x01009000u);
    CHECK(captureObserve(&state, 903) == 0 && !state.entered); /* Not a party actor. */
    word(actor + 0x30, party); word(actor + 0x0c, 6);
    CHECK(captureObserve(&state, 904) == 0 && !state.entered); /* TARGET is not COMMAND. */
    word(actor + 0x0c, 5);
    CHECK(captureObserve(&state, 905) == 0 && state.entered == 905);
    CHECK(captureObserve(&state, 939) == 0 && !state.draw);
    CHECK(captureObserve(&state, 940) == 0 && state.draw && !state.capture);
    CHECK(captureObserve(&state, 945) == 0 && state.capture == 945);
    sCapture = state;
    CHECK(p4pc_capture_state_gate(946) == 1 && p4pc_capture_state_wanted(945));
    /* Loading shifts the entry flip, never the selected menu-relative phase. */
    CHECK(captureParse(&state, spec, 4));
    CHECK(captureObserve(&state, 1800) == 0 && state.entered == 1800);
    CHECK(captureObserve(&state, 1840) == 0 && state.capture == 1840);
    CHECK(captureParse(&state, spec, 4));
    word(context + 0x2fc, 6);
    CHECK(captureObserve(&state, 4001) == -1);
    word(0x0076449cu, 0x01fffff0u);
    CHECK(captureObserve(&state, 100) == 0 && !state.entered);
}

static void battle_command_clock_tests(void)
{
    CaptureState state;
    const uint32_t context = 0x01004000u, actor = 0x01005000u, party = 0x01006000u;
    const uint32_t entry = 0x01007000u, window = 0x01008000u;
    const char *spec = "battle-command,106,1,4000";
    word(0x0076449cu, context); word(context + 0x2fc, 8); word(context + 0x0c, 0);
    word(context + 0x104, actor); word(context + 0x178, party);
    word(actor + 0x0c, 5); word(actor + 0x30, party); word(party + 0xa6c, 0);
    word(0x008817ecu, 0); word(actor + 0x1c, 104);
    CHECK(captureParse(&state, spec, 4));
    CHECK(captureObserve(&state, 900) == 0 && state.draw && !state.capture);
    word(actor + 0x1c, 106); word(0x008817ecu, entry);
    word(entry + 4, 0); word(entry + 0x0c, window);
    word(window, 0x00200204); word(window + 0x24, 1); word(window + 0x48, 0xffff);
    CHECK(captureObserve(&state, 901) == 0 && state.draw && !state.capture);
    word(0x008817ecu, 0);
    CHECK(captureObserve(&state, 902) == 0 && state.capture == 902);
    CHECK(captureParse(&state, spec, 4));
    word(actor + 0x1c, 108);
    CHECK(captureObserve(&state, 903) == -1); /* Never silently choose a later phase. */
}

static void rankup_tests(void)
{
    CaptureState state;
    const uint32_t script = 0x01000000u, task = 0x0100a000u, work = 0x0100b000u;
    const char *spec = "rankup,155_commu_open,0,6,604,7,0,1,7000,8200";
    word(0x00764608u, script);
    strcpy((char *)P4_G2H(script), "155_commu_open");
    word(script + 0x12c, 0); word(script + 0x18, 6); word(script + 0x138, 407);
    word(script + 0x150, 0);
    word(0x00764adcu, task); word(0x00764af4u, 0); word(0x00764ae8u, 0);
    strcpy((char *)P4_G2H(task), "cmmRankUpSequence");
    word(task + 0x38, work); word(task + 0x3c, 0);
    word(work, 0x18); word(work + 4, 10); word(work + 8, 578);
    word(work + 0x18, 7); word(work + 0x1c, 0); word(work + 0x20, 1);
    CHECK(captureParse(&state, spec, 4));
    /* Draw must be armed even below the target: draw advances this clock. */
    CHECK(captureObserve(&state, 7100) == 0 && state.draw && !state.capture);
    word(work + 8, 604); word(work + 0x1c, 1);
    CHECK(captureObserve(&state, 7101) == 0 && state.draw && !state.capture);
    word(work + 0x1c, 0); word(work + 0x18, 6);
    CHECK(captureObserve(&state, 7102) == 0 && !state.capture);
    word(work + 0x18, 7); word(script + 0x18, 5);
    CHECK(captureObserve(&state, 7103) == 0 && !state.draw && !state.capture);
    word(script + 0x18, 6);
    CHECK(captureObserve(&state, 7104) == 0 && state.capture == 7104);
    CHECK(captureParse(&state, spec, 4));
    word(script + 0x138, 465); /* BF timing is deliberately not the predicate. */
    CHECK(captureObserve(&state, 7200) == 0 && state.capture == 7200);
}

static void parser_tests(void)
{
    static const char *bad[] = {
        "", "unknown,x,1,2,3,4,5", "proc,,1,2,3,4,5", "proc,x,1,2,3,5,4", "proc,x,1,2,3,0,4",
        "proc,x,-1,2,3,4,5", "proc,x,1,2,3,4,5,", "proc,x,4294967296,2,3,4,5",
        "proc,abcdefghijklmnopqrstuvwxy,1,2,3,4,5",
        "event,x,1,2,3,4,5,6,7,8,9,10,11",
        "event,x,1,2,3,4,5,6,7,8,9,10,11,12,",
        "event,x,1,2,3,4,5,6,7,8,9,-10,11,12",
        "battle-menu,40,1", "battle-menu,40,1,4000,", "battle-menu,-1,1,4000",
        "battle-menu,40,4000,1", "battle-menu,40,0,4000",
        "rankup,x,0,6,604,7,0,1,7000", "rankup,x,0,6,604,7,0,1,8200,7000",
        "rankup,x,0,6,604,7,0,1,7000,8200,",
        "battle-command,106,1", "battle-command,106,4000,1", "battle-command,106,1,4000,"
    };
    CaptureState state;
    unsigned i;
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) CHECK(!captureParse(&state, bad[i], 150));
    memset(&sCapture, 0, sizeof(sCapture));
    CHECK(p4pc_capture_state_gate(20) == -1 && !p4pc_capture_state_wanted(20) && !p4pc_capture_state_draw());
}

int main(void)
{
    if (p4t_guest_map(0x00760000u, 0x10000u) != 0 ||
        p4t_guest_map(0x00880000u, 0x10000u) != 0 ||
        p4t_guest_map(0x01000000u, 0x10000u) != 0) return 1;
    memset(P4_G2H(0x00760000u), 0, 0x10000u);
    memset(P4_G2H(0x00880000u), 0, 0x10000u);
    memset(P4_G2H(0x01000000u), 0, 0x10000u);
    procedure_tests();
    event_tests();
    camera_tests();
    battle_menu_tests();
    battle_command_clock_tests();
    rankup_tests();
    parser_tests();
    if (!failures) puts("capture_state: procedure/EVT/rank-up phases, camera/player offsets, party COMMAND readiness and bounds PASS");
    return failures != 0;
}
