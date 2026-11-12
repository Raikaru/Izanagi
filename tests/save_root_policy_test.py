"""Every test registered under tests/ has a memory-card save root of its own.

usage: save_root_policy_test.py <build dir> <ctest>

The game and scene gates run side by side from one working directory. A game without P4PC_SAVE_DIR uses
./p4pc_save there, and two games on one root contend for the card's directory lock; the one that is refused never
leaves the boot card check. The effective root of a test is the last P4PC_SAVE_DIR in its ENVIRONMENT. For every test
whose add_test call lies under the source tree's tests/ directory (the tool suites of tools/ run in working
directories of their own and boot no game), it must be set, lie below the build directory's save/ tree, and belong
to no other test.
"""
import json
import posixpath
import subprocess
import sys


def effective_root(properties):
    root = None
    for item in properties.get("ENVIRONMENT", []):
        name, separator, value = item.partition("=")
        if name == "P4PC_SAVE_DIR" and separator:
            root = value
    return root


def problems(tests, build):
    """`tests` maps a test name to its property dict; returns one line per violation."""
    below = posixpath.normpath(build.replace("\\", "/")) + "/save/"
    wrong, owners = [], {}
    for name, properties in sorted(tests.items()):
        root = effective_root(properties)
        if not root:
            wrong.append(f"{name}: no P4PC_SAVE_DIR; it would share ./p4pc_save")
            continue
        path = posixpath.normpath(root.replace("\\", "/"))
        if not (path + "/").startswith(below) or path + "/" == below:
            wrong.append(f"{name}: P4PC_SAVE_DIR {root} is not a directory below {below}")
        owner = owners.setdefault(path, name)
        if owner != name:
            wrong.append(f"{name}: shares P4PC_SAVE_DIR {root} with {owner}")
    return wrong


def self_check():
    build = "/b"
    own = {"ENVIRONMENT": ["A=1", "P4PC_SAVE_DIR=/b/save/x"]}
    cases = [
        ({"x": own}, 0),
        ({"x": own, "y": {"ENVIRONMENT": ["A=1"]}}, 1),                                   # default root
        ({"x": own, "y": {}}, 1),                                                         # no environment at all
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/b/save/x"]}}, 1),               # shared
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/b/save/./x/"]}}, 1),            # shared, other spelling
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/elsewhere/y"]}}, 1),            # outside the build tree
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/b/save"]}}, 1),                 # the tree itself
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR="]}}, 1),                        # empty means default
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/b/save/x", "P4PC_SAVE_DIR=/b/save/y"]}}, 0),  # last one wins
        ({"x": own, "y": {"ENVIRONMENT": ["P4PC_SAVE_DIR=/b/save/y", "P4PC_SAVE_DIR=/b/save/x"]}}, 1),
    ]
    for index, (tests, expected) in enumerate(cases):
        found = problems(tests, build)
        if len(found) != expected:
            print(f"self-check case {index}: expected {expected} problem(s), found {found}")
            return 1
    listing = {"backtraceGraph": {"files": ["/src/tests/cmake/100-scenes.cmake", "/src/tools/mods/CMakeLists.txt",
                                            "C:\\src\\tests\\CMakeLists.txt"],
                                  "nodes": [{"file": 0}, {"file": 1}, {"file": 2}]},
               "tests": [{"name": "gate", "backtrace": 0}, {"name": "tool", "backtrace": 1},
                         {"name": "win", "backtrace": 2}, {"name": "unknown"}]}
    if registered_under_tests(listing) != {"gate", "win", "unknown"}:
        print(f"self-check scope: {sorted(registered_under_tests(listing))}")
        return 1
    return 0


def registered_under_tests(listing):
    """The names of the listed tests whose add_test call is in a file under a tests/ directory."""
    graph = listing.get("backtraceGraph", {})
    files, nodes = graph.get("files", []), graph.get("nodes", [])
    names = set()
    for test in listing["tests"]:
        node = test.get("backtrace")
        if node is None or node >= len(nodes):
            names.add(test["name"])  # no origin recorded: checked, not skipped
            continue
        path = "/" + files[nodes[node]["file"]].replace("\\", "/").lstrip("/")
        if "/tests/" in path:
            names.add(test["name"])
    return names


def main(build, ctest):
    if self_check():
        return 1
    listing = json.loads(subprocess.run([ctest, "--test-dir", build, "--show-only=json-v1"], capture_output=True,
                                        text=True, check=True).stdout)
    checked = registered_under_tests(listing)
    tests = {}
    for test in listing["tests"]:
        if test["name"] in checked:
            tests[test["name"]] = {item["name"]: item["value"] for item in test.get("properties", [])}
    wrong = problems(tests, build)
    for line in wrong:
        print(line)
    print(f"save roots: {len(tests)} tests under tests/ ({len(listing['tests']) - len(tests)} elsewhere), "
          f"{len(wrong)} wrong")
    if not tests:
        print("no test was listed")
        return 1
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
