#!/usr/bin/env python3
"""Admitted, deterministic native battle batches. Never resolves combat in Python."""
import argparse
import contextlib
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

SCHEMA = 1
STREAMS = {"additive": 1, "mt": 2, "policy": 3, "effect": 4}
OUTCOMES = {"win", "defeat", "escape", "scripted_conclusion", "action_limit",
            "update_limit", "engine_error", "policy_error"}
CENSORED = {"action_limit", "update_limit"}
ERRORS = {"engine_error", "policy_error"}
POLICIES = {"tape": 0, "greedy": 1, "cautious": 2, "typical": 3}
# p4_game_table_catalog_identity's retail value; expanded catalogs hash their content.
RETAIL_TABLE_CATALOG = hashlib.sha256(b"p4pc-table-catalog-v1:retail").hexdigest()


class ScenarioError(ValueError):
    pass


def assert_fidelity_environment(extra=None):
    # Refuse before sanitization: dropping the marker must not promote an
    # ordinary-game mod-test exception into simulator/fidelity evidence.
    for values in (os.environ, extra or {}):
        if any(key.upper() == "P4PC_MOD_WITNESS" for key in values):
            raise ScenarioError("P4PC_MOD_WITNESS is forbidden in battle simulator/fidelity runs, including an empty value")


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_digest(path):
    result = hashlib.sha256()
    with Path(path).open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def read_image(path, location):
    path = Path(path)
    maximum = 8 * 1024 * 1024
    try:
        size = path.stat().st_size
        if not 4 <= size <= maximum:
            raise ScenarioError(location + ": expected bounded native save image including version")
        with path.open("rb") as source:
            image = source.read(maximum + 1)
    except OSError as exc:
        raise ScenarioError(location + ": " + str(exc)) from exc
    if len(image) != size:
        raise ScenarioError(location + ": image changed size while being read")
    return image


def atomic_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_bytes(canonical(value) + b"\n")
    os.replace(temporary, path)


def load_json(path):
    try:
        return json.loads(Path(path).read_text(encoding="utf-8"),
                          parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))
    except (OSError, ValueError) as exc:
        raise ScenarioError("%s: %s" % (path, exc)) from exc


def object_fields(value, location, required, optional=()):
    if not isinstance(value, dict):
        raise ScenarioError(location + ": expected object")
    missing = set(required) - value.keys()
    extra = value.keys() - set(required) - set(optional)
    if missing:
        raise ScenarioError(location + ": missing " + ", ".join(sorted(missing)))
    if extra:
        raise ScenarioError(location + ": unknown fields " + ", ".join(sorted(extra)))


def integer(value, location, minimum=0, maximum=0xFFFFFFFF):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ScenarioError("%s: expected integer in %d..%d" % (location, minimum, maximum))
    return value


def text(value, location):
    if not isinstance(value, str) or not value.strip():
        raise ScenarioError(location + ": expected nonempty string")
    return value


def choice(value, location, allowed):
    if not isinstance(value, str) or value not in allowed:
        raise ScenarioError(location + ": expected " + ", ".join(allowed))
    return value


def vector(value, location, length, maximum):
    if not isinstance(value, list) or len(value) != length:
        raise ScenarioError("%s: expected exactly %d integers" % (location, length))
    for index, number in enumerate(value):
        integer(number, "%s[%d]" % (location, index), maximum=maximum)


def preserve_word(value, location):
    return 0xFFFFFFFF if value == "preserve" else integer(value, location, maximum=0xFFFFFFFE)


def validate_reference(source, location):
    object_fields(source, location, ("kind", "id", "image", "sha256"), ("parent_id", "walk_sha256"))
    choice(source["kind"], location + ".kind", ("save", "walker"))
    text(source["id"], location + ".id")
    text(source["image"], location + ".image")
    if not isinstance(source["sha256"], str) or re.fullmatch(r"[0-9a-f]{64}", source["sha256"]) is None:
        raise ScenarioError(location + ".sha256: expected lowercase SHA-256 of frozen native image")
    if source["kind"] == "walker":
        text(source.get("parent_id"), location + ".parent_id")
        if re.fullmatch(r"[0-9a-f]{64}", str(source.get("walk_sha256", ""))) is None:
            raise ScenarioError(location + ".walk_sha256: expected lowercase SHA-256")


def validate_persona_values(values, location, *, level=True):
    allowed = ("level", "exp", "skills", "stats", "bonus_a", "bonus_b") if level else ("skills", "stats", "bonus_a", "bonus_b")
    object_fields(values, location, (), allowed)
    if ("level" in values) != ("exp" in values):
        raise ScenarioError(location + ": level and exp must be supplied together")
    if "level" in values:
        integer(values["level"], location + ".level", 1, 99)
        integer(values["exp"], location + ".exp")
    if "skills" in values:
        vector(values["skills"], location + ".skills", 8, 4095)
    if "stats" in values:
        vector(values["stats"], location + ".stats", 5, 255)
    if ("bonus_a" in values) != ("bonus_b" in values):
        raise ScenarioError(location + ": bonus_a and bonus_b must be supplied together")
    for key in ("bonus_a", "bonus_b"):
        if key in values:
            vector(values[key], location + "." + key, 5, 255)


def validate_persona(persona, location, classification):
    if not isinstance(persona, dict):
        raise ScenarioError(location + ": expected object")
    mode = choice(persona.get("mode"), location + ".mode", ("preserve_source", "synthetic_exact", "native_growth"))
    common = ("owner", "slot", "mode")
    if mode == "preserve_source":
        object_fields(persona, location, common + ("source_slot",), ("persona_id", "assertions"))
    elif mode == "synthetic_exact":
        object_fields(persona, location, common + ("persona_id", "record"))
    else:
        object_fields(persona, location, common + ("persona_id", "growth"), ("source_slot", "assertions", "decisions"))
    owner = integer(persona["owner"], location + ".owner", 1, 10)
    integer(persona["slot"], location + ".slot", maximum=11 if owner == 1 else 0)
    if "persona_id" in persona:
        # Wire schema only (the native record's 16-bit ID); the active catalog bounds it natively.
        integer(persona["persona_id"], location + ".persona_id", 1, 0xFFFF)
    if classification == "source_attested" and mode != "preserve_source":
        raise ScenarioError(location + ".mode: source_attested permits source preservation only; growth/exact overrides require synthetic_lab")
    if mode == "preserve_source":
        integer(persona["source_slot"], location + ".source_slot", maximum=11 if owner == 1 else 0)
        validate_persona_values(persona.get("assertions", {}), location + ".assertions")
    elif mode == "synthetic_exact":
        record = persona["record"]
        object_fields(record, location + ".record", ("level", "exp", "skills", "stats"), ("bonus_a", "bonus_b"))
        validate_persona_values(record, location + ".record")
        if any(record.get("bonus_a", []) + record.get("bonus_b", [])):
            raise ScenarioError(location + ".record: synthetic exact bonus arrays must be zero")
    else:
        growth = persona["growth"]
        object_fields(growth, location + ".growth", ("origin", "level", "exp"), ("starting_skills",))
        choice(growth["origin"], location + ".growth.origin", ("source_record", "native_init"))
        integer(growth["level"], location + ".growth.level", 1, 99)
        integer(growth["exp"], location + ".growth.exp")
        if growth["origin"] == "source_record":
            integer(persona.get("source_slot"), location + ".source_slot", maximum=11 if owner == 1 else 0)
        elif "source_slot" in persona:
            raise ScenarioError(location + ".source_slot: omit for native_init; the wire uses FFFFFFFF for every owner")
        if "starting_skills" in growth:
            vector(growth["starting_skills"], location + ".growth.starting_skills", 8, 4095)
        validate_persona_values(persona.get("assertions", {}), location + ".assertions", level=False)
        decisions = persona.get("decisions", [])
        if not isinstance(decisions, list) or len(decisions) > 4096:
            raise ScenarioError(location + ".decisions: expected at most 4096 ordered decisions")
        previous_level = 0
        for index, decision in enumerate(decisions):
            where = "%s.decisions[%d]" % (location, index)
            object_fields(decision, where, ("level", "learned_skill", "replacement_slot", "removed_skill"))
            reached = integer(decision["level"], where + ".level", 1, 99)
            if reached < previous_level:
                raise ScenarioError(where + ".level: decisions must follow native reached-level order")
            previous_level = reached
            integer(decision["learned_skill"], where + ".learned_skill", 1, 4095)
            integer(decision["removed_skill"], where + ".removed_skill", maximum=4095)
            if decision["replacement_slot"] != "auto":
                integer(decision["replacement_slot"], where + ".replacement_slot", maximum=7)


