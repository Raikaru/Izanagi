"""Execute MODEL0 consumers after the production mirror splices (synthetic data)."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def unit(path):
    text = (ROOT / "decomp" / path).read_text(encoding="utf-8")
    fixes = (ROOT / "config/mirror_fixes.txt").read_text(encoding="utf-8")
    return _m7a_splices(text, fixes, path)


def _m7a_splices(text, fixes, path):
    """Apply the complete M7a section, including its non-milestone subheadings."""
    for line in fixes.split("# Golden M7a:", 1)[1].splitlines()[1:]:
        if re.match(r"# Golden M\d+[a-z0-9]*:", line):
            break
        fields = line.split("\t")
        if len(fields) < 4 or fields[0] != path:
            continue
        old, new = (part.replace("\\n", "\n") for part in fields[1:3])
        expected = int(fields[4]) if len(fields) == 5 else 1
        if text.count(old) != expected:
            raise RuntimeError(f"splice drift in {path}: {fields[3]}")
        text = text.replace(old, new)
    return text


def _test_milestone_sections():
    path = "src/synthetic.c"
    fixes = "\n".join((
        "# Golden M7a: synthetic fixture section.",
        f"{path}\tfirst\tmiddle\trepeated token\t2",
        "# Golden EFFECT0: a subheading, not a new milestone.",
        "# An ordinary comment containing M7d must not end the section.",
        f"{path}\tlast\tend\tfinal token\t1",
        "# Golden M7d: unrelated appended milestone.",
        f"{path}\tmissing\tforeign\tmust not be applied\t1",
    ))
    assert _m7a_splices("first first last", fixes, path) == "middle middle end"
    try:
        _m7a_splices("first last", fixes, path)
    except RuntimeError:
        pass
    else:
        raise AssertionError("M7a replacement-count drift must still fail")


def function(text, name):
    match = re.search(r"^.*\b" + re.escape(name) + r"\([^;]*?\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError(f"function not found: {name}")
    begin = text.index("{", match.start())
    depth = 1
    end = begin + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def main():
    _test_milestone_sections()
    init = unit("src/Battle/btlUnit_inferred_0019e9f0.c")
    timing = unit("src/Battle/btlUnit_inferred_001991c0.c")
    mapper = unit("src/Battle/btlUnit_inferred_001990d0.c")
    calc = unit("src/Main/Battle/Data/datCalc.c")
    parts = [function(init, name) for name in ("btlUnitInitFromRecord", "btlUnitGetRecordPtr")]
    parts += [function(timing, name) for name in ("btlUnitExtendSignedHalfword", "btlUnitMotionFrames", "btlUnitMotionFollowFrames", "btlUnitMotionHitFrames")]
    parts += [function(mapper, "btlUnitGetMotionId")]
    declaration = re.search(r"typedef struct DatCharacterTable\s*\{.*?\} DatCharacterTable;", calc, re.S)
    if not declaration:
        raise RuntimeError("character table layout not found")
    parts.append(declaration.group())
    parts += [function(calc, name) for name in ("datCalcGetCharacterTableKind", "datCalcSkillValueBase", "datCalcRollSupportSkillHit")]
    template = (ROOT / "tests/golden_model_rows_fixture.c").read_text(encoding="utf-8")
    source = template.replace("/*@@CONSUMERS@@*/", "\n".join(parts))
    with tempfile.TemporaryDirectory(prefix="p4-golden-model-rows-") as directory:
        work = Path(directory)
        path = work / "model.c"
        executable = work / ("model.exe" if os.name == "nt" else "model")
        path.write_text(source, encoding="utf-8")
        # These are the real 32-bit guest-layout bodies, not host-pointer
        # replicas. The complete generated 64-bit units are built by P2U.
        subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "clang", "-m32", "-std=c11",
                        "-malign-double", "-mfpmath=sse", "-msse2",
                        "-I", str(ROOT / "include"), "-I", str(ROOT / "tests"),
                        "-I", str(ROOT / "decomp/include"), str(path),
                        str(ROOT / "src/platform/golden/golden_battle_rows_native.c"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
