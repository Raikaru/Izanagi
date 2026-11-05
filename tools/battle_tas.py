#!/usr/bin/env python3
"""Boss-win search on the real walk: a fixed `battleturn:` tape that wins the selected fight.

The worker is the port's live-natural mode (docs/platform/battle_sim.md): title LOAD from a folder card, the real walk to
the selected fight, no reseeding or restored state. With P4PC_BATTLE_FORK (Linux only) the worker stops at each fresh
party command once the walk's battleturn: lines are spent and forks one child per candidate command, so a candidate
costs only the battle after its checkpoint. The search is a beam over native command menus: Attack, Guard, the acting
persona's skills and the battle consumables in stock, read from the request-time RAM image and the native banks.

The output is a gate-ready walk (tape.walk): the boss walk's own prefix, one `battle:` line and one `battleturn:` line per
fresh command. A fresh run of that walk (no fork) is the witness: its trace, result, lockstep input log and per-frame
states must equal the search path's concatenated segments. The only excluded value is the request-RAM SHA-256, which
covers host heap pointers stored in guest memory and so differs between processes. Enemy HP ranks candidates only;
victory is the native terminal. Not a balance claim.
"""
import argparse
import errno
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import time

RAM_START = 0x00100000
KEYS_ATTACK = "circle,cross,cross"
KEYS_GUARD = "circle,up,cross"
KEYS_SKILL = "circle,down,cross,skill:%d,cross,cross"
KEYS_ITEM = "circle,down,down,down,cross,item:%d,cross,cross"
PERSONA_SELECTED_SLOT = 0x00797F88
PERSONA_ROSTER = 0x00797F8C
PARTY_SLOTS = 0x00796E50
STOCK = 0x0079757A
SKILL_BANK_SLOT = 0x007644A8
ITEM_BANK_SLOT = 0x007642A0
SKILL_LIMIT = 0x1B8  # datCalcSkillCost's retail skill-ID bound
SILENCE = 0x8  # DatUnit +0xC ailment (datCalcChkBadStatus): the Skill command refuses
CHARGED = 0xC0000  # DatUnit +0x14: Power/Mind Charge (record word 2)
GUARDING = 0x200  # DatUnit +0x00 flags: guarding, datCalcIsGuarding (record word 0)


class SearchError(RuntimeError):
    pass


def live_entry(mode, encounter, occurrence, route_limit, fight_limit, action_limit):
    """The 64-byte P4BLIVE1 entry (include/p4pc/battle_live.h): renderless, flip clock, fight-only completion."""
    return struct.pack("<8s14I", b"P4BLIVE1", 64, 3, mode, encounter, occurrence, route_limit, fight_limit,
                       action_limit, 1, 0, 0, 0, 0, 0)


def tape_walk(prefix_lines, driver, turns, timeout):
    if any(not line or "\n" in line for line in turns):
        raise SearchError("battleturn lines must be single, nonempty lines")
    return "\n".join(list(prefix_lines) + ["timeout:%d" % timeout, "battle:" + driver] +
                     ["battleturn:" + line for line in turns] + ["waittask:battle:off"]) + "\n"


def walk_prefix(walk, through=None, count=None):
    """The boss walk up to the fight: through its single `through` line, or its first `count` lines."""
    lines = Path(walk).read_text(encoding="utf-8").splitlines()
    if count is not None:
        if not 0 < count <= len(lines):
            raise SearchError("walk has %d lines, not a %d-line prefix" % (len(lines), count))
        return lines[:count]
    if lines.count(through) != 1:
        raise SearchError("walk must contain exactly one %r line to end the prefix at" % through)
    return lines[:lines.index(through) + 1]


class Ram:
    """Request-time guest RAM image (live mode's TRACE.request-ram)."""
    def __init__(self, data):
        self.data = data

    def u8(self, address):
        return self.data[address - RAM_START]

    def u16(self, address):
        return struct.unpack_from("<H", self.data, address - RAM_START)[0]

    def u32(self, address):
        return struct.unpack_from("<I", self.data, address - RAM_START)[0]