def validate_definition(definition):
    where = "entry.source.definition"
    object_fields(definition, where, ("schema", "baseline", "classification", "preparation", "context", "party", "inventory"))
    if type(definition["schema"]) is not int or definition["schema"] != 1:
        raise ScenarioError(where + ".schema: expected 1")
    validate_reference(definition["baseline"], where + ".baseline")
    classification = choice(definition["classification"], where + ".classification", ("source_attested", "synthetic_lab"))
    preparation = definition["preparation"]
    object_fields(preparation, where + ".preparation",
                  ("master_seed", "resource_mode", "clear_temporary_statuses", "bootstrap_limit", "field_resume_limit"),
                  ("timeout_seconds",))
    try:
        derive_seeds(preparation["master_seed"], 0)
    except ScenarioError as exc:
        raise ScenarioError(where + ".preparation.master_seed: " + str(exc)) from exc
    choice(preparation["resource_mode"], where + ".preparation.resource_mode", ("inherit", "fill_living_hp_sp"))
    if type(preparation["clear_temporary_statuses"]) is not bool:
        raise ScenarioError(where + ".preparation.clear_temporary_statuses: expected boolean")
    for key in ("bootstrap_limit", "field_resume_limit"):
        integer(preparation[key], where + ".preparation." + key, 1, 10000000)
    timeout = preparation.get("timeout_seconds", 120)
    if type(timeout) not in (float, int) or not math.isfinite(timeout) or timeout <= 0:
        raise ScenarioError(where + ".preparation.timeout_seconds: expected positive finite seconds")
    object_fields(definition["context"], where + ".context", (), ("date", "time_slot", "field", "room", "step"))
    for key, value in definition["context"].items():
        integer(value, where + ".context." + key)
    party = definition["party"]
    object_fields(party, where + ".party", ("hero", "roster_mode", "selected_slot", "actors", "personas"))
    hero = party["hero"]
    if not isinstance(hero, dict):
        raise ScenarioError(where + ".party.hero: expected object")
    hero_mode = choice(hero.get("mode"), where + ".party.hero.mode", ("preserve", "synthetic_exact", "assert_source"))
    object_fields(hero, where + ".party.hero", ("mode",) if hero_mode == "preserve" else ("mode", "level", "exp"))
    if hero_mode != "preserve":
        integer(hero["level"], where + ".party.hero.level", 1, 99)
        integer(hero["exp"], where + ".party.hero.exp")
    if classification == "source_attested" and hero_mode == "synthetic_exact":
        raise ScenarioError(where + ".party.hero.mode: synthetic override requires synthetic_lab")
    if party["roster_mode"] != "preserve":
        integer(party["roster_mode"], where + ".party.roster_mode", maximum=3)
    if party["selected_slot"] != "preserve":
        integer(party["selected_slot"], where + ".party.selected_slot", maximum=11)
    actors = party["actors"]
    if not isinstance(actors, list) or not 1 <= len(actors) <= 4:
        raise ScenarioError(where + ".party.actors: expected 1..4 actors, protagonist first")
    seen = set()
    for index, actor in enumerate(actors):
        location = "%s.party.actors[%d]" % (where, index)
        object_fields(actor, location, ("id", "equipment", "resources", "tactic"))
        identity = integer(actor["id"], location + ".id", 1)
        if identity not in (1, 2, 3, 4, 6, 7, 8):
            raise ScenarioError(location + ".id: expected native combatant 1,2,3,4,6,7,8; navigator 5 is not a combatant")
        if identity in seen or (index == 0 and identity != 1):
            raise ScenarioError(location + ".id: unique native IDs required, with protagonist 1 first")
        seen.add(identity)
        object_fields(actor["equipment"], location + ".equipment", ("weapon", "armor", "accessory"))
        for key, value in actor["equipment"].items():
            preserve_word(value, location + ".equipment." + key)
        resources = actor["resources"]
        if not isinstance(resources, dict):
            raise ScenarioError(location + ".resources: expected object")
        mode = choice(resources.get("mode"), location + ".resources.mode", ("preserve", "explicit"))
        object_fields(resources, location + ".resources", ("mode",) if mode == "preserve" else ("mode", "hp", "sp"))
        if mode == "explicit":
            if preparation["resource_mode"] == "fill_living_hp_sp":
                raise ScenarioError(location + ".resources: competing_resource_policies; global fill requires all actor resource modes preserve")
            integer(resources["hp"], location + ".resources.hp", maximum=999)
            integer(resources["sp"], location + ".resources.sp", maximum=999)
        if actor["tactic"] != "preserve":
            integer(actor["tactic"], location + ".tactic", maximum=4)
    personas = party["personas"]
    if not isinstance(personas, list) or len(personas) > 21:
        raise ScenarioError(where + ".party.personas: expected at most 21 records")
    slots = set()
    decisions = 0
    for index, persona in enumerate(personas):
        location = "%s.party.personas[%d]" % (where, index)
        validate_persona(persona, location, classification)
        identity = (persona["owner"], persona["slot"])
        if identity in slots:
            raise ScenarioError(location + ": duplicate owner/output roster slot")
        slots.add(identity)
        decisions += len(persona.get("decisions", []))
    hero_slots = [persona["slot"] for persona in personas if persona["owner"] == 1]
    if hero_slots != list(range(len(hero_slots))):
        raise ScenarioError(where + ".party.personas: declared complete hero roster must list contiguous slots 0..n-1 in order; it is not an overlay")
    if decisions > 4096:
        raise ScenarioError(where + ".party.personas: total skill decisions exceed 4096")
    inventory = definition["inventory"]
    if not isinstance(inventory, dict):
        raise ScenarioError(where + ".inventory: expected object")
    mode = choice(inventory.get("mode"), where + ".inventory.mode", ("preserve", "exact_battle_consumables"))
    object_fields(inventory, where + ".inventory", ("mode",) if mode == "preserve" else ("mode", "items"))
    items = inventory.get("items", [])
    if not isinstance(items, list) or len(items) > 1536:
        raise ScenarioError(where + ".inventory.items: expected at most 1536 records")
    seen = set()
    for index, item in enumerate(items):
        location = "%s.inventory.items[%d]" % (where, index)
        object_fields(item, location, ("id", "count"))
        identity = integer(item["id"], location + ".id")
        integer(item["count"], location + ".count", maximum=99)
        if identity in seen:
            raise ScenarioError(location + ".id: duplicate inventory ID")
        seen.add(identity)
    return definition


