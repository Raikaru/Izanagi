#!/usr/bin/env python3
"""Writes tools/mipsdiff/sigs.txt: the C signature of every override and port-native function, as the
ovdiff harness needs it to build arguments and read the result.

    python tools/mipsdiff/sigs.py --gen build/gen

Sources: overrides/<addr8>.c (the definition named func_<addr8>, else the first non-static definition, prototype or K&R)
and every `func_<addr8>(...)` definition under src/. One line per function:

    <addr8> <name> <ret> <args>

ret: v void, i 32-bit integer or enum, p pointer, f float, q 64-bit integer, d double.
args: one letter per parameter, or `-` for none: i 32-bit integer, b s8, B u8/bool, h s16, H u16,
p pointer or array, f float, q packed 64-bit GP value, d GP-packed double/SSE value.
Small by-value records use the matching generated mirror's semantic Clang layout:
i/q retain their byte width and the existing port64 sx integer lowering; g is
a GP-packed float, e a GP-packed two-float HFA, n/m unaligned records of <=4/<=8
bytes. Larger/unknown records remain ? (unsupported), never scalar guesses.
"""
import argparse
import json
import os
import re
import sys
import subprocess
import tempfile
from pathlib import Path

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

INT_NAMES = {
    "int", "unsigned", "long", "s32", "u32", "RwInt32", "RwUInt32", "size_t", "uint32_t", "int32_t", "ssize_t",
    "uintptr_t", "intptr_t", "RwBool", "RwUInt", "ptrdiff_t", "u_int", "uint", "s_int", "RwFixed",
}
S8 = {"char", "s8", "int8_t", "RwInt8", "signed char"}
U8 = {"u8", "uint8_t", "RwUInt8", "unsigned char", "bool", "_Bool", "uchar", "RwChar"}
S16 = {"short", "s16", "int16_t", "RwInt16", "signed short"}
U16 = {"u16", "uint16_t", "RwUInt16", "unsigned short", "ushort"}
W64 = {"s64", "u64", "int64_t", "uint64_t", "long long", "unsigned long long", "RwInt64"}
FLT = {"float", "f32", "RwReal"}
DBL = {"double", "f64"}

DEF = re.compile(r"(?m)^([A-Za-z_][\w \t\*]*?)[ \t\r\n\*]+\b(\w+)[ \t]*\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)[ \t\r\n]*\{")
# A K&R definition (the decomp keeps C89-promoted call interfaces): an identifier list, the parameter
# declarations, then the body. Only override files are scanned with it.
KNR_DEF = re.compile(r"(?m)^([A-Za-z_][\w \t\*]*?)[ \t\r\n\*]+\b(\w+)[ \t]*\(([\w \t,]+)\)[ \t]*\r?\n"
                     r"((?:[ \t]*[A-Za-z_][^;{}()]*;[ \t]*\r?\n)+)[ \t]*\{")


def strip_comments(t):
    t = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), t, flags=re.S)
    return re.sub(r"//[^\n]*", "", t)


def classify(decl):
    """Letter for one parameter declaration (or a return type)."""
    d = " ".join(decl.replace("const", " ").replace("volatile", " ").replace("register", " ").split())
    if "*" in d or "[" in d:
        return "p"
    if d.endswith("..."):
        return "?"
    words = d.split()
    # drop the parameter name: the last identifier if more than one word and the rest forms a type
    for n in range(len(words), 0, -1):
        t = " ".join(words[:n])
        if t in FLT:
            return "f"
        if t in DBL:
            return "d"
        if t in W64:
            return "q"
        if t in S8:
            return "b"
        if t in U8:
            return "B"
        if t in S16:
            return "h"
        if t in U16:
            return "H"
        if t in INT_NAMES:
            return "i"
    if words and words[0] == "void":
        return "v"
    if words and words[0] in ("struct", "union"):
        return "?"
    if words and words[0] == "enum":
        return "i"
    # unknown typedef name: enum-like names are integers, anything else is assumed an integer too
    return "i"


