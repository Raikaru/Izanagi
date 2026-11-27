"""Execute threshold, sprite and result-counter splices on synthetic vectors."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from golden_model_rows_test import ROOT, unit, function


def conditional(text, condition):
    """Keep the actual if/else statement, including its native branch bodies."""
    start = text.index(condition)
    end = start
    while True:
        begin = text.index("{", end)
        depth, end = 1, begin + 1
        while depth:
            depth += (text[end] == "{") - (text[end] == "}")
            end += 1
        tail = re.match(r"\s*else\b", text[end:])
        if not tail:
            return text[start:end]
        end += tail.end()


def main():
    calc = unit("src/Main/Battle/Data/datCalc.c")
    action = unit("src/Battle/btlAction_inferred_001a1450.c")
    target = unit("src/Battle/btlTarget_inferred_001f1030.c")
    template = (ROOT / "tests/golden_skill_flags_fixture.c").read_text(encoding="utf-8")
    packet = re.search(r"typedef struct BtlPacketTiming\s*\{.*?\} BtlPacketTiming;", action, re.S)
    if not packet:
        raise RuntimeError("native packet timing type not found")
    skill_action = re.search(r"typedef struct BtlSkillAction\s*\{.*?\} BtlSkillAction;", action, re.S)
    if not skill_action:
        raise RuntimeError("native skill-action view not found")
    source = template.replace("/*@@TYPES@@*/", packet.group() + "\n" + skill_action.group())
    source = source.replace("/*@@THRESHOLD@@*/", function(calc, "datCalcThresholdTrigger"))
    source = source.replace("/*@@UNIT_ACCESSOR@@*/",
                            function(unit("src/g_data/g_data.c"), "p4_golden_skill_unit_guest"))
    source = source.replace("/*@@SPRITE@@*/", conditional(function(action, "btlActionSkillUpdate"), "if (isKind3 == 0)"))
    source = source.replace("/*@@COUNTER@@*/", conditional(function(target, "btlTargetResolveSelectedSkill"), "if (recordResult != 1 ||"))
    with tempfile.TemporaryDirectory(prefix="p4-golden-skill-flags-") as directory:
        work = Path(directory)
        path = work / "flags.c"
        executable = work / ("flags.exe" if os.name == "nt" else "flags")
        path.write_text(source, encoding="utf-8")
        subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "clang", "-m32", "-std=c11",
                        "-malign-double", "-mfpmath=sse", "-msse2",
                        "-I", str(ROOT / "include"), "-I", str(ROOT / "tests"),
                        "-I", str(ROOT / "decomp/include"), str(path), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