def validate_scenario(value):
    object_fields(value, "scenario", ("schema", "entry", "encounter_id", "policy", "master_seed", "seed_start", "n", "options"), ("label", "runtime"))
    if type(value["schema"]) is not int or value["schema"] != SCHEMA:
        raise ScenarioError("scenario.schema: expected 1")
    master = value["master_seed"]
    if not isinstance(master, str) or re.fullmatch(r"[0-9a-fA-F]{32}", master) is None:
        raise ScenarioError("scenario.master_seed: expected exactly 32 hexadecimal digits")
    integer(value["encounter_id"], "scenario.encounter_id", maximum=65535)
    start = integer(value["seed_start"], "scenario.seed_start", maximum=2**64 - 1)
    count = integer(value["n"], "scenario.n", 1)
    if start + count > 2**64:
        raise ScenarioError("scenario seed range exceeds LE64 run indices")
    if "label" in value:
        text(value["label"], "scenario.label")
    entry = value["entry"]
    object_fields(entry, "entry", ("source",), ("context", "rules"))
    source = entry["source"]
    if not isinstance(source, dict):
        raise ScenarioError("entry.source: expected object")
    kind = choice(source.get("kind"), "entry.source.kind", ("save", "walker", "declarative"))
    if kind == "declarative":
        object_fields(source, "entry.source", ("kind", "id", "definition"))
        text(source["id"], "entry.source.id")
        validate_definition(source["definition"])
    else:
        validate_reference(source, "entry.source")
    for key in ("context", "rules"):
        if key in entry and not isinstance(entry[key], dict):
            raise ScenarioError("entry.%s: expected object" % key)
    policy = value["policy"]
    object_fields(policy, "policy", ("kind", "version"), ("tape",))
    if not isinstance(policy["kind"], str) or policy["kind"] not in POLICIES:
        raise ScenarioError("policy.kind: expected tape, greedy, cautious or typical")
    if policy["version"] != 1 or type(policy["version"]) is not int:
        raise ScenarioError("policy.version: expected 1")
    if policy["kind"] == "tape" and "tape" not in policy:
        raise ScenarioError("policy.tape: required for tape policy")
    if policy["kind"] != "tape" and "tape" in policy:
        raise ScenarioError("policy.tape: only valid for tape policy")
    options = value["options"]
    object_fields(options, "options", ("update_limit", "bootstrap_limit", "action_limit", "latency_updates", "fill_resources"), ("timeout_seconds",))
    for key in ("update_limit", "bootstrap_limit", "action_limit"):
        integer(options[key], "options." + key, 1)
    integer(options["latency_updates"], "options.latency_updates")
    if type(options["fill_resources"]) is not bool:
        raise ScenarioError("options.fill_resources: expected boolean (no implicit preparation)")
    timeout = options.get("timeout_seconds", 120)
    if type(timeout) not in (float, int) or not math.isfinite(timeout) or timeout <= 0:
        raise ScenarioError("options.timeout_seconds: expected positive finite seconds")
    tape = policy.get("tape", [])
    if not isinstance(tape, list) or len(tape) > 100000:
        raise ScenarioError("policy.tape: expected at most 100000 records")
    end = 0
    for index, record in enumerate(tape):
        where = "policy.tape[%d]" % index
        object_fields(record, where, ("update", "buttons", "hold"))
        update = integer(record["update"], where + ".update")
        integer(record["buttons"], where + ".buttons", maximum=65535)
        hold = integer(record["hold"], where + ".hold", 1)
        if update < end or update + hold > options["update_limit"]:
            raise ScenarioError(where + ": overlapping, unsorted or exceeds update limit")
        end = update + hold
    runtime = value.get("runtime", {})
    object_fields(runtime, "runtime", (), ("iso", "elf", "video_driver"))
    for resource in ("iso", "elf"):
        if resource in runtime:
            where = "runtime." + resource
            object_fields(runtime[resource], where, ("path", "sha256"))
            text(runtime[resource]["path"], where + ".path")
            if not isinstance(runtime[resource]["sha256"], str) or re.fullmatch(r"[0-9a-f]{64}", runtime[resource]["sha256"]) is None:
                raise ScenarioError(where + ".sha256: expected lowercase SHA-256")
    if "video_driver" in runtime:
        text(runtime["video_driver"], "runtime.video_driver")
    # Also rejects non-JSON objects and nonfinite nested metadata.
    try:
        canonical(value)
    except (TypeError, ValueError) as exc:
        raise ScenarioError("scenario: invalid JSON value: " + str(exc)) from exc
    return value


def derive_seeds(master_seed, run_index):
    if not isinstance(master_seed, str) or re.fullmatch(r"[0-9a-fA-F]{32}", master_seed) is None:
        raise ScenarioError("master seed must be 32 hexadecimal digits")
    integer(run_index, "run_index", maximum=2**64 - 1)
    prefix = b"P4PC-BATTLE-SEEDS\0" + bytes.fromhex(master_seed) + struct.pack("<Q", run_index)
    return {name: struct.unpack("<I", hashlib.sha256(prefix + struct.pack("<I", tag)).digest()[:4])[0]
            for name, tag in STREAMS.items()}


def compile_preparation(definition, source_image):
    """Encode P4BPRE01; never interpret or modify native save records."""
    validate_definition(definition)
    if not 4 <= len(source_image) <= 8 * 1024 * 1024:
        raise ScenarioError("preparation source: expected bounded complete native save image")
    if digest(source_image) != definition["baseline"]["sha256"]:
        raise ScenarioError("preparation source: native image hash mismatch")
    party, preparation, inventory = definition["party"], definition["preparation"], definition["inventory"]
    actor_bytes, persona_bytes, decision_bytes = bytearray(), bytearray(), bytearray()
    decision_count = 0
    for actor in party["actors"]:
        equipment, resources = actor["equipment"], actor["resources"]
        actor_bytes.extend(struct.pack("<8I32x", actor["id"],
                                       *(preserve_word(equipment[key], "equipment." + key) for key in ("weapon", "armor", "accessory")),
                                       int(resources["mode"] == "explicit"), resources.get("hp", 0), resources.get("sp", 0),
                                       preserve_word(actor["tactic"], "tactic")))
    for persona in party["personas"]:
        mode = ("preserve_source", "synthetic_exact", "native_growth").index(persona["mode"])
        decisions = persona.get("decisions", [])
        first_decision = decision_count if decisions else 0
        for decision in decisions:
            replacement = 0xFFFFFFFF if decision["replacement_slot"] == "auto" else decision["replacement_slot"]
            decision_bytes.extend(struct.pack("<6I", persona["owner"], persona["slot"], decision["level"],
                                               decision["learned_skill"], replacement, decision["removed_skill"]))
        decision_count += len(decisions)
        if mode == 1:
            values, origin, source_slot = persona["record"], 1, 0xFFFFFFFF
            starting = [0] * 8
        elif mode == 2:
            growth = persona["growth"]
            values = {**persona.get("assertions", {}), "level": growth["level"], "exp": growth["exp"]}
            origin = int(growth["origin"] == "native_init")
            source_slot = 0xFFFFFFFF if origin else persona["source_slot"]
            starting = growth.get("starting_skills", [0] * 8)
        else:
            values, origin, source_slot = persona.get("assertions", {}), 0, persona["source_slot"]
            starting = [0] * 8
        mask = (1 if "level" in values else 0) | (2 if "skills" in values else 0) | (4 if "stats" in values else 0) | (8 if "bonus_a" in values else 0)
        if mode == 2 and "starting_skills" in persona["growth"]:
            mask |= 16
        record = bytearray(112)
        struct.pack_into("<8I", record, 0, persona["owner"], persona["slot"], persona.get("persona_id", 0xFFFFFFFF),
                         mode, source_slot, values.get("level", 0), values.get("exp", 0), origin)
        struct.pack_into("<8H", record, 32, *starting)
        struct.pack_into("<8H", record, 48, *values.get("skills", [0] * 8))
        record[64:69] = bytes(values.get("stats", [0] * 5))
        record[69:74] = bytes(values.get("bonus_a", [0] * 5))
        record[74:79] = bytes(values.get("bonus_b", [0] * 5))
        struct.pack_into("<3I", record, 80, first_decision, len(decisions), mask)
        persona_bytes.extend(record)
    items = inventory.get("items", [])
    item_bytes = b"".join(struct.pack("<2I", item["id"], item["count"]) for item in items)
    context_keys = ("date", "time_slot", "field", "room", "step")
    context = definition["context"]
    assertions = sum(1 << index for index, key in enumerate(context_keys) if key in context)
    hero = party["hero"]
    seeds = derive_seeds(preparation["master_seed"], 0)
    fields = [160, 0, len(source_image), len(party["actors"]), len(party["personas"]), len(items), decision_count,
              1 if definition["classification"] == "source_attested" else 2,
              int(preparation["resource_mode"] == "fill_living_hp_sp"), int(preparation["clear_temporary_statuses"]),
              seeds["additive"], seeds["mt"], seeds["effect"], preparation["bootstrap_limit"], preparation["field_resume_limit"],
              hero.get("level", 0), hero.get("exp", 0), preserve_word(party["roster_mode"], "roster_mode"),
              preserve_word(party["selected_slot"], "selected_slot"),
              ("preserve", "synthetic_exact", "assert_source").index(hero["mode"]), assertions,
              *(context.get(key, 0) for key in context_keys), int(inventory["mode"] == "exact_battle_consumables"), 0, 0, 0]
    header = struct.pack("<8s30I", b"P4BPRE01", *fields) + hashlib.sha256(canonical(definition)).digest()
    request = header + source_image + actor_bytes + persona_bytes + item_bytes + decision_bytes
    if len(request) > 8 * 1024 * 1024:
        raise ScenarioError("preparation request exceeds native 8 MiB limit")
    return request