def parse_def(m):
    ret_text = m.group(1).strip()
    if re.search(r"\bstatic\b", ret_text) or re.search(r"\binline\b", ret_text):
        return None
    ret_text = re.sub(r"\b(extern|static|inline|__inline__|P4_[A-Z_]+)\b", "", ret_text).strip()
    ret = "p" if "*" in m.group(0).split(m.group(2))[0] else classify(ret_text)
    params = m.group(3).strip()
    if params in ("", "void"):
        args = "-"
    else:
        parts, depth, cur = [], 0, ""
        for ch in params:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            if ch == "," and depth == 0:
                parts.append(cur)
                cur = ""
            else:
                cur += ch
        parts.append(cur)
        args = "".join(classify(p) for p in parts)
    return m.group(2), ret, args


def parse_knr_def(m):
    """parse_def for a K&R definition: each identifier takes its declaration's type (implicit int when undeclared)."""
    ret_text = m.group(1).strip()
    if re.search(r"\b(static|inline)\b", ret_text):
        return None
    ret_text = re.sub(r"\b(extern|__inline__|P4_[A-Z_]+)\b", "", ret_text).strip()
    ret = "p" if "*" in m.group(0).split(m.group(2))[0] else classify(ret_text)
    declared = {}
    for line in m.group(4).split(";"):
        parts = [p.strip() for p in line.split(",") if p.strip()]
        if not parts:
            continue
        first = re.match(r"^(.*?)([\s\*]*)\b(\w+)(\s*\[[^\]]*\])?$", parts[0])
        if not first:
            continue
        base = first.group(1).strip()
        declared[first.group(3)] = parts[0]
        for part in parts[1:]:
            name = re.search(r"(\w+)(\s*\[[^\]]*\])?$", part)
            if name:
                declared[name.group(1)] = base + " " + part
    names = [n.strip() for n in m.group(3).split(",") if n.strip()]
    args = "".join(classify(declared.get(n, "int " + n)) for n in names) or "-"
    return m.group(2), ret, args


def definitions(path, knr=False):
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    found = []
    for m in DEF.finditer(text):
        if m.group(2) in ("if", "while", "for", "switch", "return", "sizeof"):
            continue
        r = parse_def(m)
        if r:
            found.append((m.start(), r))
    if knr:
        for m in KNR_DEF.finditer(text):
            if m.group(2) in ("if", "while", "for", "switch", "return", "sizeof"):
                continue
            r = parse_knr_def(m)
            if r:
                found.append((m.start(), r))
    return [r for _, r in sorted(found, key=lambda item: item[0])]


# 00486970 mdlEffectResolveChildOrientation (u_long128): v0 is the quaternion only when no aim flag is set (lq $2, 0x50($a0)); after the aim path it holds what
# effMiscQuatMultiplyVU / effMiscEulerToQuatVU left (golden: 0x3f000000 in every call). Callers 00485630 and 00484bb0 (four sites) discard it.
IGNORED_RESULT = ("00486970",)

_MMI_NATIVE_ABIS = {
    "0019b730": ("i", "p"),
    "001ee490": ("i", "p"),
    "003a2ad0": ("i", "pp"),
    "003f6440": ("i", "ii"),
    "004adb50": ("v", "p"),
}


def _add_mmi_native_users(rows, decomp, render_definitions):
    """Direct lane users whose real providers were absent from the catalog."""
    render = [row for row in render_definitions if row[0] == "RpSkyRenderStateSet"]
    if len(render) != 1:
        raise SystemExit(f"MMI native 003f6440: expected one RpSkyRenderStateSet definition, found {len(render)}")
    for addr, expected in _MMI_NATIVE_ABIS.items():
        prototype = render[0] if addr == "003f6440" else rows.get(addr) or decomp.get(addr)
        if prototype is None or prototype[1:] != expected:
            raise SystemExit(f"MMI native {addr}: expected ABI {expected}, found {prototype}")
        rows[addr] = prototype


