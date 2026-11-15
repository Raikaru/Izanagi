#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS game proof of the bundled p4pc.qol features.

Each case loads a private save seed through the title (walk script), then drives the
real native Config/MODS hub and Camp menus with P4PC_MENU_SCRIPT, capturing PNGs of
what the player sees. Settings are either toggled through the hub's native rows or
pre-stored in mod-settings/p4pc.qol.ini beside the case's p4pc.ini. Outputs are
non-reference mod-witness artifacts; this is not a retail comparison.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness

WALKS = ROOT / "tests/walks"
# F1 opens the native Config in the field; MODS lists the one installed mod.
CONFIG_TO_PACKAGE = ("settings-open,wait240,settings-check:open,settings-row:MODS,cross,wait90,"
                     "settings-check:active-page:mods,settings-key:Home,cross,wait90,"
                     "settings-check:active-page:mods/installation,settings-key:Home,wait30")
# Castle 1F: a still frame, R1 held for 40 frames, the turned frame; then the
# field Config opens (the harness's proof the game still takes input).
CAMERA_HOLD = ("walkdone,wait60,shot:camera_before,settings-hold:r1:40,wait30,shot:camera_after,"
               "settings-open,wait240,settings-check:open")
# Leaves Config (opened in the field it returns through the Camp menu).
UP, DOWN = "up,", "down,"
CLOSE_CONFIG = "settings-key:Escape,wait120,circle,wait120,shot:closed"


def case(seed, walk, script, settings=None, expect=(), shots=(), stored=None, gate="40000", saved=(),
         meter=None, env=None, absent=(), timed=None):
    return {"seed": seed, "walk": walk, "script": script, "settings": settings or {},
            "expect": expect, "shots": shots, "stored": stored or {}, "gate": gate, "saved": saved,
            "meter": meter, "env": env or {}, "absent": absent, "timed": timed}


# Frames a walk command took, by case (the walk log's frame of that command and
# of the next one), for comparisons between cases run together.
TIMINGS = {}
WALK_LINE = re.compile(r"walk script \d+/\d+: (\S+) \(frame (\d+)\)")


def walk_frames(text, command):
    lines = [(m.group(1), int(m.group(2))) for m in WALK_LINE.finditer(text)]
    for i, (name, frame) in enumerate(lines[:-1]):
        if name == command:
            return lines[i + 1][1] - frame
    return None


METER = re.compile(r"p4snd: meter music=([\d.]+) effects=([\d.]+) voice=([\d.]+) movie=([\d.]+)")
METER_NAMES = ("music", "effects", "voice", "movie")


def check_meter(name, text, meter):
    """The mixer's per-second category levels (P4PC_AUDIO_METER) after the mod set
    its volumes: silent categories stay at zero, audible ones are heard."""
    start = text.find("QOL_AUDIO ") if meter.get("after_mod") else 0
    assert start >= 0, f"{name}: the mod never set its volumes"
    levels = [dict(zip(METER_NAMES, map(float, m.groups()))) for m in METER.finditer(text, start)]
    assert levels, f"{name}: no meter lines"
    for category in meter.get("silent", ()):
        assert all(level[category] == 0.0 for level in levels), f"{name}: {category} was heard"
    for category in meter.get("audible", ()):
        assert any(level[category] > 0.001 for level in levels), f"{name}: {category} was never heard"
    return {category: max(level[category] for level in levels) for category in METER_NAMES}


