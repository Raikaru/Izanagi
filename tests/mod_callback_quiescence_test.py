"""Exercise production bulk teardown from an active native callback; no retail data."""
import subprocess
import sys


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("hook", "event"):
        print("usage: mod_callback_quiescence_test.py hook|event [runner...] executable", file=sys.stderr)
        return 2
    kind, driver = sys.argv[1], sys.argv[2:]
    message = "bulk " + kind + " teardown requires quiescent " + ("dispatch" if kind == "hook" else "callbacks")
    for mode in ("--active-clear", "--active-owner-remove"):
        result = subprocess.run([*driver, mode], capture_output=True, text=True)
        if result.returncode == 77:
            return 77
        if result.returncode == 0 or message not in result.stderr:
            print(f"{kind} {mode} did not refuse active teardown (rc={result.returncode})\n"
                  + result.stdout + result.stderr, file=sys.stderr)
            return 1
    print(f"mod {kind} quiescence: PASS (active aggregate clear and owner removal refused)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
