#!/usr/bin/env python3
"""Extract production mirrored consumers for the synthetic C fixture."""
from pathlib import Path
import re
import sys

root, out = map(Path, sys.argv[1:])
units = [
    ("src/unplaced/unplaced_inferred_0013ad40.c", "radarApplyItemStep", "p4_mod_authorize_item_use(0"),
    ("src/unplaced/unplaced_inferred_0013ad40.c", "radarHandleEntrySelect", "p4_mod_authorize_item_use(3"),
    ("src/Battle/btlEffect_inferred_00202c60.c", "btlSelectionHandleItemInput", "p4_mod_authorize_item_use(1"),
    ("src/Battle/btlAction_inferred_001b1510.c", "btlMainRevivalMesUpdate", "p4_mod_authorize_item_use(2"),
    ("src/Battle/btlAICommand.c", "p4_btl_decode_body", "case 0x8002:"),
    ("src/Battle/btlAICommand.c", "p4_btl_decode_metadata", "p4_btl_decode_body("),
    ("src/Battle/btlAICommand.c", "btlAICommandSetActionCode", "p4_btl_decode_body("),
    ("src/Battle/btlAICommand.c", "p4_game_elements_ai_decode_script", "p4_game_elements_ai_native_decode_begin("),
    ("src/Battle/btlAction_inferred_001a1450.c", "btlActionAiUpdate", "p4_ai_item_denied"),
    ("src/Battle/btlAction_inferred_001a1450.c", "btlActionWaitUpdate", "btlTargetCreateDataPacket("),
    ("src/Battle/btlAICommand.c", "btlAICommandScriptSetSkillAction", "p4_mod_authorize_item_use(4"),
    ("src/Battle/btlAICommand.c", "btlAICommandPickSkillTargetByHp", "p4_ai_item_prediction_before"),
    ("src/Battle/btlAICommand.c", "btlAICommandPickSkillTargetNearMaxHp", "p4_ai_item_prediction_before"),
    ("src/Battle/btlAICommand.c", "btlAICommandScriptWouldBeKilled", "p4_ai_item_prediction_before"),
    ("src/Battle/btlAICommand.c", "btlAICommandScriptSetId2ClearBuffs", "p4_ai_item_before"),
    ("src/Battle/btlAICommand.c", "btlAICommandScriptSetDownFlag", "p4_ai_item_before"),
    ("src/Battle/btlAICommand.c", "btlAICommandScriptSetSlipFlag", "p4_ai_item_before"),
]
parts = []
for filename, name, marker in units:
    text = (root / filename).read_text(encoding="utf-8")
    match = re.search(r"\b(?:s32|u16|void)\s+" + name + r"\s*\([^;]*?\)\s*\{", text, re.S)
    if not match:
        raise RuntimeError(f"missing generated function {name}")
    start = match.start()
    depth = 1
    end = match.end()
    while depth and end < len(text):
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    body = text[start:end]
    if depth or marker not in body:
        raise RuntimeError(f"missing authorization in {name}")
    if name in ("radarApplyItemStep", "radarHandleEntrySelect"):
        # Preserve the native 32-bit truncation without a host pointer-size
        # diagnostic; these history arguments are ignored by the stub.
        old = "radarPushHistoryValue((s32)work,"
        expected = 3 if name == "radarApplyItemStep" else 2
        if body.count(old) != expected:
            raise RuntimeError("unexpected native history-call shape")
        body = body.replace(old, "radarPushHistoryValue((s32)(intptr_t)work,")
    if name == "btlActionWaitUpdate":
        body = body.replace("(s32)(actionRecord->unit)", "(s32)(intptr_t)(actionRecord->unit)")
        body = body.replace("(u32)battleUnit", "(u32)(uintptr_t)battleUnit")
        body = body.replace("(s32)action", "(s32)(intptr_t)action")
    if name in ("btlAICommandScriptSetId2ClearBuffs", "btlAICommandScriptSetDownFlag",
                "btlAICommandScriptSetSlipFlag"):
        # The synthetic unit lives in host memory, unlike the 32-bit guest.
        body = body.replace("(u32)", "(uintptr_t)")
        body = body.replace("u32 dataAddress", "uintptr_t dataAddress")
        body = body.replace("u32 data;", "uintptr_t data;")
    if name in ("btlAICommandPickSkillTargetByHp", "btlAICommandPickSkillTargetNearMaxHp",
                "btlAICommandScriptWouldBeKilled"):
        body = body.replace("(u32)", "(uintptr_t)")
    parts.append(body)
ready = (root / "src/Battle/btlAction_inferred_001a1450.c").read_text(encoding="utf-8")
start = ready.index("if (((BtlAction *)actionBytes)->unit->genus == 0) {",
                    ready.index("void btlActionReadyUpdate("))
assert start < ready.index("btlTargetSavePair(actionBytes + 0x38)", start)
opening = ready.index("{", start)
depth, end = 1, opening + 1
while depth and end < len(ready):
    depth += (ready[end] == "{") - (ready[end] == "}")
    end += 1
guard = ready[start:end]
if depth or "p4_mod_authorize_item_use(5" not in guard or "p4_ai_item_finish" not in guard:
    raise RuntimeError("missing READY item admission")
parts.append("static void fixture_ready(u8 *actionBytes) {\n" + guard +
             "\nif (((BtlAction *)actionBytes)->unk_6c == 3) { ++applications; --stock; ++fieldHp; ++fieldSp; }\n}")
out.write_text("\n\n".join(parts) + "\n", encoding="utf-8")
