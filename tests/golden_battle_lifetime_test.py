"""Run actual spliced action/packet consumers with the real Golden sidecar."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from golden_model_rows_test import ROOT, unit, function


def main():
    action = unit("src/Battle/btlAction_inferred_001b0930.c")
    packet = unit("src/Battle/btlUnit_inferred_00193840.c")
    presentation = unit("src/Battle/btlAction_inferred_001a1450.c")
    target = unit("src/Battle/btlTarget_inferred_001f1030.c")
    formation = unit("src/Battle/btlFormation.c")
    sound = unit("src/Battle/btlSound.c")
    sound_update = unit("src/Battle/btlSound_inferred_001f7650.c")
    sound_lifecycle = unit("src/Battle/btlSound_inferred_001f6100.c")
    parts = [function(action, name) for name in
             ("btlActionCreate", "btlActionListUpdate", "btlActionListDestroyAll")]
    parts += [function(packet, name) for name in
              ("btlPacketConditionAlwaysMet", "btlPacketAlloc", "btlPacketEnqueue", "btlPacketStep", "btlPacketFlushAll", "btlPacketCreateImmediate")]
    # Combine TU-local u8*/BtlPacket* allocator declarations without changing
    # the allocator or its guest address; production generation aligns these.
    sound_constructor = function(sound, "btlSoundCreateChannelCuePacket").replace(
        "packet = btlPacketAlloc(", "packet = (BtlPacket *)btlPacketAlloc(").replace(
        "packet->updateFunc = btlSoundSetSlotChannel;",
        "packet->updateFunc = (BtlPacketFunc)btlSoundSetSlotChannel;")
    parts += [function(sound_update, "btlSoundSetSlotChannel"),
              function(sound_lifecycle, "btlSoundResetSlotQueue"),
              function(sound_lifecycle, "btlSoundAdvanceSlotQueue"),
              function(formation, "btlFormationCreatePacket"), sound_constructor]
    parts += [function(presentation, name) for name in (
        "p4_golden_interception_timing", "p4_golden_interception_create",
        "p4_golden_interception_voice_update", "p4_golden_interception_voice_create",
        "p4_golden_interception_message_update", "p4_golden_interception_message_create",
        "p4_golden_interception_followers")]
    message_type = re.search(
        r"typedef struct P4GoldenInterceptionMessageWork\s*\{.*?\} P4GoldenInterceptionMessageWork;",
        presentation, re.S)
    if not message_type:
        raise RuntimeError("Golden message work type not found")
    parts.insert(0, message_type.group())
    packet_type = re.search(r"typedef struct BtlPacketTiming\s*\{.*?\} BtlPacketTiming;", presentation, re.S)
    if not packet_type:
        raise RuntimeError("packet timing type not found")
    parts.insert(0, packet_type.group())
    work_type = re.search(r"typedef struct BtlFormationEffectWork\s*\{.*?\} BtlFormationEffectWork;", formation, re.S)
    if not work_type:
        raise RuntimeError("formation work type not found")
    parts.insert(0, work_type.group())
    parts += [function(target, "p4_golden_interception_resolve"),
              function(presentation, "p4_golden_effect_lead")]
    prelude = (ROOT / "tests/golden_battle_lifetime_fixture.c").read_text(encoding="utf-8")
    prelude = prelude.replace("/*@@CONSUMERS@@*/", "\n".join(parts))
    opening = presentation[presentation.index(
        "    if (p4_golden_battle_rows_enabled()) {\n        goldenLead ="):]
    opening = opening[:opening.index("\n    btlPacketEnqueue")]
    primary = presentation[presentation.index(
        "                    if (p4_golden_battle_rows_enabled()) {\n                        u32 delay = hitTimeTotal;"):]
    end = "                    } else (((BtlPacketTiming *)skillFormationPacket)->delay) = hitTimeTotal;"
    primary = primary[:primary.index(end) + len(end)]
    prelude = prelude.replace("/*@@PRIMARY_DELAY@@*/", primary)
    prelude = prelude.replace("/*@@OPENING@@*/", opening)
    bank = (ROOT / "tests/golden_battle_rows_test.c").read_text(encoding="utf-8")
    source = prelude + "\n#define P4_GOLDEN_LIFETIME_FIXTURE\n#define main bank_main\n" + bank
    source += "\n#undef main\nint main(void) { int rc = bank_main(); assert(!rc && !sidecar_live); return rc; }\n"
    with tempfile.TemporaryDirectory(prefix="p4-golden-battle-lifetime-") as directory:
        work = Path(directory)
        path = work / "lifetime.c"
        instrument = work / "rows.c"
        executable = work / ("lifetime.exe" if os.name == "nt" else "lifetime")
        path.write_text(source, encoding="utf-8")
        instrument.write_text(
            '#include <stdlib.h>\nextern void *p4_test_sidecar_malloc(size_t);\n'
            'extern void p4_test_sidecar_free(void *);\n#define malloc p4_test_sidecar_malloc\n'
            '#define free p4_test_sidecar_free\n#include "' +
            (ROOT / "src/platform/golden/golden_battle_rows.c").as_posix() + '"\n', encoding="utf-8")
        subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "clang", "-m32", "-std=c11",
                        "-malign-double", "-mfpmath=sse", "-msse2",
                        "-I", str(ROOT / "include"), "-I", str(ROOT / "tests"),
                        "-I", str(ROOT / "decomp/include"), str(path), str(instrument),
                        str(ROOT / "src/platform/golden/golden_battle_rows_native.c"),
                        str(ROOT / "src/platform/gport.c"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
