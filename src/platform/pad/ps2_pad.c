/* libpad entry points for the PC port. These are the nine libpad functions the decomp
 * calls: scePadInit, scePadPortOpen, scePadRead, scePadGetState, scePadSetMainMode,
 * scePadInfoAct, scePadSetActAlign, scePadGetReqState and scePadSetActDirect.
 *
 * Model: port 0 slot 0 is one virtual DualShock 2, the keyboard plus the first
 * gamepad (see pad_sdl.h). Port 1 is disconnected. Mode/alignment requests
 * complete asynchronously in game frames, matching retail's boot negotiation:
 * SetMainMode at frame 1 stays busy through 9; SetActAlign at 11 completes at 13,
 * so sdkPadUpdateRecord first reads the controller at frame 14, not frame 4.
 * The virtual pad exposes both actuator slots for that negotiation; actual host
 * motor output remains unsupported (SetActDirect returns failure).
 *
 * Packet (scePadRead, 32 bytes, DualShock 2 layout):
 *   [0] 0x00 status OK   [1] mode id (0x41 digital, 0x79 analog pressure)
 *   [2],[3] buttons, active-low: [2] select/L3/R3/start/d-pad, [3] L2/R2/L1/R1/triangle/circle/cross/square
 *   [4..7] RX, RY, LX, LY   [8..19] pressure, order in pad_sdl.h */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pad_sdl.h"
#include "p4pc/guest.h"
#include "p4pc/lockstep.h"
#if defined(P4_MOD_GAME_BINDINGS) && defined(P4_MOD_LUAU)
#include "p4pc/mod_script.h"
#include "p4pc/mod_services.h"
#include "p4pc/platform.h"
#endif

#define P4PC_PAD_PORTS 2
#define P4PC_PAD_STATE_STABLE 6
#define P4PC_PAD_STATE_DISCONNECTED 0
#define P4PC_PAD_STATE_EXECCMD 5
#define P4PC_PAD_MODE_DIGITAL 0x41
#define P4PC_PAD_MODE_ANALOG 0x79

static int sPortOpen[P4PC_PAD_PORTS];
static int sAnalog[P4PC_PAD_PORTS];
static int sInitResult = -1; /* -1 not yet, 0 ok, 1 failed */
static uint16_t sLastButtons;
static int sLastValid;
static uint32_t sRequestEnd[P4PC_PAD_PORTS];
static int sRequestPending[P4PC_PAD_PORTS];
static void (*sPacketHook)(uint8_t *, size_t);

void p4pc_pad_set_packet_hook(void (*hook)(uint8_t *, size_t))
{
    sPacketHook = hook;
}

static int padConnected(int32_t port, int32_t slot)
{
    return port == 0 && slot == 0 && sPortOpen[0];
}

static uint32_t padFrame(void)
{
    return *(const uint32_t *)P4_G2H(0x00764290u);
}

static int padRequestState(int32_t port)
{
    if (sRequestPending[port]) {
        if ((int32_t)(padFrame() - sRequestEnd[port]) < 0) {
            return 2; /* PAD_RSTAT_BUSY */
        }
        sRequestPending[port] = 0;
    }
    return 0; /* PAD_RSTAT_COMPLETE */
}

static void padRequest(int32_t port, uint32_t frames)
{
    sRequestEnd[port] = padFrame() + frames;
    sRequestPending[port] = 1;
}

int32_t func_0042f6f8(int32_t mode)
{
    (void)mode;
    if (sInitResult < 0) {
        sInitResult = p4pc_pad_sdl_init() == 0 ? 0 : 1;
    }
    return sInitResult == 0 ? 1 : 0;
}

int32_t func_0042fb50(int32_t port, int32_t slot, uint8_t *padbuf)
{
    (void)padbuf;
    if (port < 0 || port >= P4PC_PAD_PORTS || slot != 0) {
        return 0;
    }
    if (port == 0) {
        sPortOpen[0] = 1;
    }
    return 1;
}

