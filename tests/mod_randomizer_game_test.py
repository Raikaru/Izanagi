#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the bundled seeded randomizer
(mods/p4pc.randomizer) in the running game; not a retail comparison.

The expected run is computed headlessly first: the same main.luau in the pinned
Luau VM (luau_script_runner, --runner) on the banks of the user's own disc,
extracted to a private directory. The game legs then must reproduce it:

  newgame   RANDOMIZE NEW GAMES with R1+R2+R3 and a fixed seed, a real New Game:
            the run arms at the first field, its banks become the live tables
            (every probed row equals the headless generation), the game's own
            datPersonaInitRecord/datPersonaRunSkillSlot build records from them,
            Izanagi stays stock through the tutorial battle, the spoiler log is
            written, and the 04/11 living-room save carries the run.
  reload    that save with every setting OFF: LOAD restores the run from the
            save's own block, the background check regenerates it identically.
  vanilla   a block-less retail checkpoint loaded with the randomizer ON stays
            vanilla: every probed row equals the stock bank.

The probe is the witness-only fixture mod tests/mod_fixtures/randomizer_probe.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))
import mod_witness
import mod_randomizer_engine_test as engine

PACKAGE = "p4pc.randomizer"
PROBE = "witness.randomizer-probe"
WALKS = ROOT / "tests/walks"
SLOT = "BASLUS-21782Save0"
SEED = "0123456789abcdeffedcba9876543210"
OPTIONS = 4095
# The settings keys of features R1..R12, in feature order (mod.ini).
FEATURE_KEYS = ("r1_skills", "r2_stats", "r3_affinities", "r4_fusion", "r5_enemies", "r6_enemy_stats", "r7_drops",
                "r8_chests", "r9_shops", "r10_shuffle", "r11_starter", "r12_music")


def features_text(options):
    """The mod's own spelling of a feature set (optionsText), e.g. R1,R3,R11."""
    return ",".join(f"R{i + 1}" for i in range(len(FEATURE_KEYS)) if options >> i & 1) or "none"


def settings(seed, options):
    """The mod settings that randomize the next New Game with this seed and feature set."""
    return {"randomize": "on", **{key: "on" for i, key in enumerate(FEATURE_KEYS) if options >> i & 1},
            "seed": "s:" + seed}


FEATURES = features_text(OPTIONS)
PROBES = (1, 7, 14, 26, 46, 172)
NEWGAME_ENV = {"P4PC_AUTOPRESS_NEWGAME": "1560", "P4PC_AUTOPRESS": "cross@2300:2/150*20,start@6500:2/300*14",
               "P4PC_FACING_GATE": "1", "P4PC_BATTLE_FIRST": "down,cross,cross,cross",
               "P4PC_BATTLE_DRIVER": "cross,cross,cross", "P4PC_BATTLE_SKIP": "140"}
STRIDES = {"PERSONA0": 14, "PERSONA1": 70, "UNIT2": 32}
ON = settings(SEED, OPTIONS)


