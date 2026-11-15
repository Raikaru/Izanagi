#!/usr/bin/env python3
"""Real-game p4.data catalog witness on the user's own disc; never a retail reference.

A private package's main.luau reads the catalog at boot and logs what it sees.
This fixture decodes the same disc independently (MSG.TBL names, SKILL/UNIT/
PERSONA rows, the executable's item-name bank) and checks counts, keys, spot
values and the readable lookup errors against it. No retail byte is committed:
the expected values are computed here from the disc at test time."""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))
import mod_witness  # noqa: E402
import retail_tables  # noqa: E402

MAIN = r"""--!strict
assert(p4.has("data.catalog@1"), "this build lacks data.catalog@1")
local function show(value: any): string
    return if value == nil then "nil" else tostring(value)
end
local counts = {}
for _, kind in {"skill", "persona", "enemy", "item"} do
    table.insert(counts, kind .. "=" .. #p4.data.all(kind :: any))
end
p4.log("DATA_COUNTS " .. table.concat(counts, " "))
local agilao = p4.data.skill("agi lao")
p4.log(string.format("DATA_SKILL id=%d key=%s name=%s element=%s kind=%s power=%d sp=%s accuracy=%d targets=%s",
    agilao.id, show(agilao.key), show(agilao.name), show(agilao.element), agilao.kind, agilao.power or -1,
    show(agilao.cost and agilao.cost.sp), agilao.accuracy or -1, show(agilao.targets)))
local same = p4.data.skill(agilao.key :: string)
p4.log("DATA_KEY_ROUNDTRIP " .. tostring(same.id == agilao.id))
local enemy = p4.data.enemy(2)
p4.log(string.format("DATA_ENEMY id=%d key=%s level=%d hp=%d exp=%d skills=%d drops=%d",
    enemy.id, show(enemy.key), enemy.level, enemy.hp, enemy.exp, #enemy.skills, #enemy.drops))
local special = assert(enemy.special_drop)
p4.log(string.format("DATA_SPECIAL id=%d kind=%s name=%s flag=%d chance=%d",
    special.item.id, special.item.kind, show(special.item.name), special.flag, special.chance))
local persona = p4.data.persona(1)
local first = persona.learnset[1]
p4.log(string.format("DATA_PERSONA key=%s arcana=%s level=%d learn=%d first=%s@%d",
    show(persona.key), show(persona.arcana), persona.level, #persona.learnset,
    show(first and first.skill.key), first and first.level or -1))
local item = p4.data.item(0x302)
p4.log(string.format("DATA_ITEM key=%s kind=%s price=%d skill=%s", show(item.key), item.kind, item.price,
    show(item.skill and item.skill.key)))
local _, typo = pcall(p4.data.skill, (agilao.name :: string) .. "o")
p4.log("DATA_TYPO " .. tostring(typo))
local _, missing = pcall(p4.data.enemy, "Nobody At All")
p4.log("DATA_MISSING " .. tostring(missing))
p4.log("DATA_FIRE " .. #p4.data.find("skill", {element = "fire"}))
p4.log("DATA_DONE")
"""


