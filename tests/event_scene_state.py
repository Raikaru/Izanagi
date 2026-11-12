#!/usr/bin/env python3
"""Check a real-play capture's guest RAM against the retail corpus state, without changing it."""
import argparse
import json
from pathlib import Path
import math
import struct
import sys

PROCEDURES = {
    "rain": "sdl04_12_AM_A",
    "riverbank-vignette": "sdl04_16_AM_A",
    "arcana-rankup-sparkles": "155_commu_open",
    "shadow-haze": "sdl04_15_PM_C",
    "velvet-windows-blurry": "upstair_myroom",
    "tv-world-enter": "sdl04_14_PM_C",
    "near-fog": None,
}
RAM_START = 0x00100000
RAM_END = 0x02000000
CAPTURE_SLACK = 600


def capture_registration(expected):
    """Use the scene's PC state-relative capture registration."""
    return expected.get("port_capture", {})


def _rankup_phase(word, unpack):
    for head in (0x00764ADC, 0x00764AF4, 0x00764AE8):
        node, seen = word(head), set()
        while node:
            if node in seen or len(seen) >= 256:
                raise ValueError("invalid task list")
            seen.add(node)
            if unpack("<24s", node)[0].split(b"\0", 1)[0] == b"cmmRankUpSequence":
                work = word(node + 0x38)
                return (word(work) & 0x18, word(work + 4), word(work + 8),
                        word(work + 0x18) & 0xffff, word(work + 0x1c), word(work + 0x20))
            node = word(node + 0x3c)
    return None


def _event_phase(word, unpack):
    """Observe the matched EVT/message/bank layouts; never advance their state."""
    work = None
    for head in (0x00764ADC, 0x00764AF4, 0x00764AE8):
        node, seen = word(head), set()
        while node:
            if node in seen or len(seen) >= 256:
                raise ValueError("invalid task list")
            seen.add(node)
            if unpack("<24s", node)[0].split(b"\0", 1)[0] == b"evtEvent":
                driver = word(node + 0x38)
                if not driver or word(driver) != 4:
                    return None
                child = word(driver + 0x0C)
                work = word(child + 0x38) if child else None
                break
            node = word(node + 0x3C)
        if work:
            break
    if not work or word(work + 0x678) != 1 or word(work + 0x684) != 2:
        return None
    handle = word(work + 0x680)
    node, seen, window = word(0x008817EC), set(), None
    while node:
        if node in seen or len(seen) >= 64:
            raise ValueError("invalid message window list")
        seen.add(node)
        if word(node + 8) == handle:
            window = word(node + 0x0C)
            break
        node = word(node + 4)
    if not window or word(window) & 0x307 != 0x204:
        return None
    manager = word(0x00764678)
    banks = word(manager + 0x24) if manager else None
    if not banks:
        return None
    portraits = []
    for index in range(2):
        slot = banks + 4 + index * 0x14
        if word(slot) & 0xFFFF000F == 0x0003000D:
            portraits.append(word(slot + 4))
    return (unpack("<3I", work + 0x78C), word(work + 0x14),
            word(work + 0x688), word(window + 0x2C) & 0xFFFF,
            word(window + 0x30), tuple(portraits))