def compile_entry(scenario, save_image, seeds, *, rendered=False):
    """Compile the native schema-1 envelope. Native owns save legality/checksums."""
    options, policy = scenario["options"], scenario["policy"]
    tape = policy.get("tape", [])
    flags = (0 if rendered else 1) | (4 if options["fill_resources"] else 0)
    header = struct.pack("<8s14I", b"P4BSIM01", 64, flags, scenario["encounter_id"],
                         seeds["additive"], seeds["mt"], options["update_limit"], len(tape),
                         len(save_image), 0, options["bootstrap_limit"], POLICIES[policy["kind"]],
                         options["latency_updates"], options["action_limit"], seeds["effect"])
    return header + save_image + b"".join(struct.pack("<III", row["update"], row["buttons"], row["hold"]) for row in tape)


def wilson(wins, count):
    if not count:
        return None
    z = 1.959963984540054
    p = wins / count
    denominator = 1 + z * z / count
    center = (p + z * z / (2 * count)) / denominator
    radius = z * math.sqrt(p * (1 - p) / count + z * z / (4 * count * count)) / denominator
    return [max(0.0, center - radius), min(1.0, center + radius)]


def distribution(values):
    values = sorted(values)
    if not values:
        return {"n": 0, "mean": None, "p10": None, "p50": None, "p90": None, "p95": None, "histogram": {}}
    def percentile(p):
        point = (len(values) - 1) * p
        low = math.floor(point)
        return values[low] + (values[math.ceil(point)] - values[low]) * (point - low)
    return {"n": len(values), "mean": sum(values) / len(values),
            **{"p%d" % p: percentile(p / 100) for p in (10, 50, 90, 95)},
            "histogram": dict(sorted(Counter(str(v) for v in values).items()))}


def validate_result(value):
    """Only accept actual native measurements; missing counters stay unavailable."""
    if not isinstance(value, dict) or not isinstance(value.get("outcome"), str) or value["outcome"] not in OUTCOMES:
        raise ScenarioError("worker result: missing or invalid outcome")
    if value.get("schema") != 1 or type(value.get("schema")) is not int:
        raise ScenarioError("worker result.schema: expected 1")
    for group in ("turns", "resources"):
        if group not in value:
            continue
        if not isinstance(value[group], dict):
            raise ScenarioError("worker result.%s: expected object" % group)
        for name, number in value[group].items():
            if name.endswith(("_definition", "_scope", "_boundary")):
                text(number, "worker result.%s.%s" % (group, name))
            elif group == "turns" and number is not None:
                integer(number, "worker result.turns." + name, maximum=2**64 - 1)
    for name, label in value.items():
        if name.endswith("_capability") or name == "entry_kind":
            text(label, "worker result." + name)
    if "observer_attached" in value:
        integer(value["observer_attached"], "worker result.observer_attached", maximum=1)
    if "table_catalog" in value:
        catalog = value["table_catalog"]
        where = "worker result.table_catalog"
        if catalog is None:
            if value["outcome"] not in ERRORS:
                raise ScenarioError(where + ": null identity is only valid on a native error result")
        else:
            object_fields(catalog, where, ("expanded", "identity"))
            expanded = integer(catalog["expanded"], where + ".expanded", maximum=1)
            if not isinstance(catalog["identity"], str) or re.fullmatch(r"[0-9a-f]{64}", catalog["identity"]) is None:
                raise ScenarioError(where + ".identity: expected lowercase SHA-256")
            if (catalog["identity"] == RETAIL_TABLE_CATALOG) != (expanded == 0):
                raise ScenarioError(where + ": retail identity and expanded flag disagree")
    if "party_policy" in value:
        policy = value["party_policy"]
        object_fields(policy, "worker result.party_policy", ("id", "name", "latency_updates", "party_control", "knowledge"))
        integer(policy["id"], "worker result.party_policy.id", maximum=3)
        integer(policy["latency_updates"], "worker result.party_policy.latency_updates")
        for key in ("name", "party_control", "knowledge"):
            text(policy[key], "worker result.party_policy." + key)
    if "entry_observations" in value:
        observations = value["entry_observations"]
        object_fields(observations, "worker result.entry_observations", ("first_protagonist_command",))
        command = observations["first_protagonist_command"]
        where = "worker result.entry_observations.first_protagonist_command"
        object_fields(command, where, ("boundary", "observed", "update", "party"))
        text(command["boundary"], where + ".boundary")
        integer(command["observed"], where + ".observed", maximum=1)
        if command["observed"]:
            integer(command["update"], where + ".update")
            if not isinstance(command["party"], list):
                raise ScenarioError(where + ".party: expected actual native party array")
        elif command["update"] is not None or command["party"] is not None:
            raise ScenarioError(where + ": absent observation requires null update/party, never fabricated zeros")
    canonical(value)
    return value


def resource_measurements(result):
    """Flatten reported resources, retaining per-actor HP/SP and item identities."""
    measurements = {}
    def visit(value, prefix):
        if type(value) in (int, float) and math.isfinite(value):
            measurements[prefix] = value
        elif isinstance(value, dict):
            for key, child in value.items():
                if key != "id":
                    visit(child, prefix + "." + key if prefix else key)
        elif isinstance(value, list):
            for index, child in enumerate(value):
                identity = child.get("id", index) if isinstance(child, dict) else index
                visit(child, prefix + "." + str(identity))
    visit(result.get("resources", {}), "")
    return measurements


def complete_native_item_ledger(result):
    """Native v1 emits every nonzero item decrement; only that ledger implies zeros."""
    resources = result.get("resources", {})
    if (type(result.get("schema")) is not int or result["schema"] != 1 or
            result.get("counter_capability") != "native-queue-ready-cost-target-v1" or
            type(result.get("observer_attached")) is not int or result["observer_attached"] != 1 or
            resources.get("spend_definition") != "native_party_skill_costs_and_actual_item_decrements"):
        return False
    items, total = resources.get("items_by_id"), resources.get("items_consumed")
    if not isinstance(items, list) or type(total) is not int or not 0 <= total <= 2**64 - 1:
        return False
    seen, counted = set(), 0
    for item in items:
        if (not isinstance(item, dict) or set(item) != {"id", "used"} or
                type(item["id"]) is not int or not 0 <= item["id"] < 1536 or item["id"] in seen or
                type(item["used"]) is not int or item["used"] < 0):
            return False
        seen.add(item["id"])
        counted += item["used"]
    # Missing/partial ledgers, including native out-of-domain decrements, are unknown.
    return counted == total