def names(bank, width):
    return [bank[i * width:(i + 1) * width].split(b"\0")[0].decode("latin-1") for i in range(len(bank) // width)]


def placeholder(name):
    if not name or name.lower() == "blank":
        return True
    # Untranslated rows ("???", two-byte glyphs only): nothing a mod could type.
    if not any(c.isascii() and c.isalnum() for c in name):
        return True
    digits = name[2:] if name[:2].lower() == "0x" else name
    if not digits or any(c.lower() not in "0123456789abcdef" for c in digits):
        return False
    return digits is not name or (len(digits) <= 4 and any(c.isdigit() for c in digits))


def slug(name):
    out, separator = [], False
    for c in name:
        if c in "'.":
            continue
        if not (c.isascii() and c.isalnum()):
            separator = bool(out)
            continue
        if separator:
            out.append("-")
            separator = False
        out.append(c.lower())
    return "".join(out)


def keys(kind, rows):
    result, taken = [], set()
    for i, name in enumerate(rows):
        if placeholder(name) or not slug(name):
            result.append(f"p4:{kind}:#{i}")
            continue
        base, suffix = slug(name), 2
        key = f"p4:{kind}:{base}"
        while key in taken:
            key, suffix = f"p4:{kind}:{base}-{suffix}", suffix + 1
        taken.add(key)
        result.append(key)
    return result


def elf_item_names(elf):
    data = elf.read_bytes()
    phoff, phsize, phnum = struct.unpack_from("<I", data, 0x1C)[0], *struct.unpack_from("<HH", data, 0x2A)
    segments = []
    for i in range(phnum):
        kind, offset, vaddr, _, size = struct.unpack_from("<5I", data, phoff + i * phsize)
        if kind == 1:
            segments.append((vaddr, offset, size))

    def at(address):
        for vaddr, offset, size in segments:
            if vaddr <= address < vaddr + size:
                return offset + address - vaddr
        raise ValueError(f"address {address:#x} outside the executable")

    pointers = struct.unpack_from("<1536I", data, at(0x005E2200))
    return [data[at(p):data.index(b"\0", at(p))].decode("latin-1") for p in pointers]


def expected(iso, elf):
    files = retail_tables.battle_files(iso, ["MSG.TBL", "SKILL.TBL", "UNIT.TBL", "PERSONA.TBL"])
    msg = retail_tables.blocks(files["MSG.TBL"])
    skill_names, enemy_names, persona_names = names(msg[1], 19), names(msg[2], 21), names(msg[3], 17)
    item_names = elf_item_names(elf)
    arcana = names(msg[0], 21)
    skill_keys, enemy_keys = keys("skill", skill_names), keys("enemy", enemy_names)
    persona_keys, item_keys = keys("persona", persona_names), keys("item", item_names)
    elements, skills = retail_tables.blocks(files["SKILL.TBL"])
    agilao = next(i for i, n in enumerate(skill_names) if n.lower().replace(" ", "") == "agilao")
    row = skills[agilao * 40:(agilao + 1) * 40]
    unit = retail_tables.blocks(files["UNIT.TBL"])[0][2 * 60:3 * 60]
    persona = retail_tables.blocks(files["PERSONA.TBL"])
    meta, growth = persona[0][14:28], persona[1][70:140]
    learn = [(meta[3] + growth[6 + k * 4], struct.unpack_from("<H", growth, 8 + k * 4)[0])
             for k in range(16) if growth[7 + k * 4] == 1 and struct.unpack_from("<H", growth, 8 + k * 4)[0]]
    medicine = retail_tables.data_file(iso, "INIT/ITEMTBL.BIN")[16 + 0x302 * 68:16 + 0x303 * 68]
    fire = sum(1 for i in range(576) if elements[i * 2] == 1 and not placeholder(skill_names[i]))
    count = lambda rows: sum(1 for n in rows if not placeholder(n))
    return {
        "counts": (f"DATA_COUNTS skill={count(skill_names)} persona={count(persona_names)} "
                   f"enemy={count(enemy_names)} item={count(item_names)}"),
        "skill": (f"DATA_SKILL id={agilao} key={skill_keys[agilao]} name={skill_names[agilao]} element=fire kind=magic "
                  f"power={struct.unpack_from('<H', row, 0x12)[0]} sp={struct.unpack_from('<H', row, 4)[0]} "
                  f"accuracy={row[0x0E]} targets=one_foe"),
        "enemy": (f"DATA_ENEMY id=2 key={enemy_keys[2]} level={unit[3]} hp={struct.unpack_from('<H', unit, 4)[0]} "
                  f"exp={struct.unpack_from('<H', unit, 0x1E)[0]} "
                  f"skills={sum(1 for k in range(8) if struct.unpack_from('<H', unit, 0x0E + k * 2)[0])} "
                  f"drops={sum(1 for k in range(4) if struct.unpack_from('<H', unit, 0x22 + k * 4)[0])}"),
        "special": (f"DATA_SPECIAL id={struct.unpack_from('<H', unit, 0x34)[0]} kind=item "
                    f"name={item_names[struct.unpack_from('<H', unit, 0x34)[0]]} "
                    f"flag={struct.unpack_from('<H', unit, 0x32)[0]} chance={unit[0x36]}"),
        "persona": (f"DATA_PERSONA key={persona_keys[1]} arcana={arcana[meta[2]]} level={meta[3]} learn={len(learn)} "
                    f"first={skill_keys[learn[0][1]]}@{learn[0][0]}"),
        "item": (f"DATA_ITEM key={item_keys[0x302]} kind=consumable price={struct.unpack_from('<I', medicine, 0x34)[0]} "
                 f"skill={skill_keys[struct.unpack_from('<I', medicine, 0x24)[0]]}"),
        "typo": f"no skill named \"{skill_names[agilao]}o\". Did you mean \"{skill_names[agilao]}\" ({skill_keys[agilao]})",
        "fire": f"DATA_FIRE {fire}",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("p4.data witness requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    if not elf.is_file() or not iso.is_file():
        print("mod_data_game: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    want = expected(iso, elf)
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-data-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        package = work / "mods" / "p4pc.data-witness"
        package.mkdir(parents=True)
        (package / "mod.ini").write_text("[mod]\nid = p4pc.data-witness\nname = p4.data witness\nversion = 1.0.0\n"
                                         "api = 1.5\nrequires = data.catalog@1\npriority = 0\norder = 0\n",
                                         encoding="utf-8")
        (package / "main.luau").write_text(MAIN, encoding="utf-8")
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=p4pc.data-witness\n", encoding="utf-8")
        env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(package.parent), P4PC_MODS="1",
                   P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso.resolve()),
                   P4PC_SAVE_DIR=str(work / "card"), P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1",
                   P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                   P4PC_VIRTUAL_CLOCK="1", P4PC_NEWGAME_GATE="600")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            env.pop("SDL_VIDEODRIVER", None)
        else:
            env["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_WALK_SCRIPT", "P4PC_AUTOPRESS",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT"):
            env.pop(key, None)
        log = work / "boot.log"
        with log.open("wb") as output:
            result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=900, check=False,
                                    creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
        assert "DATA_DONE" in text, f"main.luau did not finish (exit {result.returncode}); see {log}"
        for name in ("counts", "skill", "enemy", "special", "persona", "item", "fire"):
            assert want[name] in text, f"{name}: expected {want[name]!r}; see {log}"
        assert "DATA_KEY_ROUNDTRIP true" in text
        typo = re.search(r"DATA_TYPO (.*)", text)
        assert typo and want["typo"] in typo.group(1), f"typo message: {typo and typo.group(1)!r}"
        missing = re.search(r"DATA_MISSING (.*)", text)
        assert missing and 'no enemy named "Nobody At All".' in missing.group(1), missing and missing.group(1)
        outcome = "pass"
        print(f"mod_data_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