class Leg:
    def __init__(self, work, name, settings, common, driver, elf):
        self.dir = work / name
        self.dir.mkdir()
        self.card = self.dir / "card" / "mc0"
        self.card.parent.mkdir()
        self.name = name
        self.common = common
        self.driver = driver
        self.elf = elf
        (self.dir / "p4pc.ini").write_text(f"[mods]\nenabled={PACKAGE},{PROBE}\n", encoding="ascii")
        store = self.dir / "mod-settings"
        store.mkdir()
        (store / (PACKAGE + ".ini")).write_text(
            "".join(f"{key} = {value}\n" for key, value in settings.items()), encoding="ascii")

    def walk(self, walk, gate, timeout, extra=None, drop=()):
        env = dict(self.common, P4PC_CONFIG=str(self.dir / "p4pc.ini"), P4PC_SAVE_DIR=str(self.card.parent),
                   P4PC_SCENE_ARTIFACTS=str(self.dir), P4PC_WALK_SCRIPT=str(WALKS / walk), P4PC_NEWGAME_GATE=gate)
        env.update(extra or {})
        for key in drop:
            env.pop(key, None)
        log = self.dir / "run.log"
        with log.open("wb") as output:
            process = subprocess.Popen([*self.driver, str(self.elf)], cwd=self.dir, env=env, stdout=output,
                                       stderr=subprocess.STDOUT,
                                       creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise AssertionError(f"{self.name}: timed out after {timeout}s: see {log}")
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text, f"{self.name}: not a witness run"
        assert "RANDO_READY" in text and "PROBE_READY" in text, f"{self.name}: a mod did not start: see {log}"
        assert "budget exhausted" not in text, f"{self.name}: an interpreter budget was exhausted: see {log}"
        assert process.returncode == 0, f"{self.name} exited {process.returncode}: see {log}"
        print(f"mod_randomizer_game: {self.name} ok")
        return text


def need(text, pattern, what):
    match = re.search(pattern, text)
    assert match, f"missing {what}: /{pattern}/"
    return match


def probe_rows(text, tag):
    rows = {}
    for key, persona, value in re.findall(rf"PROBE_ROW {tag} (\w+) (\d+) ([0-9a-f]+)", text):
        rows[(key, int(persona))] = value
    assert len(rows) == 3 * len(PROBES), f"{tag}: probed {len(rows)} rows, expected {3 * len(PROBES)}"
    return rows


def probe_records(text, tag):
    records = {}
    for persona, ident, level, skills, stats, affinity in re.findall(
            rf"PROBE {tag} persona=(\d+) id=(\d+) level=(\d+) skills=(\S+) stats=(\S+) affinity=(\S+)", text):
        records[int(persona)] = (int(ident), int(level), skills.split(","), [int(v) for v in stats.split(",")],
                                 affinity)
    assert set(records) == set(PROBES), (tag, sorted(records))
    return records


def check_fusions(text, tag, expected, which, kind="FUSE"):
    """The game's own fclFusePair (0x003124a0), or with kind FUSE3 fclFuseTriple
    (0x003127e0), gives what the headless evaluator computes over the live pair
    or triangle charts (index 0 generated, 1 stock); a fusion a special recipe
    claims (status 2) is skipped. Returns how many differ from stock."""
    width = 2 if kind == "FUSE" else 3
    found = re.findall(rf"PROBE_{kind} {tag} " + r"(\d+) " * width + r"ret=(\d+) result=(\d+)", text)
    assert len(found) == len(expected), (tag, kind, found)
    differs = 0
    for row in found:
        status, result = row[width], row[width + 1]
        want = expected[tuple(int(v) for v in row[:width])]
        if int(status) == 1:
            assert int(result) == want[which], (tag, row, want)
            differs += want[0] != want[1]
        else:
            assert int(status) in (0, 2), (tag, row)
    return differs


def check_banks(text, tag, expected, which):
    """The live encounter, treasure and enemy banks (index 0 generated, 1 stock).
    Returns how many differ from stock."""
    found = dict(re.findall(rf"PROBE_BANK {tag} (\w+) ([0-9a-f]{{64}})", text))
    assert set(found) == set(expected), (tag, sorted(found))
    for key, digest in found.items():
        assert digest == expected[key][which], (tag, key, digest, expected[key])
    return sum(expected[key][0] != expected[key][1] for key in found)


def check_records(records, rows):
    """The game's own record builder agrees with the live rows: level and stats
    from PERSONA0, the starting (level-offset 0) learn events from PERSONA1."""
    for persona, (ident, level, skills, stats, _) in records.items():
        meta = bytes.fromhex(rows[("PERSONA0", persona)])
        growth = bytes.fromhex(rows[("PERSONA1", persona)])
        assert ident == persona and level == meta[3], (persona, ident, level, meta[3])
        assert stats == list(meta[4:9]), (persona, stats, list(meta[4:9]))
        start = []
        for slot in range(16):
            at = 6 + slot * 4
            if growth[at + 1] == 0:
                break
            if growth[at + 1] == 1 and growth[at] == 0:
                start.append(f"{int.from_bytes(growth[at + 2:at + 4], 'little'):03x}")
        assert skills[:len(start)] == start[:8], (persona, skills, start)


def expected_run(runner, iso, directory, seed=SEED, options=OPTIONS):
    engine.extract(iso, directory)
    lines = engine.run(runner, "expect", str(directory), seed, str(options), *map(str, PROBES))
    code, digest, output = need("\n".join(lines), r"EXPECT_RUN (\S+) ([0-9a-f]{64}) ([0-9a-f]{64})", "expected run").groups()
    rows, stock, fusions, triples, banks = {}, {}, {}, {}, {}
    for line in lines:
        parts = line.split(" ")
        if parts[0] in ("EXPECT_ROW", "STOCK_ROW"):
            (rows if parts[0] == "EXPECT_ROW" else stock)[(parts[1], int(parts[2]))] = parts[3]
        elif parts[0] == "EXPECT_FUSE":
            fusions[(int(parts[1]), int(parts[2]))] = (int(parts[3]), int(parts[4]))
        elif parts[0] == "EXPECT_FUSE3":
            triples[(int(parts[1]), int(parts[2]), int(parts[3]))] = (int(parts[4]), int(parts[5]))
        elif parts[0] == "EXPECT_BANK":
            banks[parts[1]] = (parts[2], parts[3])
    begin, end = lines.index("SPOILER_BEGIN"), lines.index("SPOILER_END")
    return {"code": code, "digest": digest, "output": output, "rows": rows, "stock": stock, "fusions": fusions,
            "triples": triples, "banks": banks, "seed": seed, "options": options, "features": features_text(options),
            "settings": settings(seed, options), "lines": lines, "spoiler": "\n".join(lines[begin + 1:end])}


def newgame(leg, seeds, expected):
    run = leg("newgame", ON)
    text = run.walk("newgame_save0411_to_rain_scene.walk", "90000,-1:-1", 5400, extra=NEWGAME_ENV,
                    drop=("P4PC_AUTOPRESS_LOAD",))
    need(text, r"RANDO_RESET pending=true", "the New Game reset marking a pending run")
    need(text, r"RANDO_NEW_GAME armed seed=" + SEED, "the New Game arming with the stored seed")
    run_line = need(text, r"RANDO_RUN new_game code=(\S+) digest=([0-9a-f]+) features=" + FEATURES + r" output=([0-9a-f]+)",
                    "the New Game run").groups()
    assert run_line == (expected["code"], expected["digest"], expected["output"]), (run_line, expected)
    rows = probe_rows(text, "applied")
    for key in rows:
        want = expected["stock"][key] if key[1] == 1 else expected["rows"][key]
        assert rows[key] == want, (key, rows[key], want)
    changed = sum(rows[key] != expected["stock"][key] for key in rows)
    check_records(probe_records(text, "applied"), rows)
    fused = check_fusions(text, "applied", expected["fusions"], 0)
    tripled = check_fusions(text, "applied", expected["triples"], 0, "FUSE3")
    moved = check_banks(text, "applied", expected["banks"], 0)
    # Izanagi keeps its stock starting skills through the tutorial battle.
    roster = re.findall(r"PROBE_ROSTER battle id=1 level=\d+ skills=(\S+)", text)
    assert roster, "the tutorial battle did not report the protagonist's Persona"
    izanagi = bytes.fromhex(expected["stock"][("PERSONA1", 1)])
    starting = [f"{int.from_bytes(izanagi[6 + s * 4 + 2:6 + s * 4 + 4], 'little'):03x}" for s in range(16)
                if izanagi[6 + s * 4 + 1] == 1 and izanagi[6 + s * 4] == 0]
    assert roster[0].split(",")[:len(starting)] == starting, (roster[0], starting)
    spoiler = run.dir / "mod-artifacts" / PACKAGE / "runs" / expected["digest"][:16] / "spoiler.txt"
    assert spoiler.is_file(), f"no spoiler at {spoiler}"
    assert spoiler.read_text(encoding="utf-8") == expected["spoiler"], "the game's spoiler differs from the headless run"
    need(text, r"RANDO_SAVED code=" + re.escape(expected["code"]), "the 04/11 save carrying the run")
    print(f"mod_randomizer_game: newgame: run {expected['code']} live, {changed} of {len(rows)} probed rows "
          f"randomized, records built by the game match, the game's fusions follow the generated chart "
          f"({fused} of {len(expected['fusions'])} probed pairs changed) and the generated triangle charts ({tripled} "
          f"of {len(expected['triples'])} probed triples changed), the live encounter/treasure/enemy banks "
          f"are the generated ones ({moved} of {len(expected['banks'])} differ from stock), Izanagi stock, "
          f"spoiler written, saved")
    return run


def reload(leg, seeds, expected, previous):
    run = leg("reload", {})
    shutil.copytree(previous.card, run.card)
    text = run.walk("loaded_idle.walk", "40000,-1:-1", 2400)
    run_line = need(text, r"RANDO_RUN load code=(\S+) digest=([0-9a-f]+) features=" + FEATURES + r" output=([0-9a-f]+)",
                    "the loaded run").groups()
    assert run_line == (expected["code"], expected["digest"], expected["output"]), (run_line, expected)
    need(text, r"RANDO_VERIFY ok code=" + re.escape(expected["code"]), "the background regeneration check")
    assert "RANDO_NEW_GAME" not in text, "a load armed a New Game"
    rows = probe_rows(text, "applied")
    for key in rows:
        want = expected["stock"][key] if key[1] == 1 else expected["rows"][key]
        assert rows[key] == want, (key, rows[key], want)
    check_records(probe_records(text, "applied"), rows)
    check_fusions(text, "applied", expected["fusions"], 0)
    check_fusions(text, "applied", expected["triples"], 0, "FUSE3")
    check_banks(text, "applied", expected["banks"], 0)
    print("mod_randomizer_game: reload: the save's own block restored the run with every setting OFF; "
          "the regeneration check passed")


def vanilla(leg, seeds, expected):
    run = leg("vanilla", ON)
    shutil.copytree(seeds / "story0420", run.card)
    text = run.walk("loaded_idle.walk", "40000,-1:-1", 2400)
    need(text, r"RANDO_RUN load vanilla", "the block-less save loading vanilla")
    assert "RANDO_RUN new_game" not in text and "RANDO_RUN load code" not in text, "a vanilla save was randomized"
    rows = probe_rows(text, "stock")
    for key in rows:
        assert rows[key] == expected["stock"][key], (key, rows[key], expected["stock"][key])
    check_fusions(text, "stock", expected["fusions"], 1)
    check_fusions(text, "stock", expected["triples"], 1, "FUSE3")
    check_banks(text, "stock", expected["banks"], 1)
    print("mod_randomizer_game: vanilla: a block-less save stays vanilla with the randomizer ON")


def common_env(mods, iso, driver):
    """The headless, scripted, mod-witness environment every leg starts from."""
    common = dict(os.environ, P4PC_MOD_DIR=str(mods), P4PC_MODS="1", P4PC_PACKS="0",
                  P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso), P4PC_HIDDEN_WINDOW="1",
                  P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                  P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
    if os.name == "nt" or any("wine" in Path(part).name.lower() for part in driver[:-1]):
        common.pop("SDL_VIDEODRIVER", None)
    else:
        common["SDL_VIDEODRIVER"] = "offscreen"
    for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_DUMP_RAM",
                "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT", "P4PC_NEWGAME_GATE",
                "P4PC_AUTOPRESS", "P4PC_AUTOPRESS_NEWGAME", "P4PC_BATTLE_FIRST", "P4PC_BATTLE_DRIVER",
                "P4PC_BATTLE_SKIP", "P4PC_FACING_GATE"):
        common.pop(key, None)
    return common


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--runner", nargs="+", required=True, help="luau_script_runner (and an emulator before it)")
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True,
                        help="native player (and optional Wine prefix)")
    args = parser.parse_args()
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("randomizer proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("randomizer proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", "")).resolve()
    iso = Path(os.environ.get("P4PC_ISO", "")).resolve()
    seeds = Path(os.environ.get("P4PC_RANDOMIZER_SEEDS", str(elf.parent / "saves")))
    if not elf.is_file() or not iso.is_file() or not (seeds / "story0420" / SLOT).is_dir():
        print("mod_randomizer: retail ELF/ISO or the story0420 seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-randomizer-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        expected = expected_run(args.runner, str(iso), Path(tempfile.mkdtemp(prefix="banks-", dir=work)))
        mods = work / "mods"
        mods.mkdir()
        shutil.copytree(ROOT / "mods" / PACKAGE, mods / PACKAGE)
        shutil.copytree(ROOT / "tests" / "mod_fixtures" / "randomizer_probe", mods / PROBE)
        common = common_env(mods, iso, args.driver)

        def leg(name, settings):
            return Leg(work, name, settings, common, args.driver, elf)

        vanilla(leg, seeds, expected)
        first = newgame(leg, seeds, expected)
        reload(leg, seeds, expected, first)
        outcome = "pass"
        print(f"mod_randomizer_game: PASS run={expected['code']} mod_witness=true reference_eligible=false "
              f"evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
