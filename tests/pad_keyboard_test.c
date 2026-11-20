/* Keyboard-to-pad check. The keyboard bindings turn a key array into the snapshot,
 * and the snapshot into the DualShock 2 packet scePadRead returns. The SDL keyboard
 * state cannot be driven from a test, so this feeds p4pc_pad_keys_to_snapshot a
 * synthetic array; p4pc_pad_sdl_read calls the same function on the live state. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL3/SDL_scancode.h>

#include "../src/platform/pad/pad_sdl.h"
#include "p4pc/hostmath.h"

static int failures;

#define CHECK(c)                                                          \
    do {                                                                  \
        if (!(c)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);  \
            failures++;                                                   \
        }                                                                 \
    } while (0)

/* Snapshot the way p4pc_pad_sdl_read starts it: zeroed, sticks centred. */
static void startSnapshot(P4PadSnapshot *snap)
{
    memset(snap, 0, sizeof *snap);
    snap->lx = snap->ly = snap->rx = snap->ry = 0x80;
}

/* Key array to packet; the snapshot the packet came from is left in snap. */
static void keysToPacket(const bool *keys, int numkeys, uint8_t *packet, int analog, P4PadSnapshot *snap)
{
    startSnapshot(snap);
    p4pc_pad_keys_to_snapshot(keys, numkeys, snap);
    p4pc_pad_build_packet(snap, analog, packet);
}

