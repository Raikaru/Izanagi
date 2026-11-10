"""Capability registry: per-key declaration fragments composed into the shipped p4.d.luau.

Each public capability is one file, include/p4pc/p4.d/<key>.luau, whose first line is
`--!capability <key>@<revision>` (CONTRIBUTING.md "API capabilities"). The build reads the
same first line to generate the native registry (cmake/sources/052-capabilities.cmake), so
p4.has, p4.capabilities and mod.ini `requires=` agree with these files.

Directives (each starts a block that runs to the next directive):
  (none)                  top-level declarations, emitted before `declare p4`
  --!in p4.<path>         fields appended to that namespace table (created when absent)
  --!overload p4.<path>   one more `& (...)` member of an intersection-typed field
  --!example              Luau that must type-check against the composed definitions
  --!refuse               one Luau statement per line, each of which must fail to type-check
  --!covers ITEM...       (one line) surfaces this key provides beyond its --!in/--!overload ones:
                          p4.<path> (a namespace or function), event:<name>, operation:<name>
                          (p4.authorize.on) or section:<name>[.<feature>] (a mod.ini section or feature); p4mod check
                          maps what a mod uses to keys with them

  python tools/p4_capabilities.py list              key@revision per line
  python tools/p4_capabilities.py check             validate every fragment composes
  python tools/p4_capabilities.py compose OUT       write the composed p4.d.luau
  python tools/p4_capabilities.py index OUT         write "key revision surface..." per key
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "include" / "p4pc" / "p4.d.luau"
FRAGMENTS = ROOT / "include" / "p4pc" / "p4.d"

KEY = re.compile(r"[a-z][a-z0-9_-]*(?:\.[a-z][a-z0-9_-]*)*")
HEADER = re.compile(r"--!capability (" + KEY.pattern + r")@([1-9][0-9]*)")
PATH = re.compile(r"p4(?:\.[a-z_][a-z0-9_]*)*")
COVER = re.compile(r"p4(?:\.[a-z_][a-z0-9_]*)+|(?:event|operation):[a-z0-9_-]+|section:[a-z0-9_-]+(?:\.[a-z0-9_-]+)*")
FIELD = re.compile(r"([a-z_][a-z0-9_]*)\s*:")
OPEN = re.compile(r"( *)([a-z_][a-z0-9_]*): \{\s*$")
INDENT = "    "


class CapabilityError(Exception):
    """A fragment that cannot be registered or composed: the message says which and how to fix it."""


@dataclass
class Fragment:
    key: str
    revision: int
    path: Path
    summary: str = ""
    top: list[str] = field(default_factory=list)
    fields: dict[str, list[str]] = field(default_factory=dict)
    overloads: dict[str, list[list[str]]] = field(default_factory=dict)
    examples: list[str] = field(default_factory=list)
    refusals: list[str] = field(default_factory=list)
    covers: list[str] = field(default_factory=list)

    @property
    def name(self) -> str:
        return f"{self.key}@{self.revision}"


def parse_key(text: str) -> tuple[str, int]:
    """`key` or `key@revision` (revision >= 1, default 1), as mod.ini `requires=` and p4.has accept."""
    key, _, revision = text.strip().partition("@")
    if not KEY.fullmatch(key) or (revision and not re.fullmatch(r"[1-9][0-9]*", revision)):
        raise CapabilityError(f'"{text}" is not a capability key: expected name or name@revision, e.g. "events.ailment@1"')
    return key, int(revision or 1)


def load_fragment(path: Path) -> Fragment:
    lines = path.read_text(encoding="utf-8").splitlines()
    match = HEADER.fullmatch(lines[0]) if lines else None
    if not match:
        raise CapabilityError(f"{path.name}:1: first line must be `--!capability <key>@<revision>`")
    key = match.group(1)
    if path.name != key + ".luau":
        raise CapabilityError(f"{path.name}:1: declares {key}; rename the file to {key}.luau (one file per key)")
    fragment = Fragment(key, int(match.group(2)), path)
    block, target, member = "top", None, None
    for number, line in enumerate(lines[1:], 2):
        if line.startswith("--!"):
            word, _, argument = line[3:].partition(" ")
            argument = argument.strip()
            if word in ("in", "overload"):
                if not PATH.fullmatch(argument) or (word == "overload" and argument == "p4"):
                    raise CapabilityError(f"{path.name}:{number}: --!{word} needs a p4.<path>, got {argument!r}")
                block, target = word, argument
                if word == "in":
                    fragment.fields.setdefault(target, [])
                else:
                    member = []
                    fragment.overloads.setdefault(target, []).append(member)
            elif word in ("example", "refuse") and not argument:
                block = word
            elif word == "covers" and argument:
                for item in argument.split():
                    if not COVER.fullmatch(item):
                        raise CapabilityError(f"{path.name}:{number}: --!covers item {item!r} is not p4.<path>, "
                                              "event:<name>, operation:<name> or section:<name>")
                    fragment.covers.append(item)
                block = "top"
            else:
                raise CapabilityError(f"{path.name}:{number}: unknown directive {line!r}")
            continue
        if block == "top":
            if not fragment.summary and not fragment.top and line.startswith("-- "):
                fragment.summary = line[3:].strip()
            fragment.top.append(line)
        elif block == "in":
            fragment.fields[target].append(line)
        elif block == "overload":
            member.append(line)
        elif block == "example":
            fragment.examples.append(line)
        elif line.strip():
            fragment.refusals.append(line)
    for target, members in fragment.overloads.items():
        if any(not "".join(m).strip() for m in members):
            raise CapabilityError(f"{path.name}: --!overload {target} has no member type")
    while fragment.top and not fragment.top[-1].strip():
        fragment.top.pop()
    return fragment


def load(directory: Path = FRAGMENTS) -> list[Fragment]:
    fragments = [load_fragment(path) for path in sorted(directory.glob("*.luau"))]
    return sorted(fragments, key=lambda f: f.key)


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def _field_names(lines: list[str]) -> list[str]:
    base = min((_indent(line) for line in lines if line.strip() and not line.lstrip().startswith("--")), default=0)
    names = []
    for line in lines:
        if _indent(line) == base and not line.lstrip().startswith("--"):
            match = FIELD.match(line.strip())
            if match:
                names.append(match.group(1))
    return names


def compose(base: str, fragments: list[Fragment]) -> str:
    """The base declarations with every fragment spliced in, in key order."""
    lines = base.splitlines()
    try:
        start = lines.index("declare p4: {")
        close = lines.index("}", start)
    except ValueError:
        raise CapabilityError("p4.d.luau: no `declare p4: {` ... `}` block") from None
    # Namespace tables of the base: path -> (open line, close line, field indentation).
    blocks = {"p4": (start, close, INDENT)}
    stack = [("p4", start, INDENT)]
    for index in range(start + 1, close):
        line = lines[index]
        opened = OPEN.match(line)
        if opened and opened.group(1) == stack[-1][2]:
            stack.append((stack[-1][0] + "." + opened.group(2), index, opened.group(1) + INDENT))
        elif len(stack) > 1 and line.strip() in ("}", "},") and line[: _indent(line)] + INDENT == stack[-1][2]:
            path, opened_at, inner = stack.pop()
            blocks[path] = (opened_at, index, inner)
    if len(stack) != 1:
        raise CapabilityError(f"p4.d.luau: namespace {stack[-1][0]} is not closed")
    existing = {path: _field_names(lines[o + 1:c]) for path, (o, c, _) in blocks.items()}

    appended: dict[str, list[str]] = {}
    created: dict[str, list[str]] = {}
    overloads: dict[int, list[str]] = {}
    for fragment in fragments:
        for target, body in fragment.fields.items():
            names = _field_names(body)
            parts = target.split(".")
            for depth in range(2, len(parts) + 1):
                path = ".".join(parts[:depth])
                if path in blocks or path in created:
                    continue
                parent = ".".join(parts[: depth - 1])
                if parts[depth - 1] in existing.get(parent, []):
                    raise CapabilityError(f"{fragment.path.name}: {path} is not a multi-line namespace table in p4.d.luau")
                created[path] = []
                existing.setdefault(parent, []).append(parts[depth - 1])
                existing[path] = []
            for name in names:
                if name in existing[target]:
                    raise CapabilityError(f"{fragment.path.name}: {target}.{name} is already declared")
                existing[target].append(name)
            text = list(body)
            while text and not text[-1].strip():
                text.pop()
            (appended.setdefault(target, []) if target in blocks else created[target]).extend(text)
        for target, members in fragment.overloads.items():
            parent, _, name = target.rpartition(".")
            if parent not in blocks:
                raise CapabilityError(f"{fragment.path.name}: --!overload {target}: {parent} is not a namespace in p4.d.luau")
            opened_at, closed_at, inner = blocks[parent]
            begin = next((i for i in range(opened_at + 1, closed_at)
                          if lines[i].startswith(inner + name + ":")), None)
            if begin is None:
                raise CapabilityError(f"{fragment.path.name}: --!overload {target}: no such field in p4.d.luau")
            end = begin
            while end + 1 < closed_at and (not lines[end + 1].strip() or _indent(lines[end + 1]) > len(inner)):
                end += 1
            if not lines[begin].split(":", 1)[1].strip().startswith("(") or not lines[end].rstrip().endswith("),"):
                raise CapabilityError(f"{fragment.path.name}: --!overload {target} is not a parenthesized intersection")
            for member in members:
                body = [line for line in member if line.strip()]
                rendered = [inner + INDENT + "& (" + body[0].strip()]
                rendered += [inner + INDENT * 2 + line for line in body[1:]]
                rendered[-1] += ")"
                overloads.setdefault(end, []).extend(rendered)

    def indented(body: list[str], inner: str) -> list[str]:
        return [inner + line if line.strip() else "" for line in body]

    def emit_created(parent: str, inner: str) -> list[str]:
        out = []
        for path in sorted(p for p in created if p.rpartition(".")[0] == parent):
            out.append(inner + path.rpartition(".")[2] + ": {")
            out += indented(created[path], inner + INDENT)
            out += emit_created(path, inner + INDENT)
            out.append(inner + "},")
        return out

    closes = {c: path for path, (_, c, _) in blocks.items()}
    out = []
    for index, line in enumerate(lines):
        if index in closes:
            path = closes[index]
            inner = blocks[path][2]
            out += indented(appended.get(path, []), inner)
            out += emit_created(path, inner)
        if index == start:
            for fragment in fragments:
                if any(line.strip() for line in fragment.top if not line.startswith("--")):
                    out.append(f"-- {fragment.name}")
                    out += fragment.top
                    out.append("")
        if index in overloads:
            out.append(line.rstrip()[:-1])
            out += overloads[index]
            out[-1] += ","
        else:
            out.append(line)
    return "\n".join(out) + "\n"


def surfaces(fragment: Fragment) -> list[str]:
    """What a mod can use through this key: its --!covers items, the fields it declares and the
    event names / authorize operations its overloads add."""
    found = list(fragment.covers)
    for target, body in fragment.fields.items():
        found += [f"{target}.{name}" for name in _field_names(body)]
    for target, members in fragment.overloads.items():
        literal = {"p4.events.on": ("name", "event"), "p4.authorize.on": ("operation", "operation")}.get(target)
        for member in members:
            match = literal and re.search(literal[0] + r'\s*:\s*"([a-z0-9_-]+)"', " ".join(member))
            found.append(f"{literal[1]}:{match.group(1)}" if match else target)
    return found


def index(fragments: list[Fragment]) -> str:
    return "".join(f"{f.key} {f.revision} {' '.join(surfaces(f))}".rstrip() + "\n" for f in fragments)



def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(prog="p4_capabilities", description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("list", help="print key@revision per registered capability")
    sub.add_parser("check", help="validate that every fragment parses and composes")
    out = sub.add_parser("compose", help="write the composed p4.d.luau")
    out.add_argument("output", type=Path)
    out = sub.add_parser("index", help="write the surfaces of each key, one key per line")
    out.add_argument("output", type=Path)
    args = parser.parse_args(argv)
    try:
        fragments = load()
        composed = compose(BASE.read_text(encoding="utf-8"), fragments)
    except CapabilityError as error:
        print(f"p4_capabilities: error: {error}", file=sys.stderr)
        return 1
    if args.command == "list":
        for fragment in fragments:
            print(fragment.name)
    elif args.command == "compose":
        args.output.write_bytes(composed.encode("utf-8"))
    elif args.command == "index":
        args.output.write_bytes(index(fragments).encode("utf-8"))
    else:
        print(f"p4_capabilities: {len(fragments)} capabilities compose")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