def skill_row(ram, skill):
    """DatSkillEntry (0x28 bytes): flags01 bit 2 lists it (the walker's skill_active), cost kind @3 (1 HP %, 2 SP),
    cost @4, target class @8 (1 every target), side @9 (1 allies, 2 enemies), filter @10 (4 downed members),
    HP effect @0x11 (1 damage, 2/5/11 restore, 0 none), power @0x12, status mode @0x18 (2 removes the ailments
    in the low 24 bits @0x1C)."""
    row = ram.u32(SKILL_BANK_SLOT) + skill * 0x28
    return {"id": skill, "listed": bool(ram.u8(row + 1) & 2), "cost_kind": ram.u8(row + 3), "cost": ram.u16(row + 4),
            "all": ram.u8(row + 8) == 1, "side": ram.u8(row + 9), "filter": ram.u8(row + 10),
            "effect": ram.u8(row + 0x11), "power": struct.unpack("<h", struct.pack("<H", ram.u16(row + 0x12)))[0],
            "cures": ram.u32(row + 0x1C) & 0xFFFFFF if ram.u8(row + 0x18) == 2 else 0}


def persona_skills(ram, member):
    """datPersonaGetRecord: the selected roster slot for the protagonist, the party slot's persona otherwise."""
    record = (PERSONA_ROSTER + ram.u16(PERSONA_SELECTED_SLOT) * 0x30 if member == 1 else
              PARTY_SLOTS + (member - 2) * 0x88 + 0x54)
    return [skill for skill in (ram.u16(record + 0xC + 2 * i) for i in range(8)) if 0 < skill < SKILL_LIMIT]


def menu(ram, members):
    """Skill and Item lists as btlSelectionRefreshList builds them (walk_script.c battle_list)."""
    party = {member: [row for row in (skill_row(ram, s) for s in persona_skills(ram, member)) if row["listed"]]
             for member in members}
    base = ram.u32(ITEM_BANK_SLOT)
    items = {}
    for item in range(0x301, 0x400):
        count = ram.u8(STOCK + item)
        skill = ram.u16(base + item * 0x44 + 0x24) if count else 0
        if count and 0 < skill < SKILL_LIMIT and skill_row(ram, skill)["listed"]:
            items[item] = {"count": count, "row": skill_row(ram, skill)}
    return party, items


