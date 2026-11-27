"""Compile actual pinned-decomp read sites with their M6b production splices.

Only unrelated renderer/service framing is supplied by the witness. No game
payload is copied: emitted code is built from the user's pinned decomp checkout.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
decomp, output = map(Path, sys.argv[1:])


def unit(name):
    text = (decomp / name).read_text(encoding='utf-8')
    for row in (ROOT / 'config/mirror_fixes.txt').read_text(encoding='utf-8').splitlines():
        parts = row.split('\t')
        if len(parts) < 5 or parts[0] != name:
            continue
        # Earlier unrelated recipe/asset passes expect gen_port's canonicalized
        # whole TU, not raw decomp. They do not modify these isolated read sites;
        # the full mirror build independently enforces their original contracts.
        if not parts[3].startswith(('M6b ', 'M6c ')):
            continue
        before, after = (part.replace('\\n', '\n') for part in parts[1:3])
        if text.count(before) != int(parts[4]):
            raise SystemExit('Literal witness splice count differs: ' + parts[3])
        text = text.replace(before, after)
    return text


def function(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise SystemExit('Missing actual consumer: ' + name)
    start = text.rfind('\n', 0, match.start()) + 1
    opening = text.index('{', match.start())
    # Ignore braces in comments/literals while preserving original source offsets.
    scrubbed = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: ' ' * len(m[0]), text, flags=re.S)
    depth = 1
    end = opening + 1
    while depth and end < len(text):
        depth += (scrubbed[end] == '{') - (scrubbed[end] == '}')
        end += 1
    if depth:
        raise SystemExit('Unbalanced actual consumer: ' + name)
    result = text[start:end]
    # The production pointer rewriter resolves these externs to their guest
    # addresses. The isolated witness uses equivalent address macros instead.
    return re.sub(r'^\s*extern\s+(?:s32|f32|u8)\s+D_[0-9A-F]+\[\];\s*$', '', result, flags=re.M)


def structure(text, name):
    match = re.search(r'typedef struct(?:\s+\w+)?\s*\{[^{}]*\}\s*' + re.escape(name) + r';', text)
    if not match:
        raise SystemExit('Missing actual consumer type: ' + name)
    return match[0]


calendar = unit('src/datCalendar_inferred_001104a0.c')
misc = unit('src/Yajima/y_misc_inferred.c')
parts = [function(misc, name) for name in ('fclAddClamped', 'fclSubClamped')]
parts.extend(function(calendar, name) for name in (
    'calGetMonthLength', 'calSplitDayOfYear', 'calGetWeekdayOfDay',
    'calDayOfYearFromMonthDay', 'calIsDayInRange'))
parts.append(function(unit('src/unplaced/unplaced_inferred_00110850.c'), 'calGetDayEntryForMonthDay'))
parts.append(function(unit('src/Event/mt_misc_inferred_002981f0.c'), 'mtMiscDispatchKeyAction'))
fusion = unit('src/Event/Fcl/y_fclCombine.c')
parts.append(structure(fusion, 'FclCombineRule'))
parts.append(structure(fusion, 'FclCombineSlot'))
parts.append(function(fusion, 'fclCombineCheckRules'))
outcomes = function(fusion, 'fclCombineRollOutcomeList')
outcomes = outcomes.replace('(u32)D_0063FCA0', 'P4_H2G(D_0063FCA0)')
outcomes = outcomes.replace('((s8 *)selectedAddress)', '((s8 *)P4_G2H(selectedAddress))')
parts.append(outcomes)
date = unit('src/unplaced/unplaced_inferred_003136b0.c')
parts.extend(structure(date, name) for name in ('CmmDateOption', 'CmmDateRecord', 'DateSelectionWork'))
life = unit('src/unplaced/unplaced_inferred_002e78a0.c')
parts.append(function(life, 'fclCalGetNextDate'))
parts.extend(function(date, name) for name in (
    'cmmFindDateEntry', 'cmmDateSelectionSelect', 'cmmDateSelectionRefresh'))
create_lines = [line for line in function(fusion, 'fclCombineMenuCreate').splitlines()
                if 'p4_golden_literal_weather_work_register' in line]
if len(create_lines) != 1:
    raise SystemExit('Missing actual weather work registration')
parts.append('void witness_weather_create(u8 *out)\n{\n' + create_lines[0] + '\n}')
destroy = function(life, 'fclCombineMenuDestroy').splitlines()
cleanup = [i for i, line in enumerate(destroy) if 'p4_golden_literal_weather_work_release' in line]
if len(cleanup) != 1 or 'jtbl_008873EC' not in destroy[cleanup[0] + 2]:
    raise SystemExit('Missing actual weather companion-before-work cleanup')
parts.append('void witness_weather_destroy(KwlnTask *task)\n{\n    u8 *work = task->work;\n' +
             '\n'.join(destroy[cleanup[0]:cleanup[0] + 3]) + '\n}')
growth = function(fusion, 'fclCombineMenuResultGrowth')
skill_start = growth.index('        row = D_0063FCA0 + ((FclCombineWork *)p)->ruleIndices[0] * 28;')
skill_end = growth.index('        shdPersonaSetPendingSkill(', skill_start)
parts.append('s16 witness_weather_skill(u8 *p)\n{\n'
             '    FclCombineWork *menuState = (FclCombineWork *)p;\n'
             '    u8 *row; FclCombineSlot *cal; s16 i10, i11;\n' +
             growth[skill_start:skill_end] + '\n    return menuState->outcome.signedValue;\n}')
weather_draw = unit('src/Event/Fcl/y_fclCombineDraw.c')
parts.extend(structure(weather_draw, name) for name in ('FclCombineDetailEffect', 'FclCombineCalendarRule'))
detail = function(weather_draw, 'fclCombineDrawDetailPanel')
kind_line = next(line for line in detail.splitlines() if 'if (' in line and 'p4_golden_literal_date_constraint(v, 0)' in line)
kind_expression = re.search(r'if \((.*) != 0\)', kind_line)[1]
first_expression = re.search(r'firstPersona = (.*);', detail)[1]
second_expression = re.search(r'secondPersona = (.*);', detail)[1]
effect_line = next(line for line in detail.splitlines() if 'id =' in line and 'p4_golden_literal_date_effect(v, j)' in line)
argument_line = next(line for line in detail.splitlines() if 'func_00243840(' in line and 'p4_golden_literal_date_effect(v, j)' in line)
argument_expression = re.search(r'func_00243840\((.*)\)\);', argument_line)[1]
parts.append('void witness_weather_display(s8 v, s16 j, s16 header[3], u32 *effect)\n{\n'
             '    u8 *ent = D_006432B0 + v * 28, *e = ent + j * 4; s8 id;\n' +
             '    header[0] = ' + kind_expression + ';\n'
             '    header[1] = ' + first_expression + ';\n'
             '    header[2] = ' + second_expression + ';\n' + effect_line + '\n'
             '    *effect = (u32)(u8)id | (u32)(u16)(' + argument_expression + ') << 16;\n}')
parts.extend(function(fusion, name) for name in (
    'fclCombineStageValue', 'fclCombineRollOutcome', 'fclCombinePickItem'))
parts.append(function(unit('src/unplaced/unplaced_inferred_00311900.c'), 'cmmCommunityExpRemaining'))
parts.append(function(unit('src/Script/scrScriptProcess.c'), 'scrCreateProcedureWork'))

sky = function(unit('src/Battle/btlMain_inferred_001b5ec0.c'), 'btlMainSetupCameraAndEffects')
start = sky.index('        tblA = D_0060A410;')
end = sky.index('        base = D_0076449C;', start)
parts.append('void witness_sky(u8 *dstPos, u8 *dstUv, u8 *dstCol)\n{\n'
             '    f32 *tblA, *tblB, uvScale; int i = 0; u8 white;\n' + sky[start:end] + '\n}')
cue = function(unit('src/unplaced/unplaced_inferred_00383c00.c'), 'btlShuffleDrawSpriteCues')
start = cue.index('    for (i = 0; i < 11; i++)')
parts.append('void witness_cues(u16 *counter, s32 palette)\n{\n'
             '    s32 i; f32 alpha, scaled; u8 alphaByte, red, green, blue;\n' + cue[start:])

menu = function(fusion, 'fclCombineMenuOptionPick')
start = menu.index('                    sidx = (s8)(10.0f * datGetAverageRecordScore());')
end = menu.index('                    break;', start)
message = menu[start:end]
# The player rewriter translates this guest-address result before the host call.
message = message.replace('(void *)fclCombineGetMesWindow()',
                          'P4_G2H((u32)fclCombineGetMesWindow())')
parts.append('void witness_month_message(u8 *p)\n{\n    s8 sidx; u8 *te;\n' + message + '\n}')

portrait = function(unit('src/unplaced/unplaced_inferred_00210c70.c'), 'btlPortraitDraw')
alpha_reads = [line for line in portrait.splitlines() if 'alpha =' in line and 'D_00626F00[' in line]
if len(alpha_reads) != 1:
    raise SystemExit('Missing or ambiguous 0x626f00 consumer read')
parts.append('u8 witness_layout_alpha(u16 effectFrame)\n{\n    u8 alpha;\n' +
             alpha_reads[0] + '\n    return alpha;\n}')

axis_reads = []
for path, owner, count in (
    ('src/Scene/mt_scene.c', 'MT_Scene_BuildResMatrix', 1),
    ('src/Kosaka/Field/k_sceneDraw_inferred_0028c3f0.c', 'evtUpdateSceneLight', 3)):
    found = 0
    for line in function(unit(path), owner).splitlines():
        if 'D_005EFA20' not in line or line.strip().startswith('extern '):
            continue
        call = re.search(r'RwMatrixRotate\([^,]+,\s*([^,]+),', line)
        if not call or 'D_005EFA20' not in call[1]:
            raise SystemExit('Unclassified Y-axis read in ' + owner)
        axis_reads.append('    axis_context_reference((const void *)(' + call[1] + '));')
        found += 1
    if found != count:
        raise SystemExit('Missing Y-axis argument witness: ' + owner)
parts.append('void witness_axis_contexts(void)\n{\n' + '\n'.join(axis_reads) + '\n}')

tn = unit('src/Graphics/Effect/tnEffect.c')
parts.append(re.search(r'^static u8 tnSlotRemap\[8\] = .*;$', tn, re.M)[0])
for name in ('TnSetTableBase', 'TnGetTableBase', 'TnGetOffset1C', 'TnGetOffset38', 'TnGetIndexedWord'):
    body = function(tn, name)
    # Equivalent guest-pointer translation for the isolated host witness;
    # the full player uses gptr_rewrite's existing absolute/guest-slot rules.
    body = body.replace('(u32)tnTableBase', 'P4_H2G(tnTableBase)')
    body = body.replace('(s32)tnTableBase', '(s32)P4_H2G(tnTableBase)')
    body = body.replace('((TnTable *)(idx + P4_H2G(tnTableBase)))',
                        '((TnTable *)P4_G2H(idx + P4_H2G(tnTableBase)))')
    parts.append(body)
wrappers = unit('src/Graphics/Model/mdlEffect_inferred_004b11d0.c')
parts.extend(function(wrappers, name) for name in (
    'mdlEffectFlushWithTable', 'mdlEffectAdvanceAndFlushWithTable'))

# The unchanged consumers pass a table address to the effect subsystem. Execute
# each actual table argument (including address-of-array forms), without running
# unrelated scene/resource/game state machines. Each named owner must contribute.
queue_arguments = []
for path, owner in (
    ('src/cutinDraw.c', 'cutinDrawTaskUpdate'),
    ('src/cmmRankUp.c', 'cmmRankUpRender'),
    ('src/cmmRankUp.c', 'cmmRankUpSequenceUpdate'),
    ('src/unplaced/unplaced_inferred_00383c00.c', 'btlShuffleUpdateRewardEffects')):
    found = 0
    for line in function(unit(path), owner).splitlines():
        if 'D_005DC974' not in line or line.strip().startswith('extern '):
            continue
        call = re.search(r'\b(?:shdEffectTaskCreate|shdEffectCreateBlurTask|mdlEffectFlushWithTable|mdlEffectAdvanceAndFlushWithTable)\(', line)
        if not call:
            raise SystemExit('Unclassified queue read in ' + owner)
        arguments, start, depth = [], call.end(), 0
        for pos in range(start, len(line)):
            char = line[pos]
            if char == '(':
                depth += 1
            elif char == ')':
                if not depth:
                    arguments.append(line[start:pos].strip())
                    break
                depth -= 1
            elif char == ',' and not depth:
                arguments.append(line[start:pos].strip())
                start = pos + 1
        selected = [arg for arg in arguments if 'D_005DC974' in arg]
        if len(selected) != 1:
            raise SystemExit('Ambiguous queue argument in ' + owner)
        queue_arguments.append('    queue_context_reference((const void *)(' + selected[0] + '));')
        found += 1
    if not found:
        raise SystemExit('Missing queue argument witness: ' + owner)
parts.append('void witness_queue_contexts(void)\n{\n' + '\n'.join(queue_arguments) + '\n}')

filters = unit('src/evtMisc.c')
parts.extend(structure(filters, name) for name in ('EvtBlurFilterRecord', 'EvtBlurFilterExtra'))
parts.append('typedef struct { u32 words[4]; } EvtU128;')
parts.append('#define D_0063C660 ((u8 *)P4_G2H(0x0063c660u))')
filter_set = function(filters, 'evtCreateBlurFilterTaskSet')
start = filter_set.index('        if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {')
end = filter_set.index('            source = (u32 *)typeThreeExtra;', start)
end = filter_set.rfind('        {', start, end)
parts.append('void witness_filter3_extra(u8 out[44], u8 *record)\n{\n'
             '    u8 typeThreeExtra[44] __attribute__((aligned(16)));\n' +
             filter_set[start:end] + '\n    memcpy(out, typeThreeExtra, 44);\n}')

# These are the real two shared scalar-load expressions, not a new Golden
# sampler or debug-task implementation. Preserve each containing operation.
sampler = function(unit('src/Scene/mt_sceneFunc_inferred_0026cef0.c'), 'MT_Scene_SamplePath')
sampler_line = next(line.strip() for line in sampler.splitlines() if 'tangentTime - fGpffff842c' in line)
sampler_args = re.search(r'MT_Scene_EvalCubicBezier\(([^,]+),', sampler_line)[1]
parts.append('#define fGpffff842c (*(f32 *)P4_G2H(0x0076151cu))')
parts.append('f32 witness_path_back_time(f32 tangentTime)\n{\n    return ' + sampler_args + ';\n}')
test_menu = function(unit('src/Kosaka/k_testMenu.c'), 'K_TestMenu_Update')
rain_call = re.search(r'FldRainTaskCreate\((.*?)\);', test_menu, re.S)[1]
rain_args = rain_call.split(',')
if rain_args[4].strip() != 'D_0076151C':
    raise SystemExit('Shared test-menu scalar argument changed')
parts.append('#define D_0076151C (*(f32 *)P4_G2H(0x0076151cu))')
parts.append('f32 witness_test_menu_epsilon(void)\n{\n    return ' + rain_args[4].strip() + ';\n}')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text('/* Actual pinned consumer read sites; generated at build time. */\n' + '\n\n'.join(parts) + '\n', encoding='utf-8')