def final_party_hp(result):
    party = result.get("resources", {}).get("final_party")
    if not isinstance(party, list) or not party or any(
            not isinstance(actor, dict) or type(actor.get("hp")) is not int or actor["hp"] < 0 for actor in party):
        return None
    return sum(actor["hp"] for actor in party)


def measurement_contract(result):
    return {
        "capabilities": {"counter_capability": result.get("counter_capability", "unreported"),
                         "trace_capability": result.get("trace_capability", "unreported"),
                         **{key: value for key, value in result.items() if key.endswith("_capability")}},
        "entry_kind": result.get("entry_kind", "unreported"),
        "observer_attached": result.get("observer_attached", "unreported"),
        "table_catalog": result.get("table_catalog", "unreported"),
        "party_policy": result.get("party_policy", "unreported"),
        "entry_observation_boundaries": {
            "first_protagonist_command": result.get("entry_observations", {}).get("first_protagonist_command", {}).get("boundary", "unreported")},
        "turn_definitions": {**{key: result.get("turns", {}).get(key, "unreported")
                                for key in ("actor_turns_definition", "actions_definition")},
                             **{key: value for key, value in result.get("turns", {}).items()
                                if key.endswith("_definition")}},
        "resource_definitions": {**{key: result.get("resources", {}).get(key, "unreported")
                                    for key in ("spend_definition", "recovery_scope", "initial_party_boundary")},
                                 **{key: value for key, value in result.get("resources", {}).items()
                                    if key.endswith(("_definition", "_scope", "_boundary"))}},
    }


def measurement_summary(results):
    item_keys = set()
    for result in results:
        items = result.get("resources", {}).get("items_by_id")
        if isinstance(items, list):
            item_keys.update("items_by_id.%d.used" % item["id"] for item in items
                             if isinstance(item, dict) and type(item.get("id")) is int and
                             0 <= item["id"] < 1536 and type(item.get("used")) is int and item["used"] >= 0)
    groups = {"all": results,
              "win": [result for result in results if result["outcome"] == "win"],
              "defeat": [result for result in results if result["outcome"] == "defeat"]}
    turns = {}
    resources = {}
    for label, members in groups.items():
        keys = sorted({key for result in members for key, value in result.get("turns", {}).items()
                       if type(value) is int})
        turns[label] = {key: distribution([result["turns"][key] for result in members
                                           if type(result.get("turns", {}).get(key)) is int]) for key in keys}
        measured = [resource_measurements(result) for result in members]
        for result, values in zip(members, measured):
            if complete_native_item_ledger(result):
                for key in item_keys:
                    values.setdefault(key, 0)
        keys = sorted({key for result in measured for key in result})
        resources[label] = {key: distribution([result[key] for result in measured if key in result]) for key in keys}
    return {"turns": turns, "resources": resources}


def aggregate(rows):
    rows = sorted(rows, key=lambda row: row["run_index"])
    counts = Counter(row["result"]["outcome"] for row in rows)
    wins = counts["win"]
    errors = sum(counts[key] for key in ERRORS)
    censored = sum(counts[key] for key in CENSORED)
    completed = len(rows) - errors - censored
    resolved = wins + counts["defeat"]
    unresolved = errors + censored
    contracts = {}
    for row in rows:
        result = row["result"]
        if any(key in result for key in ("turns", "resources", "entry_kind")) or any(
                key.endswith("_capability") for key in result):
            key = canonical(measurement_contract(result))
            contracts.setdefault(key, []).append(row)
    cohorts = [{"contract": json.loads(key), "run_indices": [row["run_index"] for row in members],
                **measurement_summary([row["result"] for row in members])}
               for key, members in sorted(contracts.items())]
    resolved_rows = [r for r in rows if r["result"]["outcome"] in ("win", "defeat")]
    comparable = len({canonical(measurement_contract(row["result"])) for row in resolved_rows}) == 1
    rank_turns = comparable and all(type(r["result"].get("turns", {}).get("actor_turns")) is int for r in resolved_rows)
    remaining_hp = {row["run_index"]: final_party_hp(row["result"]) for row in resolved_rows}
    rank_hp = comparable and all(value is not None for value in remaining_hp.values())
    def replay_key(row):
        result = row["result"]
        return (result["outcome"] == "win", -result["turns"]["actor_turns"] if rank_turns else 0,
                remaining_hp[row["run_index"]] if rank_hp else 0, -row["run_index"])
    ranked = sorted(resolved_rows, key=replay_key)
    def seed_row(row):
        return {"run_index": row["run_index"], "seeds": row["seeds"], "outcome": row["result"]["outcome"]}
    rng = {name: {"unique_seed_patterns": len({r["seeds"][name] for r in rows})} for name in STREAMS}
    rng["mt"]["unique_seed_or_one"] = len({r["seeds"]["mt"] | 1 for r in rows})
    boundaries = Counter()
    for row in rows:
        native_resources = row["result"].get("resources", {})
        if "initial_party" in native_resources or "initial_party_boundary" in native_resources:
            boundary = native_resources.get("initial_party_boundary")
            boundaries[boundary if isinstance(boundary, str) else "unreported"] += 1
    return {"schema": 1, "requested": len(rows), "completed": completed, "errors": errors,
            "censored": censored, "outcomes": dict(sorted(counts.items())),
            "win_rate": {"wins": wins, "denominator": completed, "rate": wins / completed if completed else None,
                         "wilson95": wilson(wins, completed),
                         "resolved_only": {"denominator": resolved, "rate": wins / resolved if resolved else None},
                         "all_requested_bounds": [wins / len(rows), (wins + unresolved) / len(rows)] if rows else None,
                         "qualification": "Conditional on this entry/policy and pseudo-random sampling; unresolved runs retained."},
            "measurement_cohorts": cohorts,
            "initial_party_boundaries": dict(sorted(boundaries.items())),
            "wipe_causes": dict(sorted(Counter(str(r["result"].get("terminal_cause", "unreported")) for r in rows
                                               if r["result"]["outcome"] == "defeat").items())),
            "rng": {"derivation_schema": 1, "streams": rng,
                    "policy_stream": "Reserved tag 3; deterministic version-1 policies have lapses off and consume no policy draws.",
                    "draw_count_scope": "Native rng.draws / trace rng_draws count only post-seeding additive-stream draws, not MT/effect/private RNG calls.",
                    "worker_observations": [{"run_index": r["run_index"], "rng": r["result"].get("rng")} for r in rows]},
            "best_seed": seed_row(ranked[-1]) if ranked else None,
            "worst_seed": seed_row(ranked[0]) if ranked else None,
            "replay_ranking": "Resolved fights only: win, fewer actor turns, more total reported final-party HP, lower index; metrics with missing values or differing native capability/definition contracts are omitted.",
            "runs": rows}


