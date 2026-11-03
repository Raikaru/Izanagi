#!/usr/bin/env python3
"""Build-input generator for the Persona 4 PC port.

units: write BUILD/units.txt (one `<mirror path>|<flags>` line per compiled
    unit) and BUILD/excluded.txt. Cheap; runs at CMake configure time.
mirror: copy the decomp's src/ and include/ into BUILD/decomp, splice
    overrides/ into those copies. Runs as a build-time custom command.
tables: once the unit objects exist, write BUILD/p4pc_tables.c: the
    function-pointer patch table, trap stubs for unresolved functions,
    absolute definitions for data symbols, and p4pc_game_main. The retail
    pointer words come from config/retail_fnptr_words.txt; --retail is optional.
fnptr-words: write config/retail_fnptr_words.txt from the retail executable.
"""

import argparse
import contextlib
import io
import difflib
import hashlib
import json
import re
import os
import shutil
import struct
import subprocess
import sys
import tarfile
from pathlib import Path, PurePosixPath
import build_cache

ROOT = Path(__file__).resolve().parent.parent

# The retail game's global pointer ($gp, set by `_start`). Names of the form
# `iGpXXXXXXXX` / `fGpXXXXXXXX` address data at GP_VALUE + signed low 16 bits.
GP_VALUE = 0x007690F0
# Function-pointer tables in the retail image (host_build_census.md).
DATA_RANGES = [(0x0052DA00, 0x0070C850), (0x0070E140, 0x00764280)]
# Where a function-pointer target can lie: the game's text and the SDK code blob between the two data ranges.
CODE_RANGES = [(0x00100000, 0x0052DA00), (0x0070C850, 0x0070E140)]
# Every word of DATA_RANGES that holds a code address, with the retail ELF's SHA-256 and image bounds
# (`gen_port.py fnptr-words` writes it from the ELF): the build reads this file, so it needs no retail input.
FNPTR_WORDS = ROOT / "config" / "retail_fnptr_words.txt"

SYMBOL_RE = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]{8});\s*//\s*type:(\w+)")
MARKER_RE = re.compile(r"^[ \t]*//[ \t]*FUN_([0-9A-Fa-f]{8})\b", re.M)
DATA_NAME_RE = re.compile(r"^(?:D|DAT|PTR_DAT|jtbl)_([0-9A-Fa-f]{8})(?:_abs)?$")
FUNC_NAME_RE = re.compile(r"^(?:func|FUN)_([0-9A-Fa-f]{8})$")
GP_NAME_RE = re.compile(r"^(?:[if]Gp|gp0x)([0-9A-Fa-f]{8})$")
OVERRIDE_RE = re.compile(r"^([0-9a-f]{8})\.c$")
# Any `FUN_<ADDR8>` token; the excluded-unit check matches these exactly as a substring test would.
FUN_TOKEN_RE = re.compile(r"FUN_[0-9A-F]{8}")
IDENT_RE = re.compile(r"([A-Za-z_]\w*)\s*$")

# MWCC-only options with no clang equivalent. The rest of the decomp's flags pass through.
DROP_FLAGS = {"-xc", "--target=mipsel-none-elf", "-ffreestanding", "-nostdinc",
              "-nosyspath", "-inline", "auto"}


def die(msg):
    sys.exit(f"gen_port: error: {msg}")

def write_if_changed(path, text):
    """Write `text` (utf-8, surrogateescape) only when the file's bytes differ. Unchanged outputs keep
    their mtime, so make does not rebuild what depends on them."""
    data = text.encode("utf-8", errors="surrogateescape")
    if path.is_file() and path.read_bytes() == data:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return True

def publish_mirror(stage, dest):
    """Make dest's src/ and include/ equal stage's. Files whose bytes already match keep their mtime;
    files the stage no longer has are removed."""
    keep = set()
    for sub in ("src", "include"):
        for f in sorted((stage / sub).rglob("*")):
            if not f.is_file():
                continue
            rel = f.relative_to(stage)
            keep.add(rel)
            write_if_changed(dest / rel, f.read_bytes().decode("utf-8", errors="surrogateescape"))
    for sub in ("src", "include"):
        for f in sorted((dest / sub).rglob("*")):
            if f.is_file() and f.relative_to(dest) not in keep:
                f.unlink()

def read_host_allow():
    """config/host_symbols_allow.txt: `name reason` lines, the host library functions the decomp units may call."""
    names = {}
    for ln in (ROOT / "config/host_symbols_allow.txt").read_text(encoding="utf-8").splitlines():
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        parts = ln.split(None, 1)
        if len(parts) < 2:
            die(f"config/host_symbols_allow.txt: `{ln}` has no reason")
        names[parts[0]] = parts[1]
    return names


def read_libc_renames():
    """Names that include/p4pc/libc_rename.h renames for decomp units (`#define NAME TARGET`)."""
    path = ROOT / "include/p4pc/libc_rename.h"
    names = {}
    text = _LITERAL_RE.sub(
        lambda m: re.sub(r"[^\n]", " ", m[0]) if m[0].startswith(("//", "/*")) else m[0],
        path.read_text(encoding="utf-8"))
    for n, line in enumerate(text.splitlines(), 1):
        match = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)(.*)$", line)
        if not match or match[2].startswith("(") or not match[2].strip():
            continue  # function-like helpers, include guard and empty SIMD declarations
        name, target = match[1], match[2].strip()
        if not re.fullmatch(r"[A-Za-z_]\w*", target):
            die(f"{path}:{n}: expected one native identifier for libc alias {name}, got {target!r}")
        if name in names:
            die(f"{path}:{n}: libc alias {name} is defined twice")
        names[name] = target
    if not names:
        die(f"{path} has no renames")
    return names


# ---------------------------------------------------------------- C text scanning

def _skip_token(text, i):
    """If a comment or literal starts at i, return the index just past it."""
    if text.startswith("//", i):
        j = text.find("\n", i)
        return len(text) if j < 0 else j
    if text.startswith("/*", i):
        j = text.find("*/", i + 2)
        return len(text) if j < 0 else j + 2
    if text[i] in "\"'":
        quote, j = text[i], i + 1
        while j < len(text) and text[j] != quote and text[j] != "\n":
            j += 2 if text[j] == "\\" else 1
        return j + 1
    return None


def _match_brace(text, i):
    """text[i] is '{'. Return the index just past its matching '}'."""
    depth = 0
    while i < len(text):
        nxt = _skip_token(text, i)
        if nxt is not None:
            i = nxt
            continue
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    die("unbalanced braces in decomp source")


def definition_end(text, start):
    """Index just past the closing brace of the function definition whose
    signature starts at `start`, or None if the declaration ends with ';'.
    A K&R definition (`f(a)` then `u8 *a;` lines, then the body) has `;` after its parameter list
    that is not a declaration end: only a `;` right after the closing `)` ends a declaration."""
    depth, i, close = 0, start, start
    while i < len(text):
        nxt = _skip_token(text, i)
        if nxt is not None:
            i = nxt
            continue
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                close = i + 1
        elif depth == 0 and c == ";":
            if not text[close:i].strip():
                return None
        elif depth == 0 and c == "{":
            return _match_brace(text, i)
        i += 1
    return None


def line_end(text, idx):
    """Index of the first character of the line after the one containing idx."""
    j = text.find("\n", idx)
    return len(text) if j < 0 else j + 1


def marker_definition_name(text, marker_end):
    """Name of the function a `// FUN_` marker introduces, or None when the
    next definition is an INCLUDE_ASM (no native body)."""
    lines = text[line_end(text, marker_end):].split("\n", 80)
    in_block = False
    for line in lines:
        s = line.strip()
        if in_block:
            in_block = "*/" not in s
            continue
        if s.startswith("/*"):
            if "*/" in s:
                s = s.split("*/", 1)[1].strip()
            else:
                in_block = True
                continue
        if MARKER_RE.match(line):
            return None  # an absent/removed body must not steal the next function
        if not s or s.startswith(("#", "//", "*")):
            continue
        paren, semi = s.find("("), s.find(";")
        if paren < 0 and not any(c in s for c in ";={}"):
            continue  # return type on its own line (`RwCamera *`), name on the next
        if paren < 0 or (0 <= semi < paren):
            return None
        if "INCLUDE_ASM" in s[:paren]:
            return None
        if s.endswith(";") and "{" not in s:
            continue  # a prototype (an override keeps single-line extern declarations between the marker and its body)
        pointer_return = re.search(r"\([ \t]*\*+[ \t]*(\w+)[ \t]*\(", s)
        if pointer_return:
            return pointer_return[1]
        m = IDENT_RE.search(s[:paren])
        return m.group(1) if m else None
    return None


def marker_body_name(text, marker_end, addr, syms):
    """marker_definition_name, but a name the symbol table places at another address is not
    this marker's body. The lax scan takes the next code line, which can be a prototype or
    the next function when this marker's own body is absent (an INCLUDE_ASM under #ifdef)."""
    name = marker_definition_name(text, marker_end)
    if name and name in syms and syms[name][0] != addr:
        return None
    return name


# ---------------------------------------------------------------- inputs

def read_symbols(decomp):
    syms = {}
    for name in ("config/symbol_addrs.txt", "config/symbol_data_addrs.txt", "config/symbols_recovered.txt"):
        for line in (decomp / name).read_text().splitlines():
            m = SYMBOL_RE.match(line)
            if m:
                syms.setdefault(m[1], (int(m[2], 16), m[3]))
    return syms


def read_list(path):
    out = []
    for line in path.read_text().splitlines():
        s = line.strip()
        if s and not s.startswith("#"):
            out.append(s)
    return out


def read_units(root):
    excluded = {}
    for line in read_list(root / "config/excluded_units.txt"):
        parts = line.split(None, 1)
        excluded[parts[0]] = parts[1] if len(parts) > 1 else "no reason given"
    inactive = read_inactive_movie(root)
    excluded.update((unit, inactive["reason"]) for unit in inactive["units"])
    units = [u for u in read_list(root / "config/units.txt") if u not in excluded]
    return units, excluded


def read_inactive_movie(root=None):
    root = ROOT if root is None else root
    return json.loads((root / "config/inactive_movie_decoder.json").read_text(encoding="utf-8"))


def remove_inactive_movie(mirror):
    """Remove the dependency-closed software decoder, not substitute success stubs."""
    policy = read_inactive_movie()
    removed = [0, 0, 0]
    for unit in policy["units"]:
        (mirror / unit).unlink()
        removed[0] += 1
    for unit in sorted(set(policy["functions"]) | set(policy["registrations"]) | set(policy["markers"])):
        path = mirror / unit
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for name in policy["functions"].get(unit, []):
            pattern = re.compile(r"^[ \t]*[A-Za-z_][\w \t*]*?\b" + re.escape(name) + r"\s*\(", re.M)
            spans = []
            for match in pattern.finditer(text):
                close = definition_end(text, match.start())
                if close is not None:
                    spans.append((match.start(), line_end(text, close)))
            if len(spans) != 1:
                die(f"inactive movie {unit}:{name}: expected one definition, found {len(spans)}")
            start, end = spans[0]
            text = text[:start] + text[end:]
            removed[1] += 1
        for name in policy["registrations"].get(unit, []):
            pattern = re.compile(r"^[ \t]*[A-Za-z_][\w \t*]*?\b" + re.escape(name)
                                 + r"[^;{}()]*=\s*\{", re.M)
            matches = list(pattern.finditer(text))
            if len(matches) != 1:
                die(f"inactive movie {unit}:{name}: expected one initializer, found {len(matches)}")
            match = matches[0]
            end = _match_brace(text, match.end() - 1)
            tail = re.match(r"\s*;", text[end:])
            if tail is None:
                die(f"inactive movie {unit}:{name}: initializer has no terminating semicolon")
            end += tail.end()
            text = text[:match.start()] + text[line_end(text, end):]
            removed[2] += 1
        for address in policy["markers"].get(unit, []):
            matches = [match for match in MARKER_RE.finditer(text) if int(match[1], 16) == int(address, 16)]
            if len(matches) != 1:
                die(f"inactive movie {unit}: expected one FUN_{address} marker, found {len(matches)}")
            match = matches[0]
            start = text.rfind("\n", 0, match.start()) + 1
            text = text[:start] + text[line_end(text, match.end()):]
        path.write_text(text, encoding="utf-8", errors="surrogateescape", newline="\n")
    return removed


def read_port_symbols():
    """config/port_symbols.txt (generated by tools/retail_symbols.py) and config/port_symbols_manual.txt
    (hand-placed, with evidence): placements that the decomp's symbol lists do not give.
    Lines are `name 0xADDR data|func`; `#` starts a comment."""
    out = {}
    for fname in ("port_symbols.txt", "port_symbols_manual.txt"):
        path = ROOT / "config" / fname
        for n, raw in enumerate(path.read_text().splitlines(), 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) != 3 or not re.fullmatch(r"0x[0-9A-Fa-f]{1,8}", parts[1]) or parts[2] not in ("data", "func"):
                die(f"{path}:{n}: expected `name 0xADDR data|func`, got {raw.strip()!r}")
            if parts[0] in out:
                die(f"{path}:{n}: {parts[0]} is placed twice")
            out[parts[0]] = (int(parts[1], 16), parts[2])
    return out


def read_native_names():
    """config/native_names.txt: retail functions that a compiled decomp function implements under
    another name. Lines are `0xADDR name`; `#` starts a comment (the evidence)."""
    path = ROOT / "config" / "native_names.txt"
    out = {}
    if not path.is_file():
        return out
    for n, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2 or not re.fullmatch(r"0x[0-9A-Fa-f]{1,8}", parts[0]):
            die(f"{path}:{n}: expected `0xADDR name`, got {raw.strip()!r}")
        if int(parts[0], 16) in out:
            die(f"{path}:{n}: {parts[0]} is named twice")
        out[int(parts[0], 16)] = parts[1]
    return out


def read_unbound_natives():
    """config/unbound_natives.txt: compiled functions that a marker or symbol places at a retail
    address but that are not the retail function there (an RE4 body linked under a retail name, or
    a marker of another function). Lines are `0xADDR name  # reason`. The address stops resolving to
    `name`: a port translation named func_<addr8> takes it, otherwise it is a trap, so a call that
    reaches it is loud instead of silently running the wrong body. The name itself still links by
    name for the decomp's own C callers."""
    path = ROOT / "config" / "unbound_natives.txt"
    out = {}
    if not path.is_file():
        return out
    for n, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2 or not re.fullmatch(r"0x[0-9A-Fa-f]{1,8}", parts[0]):
            die(f"{path}:{n}: expected `0xADDR name`, got {raw.strip()!r}")
        if int(parts[0], 16) in out:
            die(f"{path}:{n}: {parts[0]} is listed twice")
        out[int(parts[0], 16)] = parts[1]
    return out

def check_addressless_traps(traps):
    """A missing private helper must not silently become a zero-returning trap at address 0."""
    path = ROOT / "config/call_traps.txt"
    expected = {}
    for n, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        name = raw.split("#", 1)[0].strip()
        if not name:
            continue
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            die(f"{path}:{n}: expected one C function name")
        if name in expected:
            die(f"{path}:{n}: {name} is listed twice")
        expected[name] = n
    actual = {name for name, addr in traps.items() if addr == 0}
    errors = [f"{path}:{expected[name]}: {name} is no longer an addressless call trap"
              for name in sorted(expected.keys() - actual)]
    errors += [f"{path}: unexpected addressless call {name}; check for a renamed or removed decomp helper"
               for name in sorted(actual - expected.keys())]
    if errors:
        die("addressless call target drift:\n  " + "\n  ".join(errors))



# MWCC's 128-bit types (`__int128`, `mode(TI)`) become one 16-byte, 16-aligned
# struct from include/p4pc/compat.h. Plain copies and struct members keep their
# size and alignment; arithmetic or casts on it fail to compile, which marks
# the functions that need an override.
Q128_RE = re.compile(r"\b(?:(?:signed|unsigned)\s+)?__int128\b")
TI_TYPEDEF_RE = re.compile(r"typedef\s+unsigned\s+int\s+(\w+)\s+__attribute__\(\(mode\(TI\)\)\);")


# The RenderWare globals block is retail data at its retail address: config/port_symbols.txt
# places `ourGlobals`, and game code reads it there. The RWGLOBALSIZE branch defines it as a host
# array instead, which gives RenderWare a copy the game never sees. The definition becomes an
# extern declaration (baplcore.c and badevice.c carry it).
RW_GLOBALS_DEF_RE = re.compile(r"^(\s*)RwUInt32\s+ourGlobals\[RWGLOBALSIZE\s*/\s*sizeof\(RwUInt32\)\];", re.M)


# (A function returning a 128-bit value returns void on the host: void_quad_returns, after the override splice.)


def port_source(text):
    text = TI_TYPEDEF_RE.sub(r"typedef P4_Q128 \1;", text)
    text = RW_GLOBALS_DEF_RE.sub(r"\1extern RwUInt32 ourGlobals[RWGLOBALSIZE / sizeof(RwUInt32)];  /* retail address: config/port_symbols.txt */", text)
    return Q128_RE.sub("P4_Q128", text)


def mirror_tree(decomp, dest):
    """Copy the decomp's src/ and include/ trees, porting C sources and headers
    (see port_source). A destination file is rewritten only when its content
    differs from the port of its source, which also undoes any earlier splice."""
    for sub in ("src", "include"):
        for f in sorted((decomp / sub).rglob("*")):
            if not f.is_file():
                continue
            dst = dest / f.relative_to(decomp)
            data = f.read_bytes()
            if f.suffix in (".c", ".h"):
                text = data.decode("utf-8", errors="surrogateescape")
                data = port_source(text).encode("utf-8", errors="surrogateescape")
            if dst.is_file() and dst.read_bytes() == data:
                continue
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(data)