int32_t func_00430018(int32_t port, int32_t slot)
{
    if (!padConnected(port, slot)) {
        return P4PC_PAD_STATE_DISCONNECTED;
    }
    return padRequestState(port) == 2 ? P4PC_PAD_STATE_EXECCMD : P4PC_PAD_STATE_STABLE;
}

int32_t func_00430578(int32_t port, int32_t slot, int32_t mode, int32_t lock)
{
    (void)lock;
    if (!padConnected(port, slot) || padRequestState(port) == 2) {
        return 0;
    }
    sAnalog[port] = mode != 0;
    padRequest(port, 9);
    return 1;
}

int32_t func_00430210(int32_t port, int32_t slot, int32_t actNum, int32_t term)
{
    /* -1 queries the number of actuator slots (retail DS2: two). */
    (void)term;
    if (!padConnected(port, slot)) {
        return 0;
    }
    return actNum == -1 ? 2 : 0;
}

int32_t func_004306f8(int32_t port, int32_t slot, void *align)
{
    (void)align;
    if (!padConnected(port, slot) || padRequestState(port) == 2) {
        return 0;
    }
    padRequest(port, 2);
    return 1;
}

int32_t func_00430180(int32_t port, int32_t slot, uint16_t state)
{
    (void)state;
    if (!padConnected(port, slot)) {
        return 1; /* PAD_RSTAT_FAILED */
    }
    return padRequestState(port);
}

int32_t func_00430630(int32_t port, int32_t slot, int8_t *actuator)
{
    (void)actuator;
    (void)port;
    (void)slot;
    return 0; /* host rumble output is not implemented */
}

static void tracePad(uint16_t buttons)
{
    if (!sLastValid || buttons != sLastButtons) {
        fprintf(stderr, "p4pc pad: port0 buttons 0x%04x\n", (unsigned)buttons);
        fflush(stderr);
        sLastButtons = buttons;
        sLastValid = 1;
    }
}

void p4pc_pad_build_packet(const P4PadSnapshot *snap, int analog, uint8_t *out)
{
    int i;

    memset(out, 0, 32);
    out[0] = 0x00;
    out[1] = analog ? P4PC_PAD_MODE_ANALOG : P4PC_PAD_MODE_DIGITAL;
    /* libpad's layout, which sdkPadUpdateRecord reads as ~out[3] | ~out[2] << 8: out[2] carries select, L3, R3,
     * start and the d-pad (the snapshot's low byte), out[3] L2, R2, L1, R1 and the four face buttons (its high
     * byte). The game then sees cross as 0x40 and start as 0x800, which its menus test. */
    out[2] = (uint8_t)~snap->buttons;
    out[3] = (uint8_t)(~snap->buttons >> 8);
    out[4] = snap->rx;
    out[5] = snap->ry;
    out[6] = snap->lx;
    out[7] = snap->ly;
    for (i = 0; i < P4PC_PRESS_COUNT; i++) {
        out[8 + i] = snap->pressure[i];
    }
}

int32_t func_0042ffa0(int32_t port, int32_t slot, uint8_t *out)
{
    P4PadSnapshot snap;

    if (!padConnected(port, slot)) {
        return 0;
    }
    p4pc_pad_sdl_read(&snap);
    p4pc_pad_build_packet(&snap, sAnalog[0], out);
    if (sPacketHook != NULL) sPacketHook(out, 32);
#if defined(P4_MOD_GAME_BINDINGS) && defined(P4_MOD_LUAU)
    if (p4_mod_input_enabled)
        p4_mod_input_sample(out, p4pc_game_frame_count());
    /* An exclusive mod camera owns the pad after the mod has sampled it. */
    if (p4_mod_input_fence)
        p4_mod_input_fence_apply(out);
#endif
    p4pc_pad_trace_packet(out);
    tracePad((unsigned)((uint16_t)(out[2] | (out[3] << 8)) ^ 0xffffu));
    return 32; /* libpad returns packet bytes, not a boolean */
}
