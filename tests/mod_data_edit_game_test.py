#!/usr/bin/env python3
"""Real-game p4.data edit witness on the user's own disc; never a retail reference.

Two private packages edit the same tables while the game builds them: one with
data/edits.xml, one later in load order with data.luau. A third package's
main.luau then reads the loaded banks through p4.data and logs what the game
actually holds, and p4.data.conflicts. This fixture computes the expected
values from the same disc (the original numbers the relative edits start
from) and checks the composition, load order, conflict report, the data.luau
refusal of p4.data.edit in main.luau, and the renames: a skill's MSG row and
an item's name as the game's own datGetItemNameId returns it. Nothing retail
is committed."""
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

EDITS_XML = """<p4-edits schema="1">
  <skill name="Agilao" power="120" sp="10"/>
  <skill name="Bufu" rename="Frost"/>
  <enemy key="p4:enemy:selfish-basalt" hp="999" fire="weak"/>
  <item name="Medicine" price="1234"/>
</p4-edits>
"""
DATA_LUAU = """--!strict
p4.data.edit(function(d)
    local agilao = d.skill("Agilao")
    -- Relative to the layer below (edits.xml's 120): the later layer wins.
    agilao.power = (agilao.power :: number) + 30
    local basalt = d.enemy("p4:enemy:selfish-basalt")
    basalt.drops[2] = { item = "Medicine", chance = 25 }
    d.persona("Izanagi").learnset:add({ level = 9, skill = "Agilao" })
    d.item("Medicine").name = "Herbal Tonic"
    d.item("Medicine").shop.from = { month = 4, day = 20 }
    d.item("Golf Club").shop.sold = true
    d.item("5-Iron").shop.unlock = { { material = "Supple Metal", count = 3 } }
    local castle = d.floor("Yukiko's Castle 3F")
    castle.spawn = 7
    castle.encounters.rates.rare = 9
    castle.encounters.normal[1].weight = 77
    d.formation(17).enemies[2] = "p4:enemy:lying-hablerie"
end)
"""
MAIN = r"""--!strict
local agilao = p4.data.skill("Agilao")
local basalt = p4.data.enemy("p4:enemy:selfish-basalt")
local medicine = p4.data.item("Medicine")
local drop = basalt.drops[2]
local learns = ""
for _, event in p4.data.persona("Izanagi").learnset do
    if event.skill.key == "p4:skill:agilao" then learns = learns .. tostring(event.level) end
end
p4.log(string.format("EDIT_VALUES power=%d sp=%s hp=%d fire=%s price=%d drop=%s@%s learn=%s",
    agilao.power or -1, tostring(agilao.cost and agilao.cost.sp), basalt.hp, basalt.affinities.fire,
    medicine.price, tostring(drop and drop.item.key), tostring(drop and drop.chance), learns))
for _, c in p4.data.conflicts() do
    p4.log(string.format("EDIT_CONFLICT %s %s %s -> %s", tostring(c.key), c.field, table.concat(c.owners, ","), c.winner))
end
local ok, message = pcall(p4.data.edit, function() end)
p4.log("EDIT_MAIN " .. tostring(ok) .. " " .. tostring(message))
local function shop(name: string): P4ItemShop
    return assert(p4.data.item(name).shop)
end
local from = shop("Medicine").from
local gate = (shop("5-Iron").unlock or {})[1]
p4.log(string.format("EDIT_SHOP has=%s from=%s/%s golf=%s iron=%s@%s", tostring(p4.has("data.shop")),
    tostring(from and from.month), tostring(from and from.day), tostring(shop("Golf Club").sold),
    tostring(gate and gate.material.key), tostring(gate and gate.count)))
local castle = p4.data.floor("Yukiko's Castle 3F")
local group = assert(castle.encounters)
local second = p4.data.formation(17).enemies[2]
p4.log(string.format("EDIT_FLOOR has=%s spawn=%d rare=%d weight=%d enemy=%s", tostring(p4.has("data.dungeons")),
    castle.spawn, group.rates.rare, group.normal[1].weight, tostring(second and second.key)))
-- Session edits write the loaded tables now; an error undoes the whole call.
p4.data.session.edit(function(d) d.enemy("p4:enemy:selfish-basalt").hp = 777 end)
local rolled = pcall(function()
    p4.data.session.edit(function(d)
        d.enemy("p4:enemy:selfish-basalt").hp = 555
        error("stop")
    end)
end)
p4.data.session.edit(function(d) d.skill("Agilao").power = 222 end, { persist = true })
p4.log(string.format("EDIT_SESSION has=%s hp=%d rolled=%s power=%d", tostring(p4.has("data.session")),
    p4.data.enemy("p4:enemy:selfish-basalt").hp, tostring(rolled), p4.data.skill("Agilao").power or -1))
-- The game's own datGetItemNameId (0x001067F0) returns the new name once the
-- battle tables completed; read it on the first frame.
local named = false
p4.events.on("frame_start", function()
    if named then return end
    named = true
    local stream = p4.call(0x001067F0, "Ih", medicine.id) :: number
    p4.log(string.format("EDIT_NAMES has=%s skill=%s item=%s game=%s", tostring(p4.has("data.item_names")),
        tostring(p4.data.skill("Bufu").name), tostring(p4.data.item("Medicine").name), p4.mem.read_string(stream, 32)))
    p4.log("EDIT_DONE")
end)
"""


