/* The pad host must not query freed collision geometry through a surviving
 * rotate-pc task. Exercise the production registration and script reader. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guest_test.h"
#include "../src/platform/pad/pad_sdl.h"

static unsigned failures, groundCalls, sphereCalls;
static uint32_t frame;
static const uint32_t scene = 0x00110000u;
static const uint32_t player = 0x00120000u;
static const uint32_t work = 0x00121000u;
static const uint32_t matrix = 0x00122000u;

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
    failures++; \
} } while (0)

/* Witness callees stand in for the native geometry queries. Entering either
 * after sceneTask is cleared is the original use-after-unload failure. */
int32_t FldFrame_RaycastField(const float *line, float *hit)
{
    CHECK(P4_G32(scene + 0x1Cu) != 0);
    groundCalls++;
    hit[0] = line[0];
    hit[1] = 0.0f;
    hit[2] = line[2];
    return 1;
}

int32_t FldFrame_CollideSphereSelected(const float *point, float radius, float *normal, float *vector)
{
    (void)point;
    (void)radius;
    (void)normal;
    (void)vector;
    CHECK(P4_G32(scene + 0x1Cu) != 0);
    sphereCalls++;
    return 0;
}

static uint32_t clockFrame(void)
{
    return frame;
}

static uint16_t readPad(uint32_t at)
{
    P4PadSnapshot pad;

    frame = at;
    p4pc_pad_sdl_read(&pad);
    return pad.buttons;
}

int main(int argc, char **argv)
{
    unsigned queries;

    if (argc != 2 || p4t_guest_map(0x00100000u, 0x01000000u) != 0) {
        return 2;
    }
    if (p4_os_setenv("P4PC_INPUT", "script", 1) != 0 ||
        p4_os_setenv("P4PC_AUTOPRESS", "", 1) != 0 ||
        p4_os_setenv("P4PC_WALK_SCRIPT", argv[1], 1) != 0) {
        return 2;
    }
    p4pc_pad_autopress_set_clock(clockFrame);
    p4pc_pad_autopress_set_origin(0, 0.0);

    /* The shared walker holds a press for at least three frames, followed by
     * three released frames. Finish title input before withdrawing the scene. */
    CHECK((readPad(0) & P4PC_PAD_CROSS) != 0);
    CHECK((readPad(1) & P4PC_PAD_CROSS) != 0);
    CHECK((readPad(2) & P4PC_PAD_CROSS) != 0);
    for (uint32_t at = 3; at <= 6; at++) {
        CHECK(readPad(at) == 0);
    }
    CHECK(groundCalls == 0 && sphereCalls == 0);

    P4_G32(P4_GPADDR(0x9db0)) = scene;
    P4_G32(scene) = 0xFFFFFFFFu;
    P4_G32(scene + 4) = 0xFFFFFFFFu;
    P4_G32(scene + 0x24) = 8; /* teardown leaves the old template list behind */
    P4_G32(scene + 0x28) = 0x00DEAD00u;
    P4_G32(0x00764ADCu) = player;
    memcpy(P4_G2H(player), "rotate pc", sizeof "rotate pc");
    P4_G32(player + 0x38) = work;
    P4_G32(work + 0x48) = matrix;
    P4_GF32(matrix + 0x28) = 1.0f;

    for (uint32_t at = 7; at < 11; at++) {
        CHECK(readPad(at) == 0);
    }
    CHECK(groundCalls == 0 && sphereCalls == 0);

    /* A newly created scene manager publishes the loaded collision world. */
    P4_G32(scene) = 41;
    P4_G32(scene + 4) = 0;
    P4_G32(scene + 0x1C) = 0x00130000u;
    CHECK((readPad(11) & P4PC_PAD_CIRCLE) != 0);
    CHECK(groundCalls == 1 && sphereCalls != 0);
    queries = groundCalls + sphereCalls;

    /* Unloading again releases even an already-held script button. */
    P4_G32(scene + 0x1C) = 0;
    P4_G32(scene) = 0xFFFFFFFFu;
    P4_G32(scene + 4) = 0xFFFFFFFFu;
    CHECK(readPad(12) == 0);
    CHECK(groundCalls + sphereCalls == queries);

    /* Removing the old player must not prevent non-field script input. */
    P4_G32(0x00764ADCu) = 0;
    (void)readPad(13);
    CHECK(groundCalls + sphereCalls == queries);

    if (failures == 0) {
        puts("nav_probe: title input, stale-player unload fence, loaded-world resume and button release passed");
    }
    return failures != 0;
}