def markdown(report):
    rate = report["win_rate"]
    headline = "unavailable" if rate["rate"] is None else "%.1f%%" % (100 * rate["rate"])
    interval = "unavailable" if rate["wilson95"] is None else "%.1f%%–%.1f%%" % tuple(100 * x for x in rate["wilson95"])
    lines = ["# Battle simulation", "", "%d requested; %d completed; %d errors; %d censored." %
             tuple(report[k] for k in ("requested", "completed", "errors", "censored")),
             "", "Win rate: **%s** (%d/%d completed), Wilson 95%% CI %s." % (headline, rate["wins"], rate["denominator"], interval),
             "", rate["qualification"], "", "Outcomes: " + json.dumps(report["outcomes"], sort_keys=True),
             "", "Wipe terminal causes (not inferred explanations): " + json.dumps(report["wipe_causes"], sort_keys=True)]
    lines += ["", "RNG: schema-1 seeded streams; " + report["rng"]["draw_count_scope"]]
    lines += ["", "Resource preparation: " + report.get("resource_profile", "unreported"),
              "", "Initial-party snapshot boundaries: " + json.dumps(report["initial_party_boundaries"], sort_keys=True)]
    for label in ("worst_seed", "best_seed"):
        row = report[label]
        lines += ["", "%s: %s" % (label.replace("_", " ").title(), "unavailable" if row is None else str(row["run_index"]))]
    for index, cohort in enumerate(report["measurement_cohorts"], 1):
        lines += ["", "## Measurement cohort %d" % index,
                  "", "Native contract: " + json.dumps(cohort["contract"], sort_keys=True)]
        for group in ("turns", "resources"):
            lines += ["", "### " + group.title()]
            for name, stats in cohort[group]["all"].items():
                lines.append("- %s: n=%d, median=%g, p90=%g" % (name, stats["n"], stats["p50"], stats["p90"]))
            if not cohort[group]["all"]:
                lines.append("Not reported by the worker; no synthetic measurements.")
    if not report["measurement_cohorts"]:
        lines += ["", "Measurements unavailable; no synthetic measurements."]
    return "\n".join(lines) + "\n"


def admitted_case(wait, rendered=False):
    # No machine-wide admission here: a case starts as soon as a batch worker is free.
    return contextlib.nullcontext()


def worker_environment(runtime_environment, profile):
    assert_fidelity_environment(runtime_environment)
    env = {key: value for key, value in os.environ.items()
           if not key.upper().startswith(("P4PC_", "SDL_"))}
    env.update(runtime_environment or {})
    env.update(P4PC_CONFIG="none", P4PC_SAVE_DIR=str(profile), P4PC_HIDDEN_WINDOW="1",
               SDL_AUDIODRIVER="dummy", P4PC_VIRTUAL_CLOCK="1", P4PC_MODS="0",
               P4PC_PACKS="0", P4PC_INPUT="script", P4PC_TURBO="1",
               P4PC_ASPECT="4:3", P4PC_RESOLUTION="640x448", P4PC_WINDOW_W="640",
               P4PC_WINDOW_H="448", P4PC_FULLSCREEN="0", P4PC_VSYNC="off",
               P4PC_PRESENT="direct", P4PC_PRESENT_FIT="4:3",
               TMPDIR=str(profile), TMP=str(profile), TEMP=str(profile))
    return env


def native_preparation_error(receipt):
    return (isinstance(receipt, dict) and type(receipt.get("schema")) is int and receipt["schema"] == 1 and
            receipt.get("operation") == "battle_prepare" and receipt.get("outcome") == "preparation_error" and
            isinstance(receipt.get("terminal_cause"), str) and bool(receipt["terminal_cause"].strip()))


def read_prepared_output(directory, expected):
    image_file, receipt_file = directory / "effective-save.bin", directory / "receipt.json"
    for path in (directory / "request.bin", image_file, receipt_file):
        if path.is_symlink() or not path.is_file() or path.resolve().parent != directory.resolve():
            raise ScenarioError("preparation publication is not an isolated regular file: " + str(path))
    if file_digest(directory / "request.bin") != expected["request_sha256"]:
        raise ScenarioError("preparation request artifact hash mismatch")
    receipt = load_json(receipt_file)
    if not isinstance(receipt, dict) or type(receipt.get("schema")) is not int or receipt["schema"] != 1:
        raise ScenarioError("native preparation receipt.schema: expected 1")
    required = {"operation": "battle_prepare", "outcome": "prepared",
                "terminal_cause": "native_save_build_image_complete",
                **{key: expected[key] for key in ("definition_sha256", "request_sha256", "source_sha256", "classification")}}
    for key, value in required.items():
        if receipt.get(key) != value:
            raise ScenarioError("native preparation receipt mismatch: " + key)
    if receipt.get("effective_image_native_valid") is not True:
        raise ScenarioError("native preparation receipt lacks canonical native image attestation")
    image = read_image(image_file, "native preparation effective image")
    if type(receipt.get("effective_image_bytes")) is not int or receipt["effective_image_bytes"] != len(image):
        raise ScenarioError("native preparation effective_image_bytes mismatch")
    if receipt.get("effective_image_sha256") != digest(image):
        raise ScenarioError("native preparation effective_image_sha256 mismatch")
    return image, receipt


def freeze_prepared_output(directory, expected, attempt):
    attempt_dir = directory / ("attempt-%d" % attempt)
    image, native = read_prepared_output(attempt_dir, expected)
    frozen = {"schema": 1, "identity": expected, "attempt": attempt,
              "effective_image_sha256": digest(image), "receipt_sha256": file_digest(attempt_dir / "receipt.json")}
    atomic_json(directory / "frozen.json", frozen)
    return image, {"directory": str(directory), "frozen": frozen, "native_receipt": native}


def load_prepared_output(directory, expected):
    frozen = load_json(directory / "frozen.json")
    if not isinstance(frozen, dict) or frozen.get("schema") != 1 or frozen.get("identity") != expected:
        raise ScenarioError("frozen preparation identity mismatch")
    attempt = integer(frozen.get("attempt"), "frozen preparation attempt", 1, 2)
    attempt_dir = directory / ("attempt-%d" % attempt)
    image, native = read_prepared_output(attempt_dir, expected)
    if frozen.get("effective_image_sha256") != digest(image) or frozen.get("receipt_sha256") != file_digest(attempt_dir / "receipt.json"):
        raise ScenarioError("frozen preparation artifact hash mismatch; never regenerate a changed frozen entry")
    return image, {"directory": str(directory), "frozen": frozen, "native_receipt": native}


