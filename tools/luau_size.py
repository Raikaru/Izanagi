"""Report comparable stripped Luau probe deltas; never modifies build outputs."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--strip", required=True)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--suffix", default="")
    parser.add_argument("--exe-suffix", default="")
    args = parser.parse_args()
    sizes = {}
    imports = {}
    with tempfile.TemporaryDirectory(prefix="p4-luau-size-") as temporary:
        for name in ("p4_luau_size_baseline", "p4_luau_size_probe", "p4_luau_check"):
            source = args.build / (name + args.suffix + args.exe_suffix)
            output = Path(temporary) / source.name
            subprocess.run([args.strip, "--strip-all", "-o", str(output), str(source)], check=True)
            sizes[name] = output.stat().st_size
            headers = subprocess.run([args.objdump, "-p", str(source)], check=True,
                                     capture_output=True, text=True).stdout
            imports[name] = re.findall(r"(?:DLL Name:|NEEDED)\s+(\S+)", headers)
    print(json.dumps({"profile": args.profile, "release": "0.744",
                      "commit": "32c28089305f488f901aaf1dbf3fd1cb65fa3d11",
                      "stripped_bytes": sizes,
                      "dynamic_imports": imports,
                      "runtime_added_bytes": sizes["p4_luau_size_probe"] - sizes["p4_luau_size_baseline"],
                      "method": "source-compile/load/run probe minus empty C main, same flags/static runtime; not whole-game delta"}, sort_keys=True))


if __name__ == "__main__":
    main()
