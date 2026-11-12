#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof for the bundled Physical Split
mod (mods/p4pc.physsplit); never a retail comparison or reference.

Every leg plays the castle 5F training walk from the private story0420_5f card
(Yu, Yosuke and Chie) with P4PC_BATTLE_TRACE=1 and saves:

  retail   the mod installed but not enabled;
  off      enabled with its PHYSICAL SPLIT toggle at the default (off): the save
           image and every traced command and hit equal the retail leg, no
           leaf resolves and the save does not require the package;
  on       the shipped package with the toggle on: Yu's and Chie's weapons
           resolve Slash and Strike, the save requires the package and carries
           enemy knowledge; the next card walk (4/21) reloads that save;
  slash, strike, pierce
           test-only copies of the shipped package in which every enemy is weak
           to one type (and resists the next), the strike and pierce copies also
           retyping the Slash weapons: hits of that type are WEAK and Down the
           target, and a Downed survivor's attacker acts again (One More) while
           the other types' hits are not WEAK; Chie keeps her Strike weapon;
  analyze  the on leg again, capturing the battle Analyze panel that Yu opens
           after Chie's Skewer taught one enemy Pierce (evidence images only);
  descriptions
           the bundled QoL mod beside the shipped package with FULL
           DESCRIPTIONS on: its generator reads the element registry
           (p4.elements) and the Camp skill help is captured, where a split
           skill names its type instead of Phys;
  enemy-base, enemy
           the Bathhouse training walk from the private story0520glimpse card
           (Yu, Yosuke, Chie, Yukiko) with the shipped package, then with a copy
           in which only Yukiko's Konohana Sakuya is weak to Strike: enemy
           attacks typed Strike are WEAK on her and nothing else is WEAK, and
           both legs report whom the enemies' typed attacks targeted.

The variant copies change only data rows of the shipped package; they prove
the subtype mechanics with authored values, not the shipping balance."""
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

PACKAGE = "p4pc.physsplit"
SOURCE = ROOT / "mods" / PACKAGE
LEAF = {"slash": 19, "strike": 20, "pierce": 21}
NEXT = {"slash": "strike", "strike": "pierce", "pierce": "slash"}
WEAK = 4  # Native hit-row WEAK bit (target_flags), as battle_sim's policy reads it.
DOWN = 0x00100000  # Down, as added to the target's status by a WEAK or critical hit.
ALL_OUT = {0x100, 0x101, 0x102}  # All-Out Attack variants.
WALK = ("save0420_5f_to_training.walk", "38000,20:1,4/20")
BATHHOUSE = ("save0520glimpse_to_training.walk", "26000,20:1,5/20")
ANALYZE = 272  # Skill 0x110, Analysis: the battle Analyze command (kind 10).
# The bundled QoL mod's FULL DESCRIPTIONS reads the effective element through p4.elements:
# its witness walk frees the player, then the menu script opens the Camp skill help.
QOL = "p4pc.qol"
DESCRIPTIONS = ("qol_load_free.walk", "40000")
DESCRIPTION_SCRIPT = ("walkdone,wait300,open,wait60,cross,wait90,cross,wait120,square,wait60,shot:skill_help,"
                      "down,wait60,shot:skill_help2,down,wait60,shot:skill_help3,down,wait60,shot:skill_help4")
YUKIKO, SAKUYA = 4, 196  # Yukiko's native ID and her persona, Konohana Sakuya.
RELOAD = ("entrance_sleep_to_school.walk", "13000,6:6,4/21")
HIT = re.compile(r"p4pc: battle trace hit source (\d+):(\d+) target (\d+):(\d+) skill (\d+) target_flags 0x([0-9a-f]+) "
                 r"result_flags 0x([0-9a-f]+) hp (\d+)->(\d+) added_status 0x([0-9a-f]+) status 0x([0-9a-f]+)->0x([0-9a-f]+)")
COMMAND = re.compile(r"p4pc: battle trace command actor (\d+):(\d+) kind (\d+) skill (\d+) .* flip (\d+)")
LEAF_LINE = re.compile(r"p4pc: element leaf (\d+) resolves (weapon|skill|enemy attack) (\d+)")
# Party native IDs (ELF unit-name table 0x005E4480) to their weapons' shipped type.
UNIT_TYPE = {1: "slash", 2: "slash", 3: "strike", 4: "pierce", 6: "strike", 7: "pierce", 8: "slash"}
WEAPON_SKILLS = {0x0, 0x104, 0x105, 0x106, 0x107, 0x108, 0x109, 0x10A}


def authored_types(source_kind):
    """{source id: type} for the package's skill (0) or enemy-attack (2) bindings."""
    xml = (SOURCE / "data" / "split.xml").read_text(encoding="ascii")
    kinds = {value: key for key, value in LEAF.items()}
    return {int(source): kinds[int(leaf)] for source, leaf in
            re.findall(rf'<action source-kind="{source_kind}" source-id="(\d+)" component="0" element="(\d+)"', xml)}


