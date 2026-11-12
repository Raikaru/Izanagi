#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the weapon fusion groundwork
(p4.persona.stock, p4.social.rank, p4.forge, p4.equipment, the weapon_fusion
authorizer and p4.velvet.command) in the running game; not a retail comparison and not
balance evidence.

Witness-only fixture mods are written into the work directory, never shipped.
test.fusion.forge copies the stock and Social Link ranks into the log, finds
two weapons Yu may equip through the game's own item checks, gives the first
one when the save holds none (a forced witness condition, logged as
TEST_GIVE), then forges a carried, unequipped Persona into the second. Its
before/after lines come from the game's own counters. test.fusion.velvet
registers one info page and the Velvet command that opens it.

Scenarios (all from the private story0422personas seed, loaded through the
title):
  forge   qol_load_free.walk: the forge commits at the next safe point: the
          Persona leaves the stock, one vessel and the fee are taken, one item
          is added.
  deny    the same, with test.fusion.deny refusing weapon_fusion at the commit
          stage: the result is "refused" with its reason and nothing changes.
  velvet  fusion_velvet_root.walk to the Velvet Room's root menu, then the menu
          script: the hint row (menu_hint.png), Square opens the page hosted in
          the Velvet task's Config child (menu_page.png), Circle closes it and
          the root takes input again (menu_back.png, then Down moves its
          cursor: menu_after.png). Once per display: 4:3 at 640x480 and
          1920x1440, 16:9 at 1280x720 and 2560x1440, 21:9 at 1680x720 and
          3440x1440, so the hint's readability is checked at every aspect.
  weapon  fusion_velvet_root.walk, then the bundled page: forge for Yu, then
          EQUIP NOW. test.fusion.weapon checks p4.equipment's real-game
          refusals first, then sees the exchange: Yu wears the forged weapon,
          the replaced one is back in the items (one more held) and the forged
          copy left them, with exactly one core exchange.
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
SLOT = "BASLUS-21782Save0"
SEED = "story0422personas"

