#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof for p4.ui.sprite / p4.ui.overlay
(capabilities ui.sprite@1 and ui.overlay@2 for the screen context); never a retail
comparison or reference.

The menu harness starts a new-game field, dismisses its help message, captures
the field with the player in control, opens Camp and captures the idle Camp
root twice:

  control   the witness.ui-overlay fixture installed but not enabled;
  overlay   the fixture enabled: an authored Camp-label sprite from its
            power-of-two atlas and a plated Camp line drawn through the Camp
            root's queue, and a right-aligned HUD line in the last ordering
            bucket, shown in the field and hidden under Camp like the native
            field HUD.

Both legs must reach Camp. The overlay leg must log the fixture's markers, and
its captures must differ from the control in the overlay rectangles; the
control leg must log none of them. The images stay in the work directory as
mod-witness evidence."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness
from png_diff import read_png

FIXTURE = ROOT / "tests" / "mod_fixtures" / "ui_overlay"
PACKAGE = "witness.ui-overlay"
SCRIPT = "wait90,cross,wait60,shot:field,open,wait60,shot:camp"
# Canvas rectangles (x0, y0, x1, y1) of the fixture's items on the 640x448 canvas.
CAMP_BADGE = (22, 64, 113, 85)
CAMP_LINE = (22, 90, 178, 112)
HUD_LINE = (520, 24, 620, 44)
SCREEN_MARK = (530, 52, 620, 70)
STATUS_ROW = (8, 12, 130, 32)  # the fixture's p4.ui.status HUD row
BATTLE_FLIP = 420  # battle 20's command menu under the battle harness


def region_differs(a, b, rect):
    (wa, ha, ca, pa), (wb, hb, cb, pb) = a, b
    assert (wa, ha) == (wb, hb), "control and overlay captures differ in size"
    sx, sy = wa / 640.0, ha / 448.0
    x0, y0, x1, y1 = int(rect[0] * sx), int(rect[1] * sy), int(rect[2] * sx), int(rect[3] * sy)
    changed = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            ia, ib = (y * wa + x) * ca, (y * wb + x) * cb
            if max(abs(pa[ia + k] - pb[ib + k]) for k in range(3)) > 24:
                changed += 1
    return changed


def run_leg(args, elf, iso, work, label, enabled, battle=False):
    leg = work / label
    leg.mkdir()
    config = leg / "p4pc.ini"
    config.write_text("[mods]\nenabled=%s\n" % (PACKAGE if enabled else ""), encoding="utf-8")
    mods = leg / "mods"
    shutil.copytree(FIXTURE, mods / FIXTURE.name)
    env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods),
               P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
               P4PC_ISO=str(iso.resolve()), P4PC_SCENE_ARTIFACTS=str(leg), P4PC_HIDDEN_WINDOW="1",
               P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1")
    for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_DUMP_RAM",
                "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT"):
        env.pop(key, None)
    if battle:
        # The battle harness's command menu (tests/battle_gate.sh): its frame
        # opens with the Z-only TV outline that once hid the whole HUD layer.
        env.update(P4PC_START_BATTLE="20", P4PC_TURBO="2", P4PC_BATTLE_GATE=str(BATTLE_FLIP + 20),
                   P4PC_CAPTURE_FRAMES=str(BATTLE_FLIP), P4PC_CAPTURE_PNG=str(leg / "battle_%d.png"))
    else:
        env.update(P4PC_VIRTUAL_CLOCK="1", P4PC_START_MENU="7:2", P4PC_MENU_EXIT="1",
                   P4PC_DRAW_WINDOW="10", P4PC_MENU_SCRIPT=SCRIPT)
    if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
        env.pop("SDL_VIDEODRIVER", None)
    else:
        env["SDL_VIDEODRIVER"] = "offscreen"
    log = leg / "capture.log"
    with log.open("wb") as output:
        result = subprocess.run([*args.driver, str(elf.resolve())], cwd=leg, env=env,
                                stdout=output, stderr=subprocess.STDOUT, timeout=900, check=False,
                                creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
    if battle:
        shots = {"command": leg / ("battle_%d.png" % BATTLE_FLIP)}
    else:
        assert "script finished: camp opened, 2 shots, 0 dark, 0 failed writes" in text, f"{label} capture incomplete"
        shots = {name: leg / f"menu_{name}.png" for name in ("field", "camp")}
    for name, path in shots.items():
        assert path.is_file(), f"{label} capture {name} missing"
    return text, {name: read_png(str(path)) for name, path in shots.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("overlay proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("overlay proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    if not elf.is_file() or not iso.is_file():
        print("mod_overlay: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-overlay-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        control_text, control = run_leg(args, elf, iso, work, "control", False)
        overlay_text, overlay = run_leg(args, elf, iso, work, "overlay", True)
        _, control_battle = run_leg(args, elf, iso, work, "control-battle", False, battle=True)
        _, overlay_battle = run_leg(args, elf, iso, work, "overlay-battle", True, battle=True)
        assert "UI_OVERLAY_" not in control_text, "the disabled fixture ran"
        assert "UI_OVERLAY_READY badge=91x21" in overlay_text, "the fixture did not register its overlays"
        assert "UI_OVERLAY_CAMP 1 true" in overlay_text, "the Camp menu_open update was not accepted"
        badge = region_differs(control["camp"], overlay["camp"], CAMP_BADGE)
        line = region_differs(control["camp"], overlay["camp"], CAMP_LINE)
        hud = region_differs(control["field"], overlay["field"], HUD_LINE)
        hud_camp = region_differs(control["camp"], overlay["camp"], HUD_LINE)
        mark_field = region_differs(control["field"], overlay["field"], SCREEN_MARK)
        mark_camp = region_differs(control["camp"], overlay["camp"], SCREEN_MARK)
        assert badge > 200, f"Camp badge region barely changed ({badge} pixels)"
        assert line > 200, f"Camp line region barely changed ({line} pixels)"
        assert hud > 100, f"HUD line region barely changed ({hud} pixels)"
        assert hud_camp < 20, f"HUD line drawn over the Camp menu ({hud_camp} pixels)"
        assert mark_field > 100 and mark_camp > 100, \
            f"screen marker missing (field {mark_field}, Camp {mark_camp} pixels)"
        # The battle command menu: the status row and the screen marker draw over it.
        status_battle = region_differs(control_battle["command"], overlay_battle["command"], STATUS_ROW)
        mark_battle = region_differs(control_battle["command"], overlay_battle["command"], SCREEN_MARK)
        assert status_battle > 100 and mark_battle > 100, \
            f"HUD layer hidden in the battle command menu (status {status_battle}, marker {mark_battle} pixels)"
        outcome = "pass"
        print(f"mod_overlay_game: PASS mod_witness=true reference_eligible=false badge={badge} line={line} "
              f"hud={hud} hud_camp={hud_camp} mark_field={mark_field} mark_camp={mark_camp} "
              f"status_battle={status_battle} mark_battle={mark_battle} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