def check(snapshot, scene, metadata, nearby_camera=False, *, state_only=False):
    data = Path(snapshot).read_bytes()
    if len(data) != RAM_END - RAM_START:
        raise ValueError(f"expected a 31 MiB guest RAM snapshot, got {len(data)} bytes")

    def unpack(fmt, address):
        size = struct.calcsize(fmt)
        if not RAM_START <= address <= RAM_END - size:
            raise ValueError(f"guest address outside snapshot: {address:#x}")
        return struct.unpack_from(fmt, data, address - RAM_START)

    def word(address):
        return unpack("<I", address)[0]

    expected = json.loads(Path(metadata).read_text())[scene]
    field = word(0x00762EA0)  # iGpffff9db0: field state
    calendar = word(0x007642CC)  # iGpffffb1dc: calendar latch
    observed = {
        "field": word(field),
        "room": word(field + 4),
        "date": unpack("<H", 0x00797B7A)[0],
        "slot": unpack("<B", 0x00797B7C)[0],
        "weather": unpack("<h", calendar + 8)[0],
    }
    errors = [f"{key}: {observed[key]} != retail {expected[key]}"
              for key in observed if observed[key] != expected[key]]

    camera = word(0x00764B70)
    frame = word(camera + 4)
    matrix = unpack("<16f", frame + 80)
    view = unpack("<2f", camera + 104)
    planes = unpack("<2f", camera + 128)
    # Image comparisons require the exact camera except for the bounded, regional fog case.
    # A timing-only witness compares captured game-state words separately, not image/camera fidelity.
    if scene == "tv-world-enter":
        # cldDungeonIn's opaque fill and 2D ring compositor occlude the background camera.
        # The canonical retail state has compositor state 1, frame 104.
        tunnel = None
        for head in (0x00764ADC, 0x00764AF4, 0x00764AE8):
            node, seen = word(head), set()
            while node:
                if node in seen or len(seen) >= 256:
                    raise ValueError("invalid task list")
                seen.add(node)
                label = unpack("<24s", node)[0].split(b"\0", 1)[0]
                if label == b"cldDungeonIn":
                    work = word(node + 0x38)
                    tunnel = unpack("<2i", work)
                    break
                node = word(node + 0x3C)
        if tunnel is None or tunnel[0] != 1 or abs(tunnel[1] - 104) > 5:
            errors.append(f"TV tunnel compositor {tunnel} != retail phase (1,104)")
    elif nearby_camera and not state_only:
        if scene != "near-fog":
            raise ValueError("nearby camera is allowed only for the fog-region comparison")
        for start in (0, 4, 8):
            observed_axis = matrix[start:start + 3]
            expected_axis = expected["ltm"][start:start + 3]
            dot = sum(a * b for a, b in zip(observed_axis, expected_axis))
            norms = math.sqrt(sum(a * a for a in observed_axis) * sum(a * a for a in expected_axis))
            if not norms > 0 or not dot / norms >= math.cos(math.radians(12)):
                errors.append(f"camera axis {start}: more than 12 degrees from retail")
        distance = math.hypot(matrix[12] - expected["ltm"][12], matrix[14] - expected["ltm"][14])
        if not distance <= 160 or not abs(matrix[13] - expected["ltm"][13]) <= 0.25:
            errors.append(f"nearby camera position: {distance:.3f} horizontal units from retail")
    elif not state_only:
        # Matrix padding/flags are not camera coordinates.
        for index in (0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14):
            tolerance = 0.25 if index >= 12 else 0.00002
            if not abs(matrix[index] - expected["ltm"][index]) <= tolerance:
                errors.append(f"camera[{index}]: {matrix[index]:.9g} != retail {expected['ltm'][index]:.9g}")
    if not state_only and scene != "tv-world-enter":
        for index in range(2):
            if not abs(view[index] - expected["vw"][index]) <= 0.000002:
                errors.append(f"view[{index}]: {view[index]:.9g} != retail {expected['vw'][index]:.9g}")
        for index, key in enumerate(("near", "far")):
            if not abs(planes[index] - expected[key]) <= 0.05:
                errors.append(f"{key}: {planes[index]:.9g} != retail {expected[key]:.9g}")

    procedures = []
    procedure_states = []
    node = word(0x00764608)
    seen = set()
    while node:
        if node in seen or len(seen) >= 32:
            raise ValueError("invalid active-script list")
        seen.add(node)
        label = bytes(unpack("<24s", node)[0]).split(b"\0", 1)[0].decode("ascii")
        procedures.append(label)
        procedure_states.append((label, word(node + 0x12c), word(node + 0x18), word(node + 0x138)))
        node = word(node + 0x150)
    wanted = PROCEDURES[scene]
    if wanted is None and procedures:
        errors.append(f"unexpected active procedures: {procedures}")
    elif wanted is not None and wanted not in procedures:
        errors.append(f"active procedures {procedures} do not include retail {wanted}")
    registration = capture_registration(expected)
    if "procedure" in registration:
        target = (registration["procedure"], registration["index"], registration["pc"])
        if "event" in registration:
            if not any(state[:3] == target for state in procedure_states):
                errors.append(f"capture procedure/index/PC {procedure_states} does not include {target}")
            event = registration["event"]
            phase = _event_phase(word, unpack)
            wanted_phase = (tuple(event["id"]), event["frame"], event["message"], event["row"], event["elapsed"])
            if phase is None or phase[:5] != wanted_phase or event["portrait"] not in phase[5]:
                errors.append(f"inner-EVT/message/portrait phase {phase} does not include {wanted_phase + (event['portrait'],)}")
        elif "rankup" in registration:
            if not any(state[:3] == target for state in procedure_states):
                errors.append(f"capture procedure/index/PC {procedure_states} does not include {target}")
            rankup = registration["rankup"]
            phase = _rankup_phase(word, unpack)
            wanted_phase = (0x18, 10, rankup["frame"], rankup["community"], rankup["rank"], rankup["resource"])
            if phase != wanted_phase:
                errors.append(f"rank-up draw phase {phase} does not match {wanted_phase}")
        elif (*target, registration["timer"]) not in procedure_states:
            errors.append(f"capture procedure/index/PC/timer {procedure_states} does not include {(*target, registration['timer'])}")
    if errors:
        raise ValueError("; ".join(errors))
    camera_status = "nearby legal fog heading (within 12 degrees / 160 units)" if nearby_camera else "camera matches retail"
    if state_only:
        camera_status = "state-only timing witness; camera/image not compared"
    elif scene == "tv-world-enter":
        camera_status = "screen-space tunnel compositor matches retail; background 3D camera occluded"
    print(f"event_scene_state: {scene}: field/date/slot/weather/procedure match retail; {camera_status}")


