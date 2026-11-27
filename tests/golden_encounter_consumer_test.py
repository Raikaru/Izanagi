"""Execute the actual native encounter selector, including the P10 assert."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from golden_model_rows_test import ROOT, function


def main():
    name = "src/Kosaka/k_encount.c"
    text = (ROOT / "decomp" / name).read_text(encoding="utf-8")
    for line in (ROOT / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines():
        fields = line.split("\t")
        if len(fields) < 4 or fields[0] != name:
            continue
        old, new = (s.replace("\\n", "\n") for s in fields[1:3])
        count = int(fields[4]) if len(fields) == 5 else 1
        if text.count(old) != count:
            raise RuntimeError(f"splice drift: {fields[3]}")
        text = text.replace(old, new)
    source = (ROOT / "tests/golden_encounter_consumer_fixture.c").read_text(encoding="utf-8")
    source = source.replace("/*@@CONSUMER@@*/", function(text, "Encount_PickEncounter"))
    with tempfile.TemporaryDirectory(prefix="p4-golden-p10-") as directory:
        work = Path(directory)
        path = work / "consumer.c"
        exe = work / ("consumer.exe" if os.name == "nt" else "consumer")
        path.write_text(source, encoding="utf-8")
        subprocess.run([sys.argv[1] if len(sys.argv)>1 else "clang", "-m32", "-std=c11",
                        "-I", str(ROOT / "include"), str(path),
                        str(ROOT / "src/platform/golden/golden_encounter_rates.c"), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
