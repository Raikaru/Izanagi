#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS fusion/battle/save proof for an added
persona; not a retail comparison.

From the user's own disc it privately generates a package with one added
persona, ID 256: Omoikane's retail rows and name with its level byte one lower,
so normal fusion's lowest-level-at-or-above rule picks it over Omoikane, and
Omoikane as its model source. The story0422personas Velvet Room walk fuses
Izanagi with Apsaras into it, assigns it and saves; the next checkpoint
reloads that save (its persona block pins ID 256) for the 4F fight and saves
again."""
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
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mod_witness
import retail_tables

PACKAGE = "p4pc.personatest"
ADDED = 256
RETAIL_PERSONAS = 256
STRIDES = {"metadata": 14, "growth": 70, "model": 88, "candidates": 4, "affinity": 32}
NAME = 17


def persona_package(iso, root):
    """Write the private package; returns (added id, retail model source)."""
    files = retail_tables.battle_files(iso, ["PERSONA.TBL", "MODEL.TBL", "MSG.TBL", "UNIT.TBL"])
    persona, model = retail_tables.blocks(files["PERSONA.TBL"]), retail_tables.blocks(files["MODEL.TBL"])
    banks = {"metadata": persona[0], "growth": persona[1], "model": model[2], "candidates": persona[5],
             "affinity": retail_tables.blocks(files["UNIT.TBL"])[2]}
    names = retail_tables.blocks(files["MSG.TBL"])[3]
    for key, bank in banks.items():
        assert len(bank) == RETAIL_PERSONAS * STRIDES[key], f"unexpected retail persona {key} bank"
    assert len(names) == RETAIL_PERSONAS * NAME, "unexpected retail persona name bank"
    source = next((persona_id for persona_id in range(1, RETAIL_PERSONAS)
                   if names[persona_id * NAME:(persona_id + 1) * NAME].split(b"\0", 1)[0] == b"Omoikane"), None)
    assert source, "Omoikane not found in the retail persona names"
    rows = {key: bytearray(bank[source * STRIDES[key]:(source + 1) * STRIDES[key]]) for key, bank in banks.items()}
    # PERSONA0: flags u16, arcana u8, level u8. One level lower is still above
    # Izanagi 1 + Apsaras 4, so the result scan prefers the added row.
    assert rows["metadata"][3] > 1, "unexpected Omoikane level"
    rows["metadata"][3] -= 1
    fields = " ".join(f'{key}="{row.hex()}"' for key, row in rows.items())
    name = names[source * NAME:(source + 1) * NAME].split(b"\0", 1)[0]
    xml = ['<p4-tables schema="1">',
           f'<persona id="{ADDED}" model-source="{source}" key="{PACKAGE}:omoikane" {fields}/>',
           f'<string table="2" id="{ADDED}" role="0" bytes="{(name + bytes(1)).hex()}"/>',
           "</p4-tables>"]
    package = root / PACKAGE
    (package / "data").mkdir(parents=True)
    (package / "data/personas.xml").write_text("\n".join(xml) + "\n", encoding="ascii")
    (package / "mod.ini").write_text(
        "[mod]\nid = %s\nname = Added persona\nversion = 1.0.0\napi = 1.1\npriority = 0\norder = 0\n\n"
        "[table-source %s:personas]\nfile = data/personas.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n"
        % (PACKAGE, PACKAGE), encoding="ascii")
    return ADDED, source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("added-persona proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("added-persona proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_PERSONA_SEED", str(elf.parent / "saves/story0422personas")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_persona: retail ELF/ISO or private story0422personas seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-persona-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        added, source = persona_package(iso, work / "mods")
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
        cases = (("fusion", "save0422personas_to_omoikane.walk", "24000,20:1,4/22"),
                 ("battle", "save0422omoikane_to_tactics.walk", "26000,20:1,4/22"))
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
            presented = {int(persona): int(model) for persona, model in
                         re.findall(r"p4pc: added persona (\d+) presents retail model (\d+)", text)}
            if label == "fusion":
                assert presented.get(added) == source, f"fusion never presented added persona {added}"
            sidecar = card / "BASLUS-21782Save0" / ".p4mods"
            data = sidecar.read_bytes() if sidecar.is_file() else b""
            assert PACKAGE.encode() in data and b"persona-compendium" in data, \
                f"{label} save does not carry the added persona block of {PACKAGE}"
            (work / (label + ".p4mods-copy")).write_bytes(data)
            print(f"mod_persona_game: {label} presented={sorted(presented.items())}")
        outcome = "pass"
        print(f"mod_persona_game: PASS mod_witness=true reference_eligible=false added={added} "
              f"model_source={source} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
