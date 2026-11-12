#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS fusion/battle/save proof for an added
skill; not a retail comparison and not shipping content.

From the user's own disc it privately generates a package with one added
skill, ID 576: retail Agi's metadata, element and presentation rows under the
name "Test Agi", inheriting nothing, with Agi as its effect source (the cast
loads skill/001.BED and .se). It also adds persona 256 as mod_persona_game
does (Omoikane's rows one level lower, Omoikane as model source), with its
first innate skill replaced by 576 and agility 99 so the protagonist acts
before the AI companions end the fight, and binds 576's damage to
the Physical Split witness package's Pierce leaf (tests/mod_element_game_test.py),
with persona 256's alias and side rows, so the battle's element route names the
skill when it resolves it. The story0422personas Velvet Room walk fuses
Izanagi with Apsaras into persona 256, which learns 576, assigns it and saves:
that save requires the skill package. The battle leg reloads it and walks to
4F, where the protagonist's first command casts 576 from the native Skill list
(the walk driver fails if the list lacks it); the element route must report
skill 576, and the walk saves again. The last leg loads that save with mods
disabled and must be refused."""
import argparse
import os
from pathlib import Path
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
import mod_element_game_test as element

PACKAGE = "p4pc.skilltest"
SKILL, PERSONA = 576, 256
PIERCE = 21
ACTIVE_SKILLS, SKILLS, PERSONAS = 440, 576, 256
SKILL_NAME, PERSONA_NAME = 19, 17
STRIDES = {"metadata": 14, "growth": 70, "model": 88, "candidates": 4, "affinity": 32}


def named(names, stride, count, wanted):
    """Index of the retail row whose NUL-terminated name is wanted."""
    found = [i for i in range(count) if names[i * stride:(i + 1) * stride].split(b"\0", 1)[0] == wanted]
    assert len(found) == 1, f"{wanted!r} is not exactly one retail name"
    return found[0]


def skill_package(iso, root):
    """Write the private package; returns (retail skill source, retail persona source)."""
    files = retail_tables.battle_files(iso, ["SKILL.TBL", "EFFECT.TBL", "PERSONA.TBL", "MODEL.TBL", "MSG.TBL", "UNIT.TBL"])
    skill, effect = retail_tables.blocks(files["SKILL.TBL"]), retail_tables.blocks(files["EFFECT.TBL"])
    persona, model = retail_tables.blocks(files["PERSONA.TBL"]), retail_tables.blocks(files["MODEL.TBL"])
    message, unit = retail_tables.blocks(files["MSG.TBL"]), retail_tables.blocks(files["UNIT.TBL"])
    elements, metadata, presentation = skill[0], skill[1], effect[0]
    assert len(elements) == SKILLS * 2 and len(metadata) == ACTIVE_SKILLS * 40 and \
        len(presentation) == ACTIVE_SKILLS * 4, "unexpected retail skill banks"
    assert len(message[1]) == SKILLS * SKILL_NAME and len(message[3]) == PERSONAS * PERSONA_NAME, \
        "unexpected retail name banks"
    agi = named(message[1], SKILL_NAME, ACTIVE_SKILLS, b"Agi")
    element_byte, aux = struct.unpack_from("<bB", elements, agi * 2)
    leaf = 0xFFFFFFFF if element_byte < 0 else element_byte
    group = element_byte if 0 <= element_byte < 18 else 18

    banks = {"metadata": persona[0], "growth": persona[1], "model": model[2], "candidates": persona[5],
             "affinity": unit[2]}
    for key, bank in banks.items():
        assert len(bank) == PERSONAS * STRIDES[key], f"unexpected retail persona {key} bank"
    omoikane = named(message[3], PERSONA_NAME, PERSONAS, b"Omoikane")
    rows = {key: bytearray(bank[omoikane * STRIDES[key]:(omoikane + 1) * STRIDES[key]]) for key, bank in banks.items()}
    # PERSONA0 level byte one lower, as mod_persona_game: normal fusion of
    # Izanagi with Apsaras then picks the added row over Omoikane.
    assert rows["metadata"][3] > 1, "unexpected Omoikane level"
    rows["metadata"][3] -= 1
    # PERSONA0 +4..+8 are the base stats datPersonaInitRecord (0x0010CAD0)
    # copies (St, Ma, En, Ag, Lu). The 4F fight is otherwise won by the AI
    # companions before the protagonist's first turn; Ag 99 makes him act first.
    rows["metadata"][7] = 99
    # PERSONA1: six bytes, then sixteen learn events {level u8, kind s8, skill u16};
    # level 0 kind 1 is innate (datPersonaLearnFixedSkills 0x0010D150).
    innate = [i for i in range(16) if rows["growth"][6 + 4 * i] == 0 and rows["growth"][7 + 4 * i] == 1]
    assert innate, "Omoikane has no innate skill to replace"
    struct.pack_into("<H", rows["growth"], 8 + 4 * innate[0], SKILL)

    fields = " ".join(f'{key}="{row.hex()}"' for key, row in rows.items())
    persona_name = message[3][omoikane * PERSONA_NAME:(omoikane + 1) * PERSONA_NAME].split(b"\0", 1)[0]
    phys = element.decode(struct.unpack_from("<H", unit[2], omoikane * element.AFFINITY_ROW)[0])
    profile = 0x1000 + PERSONA
    xml = ['<p4-tables schema="1">',
           f'<skill id="{SKILL}" active="1" element="{leaf}" element-aux="{aux}" inheritance-group="{group}" '
           f'inheritance-rate="0" key="{PACKAGE}:agi" metadata="{metadata[agi * 40:(agi + 1) * 40].hex()}" '
           f'presentation="{presentation[agi * 4:(agi + 1) * 4].hex()}" effect-source="{agi}"/>',
           f'<string table="1" id="{SKILL}" role="0" bytes="{(b"Test Agi" + bytes(1)).hex()}"/>',
           f'<string table="1" id="{SKILL}" role="1" bytes="{(b"An added fire spell." + bytes(1)).hex()}"/>',
           f'<persona id="{PERSONA}" model-source="{omoikane}" key="{PACKAGE}:omoikane" {fields}/>',
           f'<string table="2" id="{PERSONA}" role="0" bytes="{(persona_name + bytes(1)).hex()}"/>',
           f'<action source-kind="0" source-id="{SKILL}" component="0" element="{PIERCE}" channel="0" '
           f'key="{PACKAGE}:agi-pierce"/>',
           # The Physical Split package pairs every retail persona; the added one
           # takes Omoikane's physical affinity on the three leaves the same way.
           f'<alias kind="0" entity="{PERSONA}" native-profile="0" profile="{profile}" knowledge="0" '
           f'key="{PACKAGE}:persona-{PERSONA}"/>']
    for split, _, _ in element.LEAVES:
        xml.append(f'<affinity kind="0" entity="{PERSONA}" profile="{profile}" element="{split}" channel="0" '
                   f'value="{phys}" scalar="{phys & 0xFFFF}"/>')
    xml.append("</p4-tables>")
    package = root / PACKAGE
    (package / "data").mkdir(parents=True)
    (package / "data/skills.xml").write_text("\n".join(xml) + "\n", encoding="ascii")
    (package / "mod.ini").write_text(
        "[mod]\nid = %s\nname = Added skill witness\nversion = 1.0.0\napi = 1.1\npriority = 0\norder = 1\n\n"
        "[table-source %s:skills]\nfile = data/skills.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n"
        % (PACKAGE, PACKAGE), encoding="ascii")
    return agi, omoikane


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("added-skill proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("added-skill proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_PERSONA_SEED", str(elf.parent / "saves/story0422personas")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_skill_game: retail ELF/ISO or private story0422personas seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-skill-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        element.element_package(iso, work / "mods")
        agi, omoikane = skill_package(iso, work / "mods")
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=%s,%s\n" % (element.PACKAGE, PACKAGE), encoding="utf-8")
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

        def run(label, env, timeout=2400):
            log = work / (label + ".log")
            with log.open("wb") as output:
                try:
                    code = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                          stdout=output, stderr=subprocess.STDOUT, timeout=timeout, check=False,
                                          creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0
                                          ).returncode
                except subprocess.TimeoutExpired:
                    code = None
            return code, log.read_text(encoding="utf-8", errors="replace")

        sidecar = card / "BASLUS-21782Save0" / ".p4mods"
        cases = (("fusion", ROOT / "tests/walks/save0422personas_to_omoikane.walk", "24000,20:1,4/22"),
                 ("battle", ROOT / "tools/systems/omoikane_skill_witness_battle.txt", "26000,20:1,4/22"))
        for label, walk, gate in cases:
            code, text = run(label, dict(common, P4PC_WALK_SCRIPT=str(walk), P4PC_NEWGAME_GATE=gate))
            assert code == 0, f"{label} exited {code}; see {work / (label + '.log')}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "walk script done" in text and "WALK SCRIPT FAILED" not in text and \
                "SDKMEMCARD_SAVE OK" in text, f"{label} walk/save incomplete"
            if label == "fusion":
                assert f"p4pc: added persona {PERSONA} presents retail model {omoikane}" in text, \
                    f"fusion never presented added persona {PERSONA}"
            else:
                assert f"p4pc: element leaf {PIERCE} resolves skill {SKILL}" in text, \
                    f"the battle never resolved added skill {SKILL}"
            data = sidecar.read_bytes() if sidecar.is_file() else b""
            assert PACKAGE.encode() in data and b"expanded-skills" in data and b"persona-compendium" in data, \
                f"{label} save does not carry the added skill and persona blocks of {PACKAGE}"
            (work / (label + ".p4mods-copy")).write_bytes(data)
            print(f"mod_skill_game: {label} passed")

        refused = work / "refused"
        shutil.copytree(card, refused / "mc0")
        # Without the packages the load is refused: the walk never reaches the
        # field, so this leg ends at its own short timeout.
        code, text = run("refused", dict(common, P4PC_MODS="0", P4PC_SAVE_DIR=str(refused),
                                         P4PC_WALK_SCRIPT=str(cases[1][1]), P4PC_NEWGAME_GATE=cases[1][2]),
                         timeout=600)
        assert (f"Save requires mod {element.PACKAGE} version 1.0.0, which is not enabled at that version" in text
                and "SDKMEMCARD_LOAD OK" not in text), "the save loaded with mods disabled"
        element_only = work / "element-only.ini"
        element_only.write_text(f"[mods]\nenabled={element.PACKAGE}\n", encoding="utf-8")
        code, text = run("refused-skill", dict(common, P4PC_CONFIG=str(element_only),
                                               P4PC_SAVE_DIR=str(refused),
                                               P4PC_WALK_SCRIPT=str(cases[1][1]),
                                               P4PC_NEWGAME_GATE=cases[1][2]), timeout=600)
        assert (f"Save requires mod {PACKAGE} version 1.0.0, which is not enabled at that version" in text
                and "SDKMEMCARD_LOAD OK" not in text), "the save loaded without the added-skill package"
        outcome = "pass"
        print(f"mod_skill_game: PASS mod_witness=true reference_eligible=false skill={SKILL} "
              f"source_skill={agi} persona={PERSONA} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