def prepare_entry(definition, source_image, worker, output, identity, *, resume=False,
                  prepared_from=None, admission=admitted_case, wait=-1, runtime_environment=None):
    assert_fidelity_environment(runtime_environment)
    request = compile_preparation(definition, source_image)
    expected = {"definition_sha256": digest(canonical(definition)), "request_sha256": digest(request),
                "source_sha256": digest(source_image), "classification": definition["classification"],
                **{key: identity[key] for key in ("coordinator_sha256", "port_executable", "worker_arguments", "runtime")}}
    if prepared_from is not None:
        return load_prepared_output(Path(prepared_from).resolve() / "preparation", expected)
    directory = output / "preparation"
    if (directory / "frozen.json").exists():
        if not resume:
            raise ScenarioError("preparation already exists; identical resume is required")
        return load_prepared_output(directory, expected)
    directory.mkdir(exist_ok=True)
    manifest = {"schema": 1, "identity": expected}
    manifest_file = directory / "manifest.json"
    if manifest_file.exists():
        if not resume or load_json(manifest_file) != manifest:
            raise ScenarioError("preparation manifest identity mismatch")
    else:
        if any(directory.iterdir()):
            raise ScenarioError("unbound preparation artifacts retained; refusing to overwrite them")
        atomic_json(manifest_file, manifest)
    last_error = "no completed preparation publication"
    for attempt in (1, 2):
        attempt_dir = directory / ("attempt-%d" % attempt)
        if attempt_dir.exists():
            if not resume:
                raise ScenarioError("preparation attempt already exists; refusing to overwrite it")
            record_file = attempt_dir / "attempt.json"
            recorded = load_json(record_file) if record_file.exists() else None
            if recorded is not None and (not isinstance(recorded, dict) or recorded.get("attempt") != attempt):
                raise ScenarioError("invalid retained preparation attempt receipt")
            if recorded and recorded.get("status") == "native_error":
                raise ScenarioError("recorded native preparation failure: %s; artifacts: %s" % (recorded.get("error"), attempt_dir))
            if recorded and recorded.get("status") == "prepared":
                return freeze_prepared_output(directory, expected, attempt)
            if recorded is None and (attempt_dir / "receipt.json").exists():
                try:
                    native = load_json(attempt_dir / "receipt.json")
                    if native_preparation_error(native):
                        recorded = {"attempt": attempt, "status": "native_error", "error": native["terminal_cause"],
                                    "exit_code": None, "recovered_publication": True}
                    else:
                        # Recover a complete atomic success, never a new growth run.
                        return freeze_prepared_output(directory, expected, attempt)
                except (OSError, ScenarioError, ValueError) as exc:
                    recorded = {"attempt": attempt, "status": "infrastructure_error",
                                "error": "%s: %s" % (type(exc).__name__, exc)}
                atomic_json(record_file, recorded)
                if recorded["status"] == "native_error":
                    raise ScenarioError("retained native preparation failure: %s; artifacts: %s" % (recorded["error"], attempt_dir))
            last_error = recorded.get("error", "interrupted preparation") if recorded else "interrupted preparation"
            if recorded is None:
                atomic_json(record_file, {"attempt": attempt, "status": "interrupted", "error": last_error})
            continue
        if any(output.glob("run-*")):
            raise ScenarioError("frozen preparation publication is missing after seed jobs exist; refusing to regenerate the frozen party")
        attempt_dir.mkdir()
        request_file = attempt_dir / "request.bin"
        request_file.write_bytes(request)
        profile = attempt_dir / "profile"
        profile.mkdir()
        command = list(worker) + ["--battle-prepare", str(request_file),
                                  "--battle-prepared-image", str(attempt_dir / "effective-save.bin"),
                                  "--battle-prepare-result", str(attempt_dir / "receipt.json")]
        record = {"attempt": attempt, "directory": str(attempt_dir), "command": command}
        started = None
        try:
            with admission(wait, False):
                started = time.monotonic()
                with (attempt_dir / "stdout.log").open("wb") as stdout, (attempt_dir / "stderr.log").open("wb") as stderr:
                    process = subprocess.run(command, cwd=profile, env=worker_environment(runtime_environment, profile),
                                             stdout=stdout, stderr=stderr,
                                             timeout=definition["preparation"].get("timeout_seconds", 120),
                                             creationflags=0x4000 if os.name == "nt" else 0)
                record["exit_code"] = process.returncode
                if process.returncode < 0 or process.returncode >= 0x80000000:
                    raise ScenarioError("native preparation terminated abnormally (exit %d)" % process.returncode)
                native = load_json(attempt_dir / "receipt.json")
                if process.returncode != 0 and native_preparation_error(native):
                    record.update(status="native_error", error=native["terminal_cause"])
                else:
                    if process.returncode != 0:
                        raise ScenarioError("native preparation exited %d without a recorded validation failure" % process.returncode)
                    read_prepared_output(attempt_dir, expected)
                    record["status"] = "prepared"
        except (OSError, TimeoutError, subprocess.TimeoutExpired, ScenarioError, ValueError) as exc:
            record.update(status="infrastructure_error", error="%s: %s" % (type(exc).__name__, exc))
        finally:
            record["wall_seconds"] = time.monotonic() - started if started is not None else None
        atomic_json(attempt_dir / "attempt.json", record)
        if record["status"] == "prepared":
            return freeze_prepared_output(directory, expected, attempt)
        if record["status"] == "native_error":
            raise ScenarioError("native preparation failed: %s; no seed workers launched; artifacts: %s" % (record["error"], attempt_dir))
        last_error = record["error"]
    raise ScenarioError("native preparation failed after two attempts: %s; artifacts: %s" % (last_error, directory))


def run_case(scenario, save_image, worker, output, run_index, identity, *, admission=admitted_case,
             wait=-1, rendered=False, replay=False, runtime_environment=None):
    seeds = derive_seeds(scenario["master_seed"], run_index)
    case_dir = output / ("run-%020d" % run_index)
    case_dir.mkdir()
    attempts = []
    result = None
    for attempt in (1, 2):
        directory = case_dir / ("attempt-%d" % attempt)
        directory.mkdir()
        entry = directory / "entry.bin"
        entry.write_bytes(compile_entry(scenario, save_image, seeds, rendered=rendered))
        request = {"schema": 1, "identity": identity, "run_index": run_index, "seeds": seeds,
                   "entry_sha256": digest(entry.read_bytes()), "trace": "full" if replay else "compact"}
        atomic_json(directory / "request.json", request)
        profile = directory / "profile"
        profile.mkdir()
        env = worker_environment(runtime_environment, profile)
        command = list(worker) + ["--battle-sim", str(entry), "--battle-result", str(directory / "result.json"),
                                  "--battle-trace", str(directory / "trace.jsonl")]
        receipt = {"attempt": attempt, "directory": str(directory), "command": command}
        started = None
        try:
            with admission(wait, rendered):
                started = time.monotonic()
                with (directory / "stdout.log").open("wb") as stdout, (directory / "stderr.log").open("wb") as stderr:
                    process = subprocess.run(command, cwd=profile, env=env, stdout=stdout, stderr=stderr,
                                             timeout=scenario["options"].get("timeout_seconds", 120),
                                             creationflags=0x4000 if os.name == "nt" else 0)
                receipt["exit_code"] = process.returncode
                if process.returncode < 0 or process.returncode >= 0x80000000:
                    raise ScenarioError("worker terminated abnormally (exit %d)" % process.returncode)
                published = directory / "result.json"
                if published.exists():
                    candidate = validate_result(load_json(published))
                    if "encounter_id" in candidate and candidate["encounter_id"] != scenario["encounter_id"]:
                        raise ScenarioError("worker result encounter identity mismatch")
                    policy = candidate.get("party_policy")
                    if policy is not None and (policy["id"] != POLICIES[scenario["policy"]["kind"]] or
                                               policy["latency_updates"] != scenario["options"]["latency_updates"]):
                        raise ScenarioError("worker result party policy identity/latency mismatch")
                    native_rng = candidate.get("rng")
                    if isinstance(native_rng, dict):
                        for name in ("additive", "mt", "effect"):
                            key = name + "_seed"
                            if key in native_rng and native_rng[key] != seeds[name]:
                                raise ScenarioError("worker result RNG seed identity mismatch: " + name)
                    # Native limits/errors deliberately return nonzero with a result.
                    if process.returncode != 0 and candidate["outcome"] not in CENSORED | ERRORS:
                        raise ScenarioError("worker crashed/nonzero exit with successful result")
                    result = candidate
                    receipt["status"] = "recorded"
                else:
                    raise ScenarioError("worker did not publish result (exit %d)" % process.returncode)
        except (OSError, TimeoutError, subprocess.TimeoutExpired, ScenarioError, ValueError) as exc:
            receipt.update(status="infrastructure_error", error="%s: %s" % (type(exc).__name__, exc))
        finally:
            receipt["wall_seconds"] = time.monotonic() - started if started is not None else None
        attempts.append(receipt)
        atomic_json(directory / "attempt.json", receipt)
        if result is not None:
            break
    if result is None:
        result = {"schema": 1, "outcome": "engine_error", "terminal_cause": "infrastructure_failure",
                  "error": attempts[-1]["error"]}
    row = {"schema": 1, "identity": identity, "run_index": run_index, "seeds": seeds,
           "attempts": attempts, "result": result}
    atomic_json(case_dir / "run.json", row)
    return row


