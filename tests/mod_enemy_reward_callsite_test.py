#!/usr/bin/env python3
"""Compile and run the generated native defeat/skill-roll bodies over synthetic records.

Requires the normal generated decomp mirror and mod-hook fixture. The two bodies
are copied verbatim from that mirror, not reconstructed: the game records and
unrelated native providers in the C harness are synthetic. No game or saves run.
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_port  # noqa: E402


def body(path, address, name):
    text = path.read_text(encoding="utf-8")
    matches = [m for m in gen_port.MARKER_RE.finditer(text) if int(m.group(1), 16) == address]
    if len(matches) != 1:
        raise RuntimeError(f"{path}: expected exactly one marker for {address:08x}")
    marker = matches[0]
    plain = gen_port.opaque_blanked(text)
    found = gen_port.marked_definition(plain, marker.end(), len(plain), {name})
    if not isinstance(found, tuple):
        raise RuntimeError(f"{path}: cannot identify {name}")
    start, opening, _ = found
    depth = 0
    for index in range(opening, len(plain)):
        if plain[index] == "{":
            depth += 1
        elif plain[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1] + "\n"
    raise RuntimeError(f"{path}: unterminated {name}")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: mod_enemy_reward_callsite_test.py BUILD_GEN CLANG")
    gen = Path(sys.argv[1]).resolve()
    clang = sys.argv[2]
    mirror = gen / "decomp"
    out = gen / "reward-callsite-fixture"
    out.mkdir(parents=True, exist_ok=True)
    caller = body(mirror / "src/Battle/btlTarget_inferred_001f1030.c", 0x001F2CC0,
                  "btlTargetRecordDefeatReward")
    roll = body(mirror / "src/Main/Battle/Data/datCalc.c", 0x002384B0, "datCalcSkillRoll")
    if caller.count("p4_mod_reward_begin()") != 2 or caller.count("p4_mod_reward_end(") != 2:
        raise RuntimeError("generated defeat caller lacks the two reward scopes")
    if "datCalcEnemyMoney((u8 *)unitArg)" not in roll:
        raise RuntimeError("generated kind-six skill roll no longer calls the yen calculator")
    (out / "defeat.c").write_text('''#include "type.h"
#include "btl_action_internal.h"
#include "btl_effect_internal.h"
#include "member_access.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/game_enemies_native.h"
extern u8 *D_0076449C, D_0060AB08[];
extern void kAssertFail(void *, s32);
extern u16 datCalcEnemyExp(s32), datCalcEnemyMoney(s32);
extern u16 datCalcPickWeightedSkill(u8 *, s32 *);
extern u32 datCalcRandomBelow(u32);
''' + caller, encoding="utf-8")
    (out / "skill.c").write_text('''#include "datUnit_internal.h"
#include "datPersona_internal.h"
#include "p4pc/mod_entry.h"
extern u8 *iGpffffb3b8, D_00635938[];
extern void kAssertFail(void *, s32);
extern u8 *datUnitGetRecord(s32);
extern u8 datPersonaGetLevel(s32);
extern u8 datCalcGetBattleStat(DatUnit *, s32);
extern u32 datGetMoney(void), RpRandom(void);
extern u16 datCalcEnemyMoney(u8 *);
''' + roll, encoding="utf-8")
    binary = out / "reward-callsite"
    command = [clang, "-m32", "-malign-double", "-fno-strict-aliasing", "-ffp-contract=off", "-O2", "-no-pie",
               f"-I{ROOT / 'include'}", f"-I{mirror / 'include'}", f"-I{mirror / 'src'}", f"-I{mirror}",
               str(out / "defeat.c"), str(out / "skill.c"), str(ROOT / "tests/mod_enemy_reward_callsite_test.c"),
               str(gen / "mod_hook_fixture/game.c"), str(gen / "mod_hook_fixture/mod_bridges.c"),
               str(ROOT / "src/platform/mods/hooks.c"), str(ROOT / "src/platform/mods/numbers.c"), "-lm", "-o", str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