int main(void)
{
    bool keys[SDL_SCANCODE_COUNT];
    uint8_t packet[32];
    uint8_t other[32];
    P4PadSnapshot snap;

    /* Nothing held: buttons clear, every active-low byte high, sticks centred. */
    memset(keys, 0, sizeof keys);
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    CHECK(snap.buttons == 0);
    CHECK(packet[2] == 0xFF && packet[3] == 0xFF);
    CHECK(packet[4] == 0x80 && packet[5] == 0x80 && packet[6] == 0x80 && packet[7] == 0x80);
    for (int i = 0; i < P4PC_PRESS_COUNT; i++) {
        CHECK(packet[8 + i] == 0);
    }

    /* Z is cross: bit 0x4000 is the snapshot's high byte, which libpad puts in packet[3] (bit 6). */
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_Z] = true;
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    CHECK(snap.buttons == P4PC_PAD_CROSS);
    CHECK(packet[2] == 0xFF && packet[3] == 0xBF);
    CHECK(packet[8 + P4PC_PRESS_CROSS] == 0xFF);

    /* Return is start: the snapshot's low byte, so packet[2] loses 0x08. */
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_RETURN] = true;
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    CHECK(snap.buttons == P4PC_PAD_START);
    CHECK(packet[2] == 0xF7 && packet[3] == 0xFF);

    /* W and Up are both bound to up: one bit, one pressure byte. */
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_W] = true;
    keys[SDL_SCANCODE_UP] = true;
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    CHECK(snap.buttons == P4PC_PAD_UP);
    CHECK(packet[2] == 0xEF);
    CHECK(packet[8 + P4PC_PRESS_UP] == 0xFF);

    /* Combination: cross, triangle and left shift (L3). */
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_Z] = true;
    keys[SDL_SCANCODE_C] = true;
    keys[SDL_SCANCODE_LSHIFT] = true;
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    CHECK(snap.buttons == (P4PC_PAD_CROSS | P4PC_PAD_TRIANGLE | P4PC_PAD_L3));
    CHECK(packet[2] == 0xFD && packet[3] == 0xAF);

    /* A key index past numkeys is not read. */
    memset(keys, 0, sizeof keys);
    keys[SDL_SCANCODE_Z] = true;
    keysToPacket(keys, SDL_SCANCODE_Z, packet, 0, &snap);
    CHECK(snap.buttons == 0);

    /* Mode byte follows the analog flag. */
    memset(keys, 0, sizeof keys);
    keysToPacket(keys, SDL_SCANCODE_COUNT, packet, 0, &snap);
    keysToPacket(keys, SDL_SCANCODE_COUNT, other, 1, &snap);
    CHECK(packet[1] != other[1]);

    /* PCSX2 default sensitivity/quantization, with an optional radial dead zone. */
    {
        uint8_t x, y;
        const int dead = 24 * 32767 / 100;
        static const struct { int raw, byte; } pcsx2[] = {
            {-32768, 0}, {-23170, 8}, {-16384, 43}, {-2000, 117},
            {0, 127}, {2000, 137}, {16384, 212}, {23170, 247}, {32767, 255}
        };
        for (unsigned i = 0; i < sizeof pcsx2 / sizeof pcsx2[0]; i++) {
            p4pc_pad_stick(pcsx2[i].raw, 0, 0, &x, &y);
            CHECK(x == pcsx2[i].byte && y == 127);
        }
        p4pc_pad_stick(0, 0, dead, &x, &y);
        CHECK(x == 127 && y == 127);
        p4pc_pad_stick(dead, 0, dead, &x, &y);
        CHECK(x == 127 && y == 127);
        p4pc_pad_stick(-5000, 5000, dead, &x, &y);
        CHECK(x == 127 && y == 127);
        p4pc_pad_stick(23170, -23170, dead, &x, &y);
        CHECK(x == 247 && y == 8);
        /* 8BitDo/XInput rim: +/-2000 cross-axis noise must never cross P4's
         * 125/128 walk/run boundary, including slightly short mechanical travel. */
        for (int raw = -32768; raw <= 32767; raw++) {
            p4pc_pad_stick(raw, 0, dead, &x, &y);
            CHECK(y == 127);
            if (raw < -32000) CHECK(x <= 3);
            if (raw > 32000) CHECK(x >= 253);
        }
        for (int j = -2000; j <= 2000; j += 100) {
            p4pc_pad_stick(j, -32768, dead, &x, &y);
            CHECK(y == 0x00);
            p4pc_pad_stick(j, -32075, dead, &x, &y);
            CHECK(y == 0x00);
            p4pc_pad_stick(j, 32767, dead, &x, &y);
            CHECK(y == 0xFF);
        }
        /* A circular rim at every heading still exceeds the game's run threshold. */
        for (int deg = 0; deg < 360; deg++) {
            double angle = deg * 0.017453292519943295;
            p4pc_pad_stick((int)(31200 * cos(angle)), (int)(31200 * sin(angle)), dead, &x, &y);
            int dx = (int)x - 128, dy = (int)y - 128;
            if (abs(dx) <= 48) dx = 0;
            if (abs(dy) <= 48) dy = 0;
            CHECK(dx * dx + dy * dy >= 125 * 125);
        }
        /* Match PCSX2 for every signed input even while the game has selected
         * EE round-toward-zero (the host pad mapping is not guest arithmetic). */
        {
            uint8_t nearest[65536];
            const unsigned saved = p4pc_fpu_host_enter();
            for (int raw = -32768; raw <= 32767; raw++) {
                float value = (float)raw / (raw < 0 ? -32768.0f : 32767.0f);
                value = value * 1.33f * 255.0f;
                unsigned half = (unsigned)(value > 255.0f ? 255.0f : value);
                nearest[raw + 32768] = (uint8_t)(raw > 0 ? 127u + (half + 1u) / 2u : 127u - half / 2u);
            }
            p4pc_fpu_ee_mode();
            for (int raw = -32768; raw <= 32767; raw++) {
                p4pc_pad_stick(raw, 0, 0, &x, &y);
                CHECK(x == nearest[raw + 32768] && y == 127);
            }
            (void)p4pc_fpu_host_enter();
            p4pc_fpu_host_leave(saved);
        }
    }

    /* P4PC_AUTOPRESS: start at 12 s every 4 s for 0.3 s, cross at 30 s for 0.5 s. */
    {
        P4PadAutoPress ap[4];
        int n = p4pc_pad_autopress_parse("start@12/4,cross@30:0.5", ap, 4);

        CHECK(n == 2);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 11.9, &snap);
        CHECK(snap.buttons == 0);
        p4pc_pad_autopress_apply(ap, n, 12.1, &snap);
        CHECK(snap.buttons == P4PC_PAD_START);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 12.5, &snap);
        CHECK(snap.buttons == 0);
        p4pc_pad_autopress_apply(ap, n, 16.1, &snap);
        CHECK(snap.buttons == P4PC_PAD_START);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 30.4, &snap);
        CHECK((snap.buttons & P4PC_PAD_CROSS) != 0 && snap.pressure[P4PC_PRESS_CROSS] == 0xFF);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 30.6, &snap);
        CHECK((snap.buttons & P4PC_PAD_CROSS) == 0);
        /* `*count` stops the repeat: cross at 2, every 3, hold 1, three presses (2, 5, 8). */
        n = p4pc_pad_autopress_parse("cross@2:1/3*3", ap, 4);
        CHECK(n == 1);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 8.5, &snap);
        CHECK((snap.buttons & P4PC_PAD_CROSS) != 0);
        startSnapshot(&snap);
        p4pc_pad_autopress_apply(ap, n, 11.5, &snap);
        CHECK((snap.buttons & P4PC_PAD_CROSS) == 0);
        CHECK(p4pc_pad_autopress_parse("jump@3", ap, 4) == -1);
        CHECK(p4pc_pad_autopress_parse("start", ap, 4) == -1);
        CHECK(p4pc_pad_autopress_parse("", ap, 4) == 0);
    }

    /* The Android touch pad's analog stick (PadOverlay -> p4pc_pad_touch_stick): a left stick with its own
     * dead zone, merged only while the gamepad's stick is idle, through input.deadzone like a gamepad. */
    {
        P4PadSnapshot s;
        const int dead = 24 * 32767 / 100;
        p4pc_pad_touch_stick(0.0f, 0.0f);
        startSnapshot(&s);
        CHECK(!p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x80 && s.ly == 0x80);
        p4pc_pad_touch_stick(0.1f, -0.1f); /* inside the stick's own 15 percent dead zone */
        CHECK(!p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x80 && s.ly == 0x80);
        p4pc_pad_touch_stick(NAN, 0.0f);
        CHECK(!p4pc_pad_touch_stick_apply(&s, 0, 0));
        p4pc_pad_touch_stick(1.0f, 0.0f); /* full travel right: y stays neutral */
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0xFF && s.ly == 0x7F && s.buttons == 0);
        p4pc_pad_touch_stick(-3.0f, 0.0f); /* beyond the circle clamps to the rim */
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x00 && s.ly == 0x7F);
        p4pc_pad_touch_stick(0.0f, -1.0f); /* up is negative y, as on a gamepad */
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x7F && s.ly == 0x00);
        p4pc_pad_touch_stick(0.0f, 1.0f);
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 0) && s.ly == 0xFF);
        p4pc_pad_touch_stick(0.5f, 0.0f); /* half travel walks: past the game's 48 dead band, short of the rim */
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx > 0x80 + 48 && s.lx < 0xF0);
        p4pc_pad_touch_stick(0.2f, 0.0f); /* above the stick's dead zone, below input.deadzone 24 percent */
        startSnapshot(&s);
        CHECK(!p4pc_pad_touch_stick_apply(&s, dead, 0) && s.lx == 0x80);
        p4pc_pad_touch_stick(1.0f, 0.0f);
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, dead, 0) && s.lx == 0xFF);
        startSnapshot(&s); /* a deflected gamepad stick keeps its axes */
        s.lx = 0x00;
        CHECK(!p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x00 && s.ly == 0x80);
        startSnapshot(&s); /* Config's list reads digital directions: only with `menu` */
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 1) && (s.buttons & P4PC_PAD_RIGHT) && !(s.buttons & (P4PC_PAD_LEFT | P4PC_PAD_UP | P4PC_PAD_DOWN)));
        p4pc_pad_touch_stick(0.3f, 0.0f);
        startSnapshot(&s);
        CHECK(p4pc_pad_touch_stick_apply(&s, 0, 1) && s.buttons == 0);
        p4pc_pad_touch_stick(0.0f, 0.0f); /* released */
        startSnapshot(&s);
        CHECK(!p4pc_pad_touch_stick_apply(&s, 0, 0) && s.lx == 0x80 && s.ly == 0x80);
    }

    if (failures) {
        fprintf(stderr, "pad_keyboard_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("pad_keyboard_test: ok\n");
    return 0;
}
