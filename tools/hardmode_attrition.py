#!/usr/bin/env python3
"""Attrition metrics from existing hard-mode logs (2026-11-10 protocol).

Computes, per log group (retail vs Hard/Harder, by file name), the measures
the tuning protocol asks for before epochs are frozen:

  * floors per trip: distinct (field, room) pairs seen in a trip's LOG lines,
    grouped by the scripts' trip prefix (castle-6f/7f/8f, bathhouse, ...);
  * healing SP per fight: recovered SP = final SP - initial SP + SP spent,
    from simulator RESULT ``resources`` (spent costs are native skill costs;
    recovery here is any SP regain inside the fight);
  * deaths per 100 encounters: protagonist deaths over encounters seen
    (``dng_decease`` lines in walk logs, ``protagonist_ko`` wipes in sim
    aggregates, outcomes of ``defeat`` otherwise);
  * first-turn ambush deaths: fights where the protagonist is at 0 HP on the
    first protagonist command (``entry_observations.first_protagonist_command``);
    ``null`` when the logs carry no such observation (never guessed from a
    walk banner alone).

Walk-log markers are the ones hard mode already writes:
``HARDMODE_ENCOUNTER id=...``, ``dng_decease``, and ``LOG frame ... field ...
room ...`` (one per ``log`` line). Simulator inputs are single-fight RESULT
documents (``outcome``, ``resources.initial_party``/``final_party``,
``resources.sp_spent``, ``entry_observations``) or a batch ``report`` with
``measurement_cohorts`` (win/total per cohort). A metric with no observable
input is reported as ``null`` with the reason, never filled from another
group. This is offline log analysis only; it never runs the game.

Usage:
    python tools/hardmode_attrition.py LOG... [--out REPORT.json]
        [--group retail|hard|harder|off]
Files are grouped by name (first match of retail|harder|hard|off) unless
--group is given; stdout is the JSON report, --out additionally writes it.
"""
import argparse
import json
import math
import re
import sys
from pathlib import Path

Z = 1.959964  # Wilson 95%


def wilson(wins, n):
    """Wilson 95% interval for wins/n as fractions, or None without data."""
    if n <= 0 or wins < 0 or wins > n:
        return None
    p = wins / n
    den = 1 + Z * Z / n
    centre = (p + Z * Z / (2 * n)) / den
    half = Z * math.sqrt(p * (1 - p) / n + Z * Z / (4 * n * n)) / den
    return (max(0.0, centre - half), min(1.0, centre + half))


def _group(name):
    low = name.lower()
    for label in ("retail", "harder", "hard", "off"):
        if label in low:
            return label
    return "unknown"


def _sum_sp(party):
    if not isinstance(party, list) or not party:
        return None
    total = 0.0
    for member in party:
        sp = member.get("sp")
        if not isinstance(sp, (int, float)):
            return None
        total += sp
    return total


def parse_result(doc, group):
    """One fight RESULT document into encounter counters (or None)."""
    if not isinstance(doc, dict) or "outcome" not in doc:
        return None
    res = doc.get("resources") or {}
    initial = _sum_sp(res.get("initial_party"))
    final = _sum_sp(res.get("final_party"))
    spent = res.get("sp_spent")
    spent = spent if isinstance(spent, (int, float)) else None
    healing = None
    if initial is not None and final is not None and spent is not None:
        healing = max(0.0, final - initial + spent)
    first = ((doc.get("entry_observations") or {}).get("first_protagonist_command") or {})
    party = first.get("party")
    hero_hp = None
    if party:
        hero_hp = party[0].get("hp")
    ambush = None
    if isinstance(hero_hp, (int, float)) and doc.get("outcome") == "defeat":
        ambush = 1 if hero_hp <= 0 else 0
    # A defeat not by brawl is not a death; protagonista_ko is the death cause.
    outcome = doc.get("outcome")
    death = 1 if outcome == "defeat" else 0
    cause = (doc.get("wipe_cause") or "")
    if cause and cause != "protagonist_ko":
        death = 0
    rec = {"encounters": 1, "outcomes": 1, "won": 1 if outcome == "win" else 0,
           "lost": 1 if outcome == "defeat" else 0, "deaths": death,
           "healing_sp": healing, "sp_spent": spent,
           "ambush_deaths": ambush, "floors": set(), "trip": None, "group": group}
    return rec
def parse_report(doc, group):
    """A batch aggregate with measurement_cohorts into wins/totals."""
    if not isinstance(doc, dict) or "measurement_cohorts" not in doc:
        return None
    total = won = 0
    for cohort in doc.get("measurement_cohorts") or []:
        indices = cohort.get("run_indices") or []
        total += len(indices)
    summary = doc.get("summary") or doc.get("wins")
    return {"encounters": total, "outcomes": 0, "won": None, "lost": None, "deaths": None,
            "healing_sp": None, "sp_spent": None, "ambush_deaths": None,
            "floors": set(), "trip": None, "group": group}


