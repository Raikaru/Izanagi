"""Every test with a row in tests/gate_limits.txt runs with P4PC_GATE_NAME set to its own name.

usage: gate_limit_names_test.py <build dir> <ctest> <gate_limits.txt>

src/render/p4gl_gl.c p4gl_gate_limit judges a gate by the row of its renderer only when P4PC_GATE_NAME names the gate.
the game runners that judge gates by renderer set it for the runs they make, but a plain `ctest` (a cloud GPU, a
developer's machine) did not, so the row (systems_shop_daidara64 NVIDIA 6.07) was never read there and a frame the
RTX 4080 passes at 6.0187% failed the llvmpipe limit of 6.0. tests/CMakeLists.txt sets the name for every row's test.
"""
import json
import subprocess
import sys


def rows(text):
    names = []
    for line in text.splitlines():
        fields = line.split("#", 1)[0].split()
        if len(fields) == 3 and fields[0] not in names:
            names.append(fields[0])
    return names


def problems(tests, names):
    """`tests` maps a test name to its property dict; returns one line per row whose registered test lacks its name."""
    wrong = []
    for name in names:
        if name not in tests:
            continue  # a row for a gate this configuration does not register
        values = [item.partition("=")[2] for item in tests[name].get("ENVIRONMENT", []) if item.startswith("P4PC_GATE_NAME=")]
        if not values or values[-1] != name:
            wrong.append(f"{name}: has a row in tests/gate_limits.txt but P4PC_GATE_NAME is {values[-1] if values else 'unset'}")
    return wrong


def self_check():
    text = "# c\nscene_a  NVIDIA 1.0\nscene_a  A40 2.0\nscene_b NVIDIA 3  # x\nbad line\n"
    if rows(text) != ["scene_a", "scene_b"]:
        print(f"self-check rows: {rows(text)}")
        return 1
    ok = {"ENVIRONMENT": ["A=1", "P4PC_GATE_NAME=scene_a"]}
    cases = [
        ({"scene_a": ok}, 0),
        ({"scene_a": ok, "scene_b": {"ENVIRONMENT": ["P4PC_GATE_NAME=scene_b"]}}, 0),
        ({"scene_a": {"ENVIRONMENT": ["A=1"]}}, 1),
        ({"scene_a": {}}, 1),
        ({"scene_a": {"ENVIRONMENT": ["P4PC_GATE_NAME=other"]}}, 1),
        ({"other": {}}, 0),
    ]
    for index, (tests, expected) in enumerate(cases):
        found = problems(tests, ["scene_a", "scene_b"])
        if len(found) != expected:
            print(f"self-check case {index}: expected {expected} problem(s), found {found}")
            return 1
    return 0


def main(build, ctest, limits):
    if self_check():
        return 1
    names = rows(open(limits, encoding="utf-8").read())
    listing = json.loads(subprocess.run([ctest, "--test-dir", build, "--show-only=json-v1"], capture_output=True,
                                        text=True, check=True).stdout)
    tests = {test["name"]: {item["name"]: item["value"] for item in test.get("properties", [])} for test in listing["tests"]}
    wrong = problems(tests, names)
    for line in wrong:
        print(line)
    present = [name for name in names if name in tests]
    print(f"gate limit names: {len(names)} gates with rows, {len(present)} registered here, {len(wrong)} wrong")
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3]))
