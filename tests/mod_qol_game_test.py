#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS game proof of the bundled p4pc.qol features.

Each case loads a private save seed through the title (walk script), then drives the
real native Config/MODS hub and Camp menus with P4PC_MENU_SCRIPT, capturing PNGs of
what the player sees. Settings are either toggled through the hub's native rows or
pre-stored in mod-settings/p4pc.qol.ini beside the case's p4pc.ini. Outputs are
non-reference mod-witness artifacts; this is not a retail comparison.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness

WALKS = ROOT / "tests/walks"
# F1 opens the native Config in the field; MODS lists the one installed mod.
CONFIG_TO_PACKAGE = ("settings-open,wait240,settings-check:open,settings-row:MODS,cross,wait90,"
                     "settings-check:active-page:mods,settings-key:Home,cross,wait90,"
                     "settings-check:active-page:mods/installation,settings-key:Home,wait30")
# Leaves Config (opened in the field it returns through the Camp menu).
CLOSE_CONFIG = "settings-key:Escape,wait120,circle,wait120,shot:closed"


def case(seed, walk, script, settings=None, expect=(), shots=(), stored=None, gate="40000", saved=()):
    return {"seed": seed, "walk": walk, "script": script, "settings": settings or {},
            "expect": expect, "shots": shots, "stored": stored or {}, "gate": gate, "saved": saved}