FIXTURES = {
    "test.fusion.forge": """--!strict
-- Native item/money accessors (g_data): datIdBand, datEntryHasFlag (item,
-- character), datItemCount, datItemSetCount, datGetMoney.
local BAND, FLAG, COUNT, SET_COUNT, MONEY = 0x00106c80, 0x00106c30, 0x00106600, 0x00106620, 0x00105ed0
local YU = 1
local plan: {[string]: number}? = nil
local done = false

local function state(p: {[string]: number}): string
    local stock = p4.persona.stock()
    local has = false
    for _, entry in stock do
        if entry.persona == p.persona then has = true end
    end
    return string.format("stock=%d has_persona=%s vessel=%d item=%d money=%d", #stock, tostring(has),
        p4.call(COUNT, "ih", p.vessel), p4.call(COUNT, "ih", p.item), p4.call(MONEY, "I"))
end

p4.events.on("field_enter", function()
    if plan or done then return end
    local stock = p4.persona.stock()
    for _, entry in stock do
        p4.log(string.format("FUSION_STOCK slot=%d lead=%s persona=%d arcana=%d level=%d registered=%s",
            entry.slot, tostring(entry.lead), entry.persona, entry.arcana, entry.level, tostring(entry.registered)))
    end
    for arcana = 0, 23 do
        local rank = p4.social.rank(arcana)
        if rank > 0 then p4.log(string.format("FUSION_RANK arcana=%d rank=%d", arcana, rank)) end
    end
    local weapons = {}
    for id = 1, 0xff do
        if #weapons < 2 and p4.call(BAND, "ih", id) == 0 and p4.call(FLAG, "ihh", id, YU) ~= 0 then
            table.insert(weapons, id)
        end
    end
    local persona = 0
    for _, entry in stock do
        if not entry.lead then persona = entry.persona break end
    end
    if #weapons < 2 or persona == 0 then
        p4.log(string.format("FUSION_SKIP weapons=%d persona=%d", #weapons, persona))
        done = true
        return
    end
    local p = {character = YU, persona = persona, vessel = weapons[1], item = weapons[2], fee = 0}
    if p4.call(COUNT, "ih", p.vessel) < 1 then
        p4.call(SET_COUNT, "vhi", p.vessel, 1)
        p4.log(string.format("TEST_GIVE vessel=%d", p.vessel))
    end
    -- The vessel read is the commit's check over the game's own count.
    assert(p4.forge.vessel(YU, p.vessel) == p4.call(COUNT, "ih", p.vessel) and p4.forge.vessel(5, p.vessel) == 0)
    p.fee = math.min(100, p4.call(MONEY, "I"))
    p4.log("FUSION_BEFORE " .. state(p))
    local ok, reason = p4.forge.commit(p)
    p4.log(string.format("FUSION_COMMIT ok=%s reason=%s pending=%s", tostring(ok), tostring(reason),
        tostring(p4.forge.pending())))
    plan = p
end)

p4.events.on("frame_start", function()
    local p = plan
    if not p or done then return end
    local result = p4.forge.result()
    if not result then return end
    done = true
    p4.log(string.format("FUSION_RESULT outcome=%s reason=%s character=%d persona=%d vessel=%d item=%d fee=%d",
        result.outcome, tostring(result.reason), result.character, result.persona, result.vessel, result.item, result.fee))
    p4.log("FUSION_AFTER " .. state(p))
end)

p4.log("FUSION_WITNESS_READY")
""",
    "test.fusion.deny": """--!strict
-- Authorizers run in snapshot mode (no p4.log there): the context is copied
-- into a string and logged at the next frame.
local asked: string? = nil
p4.authorize.on("weapon_fusion", function(context)
    asked = string.format("TEST_DENY_ASKED stage=%d character=%d vessel=%d persona=%d signature=%d",
        context.stage, context.character, context.vessel, context.persona, context.signature)
    return false, "witness denial"
end)
p4.events.on("frame_start", function()
    if asked then
        p4.log(asked)
        asked = nil
    end
end)
p4.log("FUSION_DENY_READY")
""",
    "test.fusion.velvet": """--!strict
local page = p4.ui.page({
    key = "forge", title = "WEAPON FUSION",
    count = function() return 1 end,
    row = function(index: number)
        return {key = "witness", label = "VELVET COMMAND WITNESS", kind = "info", value = "OK"}
    end,
    find = function(key: string) return if key == "witness" then 1 else nil end,
    opened = function() p4.log("TEST_VELVET_PAGE_OPENED") end,
    closed = function() p4.log("TEST_VELVET_PAGE_CLOSED") end,
})
p4.velvet.command({page = page, label = "WEAPON FUSION", operation = "weapon_fusion"})
p4.log("FUSION_VELVET_READY " .. page.key)
""",
    # Isolated witness fixture: the 4/22 seed has not cleared the castle. Give
    # Yu a spare vessel, check the real story reader, then set the rescue flag
    # so this seed can exercise the forge page. This is not a natural-play proof.
    "test.fusion.weapon": """--!strict
local given, unlocked = false, false
-- The native equipment provider, read and refused in the real game; then the
-- exchange EQUIP NOW makes, seen through its own read and the item counts.
local worn, worn_count = 0, 0
p4.events.on("frame_start", function()
    if not given and (p4.call(0x00106600, "ih", 1) :: number) < 1 then
        p4.call(0x00106620, "vhi", 1, 1)
        p4.log("TEST_GIVE vessel=1")
    end
    given = true
    if not unlocked then
        assert(p4.has("story.dungeon_cleared@1"))
        assert(not p4.story.dungeon_cleared(1))
        assert(not pcall(function() p4.story.dungeon_cleared(0) end))
        assert(not pcall(function() p4.story.dungeon_cleared(8) end))
        assert(not pcall(function() p4.story.dungeon_cleared(1.5) end))
        local later = {}
        for dungeon = 2, 7 do later[dungeon] = p4.story.dungeon_cleared(dungeon) end
        p4.call(0x00106390, "vii", 0x11, 1)
        assert(p4.story.dungeon_cleared(1))
        for dungeon = 2, 7 do
            assert(later[dungeon] == p4.story.dungeon_cleared(dungeon))
            p4.call(0x00106390, "vii", 0x10 + dungeon, if later[dungeon] then 0 else 1)
            assert(p4.story.dungeon_cleared(dungeon) ~= later[dungeon])
            assert(p4.story.dungeon_cleared(1))
            p4.call(0x00106390, "vii", 0x10 + dungeon, if later[dungeon] then 1 else 0)
            assert(p4.story.dungeon_cleared(dungeon) == later[dungeon])
        end
        assert(p4.has("equipment@1"))
        worn = p4.equipment.get(1, 0)
        assert(worn > 0)
        worn_count = p4.call(0x00106600, "ih", worn) :: number
        local replaces, why = p4.equipment.check({character = 1, slot = 0, item = worn})
        assert(replaces == nil and string.find(tostring(why), "already equipped", 1, true))
        replaces, why = p4.equipment.check({character = 1, slot = 1, item = worn})
        assert(replaces == nil and string.find(tostring(why), "can equip", 1, true))
        replaces, why = p4.equipment.check({character = 5, slot = 0, item = worn})
        assert(replaces == nil and string.find(tostring(why), "no equipment", 1, true))
        assert(not pcall(function() p4.equipment.get(5, 0) end))
        assert(p4.equipment.result() == nil and not p4.equipment.pending())
        p4.log(string.format("TEST_EQUIPMENT_CHECKS worn=%d", worn))
        unlocked = true
        p4.log("TEST_UNLOCK castle_cleared_flag=1")
    end
    local now = p4.equipment.get(1, 0)
    if worn ~= 0 and now ~= worn then
        p4.log(string.format("TEST_WORN item=%d old=%d old_before=%d old_after=%d item_after=%d", now, worn, worn_count,
            p4.call(0x00106600, "ih", worn) :: number, p4.call(0x00106600, "ih", now) :: number))
        worn = now
    end
    if worn ~= 0 then worn_count = p4.call(0x00106600, "ih", worn) :: number end
end)
p4.log("FUSION_WEAPON_FIXTURE_READY")
""",
}