def ally_targets(row, state, limits):
    """Ally targets the native menu accepts: a downed member to revive, a member with an ailment the row removes, a hurt
    member to restore, anyone for an effect without HP (a buff). The menu refuses a cure or restore with nothing to do."""
    alive = [u for u in state["party"] if u["hp"]]
    if row["filter"] == 4:
        downed = [u["id"] for u in state["party"] if not u["hp"]]
        return [] if not downed else [None] if row["all"] else downed
    if row["cures"]:
        needy = [u for u in alive if u["status"] & row["cures"]]
    elif row["effect"] == 0:
        needy = alive
    else:
        needy = [u for u in alive if u["hp"] < limits[u["id"]]["max_hp"]]
    if not needy:
        return []
    if row["all"]:
        return [None]
    return [min(needy, key=lambda u: (u["hp"] * 1000 // max(limits[u["id"]]["max_hp"], 1), u["id"]))["id"]]


def candidates(actor, state, party_menu, items, limits, used_items):
    """(key, battleturn line) pairs for this fresh command, in a stable order."""
    if actor not in limits:
        raise SearchError("no party admission identifies the acting member (%r)" % (actor,))
    enemies = sorted({u["id"] for u in state["enemies"] if u["hp"]})
    me = next((u for u in state["party"] if u["id"] == actor), None)
    out = [(("attack", enemy), "enemy:%d=%s" % (enemy, KEYS_ATTACK)) for enemy in enemies]
    out.append((("guard",), KEYS_GUARD))

    def place(key, keys, row):
        if row["side"] == 2:
            return [(key, keys)] if row["all"] else [(key + (e,), "enemy:%d=%s" % (e, keys)) for e in enemies]
        if row["side"] != 1:
            return []
        targets = ally_targets(row, state, limits)
        verb = "dead" if row["filter"] == 4 else "ally"
        return [(key + ((t,) if t else ()), "%s:%d=%s" % (verb, t, keys) if t else keys) for t in targets]

    for row in party_menu.get(actor, []) if me is not None and not me["status"] & SILENCE else []:
        if row["cost_kind"] == 2 and row["cost"] > me["sp"]:
            continue
        if row["cost_kind"] == 1 and me["hp"] * 100 <= row["cost"] * limits[actor]["max_hp"]:
            continue
        if row["cost_kind"] in (1, 2):
            out += place(("skill", row["id"]), KEYS_SKILL % row["id"], row)
    for item, info in sorted(items.items()):
        if info["count"] > used_items.get(item, 0):
            out += place(("item", item), KEYS_ITEM % item, info["row"])
    return out


def units(checkpoint):
    """Linked units with records; `words` holds the DatUnit record words +0x00 and +0x10..+0x20 when reported."""
    groups = checkpoint.get("linked_units")
    if not isinstance(groups, list) or len(groups) != 2:
        raise SearchError("checkpoint %s has no linked units" % checkpoint.get("node"))
    words = checkpoint.get("record_words") or [[], []]
    def side(group, extra):
        out = []
        for index, entry in enumerate(group):
            if entry["guest_record"] is None:
                continue
            unit = {"id": entry["id"], "hp": entry["hp"], "sp": entry["sp"], "status": entry["status"]}
            if index < len(extra) and extra[index] is not None:
                unit["words"] = extra[index]
            out.append(unit)
        return out
    return {"party": side(groups[0], words[0]), "enemies": side(groups[1], words[1])}


def score(state, limits):
    """Search ranking only: enemy HP lost dominates, then the party's HP and SP fractions (current values above the
    maximum count as full) and living members. With record words, each charged enemy (Power/Mind Charge) costs every
    living member an expected 80% of maximum HP, 40% while guarding, so the beam keeps guarded and healed states."""
    enemy = sum(u["hp"] for u in state["enemies"])
    party = [(u, limits[u["id"]]) for u in state["party"] if u["id"] in limits]
    hp = sum(min(u["hp"], limit["max_hp"]) / max(limit["max_hp"], 1) for u, limit in party)
    sp = sum(min(u["sp"], limit["max_sp"]) / max(limit["max_sp"], 1) for u, limit in party)
    alive = sum(1 for u, _ in party if u["hp"])
    charged = sum(1 for u in state["enemies"] if u["hp"] and u.get("words") and u["words"][2] & CHARGED)
    risk = 0.0
    for u, limit in party if charged else ():
        if not u["hp"]:
            continue
        guarding = u.get("words") and u["words"][0] & GUARDING
        loss = min(u["hp"], (0.4 if guarding else 0.8) * charged * limit["max_hp"])
        risk += 150.0 * loss / max(limit["max_hp"], 1) + (300.0 if loss >= u["hp"] else 0.0)
    return -enemy + 150.0 * hp + 40.0 * sp + 300.0 * alive - risk


def rank(node, limits):
    """score plus a dominating bonus per declared later-phase enemy that has appeared on the node's path, so a fight
    whose next form brings a fresh HP pool is not ranked behind paths that never reach it."""
    return score(node.state, limits) + 100000.0 * len(node.phases)


def trim(ranked, beam, guard_reserve):
    """The beam: the best `beam` nodes, with up to `guard_reserve` of the best Guard nodes kept in it."""
    keep = ranked[:beam]
    missing = [c for c in [c for c in ranked if c.key[0] == "guard"][:guard_reserve] if c not in keep]
    return keep[:beam - len(missing)] + missing if missing else keep


def state_key(node):
    """Pruning key: equal update/draw counts, units, actor and stock spent are treated as one state."""
    checkpoint = node.checkpoint
    return json.dumps([checkpoint["fight_updates"], checkpoint["additive_draws"], node.actor, node.state,
                       sorted(node.used_items.items())], sort_keys=True)


class Node:
    def __init__(self, name, parent, line, path, key=None):
        self.name, self.parent, self.line, self.path, self.key = name, parent, line, path, key
        self.checkpoint = self.result = self.state = self.failed = self.actor = self.process = None
        self.used_items = dict(parent.used_items) if parent else {}
        self.phases = frozenset()
        if key and key[0] == "item":
            self.used_items[key[1]] = self.used_items.get(key[1], 0) + 1

    def chain(self):
        out, node = [], self
        while node is not None:
            out.append(node)
            node = node.parent
        return out[::-1]

    def lines(self):
        return [node.line for node in self.chain()[1:]]

    def pid(self):
        marker = self.path / "pid"
        try:
            return int(marker.read_text().strip()) if marker.exists() else None
        except ValueError:
            return None


def send(node, commands, deadline):
    """Writes to a checkpoint's control FIFO; a reader that is gone is reported, never waited on forever."""
    data = "".join(command + "\n" for command in commands).encode("ascii")
    while True:
        try:
            fd = os.open(node.path / "control", os.O_WRONLY | os.O_NONBLOCK)
            break
        except OSError as error:
            if error.errno != errno.ENXIO or time.monotonic() > deadline:
                raise SearchError("checkpoint %s is not serving its control FIFO" % node.name) from error
            time.sleep(0.01)
    try:
        os.set_blocking(fd, True)
        os.write(fd, data)
    finally:
        os.close(fd)


def release(nodes):
    for node in nodes:
        if node.checkpoint is not None and node.result is None:
            try:
                send(node, ["exit"], time.monotonic() + 30)
            except SearchError:
                pass


def exited(node):
    if node.process is not None:
        return node.process.poll() is not None
    pid = node.pid()
    return pid is not None and not Path("/proc/%d" % pid).exists()


def kill(node):
    if node.process is not None:
        node.process.kill()
        node.process.wait()
    elif node.pid():
        try:
            os.kill(node.pid(), signal.SIGKILL)
        except ProcessLookupError:
            pass



def poll(node, deadline):
    """True once the node reached its next checkpoint, published a result, or failed; a straggler is killed."""
    result, checkpoint = node.path / "result.json", node.path / "checkpoint.json"
    if result.exists():
        node.result = json.loads(result.read_text(encoding="utf-8"))
    elif checkpoint.exists():
        node.checkpoint = json.loads(checkpoint.read_text(encoding="utf-8"))
        node.state = units(node.checkpoint)
        node.actor = node.checkpoint.get("actor")  # the party member whose command is ready (native unit ID)
        if node.checkpoint.get("threads") != 1:
            node.failed = "checkpoint_not_single_threaded"
    elif (node.path / "fork_failed").exists():
        node.failed = "fork_failed"
    elif exited(node):
        if result.exists() or checkpoint.exists():
            return False  # published between the checks above and the exit: read it on the next pass
        node.failed = "exited_without_checkpoint_or_result"
    elif time.monotonic() > deadline:
        node.failed = "step_timeout"
        kill(node)
    else:
        return False
    return True


def settle(nodes, deadline):
    pending = list(nodes)
    while pending:
        pending = [node for node in pending if not poll(node, deadline)]
        if pending:
            time.sleep(0.02)


def worker_env(args, profile, lockstep, walk, fork_dir):
    env = {k: v for k, v in os.environ.items() if not k.upper().startswith(("P4PC_", "SDL_"))}
    env.update(P4PC_SAVE_DIR=str(profile), P4PC_CONFIG="none", P4PC_LOCKSTEP_DIR=str(lockstep), P4PC_INPUT="script",
               P4PC_MODS="0", P4PC_PACKS="0", P4PC_VIRTUAL_CLOCK="1", P4PC_TURBO="2", P4PC_WALK_SCRIPT=str(walk),
               P4PC_AUTOPRESS_LOAD="0", P4PC_AUTOPRESS_FRAMES="1", P4PC_ISO=str(args.iso), P4PC_HIDDEN_WINDOW="1",
               SDL_AUDIODRIVER="dummy", SDL_VIDEODRIVER="offscreen", TMPDIR=str(profile), TMP=str(profile),
               TEMP=str(profile))
    if fork_dir is not None:
        env["P4PC_BATTLE_FORK"] = str(fork_dir)
    return env


def start_worker(args, directory, walk, entry, fork_dir=None):
    directory.mkdir(parents=True, exist_ok=fork_dir is not None)
    profile = directory / "profile"
    shutil.copytree(args.card, profile / "mc0")
    command = [str(args.worker), str(args.elf), "--battle-sim", str(entry), "--battle-live",
               "--battle-result", str(directory / "result.json"), "--battle-trace", str(directory / "trace.jsonl")]
    with (directory / "stdout.log").open("wb") as out, (directory / "stderr.log").open("wb") as err:
        return subprocess.Popen(command, cwd=profile, stdout=out, stderr=err,
                                env=worker_env(args, profile, directory / "lockstep", walk, fork_dir))


def segments(chain, name):
    return b"".join((node.path / name).read_bytes() for node in chain if (node.path / name).exists())


def without_request_ram_hash(data, field):
    out = []
    for line in data.decode("utf-8").splitlines():
        record = json.loads(line)
        record.pop(field, None)
        out.append(json.dumps(record, sort_keys=True, separators=(",", ":")))
    return "\n".join(out).encode()


def witness(args, out, winner, entry):
    """A fresh process runs tape.walk without the fork; its files must equal the search path's segments."""
    replay = out / "replay"
    started = time.monotonic()
    process = start_worker(args, replay, out / "tape.walk", entry)
    try:
        code = process.wait(timeout=args.replay_timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        code = None
    chain = winner.chain()
    report = {"exit_code": code, "seconds": time.monotonic() - started, "files": {}}
    pairs = {"trace.jsonl": ("ram_sha256", segments(chain, "trace.jsonl")),
             "result.json": ("request_ram_sha256", segments([winner], "result.json")),
             "lockstep/input.p4input": (None, segments(chain, "lockstep/input.p4input")),
             "lockstep/port-states.tsv": (None, segments(chain, "lockstep/port-states.tsv"))}
    for name, (field, searched) in pairs.items():
        path = replay / name
        fresh = path.read_bytes() if path.exists() else None
        entry_report = {"bytes": len(searched), "byte_equal": fresh == searched,
                        "search_sha256": hashlib.sha256(searched).hexdigest(),
                        "fresh_sha256": hashlib.sha256(fresh).hexdigest() if fresh is not None else None}
        if field:
            entry_report["excluded_field"] = field
            entry_report["equal"] = fresh is not None and \
                without_request_ram_hash(fresh, field) == without_request_ram_hash(searched, field)
        else:
            entry_report["equal"] = entry_report["byte_equal"]
        report["files"][name] = entry_report
    report["equal"] = code == 0 and all(f["equal"] for f in report["files"].values())
    return report


def search(args):
    out = Path(args.out).resolve()
    fork_dir = out / "nodes"
    fork_dir.mkdir(parents=True)
    prefix = walk_prefix(args.walk, args.prefix_through, args.prefix_lines)
    driver = args.driver
    (out / "root.walk").write_text(tape_walk(prefix, driver, [], args.fight_timeout), encoding="utf-8")
    entry = out / "fight.entry"
    entry.write_bytes(live_entry(args.mode, args.encounter, args.occurrence, args.route_limit, args.fight_limit,
                                 args.action_limit))
    started = time.monotonic()
    root = Node("root", None, None, fork_dir / "root")
    root.process = start_worker(args, root.path, out / "root.walk", entry, fork_dir)
    settle([root], started + args.root_timeout)
    if root.checkpoint is None:
        kill(root)
        raise SearchError("the root did not reach the fight's first fresh command: %s" %
                          (root.failed or (root.result or {}).get("terminal_cause")))
    log = {"schema": 1, "route_seconds": time.monotonic() - started, "beam": args.beam, "jobs": args.jobs,
           "driver": driver, "depths": []}
    first = None
    with (root.path / "trace.jsonl").open(encoding="utf-8") as trace:
        for line in trace:
            if '"kind":"first_protagonist_command"' in line:
                first = json.loads(line)
    if first is None:
        raise SearchError("root trace has no first_protagonist_command record")
    limits = {u["id"]: u for u in first["party"]}
    ram = Ram((root.path / "trace.jsonl.request-ram").read_bytes())
    party_menu, items = menu(ram, sorted(limits))
    log["menu"] = {"skills": {str(k): [r["id"] for r in v] for k, v in party_menu.items()},
                   "items": {"0x%x" % k: v["count"] for k, v in items.items()}}
    phase_ids = set(args.phase_enemy)
    root.phases = frozenset(phase_ids & {u["id"] for u in root.state["enemies"]})
    frontier, counter, winner, seen = [root], 0, None, set()
    best = root
    # A command the native menu refuses loops until the step timeout. Refusal depends on state the checkpoint does not
    # carry (buffs to dispel, for one), so a command refused at least twice and twice as often as accepted is dropped.
    refused, taken = {}, {}
    audit = (out / "nodes.jsonl").open("w", encoding="utf-8")
    for depth in range(args.max_depth):
        if args.max_seconds and time.monotonic() - started > args.max_seconds:
            log["stopped"] = "max_seconds"
            break
        queue = []
        for node in frontier:
            for key, line in candidates(node.actor, node.state, party_menu, items, limits, node.used_items):
                if refused.get(key[:2], 0) >= max(2, 2 * taken.get(key[:2], 0)):
                    continue
                counter += 1
                queue.append(Node("n%d" % counter, node, line, fork_dir / ("n%d" % counter), key))
        children, inflight, deadlines = len(queue), [], {}
        kept, wins, failed, duplicates = [], [], 0, 0
        while queue or inflight:
            # Keep --jobs continuations running: one fork per freed slot, never a wave waiting on its slowest member.
            while queue and len(inflight) < args.jobs and not wins:
                child = queue.pop(0)
                send(child.parent, ["fork %s %s" % (child.name, child.line)], time.monotonic() + 60)
                deadlines[child.name] = time.monotonic() + args.step_timeout
                inflight.append(child)
            if wins:
                queue = []
            done = [child for child in inflight if poll(child, deadlines[child.name])]
            if not done:
                time.sleep(0.02)
                continue
            for child in done:
                inflight.remove(child)
                status = child.failed or ("result" if child.result is not None else "checkpoint")
                if child.failed in ("step_timeout", "exited_without_checkpoint_or_result"):
                    refused[child.key[:2]] = refused.get(child.key[:2], 0) + 1
                elif not child.failed:
                    taken[child.key[:2]] = taken.get(child.key[:2], 0) + 1
                audit.write(json.dumps({"depth": depth, "node": child.name, "parent": child.parent.name,
                                        "actor": child.parent.actor, "line": child.line, "status": status,
                                        "outcome": (child.result or {}).get("outcome"),
                                        "score": score(child.state, limits) if child.checkpoint else None}) + "\n")
                if child.failed:
                    failed += 1
                elif child.result is not None:
                    if child.result.get("outcome") == "win":
                        wins.append(child)
                elif child.checkpoint is not None:
                    child.phases = child.parent.phases | (phase_ids & {u["id"] for u in child.state["enemies"]})
                    key = state_key(child)
                    if key in seen:
                        duplicates += 1
                        release([child])
                        continue
                    seen.add(key)
                    kept.append(child)
            audit.flush()
            kept.sort(key=lambda c: (-rank(c, limits), int(c.name[1:])))
            beam = trim(kept, args.beam, args.guard_reserve)
            release([c for c in kept if c not in beam])
            kept = sorted(beam, key=lambda c: (-rank(c, limits), int(c.name[1:])))
            if kept and rank(kept[0], limits) > rank(best, limits):
                best = kept[0]
        release(frontier)
        log["depths"].append({"depth": depth, "children": children, "kept": len(kept), "wins": len(wins),
                              "failed": failed, "duplicates": duplicates, "elapsed": time.monotonic() - started,
                              "best_score": rank(kept[0], limits) if kept else None,
                              "best_enemy_hp": sum(u["hp"] for u in kept[0].state["enemies"]) if kept else None,
                              "best_enemies": [[u["id"], u["hp"]] for u in kept[0].state["enemies"]] if kept else None,
                              "best_party_hp": [[u["id"], u["hp"]] for u in kept[0].state["party"]] if kept else None})
        print(json.dumps(log["depths"][-1]), flush=True)
        if wins:
            winner = min(wins, key=lambda c: int(c.name[1:]))
            release(kept)
            break
        frontier = kept
        if not frontier:
            break
    if winner is None:
        release(frontier)
    audit.close()
    log["refused_commands"] = sorted([list(k), n, taken.get(k, 0)] for k, n in refused.items())
    log["search_seconds"] = time.monotonic() - started
    if winner is None:
        # How close it got: the best-ranked checkpoint ever kept, and the tape that reaches it.
        log["outcome"] = "no_win_found"
        log["closest"] = {"node": best.name, "commands": len(best.lines()), "fight_updates":
                          (best.checkpoint or {}).get("fight_updates"), "enemies": best.state["enemies"],
                          "party": best.state["party"], "phases": sorted(best.phases), "rank": rank(best, limits)}
        (out / "closest.walk").write_text(tape_walk(prefix, driver, best.lines(), args.fight_timeout), encoding="utf-8")
        (out / "search.json").write_text(json.dumps(log, indent=1) + "\n", encoding="utf-8")
        print(json.dumps({"outcome": "no_win_found", "closest": log["closest"]}), flush=True)
        return 1
    turns = winner.lines()
    tape = tape_walk(prefix, driver, turns, args.fight_timeout)
    (out / "tape.walk").write_text(tape, encoding="utf-8")
    log.update(outcome="win", winner=winner.name, turns=turns, tape_sha256=hashlib.sha256(tape.encode()).hexdigest(),
               result=winner.result, path=[node.name for node in winner.chain()])
    log["witness"] = witness(args, out, winner, entry)
    (out / "search.json").write_text(json.dumps(log, indent=1) + "\n", encoding="utf-8")
    print(json.dumps({"outcome": "win", "turns": len(turns), "search_seconds": log["search_seconds"],
                      "witness_equal": log["witness"]["equal"]}), flush=True)
    return 0 if log["witness"]["equal"] else 2


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", type=Path, required=True, help="Linux p4pc64 built with the battle fork")
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--card", type=Path, required=True, help="pristine folder card (the contents of mc0)")
    parser.add_argument("--walk", type=Path, required=True, help="the boss walk")
    prefix = parser.add_mutually_exclusive_group()
    prefix.add_argument("--prefix-through", default="skiptask:battle", help="the last walk line before the boss battle")
    prefix.add_argument("--prefix-lines", type=int, help="instead, the number of walk lines before the boss battle")
    parser.add_argument("--phase-enemy", type=int, action="append", default=[],
                        help="native ID of a later form in the same battle; reaching it outranks any earlier-form HP")
    parser.add_argument("--guard-reserve", type=int, default=0, help="best Guard nodes always kept in the beam")
    parser.add_argument("--max-seconds", type=float, help="stop searching after this long and report the closest node")
    parser.add_argument("--driver", default="plan=%s;delay=80" % KEYS_GUARD,
                        help="battle: options, identical in search and gate (the walker requires a plan)")
    parser.add_argument("--mode", type=int, default=1, help="P4BLIVE1 request mode (1 script sequence)")
    parser.add_argument("--encounter", type=int, required=True)
    parser.add_argument("--occurrence", type=int, default=1)
    parser.add_argument("--route-limit", type=int, default=400000)
    parser.add_argument("--fight-limit", type=int, default=200000)
    parser.add_argument("--action-limit", type=int, default=10000)
    parser.add_argument("--fight-timeout", type=int, default=200000, help="the walk's timeout: line before battle:")
    parser.add_argument("--beam", type=int, default=8)
    parser.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    parser.add_argument("--max-depth", type=int, default=400)
    parser.add_argument("--root-timeout", type=float, default=900)
    parser.add_argument("--step-timeout", type=float, default=10,
                        help="seconds a candidate may take to reach the next fresh command (a refused command loops)")
    parser.add_argument("--replay-timeout", type=float, default=1800)
    parser.add_argument("--out", type=Path, required=True, help="new private output directory")
    args = parser.parse_args(argv)
    if not sys.platform.startswith("linux"):
        parser.error("the fork search needs a Linux worker (P4PC_BATTLE_FORK)")
    try:
        return search(args)
    except SearchError as error:
        print("battle_tas: %s" % error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