def record_parameter_class(shape, native):
    """Pack one <=8-byte EE record into its GP register, retaining host ABI."""
    if shape is None or shape.get("kind") != "record":
        return None
    size = shape["bytes"]
    if size is None or not 0 < size <= 8:
        return "?"
    if shape.get("unaligned"):
        return "n" if size <= 4 else "m"
    # The existing port64 mirror packs float-only records into an integer
    # parameter (sx); i386 still takes their original byte-sized stack copy.
    if native and native.get("sx"):
        return "i" if size <= 4 else "q"
    if shape["chunks"] == "F":
        if size <= 4:
            return "g"           # GP bits on EE, scalar SSE/HFA on the host
        if shape.get("hfa") == (4, 2):
            return "e"           # two floats: one SysV SSE or two AAPCS lanes
        return "d"               # one double, packed in the EE GP register
    return "i" if size <= 4 else "q"


def apply_record_parameters(rows, decomp, gen, abi_path, clang):
    """Reuse the port's semantic layout oracle, never guess an alias's width."""
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import audit_prototypes as audit
    native_abi = json.loads(Path(abi_path).read_text())
    wanted = {name for name, _, _ in list(rows.values()) + list(decomp.values())
              if name in native_abi and any(
                  parameter is None or parameter.get("sx")
                  for parameter in native_abi[name]["params"])}
    mirror = Path(gen).resolve() / "decomp"
    audit.class_audit_init(ROOT, str(mirror), str(Path(abi_path).resolve()), clang)
    gr, unit = audit._gr, audit._unit64
    resolved, details, changes = {}, {}, []
    for entry in (Path(gen) / "units.txt").read_text().splitlines():
        path, _, raw_flags = entry.partition("|")
        text = strip_comments(Path(path).read_text(encoding="utf-8", errors="replace"))
        names = {match.group(2) for match in DEF.finditer(text)} & wanted
        if not names:
            continue
        flags = unit.T32 + unit.base_flags(ROOT, [
            flag for flag in raw_flags.split("\t") if flag and flag != "-ferror-limit=0"])
        with tempfile.TemporaryDirectory(prefix="p4pc-sigs-records-") as work:
            preprocessed = os.path.join(work, "unit.i")
            gr.preprocess(path, flags, preprocessed, clang)
            command = [clang, "-fsyntax-only", "-Wno-everything", "-Xclang", "-ast-dump=json"]
            parsed = subprocess.run(command + gr.stage2_flags(flags) +
                                    ["-x", "cpp-output", preprocessed], capture_output=True, text=True)
            if parsed.returncode:
                raise RuntimeError(f"aggregate signature AST {path}: {parsed.stderr}")
            ast = json.loads(parsed.stdout)
            gr._MAIN[0] = preprocessed
            reader = gr.Rewriter(ast, str(mirror), ifile=preprocessed)
            reader.index(ast)
            layout_cache = None
            def layouts():
                nonlocal layout_cache
                if layout_cache is None:
                    layout_cache = gr.oracle_layouts(path, flags, clang)
                return layout_cache
            for definition in ast.get("inner", ()):
                name = definition.get("name")
                if (definition.get("kind") != "FunctionDecl" or name not in names or
                        not any(child.get("kind") == "CompoundStmt"
                                for child in definition.get("inner", ()))):
                    continue
                parameters = [child for child in definition.get("inner", ())
                              if child.get("kind") == "ParmVarDecl"]
                native = native_abi[name]["params"]
                if len(native) != len(parameters):
                    raise RuntimeError(f"aggregate signature arity changed: {name}")
                classes = []
                record_details = []
                for index, (parameter, lowered) in enumerate(zip(parameters, native)):
                    if reader.find_record(parameter) is None:
                        classes.append(None)
                    else:
                        shape = audit.argument_shape(reader, parameter, layouts)
                        classes.append(record_parameter_class(shape, lowered) or "?")
                        record_details.append({"parameter": index, "type": reader.qt(parameter),
                                               "shape": shape, "native": lowered,
                                               "class": classes[-1]})
                if definition.get("variadic"):
                    classes.append(None)  # retain the catalog's unsupported ... slot
                classes = tuple(classes)
                if name in resolved and resolved[name] != classes:
                    raise RuntimeError(f"conflicting aggregate signatures: {name}")
                resolved[name] = classes
                details.setdefault(name, record_details)
    for catalog in (rows, decomp):
        for address, (name, ret, args) in catalog.items():
            if name not in resolved:
                continue
            classes = resolved[name]
            if not any(classes):
                continue
            if len(classes) != len(args):
                raise RuntimeError(f"aggregate signature parameter count changed: {address} {name}")
            updated = "".join(replacement or original for original, replacement in zip(args, classes))
            catalog[address] = (name, ret, updated)
            if updated != args:
                changes.append({"address": address, "name": name, "before": args,
                                "after": updated, "records": details[name]})
    return changes