def write_fixtures(root):
    for name, source in FIXTURES.items():
        package = root / name
        package.mkdir(parents=True)
        (package / "mod.ini").write_text(
            f"[mod]\nid = {name}\nname = Fusion witness fixture\nversion = 1.0.0\napi = 1.6\n"
            "requires = authorize.weapon_fusion@1, persona.stock@1, social.rank@1, forge@1, forge.vessel@1, equipment@1, velvet.command@1, story.dungeon_cleared@1\n"
            "priority = 0\norder = 0\n",
            encoding="ascii")
        (package / "main.luau").write_text(source, encoding="utf-8")
    shutil.copytree(ROOT / "mods" / "p4pc.fusion-arts", root / "p4pc.fusion-arts")


def need(text, pattern, what):
    match = re.search(pattern, text)
    assert match, f"missing {what}: /{pattern}/"
    return match


def counters(match):
    return {key: int(value) if value.isdigit() else value
            for key, value in re.findall(r"(\w+)=(\S+)", match.group(0))}


def launch(work, name, fixtures, common, driver, elf, seeds, walk, extra=None, settings=None):
    """One game process from a fresh copy of the seed; returns its log text. settings: {mod id: {key: value}}
    stored the way a player sets them (mod-settings/<id>.ini beside p4pc.ini)."""
    root = work / name
    card = root / "card" / "mc0"
    shutil.copytree(seeds / SEED, card)
    config = root / "p4pc.ini"
    config.write_text("[mods]\nenabled=%s\n" % ",".join(fixtures), encoding="utf-8")
    for mod, values in (settings or {}).items():
        (root / "mod-settings").mkdir(exist_ok=True)
        (root / "mod-settings" / f"{mod}.ini").write_text("".join(f"{k} = {v}\n" for k, v in values.items()),
                                                          encoding="ascii")
    env = dict(common, P4PC_CONFIG=str(config), P4PC_SAVE_DIR=str(card.parent), P4PC_SCENE_ARTIFACTS=str(root),
               P4PC_WALK_SCRIPT=str(WALKS / walk), P4PC_NEWGAME_GATE="40000", **(extra or {}))
    log = root / (name + ".log")
    with log.open("wb") as output:
        result = subprocess.run([*driver, str(elf)], cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT,
                                timeout=2400, check=False,
                                creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0, f"{name} exited {result.returncode}; see {log}"
    assert "run_mode=mod-witness" in text and "reference_eligible=false" in text, f"{name}: not a witness run"
    assert "walk script done" in text, f"{name}: the walk did not finish; see {log}"
    return text


def run(work, name, fixtures, common, driver, elf, seeds):
    text = launch(work, name, fixtures, common, driver, elf, seeds, "qol_load_free.walk")
    assert "FUSION_WITNESS_READY" in text, f"{name}: the fixture did not start"
    assert "FUSION_SKIP" not in text, f"{name}: the seed offered no forge"
    stock = re.findall(r"FUSION_STOCK slot=\d+ lead=(true|false) persona=\d+", text)
    assert len(stock) >= 2 and stock.count("true") == 1, f"{name}: stock copy {stock}"
    need(text, r"FUSION_COMMIT ok=true reason=nil pending=true", "the queued forge")
    return text


# label: (video.aspect, window width, window height, video.resolution)
DISPLAYS = {
    "4x3-640": ("4:3", 640, 480, "native"),
    "4x3-1440": ("4:3", 1920, 1440, "scale:3"),
    "16x9-720": ("16:9", 1280, 720, "scale:1"),
    "16x9-1440": ("16:9", 2560, 1440, "scale:3"),
    "21x9-720": ("auto", 1680, 720, "scale:1"),
    "21x9-1440": ("auto", 3440, 1440, "scale:3"),
}
# Square opens the page; Esc is its top view's Back, which closes Config (docs/platform/settings.md
# "Hosted flows"); then the root takes ordinary input again.
VELVET_SCRIPT = ("walkdone,wait120,shot:hint,square,wait120,settings-check:active-page:lua/test.fusion.velvet/forge,"
                 "shot:page,settings-key:Escape,wait120,settings-check:closed,shot:back,down,wait60,shot:after")


def velvet(work, display, common, driver, elf, seeds):
    aspect, width, height, resolution = DISPLAYS[display]
    name = "velvet-" + display
    # P4PC_SETTINGS_TEST opts the menu script into the Esc keypress (settings-key:).
    text = launch(work, name, ("test.fusion.velvet",), common, driver, elf, seeds, "fusion_velvet_root.walk",
                  {"P4PC_MENU_SCRIPT": VELVET_SCRIPT, "P4PC_MENU_EXIT": "1", "P4PC_ASPECT": aspect,
                   "P4PC_WINDOW_W": str(width), "P4PC_WINDOW_H": str(height), "P4PC_RESOLUTION": resolution,
                   "P4PC_PRESENT": "blit", "P4PC_CAPTURE_WINDOW": "1", "P4PC_SETTINGS_TEST": "1"})
    key = need(text, r"FUSION_VELVET_READY (\S+)", "the registered command").group(1)
    opened = need(text, r"Velvet command opened page " + re.escape(key), "the hosted page")
    closed = need(text, r"Velvet command page closed", "the page's close")
    assert opened.start() < closed.start(), f"{name}: closed before opened"
    need(text, r"TEST_VELVET_PAGE_OPENED[\s\S]*TEST_VELVET_PAGE_CLOSED", "the page's own open/close callbacks")
    assert "weapon_fusion refused" not in text, f"{name}: the offer was refused"
    assert len(re.findall(r"Velvet command opened page", text)) == 1, f"{name}: the page opened more than once"
    root = work / name
    for shot in ("hint", "page", "back", "after"):
        assert (root / f"menu_{shot}.png").is_file(), f"{name}: no capture {shot}"


# The bundled mod's page from the Velvet root: YU, his only vessel, the first fusable Persona, FORGE (after
# five info rows), then EQUIP NOW, then Esc closes Config.
WEAPON_SCRIPT = ("walkdone,wait120,square,wait120,settings-check:active-page:lua/p4pc.fusion-arts/forge,shot:w_character,"
                 "cross,wait90,shot:w_vessel,cross,wait90,shot:w_persona,down,wait30,cross,wait90,shot:w_confirm,"
                 "down,down,down,down,down,wait30,cross,wait180,shot:w_result,down,wait30,cross,wait90,shot:w_equipped,"
                 "settings-key:Escape,wait120,settings-check:closed")


def weapon(work, common, driver, elf, seeds):
    text = launch(work, "weapon", ("p4pc.fusion-arts", "test.fusion.weapon"), common, driver, elf, seeds,
                  "fusion_velvet_root.walk",
                  {"P4PC_MENU_SCRIPT": WEAPON_SCRIPT, "P4PC_MENU_EXIT": "1", "P4PC_PRESENT": "blit",
                   "P4PC_CAPTURE_WINDOW": "1", "P4PC_SETTINGS_TEST": "1"},
                  settings={"p4pc.fusion-arts": {"weapon_fusion": "on"}})
    need(text, r"FUSION_ARTS_READY weapon_fusion=on", "the bundled mod")
    need(text, r"TEST_UNLOCK castle_cleared_flag=1", "the forced castle unlock")
    need(text, r"TEST_EQUIPMENT_CHECKS worn=\d+", "the provider's real-game refusals")
    forged = counters(need(text, r"FUSION_ARTS_FORGE outcome=forged [^\n]*", "a forge from the page"))
    assert forged["character"] == 1 and 2048 <= forged["item"] <= 2957, f"forged {forged}"
    need(text, r"forge by owner \d+: character 1 persona %d vessel %d -> item %d" %
         (forged["persona"], forged["vessel"], forged["item"]), "the core forge")
    equipped = need(text, r"FUSION_ARTS_EQUIP outcome=equipped character=1 item=%d old=(\d+)" % forged["item"], "EQUIP NOW")
    need(text, r"equip by owner \d+: character 1 slot 0 item %d \(replaced %s\)" % (forged["item"], equipped.group(1)),
         "the core equipment exchange")
    # The menu's exchange, seen by the fixture: Yu wears the forged weapon, the replaced
    # one is back in the items, and the only forged copy left the items.
    worn = counters(need(text, r"TEST_WORN [^\n]*", "the exchanged weapon"))
    assert worn["item"] == forged["item"] and worn["old"] == int(equipped.group(1)), f"worn {worn}"
    assert worn["old_after"] == worn["old_before"] + 1 and worn["item_after"] == 0, f"counts {worn}"
    assert len(re.findall(r"equip by owner", text)) == 1, "exactly one equipment exchange"
    need(text, r"Velvet command page closed", "the page's close")
    for shot in ("w_character", "w_vessel", "w_persona", "w_confirm", "w_result", "w_equipped"):
        assert (work / "weapon" / f"menu_{shot}.png").is_file(), f"weapon: no capture {shot}"


def forge(text):
    before = counters(need(text, r"FUSION_BEFORE [^\n]*", "the before state"))
    after = counters(need(text, r"FUSION_AFTER [^\n]*", "the after state"))
    result = counters(need(text, r"FUSION_RESULT outcome=forged [^\n]*", "the forged result"))
    need(text, r"forge by owner \d+: character 1 persona %d vessel %d -> item %d \(fee %d\)"
         % (result["persona"], result["vessel"], result["item"], result["fee"]), "the forge log line")
    assert before["has_persona"] == "true" and after["has_persona"] == "false", "the Persona was not consumed"
    assert after["stock"] == before["stock"] - 1, "stock count"
    assert after["vessel"] == before["vessel"] - 1, "vessel count"
    assert after["item"] == before["item"] + 1, "item count"
    assert after["money"] == before["money"] - result["fee"], "fee"


def deny(text):
    before = counters(need(text, r"FUSION_BEFORE [^\n]*", "the before state"))
    after = counters(need(text, r"FUSION_AFTER [^\n]*", "the after state"))
    need(text, r"TEST_DENY_ASKED stage=1 character=1 vessel=\d+ persona=\d+ signature=0", "the commit-stage ask")
    need(text, r"weapon_fusion refused by owner \d+: witness denial", "the logged denial")
    need(text, r"FUSION_RESULT outcome=refused reason=witness denial", "the refused result")
    assert "forge by owner" not in text, "a denied forge was applied"
    assert before == after, f"a denied forge changed state: {before} -> {after}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--scenario", action="append", choices=("forge", "deny", "velvet", "weapon"))
    parser.add_argument("--display", action="append", choices=tuple(DISPLAYS))
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("fusion proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("fusion proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", "")).resolve()
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seeds = Path(os.environ.get("P4PC_FUSION_SEEDS", str(elf.parent / "saves")))
    if not elf.is_file() or not iso.is_file() or not (seeds / SEED / SLOT).is_dir():
        print("mod_fusion_arts: retail ELF/ISO or private story0422personas seed unavailable", file=sys.stderr)
        return 77
    selected = args.scenario or ["forge", "deny", "velvet", "weapon"]
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-fusion-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        write_fixtures(work / "mods")
        common = dict(os.environ, P4PC_MOD_DIR=str(work / "mods"), P4PC_MODS="1", P4PC_PACKS="0",
                      P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso.resolve()),
                      P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy",
                      P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2", P4PC_VIRTUAL_CLOCK="1",
                      P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_DUMP_RAM",
                    "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT", "P4PC_NEWGAME_GATE",
                    "P4PC_MENU_SCRIPT", "P4PC_MENU_EXIT", "P4PC_ASPECT", "P4PC_WINDOW_W",
                    "P4PC_WINDOW_H", "P4PC_RESOLUTION", "P4PC_PRESENT", "P4PC_CAPTURE_WINDOW"):
            common.pop(key, None)
        if "forge" in selected:
            forge(run(work, "forge", ("test.fusion.forge",), common, args.driver, elf, seeds))
            print("mod_fusion_arts_game: forge ok")
        if "deny" in selected:
            deny(run(work, "deny", ("test.fusion.forge", "test.fusion.deny"), common, args.driver, elf, seeds))
            print("mod_fusion_arts_game: deny ok")
        if "velvet" in selected:
            for display in args.display or list(DISPLAYS):
                velvet(work, display, common, args.driver, elf, seeds)
                print(f"mod_fusion_arts_game: velvet {display} ok")
        if "weapon" in selected:
            weapon(work, common, args.driver, elf, seeds)
            print("mod_fusion_arts_game: weapon ok")
        outcome = "pass"
        print(f"mod_fusion_arts_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