def install(root, rows=None, weapons=None, personas=None):
    """Copy the shipped package; optionally rewrite enemy base rows, Slash weapons or persona rows."""
    package = root / PACKAGE
    shutil.copytree(SOURCE, package)
    path = package / "data" / "split.xml"
    xml = path.read_text(encoding="ascii")

    def value(kind):
        flags = {"normal": 0, "weak": 0x08000000, "resist": 0x10000000}[kind]
        percent = {"normal": 100, "weak": 125, "resist": 50}[kind]
        return f'value="{flags | percent}" scalar="{percent}"'
    if rows:
        def enemy(match):
            entity, profile, element = match.group(1), match.group(2), int(match.group(3))
            if entity != profile:
                return match.group(0)  # Scripted id2 phases keep the shipped rows.
            kind = next(k for k, leaf in LEAF.items() if leaf == element)
            return (f'<affinity kind="1" entity="{entity}" profile="{profile}" element="{element}" channel="0" '
                    f'{value(rows[kind])}/>')
        xml = re.sub(r'<affinity kind="1" entity="(\d+)" profile="(\d+)" element="(\d+)" channel="0" [^/]*/>', enemy, xml)
    if weapons:
        xml = re.sub(r'(<action source-kind="1" source-id="\d+" component="0" element=")19(")',
                     rf'\g<1>{LEAF[weapons]}\2', xml)
    if personas:
        def persona(match):
            entity, profile, element = match.group(1), match.group(2), int(match.group(3))
            if int(entity) not in personas["ids"]:
                return match.group(0)
            kind = next(k for k, leaf in LEAF.items() if leaf == element)
            return (f'<affinity kind="0" entity="{entity}" profile="{profile}" element="{element}" channel="0" '
                    f'{value(personas["rows"][kind])}/>')
        xml = re.sub(r'<affinity kind="0" entity="(\d+)" profile="(\d+)" element="(\d+)" channel="0" [^/]*/>', persona, xml)
    path.write_text(xml, encoding="ascii", newline="\n")


def known_keys(sidecar):
    """{(enemy, type)} learned keys in the save's p4pc/enemy-knowledge block: each
    key names its enemy alias, then its element key."""
    strings = re.findall(rb"[ -~]{4,}", sidecar)
    keys = set()
    for alias, element in zip(strings, strings[1:]):
        enemy = re.fullmatch(rb"p4pc\.physsplit:enemy-(\d+)", alias)
        kind = re.fullmatch(rb"p4pc\.physsplit:(slash|strike|pierce)", element)
        if enemy and kind:
            keys.add((int(enemy.group(1)), kind.group(1).decode()))
    return keys


def trace(text):
    events = []
    for line in text.splitlines():
        hit, command = HIT.search(line), COMMAND.search(line)
        if hit:
            g = hit.groups()
            events.append(("hit", int(g[0]), int(g[1]), int(g[2]), int(g[3]), int(g[4]), int(g[5], 16),
                           int(g[7]), int(g[8]), int(g[10], 16), int(g[11], 16), int(g[9], 16)))
        elif command:
            events.append(("command", int(command.group(1)), int(command.group(2)), int(command.group(4)),
                           int(command.group(3)), int(command.group(5))))
    return events


def hit_type(hit, skills, weapon_type):
    """The shipped (or variant) physical type an observed hit's action resolves to, or None."""
    _, _, source, _, _, skill = hit[:6]
    if skill in WEAPON_SKILLS:
        return weapon_type(source)
    return skills.get(skill)


