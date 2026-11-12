#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS battle/save proof for added enemies;
not a retail comparison.

From the user's own disc it privately generates a package that clones every
regular encounter enemy (ID below 0x100) as an added enemy from 336 with that
retail enemy as its model source, and points ENCOUNT.TBL's encounters at the
clones. The castle 5F training walk then fights them and saves; the next
checkpoint reloads that save with the package enabled."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mod_witness
import retail_tables

PACKAGE = "p4pc.enemytest"
RETAIL_ENEMIES = 336
STRIDES = {"metadata": 60, "affinity": 32, "model": 232, "ai": 164}


def clone_package(iso, root):
    """Write the private package; returns {added id: retail model source}."""
    files = retail_tables.battle_files(iso, ["UNIT.TBL", "MODEL.TBL", "AICALC.TBL", "MSG.TBL", "ENCOUNT.TBL"])
    unit, model = retail_tables.blocks(files["UNIT.TBL"]), retail_tables.blocks(files["MODEL.TBL"])
    ai, msg = retail_tables.blocks(files["AICALC.TBL"]), retail_tables.blocks(files["MSG.TBL"])
    banks = {"metadata": unit[0], "affinity": unit[1], "model": model[1], "ai": ai[8]}
    names = msg[2]
    for key, bank in banks.items():
        assert len(bank) == RETAIL_ENEMIES * STRIDES[key], f"unexpected retail {key} bank"
    assert len(names) == RETAIL_ENEMIES * 21, "unexpected retail enemy name bank"
    encount = bytearray(files["ENCOUNT.TBL"])
    records = struct.unpack_from("<I", encount, 0)[0]
    assert records % 0x18 == 0, "unexpected retail encounter bank"
    used = sorted({struct.unpack_from("<H", encount, 4 + record * 0x18 + 8 + 2 * slot)[0]
                   for record in range(records // 0x18) for slot in range(5)})
    clones = {}
    rows = ['<p4-tables schema="1">']
    for source in (enemy for enemy in used if 0 < enemy < 0x100):
        name = names[source * 21:(source + 1) * 21].split(b"\0", 1)[0]
        if not name:
            continue
        added = RETAIL_ENEMIES + len(clones)
        clones[added] = source
        fields = " ".join(f'{key}="{bank[source * STRIDES[key]:(source + 1) * STRIDES[key]].hex()}"'
                          for key, bank in banks.items())
        rows.append(f'<enemy id="{added}" model-source="{source}" key="{PACKAGE}:clone-{source}" {fields}/>')
        encoded = (name + bytes(1)).hex()
        rows.append(f'<string table="3" id="{added}" role="0" bytes="{encoded}"/>')
    rows.append("</p4-tables>")
    assert clones, "no regular encounter enemy to clone"
    remap = {source: added for added, source in clones.items()}
    for record in range(records // 0x18):
        for slot in range(5):
            offset = 4 + record * 0x18 + 8 + 2 * slot
            enemy = struct.unpack_from("<H", encount, offset)[0]
            if enemy in remap:
                struct.pack_into("<H", encount, offset, remap[enemy])
    package = root / PACKAGE
    (package / "data").mkdir(parents=True)
    (package / "files/BTL/BATTLE").mkdir(parents=True)
    (package / "data/enemies.xml").write_text("\n".join(rows) + "\n", encoding="ascii")
    (package / "files/BTL/BATTLE/ENCOUNT.TBL").write_bytes(bytes(encount))
    (package / "mod.ini").write_text(
        "[mod]\nid = %s\nname = Added enemy clones\nversion = 1.0.0\napi = 1.1\npriority = 0\norder = 0\n\n"
        "[table-source %s:clones]\nfile = data/enemies.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n"
        % (PACKAGE, PACKAGE), encoding="ascii")
    return clones


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("added-enemy proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("added-enemy proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_ENEMY_SEED", str(elf.parent / "saves/story0420_5f")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_enemy: retail ELF/ISO or private story0420_5f seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-enemy-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        clones = clone_package(iso, work / "mods")
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=%s\n" % PACKAGE, encoding="utf-8")
        common = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(work / "mods"),
                      P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ISO=str(iso.resolve()), P4PC_SAVE_DIR=str(card.parent),
                      P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
                      SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT"):
            common.pop(key, None)
        cases = (("battle", "save0420_5f_to_training.walk", "38000,20:1,4/20"),
                 ("reload", "entrance_sleep_to_school.walk", "13000,6:6,4/21"))
        for label, walk, gate in cases:
            env = dict(common, P4PC_WALK_SCRIPT=str(ROOT / "tests/walks" / walk),
                       P4PC_NEWGAME_GATE=gate)
            log = work / (label + ".log")
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                        stdout=output, stderr=subprocess.STDOUT, timeout=2400, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, f"{label} walk/save incomplete"
            if label == "battle":
                presented = {int(added): int(source) for added, source in
                             re.findall(r"p4pc: added enemy (\d+) presents retail model (\d+)", text)}
                assert presented, "no added enemy reached a battle"
                for added, source in presented.items():
                    assert clones.get(added) == source, f"added enemy {added} presented model {source}"
            sidecar = card / "BASLUS-21782Save0" / ".p4mods"
            assert sidecar.is_file() and PACKAGE.encode() in sidecar.read_bytes(), \
                f"{label} save does not require {PACKAGE}"
            (work / (label + ".p4mods-copy")).write_bytes(sidecar.read_bytes())
        outcome = "pass"
        print(f"mod_enemy_game: PASS mod_witness=true reference_eligible=false clones={len(clones)} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