def check_region(capture, reference, limit, region):
    x, y, width, height = map(int, region.split(","))
    if x < 0 or y < 0 or width <= 0 or height <= 0 or x + width > 640 or y + height > 448:
        raise ValueError("invalid native-frame comparison region")
    actual, expected = Path(capture).read_bytes(), Path(reference).read_bytes()
    if len(expected) != 640 * 448 * 3:
        raise ValueError("regional reference must be raw640x448 RGB")
    difference = 0
    if actual.startswith(b"\x89PNG\r\n\x1a\n"):
        tools = str(Path(__file__).resolve().parents[1] / "tools")
        if tools not in sys.path:
            sys.path.insert(0, tools)
        from png_diff import read_png
        from scale_diff import box_pixel
        aw, ah, channels, pixels = read_png(capture)
        for row in range(y, y + height):
            for column in range(x, x + width):
                rgb = box_pixel(pixels, aw, ah, channels, column, row, 640, 448)
                offset = (row * 640 + column) * 3
                difference += sum(abs(rgb[c] - expected[offset + c]) for c in range(3))
    else:
        if len(actual) != len(expected):
            raise ValueError("raw regional capture must be640x448 RGB; use a PNG for physical-size captures")
        for row in range(y, y + height):
            start = (row * 640 + x) * 3
            end = start + width * 3
            difference += sum(abs(a - b) for a, b in zip(actual[start:end], expected[start:end]))
    mad = difference * 100 / (width * height * 3 * 255)
    print(f"event_scene_pixels: region {region}: retail MAD {mad:.4f}% (region limit {limit})")
    if mad > float(limit):
        raise ValueError("regional retail image comparison failed")


def capture_spec(scene, metadata, nominal):
    """Port-side registration of the matched corpus image; flips are sanity bounds, not its trigger."""
    expected = json.loads(Path(metadata).read_text())[scene]
    registration = capture_registration(expected)
    if not registration:
        raise ValueError(f"{scene} has no state-based capture registration")
    bounds = [max(1, nominal - CAPTURE_SLACK), nominal + CAPTURE_SLACK]
    if "event" in registration:
        event = registration["event"]
        fields = ["event", registration["procedure"], registration["index"], registration["pc"],
                  *event["id"], event["frame"], event["message"], event["row"], event["elapsed"], event["portrait"], *bounds]
    elif "rankup" in registration:
        rankup = registration["rankup"]
        fields = ["rankup", registration["procedure"], registration["index"], registration["pc"],
                  rankup["frame"], rankup["community"], rankup["rank"], rankup["resource"], *bounds]
    elif "procedure" in registration:
        fields = ["proc", registration["procedure"], registration["index"], registration["pc"], registration["timer"], *bounds]
    else:
        floats = [*registration["player"], registration["radius"], *registration["basis"], registration["epsilon"]]
        fields = ["camera", expected["field"], expected["room"], *(format(value, ".9g") for value in floats), registration["offset"], *bounds]
    return ",".join(map(str, fields))


def main():
    specification = sys.argv[1:2] == ["capture-spec"]
    parser = argparse.ArgumentParser(description=__doc__)
    if not specification:
        parser.add_argument("snapshot")
    parser.add_argument("scene", choices=PROCEDURES)
    parser.add_argument("metadata")
    if specification:
        parser.add_argument("nominal", type=int)
    else:
        parser.add_argument("--nearby-camera", action="store_true")
        parser.add_argument("--state-only", action="store_true", help="check corpus identity and exact script/compositor phase without judging image/camera fidelity")
        parser.add_argument("--pixels", nargs=4, metavar=("CAPTURE", "REFERENCE", "LIMIT", "X,Y,W,H"))
    args = parser.parse_args(sys.argv[2:] if specification else None)
    try:
        if specification:
            print(capture_spec(args.scene, args.metadata, args.nominal))
            return 0
        if args.state_only and (args.nearby_camera or args.pixels):
            raise ValueError("state-only timing and image-comparison criteria cannot be combined")
        if args.nearby_camera and not args.pixels:
            raise ValueError("nearby fog camera requires a regional pixel comparison")
        check(args.snapshot, args.scene, args.metadata, args.nearby_camera, state_only=args.state_only)
        if args.pixels:
            check_region(*args.pixels)
    except (OSError, ValueError, KeyError, struct.error) as error:
        print(f"event_scene_state: {args.scene}: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
