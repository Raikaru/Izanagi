"""Exercise the real vendored Analysis frontend through its JSON CLI."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    command = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="p4-luau-check-") as directory:
        root = Path(directory)
        definitions = root / "p4.d.luau"
        source = root / "mod.luau"
        definitions.write_text("declare p4: { version: () -> number }\n", encoding="utf-8")

        def check(expected, *args):
            result = subprocess.run([*command, *map(str, args)], capture_output=True, text=True)
            report = json.loads(result.stdout)
            assert result.returncode == expected, (result.returncode, report, result.stderr)
            assert report["ok"] == (expected == 0), report
            if expected:
                assert report["diagnostics"], report
                assert report["diagnostics"][0]["line"] >= 1, report
                assert report["diagnostics"][0]["column"] >= 1, report
            return report

        source.write_text("local n: number = p4.version()\nreturn n\n", encoding="utf-8")
        check(0, definitions, source)
        source.write_text("local n: string = p4.version()\nreturn n\n", encoding="utf-8")
        check(1, definitions, source)
        definitions.write_text("declare p4: { broken: }\n", encoding="utf-8")
        check(1, definitions, source)
        check(2, root / "missing", source)
        check(2)
    print("luau analysis: definitions, type diagnostics and JSON errors passed")


if __name__ == "__main__":
    main()
