"""Exercise production Golden result/fade helpers with real i386 guest layouts."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from golden_model_rows_test import ROOT, unit, function


def block(text, anchor):
    start = text.index(anchor)
    cursor = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    return text[start:cursor]


def main():
    # Fail on drift in every changed consumer, including result-reset hooks.
    target = unit("src/Battle/btlTarget_inferred_001f1030.c")
    action = unit("src/Battle/btlAction_inferred_001a1450.c")
    tint = unit("src/Battle/btlMain.c")
    voice = unit("src/Battle/btlVoice_inferred.c")
    # Camera classification already uses the shared active-domain query before
    # M7a; replay that existing production splice rather than adding a second.
    for line in (ROOT / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines():
        fields = line.split("\t")
        if len(fields) >= 4 and fields[0] == "src/Battle/btlVoice_inferred.c" and fields[3] == "002266b0 explicit active voice classification":
            old, new = (part.replace("\\n", "\n") for part in fields[1:3])
            if voice.count(old) != int(fields[4]):
                raise RuntimeError("camera active-domain splice drift")
            voice = voice.replace(old, new)
    consumers = [function(target, "p4_golden_interception_resolve"),
                 function(action, "p4_golden_effect_lead"),
                 function(tint, "p4_golden_fade_context"),
                 function(tint, "p4_golden_fade_candidate")]
    source = (ROOT / "tests/golden_interception_fixture.c").read_text(encoding="utf-8")
    source = source.replace("/*@@CONSUMERS@@*/", "\n".join(consumers))
    source = source.replace("/*@@RESIDUAL@@*/", block(action,
        "else if (p4_golden_battle_rows_enabled()) {"))
    source = source.replace("/*@@CUTIN@@*/", block(action,
        "if ((!(act->flags & 0x40)"))
    source = source.replace("/*@@ASSIST@@*/", function(action, "btlActionAssistUpdate"))
    high = action[action.index(
        "            if (p4_golden_battle_rows_enabled() && (p4_golden_effect_flags((u16)skill) & 0x10000u))"):]
    high = high[:high.index("            if ((side == 0)")]
    source = source.replace("/*@@HIGH@@*/", high)
    options = next(line for line in action.splitlines()
                   if "hitFlags |= (p4_golden_effect_flags" in line)
    source = source.replace("/*@@OPTIONS@@*/", options)
    genus = action[action.index("    side = (s8)unitSide;"):]
    genus = genus[:genus.index("    if ((side == 1)")]
    source = source.replace("/*@@GENUS@@*/", genus)
    actor_reset = target[target.index("    context[0xD9] = 0;"):]
    actor_reset = actor_reset[:actor_reset.index("    if (!(flags & 1)")]
    target_reset = target[target.index("        unit[0xD9] = 0;"):]
    target_reset = target_reset[:target_reset.index("        res = datCalcResolveSkillHit")]
    source = source.replace("/*@@ACTOR_RESET@@*/", actor_reset)
    source = source.replace("/*@@TARGET_RESET@@*/", target_reset)
    camera = voice[voice.index("    if (distance < 200.0f)"):]
    camera = camera[:camera.index("    stepX = direction.x")]
    source = source.replace("/*@@CAMERA@@*/", camera)
    with tempfile.TemporaryDirectory(prefix="p4-golden-interception-") as directory:
        path = Path(directory) / "interception.c"
        path.write_text(source, encoding="utf-8")
        executable = Path(directory) / ("interception.exe" if os.name == "nt" else "interception")
        subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "clang", "-m32", "-std=c11",
                        "-malign-double", "-mfpmath=sse", "-msse2",
                        "-I", str(ROOT / "include"), "-I", str(ROOT / "tests"),
                        "-I", str(ROOT / "decomp/include"),
                        str(path),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