def main():
    options = argparse.ArgumentParser(description=__doc__)
    options.add_argument("--gen", default=os.path.join(ROOT, "build", "gen"),
                         help="matching generated mirror and units.txt")
    options.add_argument("--abi", help="matching obj64/abi.json (defaults below --gen)")
    options.add_argument("--clang", default="clang")
    options.add_argument("--out", help="generated catalog path (defaults to tools/mipsdiff/sigs.txt)")
    options.add_argument("--record-report", help="write the semantic per-address aggregate audit as JSON")
    cli = options.parse_args()
    rows = {}
    ovdir = os.path.join(ROOT, "overrides")
    for f in sorted(os.listdir(ovdir)):
        m = re.match(r"^([0-9a-f]{8})\.c$", f)
        if not m:
            continue
        addr = m.group(1)
        defs = definitions(os.path.join(ovdir, f), knr=True)
        pick = [d for d in defs if d[0] == "func_" + addr] or (defs if len(defs) == 1 else defs[:1])
        if pick:
            rows[addr] = pick[0]
    pat = re.compile(r"^func_([0-9a-f]{8})$")
    for dp, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if not f.endswith(".c"):
                continue
            for name, ret, args in definitions(os.path.join(dp, f)):
                m = pat.match(name)
                if m and m.group(1) not in rows:
                    rows[m.group(1)] = (name, ret, args)
    # The RenderWare clean layer (src/rwclean, docs/rw_clean.md): each `// FUN_<ADDR>` marker binds the definition after it to
    # that address when its subsystem is compiled (P4_RW_CLEAN); the signature is the API's, the same in either build.
    clean_marker = re.compile(r"//[ \t]*FUN_([0-9A-Fa-f]{8})")
    clean_dir = os.path.join(ROOT, "src", "rwclean")
    for f in sorted(os.listdir(clean_dir)) if os.path.isdir(clean_dir) else []:
        if not f.endswith(".c"):
            continue
        text = open(os.path.join(clean_dir, f), encoding="utf-8").read()
        marks = [(m.group(1).lower(), m.end()) for m in clean_marker.finditer(text)]
        for k, (addr, end) in enumerate(marks):
            nxt = marks[k + 1][1] - 20 if k + 1 < len(marks) else len(text)
            for m in DEF.finditer(strip_comments(text[end:nxt])):
                r = parse_def(m)
                if r:
                    rows.setdefault(addr, r)
                    break
    # Functions src/cri defines under their decomp symbol name (ACSSVR_Init, ACSSND_Stop, ...): the address
    # comes from the decomp's symbol table or config/port_symbols.txt.
    named = {}
    for sym in (os.path.join(ROOT, "decomp", "config", "symbol_addrs.txt"), os.path.join(ROOT, "config", "port_symbols.txt")):
        if os.path.isfile(sym):
            for line in open(sym, encoding="utf-8", errors="replace"):
                m = re.match(r"^(\w+)\s*=\s*0x([0-9A-Fa-f]{8})\b.*type:func", line) or re.match(r"^(\w+)\s+0x([0-9A-Fa-f]{8})\s+func\b", line)
                if m:
                    named.setdefault(m.group(1), m.group(2).lower())
    for dp, _, fs in os.walk(os.path.join(ROOT, "src", "cri")):
        for f in fs:
            if f.endswith(".c"):
                for name, ret, args in definitions(os.path.join(dp, f)):
                    if name in named and named[name] not in rows:
                        rows[named[name]] = (name, ret, args)
    # The decomp's own functions (marker `// FUN_<ADDR>` followed by the definition): kept only where the default
    # (all-integer arguments, an integer result) would be wrong, so ovdiff can call them as callees correctly. Rows
    # marked `d` are never tested themselves.
    marker = re.compile(r"//[ \t]*FUN_([0-9A-Fa-f]{8})")
    decomp, named_decomp = {}, {}
    for dp, _, fs in os.walk(os.path.join(ROOT, "decomp", "src")):
        for f in fs:
            if not f.endswith(".c"):
                continue
            text = open(os.path.join(dp, f), encoding="utf-8", errors="replace").read()
            # RenderWare's named definitions do not all carry FUN markers.
            # Resolve those through the same symbol table used for port CRI
            # definitions; never lose an ABI row merely for lacking a marker.
            for definition in DEF.finditer(strip_comments(text)):
                prototype = parse_def(definition)
                if prototype and prototype[0] in named:
                    named_decomp.setdefault(named[prototype[0]], prototype)
            if "FUN_" not in text:
                continue
            marks = [(m.group(1).lower(), m.end(), m.start()) for m in marker.finditer(text)]
            for k, (addr, end, start) in enumerate(marks):
                nxt = marks[k + 1][1] - 20 if k + 1 < len(marks) else len(text)
                chunk = strip_comments(text[end:nxt])
                for m in DEF.finditer(chunk):
                    if m.group(2) in ("if", "while", "for", "switch", "return", "sizeof"):
                        continue
                    r = parse_def(m)
                    if r:
                        # `#pragma tailcall on` ahead of a void wrapper: the wrapper jumps to a function that returns a value
                        # (func_003c54a0 -> the allocator slot), so the retail function's result is that value
                        if r[1] == "v" and "tailcall on" in text[max(0, start - 40):start]:
                            r = (r[0], "i", r[2])
                        decomp.setdefault(addr, r)
                    break
    for address, prototype in named_decomp.items():
        decomp.setdefault(address, prototype)
    _add_mmi_native_users(rows, decomp, definitions(os.path.join(ROOT, "src/render/p4gl_state.c")))
    # Results retail leaves undefined: the C type returns a value, the retail code returns it only on one path and every caller
    # ignores it (a register left over from the last callee on the other path), so the diff must not compare it.
    for a in IGNORED_RESULT:
        if a in rows:
            rows[a] = (rows[a][0], "v", rows[a][2])
    record_changes = apply_record_parameters(rows, decomp, cli.gen,
                                            cli.abi or os.path.join(cli.gen, "obj64", "abi.json"), cli.clang)
    if cli.record_report:
        Path(cli.record_report).write_text(json.dumps(record_changes, indent=2, sort_keys=True) + "\n")
    out = cli.out or os.path.join(ROOT, "tools", "mipsdiff", "sigs.txt")
    nd = 0
    with open(out, "w", newline="\n") as fh:
        fh.write("# <addr8> <name> <ret> <args> <o|d>, generated by tools/mipsdiff/sigs.py (o: override or port native, tested by ovdiff;\n"
                 "# d: a decomp function whose signature is not the all-integer default, used only to call it as a callee)\n")
        for a in sorted(rows):
            n, r, ar = rows[a]
            fh.write("%s %s %s %s o\n" % (a, n, r, ar))
        for a in sorted(decomp):
            if a in rows:
                continue
            n, r, ar = decomp[a]
            if r in "ip" and not re.search("[fqdbBhH?gemn]", ar):
                continue
            nd += 1
            fh.write("%s %s %s %s d\n" % (a, n, r, ar))
    print("sigs.py: %d signatures (+%d decomp callee signatures) -> %s" % (len(rows), nd, out))


if __name__ == "__main__":
    sys.exit(main())
