#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS shop/equipment/battle/save proof for
added items; not a retail comparison and not shipping content.

From the user's own disc it privately generates a package with three added
items cloned from retail ITEMTBL rows (no retail byte is bundled): a sword for
the protagonist (1536, the row and weapon model of retail weapon 0x004, with
attack 1), an armor (1537, from 0x106) and an accessory (1538, from 0x204), each with its own
name, description and synthetic icon, at low prices without material
requirements. It composes with the Physical Split witness package
(tests/mod_element_game_test.py): the added sword deals Slash, every retail
enemy's attack and every Ice and Elec spell is Slash (Marukyu's eyes cast
Mazio rather than attack), and the accessory resists Slash through an
EQUIPPED_ITEM effect row.

From the level-99 Apollo save (saves/card_apollo, as the systems gates) the
shop leg buys the sword, the armor and four accessories at Daidara through the
game's own Yes/No choices, equips them in Camp (the protagonist all three;
Yosuke, Chie and Yukiko an accessory) and saves; that save requires the
package. The battle leg reloads it, walks to Marukyu 1F, engages the nearest
Shadow, sets the companions to Direct Commands, guards for a while so the
enemies act, strikes once with the sword and escapes, so it never depends on a
victory or Shuffle Time: the battle must resolve the added sword's Slash leaf
and the accessory's effect row, which proves the reloaded equipment reached the
battle consumers. The last leg reloads the save with mods disabled and must be
refused."""
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

PACKAGE = "p4pc.itemtest"
ITEMS, ITEM_ROW = 1536, 68
SWORD, ARMOR, WARD = 1536, 1537, 1538
SLASH = 19
RESIST = 0x10000000
# Retail Ice (Bufu..Mabufudyne, Cocytus Pain, Niflheim, Bloody Rain) and Elec (Zio..Maziodyne, Narukami,
# Thunder Reign, Fanatical Spark) damage skills, all status mode 0 in SKILL.TBL.
SPELLS = list(range(0x1A, 0x23)) + list(range(0x27, 0x2F)) + [0x168]
# The menu harness saves once the shop walk has finished, then shows the
# protagonist's equipment; after the battle walk it opens the Camp once.
SAVE_SCRIPT = ("walkdone,save,wait150,cross,wait250,cross,wait250,cross,wait400,cross,wait400,"
               "open,wait60,down,down,cross,wait90,cross,wait90,shot:equip,circle,wait60,circle,wait60,circle,wait60")
BATTLE_SCRIPT = "walkdone,open,wait60,shot:after_battle,circle,wait60"


def item_package(iso, root):
    """Write the private item package next to the Physical Split one."""
    data = retail_tables.data_file(iso, "INIT/ITEMTBL.BIN")
    header = len(data) - ITEMS * ITEM_ROW
    assert header >= 4 and struct.unpack_from("<I", data, 0)[0] == ITEMS, "unexpected retail item table"

    def added(source, unit_mask, price, attack=None):
        row = bytearray(data[header + source * ITEM_ROW:header + (source + 1) * ITEM_ROW])
        kind, mask = struct.unpack_from("<I", row, 0)[0], struct.unpack_from("<I", row, 0x30)[0]
        assert mask & unit_mask, f"retail item {source:#x} is not the expected equipment"
        if attack is not None:
            struct.pack_into("<H", row, 0x08, attack)  # a weapon's attack (P4ItemRow.primary_value)
        struct.pack_into("<I", row, 0x34, price)
        struct.pack_into("<II", row, 0x3c, 0, 0)  # no material requirement: stocked when the shop opens
        return row, kind

    # Attack 1: the battle leg's single strike leaves the Shadow alive, so the leg escapes rather than winning.
    rows = {SWORD: added(0x004, 0x20, 140, attack=1) + (b"Test Blade", b"An added sword.", ' right-model="4"'),
            ARMOR: added(0x106, 0x20, 120) + (b"Test Lining", b"An added armor.", ""),
            WARD: added(0x204, 0x20, 100) + (b"Test Ward", b"An added accessory.", "")}
    assert [rows[i][1] for i in (SWORD, ARMOR, WARD)] == [0, 1, 2], "unexpected retail equipment kinds"
    xml = ['<p4-tables schema="1">']
    for item_id, (metadata, _, name, text, model) in rows.items():
        xml.append(f'<item id="{item_id}" key="{PACKAGE}:item{item_id}" metadata="{metadata.hex()}" '
                   f'icon-owner="{PACKAGE}" icon-key="{PACKAGE}:icon" icon-revision="1"{model}/>')
        xml.append(f'<string table="0" id="{item_id}" role="0" bytes="{(name + bytes(1)).hex()}"/>')
        xml.append(f'<string table="0" id="{item_id}" role="1" bytes="{(text + bytes(1)).hex()}"/>')
    xml.append(f'<action source-kind="1" source-id="{SWORD}" component="0" element="{SLASH}" channel="0" '
               f'key="{PACKAGE}:blade"/>')
    for enemy in range(element.ENEMIES):
        xml.append(f'<action source-kind="2" source-id="{enemy}" component="0" element="{SLASH}" channel="0" '
               f'key="{PACKAGE}:attack-{enemy}"/>')
    for skill in SPELLS:
        xml.append(f'<action source-kind="0" source-id="{skill}" component="0" element="{SLASH}" channel="0" '
                   f'key="{PACKAGE}:spell-{skill}"/>')
    xml.append(f'<effect source-kind="1" source-id="{WARD}" element="{SLASH}" channel="0" flags="{RESIST}" '
               f'key="{PACKAGE}:ward"/>')
    xml.append("</p4-tables>")
    package = root / PACKAGE
    (package / "data").mkdir(parents=True)
    (package / "data/items.xml").write_text("\n".join(xml) + "\n", encoding="ascii")
    (package / "icon.png").write_bytes(element.png(32, 32, lambda x, y: b"\xff\xff\xff\xff"))
    (package / "mod.ini").write_text(
        f"[mod]\nid = {PACKAGE}\nname = Added gear witness\nversion = 1.0.0\napi = 1.1\npriority = 0\norder = 1\n\n"
        f"[resource-image {PACKAGE}:icon]\nfile = icon.png\nformat = png\nschema = 1\nrevision = 1\n\n"
        f"[table-source {PACKAGE}:items]\nfile = data/items.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n",
        encoding="ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("added-item proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("added-item proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_SAVES_DIR", str(elf.parent / "saves"))) / "card_apollo"
    if not elf.is_file() or not iso.is_file() or not seed.is_dir():
        print("mod_item: retail ELF/ISO or private Apollo save unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-item-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        element.element_package(iso, work / "mods")
        item_package(iso, work / "mods")
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
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_MENU_SCRIPT",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_START_MENU"):
            common.pop(key, None)

        def run(label, env, timeout=2400):
            log = work / (label + ".log")
            with log.open("wb") as output:
                try:
                    code = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                          stdout=output, stderr=subprocess.STDOUT, timeout=timeout, check=False,
                                          creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0).returncode
                except subprocess.TimeoutExpired:
                    code = None
            return code, log.read_text(encoding="utf-8", errors="replace")

        code, text = run("shop", dict(common, P4PC_MENU_EXIT="1", P4PC_MENU_SCRIPT=SAVE_SCRIPT,
                                      P4PC_WALK_SCRIPT=str(ROOT / "tools/systems/apollo_item_witness_shop.txt")))
        assert code == 0, f"shop exited {code}; see {work / 'shop.log'}"
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
        done = text.find("walk script done")
        assert done >= 0 and "WALK SCRIPT FAILED" not in text, "shop walk incomplete"
        assert "SDKMEMCARD_SAVE OK" in text and text.find("SDKMEMCARD_SAVE START") > done, \
            "the shop leg did not save after its walk"
        saves = [path for path in card.iterdir() if (path / ".p4mods").is_file()]
        assert len(saves) == 1, "expected exactly one saved slot with a mod sidecar"
        sidecar = (saves[0] / ".p4mods").read_bytes()
        assert b"expanded-items" in sidecar and all(f"{PACKAGE}:item{i}".encode() in sidecar for i in (SWORD, ARMOR, WARD)), \
            "the save does not carry the added items"
        (work / "shop.p4mods-copy").write_bytes(sidecar)

        code, text = run("battle", dict(common, P4PC_MENU_EXIT="1", P4PC_MENU_SCRIPT=BATTLE_SCRIPT,
                                        P4PC_WALK_SCRIPT=str(ROOT / "tools/systems/apollo_item_witness_battle.txt")))
        assert code == 0, f"battle exited {code}; see {work / 'battle.log'}"
        assert "walk script done" in text and "WALK SCRIPT FAILED" not in text, "battle walk incomplete"
        assert f"p4pc: element leaf {SLASH} resolves weapon {SWORD}" in text, "the added sword never struck"
        assert f"p4pc: affinity effect of equipped item {WARD} resolves element leaf {SLASH}" in text, \
            "no enemy hit resolved the added accessory's effect"
        assert "element route refused" not in text.lower() and "Native element" not in text, "a native route refused"

        refused = work / "refused"
        shutil.copytree(card, refused / "mc0")
        # Without the package every load attempt is refused; the walk never
        # reaches free roam, so this leg ends at its own short timeout.
        code, text = run("refused", dict(common, P4PC_MODS="0", P4PC_SAVE_DIR=str(refused),
                                         P4PC_WALK_SCRIPT=str(ROOT / "tools/systems/apollo_item_witness_battle.txt")),
                         timeout=600)
        assert (f"Save requires mod {PACKAGE} version 1.0.0, which is not enabled at that version" in text
                and "SDKMEMCARD_LOAD OK" not in text), "the save loaded without the added-item package"
        outcome = "pass"
        print(f"mod_item_game: PASS mod_witness=true reference_eligible=false items={SWORD},{ARMOR},{WARD} "
              f"evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