# A decomp unit often re-declares a function it already gets from a header, with
# placeholder types (`s32 arg0`, `u8 *`). MWCC accepts the second declaration;
# clang rejects it as "conflicting types". A bare prototype whose name the unit
# already declares (in an included header, or earlier in the same file) is
# dropped, so the first declaration (normally the header's named one) governs
# every call. Definitions and multi-line prototypes are left alone.
PROTO_LINE_RE = re.compile(
    r"^[ \t]*(?:(?:extern|static)[ \t]+)?(?!(?:return|else|case|goto|sizeof|typedef)\b)"
    r"[A-Za-z_][\w \t\*]*?[ \t\*]\s*(?!__)(?!RWALIGN\b)(?!(?:volatile|asm)\b)([A-Za-z_]\w*)(?!\s*\(\s*\*)\s*\([^;{}=]*\)\s*;[ \t]*$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
# The same clash one scope down: an `extern` prototype inside a function body that
# disagrees with the file-scope declaration or definition of the same name. The
# file-scope type governs, so the block-scope line is dropped.
BLOCK_PROTO_RE = re.compile(
    r"^[ \t]+extern[ \t]+(?!(?:return|else|case|goto|sizeof|typedef)\b)"
    r"[A-Za-z_][\w \t\*]*?[ \t\*]\s*(?!__)(?!RWALIGN\b)(?!(?:volatile|asm)\b)([A-Za-z_]\w*)(?!\s*\(\s*\*)\s*\([^;{}=]*\)\s*;[ \t]*$")
# RWALIGN(name, 16) is a declarator macro (`Vec3 RWALIGN(v0, 16);` declares a member), not a function,
# and `__asm__ volatile("...")` is a statement: neither may be taken for a prototype and dropped.
# A column-0 function declaration or definition header.
FILE_NAME_RE = re.compile(r"^[A-Za-z_][\w \t\*]*?[ \t\*]\s*([A-Za-z_]\w*)\s*\(")


def _prototype_names(text):
    names, prev = set(), ""
    for line in text.split("\n"):
        m = PROTO_LINE_RE.match(line) if ";" in line else None
        if m and line.count("(") == line.count(")") and _starts_statement(prev):
            names.add(m.group(1))
        if line.strip():
            prev = line.strip()
    return names


def _header_prototype_names(path, mirror, seen):
    if path in seen:
        return set()
    seen.add(path)
    text = path.read_text(encoding="utf-8", errors="surrogateescape")
    names = _prototype_names(text)
    for inc in INCLUDE_RE.findall(text):
        for cand in (path.parent / inc, mirror / inc, mirror / "include" / inc, mirror / "src" / inc):
            if cand.is_file():
                names |= _header_prototype_names(cand, mirror, seen)
                break
    return names


def _starts_statement(prev):
    return not prev or prev[-1] in ";{}" or prev.startswith(("#", "//", "/*", "*")) or prev.endswith("*/")


def drop_redundant_prototypes(mirror):
    dropped, block_dropped, files = 0, 0, 0
    header_names = {}
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        known = set()
        for inc in INCLUDE_RE.findall(text):
            for cand in (path.parent / inc, mirror / inc, mirror / "include" / inc, mirror / "src" / inc):
                if cand.is_file():
                    if cand not in header_names:
                        header_names[cand] = _header_prototype_names(cand, mirror, set())
                    known |= header_names[cand]
                    break
        lines = text.split("\n")
        # File-scope names declared or defined above the current line, or in an included
        # header. A block-scope extern is dropped only when its file-scope declaration
        # came first. If the file-scope declaration comes later, the earlier uses are
        # implicitly declared and the block-scope line must stay.
        seen = set(known)
        out = []
        prev = ""
        for line in lines:
            # Only a balanced whole statement can be a prototype: continuation lines
            # of a multi-line call also look like `type name(args);`.
            whole = line.count("(") == line.count(")") and _starts_statement(prev)
            # Block-scope checked first: PROTO_LINE_RE also matches indented lines.
            b = BLOCK_PROTO_RE.match(line) if whole and line[:1] in (" ", "\t") and "extern" in line else None
            if b and b.group(1) in seen:
                block_dropped += 1
                continue
            m = PROTO_LINE_RE.match(line) if ";" in line else None
            if m and whole:
                if m.group(1) in known:
                    dropped += 1
                    continue
                known.add(m.group(1))
            out.append(line)
            fm = FILE_NAME_RE.match(line) if "(" in line else None
            if fm:
                seen.add(fm.group(1))
            if line.strip():
                prev = line.strip()
        new = "\n".join(out)
        if new != text:
            files += 1
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
    return dropped, block_dropped, files


# Prototypes the later passes match line by line are normalized first: a prototype that wraps
# its parameter list is joined onto one line, and a trailing `/* ... */` comment is dropped.
# Only statements shaped like a declaration are touched (`type name(...);`, no `=`).
_DECL_START_RE = re.compile(
    r"^[ \t]*(?:(?:extern|static)[ \t]+)?(?!(?:return|else|case|goto|sizeof|typedef|if|while|for|switch)\b)"
    r"[A-Za-z_][\w \t\*]*?[ \t\*]\s*(?!__)(?!(?:if|while|for|switch|return|sizeof)\b)[A-Za-z_]\w*\s*\(")
_TRAILING_COMMENT_RE = re.compile(r"[ \t]*/\*[^*]*(?:\*(?!/)[^*]*)*\*/[ \t]*$")


def normalize_prototype_lines(mirror):
    joined, files = 0, 0
    for pattern in ("*.c", "*.h"):
        for path in sorted(mirror.rglob(pattern)):
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            lines = text.split("\n")
            out, i, prev, changed = [], 0, "", False
            while i < len(lines):
                line = lines[i]
                if "(" in line and _DECL_START_RE.match(line) and _starts_statement(prev):
                    stmt, j = line, i
                    bad = False
                    while stmt.count("(") > stmt.count(")") and j + 1 < len(lines) and j - i < 8:
                        j += 1
                        stmt += " " + lines[j].strip()
                        if "{" in lines[j] or ";" in lines[j][:-1]:
                            bad = True
                    if bad or "//" in stmt or stmt.count("(") != stmt.count(")") or "{" in stmt or "=" in stmt.split("(")[0]:
                        stmt, j = line, i
                    tail = _TRAILING_COMMENT_RE.sub("", stmt) if stmt.rstrip().endswith("*/") else stmt
                    if tail.rstrip().endswith(";") and "/*" not in tail:
                        if j > i:
                            joined += 1
                        if tail != lines[i] or j > i:
                            changed = True
                        out.append(tail)
                        prev = tail.strip()
                        i = j + 1
                        continue
                out.append(line)
                if line.strip():
                    prev = line.strip()
                i += 1
            if changed:
                files += 1
                path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    return joined, files


def _split_params(args):
    params, depth, cur = [], 0, ""
    for ch in args:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            params.append(cur)
            cur = ""
        else:
            cur += ch
    params.append(cur)
    return [p.strip() for p in params if p.strip()]


_TYPE_WORDS = {"unsigned", "signed", "int", "long", "short", "char", "float", "double", "const",
               "struct", "enum", "union", "volatile", "void"}


def _param_type(param):
    """The parameter's type text without its name."""
    # `struct X *` and its typedef `X *` are the same type, so the tag is not part of the comparison.
    param = re.sub(r"\b(?:struct|enum|union)\s+", "", re.sub(r"\s+", " ", param.strip()))
    if param == "..." or "(" in param:
        return param.replace(" ", "")
    param = re.sub(r"\s*\[[^\]]*\]$", "*", param)
    m = re.match(r"^(.*?[\s\*])([A-Za-z_]\w*)$", param)
    if m and m.group(2) not in _TYPE_WORDS:
        head = m.group(1).strip()
        words = [w for w in re.split(r"[\s\*]+", head) if w and w != "const"]
        if "*" in head or len(words) >= 2 or (words and words[-1] in _TYPE_WORDS and words[0] in _TYPE_WORDS):
            param = head
        elif words and words[0] in ("struct", "enum", "union"):
            param = head
    return param.replace(" ", "")


def _signature(text):
    """(return type, [param types] or None for `()`) of `ret name(params)`; text has no trailing `;`."""
    m = re.match(r"^\s*(?:(?:extern|static|inline)\s+)*(?P<ret>[A-Za-z_][\w \t\*]*?)[ \t\*]*(?P<name>[A-Za-z_]\w*)\s*\((?P<args>.*)\)\s*$", text, re.S)
    if not m:
        return None
    ret = re.sub(r"\s+", "", re.sub(r"\b(?:struct|enum|union)\s+", "", m.group("ret")))
    star = "*" * text[:m.start("name")].count("*")
    args = _split_params(m.group("args"))
    if not args:
        return m.group("name"), ret + star, None
    if args == ["void"]:
        return m.group("name"), ret + star, []
    return m.group("name"), ret + star, [_param_type(a) for a in args]


def _definitions(lines):
    """name -> (line index, signature text) for column-0 non-static function definitions."""
    defs = {}
    for i, line in enumerate(lines):
        if line[:1] in (" ", "\t", "#", "/", "*", "}", "{", "") or line.startswith(("static", "asm")) or "(" not in line:
            continue
        if not _DECL_START_RE.match(line) or _NOT_A_PROTOTYPE_RE.search(line):
            continue
        text, j = line, i
        while text.count("(") > text.count(")") and j + 1 < len(lines) and j - i < 8:
            j += 1
            text += " " + lines[j].strip()
        if text.count("(") != text.count(")"):
            continue
        k = j + 1 if j + 1 < len(lines) and lines[j + 1].strip() == "{" and "{" not in text else j
        if "{" not in text and not (k == j + 1):
            continue
        sig = text.split("{")[0].strip()
        sig = re.sub(r"/\*.*?\*/", "", sig).strip()
        parsed = _signature(sig)
        if parsed and parsed[0] not in defs:
            defs[parsed[0]] = (i, sig)
    return defs


def reconcile_prototypes_with_definitions(mirror):
    """A prototype of a function the same file defines, whose parameter or return types differ from
    the definition's, is a decomp type disagreement (MWCC accepts it, clang rejects it as
    `conflicting types`). The definition is the authority: the prototype takes its signature."""
    fixed, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        lines = text.split("\n")
        defs = _definitions(lines)
        if not defs:
            continue
        changed = False
        prev = ""
        for i, line in enumerate(lines):
            stmt_start = _starts_statement(prev)
            if line.strip():
                prev = line.strip()
            if not stmt_start or ";" not in line or "(" not in line or not line.rstrip().endswith(";"):
                continue
            m = PROTO_LINE_RE.match(line)
            if not m or line.count("(") != line.count(")") or line.lstrip().startswith("static"):
                continue
            name = m.group(1)
            if name not in defs or defs[name][0] == i:
                continue
            want = _signature(defs[name][1])
            have = _signature(line.strip().rstrip(";"))
            if not want or not have or have[2] is None or (have[1], have[2]) == (want[1], want[2]):
                continue
            indent = line[:len(line) - len(line.lstrip())]
            lines[i] = f"{indent}extern {defs[name][1]};"
            fixed += 1
            changed = True
        if changed:
            files += 1
            path.write_text("\n".join(lines), encoding="utf-8", errors="surrogateescape")
    return fixed, files


# A function used before an `extern` prototype of it appears inside a later function body is
# implicitly declared as `int name()` at the first use, and the later block-scope prototype then
# conflicts with it (`conflicting types`). MWCC does not diagnose that. The prototype is hoisted to
# file scope, immediately above the first function that uses the name (not after the includes), so
# every type that function can see is visible to the prototype. The block-scope copies are then
# dropped by drop_redundant_prototypes, which runs again afterwards.
def _comment_free(line):
    s = line.strip()
    return s and not s.startswith(("//", "/*", "*"))


_NOT_A_PROTOTYPE_RE = re.compile(r"\b(?:__asm__|asm|__asm|volatile|__volatile__)\b")


def _block_prototype(line):
    """An indented function prototype, with or without `extern` (never `static`)."""
    if _NOT_A_PROTOTYPE_RE.search(line):
        return None
    m = BLOCK_PROTO_RE.match(line)
    if m:
        return m
    return None if line.lstrip().startswith("static") else PROTO_LINE_RE.match(line)


def _ends_toplevel_construct(lines, j):
    """Line j (column 0) closes a function, struct or declaration, so a new declaration may follow it.
    A column-0 `;` line is not one when it is a K&R parameter declaration (a `{` line follows the run)."""
    line = lines[j]
    if line[:1] in (" ", "\t", "#", "") or not (line.rstrip().endswith(";") or line.startswith("}")):
        return False
    if line.startswith("}"):
        return True
    k = j + 1
    while k < len(lines) and (not lines[k].strip() or (lines[k][:1] not in (" ", "\t", "#", "{", "}")
                                                          and lines[k].rstrip().endswith(";")
                                                          and "(" not in lines[k])
                              or lines[k].lstrip().startswith(("//", "/*", "*"))):
        k += 1
    return not (k < len(lines) and lines[k].strip() == "{")


def hoist_block_prototypes(mirror):
    hoisted, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        lines = text.split("\n")
        inserts = {}  # line index to insert before -> [declaration lines]
        done = set()
        prev = ""
        for i, line in enumerate(lines):
            whole = line.count("(") == line.count(")") and _starts_statement(prev)
            if line.strip():
                prev = line.strip()
            b = _block_prototype(line) if whole and line[:1] in (" ", "\t") else None
            if not b or b.group(1) in done:
                continue
            name = b.group(1)
            done.add(name)
            use_re = re.compile(r"\b" + re.escape(name) + r"\s*\(")
            use = next((j for j in range(i) if lines[j][:1] in (" ", "\t") and _comment_free(lines[j])
                        and use_re.search(lines[j]) and not _block_prototype(lines[j])), None)
            if use is None:
                continue
            # The enclosing function starts after the previous column-0 `}` or `;` line.
            top = next((j for j in range(use, -1, -1) if _ends_toplevel_construct(lines, j)), -1)
            if top >= 0 and top > i:
                continue
            decl = line.strip()
            inserts.setdefault(top + 1, []).append(decl if decl.startswith("extern") else "extern " + decl)
            hoisted += 1
        if inserts:
            out = []
            for i, line in enumerate(lines):
                out.extend(inserts.get(i, ()))
                out.append(line)
            files += 1
            path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    return hoisted, files

# A `struct Tag` that first appears in a prototype's parameter list is scoped to that prototype
# in standard C, so the function's later definition (using the real struct) conflicts with it.
# MWCC gives the tag file scope. A forward declaration is inserted above the first such prototype.
_STRUCT_TAG_RE = re.compile(r"\bstruct\s+([A-Za-z_]\w*)")


def forward_declare_prototype_structs(mirror):
    inserted, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "struct" not in text:
            continue
        lines = text.split("\n")
        seen, inserts, prev = set(), {}, ""
        for i, line in enumerate(lines):
            stmt_start = _starts_statement(prev)
            if line.strip():
                prev = line.strip()
            m = None
            if stmt_start and ";" in line and "(" in line and not _NOT_A_PROTOTYPE_RE.search(line):
                m = PROTO_LINE_RE.match(line)
            if m and line.count("(") == line.count(")"):
                args = line[line.index("(", m.start(1)):]
                for tag in _STRUCT_TAG_RE.findall(args):
                    if tag in seen:
                        continue
                    seen.add(tag)
                    where = i
                    if line[:1] in (" ", "\t"):
                        top = next((j for j in range(i, -1, -1) if _ends_toplevel_construct(lines, j)), -1)
                        where = top + 1
                    inserts.setdefault(where, []).append(f"struct {tag};")
                    inserted += 1
            for tag in _STRUCT_TAG_RE.findall(line.split("(")[0] if m else line):
                seen.add(tag)
        if inserts:
            out = []
            for i, line in enumerate(lines):
                out.extend(inserts.get(i, ()))
                out.append(line)
            files += 1
            path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    return inserted, files


# Absolute data symbols (D_xxxxxxxx, iGpxxxxxxxx, ...) are declared per use in the decomp, and one
# unit may declare the same symbol with different types in different functions (MWCC does not
# check). clang rejects the redeclarations. The declarations that disagree with the file-scope one
# (or the first one) get a unique C name bound to the same linker symbol with an asm label, and
# the uses from that declaration to the end of its function are renamed.
_DATA_SYMBOL_RE = re.compile(r"\b((?:D|DAT|PTR_DAT|jtbl)_[0-9A-Fa-f]{8}|[if]Gp[0-9a-f]{8}|gp0x[0-9a-f]{8})\b")


def _declared_symbol(line):
    """(name, canonical declaration) of an `extern` data declaration of an absolute symbol, else None."""
    st = line.strip()
    if not st.startswith("extern ") or not st.endswith(";") or "=" in st or len(_split_params(st[:-1])) != 1:
        return None
    names = set(_DATA_SYMBOL_RE.findall(st))
    if len(names) != 1:
        return None
    name = names.pop()
    # A function prototype of that name (`name(`) is not data.
    if re.search(r"\b" + re.escape(name) + r"\s*\(", st):
        return None
    return name, re.sub(r"\s+", "", st.replace(name, "@"))


def rename_conflicting_data_declarations(mirror):
    renamed, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "extern" not in text:
            continue
        lines = text.split("\n")
        decls = {}
        for i, line in enumerate(lines):
            d = _declared_symbol(line)
            if d:
                decls.setdefault(d[0], []).append((i, d[1], line[:1] not in (" ", "\t")))
        changed = False
        for name, entries in decls.items():
            if len({canon for _, canon, _ in entries}) < 2:
                continue
            keeper = next((canon for _, canon, top in entries if top), entries[0][1])
            variants = {}
            for i, canon, top in entries:
                if canon == keeper or top:
                    continue
                n = variants.setdefault(canon, len(variants) + 1)
                alias = f"{name}__p4v{n}"
                end = next((j for j in range(i, len(lines)) if lines[j].startswith("}")), len(lines) - 1)
                decl = _DATA_SYMBOL_RE.sub(lambda m: alias if m.group(1) == name else m.group(0), lines[i])
                lines[i] = decl.rstrip().rstrip(";") + f' __asm__("{name}");'
                for j in range(i + 1, end + 1):
                    if name in lines[j]:
                        lines[j] = re.sub(r"\b" + re.escape(name) + r"\b", alias, lines[j])
                renamed += 1
                changed = True
        if changed:
            files += 1
            path.write_text("\n".join(lines), encoding="utf-8", errors="surrogateescape")
    return renamed, files


# A prototype in a TU whose return type differs from the definition of the same function in
# that TU is a decomp type mismatch. The definition is authoritative, so the prototype's return
# type is rewritten to the definition's. Argument lists and header prototypes are not touched.
PROTO_SIG_RE = re.compile(
    r"^(?P<lead>[ \t]*(?:extern[ \t]+)?)(?P<ret>[A-Za-z_][\w \t\*]*?)[ \t\*]*(?P<name>[A-Za-z_]\w*)"
    r"[ \t]*\((?P<args>[^;{}]*)\)[ \t]*;[ \t]*$")


def _norm_type(text):
    return re.sub(r"\s+", "", re.sub(r"\b(?:static|extern|inline)\b", "", text))


def align_prototype_returns(mirror):
    fixed, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        lines = text.split("\n")
        defs = {}
        for line in lines:
            if line[:1] in (" ", "\t") or line.rstrip().endswith(";"):
                continue
            m = FILE_NAME_RE.match(line) if "(" in line else None
            if not m:
                continue
            ret = re.sub(r"\b(?:static|extern|inline|asm)\b", "", line[:m.start(1)]).strip()
            if ret and m.group(1) not in defs:
                defs[m.group(1)] = ret
        if not defs:
            continue
        out, prev, changed = [], "", False
        for line in lines:
            p = PROTO_SIG_RE.match(line) if ";" in line and "(" in line else None
            if (p and p.group("name") in defs and line.count("(") == line.count(")")
                    and (p.group("lead").strip() or (" " in p.group("ret") or "*" in p.group("ret")))
                    and (p.group("lead").strip() or _starts_statement(prev))):
                want = defs[p.group("name")]
                if _norm_type(p.group("ret")) != _norm_type(want):
                    keep = " ".join(re.findall(r"\b(?:static|inline)\b", p.group("ret")))
                    fmt = (keep + " " if keep else "") + (want if want.endswith("*") else want + " ")
                    line = line[:p.start("ret")] + fmt + line[p.start("name"):]
                    fixed += 1
                    changed = True
            out.append(line)
            if line.strip():
                prev = line.strip()
        if changed:
            files += 1
            path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    return fixed, files




# Promoted code1 units are copies of whole code regions. Some functions in them are also defined by
# a compiled non-promoted unit (same name, same address), and the link fails with multiple definitions.
# A non-static definition is owned by the first compiled unit that defines it (non-promoted units
# first, then promoted units in units.txt order). A promoted definition of an already-owned name is
# dropped from the mirror, from its signature line through the closing col-0 brace.
DEF_START_RE = re.compile(r"^(?:[A-Za-z_][\w \t\*]*[ \t\*])?(?P<name>[A-Za-z_]\w*)[ \t]*\([^;]*$")


FUNC_ALIAS_RE = re.compile(r"^#define\s+([A-Za-z_]\w*)\s+(func_[0-9A-Za-z_]+)\s*$")
# Legacy library spellings may bind to the canonical implementation selected
# by libc_rename.h; they must never introduce an unreviewed host math thunk.
# Prefixes of host-library function names (POSIX threads and semaphores, SDL, FFmpeg, the C library's
# internal and Windows-runtime names). Retail names are never spelled like this.
HOST_LIKE_RE = re.compile(r"^(pthread_|sem_|SDL_|avcodec_|avutil_|av_|swscale_|sws_|_?_imp_|_Unwind_|__cxa_|__gnu_|_?mingw_|__mingw_)")
HOST_ALIASES = {"__ieee754_sqrtf": "sqrtf"}


def _opens_body(lines, i):
    # A signature may wrap its parameter list; the body opens at the first '{' before any ';'.
    for line in lines[i:i + 8]:
        if ";" in line:
            return False
        if "{" in line:
            return True
    return False


def _non_static_definitions(lines):
    found = []
    for i, line in enumerate(lines):
        m = DEF_START_RE.match(line)
        if not m or line.startswith("static") or m.group("name") in ("if", "while", "for", "switch", "return", "sizeof"):
            continue
        # `static RpWorldSector *` on the line above the name (the RenderWare sources' style) makes it static as well
        if i and lines[i - 1].startswith("static") and not lines[i - 1].rstrip().endswith((";", "}")):
            continue
        if not _opens_body(lines, i):
            continue
        found.append((i, m.group("name")))
    return found


# Legacy src/rw units that duplicate one function of the owning renderware/ unit (retail has one copy,
# split into both decomp trees). They are compiled like promoted units: a definition another unit owns
# is dropped, the rest is kept (these hold rows the owner lacks, e.g. RwFrameTransform).
DEDUPE_LEGACY_UNITS = {"src/rw/baframe.c", "src/rw/batextur.c"}


def _is_secondary_unit(u):
    return u.startswith("src/promoted/") or u in DEDUPE_LEGACY_UNITS


def drop_duplicate_promoted_definitions(mirror, units):
    owners = set()
    for u in units:
        if not _is_secondary_unit(u) and (mirror / u).exists():
            lines = (mirror / u).read_text(encoding="utf-8", errors="surrogateescape").split("\n")
            # `#define RpAtomicDestroy func_003c02e0` plus a definition of RpAtomicDestroy: the unit owns the func_ name.
            aliases = {m.group(1): m.group(2) for m in map(FUNC_ALIAS_RE.match, lines) if m}
            names = [name for _, name in _non_static_definitions(lines)]
            owners.update(names)
            owners.update(aliases[n] for n in names if n in aliases)
    dropped, files = 0, 0
    for u in units:
        if not _is_secondary_unit(u) or not (mirror / u).exists():
            continue
        lines = (mirror / u).read_text(encoding="utf-8", errors="surrogateescape").split("\n")
        drop = set()
        for i, name in _non_static_definitions(lines):
            if name not in owners:
                owners.add(name)
                continue
            end = next((j for j in range(i + 1, len(lines)) if lines[j] == "}"), None)
            if end is None:
                raise SystemExit(f"gen_port: {u}: no closing brace for duplicate {name}")
            drop.update(range(i, end + 1))
            dropped += 1
        if drop:
            files += 1
            (mirror / u).write_text("\n".join(l for k, l in enumerate(lines) if k not in drop),
                                    encoding="utf-8", errors="surrogateescape")
    return dropped, files


# Many decomp units define their own `va_list`/`va_start` (an MWCC register-save
# emulation built on `__builtin_args_info` and `__builtin_next_arg`). clang cannot
# run that, and the port's varargs callers are compiled for i386 cdecl anyway. The
# mirror drops those lines so every unit uses the standard <stdarg.h>, which
# compat.h includes.
LOCAL_VARARGS_RE = re.compile(
    r"^[ \t]*(?:typedef[ \t]+char[ \t]*\*[ \t]*va_list[ \t]*;|#[ \t]*define[ \t]+va_(?:start|end|arg)\b.*)[ \t]*$")


def strip_local_varargs(mirror):
    dropped, files = 0, 0
    for path in sorted(mirror.rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "va_" not in text:
            continue
        lines = text.split("\n")
        kept = [line for line in lines if not LOCAL_VARARGS_RE.match(line)]
        if len(kept) != len(lines):
            dropped += len(lines) - len(kept)
            files += 1
            path.write_text("\n".join(kept), encoding="utf-8", errors="surrogateescape")
    return dropped, files


# clang rejects a prototype whose only parameter is `...` ("ISO C requires a named parameter
# before '...'"). The decomp declares such functions only to call them with fixed arguments,
# so the mirror writes `()`: an unprototyped declaration, which takes those arguments too.
# Definitions are not matched (they need a named parameter for va_start).
UNNAMED_VARARGS_PROTO_RE = re.compile(r"^(\s*(?:extern\s+)?[^(){};=]*?\b\w+)\(\s*\.\.\.\s*\)(\s*;.*)$")


def unnamed_varargs_prototypes(mirror):
    rewritten, files = 0, 0
    for pattern in ("*.c", "*.h"):
        for path in sorted(mirror.rglob(pattern)):
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            if "(...)" not in text:
                continue
            lines = text.split("\n")
            out = []
            for line in lines:
                new, n = UNNAMED_VARARGS_PROTO_RE.subn(r"\1()\2", line)
                rewritten += n
                out.append(new)
            if out != lines:
                files += 1
                path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    return rewritten, files


_WORD_CHAR_RE = re.compile(r"\w")
_STATIC_PATTERNS = {}


def _drop_static(text, name):
    r"""Same result as `re.subn(r"\bstatic(\s[^;{}]*?\bNAME\s*\()", r"\1", text)`, but it tries a
    match only at each `static` word rather than at every character."""
    pat = _STATIC_PATTERNS.get(name)
    if pat is None:
        pat = _STATIC_PATTERNS[name] = re.compile(r"static(\s[^;{}]*?\b" + re.escape(name) + r"\s*\()")
    pieces, pos, last, n = [], 0, 0, 0
    while True:
        i = text.find("static", pos)
        if i < 0:
            break
        pos = i + 1
        if i < last or (i and _WORD_CHAR_RE.match(text, i - 1)):
            continue
        m = pat.match(text, i)
        if m:
            pieces.append(text[last:i])
            pieces.append(m.group(1))
            last = m.end()
            n += 1
    pieces.append(text[last:])
    return "".join(pieces), n


def globalize_marker_statics(mirror):
    """A `// FUN_` marker names the retail function the port links by name. A decomp prototype
    `static T name(` makes the later definition file-local, so the mirror copy drops that
    `static` (the decomp itself is not edited)."""
    total = 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "// FUN_" not in text:
            continue
        names = {n for n in (marker_definition_name(text, mk.end()) for mk in MARKER_RE.finditer(text)) if n}
        out = text
        for name in sorted(names):
            out, n = _drop_static(out, name)
            total += n
        if out != text:
            path.write_text(out, encoding="utf-8", errors="surrogateescape")
    return total


# MWCC top-level `asm <type> name(...) { ... }` functions are PS2 MIPS. clang cannot
# parse them, so each block becomes a comment and its name is left undefined: a
# trap, or a C override where one exists. Removed names go to asm_blocks.txt.
ASM_BLOCK_RE = re.compile(r"^asm\s+[^=;()]*?\b([A-Za-z_]\w*)\s*\(")


def drop_asm_blocks(mirror, build):
    removed = []
    for path in sorted((mirror / "src").rglob("*.c")):
        lines = path.read_text(encoding="utf-8", errors="surrogateescape").split("\n")
        out, i, changed = [], 0, False
        while i < len(lines):
            m = ASM_BLOCK_RE.match(lines[i])
            if not m:
                out.append(lines[i])
                i += 1
                continue
            depth, j, opened = 0, i, False
            while j < len(lines):
                depth += lines[j].count("{") - lines[j].count("}")
                opened = opened or "{" in lines[j]
                if opened and depth == 0:
                    break
                j += 1
            if j >= len(lines):
                out.append(lines[i])
                i += 1
                continue
            name = m.group(1)
            removed.append(f"{path.relative_to(mirror).as_posix()}\t{name}")
            out.append(f"/* gen_port: MWCC asm function {name} removed (clang cannot parse it) */")
            i = j + 1
            changed = True
        if changed:
            path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
    write_if_changed(build / "asm_blocks.txt", "".join(r + "\n" for r in removed))
    return removed




# The EE `sync` memory barrier is not an i386 instruction. As a statement on its own it orders
# nothing the port's single-threaded model of the EE depends on, so it becomes a compiler barrier.
_SYNC_ASM_RE = re.compile(r'__asm__\s+volatile\s*\(\s*"sync(?:\.[lp])?"\s*:\s*:\s*:\s*"memory"\s*\)')


def lower_sync_barriers(mirror):
    lowered, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "sync" not in text:
            continue
        new, n = _SYNC_ASM_RE.subn('__asm__ volatile ("" : : : "memory")', text)
        if n:
            lowered += n
            files += 1
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
    return lowered, files


_OFFSET_ASSERT_RE = re.compile(
    r"^([ \t]*typedef[ \t]+char[ \t]+\w+[ \t]*\[)([^;]*)(\];[ \t]*)$", re.M)
_OFFSET_ASSERT_EXPR_RE = re.compile(
    r"\(u32\)[ \t]*&[ \t]*\(\([ \t]*((?:(?:struct|union)[ \t]+)?\w+)"
    r"[ \t]*\*[ \t]*\)[ \t]*0[ \t]*\)[ \t]*->[ \t]*"
    r"(\w+(?:(?:[ \t]*\.[ \t]*\w+)|(?:\[[^\]\n]*\]))*)")


def portable_offset_asserts(mirror):
    """Preserve MWCC's layout checks as integral constants on both pointer widths."""
    count, files = 0, 0
    paths = sorted(list((mirror / "src").rglob("*.c")) +
                   list((mirror / "src").rglob("*.h")) +
                   list((mirror / "include").rglob("*.h")))
    for path in paths:
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        changed = 0
        def rewrite_assert(match):
            nonlocal changed
            expr, n = _OFFSET_ASSERT_EXPR_RE.subn(
                lambda m: f"((u32)__builtin_offsetof({m[1]}, {m[2]}))", match[2])
            changed += n
            return match[1] + expr + match[3]
        new = _OFFSET_ASSERT_RE.sub(rewrite_assert, text)
        if changed:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            count += changed
            files += 1
    return count, files


# ---------------------------------------------------------------- prototypes for unprototyped declarations
# A declaration without a prototype (`f32 sinf();`, `extern void foo();`) makes every caller promote
# float arguments to double, pass any argument count and read the result in the declared class. MWCC saw
# the real prototypes through its headers; clang only sees what the decomp wrote, so a call can disagree
# with its callee's definition without any diagnostic, on x86 and equally in the Windows build. The fix
# is to give each such declaration the prototype of the function's one real definition (decomp C, an
# override, a port source or, for libc, the standard prototype) in ABI-erased types: integers `int`,
# pointers `void *`, `float`, `double`, `long long`. Erasure keeps the call's argument classes and
# conversions exact while avoiding any type name that is not visible in the declaring unit. A function
# is left alone, and reported, when its definitions disagree, a parameter is a struct by value or of an
# unknown type, it is variadic, or any call in the tree passes a different number of arguments.
# `build/gen/prototype_report.txt` lists every function that was not rewritten and why.
_QUALIFIERS = {"const", "volatile", "extern", "static", "register", "signed", "unsigned", "inline", "restrict",
               "__restrict", "__inline", "__inline__"}
_ABI_TEXT = {"i": "int", "p": "void *", "f": "float", "d": "double", "q": "long long", "v": "void"}
# libm functions the tree declares without a prototype and does not define: name -> (return, params). The
# string functions are left alone: their K&R declarations take only integers and pointers, and a prototype
# with the wrong size_t spelling conflicts with the system header.
_LIBC_PROTOS = {}
for _names, _sig in (
        ("sinf cosf tanf asinf acosf atanf sqrtf fabsf floorf ceilf expf logf log10f", "f(f)"),
        ("atan2f powf fmodf", "f(ff)"),
        ("sin cos tan asin acos atan sqrt fabs floor ceil exp log log10", "d(d)"),
        ("atan2 pow fmod", "d(dd)"),
        ("abs toupper tolower", "i(i)"), ("atoi", "i(p)")):
    for _n in _names.split():
        _LIBC_PROTOS[_n] = (_sig[0], list(_sig[2:-1]))
_UNPROTO_DECL_RE = re.compile(
    r"^(?P<indent>[ \t]*)(?P<ext>(?:extern[ \t]+)?)(?P<ret>(?!(?:return|else|case|goto|sizeof|typedef|static)\b)"
    r"[A-Za-z_][\w \t\*]*?[ \t\*])(?P<name>[A-Za-z_]\w*)\s*\(\s*\)\s*;(?P<tail>[ \t]*(?://.*|/\*.*\*/)?)[ \t]*$")
_CALL_KEYWORDS = {"return", "else", "case", "do", "sizeof", "goto", "while", "if", "for", "switch"}


_LITERAL_RE = re.compile(r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'", re.S)


def _strip_literals(text):
    """text with comments and string/char literals blanked (newlines kept, so offsets and lines agree)."""
    return _LITERAL_RE.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def _typedef_classes(texts):
    """typedef name -> its ABI-class source: an underlying type text, or '@agg' for a struct/union by value."""
    td = {}
    for text in texts:
        for m in re.finditer(r"^[ \t]*typedef\b", text, re.M):
            i, depth = m.end(), 0
            stmt_end = None
            while i < len(text):
                nxt = _skip_token(text, i)
                if nxt is not None:
                    i = nxt
                    continue
                if text[i] == "{":
                    depth += 1
                elif text[i] == "}":
                    depth -= 1
                elif text[i] == ";" and depth == 0:
                    stmt_end = i
                    break
                i += 1
            if stmt_end is None or stmt_end - m.end() > 6000:
                continue
            stmt = text[m.end():stmt_end]
            if "{" in stmt:
                head = stmt[:stmt.index("{")].split()
                close = stmt.rindex("}")
                kind = "@agg" if head and head[0] in ("struct", "union") else "int"
                for decl in stmt[close + 1:].split(","):
                    decl = decl.strip()
                    nm = re.search(r"([A-Za-z_]\w*)\s*(?:\[.*)?$", decl)
                    if nm:
                        td.setdefault(nm.group(1), "void *" if "*" in decl else kind)
                continue
            if "(" in stmt:
                nm = re.search(r"\(\s*\*+\s*([A-Za-z_]\w*)\s*\)", stmt)
                if nm:
                    td.setdefault(nm.group(1), "void *")
                continue
            nm = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*$", stmt)
            if nm and nm.start() > 0:
                under = stmt[:nm.start()].strip()
                if "[" in stmt:
                    under += " *"
                td.setdefault(nm.group(1), under)
        for m in re.finditer(r"^[ \t]*#\s*define[ \t]+([A-Za-z_]\w*)[ \t]+((?:(?:unsigned|signed|const|long|short|char|int|float|double|void)[ \t]*)+)$", text, re.M):
            td.setdefault(m.group(1), m.group(2))
    return td


def _abi_class(text, td, depth=0):
    """'v' void, 'i' integer up to 32 bits or enum, 'p' pointer, 'f' float, 'd' double, 'q' 64-bit integer,
    '@agg' struct/union by value, 'x' 128-bit, '?' unknown."""
    t = re.sub(r"/\*.*?\*/", "", text)
    if "*" in t or "[" in t or "(" in t:
        return "p"
    words = [w for w in re.findall(r"[A-Za-z_]\w*", t) if w not in _QUALIFIERS]
    if not words:
        return "i" if re.search(r"\b(?:unsigned|signed)\b", t) else "?"
    if "struct" in words or "union" in words:
        return "@agg"
    if "enum" in words:
        return "i"
    if "__int128" in words or any(w.startswith("__attribute__") for w in words):
        return "x" if "__int128" in words else "?"
    if "double" in words:
        return "d"
    if "float" in words:
        return "f"
    if words.count("long") >= 2:
        return "q"
    if all(w in ("char", "short", "int", "long", "_Bool") for w in words):
        return "i"
    if words == ["void"]:
        return "v"
    if len(words) == 1 and depth < 8:
        under = td.get(words[0])
        if under is None:
            return "?"
        return "@agg" if under == "@agg" else _abi_class(under, td, depth + 1)
    return "?"


def _param_type_text(param):
    """The parameter's type text (spaces kept) without its name: `u8 *p` -> `u8 *`, `unsigned int` stays."""
    p = re.sub(r"\s+", " ", param.strip())
    if p == "..." or "(" in p or "[" in p:
        return "*" if p != "..." else p
    m = re.match(r"^(.*?[\s\*])([A-Za-z_]\w*)$", p)
    if m:
        head = m.group(1)
        words = [w for w in re.findall(r"[A-Za-z_]\w*", head) if w not in _QUALIFIERS]
        if "*" in head or words:
            return head
    return p


def _sig_classes(sig_text, td):
    """(name, return class, [parameter classes] or None for `()`, variadic) of a definition's signature."""
    m = re.match(r"^\s*(?:(?:extern|static|inline)\s+)*(?P<ret>[A-Za-z_][\w \t\*]*?[ \t\*])(?P<name>[A-Za-z_]\w*)\s*\((?P<args>.*)\)\s*$",
                 sig_text, re.S)
    if not m:
        return None
    args = _split_params(m.group("args"))
    ret = _abi_class(m.group("ret"), td)
    if not args:
        return m.group("name"), ret, None, False
    if args == ["void"]:
        return m.group("name"), ret, [], False
    variadic = any(a.strip() == "..." for a in args)
    return m.group("name"), ret, [_abi_class(_param_type_text(a), td) for a in args if a.strip() != "..."], variadic


class _Visibility:
    """What a unit can see declared: the names it and its (transitively) included headers declare or
    define, and the subset declared with a typed prototype (parameters between the parentheses). Includes
    resolve against the includer's directory, the mirror roots and, failing that, any mirror header whose
    path ends with the include text (the real build searches several -I directories)."""

    def __init__(self, mirror, texts, extra=None, extra_root=None):
        self.mirror = mirror
        self.suffix = {}
        self.texts = dict(texts)
        self.texts.update(extra or {})
        for p in self.texts:
            if p.suffix in (".h", ".inc"):
                base = extra_root if extra_root is not None and p in (extra or {}) else mirror
                parts = p.relative_to(base).as_posix().split("/")
                for k in range(len(parts)):
                    self.suffix.setdefault("/".join(parts[k:]), []).append(p)
        self.own = {}
        self.closure = {}

    def _own(self, path):
        if path not in self.own:
            text = self.texts[path] if path in self.texts else path.read_text(encoding="utf-8", errors="surrogateescape")
            stripped = _strip_literals(text)
            declared, typed = set(), set()
            for m in re.finditer(r"([A-Za-z_]\w*)\s*\(", stripped):
                k = m.start() - 1
                while k >= 0 and stripped[k] in " \t\n":
                    k -= 1
                if k < 0:
                    continue
                prev = stripped[k]
                is_decl = prev == "*"
                if prev == ",":
                    # `extern void a(void), b(s8 *);`: a later declarator of an extern/static declaration
                    ls = stripped.rfind("\n", 0, m.start()) + 1
                    is_decl = stripped[ls:m.start()].lstrip().startswith(("extern", "static"))
                if prev.isalnum() or prev == "_":
                    st = k
                    while st >= 0 and (stripped[st].isalnum() or stripped[st] == "_"):
                        st -= 1
                    is_decl = stripped[st + 1:k + 1] not in _CALL_KEYWORDS
                if not is_decl:
                    continue
                declared.add(m.group(1))
                j, depth = m.end(), 1
                while j < len(stripped) and depth:
                    depth += (stripped[j] in "([") - (stripped[j] in ")]")
                    j += 1
                inner = stripped[m.end():j - 1].strip()
                if inner and inner != "void":
                    typed.add(m.group(1))
            self.own[path] = (declared, typed, INCLUDE_RE.findall(text))
        return self.own[path]

    def of(self, path, _stack=None):
        """(declared names, typed-prototype names) visible in `path`."""
        if path in self.closure:
            return self.closure[path]
        _stack = _stack or set()
        if path in _stack:
            return set(), set()
        _stack = _stack | {path}
        declared, typed, includes = self._own(path)
        declared, typed = set(declared), set(typed)
        for inc in includes:
            found = [c for c in (path.parent / inc, self.mirror / inc, self.mirror / "include" / inc, self.mirror / "src" / inc)
                     if c in self.texts]
            found = found or self.suffix.get(inc, [])
            for cand in found:
                d, t = self.of(cand, _stack)
                declared |= d
                typed |= t
        self.closure[path] = (declared, typed)
        return declared, typed


def _param_type_text(param):
    """The parameter's type text (spaces kept) without its name: `u8 *p` -> `u8 *`, `unsigned int` stays."""
    p = re.sub(r"\s+", " ", param.strip())
    if p == "..." or "(" in p or "[" in p:
        return "*" if p != "..." else p
    m = re.match(r"^(.*?[\s\*])([A-Za-z_]\w*)$", p)
    if m:
        head = m.group(1)
        words = [w for w in re.findall(r"[A-Za-z_]\w*", head) if w not in _QUALIFIERS]
        if "*" in head or words:
            return head
    return p


def _sig_classes(sig_text, td):
    """(name, return class, [parameter classes] or None for `()`, variadic) of a definition's signature."""
    m = re.match(r"^\s*(?:(?:extern|static|inline)\s+)*(?P<ret>[A-Za-z_][\w \t\*]*?[ \t\*])(?P<name>[A-Za-z_]\w*)\s*\((?P<args>.*)\)\s*$",
                 sig_text, re.S)
    if not m:
        return None
    args = _split_params(m.group("args"))
    ret = _abi_class(m.group("ret"), td)
    if not args:
        return m.group("name"), ret, None, False
    if args == ["void"]:
        return m.group("name"), ret, [], False
    variadic = any(a.strip() == "..." for a in args)
    return m.group("name"), ret, [_abi_class(_param_type_text(a), td) for a in args if a.strip() != "..."], variadic


def _typed_prototype_names(path, mirror, memo):
    """Names with a prototype that has parameters, declared in `path` or the headers it includes."""
    if path in memo:
        return memo[path]
    memo[path] = set()
    text = path.read_text(encoding="utf-8", errors="surrogateescape")
    names, prev = set(), ""
    for line in text.split("\n"):
        m = PROTO_LINE_RE.match(line) if ";" in line else None
        if m and line.count("(") == line.count(")") and _starts_statement(prev) \
                and not re.search(r"\(\s*\)\s*;", line) and not re.search(r"\(\s*void\s*\)\s*;", line):
            names.add(m.group(1))
        if line.strip():
            prev = line.strip()
    for inc in INCLUDE_RE.findall(text):
        for cand in (path.parent / inc, mirror / inc, mirror / "include" / inc, mirror / "src" / inc):
            if cand.is_file():
                names |= _typed_prototype_names(cand, mirror, memo)
                break
    memo[path] = names
    return names


def _call_arg_counts(stripped, names):
    """name -> set of argument counts over the calls in the (literal-stripped) text; declarations and
    definitions (a type word or `*` before the name) are skipped."""
    counts = {}
    if not names:
        return counts
    pat = re.compile(r"\b(" + "|".join(sorted(map(re.escape, names), key=len, reverse=True)) + r")\b\s*\(")
    for m in pat.finditer(stripped):
        k = m.start() - 1
        while k >= 0 and stripped[k] in " \t\n":
            k -= 1
        if k >= 0:
            prev = stripped[k]
            if prev == "*" or prev == ".":
                continue
            if prev.isalnum() or prev == "_":
                s = k
                while s >= 0 and (stripped[s].isalnum() or stripped[s] == "_"):
                    s -= 1
                if stripped[s + 1:k + 1] not in _CALL_KEYWORDS:
                    continue
            elif prev == ">" and stripped[max(0, k - 1)] == "-":
                continue
        i, depth, args, empty = m.end(), 1, 0, True
        while i < len(stripped) and depth:
            c = stripped[i]
            if c in "([":
                depth += 1
            elif c in ")]":
                depth -= 1
            elif c == "," and depth == 1:
                args += 1
            elif not c.isspace() and depth >= 1:
                empty = False
            i += 1
        counts.setdefault(m.group(1), set()).add(0 if empty else args + 1)
    return counts


def _declared_in_text(stripped, names):
    """Names among `names` that the (literal-stripped) text declares or defines: an occurrence of
    `name(` with a type word or `*` before it, whatever its storage class."""
    found = set()
    if not names:
        return found
    pat = re.compile(r"\b(" + "|".join(sorted(map(re.escape, names), key=len, reverse=True)) + r")\b\s*\(")
    for m in pat.finditer(stripped):
        k = m.start() - 1
        while k >= 0 and stripped[k] in " \t\n":
            k -= 1
        if k < 0:
            continue
        prev = stripped[k]
        if prev == "*":
            found.add(m.group(1))
        elif prev.isalnum() or prev == "_":
            s = k
            while s >= 0 and (stripped[s].isalnum() or stripped[s] == "_"):
                s -= 1
            if stripped[s + 1:k + 1] not in _CALL_KEYWORDS:
                found.add(m.group(1))
    return found


def _blank_preprocessor(text):
    """text with preprocessor directive lines (and their continuations) blanked."""
    out, cont = [], False
    for line in text.split("\n"):
        if cont or line.lstrip().startswith("#"):
            cont = line.rstrip().endswith("\\")
            out.append(" " * len(line))
        else:
            out.append(line)
    return "\n".join(out)


_NOT_FUNCTIONS = _CALL_KEYWORDS | {"defined", "__attribute__", "typeof", "__typeof__", "sizeof", "__asm__", "asm",
                                   "__declspec", "_Alignof", "alignof", "va_start", "va_arg", "va_end", "va_copy"}


def _scan_functions(stripped, td):
    """Every function definition and prototype in literal-stripped text, as (name, is_definition, return class,
    [parameter classes] or None for `()`, variadic). Static functions are skipped (unit-local)."""
    stripped = _blank_preprocessor(stripped)
    found = []
    for m in re.finditer(r"([A-Za-z_]\w*)[ \t]*\(", stripped):
        name = m.group(1)
        if name in _NOT_FUNCTIONS:
            continue
        i, depth = m.end(), 1
        while i < len(stripped) and depth:
            depth += (stripped[i] in "([") - (stripped[i] in ")]")
            i += 1
        j = i
        while j < len(stripped) and stripped[j] in " \t\n":
            j += 1
        nxt = stripped[j] if j < len(stripped) else ""
        if nxt not in (";", "{"):
            continue
        b = max(stripped.rfind(c, 0, m.start()) for c in ";{}")
        ret = stripped[b + 1:m.start()].strip()
        if not ret or re.search(r"[^A-Za-z0-9_\s*]", ret) or re.match(r"(?:return|typedef|else|case|goto|do|if|while|for|switch)\b", ret):
            continue
        words = re.findall(r"[A-Za-z_]\w*", ret)
        if "static" in words or "typedef" in words:
            continue
        # library/export macros (RWAPI, RWEXPORT, ...) are not part of the type
        ret = " ".join(w if (w in td or not re.fullmatch(r"[A-Z][A-Z0-9_]*", w) or len(words) == 1) else "" for w in re.findall(r"[A-Za-z_]\w*|\*", ret))
        args = _split_params(stripped[m.end():i - 1])
        variadic = any(a.strip() == "..." for a in args)
        if not args:
            params = None
        elif args == ["void"]:
            params = []
        else:
            params = [_abi_class(_param_type_text(a), td) for a in args if a.strip() != "..."]
        found.append((name, nxt == "{", _abi_class(ret, td), params, variadic))
    return found


def _word(cls):
    return "w" if cls in ("i", "p") else cls


def _split_args_at(stripped, open_idx):
    """For the `(` at stripped[open_idx]: ([(start, end) of each top-level argument], index of the matching `)`)."""
    i, depth, start, spans = open_idx + 1, 1, open_idx + 1, []
    while i < len(stripped) and depth:
        c = stripped[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c == "," and depth == 1:
            spans.append((start, i))
            start = i + 1
        i += 1
    spans.append((start, i - 1))
    return spans, i - 1


def _retype_param(param, cls):
    """`param` (a parameter declaration) with its type replaced by the erased type of class `cls`; the name stays."""
    p = re.sub(r"\s+", " ", param.strip())
    ty = _param_type_text(p)
    name = p[len(ty):].strip() if p.startswith(ty) else ""
    return (_ABI_TEXT[cls] + (" " + name if re.fullmatch(r"[A-Za-z_]\w*", name) else "")).strip()


_LOCAL_PROTO_RE = re.compile(
    r"^(?P<indent>[ \t]*)(?P<ext>(?:extern[ \t]+)?)(?P<ret>(?!(?:return|else|case|goto|sizeof|typedef|static)\b)"
    r"[A-Za-z_][\w \t\*]*?[ \t\*])(?P<name>[A-Za-z_]\w*)\s*\((?P<args>[^;{}()]*(?:\([^)]*\)[^;{}()]*)*)\)"
    r"\s*(?P<asm>__asm__\(\"(?P<link>[A-Za-z_]\w*)\"\))?\s*;(?P<tail>[ \t]*(?://.*|/\*.*\*/)?)[ \t]*$")


def add_alias_definition_signatures(defsig, names, native_names, unbound):
    """An undefined C alias inherits its native target's signature, never a different name's prototype."""
    aliases = set()
    groups = {}
    for name, addr in names.items():
        groups.setdefault(addr, set()).add(name)
    for addr, group in sorted(groups.items()):
        generic = f"func_{addr:08x}"
        candidates = [native_names.get(addr), generic] + sorted(group)
        target = next((name for name in candidates if name in defsig and name != unbound.get(addr)), None)
        if target is None or defsig[target][0] not in _ABI_TEXT:
            continue
        for alias in group | {generic, f"FUN_{addr:08X}"}:
            if alias not in defsig:
                defsig[alias] = defsig[target]
                aliases.add(alias)
    return aliases


def align_prototypes_with_definitions(mirror, defsig, td, own_defs, alias_targets=()):
    """A typed prototype in a unit whose ABI shape differs from the function's definition is the decomp
    disagreeing with itself. MWCC on the PS2 assigns integer and floating-point arguments to registers
    independently, so `f(int a, float b)` and `f(float b, int a)` are the same call there; on x86 the
    argument order is the stack order and the two disagree. Same-arity prototypes are made to match the
    definition: a parameter of the wrong class (a 32-bit integer where the definition takes a 64-bit one,
    an integer where it takes a float, and the reverse) is retyped; when the prototype's classes are a
    reordering of the definition's, its parameters are reordered and every call in the unit reorders its
    arguments the same way. Returns (retyped, reordered, calls_reordered, arity_conflicts, ret_fixed)."""
    retyped = reordered = calls_reordered = arity = ret_fixed = 0
    conflicts = []
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        lines = text.split("\n")
        mine = own_defs.get(path, set())
        edits, perms = [], {}
        for i, line in enumerate(lines):
            if "(" not in line or not line.rstrip().endswith((";", "*/")):
                continue
            m = _LOCAL_PROTO_RE.match(line)
            if not m or m.group("name") in mine:
                continue
            name = m.group("name")
            target = m.group("link") or name
            if target not in defsig:
                continue
            ret_d, params_d = defsig[target]
            args = _split_params(m.group("args"))
            if not args or args == ["void"] or any(a.strip() == "..." for a in args):
                continue
            params_p = [_abi_class(_param_type_text(a), td) for a in args]
            if "?" in params_p:
                continue
            permutation_only = bool(m.group("link") or target in alias_targets or "@agg" in params_p or "@agg" in params_d)
            # New alias coverage repairs GP/FP order only. Do not silently expand
            # this audit into unrelated integer widths or return-value semantics.
            nrm = (lambda c: "w" if c in ("i", "p", "q") else "f" if c in ("f", "d") else c) if permutation_only else (
                lambda c: "w" if c in ("i", "p") else c)
            if len(params_p) != len(params_d):
                arity += 1
                conflicts.append(f"{name}: {path.relative_to(mirror).as_posix()} declares {len(params_p)} parameters, definition takes {len(params_d)}")
                continue
            if permutation_only and sorted(map(nrm, params_p)) != sorted(map(nrm, params_d)):
                conflicts.append(f"{name}: {path.relative_to(mirror).as_posix()} non-permutation alias/aggregate ABI requires an explicit repair")
                continue
            ret_p = _abi_class(m.group("ret"), td)
            if permutation_only:
                ret_d = ret_p
            new_ret = m.group("ret")
            if ret_d != "v" and ((ret_d in ("f", "d")) != (ret_p in ("f", "d")) or (ret_p == "q" and ret_d != "q")):
                new_ret = _ABI_TEXT[ret_d] + " "
                ret_fixed += 1
            if [nrm(c) for c in params_p] == [nrm(c) for c in params_d]:
                new_args = args
            elif sorted(nrm(c) for c in params_p) == sorted(nrm(c) for c in params_d):
                # reordering: the k-th parameter of a class in the prototype is the k-th of that class in the definition
                slots, order = {}, []
                for k, c in enumerate(params_d):
                    slots.setdefault(nrm(c), []).append(k)
                taken = {}
                for c in params_p:
                    idx = taken.get(nrm(c), 0)
                    order.append(slots[nrm(c)][idx])
                    taken[nrm(c)] = idx + 1
                new_args = [None] * len(args)
                for k, dst in enumerate(order):
                    new_args[dst] = args[k]
                perms.setdefault(name, order)
                reordered += 1
                conflicts.append(f"REORDERED {name}: {path.relative_to(mirror).as_posix()} (prototype and calls now in the definition's order)")
            else:
                new_args = [a if nrm(c) == nrm(d) else _retype_param(a, d)
                            for a, c, d in zip(args, params_p, params_d)]
                retyped += sum(1 for c, d in zip(params_p, params_d) if nrm(c) != nrm(d))
            if new_args != args or new_ret != m.group("ret"):
                label = f" {m.group('asm')}" if m.group("asm") else ""
                edits.append((i, f"{m.group('indent')}{m.group('ext')}{new_ret}{name}({', '.join(new_args)}){label};{m.group('tail')}"))
        if not edits:
            continue
        for i, newline in edits:
            lines[i] = newline
        text = "\n".join(lines)
        if perms:
            stripped = _strip_literals(text)
            repl = []
            for mm in re.finditer(r"\b(" + "|".join(map(re.escape, perms)) + r")\b[ \t]*\(", stripped):
                k = mm.start() - 1
                while k >= 0 and stripped[k] in " \t\n":
                    k -= 1
                if k >= 0 and (stripped[k] == "*" or stripped[k] == "." or (stripped[k].isalnum() or stripped[k] == "_")):
                    st = k
                    while st >= 0 and (stripped[st].isalnum() or stripped[st] == "_"):
                        st -= 1
                    if stripped[k] in "*." or stripped[st + 1:k + 1] not in _CALL_KEYWORDS:
                        continue
                spans, close = _split_args_at(stripped, mm.end() - 1)
                order = perms[mm.group(1)]
                if len(spans) != len(order):
                    conflicts.append(f"{mm.group(1)}: {path.relative_to(mirror).as_posix()} call passes {len(spans)} arguments, prototype has {len(order)}")
                    continue
                parts = [text[a:b] for a, b in spans]
                new_parts = [None] * len(parts)
                for k2, dst in enumerate(order):
                    new_parts[dst] = parts[k2]
                repl.append((mm.end(), close, ",".join(new_parts)))
                calls_reordered += 1
            for a, b, new in sorted(repl, reverse=True):
                text = text[:a] + new + text[b:]
        path.write_text(text, encoding="utf-8", errors="surrogateescape")
    return retyped, reordered, calls_reordered, arity, ret_fixed, conflicts


def prototype_unprototyped_declarations(mirror, root, report_path):
    """Rewrites `T name();` declarations to the prototype of the function's definition (see above) and
    declares implicitly-declared callees. Returns (rewritten, files, implicit, ret_fixed, skipped)."""
    paths = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h"))
                   + list((mirror / "include").rglob("*.h")))
    texts = {p: p.read_text(encoding="utf-8", errors="surrogateescape") for p in paths}
    # the port's own headers and .inc fragments (include/p4pc/...) are visible to the units that include them
    port_paths = sorted([*(root / "include").rglob("*.h"), *(root / "include").rglob("*.inc")])
    port_headers = {p: p.read_text(encoding="utf-8", errors="surrogateescape") for p in port_paths}
    port_texts = [p.read_text(encoding="utf-8", errors="surrogateescape") for p in sorted((root / "src").rglob("*.c"))]
    td = _typedef_classes(list(texts.values()))

    # 1. one ABI signature per declared name, from every definition and typed prototype in the tree
    sigs, conflicts = {}, set()
    records = {}
    sources = [(p, t) for p, t in texts.items()] + [(None, t) for t in port_texts] + list(port_headers.items())
    own_defs, def_shapes = {}, {}
    for path, text in sources:
        for name, is_def, ret, params, variadic in _scan_functions(_strip_literals(text), td):
            if params is None and not is_def:
                continue          # an unprototyped declaration says nothing about the parameters
            records.setdefault(name, []).append((ret, [] if params is None else params, variadic, is_def))
            if is_def:
                own_defs.setdefault(path, set()).add(name)
                if not variadic:
                    def_shapes.setdefault(name, set()).add((ret, tuple([] if params is None else params)))
    for name, recs in records.items():
        if any(v for _, _, v, _ in recs):
            sigs[name] = None
            continue
        # a definition is the authority; typed prototypes are the fallback when the tree has no definition
        use = [r for r in recs if r[3]] or recs
        shapes = {(_word(r), tuple(_word(c) for c in ps)) for r, ps, _, _ in use}
        if len(shapes) > 1:
            conflicts.add(name)
            continue
        merged = []
        for k in range(len(use[0][1])):
            col = {r[1][k] for r in use}
            merged.append("p" if col == {"p"} else ("i" if col <= {"i", "p"} else use[0][1][k]))
        retset = {r[0] for r in use}
        ret = "p" if retset == {"p"} else ("i" if retset <= {"i", "p"} else use[0][0])
        sigs[name] = (ret, tuple(merged))
    # 1b. typed prototypes that disagree with the definition
    defsig = {}
    for name, shapes in def_shapes.items():
        if len(shapes) == 1:
            ret, params = next(iter(shapes))
            if not any(c in ("x", "?") for c in params) and ret not in ("x", "?"):
                defsig[name] = (ret, list(params))
    alias_names = retail_function_names(root / "decomp", mirror.parent, staged_texts=texts)
    definition_aliases = add_alias_definition_signatures(defsig, alias_names, read_native_names(), read_unbound_natives())
    retyped, reordered, calls_reordered, arity_conf, ret_fixed2, align_conflicts = align_prototypes_with_definitions(
        mirror, defsig, td, own_defs, definition_aliases)
    for path in texts:
        if path.suffix == ".c":
            texts[path] = path.read_text(encoding="utf-8", errors="surrogateescape")

    # 2. the unprototyped declaration sites and the call arities
    decl_names = set()
    for path, text in texts.items():
        for line in text.split("\n"):
            m = _UNPROTO_DECL_RE.match(line)
            if m:
                decl_names.add(m.group("name"))
    wanted = {n for n in decl_names if n in sigs}
    stripped = {p: _strip_literals(t) for p, t in texts.items() if p.suffix == ".c"}
    arity = {}
    for text in stripped.values():
        for n, c in _call_arg_counts(text, wanted).items():
            arity.setdefault(n, set()).update(c)
    skipped, ok = {}, {}
    for name in sorted(wanted):
        sig = sigs[name]
        if name in conflicts:
            skipped[name] = "definitions disagree"
        elif sig is None:
            skipped[name] = "variadic or unparsable definition"
        elif sig[1] is None:
            skipped[name] = "definition has no prototype"
        elif any(c in ("@agg", "x", "?") for c in sig[1]) or sig[0] in ("x",):
            skipped[name] = "parameter is a struct by value, 128-bit or an unknown type"
        elif arity.get(name, set()) - {len(sig[1])}:
            skipped[name] = f"calls pass {sorted(arity[name])} arguments, definition takes {len(sig[1])}"
        else:
            ok[name] = sig

    # 3. rewrite the declaration sites
    rewritten, files, ret_fixed = 0, 0, 0
    vis = _Visibility(mirror, texts, port_headers, root / "include")
    for path, text in texts.items():
        if path.suffix != ".c":
            continue   # a header's declaration is seen by every includer, whose own prototypes may differ
        own = set(_definitions(text.split("\n")).keys()) | vis.of(path)[1]
        lines, changed = text.split("\n"), False
        for i, line in enumerate(lines):
            if "(" not in line:
                continue
            m = _UNPROTO_DECL_RE.match(line)
            if not m or m.group("name") not in ok or m.group("name") in own:
                continue
            name = m.group("name")
            ret_cls, params = ok[name]
            decl_ret = m.group("ret")
            decl_cls = _abi_class(decl_ret, td)
            ret_text = decl_ret.rstrip()
            if decl_cls != ret_cls and (ret_cls in ("f", "d", "q") or decl_cls in ("f", "d", "q")) and ret_cls != "v":
                ret_text = _ABI_TEXT[ret_cls]
                ret_fixed += 1
            plist = ", ".join(_ABI_TEXT[c] for c in params) if params else "void"
            lines[i] = f"{m.group('indent')}extern {ret_text} {name}({plist});{m.group('tail')}"
            rewritten += 1
            changed = True
        if changed:
            files += 1
            path.write_text("\n".join(lines), encoding="utf-8", errors="surrogateescape")
            texts[path] = "\n".join(lines)

    # 4. callees with a definition that the unit never declares (implicitly `int f()`)
    implicit, implicit_fixed = 0, 0
    macro_names = set()
    for t in texts.values():
        macro_names.update(re.findall(r"^[ \t]*#\s*define[ \t]+([A-Za-z_]\w*)", t, re.M))
    candidates = {n for n, s in sigs.items() if s is not None and s[1] is not None
                  and not any(c in ("@agg", "x", "?") for c in s[1]) and n not in conflicts and n not in macro_names}
    called_by = {}
    stripped = {p: _strip_literals(t) for p, t in texts.items() if p.suffix == ".c"}
    all_arity = {}
    for path, text in stripped.items():
        used = candidates.intersection(re.findall(r"\b([A-Za-z_]\w*)\s*\(", text))
        got = _call_arg_counts(text, used)
        if got:
            called_by[path] = got
            for n, c in got.items():
                all_arity.setdefault(n, set()).update(c)
    for path, got in called_by.items():
        text = texts[path]
        declared = vis.of(path)[0]
        add = []
        for name in sorted(got):
            if name in declared:
                continue
            ret_cls, params = sigs[name]
            if all_arity.get(name, set()) - {len(params)}:
                skipped.setdefault(name, f"implicit calls pass {sorted(all_arity[name])} arguments, definition takes {len(params)}")
                continue
            ret_text = _ABI_TEXT[ret_cls] if ret_cls in ("f", "d", "q") else "int"
            if ret_cls in ("f", "d", "q"):
                implicit_fixed += 1
            plist = ", ".join(_ABI_TEXT[c] for c in params) if params else "void"
            add.append(f"extern {ret_text} {name}({plist});")
        if add:
            lines = text.split("\n")
            last = max((i for i, l in enumerate(lines) if re.match(r"\s*#\s*include\b", l)), default=-1)
            lines[last + 1:last + 1] = ["/* gen_port: prototypes for callees the decomp never declares (implicit int) */"] + add
            path.write_text("\n".join(lines), encoding="utf-8", errors="surrogateescape")
            implicit += len(add)
    report = [f"aligned typed prototypes with their definitions: {retyped} parameters retyped, {reordered} prototypes "
              f"reordered ({calls_reordered} call sites reordered), {ret_fixed2} return classes fixed, "
              f"{arity_conf} parameter-count disagreements left as they are",
              f"rewritten {rewritten} declarations in {files} files for {len(ok)} functions ({ret_fixed} return classes fixed)",
              f"declared {implicit} implicitly-declared callees ({implicit_fixed} with a float/double/64-bit return)",
              f"not rewritten: {len(skipped)} functions"]
    report += [f"  {n}: {why}" for n, why in sorted(skipped.items())]
    report += ["prototypes reordered to the definition's parameter order (and their calls with them):"] + \
        [f"  {c[10:]}" for c in sorted(align_conflicts) if c.startswith("REORDERED ")]
    report += ["parameter-count disagreements between a prototype and the definition:"] + \
        [f"  {c}" for c in sorted(align_conflicts) if not c.startswith("REORDERED ")]
    Path(report_path).write_text("\n".join(report) + "\n", encoding="utf-8")
    return rewritten, files, implicit, ret_fixed, len(skipped), (retyped, reordered, calls_reordered, arity_conf, ret_fixed2)


# ---------------------------------------------------------------- mirror fixes

def read_mirror_fixes():
    """config/mirror_fixes.txt: `<unit>TAB<old text>TAB<new text>TAB<reason>`, one literal substitution
    per line (`\n` is a newline in either text). These repair spots where the decomp's C is wrong for
    the PC build (a call that omits an argument the callee reads from the caller's own register, a
    prototype of the wrong arity) and that no general pass can see."""
    fixes = []
    for n, line in enumerate((ROOT / "config" / "mirror_fixes.txt").read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) not in (4, 5) or not all(p.strip() for i, p in enumerate(parts) if i != 2):
            die(f"config/mirror_fixes.txt:{n}: expected <unit>TAB<old>TAB<new>TAB<reason>[TAB<count>]")
        unit, old, new, _ = parts[:4]
        expected = parts[4] if len(parts) == 5 else "1"
        if not expected.isdecimal() or int(expected) < 1:
            die(f"config/mirror_fixes.txt:{n}: expected a positive match count, got {expected!r}")
        fixes.append((n, unit, old.replace("\\n", "\n"), new.replace("\\n", "\n"), int(expected)))
    return fixes


def apply_mirror_fixes(mirror):
    applied = 0
    errors = []
    for n, unit, old, new, expected in read_mirror_fixes():
        path = mirror / unit
        if not path.is_file():
            errors.append(f"config/mirror_fixes.txt:{n}: {unit} is not in the mirror")
            continue
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        count = text.count(old)
        if count != expected:
            errors.append(f"config/mirror_fixes.txt:{n}: expected {expected} matches in {unit}, found {count} "
                          f"(the decomp changed?): {old[:70]!r}")
            continue
        path.write_text(text.replace(old, new), encoding="utf-8", errors="surrogateescape")
        applied += 1
    if errors:
        die("mirror fix match failures:\n  " + "\n  ".join(errors))
    return applied


# ---------------------------------------------------------------- soft-double helpers
# The EE has no double hardware: the retail soft-double helpers (and the double libm cores behind them) take and return the 64-bit double BIT PATTERN
# (MIPS passes it in a register pair; the port's natives in src/platform/ps2_fpbit.c, overrides/0044b310.c, src/platform/ps2_libc.c and the decomp's own
# gcc_ee_wrappers.c use uint64_t). The decomp declares them in every unit that uses them, mostly with a 32-bit or f64 type (`extern s32 fptodp(f32)`,
# `u32 uVar1 = func_0044b8d8(...)`): on i386 the caller then reads half the result and passes half the argument, which turned the yaw of every moving scene
# resource into garbage (event walkers and crowd NPCs faced +Z) and the fog puffs' alpha into noise. This pass gives every declaration of these functions the
# port's own signature and widens the locals that receive a 64-bit result; it dies if a declaration still disagrees, so the class cannot come back.
SOFT_DOUBLE = {
    "fptodp": ("u64", ["f32"]),
    "dptofp": ("f32", ["u64"]),
    "dpsub": ("u64", ["u64", "u64"]),
    "func_0044e168": ("u64", ["u64", "u64"]),
    "func_0044b310": ("u64", ["u64"]),
    "func_004470f0": ("u64", ["u64"]),
    "func_00447b88": ("u64", ["u64"]),
    "func_00447d00": ("u64", ["u64", "u64"]),
    "func_0044b8d8": ("s64", ["s64"]),
    "func_0044b8f0": ("s32", ["s64"]),
    "func_0044b908": ("s32", ["s64", "s32"]),
}
SOFT_DOUBLE_NAMES = "|".join(sorted(SOFT_DOUBLE))
SOFT_DOUBLE_DECL_RE = re.compile(
    r"(?m)^([ \t]*)(?:extern[ \t]+)?(?!return\b)((?:[A-Za-z_]\w*[ \t*]+)+?)\*?[ \t]*\b(" + SOFT_DOUBLE_NAMES + r")[ \t]*\(([^()]*)\)[ \t]*;")
SOFT_DOUBLE_ASSIGN_RE = re.compile(r"\b([A-Za-z_]\w*)[ \t]*=[ \t]*(?:\([^()]*\)[ \t]*)?\b(" + SOFT_DOUBLE_NAMES + r")[ \t]*\(")
NARROW_LOCAL_TYPES = r"(?:unsigned[ \t]+int|unsigned|int|long|short|u8|s8|u16|s16|u32|s32|u_int|f32|float|f64|double)"


def soft_double_prototype(name):
    ret, args = SOFT_DOUBLE[name]
    return f"extern {ret} {name}({', '.join(args)});"


def pass_soft_double_types(mirror):
    """Retype every declaration of a soft-double helper to the port's signature and widen the locals that receive a 64-bit result."""
    decls = widened = 0
    for path in sorted((mirror / "src").rglob("*.c")) + sorted((mirror / "src").rglob("*.h")) + sorted((mirror / "include").rglob("*.h")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if not re.search(SOFT_DOUBLE_NAMES, text):
            continue
        new = text

        def decl(m):
            nonlocal decls
            name = m.group(3)
            want = soft_double_prototype(name)
            if m.group(0).strip() == want:
                return m.group(0)
            decls += 1
            return m.group(1) + want
        new = SOFT_DOUBLE_DECL_RE.sub(decl, new)
        # a local that receives a 64-bit result: the nearest declaration above the assignment
        for m in list(SOFT_DOUBLE_ASSIGN_RE.finditer(new)):
            if not SOFT_DOUBLE[m.group(2)][0] in ("u64", "s64"):
                continue
            var = m.group(1)
            head = new[:m.start()]
            dm = None
            for dm in re.finditer(r"(?m)^([ \t]*)(" + NARROW_LOCAL_TYPES + r")([ \t]+)" + re.escape(var) + r"[ \t]*;", head):
                pass
            if dm is not None and head.count("\n}") == head[:dm.start()].count("\n}"):
                wide = "s64" if dm.group(2) in ("int", "long", "short", "s8", "s16", "s32") else "u64"
                new = new[:dm.start()] + dm.group(1) + wide + dm.group(3) + var + ";" + new[dm.end():]
                widened += 1
        if new != text:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
    # the class must not come back: every declaration agrees and every local that receives a 64-bit result is wide
    bad = []
    for path in sorted((mirror / "src").rglob("*.c")) + sorted((mirror / "src").rglob("*.h")) + sorted((mirror / "include").rglob("*.h")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for m in SOFT_DOUBLE_DECL_RE.finditer(text):
            if m.group(0).strip() != soft_double_prototype(m.group(3)):
                bad.append(f"{path.relative_to(mirror)}: {m.group(0).strip()}")
        for m in SOFT_DOUBLE_ASSIGN_RE.finditer(text):
            if SOFT_DOUBLE[m.group(2)][0] not in ("u64", "s64"):
                continue
            head = text[:m.start()]
            dm = None
            for dm in re.finditer(r"(?m)^[ \t]*(" + NARROW_LOCAL_TYPES + r")[ \t]+" + re.escape(m.group(1)) + r"[ \t]*;", head):
                pass
            if dm is not None and head.count("\n}") == head[:dm.start()].count("\n}"):
                bad.append(f"{path.relative_to(mirror)}: `{m.group(1)}` is {dm.group(1)} but receives {m.group(2)}'s 64-bit result")
    if bad:
        die("soft-double helpers declared or received with a narrower type (they take and return the 64-bit double bit pattern):\n  " + "\n  ".join(bad[:20]))
    return decls, widened


# ---------------------------------------------------------------- effect kind dispatch
# The flash and wind effect kinds' Stop/Advance/UpdateIfActive stubs (mdlEffect.c) pick a function from a per-kind table
# and call it with the effect pointer still in $a0: the retail asm moves the argument to s0 and does not touch a0 before
# the jalr. The decomp wrote the calls with an empty argument list, so on the host the kind function (FlashRadialEmitQuads,
# ...) received a stack word as its effect and read junk (a state block full of file data, frame 0x2900000f).
EFFECT_KIND_CALL_RE = re.compile(r"\(\*\(void \(\*\*\)\(void\)\)(\(D_00714[0-9a-f]{3} \+ \(\(\(MdlEffectVectorInstance \*\)(\w+)\)->kind\.value \* 0x1C\)\))\)\(\);")


def pass_effect_kind_dispatch(mirror):
    count = 0
    for path in (mirror / "src").rglob("mdlEffect.c"):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        new, n = EFFECT_KIND_CALL_RE.subn(lambda m: "(*(void (**)(u8 *))" + m.group(1) + ")(" + m.group(2) + ");", text)
        if n:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            count += n
    return count


# ---------------------------------------------------------------- bool returns
# A MIPS function returns a full register; a function typed bool leaves its result in %al on x86 with the rest of the register undefined, and a
# caller that reads it as an int (through a function pointer typed `s32 (*)(u8 *)`, which no declaration pass can retype) tests the stale upper
# bytes: btlFormationHasEntry, the wait condition of a battle packet, returned false in AL with a pointer above it on i386, the packet read that
# as true and the battle's first action started 31 frames early (the x86_64 build, whose RAX was clean there, waited as retail does). Every bool
# definition returns u32 on the host (its `return expr;` becomes `return (bool)(expr);`, the truncation the type did) and every declaration of
# the name takes the wider type. (The s8/s16/u8/u16 results of direct calls are align_narrow_returns', below: it gives the declarations the
# definition's narrow type, so the caller extends the value itself.)
BOOL_RETURN = {"bool": "u32", "_Bool": "u32"}
_BOOL_ALT = "|".join(sorted((re.escape(t) for t in BOOL_RETURN), key=len, reverse=True))
BOOL_DEF_RE = re.compile(r"^((?:static[ \t]+)?)(" + _BOOL_ALT + r")([ \t]+)(\w+)([ \t]*\()", re.M)
# Upstream made these logical predicates explicitly u8 instead of bool. They still occupy
# the same indirect full-register-return slots; align_narrow_returns cannot fix those calls.
BOOL_U8_RETURNS = {"btlCond_ANALYZE", "btlFormationHasEntry"}
BOOL_U8_DEF_RE = re.compile(r"^((?:static[ \t]+)?)(u8)([ \t]+)(" +
                            "|".join(sorted(BOOL_U8_RETURNS)) + r")([ \t]*\()", re.M)


def widen_narrow_returns(mirror):
    files = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h")))
    texts = {f: f.read_text(encoding="utf-8", errors="surrogateescape") for f in files}
    names = set()
    u8_definitions = {name: 0 for name in BOOL_U8_RETURNS}
    for f, t in texts.items():
        if f.suffix != ".c":
            continue
        for pattern in (BOOL_DEF_RE, BOOL_U8_DEF_RE):
            for m in pattern.finditer(t):
                close = _split_args_at(t, m.end() - 1)[1]
                if re.match(r"\s*\{", t[close + 1:]):
                    names.add(m[4])
                    if pattern is BOOL_U8_DEF_RE:
                        u8_definitions[m[4]] += 1
    bad = {name: count for name, count in u8_definitions.items() if count != 1}
    if bad:
        die(f"u8 predicate return contracts require one definition each, found {bad}")
    name_re = "|".join(sorted(map(re.escape, names)))
    # (a unit that says `typedef u8 bool;` and a header that declares the function u8 are the same function)
    decl_alt = _BOOL_ALT + "|u8|s8|unsigned char"
    pat = re.compile(r"^([ \t]*(?:extern[ \t]+|static[ \t]+)*)(" + decl_alt + r")([ \t]+)(" + name_re + r")([ \t]*\()", re.M)
    for f, t in texts.items():
        if not any(n in t for n in names):
            continue
        out, pos = [], 0
        for m in pat.finditer(t):
            out.append(t[pos:m.start()])
            out.append(m[1] + "u32" + m[3] + m[4] + m[5])
            pos = m.end()
            close = _split_args_at(t, m.end() - 1)[1]
            brace = re.match(r"\s*\{", t[close + 1:])
            if brace:                                        # a definition: keep the type's truncation
                end_b = _match_brace(t, close + 1 + brace.end() - 1)
                body = t[m.end():end_b]
                body = re.sub(r"\breturn[ \t]+([^;]+);", lambda r: "return (" + m[2] + ")(" + r[1] + ");", body)
                out.append(body)
                pos = end_b
        out.append(t[pos:])
        new = "".join(out)
        if new != t:
            f.write_text(new, encoding="utf-8", errors="surrogateescape")
    return len(names)


# ---------------------------------------------------------------- float results ignored through a void prototype
# An i386 float result arrives in st(0) and the caller pops it. A call through a prototype that says `void` (a decomp
# `extern void RwV3dLength(void *);`, an asm alias declared void) never pops, so every call leaks one x87 register; with the
# register stack filling up, the next x87 load (glibc's acosf, sinf, ...) returns the real indefinite NaN 0xFFC00000, which
# is how the battle camera's quaternions turned to NaN. A function defined to return f32/float (in the mirror, an override
# or a port native) has its `void` prototypes retyped f32 so the caller pops the result it ignores.
FLOAT_DEF_RE = re.compile(r"^[ \t]*(?:static[ \t]+)?(?:f32|float)[ \t]+(\w+)[ \t]*\(", re.M)
VOID_DECL_RE = re.compile(r"^([ \t]*(?:extern[ \t]+)*)void([ \t]+)(\w+)([ \t]*\()", re.M)


def float_result_decls(mirror):
    defs = set()
    sources = list((mirror / "src").rglob("*.c")) + list((ROOT / "overrides").glob("*.c")) + list((ROOT / "src").rglob("*.c"))
    for f in sources:
        t = f.read_text(encoding="utf-8", errors="surrogateescape")
        for m in FLOAT_DEF_RE.finditer(t):
            close = _split_args_at(t, m.end() - 1)[1]
            if re.match(r"\s*\{", t[close + 1:]):
                defs.add(m[1])
    fixed = 0
    files = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h")))
    for f in files:
        t = f.read_text(encoding="utf-8", errors="surrogateescape")
        out, pos = [], 0
        for m in VOID_DECL_RE.finditer(t):
            if m[3] not in defs:
                continue
            close = _split_args_at(t, m.end() - 1)[1]
            if not re.match(r"\s*;", t[close + 1:]):        # only prototypes: a definition keeps its own type
                continue
            out.append(t[pos:m.start()])
            out.append(m[1] + "f32" + m[2] + m[3] + m[4])
            pos = m.end()
            fixed += 1
        if out:
            out.append(t[pos:])
            f.write_text("".join(out), encoding="utf-8", errors="surrogateescape")
    return fixed, len(defs)


# ---------------------------------------------------------------- 128-bit returns
# MWCC leaves a quadword result in $v0 and the decomp types such a function as returning s128 / u_long128 (the
# matching asm loads and stores with lq/sq and keeps the value live). On i386 a 16-byte struct return is a hidden
# first argument that the callee pops (`ret $4`): a caller that does not pass it (every call through a function
# pointer, the effect vtable's position/rotation/quad slots) corrupts the stack, and the call returns to a stack
# word. No caller uses the value, so these functions return void on the host.
def _is_declaration(text, m):
    """The match is `type name(` at a line start: a declaration or definition when its parameter list is followed by `;` or `{`."""
    close = _split_args_at(text, m.end() - 1)[1]
    return re.match(r"\s*[;{]", text[close + 1:]) is not None


QUAD_RETURN_RE = re.compile(r"^([ \t]*(?:extern[ \t]+|static[ \t]+)*)(?:s128|u_long128|u128|u_int128|__int128)([ \t]+)(\w+)([ \t]*\()", re.M)


def void_quad_returns(mirror):
    files = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h")))
    texts = {f: f.read_text(encoding="utf-8", errors="surrogateescape") for f in files}
    names = {m[3] for t in texts.values() for m in QUAD_RETURN_RE.finditer(t)}
    anytype = re.compile(r"^([ \t]*(?:extern[ \t]+|static[ \t]+)*)(?!void\b|return\b|else\b|goto\b|case\b|sizeof\b)[A-Za-z_]\w*(?:[ \t]+[A-Za-z_]\w*)*([ \t]+)(" +
                         "|".join(sorted(map(re.escape, names))) + r")([ \t]*\()", re.M) if names else None
    for f, t in texts.items():
        if not names or not any(n in t for n in names):
            continue
        orig = t
        # a declaration that spells the quadword through a typedef (ParticleSnapshot) is a declaration of the same function
        t = anytype.sub(lambda m: m[1] + "void" + m[2] + m[3] + m[4] if re.match(r"\s*(?:s128|u_long128|u128|u_int128|__int128)\b", m[0][len(m[1]):]) is None and _is_declaration(t, m) else m[0], t)
        out = []
        pos = 0
        for m in QUAD_RETURN_RE.finditer(t):
            if m[3] not in names:
                continue
            out.append(t[pos:m.start()])
            out.append(m[1] + "void" + m[2] + m[3] + m[4])
            pos = m.end()
            # a definition: its `return value;` statements become `{ (void)(value); return; }`
            close = _split_args_at(t, m.end() - 1)[1]
            brace = re.match(r"\s*\{", t[close + 1:])
            if brace:
                open_b = close + 1 + brace.end() - 1
                end_b = _match_brace(t, open_b)
                body = t[m.end():end_b]
                body = re.sub(r"\breturn\s+([^;]+);", lambda r: "{ (void)(" + r[1] + "); return; }", body)
                out.append(body)
                pos = end_b
        out.append(t[pos:])
        new = "".join(out)
        if new != orig:
            f.write_text(new, encoding="utf-8", errors="surrogateescape")
    return len(names)


# ---------------------------------------------------------------- pass-through arguments
# On the EE a callee reads argument registers a0..a3 / $f12.. whether or not the caller set them, and the
# decomp often wrote `f()` where retail jals with the caller's own incoming register untouched (the callee
# then sees the caller's argument). On x86 the callee reads stack garbage. Each row of
# config/passthrough_calls.txt names a function, a callee and which call in the function's body (1-based,
# source order), and gives the argument list the call needs: the original arguments plus the caller's own
# parameters for the omitted ones. Rows come from tools/verify_passthrough.py, which checks the retail asm:
# the omitted registers are unwritten between the function's entry and the jal.
def read_passthrough_calls():
    rows = []
    # passthrough_calls.txt is generated by tools/verify_passthrough.py; the manual file holds the sites that tool
    # cannot verify (a branch decides the register) and that were read from the asm by hand
    for name in ("passthrough_calls.txt", "passthrough_calls_manual.txt"):
        path = ROOT / "config" / name
        if not path.is_file():
            continue
        for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) < 5:
                die(f"config/{name}:{n}: expected unit, function, callee, call number, arguments")
            rows.append((n, parts[0], parts[1], parts[2], int(parts[3]), parts[4]))
    return rows


def apply_passthrough_calls(mirror):
    applied = 0
    groups = {}
    params = {}
    for n, unit, fn, callee, nth, args in read_passthrough_calls():
        if callee == "@param":
            params[(unit, fn)] = (n, nth, args)
            continue
        groups.setdefault((unit, fn, callee), []).append((n, nth, args))
    # a function that gains parameters: its definition (which must have exactly `have` parameters), and every
    # declaration of it in the tree, which takes the new ones as `void *` too
    if params:
        files = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h")))
        for (unit, fn), (n, have, args) in params.items():
            path = mirror / unit
            if not path.is_file():
                die(f"config/passthrough_calls.txt:{n}: {unit} is not in the mirror")
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            pat = re.compile(r"((?:^|\n)[A-Za-z_][^\n;{}]*?\b" + re.escape(fn) + r"\s*\()([^;{}]*)(\)\s*\{)")
            m = pat.search(text)
            if not m or len([] if m.group(2).strip() in ("", "void") else _split_params(m.group(2))) != have:
                die(f"config/passthrough_calls.txt:{n}: definition of {fn} with {have} parameters not found in {unit} (the decomp changed?)")
            old = [] if m.group(2).strip() in ("", "void") else _split_params(m.group(2))
            path.write_text(text[:m.start(2)] + ", ".join(old + [args]) + text[m.end(2):], encoding="utf-8", errors="surrogateescape")
            added = len(_split_params(args))
            decl = re.compile(r"^([ \t]*(?:extern[ \t]+)?[A-Za-z_][\w \t\*]*?[ \t\*]" + re.escape(fn) + r"[ \t]*\()([^;{}()]*)(\)[ \t]*;)", re.M)

            def widen_decl(dm, have=have, added=added):
                if re.match(r"\s*(?:return|else|case|goto|do)\b", dm.group(1)):
                    return dm.group(0)                      # a call statement, not a declaration
                if not dm.group(2).strip():
                    return dm.group(0)                      # unprototyped: callers may pass any count
                dp = [] if dm.group(2).strip() == "void" else _split_params(dm.group(2))
                want = max(len(dp), have + added)
                return dm.group(1) + ", ".join(dp[:have] + ["void *"] * (want - min(len(dp), have))) + dm.group(3)
            for f in files:
                t = f.read_text(encoding="utf-8", errors="surrogateescape")
                if fn in t and decl.search(t):
                    f.write_text(decl.sub(widen_decl, t), encoding="utf-8", errors="surrogateescape")
            applied += 1
    by_unit = {}
    for (unit, fn, callee), rows in groups.items():
        by_unit.setdefault(unit, []).append((fn, callee, rows))
    for unit, entries in by_unit.items():
        path = mirror / unit
        if not path.is_file():
            die(f"config/passthrough_calls.txt: {unit} is not in the mirror")
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        edits = []
        for fn, callee, rows in entries:
            m = re.search(r"(?:^|\n)[A-Za-z_][^\n;{}]*?\b" + re.escape(fn) + r"\s*\([^;{}]*\)\s*\{", text)
            if not m:
                die(f"config/passthrough_calls.txt:{rows[0][0]}: function {fn} not found in {unit} (the decomp changed?)")
            body_open = m.end() - 1
            body_end = _match_brace(text, body_open)
            stripped = _strip_literals(text[body_open:body_end])
            calls = []
            for cm in re.finditer(r"\b" + re.escape(callee) + r"\s*\(", stripped):
                k = cm.start() - 1
                while k >= 0 and stripped[k] in " \t\n":
                    k -= 1
                if k >= 0 and (stripped[k] in "*." or ((stripped[k].isalnum() or stripped[k] == "_")
                                                      and re.search(r"(\w+)$", stripped[:k + 1]).group(1) not in _CALL_KEYWORDS)):
                    continue
                _, close = _split_args_at(stripped, cm.end() - 1)
                calls.append((body_open + cm.end(), body_open + close))
            for n, nth, args in rows:
                if nth > len(calls):
                    die(f"config/passthrough_calls.txt:{n}: {fn} has {len(calls)} calls to {callee}, not {nth} (the decomp changed?)")
                edits.append((calls[nth - 1][0], calls[nth - 1][1], args))
                applied += 1
        for a, b, args in sorted(edits, reverse=True):
            text = text[:a] + args + text[b:]
        path.write_text(text, encoding="utf-8", errors="surrogateescape")
    # A typed declaration of a callee that takes fewer parameters than a call now passes becomes unprototyped,
    # which the prototype pass then replaces by the definition's when every call agrees.
    need = {}
    for (unit, fn, callee), rows in groups.items():
        need[callee] = max(need.get(callee, 0), max(len(_split_params(args)) for _, _, args in rows))
    if need:
        pats = {c: re.compile(r"^([ \t]*(?:extern[ \t]+)?[A-Za-z_][\w \t\*]*?[ \t\*]" + re.escape(c) + r"[ \t]*\()([^()]*)(\)[ \t]*;)", re.M)
                for c in need}
        for f in sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h"))):
            t = f.read_text(encoding="utf-8", errors="surrogateescape")
            new_t = t
            for c, pat in pats.items():
                if c not in new_t:
                    continue

                def widen(m, want=need[c]):
                    if re.match(r"\s*(?:return|else|case|goto|do)\b", m.group(1)) or "..." in m.group(2):
                        return m.group(0)
                    have = [] if m.group(2).strip() in ("", "void") else _split_params(m.group(2))
                    return m.group(0) if len(have) >= want or not m.group(2).strip() else m.group(1) + m.group(3)
                new_t = pat.sub(widen, new_t)
            if new_t != t:
                f.write_text(new_t, encoding="utf-8", errors="surrogateescape")
    return applied


# ---------------------------------------------------------------- overrides

def conditional_block_end(text, start):
    """Index just past the `#endif` that closes the `#if` opened at `start`, or None."""
    depth, i = 0, start
    while i < len(text):
        end = line_end(text, i)
        s = text[i:end].strip()
        if s.startswith("#if"):
            depth += 1
        elif s.startswith("#endif"):
            depth -= 1
            if depth == 0:
                return end
        i = end
    return None


def override_signature(body, addr8):
    """The address-named override's signature; named caller ABIs are reconciled later."""
    names = re.escape(f"func_{addr8}")
    m = re.search(r"(?m)^([A-Za-z_][\w \t*]*(?:\n[ \t*]*)?\b(" + names +
                  r")[ \t]*\([^{};]*\))\s*\{", body)
    if m is None:
        m = re.search(r"(?m)^([A-Za-z_][\w \t*]*\([ \t]*\*+[ \t]*(" + names +
                      r")\([^{};]*?\)\s*\)\s*\([^{};]*\))\s*\{", body)
    return (m[2], " ".join(m[1].split())) if m else None



def apply_overrides(root, mirror_root, units):
    overrides = root / "overrides"
    texts, orig = {}, {}
    excluded_tokens = None
    signatures = {}

    def load(p):
        if p not in texts:
            orig[p] = p.read_text(encoding="utf-8", errors="surrogateescape")
            texts[p] = orig[p]
        return texts[p]

    unit_files = [mirror_root / u for u in units]

    marker_index = {}

    def markers(p):
        t = load(p)
        cached = marker_index.get(p)
        if cached is None or cached[0] is not t:
            cached = (t, [(int(mk[1], 16), mk.end()) for mk in MARKER_RE.finditer(t)])
            marker_index[p] = cached
        return cached[1]

    # overrides/<addr8>.c replaces the definition after `// FUN_<ADDR>`, through its closing brace.
    # Single-line extern declarations between the marker and the definition are kept.
    if (overrides).is_dir():
        for f in sorted(overrides.glob("*.c")):
            m = OVERRIDE_RE.match(f.name)
            if not m:
                die(f"{f}: marker overrides must be named <lowercase addr8>.c")
            addr = int(m[1], 16)
            body = f.read_text(encoding="utf-8")
            hits = []
            for p in unit_files:
                hits.extend((p, end) for a, end in markers(p) if a == addr)
            if not hits:
                # Marker exists only in an excluded unit: the override waits for that unit.
                if excluded_tokens is None:
                    excluded_tokens = set()
                    for p in sorted((mirror_root / "src").rglob("*.c")):
                        excluded_tokens.update(FUN_TOKEN_RE.findall(load(p)))
                if f"FUN_{m[1].upper()}" in excluded_tokens:
                    # The only C for it is in an excluded alternate copy (`X_cw119.c` next to the built `X.c`, which
                    # has no definition of it, so retail's asm was a trap). The override joins the built sibling.
                    owner = next((q for q in sorted((mirror_root / "src").rglob("*.c"))
                                  if f"FUN_{m[1].upper()}" in FUN_TOKEN_RE.findall(load(q))), None)
                    sibling = owner.with_name(re.sub(r"_cw\d+\.c$", ".c", owner.name)) if owner else None
                    if sibling is not None and sibling != owner and sibling in unit_files:
                        load(sibling)
                        texts[sibling] = texts[sibling].rstrip("\n") + (
                            f"\n\n/* override {f.name}: FUN_{m[1].upper()} exists only in {owner.name}, an excluded alternate copy */\n"
                            + body.rstrip("\n") + "\n")
                        sig = override_signature(body, m[1])
                        if sig is not None:
                            signatures[sig[0]] = sig[1]
                        continue
                    print(f"gen_port: note: {f.name} skipped; FUN_{m[1].upper()} is in an excluded unit", file=sys.stderr)
                    continue
                die(f"{f.name}: no `// FUN_{m[1].upper()}` marker in the decomp tree")
            if len(hits) > 1:
                die(f"{f.name}: marker FUN_{m[1].upper()} appears {len(hits)} times in compiled units")
            p, mk_end = hits[0]
            t = texts[p]
            start = line_end(t, mk_end)
            while True:
                nxt = t[start:line_end(t, start)].strip()
                if not (nxt.startswith("extern ") and nxt.endswith(";")):
                    break
                start = line_end(t, start)
            nxt = t[start:line_end(t, start)].strip()
            if nxt.startswith("#if"):
                # A gated block (`#ifdef NON_MATCHING` C body, `#else` INCLUDE_ASM, `#endif`) is replaced
                # whole. Keeping the gate would leave its `#else` and `#endif` without an opener.
                end = conditional_block_end(t, start)
                if end is None:
                    die(f"{f.name}: FUN_{m[1].upper()} opens a conditional that is never closed")
            elif nxt.startswith("INCLUDE_ASM(") and nxt.endswith(";"):
                # The promoted unit kept the asm marker: the override body replaces that line.
                end = line_end(t, start)
            else:
                close = definition_end(t, start)
                if close is None:
                    die(f"{f.name}: no function definition follows FUN_{m[1].upper()} in {p.relative_to(mirror_root)}")
                end = line_end(t, close)
            texts[p] = t[:start] + body.rstrip("\n") + "\n" + t[end:]
            # A block-scope prototype of the replaced function is a placeholder from the
            # INCLUDE_ASM era. It conflicts with the override's signature, and uses after
            # the definition take its type, so the placeholder is removed.
            body_end = start + len(body.rstrip("\n")) + 1
            proto = re.compile(r"(?m)^[ \t]+extern[ \t][^;{}\n]*\bfunc_%s[ \t]*\([^;{}]*\)[ \t]*;[ \t]*\n" % m[1])
            texts[p] = texts[p][:body_end] + proto.sub("", texts[p][body_end:])
            sig = override_signature(body, m[1])
            if sig is not None:
                signatures[sig[0]] = sig[1]

    # Declarations of an overridden function keep the INCLUDE_ASM-era `(void)` type, which conflicts with
    # the override's signature in the same unit. Every declaration (with or without `extern`) takes the override's signature.
    # Do not erase named prototypes here: they can express the EE caller's argument order.
    # The later ABI pass needs that order to rearrange calls (e.g. effMiscAxisAngleVU).
    if signatures:
        for q in sorted(list((mirror_root / "src").rglob("*.c")) + list((mirror_root / "src").rglob("*.h"))
                        + list((mirror_root / "include").rglob("*.h"))):
            t = load(q)
            new = t
            for name, sig in signatures.items():
                if name in new:
                    new = re.sub(r"(?m)^([ \t]*)(?:extern[ \t]+)?(?!return\b|else\b|case\b)(?:[A-Za-z_]\w*[ \t\*]+)+" + re.escape(name) + r"[ \t]*\([^;{}]*\)[ \t]*;",
                                 lambda mm, s=sig: mm[1] + "extern " + s + ";", new)
            if new != t:
                texts[q] = new

    # overrides/static/<name>.c replaces every definition of the named static function.
    static_dir = overrides / "static"
    if static_dir.is_dir():
        files = sorted(list((mirror_root / "src").rglob("*.c")) + list((mirror_root / "src").rglob("*.h"))
                       + list((mirror_root / "include").rglob("*.h")))
        for f in sorted(static_dir.glob("*.c")):
            name = f.stem
            if not re.fullmatch(r"[A-Za-z_]\w*", name):
                die(f"{f}: static override name {name!r} is not an identifier")
            body = f.read_text(encoding="utf-8").rstrip("\n") + "\n"
            pat = re.compile(r"^[ \t]*static\b[^;{}()]*?\b" + re.escape(name) + r"\s*\(", re.M)
            sites = []
            for p in files:
                t = load(p)
                for m in pat.finditer(t):
                    close = definition_end(t, m.start())
                    if close is None:
                        continue
                    sites.append((p, m.start(), line_end(t, close), " ".join(t[m.start():close].split())))
            if not sites:
                die(f"{name}: no static definition found for override {f.name}")
            distinct = {s[3] for s in sites}
            if len(distinct) > 1:
                die(f"{name}: {len(distinct)} differing static definitions; the override is ambiguous")
            for p in sorted({s[0] for s in sites}):
                t = texts[p]
                for _, ls, le, _ in sorted((s for s in sites if s[0] == p), key=lambda s: s[1], reverse=True):
                    t = t[:ls] + body + t[le:]
                texts[p] = t

    for p, t in texts.items():
        if t != orig[p]:
            p.write_text(t, encoding="utf-8", errors="surrogateescape", newline="\n")
    return len([p for p in texts if texts[p] != orig[p]])


# ---------------------------------------------------------------- port-owned RenderWare storage

# The libc FILE streams resolve through the link to libc, never to port storage.
LIBC_STREAMS = {"stdin", "stdout", "stderr"}
EXTERN_DECL_RE = re.compile(r"^[ \t]*extern\s+([^;()\[\]]*?)\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:__RWUNUSED__\s*)?;")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)")
# Port-owned storage with a fixed initializer: (initializer, note for the definition's comment).
PORT_STORAGE_INIT = {"_rwBuildDateTime": ('"Persona4-PC"', "RenderWare build banner: port text, not the retail string")}
FUNC_ALIAS_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(\S+)\s*$")


def strip_comment(line):
    if "/*" not in line and "//" not in line:
        return line
    return re.sub(r"/\*.*?\*/", " ", line).split("//", 1)[0]


def port_definition(line, name):
    """The extern declaration as the port's definition line; PORT_STORAGE_INIT adds an initializer."""
    if name not in PORT_STORAGE_INIT:
        return re.sub(r"\bextern\s+", "", line, count=1)
    init, note = PORT_STORAGE_INIT[name]
    code = re.sub(r"\bextern\s+", "", strip_comment(line), count=1).strip()
    if not code.endswith(";"):
        die(f"{name}: extern declaration does not end in ';': {line.strip()!r}")
    return f"{code[:-1].rstrip()} = {init};  /* {note} */"


def read_port_owned_names():
    """config/port_owned.txt: the RenderWare extern storage the port defines. One C identifier
    per line; `#` starts a comment, and the comment holds the decomp's extern declaration."""
    path = ROOT / "config" / "port_owned.txt"
    out = []
    for n, raw in enumerate(path.read_text().splitlines(), 1):
        name = raw.split("#", 1)[0].strip()
        if not name:
            continue
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            die(f"{path}:{n}: expected one C identifier, got {raw.strip()!r}")
        if name in out:
            die(f"{path}:{n}: {name} is listed twice")
        out.append(name)
    return out


def check_port_owned(decomp, names):
    """Each listed name must still be a file-scope extern declaration in the decomp's src/ or
    include/. A name the decomp no longer declares is stale: remove it from config/port_owned.txt.
    Whether the decomp also defines it depends on preprocessor branches (RWGLOBALSIZE) that a
    text scan cannot follow, so the tables step checks the compiled objects instead."""
    wanted_re = re.compile(r"\b(" + "|".join(sorted(names)) + r")\b")
    declared = set()
    for sub in ("src", "include"):
        for f in sorted((decomp / sub).rglob("*")):
            if not (f.is_file() and f.suffix in (".c", ".h")):
                continue
            for line in f.read_text(encoding="utf-8", errors="surrogateescape").splitlines():
                code = strip_comment(line)
                if not wanted_re.search(code):
                    continue
                for name in set(wanted_re.findall(code)):
                    if re.match(rf"\s*extern\b[^;(]*\b{name}\b", code):
                        declared.add(name)
    for name in names:
        if name not in declared:
            die(f"{name}: no file-scope extern declaration in the decomp's src/ or include/; remove it from config/port_owned.txt")


def port_owned_storage(decomp, mirror, units):
    """Define the RenderWare globals that config/port_owned.txt lists. The decomp declares them
    `extern` and nothing defines them. Each is defined once, with its extern declaration's type,
    in the first compiled RenderWare unit that declares it. A listed name with a `#define` alias
    is a hard error: it would give one variable two names. Returns [(name, unit)]."""
    names = read_port_owned_names()
    known = set(read_symbols(decomp)) | set(read_port_symbols())
    for name in names:
        if name in known:
            die(f"{name} is port-owned RenderWare storage but is also known by address; remove one of them")
        if name in LIBC_STREAMS or DATA_NAME_RE.match(name) or GP_NAME_RE.match(name) or FUNC_NAME_RE.match(name):
            die(f"{name} is not RenderWare storage; remove it from config/port_owned.txt")
    check_port_owned(decomp, names)
    texts = {}
    for sub in ("src", "include"):
        for f in sorted((mirror / sub).rglob("*")):
            if f.is_file() and f.suffix in (".c", ".h"):
                texts[f.relative_to(mirror).as_posix()] = f.read_text(encoding="utf-8", errors="surrogateescape")
    aliased = set()
    for text in texts.values():
        for line in text.splitlines():
            dm = DEFINE_RE.match(strip_comment(line))
            if dm:
                aliased.add(dm[1])
    sites = {}
    for u in units:
        if u.startswith("src/renderware/") and u in texts:
            for i, line in enumerate(texts[u].split("\n")):
                m = EXTERN_DECL_RE.match(strip_comment(line))
                if m:
                    sites.setdefault(m[2], []).append((u, i))
    owned, edited = [], {}
    for name in names:
        if name in aliased:
            die(f"{name}: RenderWare extern storage is also a #define alias; it cannot be port-owned")
        if name not in sites:
            die(f"{name}: no extern declaration in a compiled RenderWare unit")
        unit, i = sites[name][0]
        if unit not in edited:
            edited[unit] = texts[unit].split("\n")
        edited[unit][i] = port_definition(edited[unit][i], name)
        owned.append((name, unit))
    for unit in sorted(edited):
        (mirror / unit).write_text("\n".join(edited[unit]), encoding="utf-8", errors="surrogateescape", newline="\n")
    return owned

# ---------------------------------------------------------------- compile flags

def _kv_lines(path):
    out = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        s = line.split("#", 1)[0].strip()
        if not s:
            continue
        key, *flags = s.split()
        out.setdefault(key, []).extend(flags)
    return out


# Appended after the decomp's own flags (which name -Wincompatible-pointer-types): newer clang
# (llvm-mingw's, 23) makes that warning an error by default, and the m2c-typed decomp relies on
# it staying a warning, as it is under the Linux build's clang 19.
# MWCC's anonymous typedef members (`ResrcHeader;`) promote the base's fields. Clang
# implements that same layout and member lookup through its Microsoft-extensions mode.
PORT_UNIT_FLAGS = ["-Wno-error=incompatible-pointer-types", "-fms-extensions"]


def translate(tokens, mirror_dir):
    out = []
    for tok in tokens:
        if tok in DROP_FLAGS:
            continue
        if tok.startswith("-I"):
            out.append("-I" + (mirror_dir / tok[2:]).as_posix())
            continue
        if tok == "-O4":
            out.append("-O3")  # clang has no -O4
            continue
        out.append(tok)
    return out + PORT_UNIT_FLAGS


def unit_flags(decomp, build, units):
    base = [l.strip() for l in (decomp / "compile_flags.txt").read_text().splitlines() if l.strip()]
    unit_key = {}
    for line in read_list(decomp / "config/compiler_units.txt"):
        parts = line.split()
        if len(parts) == 2:
            unit_key[parts[0]] = parts[1]
    versions = _kv_lines(decomp / "config/version_flags.txt")
    mirror_dir = build / "decomp"
    lines = []
    for u in units:
        extra = versions.get(unit_key.get(u, ""), [])
        flags = translate(base + extra, mirror_dir)
        bad = [f for f in flags if "\t" in f or ";" in f]
        if bad:
            die(f"{u}: flags with tab or ';' cannot pass through units.txt: {bad}")
        lines.append(f"{(mirror_dir / u).as_posix()}|" + "\t".join(flags))
    return lines


# ---------------------------------------------------------------- configure

def cmd_units(a):
    """Configure-time part: the unit list and each unit's flags. Cheap; CMake reruns it only when
    its inputs change. The mirror itself is `mirror`, a build-time step."""
    decomp, build = Path(a.decomp), Path(a.build)
    units, excluded = read_units(ROOT)
    for u in units:
        if not (decomp / u).is_file():
            die(f"unit {u} is not in the decomp checkout")
    lines = unit_flags(decomp, build, units)
    write_if_changed(build / "units.txt", "\n".join(lines) + "\n")
    write_if_changed(build / "excluded.txt",
                     "".join(f"{u}\t{why}\n" for u, why in sorted(excluded.items())))
    print(f"gen_port: units: {len(units)} units, {len(excluded)} excluded")


# A decomp unit that defines a retail-addressed name (`static s32 iGpffffb9e0;`, `f32 fGpffff83bc = 8.1f;`,
# `u8 *D_0072449C;`) gives the host its own copy, while every other unit and the retail data refer to the
# name's retail address: the writer and the reader then see different storage. The retail image holds the
# initial value at that address, so a single-line definition of such a name becomes an extern declaration.
# Multi-line initialisers and names with another meaning are left alone (reported by the count).
RETAIL_DEF_RE = re.compile(
    r"^(?P<static>static[ \t]+)?(?P<type>[A-Za-z_][^;=(){}]*?[ \t\*])(?P<name>\w+)(?P<dims>(?:\[[^\]]*\])*)"
    r"[ \t]*(?:=[^;{]*)?;[ \t]*(?://.*|/\*.*\*/)?$")
RETAIL_FPTR_RE = re.compile(
    r"^(?P<static>static[ \t]+)?(?P<ret>[A-Za-z_][^;=(){}]*?)\(\*[ \t]*(?P<name>\w+)\)(?P<args>[ \t]*\([^;{}]*\))"
    r"[ \t]*(?:=[^;{]*)?;[ \t]*(?://.*|/\*.*\*/)?$")


def externalize_retail_data(mirror):
    count, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "Gp" not in text and "D_0" not in text and "DAT_" not in text:
            continue
        out, changed = [], 0
        for line in text.split("\n"):
            m = None
            if line[:1] not in ("", " ", "\t", "#", "/", "}", "{") and not line.startswith(("extern ", "typedef ", "return ")):
                m = RETAIL_FPTR_RE.match(line) or RETAIL_DEF_RE.match(line)
            if m and (GP_NAME_RE.match(m.group("name")) or DATA_NAME_RE.match(m.group("name"))):
                tail = ""
                cm = re.search(r"[ \t]*(//.*|/\*.*\*/)[ \t]*$", line)
                if cm:
                    tail = " " + cm.group(1)
                if "args" in m.groupdict() and m.groupdict().get("args") is not None:
                    decl = f"{m.group('ret')}(*{m.group('name')}){m.group('args').strip()}"
                else:
                    decl = f"{m.group('type')}{m.group('name')}{m.group('dims')}"
                out.append(f"extern {decl};{tail}")
                changed += 1
                continue
            out.append(line)
        if changed:
            path.write_text("\n".join(out), encoding="utf-8", errors="surrogateescape")
            count += changed
            files += 1
    return count, files


# The decomp calls `kwlnTaskGetWork()` with no argument where retail passes the task handle the
# function itself received: $a0 is still live, so the call needs no explicit argument on the PS2. On
# the host an empty argument list passes nothing. The pass hands the call the function's first integer
# parameter, and gives a `(void)` function (the decomp's shdWindowMarkDone) that parameter. It also makes
# a void function that ends in a kwlnTaskCreateChild call return that call's handle. The allocator's free slot
# (0x008873EC) is called the same way: `jtbl_008873EC[0]()` frees the pointer left in $a0.
TASKWORK_CALL_RE = re.compile(r"\bkwlnTask(?:GetWork|IsLinked)\(\)|\(\*jtbl_008873EC\)\(\)|\b\w*8873[eE][cC](?:_abs)?\[0\]\(\)")
TASKWORK_DEF_RE = re.compile(r"^(?P<head>[A-Za-z_][\w \t\*]*?\b(?P<name>\w+))\((?P<params>[^()]*)\)(?P<tail>[ \t]*\{?[ \t]*)$")


def pass_taskwork_argument(mirror):
    count, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if not TASKWORK_CALL_RE.search(text) and "kwlnTaskCreate" not in text:
            continue
        lines = text.split("\n")
        i, changed = 0, 0
        while i < len(lines):
            m = TASKWORK_DEF_RE.match(lines[i]) if lines[i][:1].isalpha() else None
            if not m or m.group("name") in ("if", "while", "for", "switch", "return", "sizeof"):
                i += 1
                continue
            start = i
            if "{" not in m.group("tail"):
                j = i + 1
                if j >= len(lines) or lines[j].strip() != "{":
                    i += 1
                    continue
                i = j
            end = i + 1
            while end < len(lines) and lines[end] != "}":
                end += 1
            if end >= len(lines):
                break
            body = range(i + 1, end)
            hits = [k for k in body if TASKWORK_CALL_RE.search(lines[k]) and not lines[k].lstrip().startswith("extern ")]
            if hits:
                params = [q.strip() for q in m.group("params").split(",") if q.strip() not in ("", "void")]
                name = None
                for q in params:
                    if re.match(r"(?:const\s+)?(?:f32|float|f64|double)\b", q):
                        continue
                    pm = re.search(r"(\w+)\s*(?:\[[^\]]*\])?$", q)
                    if pm and " " in q.replace("*", " * ").strip() or (pm and "*" in q):
                        name = pm.group(1)
                        break
                if name is None:
                    name = "p4_task"
                    params.insert(0, "s32 p4_task")
                    lines[start] = f"{m.group('head')}({', '.join(params)}){m.group('tail')}"
                for k in hits:
                    new = TASKWORK_CALL_RE.sub(lambda m: m.group(0)[:-1] + name + ")", lines[k])
                    if new != lines[k]:
                        lines[k] = new
                        changed += 1
            # A void function whose last statement is the task-creating call returns its handle in $v0
            # on the PS2 (the caller keeps it); on the host the call's value is dropped.
            last = end - 1
            while last > i and not lines[last].strip():
                last -= 1
            hm = re.match(r"^(?:void|s32|u32|int)[ \t]+(?!\*)", lines[start])
            if hm and re.match(r"^    (?:\(s32\))?kwlnTaskCreate(?:Child)?\(", lines[last]) and lines[last].rstrip().endswith(");"):
                if lines[start].startswith("void"):
                    lines[start] = "s32" + lines[start][len("void"):]
                lines[last] = "    return " + lines[last].lstrip().replace("(s32)", "", 1)
                changed += 1
            i = end + 1
        if changed:
            # The free slot's declaration `void (*X[])(void)` rejects the argument now passed.
            lines = [re.sub(r"(\(\*\w*8873[eE][cC](?:_abs)?\[\]\))\(void\)", r"\1()", l) for l in lines]
            path.write_text("\n".join(lines), encoding="utf-8", errors="surrogateescape")
            count += changed
            files += 1
    return count, files


# A decomp-local struct with the RwMatrix field layout (right/up/at/pos) is passed to RenderWare as an
# RwMatrix, whose host code uses 16-byte aligned SSE loads and stores; on the stack the struct is only
# 4-byte aligned, and RwMatrixRotate faults. The file-level RwMatrix typedefs are fixed by mirror rows;
# this gives the matrix-shaped local structs the same alignment.
MATRIX_STRUCT_RE = re.compile(r"\btypedef[ \t]+struct[^{};]*\{")


# The same for a plain `struct RwMatrixTag { right; up; at; pos };` that a unit defines next to its own typedef.
PLAIN_MATRIX_STRUCT_RE = re.compile(
    r"(?<!typedef )\bstruct(\s+)(RwMatrixTag|RwMatrix)(\s*\{[^{}]*\bright\b[^{}]*\bup\b[^{}]*\bat\b[^{}]*\bpos\b[^{}]*\})")


# A struct whose first member is a 16-float matrix (`struct Frame { f32 matrix[16]; ... } frame;`, passed to RwMatrixRotate)
# is in the same position: its address is the matrix's.
FLOAT16_FIRST_STRUCT_RE = re.compile(r"\bstruct(\s+\w+)?(\s*)\{([ \t\r\n]*(?:f32|float)[ \t]+\w+\[16\][ \t]*;)")


def align_matrix_structs(mirror):
    count, files = 0, 0
    # Headers too: scene_light_override_internal.h defines SceneLightMatrix (a 64-byte RwMatrix) for local variables.
    for path in sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "include").rglob("*.h"))):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        if "right" not in text and "RwMatrix" not in text and "[16]" not in text:
            continue
        new = text
        # The shared RwMatrixTag now exposes word views through nested anonymous unions.
        # Balance braces instead of silently losing that matrix to a flat-body regex.
        for m in reversed(list(MATRIX_STRUCT_RE.finditer(text))):
            end = _match_brace(text, m.end() - 1)
            alias = re.match(r"\s*(\w+)[ \t]*;", text[end:])
            body = _strip_literals(text[m.start():end])
            if (alias and "aligned" not in text[m.start():m.end()]
                    and (alias[1] in ("RwMatrixTag", "RwMatrix") or "{" not in text[m.end():end - 1])
                    and re.search(r"\bright\b[\s\S]*\bup\b[\s\S]*\bat\b[\s\S]*\bpos\b", body)):
                stop = end + alias.end()
                new = new[:end] + f" __attribute__((aligned(16))) {alias[1]};" + new[stop:]
                count += 1
        # A local word array used as an RwMatrix (`u32 auStack_40[16]; RwMatrixRotate((RwMatrix *)auStack_40, ...)`).
        for m in list(re.finditer(r"^([ \t]+)(?:u32|s32|f32|u8|s8)[ \t]+(\w+)\[(0x[0-9a-fA-F]+|\d+)\];", new, re.M)):
            name = m.group(2)
            if m.group(3).lower() in ("16", "64", "0x10", "0x40") and re.search(r"\((?:const[ \t]+)?(?:struct[ \t]+)?RwMatrix(?:Tag)?[ \t]*\*\)[ \t]*\(*[ \t]*&?[ \t]*" + re.escape(name) + r"\b", new):
                new = new.replace(m.group(0), m.group(0)[:-1] + " __attribute__((aligned(16)));", 1)
                count += 1

        new = FLOAT16_FIRST_STRUCT_RE.sub(lambda m: m.group(0) if "aligned" in m.group(0) else
                                          f"struct __attribute__((aligned(16))){m.group(1) or ''}{m.group(2)}{{{m.group(3)}", new)
        new = PLAIN_MATRIX_STRUCT_RE.sub(lambda m: m.group(0) if "aligned" in m.group(0) else
                                         f"struct __attribute__((aligned(16))){m.group(1)}{m.group(2)}{m.group(3)}", new)
        if new != text:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            files += 1
    return count, files


