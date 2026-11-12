"""Shipped Life Drain rebalance record -> real table boot -> real amount equation, synthetic inputs only."""
import configparser
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from game_elements_inputs_test import input_disc


def run(command, env):
    result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=45)
    if result.returncode == 77:
        raise SystemExit(77)
    if result.returncode:
        raise RuntimeError(f"{command!r} exited {result.returncode}:\n{result.stdout}{result.stderr}")
    print(result.stdout, end="")


def main():
    amount_separator = sys.argv.index("--amount")
    end = sys.argv.index("--driver-end")
    table_driver = sys.argv[1:amount_separator]
    amount_driver = sys.argv[amount_separator + 1:end]
    if not table_driver or not amount_driver or end != len(sys.argv) - 1:
        raise SystemExit("expected table driver --amount amount driver --driver-end")

    source = Path(__file__).resolve().parents[1] / "mods" / "p4pc.hardmode"
    manifest = configparser.ConfigParser(interpolation=None)
    with (source / "mod.ini").open(encoding="ascii") as stream:
        manifest.read_file(stream)
    with tempfile.TemporaryDirectory(prefix="p4-rebalance-drain-") as temporary:
        root = Path(temporary)
        packages = root / "packages"
        owner = packages / "owner"
        delta = packages / "delta"
        owner.mkdir(parents=True)
        (delta / "data").mkdir(parents=True)
        settings = root / "settings"
        settings.mkdir()
        (owner / "mod.ini").write_text(
            "[mod]\nid=derived.owner\nname=Synthetic producer\nversion=1.0.0\napi=1.1\npriority=0\norder=0\n"
            "[table-derived golden]\nproducer=test:golden\nrevision=3\ninputs=skill, item msg\n",
            encoding="ascii")
        # Keep the shipped setting and declaration; only the fixture package's
        # identity and unrelated declarations differ from the full hard-mode mod.
        selected = configparser.ConfigParser(interpolation=None)
        selected["mod"] = {"id": "derived.delta", "name": "Synthetic rebalance", "version": "1.0.0",
                           "api": "1.6", "priority": "0", "order": "0"}
        # The rebalance rows ride hard mode's PARTY KIT setting (`when = party_kit`).
        for section in ("setting party_kit", "table-delta rebalance"):
            selected[section] = dict(manifest[section])
        with (delta / "mod.ini").open("w", encoding="ascii", newline="\n") as stream:
            selected.write(stream)
        # The shipped Life Drain record only: the file's other deltas (ailment
        # riders, multi-hit splits, ...) are validated against real composed
        # rows, which this single-row synthetic input does not model.
        tree = ET.parse(source / "data" / "rebalance.xml")
        deltas = tree.getroot()
        drain = [row for row in deltas if row.tag == "skill-delta" and int(row.get("id"), 0) == 0x65]
        assert len(drain) == 1, "the shipped rebalance changes Life Drain exactly once"
        for row in list(deltas):
            deltas.remove(row)
        deltas.append(drain[0])
        tree.write(delta / "data" / "rebalance.xml", encoding="us-ascii", xml_declaration=True)

        # Synthetic retail-layout Life Drain row, not extracted game data.
        baseline = bytearray(40)
        baseline[0] = 2
        baseline[2] = 2
        struct.pack_into("<H", baseline, 4, 3)
        baseline[0x0e] = 98
        baseline[0x11] = 13
        struct.pack_into("<h", baseline, 0x12, 30)
        metadata = bytearray(440 * 40)
        # Rows the table-derived fixture's producer asserts on its base input.
        metadata[0] = 2
        metadata[11 * 40 + 0x1E] = 8
        metadata[0x65 * 40:0x66 * 40] = baseline
        image = root / "synthetic.iso"
        image.write_bytes(input_disc(effective_ai=True, skill_metadata=metadata))
        row_path = root / "composed-row.bin"
        env = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_CONFIG="none",
                   SDL_AUDIODRIVER="dummy", P4PC_ISO=str(image), P4PC_TEST_DRAIN_ROW=str(row_path),
                   P4PC_TEST_DRAIN_SETTINGS=str(settings))
        env.pop("P4PC_OVERRIDE_DIR", None)
        for choice in ("off", "on"):
            env["P4PC_TEST_DRAIN_CHOICE"] = choice
            run([*table_driver, "drain", str(packages), "derived.owner,derived.delta"], env)
            expected = bytearray(baseline)
            if choice == "on":
                struct.pack_into("<h", expected, 0x12, 35)
            assert row_path.read_bytes() == expected, f"unexpected composed row with rebalance {choice}"
            run([*amount_driver, "--drain-row", str(row_path), choice], env)
    print("game_rebalance_drain: shipped Life Drain record, on/off composition and actual amount passed (synthetic inputs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