# Package page rows: identity, status, enabled, order, details, the UI heading,
# then the declared settings in manifest order, then the Lua pages.
CASES = {
    # The hub's native FULL DESCRIPTIONS row turns the feature on and stores it.
    "hub": case(
        "story0420", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},shot:package_off,down,down,down,down,down,down,wait30,cross,wait60,"
        f"shot:package_on,wait300,{CLOSE_CONFIG}",
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("package_off", "package_on"), stored={"full_descriptions": "on"}),
    # With the stored toggle on, the Camp skill list's help box shows the
    # generated Simple text.
    "descriptions": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,cross,wait90,cross,wait120,square,wait60,shot:skill_help,"
        "down,wait60,shot:skill_help2,down,wait60,shot:skill_help3,down,wait60,shot:skill_help4",
        settings={"full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("skill_help", "skill_help2", "skill_help3", "skill_help4")),
    # The stored Exact style is applied at startup: Zio shows its raw power.
    "descriptions_exact": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,cross,wait90,cross,wait120,square,wait60,down,wait60,down,wait60,"
        "shot:skill_exact",
        settings={"full_descriptions": "on", "description_style": "Exact"},
        expect=(r"QOL_DESCRIPTIONS style=Exact generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("skill_exact",)),
    # Items and equipment: the Camp Item and Equip menus' help boxes.
    "descriptions_items": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,shot:camp_root,down,wait60,shot:camp_item_row,cross,wait120,shot:items,"
        "square,wait60,shot:item_help,circle,wait90,circle,wait90,shot:camp_back,down,wait60,cross,wait120,"
        "shot:equip_members,cross,wait120,shot:equip_slots,cross,wait120,shot:equip_list,square,wait60,shot:equip_help",
        settings={"full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("item_help", "equip_help")),
    # The read-only pages, each from its link at the end of the package page
    # (after the DIALOGUE BACKLOG link: SKILL A VS B, SKILL COMPARE, REGISTERED
    # VS CARRIED, COMPENDIUM FILTER, FUSION RESULTS, GLOSSARY ENTRY, GLOSSARY).
    "pages": case(
        "playthrough0606postkanji", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,up,up,up,up,up,wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/skills,shot:skills,down,wait30,cross,wait30,down,wait30,"
        "down,wait30,cross,wait30,shot:skills_picked,settings-key:End,wait30,cross,wait90,shot:skills_ab,"
        "circle,wait90,circle,wait90,settings-key:End,wait30,up,up,up,wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/compendium,shot:compendium,right,wait60,shot:compendium_arcana,"
        "down,down,down,down,wait30,cross,wait90,shot:compendium_compare,circle,wait90,circle,wait90,"
        "settings-key:End,wait30,up,up,wait30,cross,wait90,settings-check:active-page:lua/p4pc.qol/fusion,"
        "shot:fusion,down,wait30,shot:fusion_first,circle,wait90,settings-key:End,wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/glossary,shot:glossary,down,down,wait30,cross,wait90,"
        f"shot:glossary_entry,circle,wait90,circle,wait90,{CLOSE_CONFIG}",
        settings={"skill_compare": "on", "compendium_filter": "on", "fusion_results": "on", "glossary": "on"},
        expect=(r"QOL_FUSIONS roster=(\d+) combinations=(\d+) results=([1-9]\d*)",),
        shots=("skills", "skills_ab", "compendium", "compendium_compare", "fusion", "glossary_entry")),
    # A save written while features ran carries the mod's provenance block:
    # the 04/18 route saves at the classroom butterfly after the backlog and
    # the descriptions have both been used.
    "save_tag": case(
        "story0417", "save0417_to_school_0418.walk", "",
        settings={"backlog": "on", "full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+)", r"SDKMEMCARD_SAVE OK"),
        saved=(b"p4pc.qol", b"1.0.0", b"lua.value", b"backlog", b"features", b"Simple"), gate="60000"),
    # The backlog keeps the conversations of the 04/18 morning and lists them
    # on the package page's DIALOGUE BACKLOG detail.
    "backlog": case(
        "story0417", "qol_backlog_0418.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,shot:links,up,up,up,up,up,up,up,wait30,cross,wait90,"
        f"shot:backlog,r1,wait30,shot:backlog_page2,{CLOSE_CONFIG}",
        settings={"backlog": "on"},
        expect=(r"QOL_BACKLOG entries=([1-9]\d*)",),
        shots=("links", "backlog", "backlog_page2"), gate="60000"),
}


def run_case(name, spec, args, work, iso, elf, saves):
    root = work / name
    card = root / "card" / "mc0"
    shutil.copytree(saves / spec["seed"], card)
    mods = root / "mods"
    shutil.copytree(ROOT / "mods/p4pc.qol", mods / "p4pc.qol")
    config = root / "p4pc.ini"
    config.write_text("[mods]\nenabled=p4pc.qol\n", encoding="ascii")
    if spec["settings"]:
        (root / "mod-settings").mkdir()
        (root / "mod-settings/p4pc.qol.ini").write_text(
            "".join(f"{key} = {value}\n" for key, value in spec["settings"].items()), encoding="ascii")
    env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods), P4PC_MODS="1", P4PC_PACKS="0",
               P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso), P4PC_SAVE_DIR=str(card.parent),
               P4PC_SCENE_ARTIFACTS=str(root), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
               SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2", P4PC_VIRTUAL_CLOCK="1",
               P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0", P4PC_SETTINGS_TEST="1",
               P4PC_WALK_SCRIPT=str(WALKS / spec["walk"]), P4PC_NEWGAME_GATE=spec["gate"])
    if spec["script"]:
        env.update(P4PC_MENU_SCRIPT=spec["script"], P4PC_MENU_EXIT="1")
    if os.name != "nt" and not any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
        env["SDL_VIDEODRIVER"] = "offscreen"
    for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_START_MENU", "P4PC_MOD_WITNESS_OUT"):
        env.pop(key, None)
    log = root / "game.log"
    with log.open("wb") as output:
        result = subprocess.run([*args.driver, str(elf)], cwd=root, env=env, stdout=output,
                                stderr=subprocess.STDOUT, timeout=3000, check=False,
                                creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0, f"{name} exited {result.returncode}; see {log}"
    assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
    assert "QOL_READY version=" in text, f"{name}: the mod did not start"
    assert "walk script done" in text, f"{name}: the walk did not finish"
    if spec["script"]:
        assert re.search(r"script finished: .* PASS", text), f"{name}: the menu script failed"
    found = {}
    for pattern in spec["expect"]:
        match = re.search(pattern, text)
        assert match, f"{name}: missing {pattern}"
        found[pattern] = match.groups()
    for shot in spec["shots"]:
        assert (root / f"menu_{shot}.png").is_file(), f"{name}: no capture {shot}"
    if spec["saved"]:
        sidecar = card / "BASLUS-21782Save0" / ".p4mods"
        assert sidecar.is_file(), f"{name}: the save has no mod sidecar"
        data = sidecar.read_bytes()
        for token in spec["saved"]:
            assert token in data, f"{name}: the save sidecar lacks {token!r}"
        (root / "save.p4mods-copy").write_bytes(data)
    stored = root / "mod-settings/p4pc.qol.ini"
    for key, value in spec["stored"].items():
        assert stored.is_file() and re.search(rf"^{re.escape(key)} = {re.escape(value)}$",
                                              stored.read_text(encoding="ascii"), re.M), \
            f"{name}: {key} was not stored as {value} by the hub"
    print(f"mod_qol_game: case {name} PASS {found}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--case", action="append", choices=sorted(CASES))
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("QoL proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("QoL proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    saves = Path(os.environ.get("P4PC_QOL_SAVES", str(elf.parent / "saves")))
    selected = args.case or sorted(CASES)
    if not elf.is_file() or not iso.is_file() or any(not (saves / CASES[n]["seed"]).is_dir() for n in selected):
        print("mod_qol_game: retail ELF/ISO or private save seeds unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-qol-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        for name in selected:
            run_case(name, CASES[name], args, work, iso.resolve(), elf.resolve(), saves)
        outcome = "pass"
        print(f"mod_qol_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