# A function defined to return s8/s16/u8/u16 leaves the value in %al/%ax on x86 with the upper register bits
# undefined; the retail EE returns it sign- or zero-extended in the full register. Callers in other units that
# declare the function with a wider return type (`extern s32 itfMesWindowState();`, which the decomp does on
# purpose because retail "consumes the full register") then test garbage upper bits: a window state of -1 read
# as 255 and the first event's message never finished. Declarations of such a function get its own return type,
# so the host caller extends the narrow value itself.
NARROW_DEF_RE = re.compile(r"^(?P<ret>s8|s16|u8|u16)[ \t]+(?P<name>\w+)[ \t]*\((?P<params>[^;{}]*)\)[ \t]*\r?\n?[ \t]*\{", re.M)
NARROW_DECL_RE = re.compile(
    r"^(?P<ind>[ \t]*)(?P<ext>extern[ \t]+)?(?P<ret>s32|int|u32|s16|u16|s8|u8)(?P<sp>[ \t]+)(?P<name>\w+)(?P<rest>[ \t]*\([^{};]*\)[ \t]*;)",
    re.M)

def _narrow_declaration_insertion(text, names):
    # Overrides may introduce #includes after an earlier function has already
    # called the narrow-return function. Only includes preceding the first
    # occurrence can anchor a declaration that is visible to every caller.
    first = min(re.search(r"\b" + re.escape(name) + r"\b", text).start() for name in names)
    includes = list(re.finditer(r"^#[ \t]*include[^\n]*\n", text[:first], re.M))
    return includes[-1].end() if includes else 0



