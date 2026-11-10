"""Every Windows x64 test run under Wine has its native twin's TIMEOUT times the Wine scale.

usage: wine_timeout_test.py <build dir> <scale> <ctest>

The twin of NAME64w is NAME64 when that test exists (the mod profiles), else NAME
(the host witness clones of tests/cmake/220-port64-host.cmake). A 64w test may set
a TIMEOUT only through its twin's (cmake/wine-tests.cmake), so one with a TIMEOUT
whose twin has none, or with no twin at all, fails too: a new test cannot bypass the
scale. A 64w test without a TIMEOUT keeps the suite's default. OWN_BUDGET lists the
tests that have no native twin by design, with their reason.
"""
import json
import subprocess
import sys

OWN_BUDGET = {
    "wine_timeouts64w": "this check; it starts no Wine process",
    "windows_game_subsystem64w": "inspects PE headers on the host; it starts no Wine process",
}


def main(build, scale, ctest):
    listing = subprocess.run([ctest, "--test-dir", build, "--show-only=json-v1"], capture_output=True, text=True,
                             check=True)
    tests = {}
    for test in json.loads(listing.stdout)["tests"]:
        tests[test["name"]] = {item["name"]: item["value"] for item in test.get("properties", [])}
    checked, wrong = 0, []
    for name, properties in sorted(tests.items()):
        if not name.endswith("64w") or "port64win" not in properties.get("LABELS", []) or name in OWN_BUDGET:
            continue
        stem = name[:-3]
        twin = tests.get(stem + "64", tests.get(stem))
        native = twin.get("TIMEOUT") if twin else None
        timeout = properties.get("TIMEOUT")
        if not native:
            if timeout:
                wrong.append(f"{name}: TIMEOUT {timeout} without a native twin TIMEOUT to scale")
            continue
        checked += 1
        want = float(native) * scale
        if float(timeout or 0) != want:
            wrong.append(f"{name}: TIMEOUT {timeout} (native {native}, want {want:g})")
    for line in wrong:
        print(line)
    print(f"wine timeouts: {checked} checked, {len(wrong)} wrong, scale {scale:g}")
    if checked == 0:
        print("no Windows x64 test with a native TIMEOUT was found")
        return 1
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], float(sys.argv[2]), sys.argv[3]))