def run_batch(scenario, base, worker, output, jobs=1, *, resume=False, admission=admitted_case,
              wait=-1, run_index=None, rendered=False, prepared_from=None):
    assert_fidelity_environment()
    validate_scenario(scenario)
    integer(jobs, "jobs", 1, os.cpu_count() or 1)
    if not worker or not all(isinstance(part, str) and part for part in worker):
        raise ScenarioError("worker: expected executable and native resource arguments")
    # Preserve executable reparse-point semantics (notably Windows app execution
    # aliases); dereferencing with Path.resolve can produce an unreadable target.
    # Each native process has a private cwd, so relative files become absolute.
    worker = [os.path.abspath(part) if Path(part).is_file() else part for part in worker]
    if not Path(worker[0]).is_file():
        raise ScenarioError("worker executable must be an existing file")
    source = scenario["entry"]["source"]
    definition = source["definition"] if source["kind"] == "declarative" else None
    if prepared_from is not None and definition is None:
        raise ScenarioError("--prepared-from is only valid for a declarative source")
    if definition is not None and run_index is not None and prepared_from is None:
        raise ScenarioError("declarative replay requires --prepared-from the original batch output; never regenerate the frozen party")
    reference = definition["baseline"] if definition is not None else source
    source_location = "entry.source.definition.baseline" if definition is not None else "entry.source"
    image = Path(reference["image"])
    if not image.is_absolute():
        image = Path(base) / image
    save_image = read_image(image, source_location + ".image")
    if digest(save_image) != reference["sha256"]:
        raise ScenarioError(source_location + ".sha256: frozen image hash mismatch")
    runtime = scenario.get("runtime", {})
    runtime_environment = {}
    runtime_identity = {}
    for resource in ("iso", "elf"):
        if resource not in runtime:
            continue
        path = Path(runtime[resource]["path"])
        if not path.is_absolute():
            path = Path(base) / path
        observed = file_digest(path)
        if observed != runtime[resource]["sha256"]:
            label = "disc image" if resource == "iso" else "retail ELF"
            raise ScenarioError("runtime.%s.sha256: %s hash mismatch" % (resource, label))
        absolute = os.path.abspath(path)
        runtime_identity[resource] = {"path": absolute, "sha256": observed}
        if resource == "iso":
            runtime_environment["P4PC_ISO"] = absolute
        elif len(worker) < 2 or os.path.normcase(os.path.abspath(worker[1])) != os.path.normcase(absolute):
            raise ScenarioError("runtime.elf.path must identify the explicit worker ELF argument")
    if "video_driver" in runtime:
        runtime_environment["SDL_VIDEODRIVER"] = runtime["video_driver"]
        runtime_identity["video_driver"] = runtime["video_driver"]
    identity = {"scenario_sha256": digest(canonical(scenario)), "source_entry_sha256": digest(save_image),
                "coordinator_sha256": file_digest(__file__), "runtime": runtime_identity,
                "policy_sha256": digest(canonical(scenario["policy"])),
                "rules_sha256": digest(canonical(scenario["entry"].get("rules", {}))),
                "master_seed": scenario["master_seed"].lower(),
                "port_executable": {"path": worker[0], "sha256": file_digest(worker[0])},
                "worker_arguments": [{"argument": arg, "sha256": file_digest(arg) if Path(arg).is_file() else None} for arg in worker[1:]],
                "rendered": rendered, "replay": run_index is not None}
    if run_index is not None:
        integer(run_index, "replay run_index", maximum=2**64 - 1)
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    manifest = {"schema": 1, "input_identity": identity, "scenario": scenario,
                "prepared_from": str(Path(prepared_from).resolve()) if prepared_from is not None else None}
    manifest_file = output / "manifest.json"
    if manifest_file.exists():
        if not resume or load_json(manifest_file) != manifest:
            raise ScenarioError("output already contains a batch; resume requires identical scenario/worker identity")
    else:
        if any(output.iterdir()):
            raise ScenarioError("output must be an empty private directory")
        atomic_json(manifest_file, manifest)
    preparation_info = None
    if definition is not None:
        save_image, preparation_info = prepare_entry(
            definition, save_image, worker, output, identity, resume=resume,
            prepared_from=prepared_from, admission=admission, wait=wait, runtime_environment=runtime_environment)
        identity = {**identity, "entry_sha256": digest(save_image),
                    "preparation": {key: preparation_info["frozen"]["identity"][key]
                                    for key in ("definition_sha256", "request_sha256", "classification")}}
    else:
        identity = {**identity, "entry_sha256": digest(save_image)}
    if 64 + len(save_image) + 12 * len(scenario["policy"].get("tape", [])) > 8 * 1024 * 1024:
        raise ScenarioError("battle ENTRY exceeds native 8 MiB request limit; no seed workers launched")
    indices = [run_index] if run_index is not None else range(scenario["seed_start"], scenario["seed_start"] + scenario["n"])
    rows, pending = [], []
    for index in indices:
        case_dir = output / ("run-%020d" % index)
        receipt = case_dir / "run.json"
        if receipt.exists() and resume:
            row = load_json(receipt)
            if row.get("identity") != identity or row.get("run_index") != index or row.get("seeds") != derive_seeds(scenario["master_seed"], index):
                raise ScenarioError("resume result identity mismatch at run %d" % index)
            validate_result(row.get("result"))
            rows.append(row)
        elif case_dir.exists():
            # Retain interrupted attempt files rather than overwriting them.
            suffix = 1
            while case_dir.with_name(case_dir.name + ".interrupted-%d" % suffix).exists():
                suffix += 1
            case_dir.rename(case_dir.with_name(case_dir.name + ".interrupted-%d" % suffix))
            pending.append(index)
        else:
            pending.append(index)
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = [pool.submit(run_case, scenario, save_image, worker, output, index, identity,
                               admission=admission, wait=wait, rendered=rendered, replay=run_index is not None,
                               runtime_environment=runtime_environment) for index in pending]
        rows.extend(future.result() for future in futures)
    report = aggregate(rows)
    report["identity"] = identity
    report["resource_profile"] = "declared_fill_living_hp_sp" if scenario["options"]["fill_resources"] else "native_inherit"
    if preparation_info is not None:
        report["preparation"] = preparation_info
    atomic_json(output / "report.json", report)
    (output / "summary.md").write_text(markdown(report), encoding="utf-8")
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("batch", "replay"))
    parser.add_argument("--scenario", type=Path, required=True)
    parser.add_argument("--worker", nargs="+", required=True, help="native EXE ELF (absolute paths recommended)")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=1, help="maximum concurrent fresh native processes, bounded by logical cores")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--prepared-from", type=Path, help="original declarative batch output containing the frozen native preparation; required for declarative replay")
    parser.add_argument("--index", type=int, help="original run index for replay; does not alter seed derivation")
    parser.add_argument("--wait", type=float, default=-1, help="CPU admission wait, excluding land pauses")
    parser.add_argument("--rendered", action="store_true", help="rendered-reference replay using GPU admission")
    args = parser.parse_args(argv)
    if (args.command == "replay") != (args.index is not None):
        parser.error("replay requires --index; batch does not accept it")
    if args.rendered and args.command != "replay":
        parser.error("--rendered is only for a replay reference")
    try:
        assert_fidelity_environment()
        report = run_batch(load_json(args.scenario), args.scenario.resolve().parent, args.worker, args.out,
                           args.jobs, resume=args.resume, wait=args.wait, run_index=args.index,
                           rendered=args.rendered, prepared_from=args.prepared_from)
    except (ScenarioError, OSError) as exc:
        parser.exit(2, "battle_sim: %s\n" % exc)
    print(markdown(report))
    return 1 if report["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