def align_narrow_returns(mirror):
    defs = {}
    promoted = set()
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for m in NARROW_DEF_RE.finditer(text):
            defs.setdefault(m.group("name"), set()).add(m.group("ret"))
            if re.search(r"\b(?:s8|u8|s16|u16|f32|float|double|char|short)\b[ \t]+\w+[ \t]*(?:,|$)", m.group("params").strip()) and not re.search(r"[\*]", m.group("params").split(",")[0]) or re.search(r"\b(?:s16|u16|s8|u8|f32|float|double|char|short)\b[ \t]+\w+", m.group("params")):
                promoted.add(m.group("name"))
    defs = {n: next(iter(t)) for n, t in defs.items() if len(t) == 1}
    # Names a header already declares keep that (typed) declaration: a K&R one here would clash with its
    # promoted parameter types.
    header_names = set()
    for hdr in sorted((mirror / "include").rglob("*.h")) if (mirror / "include").is_dir() else []:
        htext = hdr.read_text(encoding="utf-8", errors="surrogateescape")
        for m in re.finditer(r"\b(\w+)[ \t]*\([^;{}]*\)[ \t]*;", htext):
            header_names.add(m.group(1))
    for nm in header_names:
        defs.pop(nm, None)
    call_re = re.compile(r"\b(" + "|".join(map(re.escape, sorted(defs))) + r")[ \t]*\(")
    count, files = 0, 0
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        n = 0

        def sub(m):
            nonlocal n
            want = defs.get(m.group("name"))
            if want is None or want == m.group("ret"):
                return m.group(0)
            n += 1
            return f"{m.group('ind')}{m.group('ext') or ''}{want}{m.group('sp')}{m.group('name')}{m.group('rest')}"

        new = NARROW_DECL_RE.sub(sub, text)
        # Calls with no declaration at all in the unit are implicit `int f()`: declare them too.
        declared = {m.group("name") for m in NARROW_DECL_RE.finditer(new)}
        defined_here = {m.group("name") for m in NARROW_DEF_RE.finditer(new)}
        implicit = []
        for m in call_re.finditer(new):
            nm = m.group(1)
            if nm in declared or nm in defined_here or nm in implicit or nm in promoted:
                continue
            # Any declaration form already in the unit (a void or pointer return, a typedef'd type) wins.
            if re.search(r"^[^\n=(){}]*\b" + re.escape(nm) + r"[ \t]*\([^;{}]*\)[ \t]*;", new, re.M):
                continue
            implicit.append(nm)
        if implicit:
            at = _narrow_declaration_insertion(new, implicit)
            new = new[:at] + "".join(f"extern {defs[nm]} {nm}();\n" for nm in implicit) + new[at:]
            n += len(implicit)
        if n:
            # A call that precedes the declaration is an implicit `int f()`; the narrow declaration must come
            # first or the two conflict. Put it after the last #include of the file's head.
            hoist = []
            for m in NARROW_DECL_RE.finditer(new):
                want = defs.get(m.group("name"))
                if want is None or want != m.group("ret"):
                    continue
                first = re.search(r"\b" + re.escape(m.group("name")) + r"\b", new)
                if first and first.start() < m.start() and m.group("name") not in [h[0] for h in hoist]:
                    hoist.append((m.group("name"), want, m.group("rest").strip()))
            if hoist:
                at = _narrow_declaration_insertion(new, [item[0] for item in hoist])
                new = new[:at] + "".join(f"extern {w} {nm}{rest}\n" for nm, w, rest in hoist) + new[at:]
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            count += n
            files += 1
    return count, files, len(defs)