def parse_walk_log(text, group, name):
    if group == "unknown" and "HARDMODE_ENCOUNTER" in text:
        group = "hard"  # the encounter line only exists with hard mode on
    encounters = re.findall(r"HARDMODE_ENCOUNTER id=(\d+) boss=(true|false)", text)
    floors = set()
    for field, room in re.findall(r"field (\d+) room (\d+)", text):
        floors.add((int(field), int(room)))
    deaths = len(re.findall(r"dng_decease", text))
    trip = None
    stem = Path(name).stem
    for prefix in ("castle", "bathhouse", "kanji", "yukiko", "knight"):
        if prefix in stem.lower():
            trip = stem
            break
    return {"encounters": len(encounters), "outcomes": 0, "won": None, "lost": deaths,
            "deaths": deaths, "healing_sp": None, "sp_spent": None,
            "ambush_deaths": None, "floors": floors, "trip": trip,
            "group": group}


def load_file(path, forced_group=None):
    group = forced_group or _group(path.name)
    if path.suffix.lower() == ".json":
        doc = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(doc, list):
            return [parse_result(item, group) for item in doc]
        parsed = parse_result(doc, group) or parse_report(doc, group)
        return [parsed] if parsed else []
    return [parse_walk_log(path.read_text(encoding="utf-8", errors="replace"),
                           group, path.name)]


def summarize(records):
    groups = {}
    for rec in records:
        if not rec:
            continue
        acc = groups.setdefault(rec["group"], {"encounters": 0, "outcomes": 0,
                                             "won": 0, "lost": 0, "deaths": 0,
                                             "healing": [], "sp_spent": [],
                                             "ambush_seen": 0, "ambush": 0,
                                             "floors": 0, "trips": set()})
        acc["encounters"] += rec["encounters"]
        acc["outcomes"] += rec["outcomes"]
        acc["won"] += rec["won"] or 0
        acc["lost"] += rec["lost"] or 0
        acc["deaths"] += rec["deaths"] or 0
        if rec["healing_sp"] is not None:
            acc["healing"].append(rec["healing_sp"])
        if rec["sp_spent"] is not None:
            acc["sp_spent"].append(rec["sp_spent"])
        if rec["ambush_deaths"] is not None:
            acc["ambush_seen"] += 1
            acc["ambush"] += rec["ambush_deaths"]
        acc["floors"] += len(rec["floors"])
        if rec["trip"]:
            acc["trips"].add(rec["trip"])
    out = {}
    for group, acc in sorted(groups.items()):
        encounters = acc["encounters"]
        healing = acc["healing"]
        out[group] = {
            "encounters": encounters,
            "win_rate": ({"wins": acc["won"], "n": acc["outcomes"],
                          "wilson": wilson(acc["won"], acc["outcomes"])}
                         if acc["outcomes"] else None),
            "floors_per_trip": ({"floors": acc["floors"],
                                 "trips": sorted(acc["trips"]),
                                 "floors_per_trip":
                                 (acc["floors"] / len(acc["trips"]))
                                 if acc["trips"] else None}),
            "healing_sp_per_fight": ({"mean": sum(healing) / len(healing),
                                      "n": len(healing)}
                                     if healing else None),
            "sp_spent_per_fight": ({"mean": sum(acc["sp_spent"]) / len(acc["sp_spent"]),
                                    "n": len(acc["sp_spent"])}
                                   if acc["sp_spent"] else None),
            "deaths_per_100_encounters": (
                {"deaths": acc["deaths"], "encounters": encounters,
                 "per_100": (100 * acc["deaths"] / encounters)
                 if encounters else None}),
            "first_turn_ambush_deaths": (
                {"ambush_deaths": acc["ambush"], "observed_fights": acc["ambush_seen"]}
                if acc["ambush_seen"] else None),
        }
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="+", help="walk .log or RESULT .json files")
    parser.add_argument("--out", help="also write the JSON report here")
    parser.add_argument("--group", choices=("retail", "hard", "harder", "off"),
                        help="force every input into this group")
    args = parser.parse_args()
    records = []
    for name in args.logs:
        records.extend(load_file(Path(name), args.group))
    records = [r for r in records if r]
    report = {"metrics": summarize(records),
              "notes": ("null = not observable in the provided logs; "
                        "GPU walks are needed for ambush/healing/attrition "
                        "over real dungeon trips")}
    text = json.dumps(report, indent=2, sort_keys=True)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