def check_types(label, events, party_side, weak_type, skills, weapon_type):
    """Party hits of weak_type are WEAK and Down a survivor, who's attacker acts again; others are not WEAK."""
    weak_hits = knockdowns = one_more = 0
    for index, event in enumerate(events):
        if event[0] != "hit" or event[1] != party_side or event[3] == party_side:
            continue
        kind = hit_type(event, skills, weapon_type)
        if kind is None or event[7] == event[8]:
            continue  # Not physical, or a miss (the variants never null an attack).
        flagged = bool(event[6] & WEAK)
        assert flagged == (kind == weak_type), f"{label}: {kind} hit {event} WEAK={flagged}"
        if not flagged:
            continue
        weak_hits += 1
        if event[8] == 0 or event[9] & DOWN:
            continue  # A defeated or already Downed target is not knocked down again.
        assert event[11] & DOWN, f"{label}: WEAK hit {event} did not add Down"
        knockdowns += 1
        # One More: the attacker's next command, or, when every enemy is Down,
        # the All-Out Attack that One More offers (it has no command event).
        following = next((e for e in events[index + 1:] if e[0] == "command" or
                          (e[0] == "hit" and e[5] in ALL_OUT)), None)
        if following and following[0] == "hit":
            assert following[1] == party_side, f"{label}: enemy All-Out after WEAK hit {event}"
            one_more += 1
        elif following and following[1] == party_side:
            assert following[2] == event[2], f"{label}: after WEAK hit {event} actor {following[2]} acted, not One More"
            one_more += 1
    assert weak_hits, f"{label}: no party hit of type {weak_type}"
    return weak_hits, knockdowns, one_more


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--legs", default="retail,off,on,reload,analyze,slash,strike,pierce,enemy-base,enemy,descriptions")
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("the Physical Split proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("the Physical Split proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_ELEMENT_SEED", str(elf.parent / "saves/story0420_5f")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_physsplit: retail ELF/ISO or private story0420_5f seed unavailable", file=sys.stderr)
        return 77
    legs = args.legs.split(",")
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-physsplit-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    skills, enemy_basic = authored_types(0), authored_types(2)
    results = {}
    try:
        common = dict(os.environ, P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ISO=str(iso.resolve()), P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1",
                      P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0",
                      P4PC_BATTLE_TRACE="1")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_MOD_DIR",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_CONFIG"):
            common.pop(key, None)

        def play(label, walk, enabled=True, toggle=None, card_from=None, capture=None, qol=False, **variant):
            leg = work / label
            (leg / "mods").mkdir(parents=True)
            install(leg / "mods", **variant)
            card = leg / "card" / "mc0"
            shutil.copytree(card_from or seed, card)
            config = leg / "p4pc.ini"
            selected = ([PACKAGE] if enabled else []) + ([QOL] if qol else [])
            config.write_text("[mods]\nenabled=%s\n" % ",".join(selected), encoding="utf-8")
            if toggle is not None or qol:
                (leg / "mod-settings").mkdir()
            if toggle is not None:
                (leg / "mod-settings" / (PACKAGE + ".ini")).write_text("split = %s\n" % toggle, encoding="utf-8")
            if qol:
                shutil.copytree(ROOT / "mods" / QOL, leg / "mods" / QOL)
                (leg / "mod-settings" / (QOL + ".ini")).write_text("full_descriptions = on\n", encoding="utf-8")
            env = dict(common, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(leg / "mods"),
                       P4PC_BUNDLED_MOD_DIR="",
                       P4PC_SAVE_DIR=str(card.parent),
                       P4PC_WALK_SCRIPT=str(ROOT / "tests/walks" / walk[0]), P4PC_NEWGAME_GATE=walk[1])
            if capture:
                env.update(P4PC_CAPTURE_FRAMES=",".join(map(str, capture)),
                           P4PC_CAPTURE_PNG=str(leg / "capture_%d.png"))
            if qol:
                env.update(P4PC_MENU_SCRIPT=DESCRIPTION_SCRIPT, P4PC_MENU_EXIT="1", P4PC_SETTINGS_TEST="1")
            log = leg / "game.log"
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=leg, env=env, stdout=output,
                                        stderr=subprocess.STDOUT, timeout=3600, check=False,
                                        creationflags=(subprocess.IDLE_PRIORITY_CLASS | subprocess.CREATE_NO_WINDOW) if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text, f"{label}: not a witness run"
            assert "element-ui" not in text, f"{label} refused the element-ui layout or participant"
            save = card / "BASLUS-21782Save0"
            sidecar = save / ".p4mods"
            leaves = [(int(a), b, int(c)) for a, b, c in LEAF_LINE.findall(text)]
            print(f"mod_physsplit_game: {label} exit={result.returncode} leaves={leaves}")
            results[label] = dict(code=result.returncode, text=text, events=trace(text), leaves=leaves, card=card,
                                  image=(save / "BASLUS-21782Save0").read_bytes() if (save / "BASLUS-21782Save0").is_file() else None,
                                  sidecar=sidecar.read_bytes() if sidecar.is_file() else b"",
                                  saved="walk script done" in text and "SDKMEMCARD_SAVE OK" in text)
            return results[label]

        def party_side(events):
            return next(e[1] for e in events if e[0] == "command" and e[2] == 1)

        if "retail" in legs:
            retail = play("retail", WALK, enabled=False)
            assert retail["code"] == 0 and retail["saved"], "retail leg walk/save incomplete"
            assert not retail["leaves"] and PACKAGE.encode() not in retail["sidecar"]
        if "off" in legs:
            off = play("off", WALK)
            assert off["code"] == 0 and off["saved"], "off leg walk/save incomplete"
            assert not off["leaves"], "the split resolved a leaf with its toggle off"
            assert PACKAGE.encode() not in off["sidecar"], "a toggle-off save requires the package"
            if "retail" in legs:
                assert off["image"] == results["retail"]["image"], "toggle-off save differs from the retail save"
                assert off["events"] == results["retail"]["events"], "toggle-off battle trace differs from retail"
                print(f"mod_physsplit_game: off == retail: save image {len(off['image'])} bytes, "
                      f"{len(off['events'])} traced commands/hits")
        if "on" in legs:
            on = play("on", WALK, toggle="on")
            assert on["code"] == 0 and on["saved"], "on leg walk/save incomplete"
            # The route prints each leaf's first resolution only: Slash comes from
            # Yu's weapon; Strike may first come from a skill (the strike leg
            # shows Chie's weapon hits typed Strike).
            assert (LEAF["slash"], "weapon") in {(leaf, kind) for leaf, kind, _ in on["leaves"]}, \
                f"Yu's weapon did not resolve Slash: {on['leaves']}"
            assert LEAF["strike"] in {leaf for leaf, _, _ in on["leaves"]}, f"no Strike resolution: {on['leaves']}"
            assert PACKAGE.encode() in on["sidecar"], "the toggle-on save does not require the package"
            assert b"enemy-knowledge" in on["sidecar"], "the toggle-on save carries no enemy knowledge"
            # Analyze knowledge is per type: every learned (enemy, type) was a
            # party hit of that type on that enemy, and Yu's weapon taught Slash.
            learned = known_keys(on["sidecar"])
            side = party_side(on["events"])
            hit_types = {(e[4], hit_type(e, skills, UNIT_TYPE.get)) for e in on["events"]
                         if e[0] == "hit" and e[1] == side and e[3] != side}
            assert learned and learned <= hit_types, f"learned {sorted(learned)} beyond hit types {sorted(hit_types)}"
            assert any(kind == "slash" for _, kind in learned), f"no Slash learned: {sorted(learned)}"
            print(f"mod_physsplit_game: on: learned {sorted(learned)}; untouched types stay unknown")
        if "reload" in legs:
            reload = play("reload", RELOAD, toggle="on", card_from=results["on"]["card"])
            assert reload["code"] == 0 and reload["saved"], "reload leg walk/save incomplete"
            assert PACKAGE.encode() in reload["sidecar"], "the reloaded save does not require the package"
            assert known_keys(reload["sidecar"]) >= known_keys(results["on"]["sidecar"]), \
                "the reloaded save lost knowledge learned before the save"
        if "analyze" in legs:
            on = results["on"]
            side = party_side(on["events"])
            flips = [e[5] for e in on["events"] if e[0] == "command" and e[1] == side and e[2] == 1 and e[3] == ANALYZE]
            assert flips, "on: Yu never opened Analyze"
            frames = list(range(flips[0] - 40, flips[0] + 400, 20))
            leg = play("analyze", WALK, toggle="on", capture=frames)
            assert leg["events"] == on["events"], "the capture replay diverged from the on leg"
            shots = sorted((work / "analyze").glob("capture_*.png"))
            assert shots, "analyze: no capture written"
            print(f"mod_physsplit_game: analyze: {len(shots)} captures around flip {flips[0]} in {work / 'analyze'}")
        for kind in ("slash", "strike", "pierce"):
            if kind not in legs:
                continue
            rows = {kind: "weak", NEXT[kind]: "resist", NEXT[NEXT[kind]]: "normal"}
            # Yu's (and Yosuke's) Slash weapons take the leg's type in the Strike
            # and Pierce copies, so a non-lethal Attack can knock a survivor down;
            # Chie keeps her shipped Strike weapon throughout.
            retype = kind if kind != "slash" else None
            leg = play(kind, WALK, toggle="on", rows=rows, weapons=retype)
            assert leg["leaves"], f"{kind}: no leaf resolved"
            side = party_side(leg["events"])

            def weapon_type(unit, retype=retype):
                shipped = UNIT_TYPE.get(unit)
                return retype if retype and shipped == "slash" else shipped
            weak, downs, more = check_types(kind, leg["events"], side, kind, skills, weapon_type)
            print(f"mod_physsplit_game: {kind}: {weak} WEAK party hits, {downs} knockdowns, {more} One More")
            assert downs and more, f"{kind}: no knockdown with One More after a WEAK hit"
        bathhouse = Path(os.environ.get("P4PC_BATHHOUSE_SEED", str(elf.parent / "saves/story0520glimpse")))
        for label in ("enemy-base", "enemy"):
            if label not in legs:
                continue
            assert (bathhouse / "BASLUS-21782Save0").is_dir(), "private story0520glimpse seed unavailable"
            variant = {"personas": {"ids": (SAKUYA,), "rows": {"slash": "normal", "strike": "weak", "pierce": "normal"}}}
            leg = play(label, BATHHOUSE, toggle="on", card_from=bathhouse, **(variant if label == "enemy" else {}))
            side = party_side(leg["events"])
            typed = [e for e in leg["events"] if e[0] == "hit" and e[1] != side and e[3] == side and e[7] != e[8] and
                     ((e[5] in WEAPON_SKILLS and e[2] in enemy_basic) or e[5] in skills)]
            targets = {}
            for event in typed:
                kind = enemy_basic[event[2]] if event[5] in WEAPON_SKILLS else skills[event[5]]
                targets.setdefault(event[4], []).append(kind)
                expected = label == "enemy" and event[4] == YUKIKO and kind == "strike"
                assert bool(event[6] & WEAK) == expected, f"{label}: {kind} enemy hit {event}"
            print(f"mod_physsplit_game: {label}: typed enemy hits by target {targets}, "
                  f"{sum(1 for e in typed if e[6] & WEAK)} WEAK")
            if label == "enemy":
                assert "strike" in targets.get(YUKIKO, []), "enemy: no Strike-typed enemy attack hit Yukiko"
        if "descriptions" in legs:
            description_seed = Path(os.environ.get("P4PC_DESCRIPTION_SEED", str(elf.parent / "saves/story0420")))
            if not (ROOT / "mods" / QOL / "mod.ini").is_file() or not (description_seed / "BASLUS-21782Save0").is_dir():
                print("mod_physsplit_game: descriptions: the bundled QoL mod or story0420 seed is absent; leg skipped")
            else:
                leg = play("descriptions", DESCRIPTIONS, toggle="on", card_from=description_seed, qol=True)
                text = leg["text"]
                assert re.search(r"QOL_DESCRIPTIONS .*elements=registry", text), \
                    "descriptions: FULL DESCRIPTIONS did not read the element registry"
                assert re.search(r"script finished: .* PASS", text), "descriptions: the menu script failed"
                shots = [work / "descriptions" / f"menu_{name}.png" for name in ("skill_help", "skill_help2", "skill_help3", "skill_help4")]
                assert all(shot.is_file() for shot in shots), "descriptions: a help capture is missing"
                print(f"mod_physsplit_game: descriptions: FULL DESCRIPTIONS read the registry; captures in "
                      f"{work / 'descriptions'}")
        outcome = "pass"
        print(f"mod_physsplit_game: PASS mod_witness=true reference_eligible=false legs={','.join(legs)} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