# A void function whose whole body is one call to a value-returning function is a retail tail call
# (`j callee`): the callee's $v0 is the wrapper's result, and callers declare it with the real return
# type locally (`frFontDrawListDefault(second) > 0`). Written void, the host wrapper drops the value and
# the caller reads garbage (the message window's text never reported done). The wrapper returns the call.
TAILRET_DEF_RE = re.compile(
    r"^(?P<head>void)(?P<sp>[ \t]+)(?P<name>\w+)\((?P<params>[^;{}]*)\)(?P<br>[ \t]*\r?\n?[ \t]*)\{(?P<ws1>[ \t]*\r?\n)"
    r"(?P<ind>[ \t]+)(?P<call>(?P<callee>\w+)\([^;{}]*\));(?P<ws2>[ \t]*\r?\n)\}", re.M)
TAILRET_RET_RE = re.compile(r"^(?:extern[ \t]+)?(?P<ret>s32|int|u32|s16|u16|s8|u8|Sint32|Uint32)[ \t]+(?P<name>\w+)\(", re.M)


def _spell_int(ret, text):
    """Use only integer spellings visible in this C file or shared header."""
    base = {"int": "s32", "unsigned int": "u32", "Sint32": "s32", "Uint32": "u32"}.get(ret, ret)
    if base not in ("s32", "u32"):
        return base
    if re.search(r"\b(?:s32|u32|u8|s8|u16|s16)\b", text):
        return base
    if re.search(r"\b(?:Sint32|Uint32|Sint16|Uint16|Sint8|Uint8)\b|#[ \t]*include[^\n]*cri_xpt\.h", text):
        return {"s32": "Sint32", "u32": "Uint32"}[base]
    return {"s32": "int", "u32": "unsigned int"}[base]