# Package page rows: identity, status, enabled, order, details, the UI heading,
# then the declared settings in manifest order, then the Lua pages.
CASES = {
    # The hub's native FULL DESCRIPTIONS row turns the feature on and stores it;
    # the mod follows when the menus close (Config to Camp to the field), so the
    # field runs on until the text is generated.
    "hub": case(
        "story0420", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},shot:package_off,down,down,down,down,down,down,wait30,cross,wait60,"
        f"shot:package_on,wait300,{CLOSE_CONFIG},circle,wait120,circle,wait120,shot:field,wait900",
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("package_off", "package_on", "field"), stored={"full_descriptions": "on"}),
    # With the stored toggle on, the Camp skill list's help box shows the
    # generated Simple text.
    "descriptions": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,cross,wait90,cross,wait120,square,wait60,shot:skill_help,"
        "down,wait60,shot:skill_help2,down,wait60,shot:skill_help3,down,wait60,shot:skill_help4",
        settings={"full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("skill_help", "skill_help2", "skill_help3", "skill_help4")),
    # The stored Exact style is applied at startup: Zio shows its raw power.
    "descriptions_exact": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,cross,wait90,cross,wait120,square,wait60,down,wait60,down,wait60,"
        "shot:skill_exact",
        settings={"full_descriptions": "on", "description_style": "Exact"},
        expect=(r"QOL_DESCRIPTIONS style=Exact generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("skill_exact",)),
    # Items and equipment: the Camp Item and Equip menus' help boxes.
    "descriptions_items": case(
        "story0420", "qol_load_free.walk",
        "walkdone,wait300,open,wait60,shot:camp_root,down,wait60,shot:camp_item_row,cross,wait120,shot:items,"
        "square,wait60,shot:item_help,circle,wait90,circle,wait90,shot:camp_back,down,wait60,cross,wait120,"
        "shot:equip_members,cross,wait120,shot:equip_slots,cross,wait120,shot:equip_list,square,wait60,shot:equip_help",
        settings={"full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+) kept=(\d+) reference=100 tables=5",),
        shots=("item_help", "equip_help")),
    # The read-only pages, each from its link at the end of the package page
    # (after the DIALOGUE BACKLOG link: SKILL A VS B, SKILL COMPARE, REGISTERED
    # VS CARRIED, COMPENDIUM FILTER, FUSION RESULTS, FUSION PLAN, FUSION
    # PLANNER, FORECAST, GLOSSARY ENTRY, GLOSSARY).
    "pages": case(
        "playthrough0606postkanji", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,{UP * 8}wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/skills,shot:skills,down,wait30,cross,wait30,down,wait30,"
        "down,wait30,cross,wait30,shot:skills_picked,settings-key:End,wait30,cross,wait90,shot:skills_ab,"
        f"circle,wait90,circle,wait90,settings-key:End,wait30,{UP * 6}wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/compendium,shot:compendium,right,wait60,shot:compendium_arcana,"
        "down,down,down,down,wait30,cross,wait90,shot:compendium_compare,circle,wait90,circle,wait90,"
        f"settings-key:End,wait30,{UP * 5}wait30,cross,wait90,settings-check:active-page:lua/p4pc.qol/fusion,"
        "shot:fusion,down,wait30,shot:fusion_first,circle,wait90,"
        f"settings-key:End,wait30,{UP * 3}wait30,cross,wait90,settings-check:active-page:lua/p4pc.qol/planner,"
        f"shot:planner,{DOWN * 12}wait30,cross,wait600,shot:planner_plan,circle,wait90,circle,wait90,"
        f"settings-key:End,wait30,{UP * 2}wait30,cross,wait90,settings-check:active-page:lua/p4pc.qol/forecast,"
        "shot:forecast,circle,wait90,"
        "settings-key:End,wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/glossary,shot:glossary,down,down,wait30,cross,wait90,"
        f"shot:glossary_entry,circle,wait90,circle,wait90,{CLOSE_CONFIG}",
        settings={"skill_compare": "on", "compendium_filter": "on", "fusion_results": "on",
                  "fusion_planner": "on", "forecast": "on", "glossary": "on"},
        expect=(r"QOL_FUSIONS roster=(\d+) combinations=(\d+) results=([1-9]\d*)",
                r"QOL_PLAN target=(\d+) candidates=(\d+) recipes=(\d+)",
                r"QOL_FORECAST today=66 (Mon 06/06: .*)"),
        shots=("skills", "skills_ab", "compendium", "compendium_compare", "fusion", "planner_plan",
               "forecast", "glossary_entry")),
    # Teddie's 04/18 tutorial battle: the hook tests the real target's affinity
    # against the party's knowledge (Zio on a known Elec weakness: a weakness
    # has no Third Eye marker, so nothing is marked) and reports per battle.
    "hints": case(
        "story0418tv", "save0418tv_to_castle_3f.walk", "",
        settings={"affinity_hints": "on"},
        expect=(r"QOL_HINTS battle_end outcome=1 evaluated=([1-9]\d*) marked=(\d+)",), gate="60000"),
    # Castle 1F's free camera: hold R1 for 40 frames. With CAMERA SPEED at 150%
    # every held turn the game applies is half again as large (camera_on); the
    # same hold with the feature off turns at the game's own speed (camera_off).
    "camera_on": case(
        "story0418tv", "qol_camera_castle.walk",
        CAMERA_HOLD,
        settings={"camera_speed": "on", "camera_speed_percent": "150%"},
        expect=(r"QOL_CAMERA percent=150 turns=30 game_degrees=(\d+) applied_degrees=(\d+)",),
        shots=("camera_before", "camera_after"), gate="30000"),
    "camera_off": case(
        "story0418tv", "qol_camera_castle.walk",
        CAMERA_HOLD,
        shots=("camera_before", "camera_after"), gate="30000"),
    # Teddie's battle tutorial has music, effects and the navigator's voice. With
    # MUSIC VOLUME at 0% the mixer's music level stays at zero once the mod has
    # set its volumes while effects and voices are heard (audio_on); with the
    # feature off the same route's music is heard, and no stream is tagged, so
    # every ADX stream (voices too) counts as music (audio_off).
    "audio_on": case(
        "story0418tv", "save0418tv_to_castle_3f.walk", "",
        settings={"audio_volumes": "on", "music_volume": "0%"},
        expect=(r"QOL_AUDIO music=0 effects=100 voice=100 movie=100",), gate="60000",
        meter={"after_mod": True, "silent": ("music",), "audible": ("effects", "voice")}),
    "audio_off": case(
        "story0418tv", "save0418tv_to_castle_3f.walk", "", gate="60000",
        meter={"silent": ("voice",), "audible": ("music", "effects")}),
    # 04/18 morning: the walk's only input after the conversation starts is one
    # START press while Yosuke's line waits. With FAST TEXT on, the rows the
    # game would hold for a button advance by themselves, and the latch lets go
    # at Chie's first choice, which stays unanswered (the walk then ends there).
    "fast_text": case(
        "story0417", "qol_fast_text_0418.walk", "",
        settings={"fast_text": "on"},
        expect=(r"QOL_FAST_TEXT latch=on", r"QOL_TEXT advanced=3 latched=true",
                r"QOL_FAST_TEXT latch=off choice"),
        gate="6000"),
    # The same conversation with AUTO DIALOGUE at 1s and no input at all: the
    # three lines that wait for a button move on by themselves after the delay
    # (the classroom's looping ambience is not a voice line), up to the choice.
    "auto_dialogue": case(
        "story0417", "qol_auto_dialogue_0418.walk", "",
        settings={"auto_dialogue": "on", "auto_dialogue_delay": "1s"},
        expect=(r"QOL_TEXT advanced=3 latched=false auto=true",), gate="6000"),
    # The opening movie (index 21, sfd/P4CTOP1.sfd) before the title. Watched to
    # its end with SKIP SEEN MOVIES on, it is recorded in the mod's settings
    # (movie_seen; a stored string reads "s:<text>" in the file); recorded, the
    # next boot skips it the way START does and the
    # title and load go on (movie_skip); skipped with START by the title driver,
    # it is never recorded (movie_pressed).
    "movie_seen": case(
        "story0420", "qol_load_free.walk", "",
        settings={"skip_seen_movies": "on"}, env={"P4PC_TITLE_WATCH_OPENING": "1"},
        expect=(r"QOL_MOVIE seen index=21 path=sfd/P4CTOP1\.sfd frames=(\d+) stored=true",),
        stored={"movie.21": "s:sfd/P4CTOP1.sfd"}),
    "movie_skip": case(
        "story0420", "qol_load_free.walk", "",
        settings={"skip_seen_movies": "on", "movie.21": "s:sfd/P4CTOP1.sfd"},
        expect=(r"QOL_MOVIE skip index=21 path=sfd/P4CTOP1\.sfd",), absent=(r"QOL_MOVIE seen",)),
    "movie_pressed": case(
        "story0420", "qol_load_free.walk", "",
        settings={"skip_seen_movies": "on"}, absent=(r"QOL_MOVIE",)),
    # Bathhouse 2F (story0520bathhouse2f): all four members are hurt. The PARTY
    # HEAL page (the first page link) previews the casts; HEAL THE PARTY queues
    # them, and closing Config and the Camp menu casts them through the game's
    # own skill routines, cheapest SP first.
    "heal": case(
        "story0520bathhouse2f", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,{UP * 11}wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/heal,shot:heal_plan,settings-key:End,wait30,up,wait30,cross,"
        f"wait60,shot:heal_queued,{CLOSE_CONFIG},circle,wait120,circle,wait120,wait300",
        settings={"party_heal": "on"},
        expect=(r"QOL_HEAL cast=\S+ caster=\d+ target=\S+ sp=(\d+) healed=([1-9]\d*)",
                r"QOL_HEAL done casts=([1-9]\d*) sp=(\d+) healed=([1-9]\d*)"),
        shots=("heal_plan", "heal_queued"), gate="40000"),
    # Teddie's tutorial battle (story0418tv). The leader's first turn presses
    # R1 (WEAKNESS ASSIST): Skill opens with the cursor on Zio, the target
    # choice starts on the weak, already analyzed Hablerie (unit 17), and the
    # battle driver's two confirms cast it (battle_assist). In the same battle,
    # Square on the first turn (DIRECT COMMANDS) is the game's own Guard
    # (battle_direct).
    "battle_assist": case(
        "story0418tv", "qol_battle_castle.walk", "",
        settings={"weakness_assist": "on"},
        expect=(r"QOL_ASSIST skill=Zio unit=1", r"QOL_ASSIST point=(\d+) skill=Zio aimed=true pointed=true",
                r"QOL_ASSIST targeting skill=Zio target=17"),
        gate="60000"),
    "battle_direct": case(
        "story0418tv", "qol_battle_direct_castle.walk", "",
        settings={"direct_commands": "on"},
        expect=(r"QOL_COMMAND guard unit=1 ok=true", r"QOL_COMMAND action kind=7"), gate="60000"),
    # The same tutorial battle with HALF SP COST and DOUBLE BATTLE MONEY: the
    # leader's Zio (4 SP) costs 2, and the victory's money award is doubled
    # once, at the award commit.
    "assistance": case(
        "story0418tv", "qol_battle_castle.walk", "",
        settings={"weakness_assist": "on", "half_sp": "on", "double_money": "on"},
        expect=(r"QOL_SP skill=Zio cost=4 half=2", r"QOL_MONEY award=(\d+) doubled=(\d+) before=(\d+)",
                r"QOL_MONEY committed after=(\d+)"),
        gate="60000"),
    # Castle 1F to its upstairs with battles skipped: with DUNGEON DASH on
    # (Always) the player's run is 1.25x and the walk to the goal takes clearly
    # fewer frames than with it off (compared when both run).
    "dash_on": case(
        "story0418tv", "qol_dash_castle.walk", "", settings={"dungeon_dash": "on", "dash_mode": "Always"},
        expect=(r"QOL_DASH speed=(\d+) dashed=(\d+)",), gate="40000", timed="walk:goal"),
    "dash_off": case("story0418tv", "qol_dash_castle.walk", "", gate="40000", timed="walk:goal"),
    # At the Velvet Room door (playthrough0520andra_registered): plan Apsaras
    # (the 13th row) on FUSION PLANNER, queue the first recipe on PLAN SUMMONS that
    # needs a registered Persona, close Config and enter. At the Velvet Room's
    # menu the Persona is summoned at the compendium's own price.
    "summons": case(
        "playthrough0520andra_registered", "qol_velvet_0520.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,{UP * 3}wait30,cross,wait90,"
        f"{DOWN * 12}wait30,cross,wait300,shot:plan,circle,wait90,circle,wait90,settings-key:End,wait30,"
        f"{UP * 12}wait30,cross,wait90,settings-check:active-page:lua/p4pc.qol/summons,shot:summons,"
        f"down,wait30,down,wait30,cross,wait60,shot:summons_queued,{CLOSE_CONFIG},circle,wait120,circle,wait120,circle,wait120,"
        "circle,wait300,shot:door,cross,wait900,circle,wait100,circle,wait100,circle,wait600,shot:velvet",
        settings={"fusion_planner": "on", "plan_summons": "on"},
        expect=(r"QOL_SUMMON id=(\d+) name=(.+) price=(\d+) money=(\d+)->(\d+)",),
        shots=("plan", "summons", "summons_queued", "door", "velvet"), gate="60000"),
    # Teddie's tutorial battle with DAMAGE DEALT 2x, DAMAGE TAKEN 0.5x and
    # BATTLE EXP 2x: the final damage stage (p4.damage, API 1.6) scales the
    # party's hits and the hits it takes, and each Persona's base award doubles.
    "damage_exp": case(
        "story0418tv", "save0418tv_to_castle_3f.walk", "",
        settings={"damage_dealt": "on", "damage_dealt_factor": "2x", "damage_taken": "on",
                  "damage_taken_factor": "0.5x", "battle_exp": "on", "battle_exp_factor": "2x"},
        expect=(r"QOL_DAMAGE dealt hits=([1-9]\d*) first_stock=(\d+) factor=2 ",
                r"QOL_EXP level=(\d+) award=(\d+) scaled=(\d+)"),
        gate="60000"),
    # March with every member joined: after one won Bathhouse battle each
    # member outside the battle party earns the party formula's EXP through the
    # game's own EXP and level-up routines.
    "reserve_exp": case(
        "playthrough0320tv", "qol_reserve_0320.walk", "",
        settings={"reserve_exp": "on"},
        expect=(r"QOL_RESERVE id=(\d+) level=(\d+) award=([1-9]\d*) exp=(\d+)->(\d+) leveled=(\w+) now=(\d+)",),
        gate="60000"),
    # March (playthrough0320tv): Yosuke's Susano-o has eight skills and has
    # forgotten Garu (its learnset's level-0 skill). RELEARN SKILLS picks it,
    # replacing the first slot, and the game's own remove/add routines make the
    # change when the menus close.
    "relearn": case(
        "playthrough0320tv", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,{UP * 13}wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/relearn,shot:relearn,down,wait30,down,wait30,down,wait30,cross,"
        f"wait30,settings-key:End,wait30,cross,wait60,shot:relearn_queued,{CLOSE_CONFIG},circle,wait120,circle,"
        "wait120,wait300",
        settings={"relearn_skills": "on"},
        expect=(r"QOL_RELEARN member=2 skill=14 replaced=16 ok=true skills=(\d+(?:,\d+){7})",),
        shots=("relearn", "relearn_queued"), gate="40000"),
    # story0422personas: summon Izanagi and fuse it with Apsaras into Omoikane
    # (save0422personas_to_omoikane.walk). With CHOSEN INHERITANCE on and a
    # skills marked (the stored data key, as the page writes it), marked
    # skills in each roll's pool replace its random picks, and the fused
    # Persona (172, from 14 and 1) joins with marked skill 222 or 121 where
    # the game's roll picked skill 192.
    "inherit": case(
        "story0422personas", "save0422personas_to_omoikane.walk", "",
        settings={"chosen_inheritance": "on", "inherit.wanted": "s:222,121,224,157,213,14,83"},
        expect=(r"QOL_INHERIT persona=(\d+) from=(\S+) pool=(\S+) picked=([1-9]) marked=([1-9]) swapped=(\d+>\d+\S*)",
                r"QOL_INHERIT_FUSED persona=172 marked=1 of 1 skills=39,213,211,(222|121),0,0,0,0"),
        gate="80000"),
    # The INHERITANCE page lists the carried Personas' skills; marking the
    # first one stores it (the inherit.wanted data key) when the menus close.
    "inherit_page": case(
        "story0422personas", "qol_load_free.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,{UP * 14}wait30,cross,wait90,"
        "settings-check:active-page:lua/p4pc.qol/inherit,shot:inherit,down,wait30,down,wait30,cross,wait30,"
        f"shot:inherit_marked,{CLOSE_CONFIG},circle,wait120,circle,wait120,wait300",
        settings={"chosen_inheritance": "on"},
        expect=(r"QOL_INHERIT_SAVED marks=(\d+) ok=true",),
        shots=("inherit", "inherit_marked"), gate="40000"),
    # A save written while features ran carries the mod's provenance block:
    # the 04/18 route saves at the classroom butterfly after the backlog and
    # the descriptions have both been used.
    "save_tag": case(
        "story0417", "save0417_to_school_0418.walk", "",
        settings={"backlog": "on", "full_descriptions": "on"},
        expect=(r"QOL_DESCRIPTIONS style=Simple generated=(\d+)", r"SDKMEMCARD_SAVE OK"),
        saved=(b"p4pc.qol", b"1.0.0", b"lua.value", b"backlog", b"features", b"Simple"), gate="60000"),
    # The backlog keeps the conversations of the 04/18 morning and lists them
    # on the package page's DIALOGUE BACKLOG detail.
    "backlog": case(
        "story0417", "qol_backlog_0418.walk",
        f"walkdone,{CONFIG_TO_PACKAGE},settings-key:End,wait30,shot:links,{UP * 10}wait30,cross,wait90,"
        f"shot:backlog,r1,wait30,shot:backlog_page2,{CLOSE_CONFIG}",
        settings={"backlog": "on"},
        expect=(r"QOL_BACKLOG entries=([1-9]\d*)",),
        shots=("links", "backlog", "backlog_page2"), gate="60000"),
}


def run_case(name, spec, args, work, iso, elf, saves):
    root = work / name
    card = root / "card" / "mc0"
    shutil.copytree(saves / spec["seed"], card)
    mods = root / "mods"
    shutil.copytree(ROOT / "mods/p4pc.qol", mods / "p4pc.qol")
    config = root / "p4pc.ini"
    config.write_text("[mods]\nenabled=p4pc.qol\n", encoding="ascii")
    if spec["settings"]:
        (root / "mod-settings").mkdir()
        (root / "mod-settings/p4pc.qol.ini").write_text(
            "".join(f"{key} = {value}\n" for key, value in spec["settings"].items()), encoding="ascii")
    env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods), P4PC_MODS="1", P4PC_PACKS="0",
               P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso), P4PC_SAVE_DIR=str(card.parent),
               P4PC_SCENE_ARTIFACTS=str(root), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
               SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2", P4PC_VIRTUAL_CLOCK="1",
               P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0", P4PC_SETTINGS_TEST="1",
               P4PC_WALK_SCRIPT=str(WALKS / spec["walk"]), P4PC_NEWGAME_GATE=spec["gate"])
    if spec["script"]:
        env.update(P4PC_MENU_SCRIPT=spec["script"], P4PC_MENU_EXIT="1")
    env["P4PC_AUDIO_METER"] = "1" if spec["meter"] else "0"
    env["P4PC_TITLE_WATCH_OPENING"] = "0"
    env.update(spec["env"])
    if os.name != "nt" and not any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
        env["SDL_VIDEODRIVER"] = "offscreen"
    for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_START_MENU", "P4PC_MOD_WITNESS_OUT"):
        env.pop(key, None)
    log = root / "game.log"
    with log.open("wb") as output:
        result = subprocess.run([*args.driver, str(elf)], cwd=root, env=env, stdout=output,
                                stderr=subprocess.STDOUT, timeout=3000, check=False,
                                creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0, f"{name} exited {result.returncode}; see {log}"
    assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
    assert "QOL_READY version=" in text, f"{name}: the mod did not start"
    assert "walk script done" in text, f"{name}: the walk did not finish"
    if spec["script"]:
        assert re.search(r"script finished: .* PASS", text), f"{name}: the menu script failed"
    found = {}
    for pattern in spec["expect"]:
        match = re.search(pattern, text)
        assert match, f"{name}: missing {pattern}"
        found[pattern] = match.groups()
    if spec["timed"]:
        frames = walk_frames(text, spec["timed"])
        assert frames is not None, f"{name}: the walk has no timed {spec['timed']}"
        TIMINGS[name] = frames
        found["frames"] = frames
    for pattern in spec["absent"]:
        assert not re.search(pattern, text), f"{name}: unexpected {pattern}"
    if spec["meter"]:
        found["meter peaks"] = check_meter(name, text, spec["meter"])
    for shot in spec["shots"]:
        assert (root / f"menu_{shot}.png").is_file(), f"{name}: no capture {shot}"
    if spec["saved"]:
        sidecar = card / "BASLUS-21782Save0" / ".p4mods"
        assert sidecar.is_file(), f"{name}: the save has no mod sidecar"
        data = sidecar.read_bytes()
        for token in spec["saved"]:
            assert token in data, f"{name}: the save sidecar lacks {token!r}"
        (root / "save.p4mods-copy").write_bytes(data)
    stored = root / "mod-settings/p4pc.qol.ini"
    for key, value in spec["stored"].items():
        assert stored.is_file() and re.search(rf"^{re.escape(key)} = {re.escape(value)}$",
                                              stored.read_text(encoding="ascii"), re.M), \
            f"{name}: {key} was not stored as {value} by the hub"
    print(f"mod_qol_game: case {name} PASS {found}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--case", action="append", choices=sorted(CASES))
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("QoL proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("QoL proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    saves = Path(os.environ.get("P4PC_QOL_SAVES", str(elf.parent / "saves")))
    selected = args.case or sorted(CASES)
    if not elf.is_file() or not iso.is_file() or any(not (saves / CASES[n]["seed"]).is_dir() for n in selected):
        print("mod_qol_game: retail ELF/ISO or private save seeds unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-qol-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        for name in selected:
            run_case(name, CASES[name], args, work, iso.resolve(), elf.resolve(), saves)
        if "dash_on" in TIMINGS and "dash_off" in TIMINGS:
            on, off = TIMINGS["dash_on"], TIMINGS["dash_off"]
            assert on * 10 < off * 9, f"dash: {on} frames dashing is not clearly fewer than {off}"
            print(f"mod_qol_game: dash walk:goal {on} frames dashing, {off} without")
        outcome = "pass"
        print(f"mod_qol_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