def manifest(identifier, order, requires):
    return (f"[mod]\nid={identifier}\nname={identifier}\nversion=1.0.0\napi=1.5\nrequires={requires}\n"
            f"priority=0\norder={order}\n")


def expected(iso):
    files = retail_tables.battle_files(iso, ["SKILL.TBL", "UNIT.TBL", "PERSONA.TBL"])
    skills = retail_tables.blocks(files["SKILL.TBL"])[1]
    return {"agilao_power_retail": struct.unpack_from("<H", skills, 2 * 40 + 0x12)[0]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("p4.data edit witness requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    if not elf.is_file() or not iso.is_file():
        print("mod_data_edit_game: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    retail = expected(iso)
    assert retail["agilao_power_retail"] != 150, "the witness needs Agilao's retail power to differ from its edit"
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-data-edit-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        mods = work / "mods"
        xml = mods / "witness.xml"
        (xml / "data").mkdir(parents=True)
        (xml / "mod.ini").write_text(manifest("witness.xml", 0, "data.xml@1"), encoding="utf-8")
        (xml / "data" / "edits.xml").write_text(EDITS_XML, encoding="utf-8")
        luau = mods / "witness.luau"
        luau.mkdir(parents=True)
        (luau / "mod.ini").write_text(
            manifest("witness.luau", 1, "data.edit@1, data.item_names@1, data.shop@1, data.dungeons@1"), encoding="utf-8")
        (luau / "data.luau").write_text(DATA_LUAU, encoding="utf-8")
        reader = mods / "witness.reader"
        reader.mkdir(parents=True)
        (reader / "mod.ini").write_text(manifest("witness.reader", 2, "data.catalog@1, data.edit@1, data.session@1"),
                                        encoding="utf-8")
        (reader / "main.luau").write_text(MAIN, encoding="utf-8")
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=witness.xml,witness.luau,witness.reader\n", encoding="utf-8")
        env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods), P4PC_MODS="1",
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
        assert "EDIT_DONE" in text, f"main.luau did not finish (exit {result.returncode}); see {log}"
        want = ("EDIT_VALUES power=150 sp=10 hp=999 fire=weak price=1234 drop=p4:item:medicine@25 learn=9")
        assert want in text, f"expected {want!r}; see {log}"
        assert "EDIT_CONFLICT p4:skill:agilao power witness.xml,witness.luau -> witness.luau" in text, f"see {log}"
        assert len(re.findall(r"EDIT_CONFLICT ", text)) == 1, f"one shared field only; see {log}"
        main_edit = re.search(r"EDIT_MAIN (.*)", text)
        assert main_edit and main_edit.group(1).startswith("false") and "move this call to data.luau" in main_edit.group(1)
        # Renames: the skill's MSG row, and the item through the game's own name function.
        names = "EDIT_NAMES has=true skill=Frost item=Herbal Tonic game=Herbal Tonic"
        assert names in text, f"expected {names!r}; see {log}"
        # Shops: a consumable's date, the never-sold mark off gear, and a gear gate.
        shop = "EDIT_SHOP has=true from=4/20 golf=true iron=p4:item:supple-metal@3"
        assert shop in text, f"expected {shop!r}; see {log}"
        # Dungeons: a floor's spawn, its encounter group's rare rate and first
        # normal weight, and a formation's second enemy.
        floor = "EDIT_FLOOR has=true spawn=7 rare=9 weight=77 enemy=p4:enemy:lying-hablerie"
        assert floor in text, f"expected {floor!r}; see {log}"
        # Session edits: live at once, an erroring call undone, a persisted one.
        session = "EDIT_SESSION has=true hp=777 rolled=false power=222"
        assert session in text, f"expected {session!r}; see {log}"
        outcome = "pass"
        print(f"mod_data_edit_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