def _tail_call_round(mirror):
    rets = {}
    texts = {}
    for path in sorted((mirror / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        texts[path] = text
        for m in re.finditer(r"^(?P<ret>s32|unsigned[ \t]+int|int|u32|s16|u16|s8|u8|Sint32|Uint32)[ \t]+(?P<name>\w+)\([^;{}]*\)[ \t]*\r?\n?[ \t]*\{", text, re.M):
            rets.setdefault(m.group("name"), set()).add(m.group("ret"))
    rets = {n: next(iter(t)) for n, t in rets.items() if len(t) == 1}
    changed = {}
    used_callees = {}
    for path, text in texts.items():
        def sub(m):
            ret = rets.get(m.group("callee"))
            if ret is None or m.group("callee") == m.group("name"):
                return m.group(0)
            changed[m.group("name")] = _spell_int(ret, text)
            used_callees[m.group("callee")] = _spell_int(ret, text)
            return (f"{changed[m.group('name')]}{m.group('sp')}{m.group('name')}({m.group('params')}){m.group('br')}{{{m.group('ws1')}"
                    f"{m.group('ind')}return {m.group('call')};{m.group('ws2')}}}")
        new = TAILRET_DEF_RE.sub(sub, text)
        if new != text:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
    if not changed:
        return 0, 0
    kinds = dict(used_callees)
    kinds.update(changed)
    pat = re.compile(r"\bvoid([ \t]+)(" + "|".join(map(re.escape, sorted(kinds))) + r")([ \t]*\([^{};]*\)[ \t]*;)")
    files = 0
    for path in sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h"))):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        new = pat.sub(lambda m: f"{_spell_int(kinds[m.group(2)], text)}{m.group(1)}{m.group(2)}{m.group(3)}", text)
        if new != text:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            files += 1
    return len(changed), files


def tail_call_returns(mirror):
    """Rounds until stable: a wrapper of a wrapper (frFontDrawListDefault -> frFontDrawListMode -> frFontDrawList)."""
    total, files = 0, 0
    for _ in range(8):
        n, f = _tail_call_round(mirror)
        if n == 0:
            break
        total += n
        files += f
    return total, files


# A void function whose body creates a task as a statement (`(s32)kwlnTaskCreateChild(...);`) and then fills the work block leaves the
# task handle in $v0 on retail, and other units call it as `s32 name(...)` and keep the handle (the field record's rain / fog slots:
# FldRainTaskCreate returned a stale register on the host, 15, and the unload destroyed task 15). The function returns the handle.
TASKRET_DEF_RE = re.compile(r"^void(?P<sp>[ \t]+)(?P<name>\w+)\((?P<params>[^;{}]*)\)(?P<br>[ \t]*\n?[ \t]*)\{(?P<body>.*?)^\}", re.M | re.S)
TASKRET_STMT_RE = re.compile(r"^(?P<ind>    )(?:\(s32\)[ \t]*)?(?P<call>kwlnTaskCreate\w*\()", re.M)
TASKRET_DECL_RE = re.compile(r"^[ \t]*(?:extern[ \t]+)?(?:s32|int|u32)[ \t]+(?P<name>\w+)[ \t]*\(", re.M)


def task_create_returns(mirror):
    files = sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h")))
    texts = {p: p.read_text(encoding="utf-8", errors="surrogateescape") for p in files}
    nonvoid = set()
    for t in texts.values():
        nonvoid.update(m.group("name") for m in TASKRET_DECL_RE.finditer(t))
    changed = set()
    for path, text in texts.items():
        if path.suffix != ".c":
            continue

        def sub(m):
            name = m.group("name")
            body = m.group("body")
            stmts = list(TASKRET_STMT_RE.finditer(body))
            if name not in nonvoid or len(stmts) != 1 or "return" in body:
                return m.group(0)
            st = stmts[0]
            # the statement must end at its own `;` (balanced parentheses) before anything else uses the register
            depth, i = 0, st.start("call")
            while i < len(body):
                if body[i] == "(":
                    depth += 1
                elif body[i] == ")":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            if i >= len(body) or not body[i + 1:].lstrip().startswith(";"):
                return m.group(0)
            changed.add(name)
            call = body[st.start("call"):i + 1]
            body = body[:st.start()] + st.group("ind") + "p4_created = (s32)" + call + body[i + 1:]
            body = body.rstrip() + "\n    return p4_created;\n"
            return (f"s32{m.group('sp')}{name}({m.group('params')}){m.group('br')}{{\n    s32 p4_created;" + body + "}")
        new = TASKRET_DEF_RE.sub(sub, text)
        if new != text:
            texts[path] = new
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
    if not changed:
        return 0, 0
    pat = re.compile(r"\bvoid([ \t]+)(" + "|".join(map(re.escape, sorted(changed))) + r")([ \t]*\([^{};]*\)[ \t]*;)")
    nfiles = 0
    for path, text in texts.items():
        new = pat.sub(lambda m: f"s32{m.group(1)}{m.group(2)}{m.group(3)}", text)
        if new != text:
            path.write_text(new, encoding="utf-8", errors="surrogateescape")
            nfiles += 1
    return len(changed), nfiles


def check_decomp_targets(decomp):
    """Claims and unbindings must still identify real decomp addresses, not inert rows."""
    syms = read_symbols(decomp)
    addresses = {addr for addr, kind in syms.values() if kind == "func"}
    bindings = {(addr, name) for name, (addr, kind) in syms.items() if kind == "func"}
    for path in sorted((decomp / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for marker in MARKER_RE.finditer(text):
            addr = int(marker[1], 16)
            addresses.add(addr)
            name = marker_definition_name(text, marker.end())
            if name:
                bindings.add((addr, name))
    errors = []
    claims = {}
    for n, line in enumerate((ROOT / "overrides/CLAIMS.md").read_text(encoding="utf-8").splitlines(), 1):
        match = re.match(r"^([0-9a-fA-F]{8})\s+(\S+)\s+", line)
        if not match:
            if re.match(r"^(?:0x|[0-9])", line):
                errors.append(f"overrides/CLAIMS.md:{n}: expected an eight-digit hexadecimal address, owner and description")
            continue
        addr = int(match[1], 16)
        if addr in claims:
            errors.append(f"overrides/CLAIMS.md:{n}: duplicate address {addr:08x} (line {claims[addr]})")
        claims[addr] = n
        if addr not in addresses:
            errors.append(f"overrides/CLAIMS.md:{n}: {addr:08x} no longer has a decomp marker or function symbol")
    for addr, name in read_unbound_natives().items():
        if (addr, name) not in bindings:
            errors.append(f"config/unbound_natives.txt: {addr:08x} {name} no longer binds this decomp address")
    for addr, name in read_native_names().items():
        if addr not in addresses:
            errors.append(f"config/native_names.txt: {addr:08x} {name} no longer targets a decomp function address")
    if errors:
        die("decomp target failures:\n  " + "\n  ".join(errors))


class MirrorContracts:
    """Reviewed per-pass/per-unit edit census; normal builds never learn expectations."""

    def __init__(self, mirror, refresh=False):
        self.mirror = mirror
        self.path = ROOT / "config/mirror_contracts.json"
        self.refresh = refresh
        self.actual = {}
        self.errors = []
        if not refresh and not self.path.is_file():
            die(f"{self.path}: missing mirror match contracts")
        self.expected = {} if refresh else json.loads(self.path.read_text(encoding="utf-8"))

    def snapshot(self, root=None):
        root = self.mirror if root is None else root
        return {p.relative_to(root).as_posix(): p.read_bytes().replace(b"\r\n", b"\n")
                for tree in ("src", "include")
                for p in sorted((root / tree).rglob("*"))
                if p.is_file() and p.suffix in (".c", ".h")}

    def run(self, fn, *args, stage_name=None, before_root=None):
        name = stage_name or fn.__name__
        with build_cache.timed("mirror.snapshot-before", stage=name):
            before = self.snapshot(before_root)
        with build_cache.timed("mirror.transform", stage=name):
            result = fn(*args)
        with build_cache.timed("mirror.snapshot-after", stage=name):
            after = self.snapshot()
        edits = {}
        with build_cache.timed("mirror.contract-diff", stage=name):
            for unit in sorted(before.keys() | after.keys()):
                old, new = before.get(unit, b""), after.get(unit, b"")
                if old == new:
                    if before_root is not None:
                        # Unedited headers still determine inherited types and pointer strides.
                        edits[unit] = {"input_sha256": hashlib.sha256(old).hexdigest(), "hunks": []}
                    continue
                data = build_cache.run_bytes("mirror.contract-unit", lambda: {
                    "stage": name, "unit": unit,
                    "before": hashlib.sha256(old).hexdigest(),
                    "after": hashlib.sha256(new).hexdigest(),
                    "versions": build_cache.versions(__file__)},
                    lambda: json.dumps(self._edit_facts(old, new), sort_keys=True).encode("ascii"))
                edits[unit] = json.loads(data)
        self.record(name, edits)
        return result

    @staticmethod
    def _edit_facts(old, new):
        left, right = old.splitlines(keepends=True), new.splitlines(keepends=True)
        hunks = []
        for tag, a, b, c, d in difflib.SequenceMatcher(None, left, right, autojunk=False).get_opcodes():
            if tag != "equal":
                hunks.append([b - a, d - c,
                              hashlib.sha256(b"".join(left[a:b])).hexdigest(),
                              hashlib.sha256(b"".join(right[c:d])).hexdigest()])
        # The entire input, not only edited call text, binds surrounding ABI declarations.
        return {"input_sha256": hashlib.sha256(old).hexdigest(), "hunks": hunks}

    def record(self, name, edits):
        if name in self.actual:
            die(f"mirror contract pass {name} ran twice")
        self.actual[name] = edits
        if not self.refresh and self.expected.get(name) != edits:
            expected = self.expected.get(name, {})
            bad = [u for u in sorted(expected.keys() | edits.keys()) if expected.get(u) != edits.get(u)]
            message = (f"mirror contract {name}: changed match census in {', '.join(bad) or 'new pass'}; "
                       "review the decomp and port transformations before --refresh-contracts")
            self.errors.append(message)
            print(f"gen_port: error: {message}", file=sys.stderr)

    def finish(self):
        if self.refresh:
            write_if_changed(self.path, json.dumps(self.actual, indent=2, sort_keys=True) + "\n")
        elif self.actual.keys() != self.expected.keys():
            die("mirror contracts contain passes no longer run: " +
                ", ".join(sorted(self.expected.keys() - self.actual.keys())))
        if self.errors:
            die(f"{len(self.errors)} mirror stages changed their reviewed matches; mirror not published")


def mirror_cache_inputs(decomp, build, mod_hooks):
    """Conservative file inputs actually consumed by the mirror passes."""
    decomp_paths = [p for tree in ("src", "include", "config")
                    for p in (decomp / tree).rglob("*") if p.is_file()]
    port_paths = [p for tree in ("config", "overrides")
                  for p in (ROOT / tree).rglob("*") if p.is_file()]
    port_paths += list((ROOT / "src").rglob("*.c")) + list((ROOT / "include").rglob("*.h"))
    # Prototype aliases also read ROOT/decomp when --decomp names another tree.
    if (ROOT / "decomp").resolve() != decomp.resolve():
        port_paths += list((ROOT / "decomp/config").glob("*"))
    return {"decomp": str(decomp.resolve()), "build": str(build.resolve()),
            "mod_hooks": mod_hooks,
            "decomp_files": build_cache.input_files(decomp_paths, root=decomp),
            "port_files": build_cache.input_files(port_paths, root=ROOT),
            "versions": build_cache.versions(__file__, str(ROOT / "tools/mod_codegen.py")),
            "toolchain": build_cache.compiler_identity(sys.executable)}


def pack_mirror(build, actual, stdout, stderr):
    payload = io.BytesIO()
    with tarfile.open(fileobj=payload, mode="w") as archive:
        sources = [p for p in (build / "decomp").rglob("*") if p.is_file()]
        sources += [build / "asm_blocks.txt", build / "prototype_report.txt",
                    build / "mod_bridges.c", build / "mod_hooks.tsv"]
        for path in sorted(sources):
            info = tarfile.TarInfo(path.relative_to(build).as_posix())
            info.size = path.stat().st_size
            info.mode = 0o644
            with path.open("rb") as source:
                archive.addfile(info, source)
        for name, content in (("_contracts.json", json.dumps(actual, sort_keys=True).encode("utf-8")),
                              ("_stdout", stdout.encode("utf-8", errors="surrogateescape")),
                              ("_stderr", stderr.encode("utf-8", errors="surrogateescape"))):
            info = tarfile.TarInfo(name)
            info.size = len(content)
            archive.addfile(info, io.BytesIO(content))
    return payload.getvalue()


def restore_mirror(build, payload):
    stage = build / "decomp.stage"
    with tarfile.open(fileobj=io.BytesIO(payload), mode="r:") as archive:
        members = archive.getmembers()
        names = set()
        for member in members:
            name = PurePosixPath(member.name)
            if (member.name in names or not member.isfile() or name.is_absolute() or
                    ".." in name.parts or "\\" in member.name or
                    not (member.name.startswith(("decomp/src/", "decomp/include/")) or
                         member.name in ("asm_blocks.txt", "prototype_report.txt", "mod_bridges.c",
                                         "mod_hooks.tsv", "_contracts.json", "_stdout", "_stderr"))):
                raise build_cache.CacheCorruption(f"invalid cached mirror member {member.name}")
            names.add(member.name)
        if not {"asm_blocks.txt", "prototype_report.txt", "mod_bridges.c", "mod_hooks.tsv",
                "_contracts.json", "_stdout", "_stderr"} <= names:
            raise build_cache.CacheCorruption("cached mirror is missing its reports/contracts/diagnostics")
        actual = json.loads(archive.extractfile("_contracts.json").read())
        contracts = MirrorContracts(stage)
        for name, edits in actual.items():
            contracts.record(name, edits)
        contracts.finish()  # Cached facts never bypass the current reviewed guard.
        if stage.exists():
            shutil.rmtree(stage)
        for member in members:
            if member.name.startswith("_"):
                continue
            destination = stage / member.name[len("decomp/"):] if member.name.startswith("decomp/") else build / member.name
            write_if_changed(destination, archive.extractfile(member).read().decode("utf-8", errors="surrogateescape"))
        publish_mirror(stage, build / "decomp")
        shutil.rmtree(stage)
        sys.stdout.write(archive.extractfile("_stdout").read().decode("utf-8", errors="surrogateescape"))
        sys.stderr.write(archive.extractfile("_stderr").read().decode("utf-8", errors="surrogateescape"))


def cmd_mirror(a):
    # An explicit reviewed refresh always computes its facts afresh.
    if a.refresh_contracts or not build_cache.enabled():
        with build_cache.timed("mirror"):
            generate_mirror(a)
        return
    decomp, build = Path(a.decomp), Path(a.build)
    generated = []
    def produce():
        stdout, stderr = io.StringIO(), io.StringIO()
        try:
            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                actual = generate_mirror(a)
        finally:
            sys.stdout.write(stdout.getvalue())
            sys.stderr.write(stderr.getvalue())
        generated.append(True)
        return pack_mirror(build, actual, stdout.getvalue(), stderr.getvalue())
    payload = build_cache.run_bytes("mirror", lambda: mirror_cache_inputs(decomp, build, a.mod_hooks), produce)
    if not generated:
        restore_mirror(build, payload)


def generate_mirror(a):
    """Build-time part: write BUILD/decomp (the decomp's src/ and include/ ported, with the overrides
    spliced in) and BUILD/asm_blocks.txt. CMake lists the unit files as outputs and reruns this only
    when a decomp source, an override, or a config file changes."""
    decomp, build = Path(a.decomp), Path(a.build)
    units, _ = read_units(ROOT)
    check_decomp_targets(decomp)
    # Every pass edits build/decomp.stage. Only the final publish touches build/decomp, so files
    # the passes leave unchanged keep their mtime and their objects are not rebuilt.
    stage = build / "decomp.stage"
    # A failed attempt leaves its unpublished stage for diagnosis. It may belong
    # to a different pin; mirror_tree copies inputs but cannot remove its extra
    # files. Never let that warm scratch become an input to the next census.
    if stage.exists():
        shutil.rmtree(stage)
    contracts = MirrorContracts(stage, a.refresh_contracts)
    contracts.run(mirror_tree, decomp, stage, before_root=decomp)
    va_lines, va_files = contracts.run(strip_local_varargs, stage)
    print(f"gen_port: mirror: removed {va_lines} unit-local va_list/va_start lines in {va_files} files")
    passed = contracts.run(apply_passthrough_calls, stage)
    print(f"gen_port: mirror: passed the caller's own registers to {passed} calls that omitted them "
          f"(config/passthrough_calls.txt)")
    task_args, task_files = contracts.run(pass_taskwork_argument, stage)
    print(f"gen_port: mirror: gave {task_args} kwlnTaskGetWork() calls their function's task argument (and task-create returns) in {task_files} files")
    tails, tail_files = contracts.run(tail_call_returns, stage)
    print(f"gen_port: mirror: made {tails} void tail-call wrappers return their callee's value (declarations in {tail_files} files)")
    created, created_files = contracts.run(task_create_returns, stage)
    print(f"gen_port: mirror: made {created} void task-creating functions return the task handle (declarations in {created_files} files)")
    proto_rewrites, proto_files = contracts.run(unnamed_varargs_prototypes, stage)
    print(f"gen_port: mirror: rewrote {proto_rewrites} varargs-only prototypes to () in {proto_files} files")
    statics = contracts.run(globalize_marker_statics, stage)
    print(f"gen_port: mirror: made {statics} static prototypes of marker functions global")
    joined, joined_files = contracts.run(normalize_prototype_lines, stage)
    print(f"gen_port: mirror: joined {joined} wrapped prototypes in {joined_files} files")
    dropped, block_dropped, dropped_files = contracts.run(drop_redundant_prototypes, stage)
    print(f"gen_port: mirror: dropped {dropped} redundant prototypes and {block_dropped} "
          f"conflicting block-scope externs in {dropped_files} files")
    reconciled, reconciled_files = contracts.run(reconcile_prototypes_with_definitions, stage)
    print(f"gen_port: mirror: gave {reconciled} prototypes their definition's signature in {reconciled_files} files")
    hoisted, hoist_files = contracts.run(hoist_block_prototypes, stage)
    if hoisted:
        # The hoisted prototypes now make later block-scope copies of the same name redundant.
        contracts.run(drop_redundant_prototypes, stage, stage_name="drop_redundant_prototypes_after_hoist")
    print(f"gen_port: mirror: hoisted {hoisted} block-scope prototypes to file scope in {hoist_files} files")
    ext, ext_files = contracts.run(externalize_retail_data, stage)
    print(f"gen_port: mirror: made {ext} definitions of retail-addressed names extern in {ext_files} files")
    renamed, renamed_files = contracts.run(rename_conflicting_data_declarations, stage)
    print(f"gen_port: mirror: aliased {renamed} data declarations whose type disagrees with another in {renamed_files} files")
    forwarded, forwarded_files = contracts.run(forward_declare_prototype_structs, stage)
    print(f"gen_port: mirror: forward-declared {forwarded} prototype-scope struct tags in {forwarded_files} files")
    fixed_protos, fixed_files = contracts.run(align_prototype_returns, stage)
    print(f"gen_port: mirror: retyped {fixed_protos} prototypes to match their definitions in {fixed_files} files")
    synced, synced_files = contracts.run(lower_sync_barriers, stage)
    print(f"gen_port: mirror: lowered {synced} EE sync barriers to compiler barriers in {synced_files} files")
    fixes = contracts.run(apply_mirror_fixes, stage)
    print(f"gen_port: mirror: applied {fixes} mirror fixes (config/mirror_fixes.txt)")
    aligned_structs, aligned_files = contracts.run(align_matrix_structs, stage)
    print(f"gen_port: mirror: aligned {aligned_structs} matrix-shaped local structs to 16 bytes in {aligned_files} files")
    changed = contracts.run(apply_overrides, ROOT, stage, units)
    sd_decls, sd_widened = contracts.run(pass_soft_double_types, stage)
    print(f"gen_port: mirror: gave {sd_decls} soft-double helper declarations their 64-bit bit-pattern signature and widened {sd_widened} locals that receive one")
    kinds = contracts.run(pass_effect_kind_dispatch, stage)
    print(f"gen_port: mirror: gave {kinds} effect-kind dispatch calls their effect argument")
    quads = contracts.run(void_quad_returns, stage)
    float_fixed, float_defs = contracts.run(float_result_decls, stage)
    print(f"gen_port: mirror: retyped {float_fixed} void prototypes of {float_defs} float-returning functions to f32 (an ignored x87 result must still be popped)")
    widened = contracts.run(widen_narrow_returns, stage)
    print(f"gen_port: mirror: {widened} bool functions return a full 32-bit register")
    print(f"gen_port: mirror: {quads} functions that returned a 128-bit value return void (a hidden struct return breaks calls through pointers)")
    asm_removed = contracts.run(drop_asm_blocks, stage, build)
    print(f"gen_port: mirror: removed {len(asm_removed)} MWCC asm function blocks "
          f"(build/gen/asm_blocks.txt)")
    protos, proto_files, implicit, ret_fixed, proto_skipped, aligned = contracts.run(
        prototype_unprototyped_declarations, stage, ROOT, build / "prototype_report.txt")
    print(f"gen_port: mirror: aligned typed prototypes with their definitions: {aligned[0]} parameters retyped, "
          f"{aligned[1]} prototypes reordered ({aligned[2]} call sites), {aligned[4]} return classes fixed, "
          f"{aligned[3]} parameter-count disagreements reported")
    print(f"gen_port: mirror: gave {protos} unprototyped declarations their definition's prototype in {proto_files} "
          f"files ({ret_fixed} return classes fixed), declared {implicit} implicit callees, left {proto_skipped} "
          f"functions unprototyped (build/gen/prototype_report.txt)")
    narrow, narrow_files, narrow_defs = contracts.run(align_narrow_returns, stage)
    print(f"gen_port: mirror: gave {narrow} declarations the narrow return type of their definition ({narrow_defs} s8/s16/u8/u16 functions) in {narrow_files} files")
    dup_fns, dup_files = contracts.run(drop_duplicate_promoted_definitions, stage, units)
    print(f"gen_port: mirror: dropped {dup_fns} promoted definitions already owned by another unit "
          f"in {dup_files} files")
    owned = contracts.run(port_owned_storage, decomp, stage, units)
    offset_checks, offset_files = contracts.run(portable_offset_asserts, stage)
    print(f"gen_port: mirror: preserved {offset_checks} constant offset assertions in {offset_files} files")
    inactive_units, inactive_functions, inactive_registrations = contracts.run(remove_inactive_movie, stage)
    print(f"gen_port: mirror: removed inactive movie decoder: {inactive_units} units, "
          f"{inactive_functions} mixed-unit functions, {inactive_registrations} registrations")
    contracts.finish()
    import mod_codegen
    hooks = mod_codegen.instrument(stage, units, build, sys.modules[__name__], a.mod_hooks)
    print(f"gen_port: mirror: emitted {hooks} typed mod entry guards after the reviewed mirror passes")
    publish_mirror(stage, build / "decomp")
    shutil.rmtree(stage)
    print(f"gen_port: mirror: {len(units)} units, {changed} mirror files spliced by overrides")
    for name, unit in owned:
        print(f"gen_port: mirror: port-owned RenderWare storage: {name} <- {unit}")
    return contracts.actual


# ---------------------------------------------------------------- tables

class Tools:
    """How the target's objects and libraries are read. Linux i386 is ELF with C names as symbols.
    The Windows build (--coff) is i686 COFF: cdecl names carry a leading underscore, and the
    Win32 imports (`__imp__name`, stdcall `_name@N`) belong to the host by definition. macOS
    (--macho) is Mach-O: every C name carries a leading underscore. An asm label there names the
    symbol as written, so labels are spelled with the prefix (the rewriter's Mach-O pass for the
    mirror and overrides, P4_ASM_LABEL in port code); a reference without one is an error."""
    coff = False
    macho = False
    arm64 = False   # the objects are AArch64 ELF (--arm64): thunks are a branch, `%function`, the GNU assembler's comment character being `@`
    us = "_"        # the symbol prefix of C names in COFF objects: i686 has one, x86_64 does not (--coff64)
    nm = "nm"
    objdump = "objdump"
    raw_undefined = set()   # Mach-O: symbols the objects reference without the C prefix (an asm label spelled for ELF)


def target_name(raw):
    """The C name of an object symbol, or None for a symbol that never needs a placement."""
    if Tools.macho:
        name = raw.split("@", 1)[0]   # (an x86_64 GOT reference prints as `_name@GOTPCREL`)
        return name[1:] if name.startswith("_") else name
    if not Tools.coff:
        return raw
    if raw.startswith("__imp_") or "@" in raw or raw.startswith("."):
        return None
    return raw[1:] if Tools.us and raw.startswith("_") else raw


def nm_symbols(objs):
    defined, undefined, functions = set(), set(), set()
    for chunk_start in range(0, len(objs), 400):
        chunk = [str(o) for o in objs[chunk_start:chunk_start + 400]]
        out = subprocess.run([Tools.nm, "-P", "-g", *chunk], capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            parts = line.split()
            if len(parts) < 2 or line.endswith(":"):
                continue
            name, kind = target_name(parts[0]), parts[1]
            if name is None:
                continue
            if Tools.macho and kind in ("U", "w") and not parts[0].startswith("_"):
                Tools.raw_undefined.add(parts[0])
            if kind in ("U", "w"):
                undefined.add(name)
            else:
                defined.add(name)
                if kind in ("T", "W", "i"):
                    functions.add(name)
    return defined, undefined, functions

def project_symbols(paths, host):
    """Compiled project definitions cannot turn system exports into project calls."""
    defined, _, _ = nm_symbols([Path(path) for path in paths])
    return defined - host


# Relocation types that reach a symbol by calling it. Any other reference (data
# read, address taken, GOT entry) needs a real placement.
# (x86_64 COFF has one PC-relative relocation for calls and data alike, IMAGE_REL_AMD64_REL32; a 64-bit mirror reaches no retail data by
# symbol -- that is an error below -- so what it references through one is a function.)
CALL_RELOCS = {"R_386_PC32", "R_386_PLT32", "IMAGE_REL_I386_REL32", "R_X86_64_PLT32", "IMAGE_REL_AMD64_REL32", "R_AARCH64_CALL26", "R_AARCH64_JUMP26",
               "ARM64_RELOC_BRANCH26", "X86_64_RELOC_BRANCH"}
RELOC_RE = re.compile(r"^[0-9a-fA-F]+\s+(R_386_\w+|R_X86_64_\w+|R_AARCH64_\w+|IMAGE_REL_I386_\w+|IMAGE_REL_AMD64_\w+|ARM64_RELOC_\w+|X86_64_RELOC_\w+)"
                      r"\s+(\S+?)(?:[+-]0x[0-9a-fA-F]+)?$")
# Mach-O objects are read with `objdump --macho -r`, every relocation entry as stored: the generic `-r` listing hides some x86_64
# X86_64_RELOC_UNSIGNED entries of a data section (thousands of the tables object's function-table pointers, LLVM 21 and 23).
# A row is `address pcrel length extern type scattered symbol`; only an external entry names a symbol.
MACHO_RELOC_RE = re.compile(r"^[0-9a-fA-F]+\s+(?:True|False)\s+(?:\?\(\s*\d+\)|\S+)\s+(True|False)\s+(\S+)\s+(?:True|False)\s+(\S+)")
MACHO_RELOC_TYPES = {
    "x86_64": {"UNSIGND": "X86_64_RELOC_UNSIGNED", "SIGNED": "X86_64_RELOC_SIGNED", "BRANCH": "X86_64_RELOC_BRANCH",
               "GOT_LD": "X86_64_RELOC_GOT_LOAD", "GOT": "X86_64_RELOC_GOT", "SUB": "X86_64_RELOC_SUBTRACTOR", "SIGNED1": "X86_64_RELOC_SIGNED_1",
               "SIGNED2": "X86_64_RELOC_SIGNED_2", "SIGNED4": "X86_64_RELOC_SIGNED_4", "TLV": "X86_64_RELOC_TLV"},
    "arm64": {"UNSIGND": "ARM64_RELOC_UNSIGNED", "SUB": "ARM64_RELOC_SUBTRACTOR", "BR26": "ARM64_RELOC_BRANCH26", "PAGE21": "ARM64_RELOC_PAGE21",
              "PAGOF12": "ARM64_RELOC_PAGEOFF12", "GOTLDP": "ARM64_RELOC_GOT_LOAD_PAGE21", "GOTLDPOF": "ARM64_RELOC_GOT_LOAD_PAGEOFF12",
              "PTRTGOT": "ARM64_RELOC_POINTER_TO_GOT", "TLVLDP": "ARM64_RELOC_TLVP_LOAD_PAGE21", "TLVLDPOF": "ARM64_RELOC_TLVP_LOAD_PAGEOFF12",
              "ADDEND": "ARM64_RELOC_ADDEND"},
}


def reloc_kinds(objs, names):
    """name -> set of relocation types that reference it, from `objdump -r` (`--macho -r` for Mach-O)."""
    kinds = {}
    types = MACHO_RELOC_TYPES["arm64" if Tools.arm64 else "x86_64"]
    for chunk_start in range(0, len(objs), 400):
        chunk = [str(o) for o in objs[chunk_start:chunk_start + 400]]
        out = subprocess.run([Tools.objdump, *(["--macho"] if Tools.macho else []), "-r", *chunk],
                             capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            if Tools.macho:
                m = MACHO_RELOC_RE.match(line.strip())
                if not m or m[1] != "True":
                    continue
                kind, raw = types.get(m[2], m[2]), m[3]
            else:
                m = RELOC_RE.match(line.strip())
                if not m:
                    continue
                kind, raw = m[1], m[2]
            name = target_name(raw)
            if name in names:
                kinds.setdefault(name, set()).add(kind)
    return kinds


def place_undefined(name, reloc_types, syms):
    """Where an undefined name lives: ("data" | "trap", PS2 address), or (None, reason).
    A name that is only ever called and has no other placement is trapped at address 0."""
    dm = DATA_NAME_RE.match(name)
    if dm and name in syms and syms[name][1] == "data" and syms[name][0] != int(dm[1], 16) \
            and not reloc_types & CALL_RELOCS:
        # The decomp's symbol lists place a handful of D_XXXXXXXX names at a different address than
        # the name says (D_00724840 = 0x00764840, evidence in the list). The list is the evidence.
        return "data", syms[name][0]
    if dm:
        return "data", int(dm[1], 16)
    gm = GP_NAME_RE.match(name)
    if gm:
        off = int(gm[1], 16) & 0xFFFF
        if off >= 0x8000:
            off -= 0x10000
        return "data", GP_VALUE + off
    if name in syms:
        addr, kind = syms[name]
        if kind == "data" and reloc_types & CALL_RELOCS:
            kind = "func"  # the decomp's symbol lists type some code as data; a call makes it code
        return ("trap" if kind == "func" else "data"), addr
    fm = FUNC_NAME_RE.match(name)
    if fm:
        return "trap", int(fm[1], 16)
    if reloc_types and reloc_types <= CALL_RELOCS:
        return "trap", 0
    return None, "referenced as data or address with no placement"


def host_names(paths):
    """Names the host libraries export. The port never stubs these. Linux: the dynamic symbols
    of the shared libraries. Windows: the symbols the UCRT/mingw-w64 libraries, the compiler
    runtime and the SDL3/FFmpeg import libraries define. macOS: the exports of the dylibs and of
    the SDK's libSystem text stub, with the libraries it re-exports (libc, libm, the compiler runtime)."""
    names = set()
    for path in paths:
        if not Path(path).is_file():
            die(f"host library for the i386 link not found at {path}")
        if Tools.coff or Tools.macho:
            inlined = ["--add-inlinedinfo"] if path.endswith(".tbd") else []
            out = subprocess.run([Tools.nm, "-P", "-g", "--defined-only", *inlined, path],
                                 capture_output=True, text=True, check=True).stdout
            for line in out.splitlines():
                if line.strip() and not line.endswith(":"):
                    name = target_name(line.split()[0])
                    if name is not None:
                        names.add(name)
            continue
        # a static archive (the compiler runtime: __divdi3 and the like) has no dynamic symbols
        out = subprocess.run([Tools.nm, *(["-P", "-g"] if path.endswith(".a") else ["-D", "-P"]), "--defined-only", path],
                             capture_output=True, text=True, check=True).stdout
        # nm -D prints versioned names such as stderr@@GLIBC_2.0; the set needs the bare name.
        names |= {line.split()[0].split("@")[0] for line in out.splitlines()
                  if line.strip() and not line.endswith(":")}
    return names


def load_segments(data):
    if data[:4] != b"\x7fELF":
        die("retail image is not an ELF file")
    e_phoff, = struct.unpack_from("<I", data, 0x1C)
    e_phentsize, e_phnum = struct.unpack_from("<HH", data, 0x2A)
    segs = []
    for i in range(e_phnum):
        p_type, p_offset, p_vaddr, _, p_filesz, _, _, _ = struct.unpack_from(
            "<8I", data, e_phoff + i * e_phentsize)
        if p_type == 1:  # PT_LOAD
            segs.append((p_vaddr, p_filesz, p_offset))
    return segs


def retail_function_names(decomp, build, *, staged_texts=None):
    """{name: retail address} from symbols, native bindings, markers and #define aliases.
    staged_texts supplies the unpublished mirror during prototype alignment, before units.txt exists.
    The compiled form remains the authority for tools/port64/unit64.py's ABI aliases."""
    syms = read_symbols(decomp)
    syms.update(read_port_symbols())
    names = {n: addr for n, (addr, kind) in syms.items() if kind == "func"}
    if staged_texts is None:
        units = [Path(line.split("|")[0]) for line in (build / "units.txt").read_text().splitlines() if line.strip()]
        unit_texts = ((unit if unit.is_absolute() else ROOT / unit).read_text(encoding="utf-8", errors="surrogateescape")
                      for unit in units)
        alias_texts = (source.read_text(encoding="utf-8", errors="surrogateescape")
                       for source in sorted((build / "decomp" / "src").rglob("*.c")))
    else:
        unit_texts = staged_texts.values()
        alias_texts = (text for path, text in staged_texts.items() if path.suffix == ".c")
    for text in unit_texts:
        for mk in MARKER_RE.finditer(text):
            name = marker_body_name(text, mk.end(), int(mk[1], 16), syms)
            if name:
                names.setdefault(name, int(mk[1], 16))
    for addr, name in read_native_names().items():
        names[name] = addr
    for text in alias_texts:
        for line in text.splitlines():
            m = FUNC_ALIAS_RE.match(strip_comment(line))
            if m and FUNC_NAME_RE.match(m[2]):
                names.setdefault(m[1], int(FUNC_NAME_RE.match(m[2])[1], 16))
    # the symbol lists type some code as data (FUN_003b3e00_typed at 0x00276290): a name there at the address of a function is one of
    # that function's names
    starts = set(names.values())
    for n, (addr, kind) in syms.items():
        if kind == "data" and addr in starts:
            names.setdefault(n, addr)
    return names


def pointer_word_rows(data, base):
    """(word address, value, inside a C string literal) for every word of data that holds a code address."""
    # A retail code address is 24-bit, so a callback alone can resemble at most
    # three printable bytes followed by NUL. Longer terminated runs are text.
    # In particular, " 6F\0" in a floor name is also the address 0x00463620.
    string_words = set()
    for match in re.finditer(rb"[\x20-\x7e]{4,}\x00", data):
        lo = (base + match.start() + 3) & ~3
        hi = base + match.end() - 3
        string_words.update(range(lo, hi, 4))
    rows = []
    for offset in range(0, len(data), 4):
        value = struct.unpack_from("<I", data, offset)[0]
        if not value & 3 and any(lo <= value < hi for lo, hi in CODE_RANGES):
            va = base + offset
            rows.append((va, value, va in string_words))
    return rows


def function_pointer_words(rows, starts, preserve_strings):
    """Callback words of pointer_word_rows; preserve_strings keeps words embedded in C string literals."""
    for va, value, in_string in rows:
        if value in starts and not (preserve_strings and in_string):
            yield va, value


def segment_bytes(data, segs, lo, hi):
    for vaddr, filesz, offset in segs:
        if vaddr <= lo and hi <= vaddr + filesz:
            return data[offset + lo - vaddr:offset + hi - vaddr]
    die(f"range 0x{lo:08x}..0x{hi:08x} is outside the retail image")


def scan_fnptr_words(retail):
    """(SHA-256, (image lo, image hi), pointer_word_rows of DATA_RANGES) of a retail ELF image."""
    segs = load_segments(retail)
    rows = []
    for lo, hi in DATA_RANGES:
        rows += pointer_word_rows(segment_bytes(retail, segs, lo, hi), lo)
    image = (min(vaddr for vaddr, _, _ in segs), max(vaddr + filesz for vaddr, filesz, _ in segs))
    return hashlib.sha256(retail).hexdigest(), image, rows


def write_fnptr_words(path, sha, image, rows):
    lines = ["# Function-pointer words of the retail SLUS_217.82: every word of tools/gen_port.py's DATA_RANGES whose value is a",
             "# code address (CODE_RANGES), as `<word address> <value>`, plus ` s` when the word lies in a C string literal.",
             "# Addresses only: the tables step reads this file instead of the ELF, and checks it against the ELF when one is given.",
             "# Regenerate: python tools/gen_port.py fnptr-words --retail SLUS_217.82 --out config/retail_fnptr_words.txt",
             f"sha256 {sha}",
             f"image {image[0]:08x} {image[1]:08x}"]
    lines += [f"{va:08x} {value:08x}{' s' if in_string else ''}" for va, value, in_string in rows]
    Path(path).write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")


def read_fnptr_words(path):
    sha = image = None
    rows = []
    for line in Path(path).read_text(encoding="ascii").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if parts[0] == "sha256":
            sha = parts[1]
        elif parts[0] == "image":
            image = (int(parts[1], 16), int(parts[2], 16))
        else:
            rows.append((int(parts[0], 16), int(parts[1], 16), parts[2:] == ["s"]))
    if not sha or not image or not rows:
        die(f"{path} is incomplete: regenerate it with gen_port.py fnptr-words")
    return sha, image, rows


def cmd_fnptr_words(a):
    sha, image, rows = scan_fnptr_words(Path(a.retail).read_bytes())
    write_fnptr_words(a.out, sha, image, rows)
    print(f"gen_port: fnptr-words: {len(rows)} code-address words, ELF SHA-256 {sha}")


def cmd_tables(a):
    decomp, build = Path(a.decomp), Path(a.build)
    Tools.coff, Tools.nm, Tools.objdump, Tools.pic = a.coff or a.coff64, a.nm, a.objdump, a.pic
    Tools.us = "" if a.coff64 else "_"
    Tools.arm64 = a.arm64
    Tools.macho = a.macho
    syms = read_symbols(decomp)
    port = read_port_symbols()
    clash = sorted(set(port) & set(syms))
    if clash:
        die("config/port_symbols.txt repeats names the decomp already places: " + ", ".join(clash[:20]))
    syms.update(port)
    units = [l.split("|")[0] for l in (build / "units.txt").read_text().splitlines() if l.strip()]

    # Marker-derived native names, from the compiled (spliced) unit copies.
    marker_names = {}
    for u in units:
        t = (build / u).read_text(encoding="utf-8", errors="surrogateescape")
        for mk in MARKER_RE.finditer(t):
            name = marker_body_name(t, mk.end(), int(mk[1], 16), syms)
            if name:
                marker_names.setdefault(int(mk[1], 16), name)
    compiled_markers = dict(marker_names)

    # Excluded units still define functions the decomp names by marker. They have
    # no compiled copy, so they become traps with their real names and addresses.
    _, excluded = read_units(ROOT)
    inactive = read_inactive_movie()
    for u in excluded:
        if u in inactive["units"]:
            continue
        f = decomp / u
        if f.is_file():
            t = f.read_text(encoding="utf-8", errors="surrogateescape")
            for mk in MARKER_RE.finditer(t):
                name = marker_body_name(t, mk.end(), int(mk[1], 16), syms)
                if name:
                    marker_names.setdefault(int(mk[1], 16), name)
    retail_named = {}
    for name, (addr, kind) in syms.items():
        if kind == "func":
            retail_named.setdefault(addr, set()).add(name)
    for addr, name in marker_names.items():
        syms.setdefault(name, (addr, "func"))

    # CMake writes the object list to --objects-file, one path per line (paths contain spaces).
    objs = [Path(p) for p in Path(a.objects_file).read_text(encoding="utf-8").splitlines() if p.strip()]  # exactly the objects p4decomp and p4port link
    defined, undefined, function_defs = nm_symbols(objs)
    owned = read_port_owned_names()
    check_port_owned(decomp, owned)
    for name in owned:
        if name in syms:
            die(f"{name} is port-owned RenderWare storage but is also known by address; remove one of them")
        if name not in defined:
            die(f"{name} is port-owned RenderWare storage but no compiled object defines it")
    print(f"gen_port: tables: {len(owned)} port-owned RenderWare storage symbols")
    host = host_names(a.host_lib) | set(a.host_name or [])
    # Project archives are real compiled definitions, not host-library exports.
    # An identically named system export stays a host name and remains guarded.
    defined.update(project_symbols(a.project_lib, host))

    # The function-pointer words and image bounds come from the committed config/retail_fnptr_words.txt, so the build needs no
    # retail input. A retail ELF, when given, must be the one that file was made from.
    fnptr_sha, (image_lo, image_hi), pointer_rows = read_fnptr_words(FNPTR_WORDS)
    if a.retail:
        scanned = scan_fnptr_words(Path(a.retail).read_bytes())
        if scanned[0] != fnptr_sha:
            die(f"{a.retail} is not the retail executable this build expects (SHA-256 {scanned[0]}, expected {fnptr_sha})")
        if scanned != (fnptr_sha, (image_lo, image_hi), pointer_rows):
            die(f"{FNPTR_WORDS} differs from the scan of {a.retail}: regenerate it with gen_port.py fnptr-words")

    func_names = {}
    for name, (addr, kind) in sorted(syms.items()):
        if kind == "func":
            func_names.setdefault(addr, name)

    # Undefined names need a placement. Port-owned names are never retail stubs.
    # A host-library name is the host's: a stub would shadow libm's sinf or SDL_Init at link
    # time. The exception is a name the decomp places as data at a retail address that the code
    # reads rather than calls; that one stays the decomp's.
    unresolved = undefined - defined
    if Tools.raw_undefined:
        die("Mach-O objects reference symbols without the C prefix (an asm label spelled for ELF: use P4_ASM_LABEL in port code; the "
            "rewriter adds it in the mirror): " + ", ".join(sorted(Tools.raw_undefined)[:20]))
    # Renamed CRT operations bind to retail, except byte operations whose host
    # semantics are explicitly reviewed in the same authoritative allowlist.
    # LLVM can emit those byte helpers after source-level EE aliases have run.
    libc_renamed = read_libc_renames()
    missing_libc = [(name, target) for name, target in libc_renamed.items() if target not in function_defs]
    if missing_libc:
        die("include/p4pc/libc_rename.h targets are not compiled functions: " +
            ", ".join(f"{name} -> {target}" for name, target in missing_libc))
    decomp_objs = [o for o in objs if (("obj64w/units" if a.coff64 else "obj64/units") if a.ptr64 else "p4decomp.dir") in o.as_posix()]
    if not decomp_objs:
        die("no p4decomp objects in the objects file")
    _, decomp_undefined, _ = nm_symbols(decomp_objs)
    host_allow = read_host_allow()
    still_host = sorted((libc_renamed.keys() & decomp_undefined) - host_allow.keys())
    if still_host:
        die(f"decomp units still reference renamed libc names {still_host}; "
            "include/p4pc/libc_rename.h must cover them")
    # The class guard: every host library function the decomp units (mirror and overrides) still call must be listed in config/host_symbols_allow.txt with a
    # reason. The EE has no glibc: its float libm is the retail cores, and a host sin/cos/pow under round-toward-zero is a different function (a NaN the EE add reads
    # as FLT_MAX hung the new-game walk, docs/porting.md "EE float mode"). Plain byte seams require the same explicit review.
    host_bound = sorted(decomp_undefined & host)
    unlisted = [n for n in host_bound if n not in host_allow]
    if unlisted:
        die(f"decomp units call host library functions that config/host_symbols_allow.txt does not list: {unlisted}. A float or double maths function must be "
            "bound to its retail core (include/p4pc/libc_rename.h); a memory or string seam needs a line `name reason`.")
    print(f"gen_port: host library functions called by decomp units: {len(host_bound)} ({', '.join(host_bound)})")
    kinds = reloc_kinds(objs, unresolved)
    host_data = {n for n in unresolved & host if n not in libc_renamed
                 if n in syms and syms[n][1] == "data" and image_lo <= syms[n][0] < image_hi
                 and not kinds.get(n, set()) <= CALL_RELOCS}
    # A host name whose retail address is a function is that function, not data. rwstring takes
    # strtok's address and the decomp files it as data at func_00443f18. Such a name becomes a hidden
    # thunk to the function, so the executable never interposes the host's own symbol.
    fn_starts = set(func_names) | set(marker_names)
    host_thunk_names = {n for n in host_data if syms[n][0] in fn_starts}
    host_data -= host_thunk_names
    # symbols the linker itself defines (the executable's own extent, for p4_os_image_range)
    # and libc_nonshared.a (atexit and friends are linked into the executable, not exported by libc.so.6)
    linker_defined = {"__ehdr_start", "_end", "_edata", "__bss_start", "_etext", "etext", "edata", "end",
                      "atexit", "at_quick_exit", "pthread_atfork"}
    candidates = sorted(n for n in unresolved - (host - host_data) - linker_defined
                        if not n.startswith(("_GLOBAL_", "p4pc_", "p4_")))
    # `#define NAME func_XXXXXXXX` in the decomp names a function through an alias: that name
    # becomes a linker alias to the function (native when compiled, else its trap).
    func_alias = {}
    for f in sorted((build / "decomp" / "src").rglob("*.c")):
        for line in f.read_text(encoding="utf-8", errors="surrogateescape").splitlines():
            m = FUNC_ALIAS_RE.match(strip_comment(line))
            if m and FUNC_NAME_RE.match(m[2]):
                func_alias.setdefault(m[1], m[2])

    # A decomp `#define RwName func_XXXXXXXX` whose RwName this build compiles is native at that address.
    native_by_alias = {}
    for rw_name, target in sorted(func_alias.items()):
        tm = FUNC_NAME_RE.match(target)
        if tm and rw_name in function_defs:
            native_by_alias.setdefault(int(tm[1], 16), rw_name)

    # config/native_names.txt: a compiled function named differently from the retail symbol. It wins
    # over the marker and symbol names; a name no object defines is stale.
    native_names = read_native_names()
    for addr, name in sorted(native_names.items()):
        if name not in function_defs:
            die(f"config/native_names.txt: 0x{addr:08x} {name} is not defined by any compiled function")

    unbound_natives = read_unbound_natives()
    for addr, name in sorted(unbound_natives.items()):
        if name not in function_defs:
            die(f"config/unbound_natives.txt: 0x{addr:08x} {name} is not defined by any compiled function; review and retarget the binding")

    def native_for(addr):
        # The compiled function at this PS2 address, found by config/native_names.txt, its marker, its
        # symbol or RW alias name, or by the address-named definition (func_<addr8>) a port file gives
        # a function the decomp names differently (fptodp, WakeupThread, sceDopen). A name listed in
        # config/unbound_natives.txt for this address is skipped.
        for cand in (native_names.get(addr), marker_names.get(addr), func_names.get(addr), "func_%08x" % addr):
            if cand and cand in function_defs and cand != unbound_natives.get(addr):
                return cand
        return native_by_alias.get(addr)

    # An override's marker binds its address to the first function the override defines. A helper
    # defined ahead of the retail-named function takes that binding, so the retail function is
    # bypassed or the address becomes a trap; a static retail function cannot be linked by name at
    # all. Either is fatal unless config/native_names.txt names the native explicitly.
    overridden = {int(m[1], 16) for m in map(OVERRIDE_RE.match, (f.name for f in (ROOT / "overrides").glob("*.c"))) if m}
    # Its own names: cmd_tables later reads the retail image through `retail`, and later loops reuse addr/name.
    misbound = []
    for marker_addr, marker_name in sorted(compiled_markers.items()):
        if marker_addr not in overridden or marker_addr in native_names:
            continue
        expected = retail_named.get(marker_addr, set()) | {"func_%08x" % marker_addr}
        if marker_name in expected and marker_name in function_defs:
            continue
        defined_expected = sorted(expected & function_defs)
        if marker_name not in function_defs and marker_name in expected:
            misbound.append(f"overrides/{marker_addr:08x}.c: the marker's function {marker_name} is static or not compiled; "
                            "give it external linkage")
        elif defined_expected and native_for(marker_addr) not in defined_expected:
            misbound.append(f"overrides/{marker_addr:08x}.c: the marker binds {native_for(marker_addr) or 'a trap'} (first "
                            f"definition {marker_name}), but the retail-named {defined_expected[0]} is defined: define it "
                            "first after the marker, helpers after it")
    if misbound:
        # The first line names every file: the land log keeps only that line.
        die("address markers bound to the wrong function in " + ", ".join(row.split(":", 1)[0] for row in misbound)
            + ":\n  " + "\n  ".join(misbound))

    traps = {}   # trap name -> PS2 address (0 when unknown)
    data = {}    # absolute data symbol name -> PS2 address
    natives = set()
    hosts = set()  # host-library names the retail tables point at; declared, never stubbed
    unplaced = []
    aliases = {}  # linker alias name -> the function it names (func_alias)
    for name in candidates:
        if name in HOST_ALIASES:
            if HOST_ALIASES[name] not in host:
                die(f"{name}: alias target {HOST_ALIASES[name]} is not a host-library export")
            aliases[name] = HOST_ALIASES[name]
            continue
        if name in func_alias:
            aliases[name] = func_alias[name]
            continue
        fm = FUNC_NAME_RE.match(name)
        if fm and native_for(int(fm[1], 16)):
            # m2c-named call to a function this build compiles: thunk to the native, not a trap.
            aliases[name] = native_for(int(fm[1], 16))
            continue
        rk = kinds.get(name, set())
        kind, value = place_undefined(name, rk, syms)
        if a.ptr64 and kind == "data" and value in fn_starts:
            kind = "trap"        # the address of retail code taken by name: the function (or its trap), not data
        if kind is None:
            unplaced.append((name, value, rk))
        elif kind == "data" and not (image_lo <= value < image_hi):
            unplaced.append((name, f"data address 0x{value:08x} outside the retail image", rk))
        elif kind == "data" and a.ptr64:
            unplaced.append((name, f"retail data 0x{value:08x} referenced by symbol from 64-bit code (use P4_GDATA / P4_GPDATA, include/p4pc/gmem.h)", rk))
        elif kind == "data":
            data[name] = value
        else:
            # A named call to a function this build compiles at the same address: thunk, not a trap.
            native = native_for(value) if value else None
            if native:
                aliases[name] = native
            else:
                traps[name] = value
    for target in aliases.values():
        if target not in defined and target not in host:
            traps.setdefault(target, int(FUNC_NAME_RE.match(target)[1], 16))
    report = build / "unplaced_names.txt"
    if unplaced:
        report.write_text("".join(f"{n}\t{why}\t{','.join(sorted(rk)) or '-'}\n" for n, why, rk in unplaced))
        die(f"{len(unplaced)} undefined names have no placement (listed in {report}); "
            f"RenderWare extern storage goes in {ROOT / 'config' / 'port_owned.txt'}, "
            f"retail addresses in {ROOT / 'config' / 'port_symbols.txt'}")
    report.unlink(missing_ok=True)

    def target_for(addr):
        cand = native_for(addr)
        if cand:
            natives.add(cand)
            return cand
        name = func_names.get(addr) or marker_names.get(addr) or f"func_{addr:08x}"
        if name == unbound_natives.get(addr):
            name = f"func_{addr:08x}"
        if name in host and name not in defined:
            # The host library provides this name (libm sinf, SDL_Init, ...): point at the real function.
            hosts.add(name)
            return name
        traps.setdefault(name, addr)
        return name

    host_thunks = {n: target_for(syms[n][0]) for n in sorted(host_thunk_names)}

    # 64-bit: the register signature of a function (pointer or integer per parameter, guest.h) from the merged ABI table
    abi = json.loads(Path(a.abi).read_text()) if a.abi else {}

    def sig_c(name):
        e = abi.get(name)
        if not e or e.get("skip") or e.get("va") or len(e["params"]) > 6 or any(p is None or p["cls"] not in ("ptr", "int", "float") for p in e["params"]):
            return "0"
        if e["ret"] not in (None, "ptr", "int", "float"):
            return "0"
        # 'x': a float or double (xmm register)
        return '"' + ("p" if e["ret"] == "ptr" else "x" if e["ret"] == "float" else "i") + "".join(
            ("f" if p.get("fn") else "p") if p["cls"] == "ptr" else "x" if p["cls"] == "float" else "i" for p in e["params"]) + '"'

    starts = set(func_names) | set(marker_names)
    outside = sorted(s for s in starts if s & 3 or not any(lo <= s < hi for lo, hi in CODE_RANGES))
    if outside:
        die("function starts outside CODE_RANGES, which config/retail_fnptr_words.txt does not cover: "
            + ", ".join(f"{s:08x}" for s in outside[:20]))
    patches = []  # (word address, C target name)
    retired_words = {int(word, 16): int(code, 16) for word, code in inactive["retired_words"].items()}
    retired_seen = set()
    # 64-bit leaves the retail image intact and only registers the target
    # functions; retain its existing table output. Only 32-bit rewrites words.
    for va, value in function_pointer_words(pointer_rows, starts, not a.ptr64):
        if va in retired_words:
            if value != retired_words[va]:
                die(f"inactive movie word {va:08x}: expected {retired_words[va]:08x}, got {value:08x}")
            retired_seen.add(va)
            continue
        patches.append((va, target_for(value)))
    if retired_seen != set(retired_words):
        die("inactive movie registrations absent from retail pointer scan: "
            + ", ".join(f"{word:08x}" for word in sorted(set(retired_words) - retired_seen)))

    # A trap stub is an exe-level definition: it shadows the real function of the same name at link
    # time. The names a link provides come from the --host-lib list; one that list misses (winpthreads'
    # pthread_mutex_* on Windows, once) is trapped at address 0 and fails only when first called. These
    # prefixes name host libraries, never retail code, so a trap with one of them is a build error.
    host_like = sorted(n for n in traps if HOST_LIKE_RE.match(n))
    if host_like:
        die("trap stubs would shadow host library functions (add the library to --host-lib): "
            + ", ".join(host_like[:20]))
    check_addressless_traps(traps)
    lines = [
        "/* Generated by tools/gen_port.py tables. Do not edit. */",
        "#include <stddef.h>",
        '#include "p4pc/tables.h"',
        "/* Hidden: trap stubs may share libc names (strncpy, ...) and must not interpose libc. */",
        "#pragma GCC visibility push(hidden)",
        "",
    ]
    # Natives are declared here; traps are defined here (int, so callers that use a result get 0).
    for name in sorted(natives | hosts):
        lines.append(f"void {name}();")
    lines.append("")
    up = Tools.us if Tools.coff else "_" if Tools.macho else ""  # i686 COFF and Mach-O prefix C symbols with an underscore
    for name in sorted(data):
        # A PIC shared object must not export them: an exported absolute symbol is resolved at load time and Android's linker adds the load
        # base to it (the guest address becomes a host address in the library); hidden, the link resolves every use to the constant.
        hide = f".hidden {name}\\n" if Tools.pic and not Tools.coff and not Tools.macho else ""
        lines.append(f'__asm__(".globl {up}{name}\\n{hide}.set {up}{name}, 0x{data[name]:08x}\\n");')
    lines.append("")
    if a.ptr64:
        # A trap has no signature: an argument is a 32-bit integer or a host pointer into the guest block (a native passes pointers as they
        # are), which is logged as its guest address, not as the low half of a host address.
        lines.append("extern unsigned char *p4_guest_base;")
        lines.append("static inline unsigned p4trapw(unsigned long long v) { unsigned long long d = v - (unsigned long long)(size_t)p4_guest_base; "
                     "return v > 0xFFFFFFFFull && d <= 0xFFFFFFFFull ? (unsigned)d : (unsigned)v; }")
    for name in sorted(traps):
        # The first four argument slots go to p4pc_trap so a differential harness (tools/mipsdiff) can log them.
        if a.ptr64:
            lines.append(f'int {name}(unsigned long long a0, unsigned long long a1, unsigned long long a2, unsigned long long a3) '
                         f'{{ unsigned a[4] = {{ p4trapw(a0), p4trapw(a1), p4trapw(a2), p4trapw(a3) }}; '
                         f'return p4pc_trap_args("{name}", 0x{traps[name]:08x}, __builtin_return_address(0), a); }}')
        else:
            lines.append(f'int {name}(unsigned a0, unsigned a1, unsigned a2, unsigned a3) {{ unsigned a[4] = {{ a0, a1, a2, a3 }}; '
                         f'return p4pc_trap_args("{name}", 0x{traps[name]:08x}, __builtin_return_address(0), a); }}')
    lines.append("")
    # A real thunk, not `.set`: GAS would leave an alias of a symbol defined in another object undefined.
    def macho_thunk(symbol, target, hidden):
        # Mach-O: no .type/.size; a private extern stays within the executable
        hide = f".private_extern {symbol}\\n" if hidden else ""
        if Tools.arm64:
            return f'__asm__(".text\\n.p2align 2\\n.globl {symbol}\\n{hide}{symbol}:\\n\\tb {target}\\n");'
        return f'__asm__(".text\\n.globl {symbol}\\n{hide}{symbol}:\\n\\tjmp {target}\\n");'
    def thunk(name, target, hidden):
        if Tools.coff:
            u = Tools.us
            return (f'__asm__(".text\\n.globl {u}{name}\\n.def {u}{name}\\n.scl 2\\n.type 32\\n.endef\\n'
                    f'{u}{name}:\\n\\tjmp {u}{target}\\n");')
        if Tools.macho:
            return macho_thunk(f"_{name}", f"_{target}", hidden)
        hide = f".hidden {name}\\n" if hidden else ""
        if Tools.arm64:
            return f'__asm__("{hide}.globl {name}\\n.type {name},%function\\n{name}:\\n\\tb {target}\\n");'
        # a position-independent shared object (Android) cannot jump to a host function with a plain PC-relative relocation
        return f'__asm__("{hide}.globl {name}\\n.type {name},@function\\n{name}:\\n\\tjmp {target}{"@PLT" if Tools.pic else ""}\\n");'
    for name in sorted(aliases):
        lines.append(thunk(name, aliases[name], False))
    for name in sorted(host_thunks):
        lines.append(thunk(name, host_thunks[name], True))
    lines.append("")
    if a.ptr64:
        # the m2c names of retail functions that are thunks to a compiled function: a function pointer taken through such a name
        # (the address of `func_003bcbc0`) is the retail function, so the function table must know the thunk by its address
        alias_rows = []
        for name in sorted(aliases):
            m = FUNC_NAME_RE.match(name)
            if m:
                alias_rows.append((int(m[1], 16), name))
        for _, name in alias_rows:
            if name not in natives and name not in hosts:
                lines.append(f"void {name}();")
        lines.append("const p4pc_alias_t p4pc_alias_table[] = {")
        for addr, name in alias_rows:
            lines.append(f"    {{ 0x{addr:08x}, (void *)&{name} }},")
        lines.append("};")
        lines.append(f"const size_t p4pc_alias_count = {len(alias_rows)};")
        lines.append("")
    lines.append("const p4pc_patch_t p4pc_patch_table[] = {")
    for va, name in patches:
        lines.append(f"    {{ 0x{va:08x}, (void *)&{name}, {sig_c(name)} }},")
    lines.append("};")
    lines.append(f"const size_t p4pc_patch_count = {len(patches)};")
    lines.append("")
    # Every compiled function at a retail address, for tools/mipsdiff (it calls the native where the retail asm calls a function).
    native_rows = []
    for addr in sorted(starts | set(native_names)):
        cand = native_for(addr)
        if cand:
            native_rows.append((addr, cand))
    lines.append("/* Natives referenced by the table below are declared here. */")
    for cand in sorted({c for _, c in native_rows} - natives - hosts):
        lines.append(f"void {cand}();")
    lines.append("const p4pc_native_t p4pc_native_table[] = {")
    for addr, cand in native_rows:
        lines.append(f'    {{ 0x{addr:08x}, (void *)&{cand}, "{cand}", {sig_c(cand)} }},')
    lines.append("};")
    lines.append(f"const size_t p4pc_native_count = {len(native_rows)};")
    lines.append("")
    # 64-bit: every other function the ABI table knows (no retail address), by host address with its register signature
    have = {c for _, c in native_rows}
    host_rows = []
    if a.abi:
        for name in sorted(abi):
            sg = sig_c(name)
            if sg != "0" and name in defined and name not in have and name not in hosts and name not in traps and re.fullmatch(r"[A-Za-z_]\w*", name):
                host_rows.append((name, sg))
    # (declared as data under another name: a name's own prototype may be in a header this file includes)
    lines.append("/* Functions with no retail address, by symbol. */")
    for i, (name, _) in enumerate(host_rows):
        lines.append(f'extern char p4hf{i}[] __asm__("{up}{name}");')
    lines.append("const p4pc_hostfn_t p4pc_hostfn_table[] = {")
    for i, (name, sg) in enumerate(host_rows):
        lines.append(f"    {{ (void *)p4hf{i}, {sg} }},")
    if not host_rows:
        lines.append("    { 0, 0 }")
    lines.append("};")
    lines.append(f"const size_t p4pc_hostfn_count = {len(host_rows)};")
    lines.append("")
    write_if_changed(Path(a.out), "\n".join(lines))

    print(f"gen_port: tables: {len(patches)} function pointers patched "
          f"({len([1 for _, t in patches if t in natives])} native, {len([1 for _, t in patches if t not in natives])} trap)")
    print(f"gen_port: tables: {len(traps)} trap stubs, {len(data)} absolute data symbols, "
          f"{len(natives)} native symbols referenced")




def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("units")
    c.add_argument("--decomp", required=True)
    c.add_argument("--build", required=True)
    c.set_defaults(fn=cmd_units)

    m = sub.add_parser("mirror")
    m.add_argument("--decomp", required=True)
    m.add_argument("--build", required=True)
    m.add_argument("--mod-hooks", action="store_true",
                   help="emit typed per-address entry guards and host-only original-call bridges")
    m.add_argument("--refresh-contracts", action="store_true",
                   help="explicitly replace reviewed per-pass/per-unit edit expectations; never used by builds or nightly")
    m.set_defaults(fn=cmd_mirror)

    t = sub.add_parser("tables")
    t.add_argument("--decomp", required=True)
    t.add_argument("--build", required=True)
    t.add_argument("--objects-file", required=True)
    t.add_argument("--retail", help="optional retail SLUS_217.82: checked against config/retail_fnptr_words.txt")
    t.add_argument("--host-lib", action="append", required=True)
    t.add_argument("--project-lib", action="append", default=[],
                   help="compiled project archive or object whose definitions satisfy game references")
    t.add_argument("--abi", help="64-bit: the merged ABI table (tools/port64/unit64.py merge), for the register signatures of the function table")
    t.add_argument("--coff", action="store_true", help="the objects are i686 COFF (Windows build)")
    t.add_argument("--coff64", action="store_true", help="the objects are x86_64 COFF (the Windows x64 build, with --ptr64): no symbol prefix, units in obj64w")
    t.add_argument("--ptr64", action="store_true", help="64-bit objects (docs/port64.md): the decomp units are obj64/units, and retail data is never "
                   "a linker symbol, so a reference to it is an error")
    t.add_argument("--host-name", action="append", help="a host function that no --host-lib exports: bionic's libc_nonshared.a supplies atexit, at_quick_exit and pthread_atfork statically")
    t.add_argument("--arm64", action="store_true", help="the objects are AArch64 ELF: host thunks branch, the type directive is %%function")
    t.add_argument("--macho", action="store_true", help="the objects are Mach-O (macOS): C names carry an underscore, asm labels get thunks, "
                   "host names come from dylibs and the SDK's .tbd stubs; with --arm64 for Apple silicon")
    t.add_argument("--pic", action="store_true", help="the objects are position independent (the Android shared library): host thunks go through the PLT")
    t.add_argument("--nm", default="nm", help="nm for the target objects and host libraries")
    t.add_argument("--objdump", default="objdump", help="objdump for the target objects")
    t.add_argument("--out", required=True)
    t.set_defaults(fn=cmd_tables)

    f = sub.add_parser("fnptr-words", help="write config/retail_fnptr_words.txt from the retail executable")
    f.add_argument("--retail", required=True)
    f.add_argument("--out", default=str(FNPTR_WORDS))
    f.set_defaults(fn=cmd_fnptr_words)

    a = p.parse_args()
    a.fn(a)


if __name__ == "__main__":
    main()
