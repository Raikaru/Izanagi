# Input and libpad

[Platform index](../platform.md) · [Settings persistence](settings.md)

## Player controls

`src/platform/pad/pad_sdl.c` uses SDL3 keyboard input plus the first available gamepad.
Hotplug opens a pad; disconnection selects another. The window is created by `scePadInit`.
Closing it exits the process. SDL's positional gamepad layout maps South/East/West/North
to Cross/Circle/Square/Triangle, shoulders to L1/R1, stick clicks to L3/R3, and analog
triggers to L2/R2 with pressure. Start and Back map to Start and Select.

Keyboard defaults (physical keys):

| Action | Keys |
| --- | --- |
| D-pad | Arrows or W/A/S/D |
| Cross / Circle / Triangle / Square | Z / X / C / V |
| Start / Select | Enter / Backspace |
| L1 / R1 | Q / E |
| L2 / R2 | 1 / 3 |
| L3 / R3 | Left Shift / Right Shift |

`input.kb.<button>` takes up to two comma-separated SDL key names or `none`.
`input.pad.<button>` takes an SDL gamepad button name or `none`. With no stick deflected,
the D-pad synthesizes the left stick. Both byte values 127 and 128 count as neutral.

Stick bytes reproduce PCSX2 `PadDualshock2::Set`: divide negative axes by 32768 and
positive axes by 32767, multiply each half-axis by 1.33, clamp, then merge around 127.
Quantization uses host round-to-nearest even while guest arithmetic uses EE truncation.
Do not renormalize the rim to a circle: the game clears components within 48 of center
before its sharp 125/128 run threshold. SDL already converts XInput Y; do not invert twice.
`input.deadzone` / `P4PC_STICK_DEADZONE` optionally adds a circular inner dead zone
(0..50 percent, default 0), without rescaling the remaining travel.

On Android the touch pad's stick (see [Android](../android.md)) is a second source for the
left stick: `p4pc_pad_touch_stick(x, y)` takes the thumb offset in units of the stick's travel
(y down), clamps it to the unit circle and rescales away its own 15 percent dead zone;
`p4pc_pad_sdl_read` then converts it like a gamepad axis, through `input.deadzone` and the
PCSX2 quantization above, only while the gamepad's left stick is idle (it then outranks the D-pad's synthesized stick).

## Libpad packet and request lifetime

Port 0 slot 0 is the controller; port 1 reports disconnected (0). Packets are 32-byte
DualShock 2 packets. The controller reports stable state 6 when idle and request state 5
while negotiating. Two virtual actuator slots support the game's negotiation; actual
host rumble output is unsupported.

Requests advance by **game frames**, never repeated status queries. For the boot request
at game frame 1, mode negotiation stays busy through 9 and becomes stable at 10; actuator
alignment starts at 11, is busy at 12 and completes at 13. The first read at 14 returns 32.
Disconnection and counter wrapping are covered by `boot_seams` without physical devices.

## Automation isolation

All ordinary unattended runs must set `P4PC_INPUT=script`. SDL video initializes, but the
joystick/gamepad subsystem is not opened or polled; keyboard/gamepad hotplug, keyboard
state and key actions are ignored. Only synthetic title/menu/battle/walk input reaches
the guest. Window-close and device-reset events still work. Runners force this after
caller overrides; a virtual clock alone does not isolate player input.

The controller regression alone uses `P4PC_INPUT=virtual` plus `P4PC_PAD_VIRTUAL`.
Only its explicitly created fixture ID may open; no physical/foreign pad or keyboard
state is merged. `pad_isolation_script` and `pad_isolation_virtual` inject foreign pads
and F11 and require no input/fullscreen change while scripted Up still works.
An unset `P4PC_INPUT` retains normal player controls.

`P4PC_AUTOPRESS` accepts comma-separated `button@start[:hold][/every][*count]` presses.
Times are seconds since first pad read unless `P4PC_AUTOPRESS_FRAMES=1` selects presented
flip counts. Hold defaults to 0.3 seconds; frame scripts should use short holds (typically
2) to avoid menu repeat. Names are Start, Select, the four face buttons, L1/R1/L2/R2/L3/R3,
and Up/Down/Left/Right. Presses are ORed into the snapshot at full pressure, preserving
edges. A malformed entry logs a diagnostic and rejects the entire list.
State-driven title and battle controls are in [testing](testing.md#title-and-battle-drivers).

## Diagnosis and fixtures

`P4PC_PAD_TRACE=FILE.csv` records each game frame's raw SDL input, libpad packet,
processed guest words, movement step (`0x007643dc`, despite the decomp name `D_007243DC`),
event/control state and player position. Speed belongs to the preceding movement update;
the packet belongs to the new read.

`P4PC_PAD_VIRTUAL=START,X,Y,XJITTER,DPAD_UP,HZ` attaches an SDL virtual controller.
From flip START, it cycles X-XJITTER, X, X+XJITTER, holding Y and optional D-pad Up.
HZ is a logical report rate on the 30 Hz game flip clock: 1000 emits 33/34 reports
per flip through the real SDL virtual-device setters and `SDL_UpdateJoysticks`.
Reports are published before settings/game input reads, independently of the
window pump's wall-clock throttle; repeated reads of one flip retain one state.
There is no separately scheduled producer thread or activation grace period.
HZ=0 leaves the owned SDL device under the adapter unit test's manual control.
This is a real SDL fixture, not packet injection.
Retail `p4hook --pad-trace FILE --script l_up@300:800` observes the same movement words;
`l_down`, `l_left`, `l_right` and digital `up` are also supported.

`pad_keyboard_test` covers all 65,536 signed axes in both float rounding modes, signed
endpoints, PCSX2 vectors and rim headings. `pad_gamepad_test` uses SDL virtual devices;
`pad_gamepad_frame_clock` checks the first read at START, exact report phases,
same-frame repeats without pump sleeps, and clock rewind/replay.
`pad_field_run` / `pad_field_run64` load private `saves/story0415` and require at least
1001 consecutive, gap-free game-frame reads at full run speed for full-forward, near-rim
and D-pad input at 1000 logical Hz. The fixture also requires non-neutral input
from START itself and the exact SDL raw-axis jitter phase, without changing the
existing 3100..4100 movement-speed window.

The field witness is state-only (`P4PC_TURBO=2`), forces `P4PC_CONFIG=none`, and preserves
the framework's virtual-clock policy. `pad_field_gate.py --paced` retains full rendering
and the real clock. `P4PC_GATE_TIMEOUT` is the total three-case budget; the native default
is 500 seconds per case. Failure logs/packet CSV/load scripts stay in
`p4pc_pad_field_failed_*` or explicit `P4PC_PAD_GATE_OUT`; cards are not copied into that
diagnostic directory. This observer does not alter movement or controller thresholds.
