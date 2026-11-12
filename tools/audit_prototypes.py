#!/usr/bin/env python3
"""Audit the mirrored decomp tree for calls through declarations without a prototype.

K&R declarations (`float sinf();`) make the caller promote float arguments to double and pass any
argument count, so a call can disagree with the callee's real definition without any compile error. On
the PS2 and on x86 alike the callee then reads garbage. This runs clang over every unit of a lane's
build/gen (units.txt) with the diagnostics that expose the class and prints the findings by callee:

  - `-Wdeprecated-non-prototype`  a call that passes arguments through a declaration with no prototype
  - `-Wstrict-prototypes`         an unprototyped declaration or definition
  - `-Wimplicit-function-declaration`  a call to an undeclared function (implicitly `int f()`)
  - `-Wincompatible-function-pointer-types`, `-Wincompatible-pointer-types`

Usage (inside the p4pc-ci container, or anywhere with clang for i386): python3 tools/audit_prototypes.py
--gen <build/gen> [--jobs N] [--json out.json].

`--classes --abi <obj64/abi.json> --json <report.json>` preprocesses every active unit and checks every
call's effective GP/FP class sequence against the compiled callee ABI. It resolves asm labels and
immutable pointer aliases, reports dynamic/unknown targets explicitly, and rejects incomplete ASTs.
The source AST uses the build's i386 layout oracle: the original mirror's retail-sized assertions are
not valid on an unrewritten 64-bit layout. Pointer/integer widths normalize to GP; supported small
record arguments are classified after the existing port64 packing adapter, not mistaken for missing scalars.
Opaque record slots are checked against their concrete callee definitions after all units are parsed:
byte sizes, eightbyte register classes and homogeneous-float widths must agree. Unsupported record
layouts remain explicit unknowns; pointer-versus-record calls are never counted as one matching GP slot.
Unlike comparing argument expressions before implicit conversions, this does not flag legitimate
integer literals converted by a correct float prototype.
"""
import argparse
import collections
import json
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor
from pathlib import Path

WARNINGS = ["-Wstrict-prototypes", "-Wdeprecated-non-prototype", "-Wimplicit-function-declaration",
            "-Wincompatible-function-pointer-types", "-Wincompatible-pointer-types",
            "-Wold-style-definition", "-Wdouble-promotion", "-Wno-incompatible-library-redeclaration", "-Wno-builtin-requires-header"]
DIAG = re.compile(r"^(?P<file>[^:\n]+):(?P<line>\d+):(?P<col>\d+): warning: (?P<msg>.*) \[(?P<flag>-W[\w-]+)\]$")
NAME = re.compile(r"'([^']+)'")


def run_unit(entry, extra):
    path, _, flags = entry.partition("|")
    cmd = ["clang", "--target=i386-linux-gnu", "-m32", "-fsyntax-only", "-Wno-everything"] + WARNINGS + extra + flags.split("\t") + [path]
    p = subprocess.run(cmd, capture_output=True, text=True)
    return path, p.stderr


def arity_report(gen, findings, out):
    """Calls through an unprototyped declaration whose argument count differs from the callee's definition."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import gen_port as gp
    mirror = gen / "decomp"
    texts = {}
    for p in sorted(list((mirror / "src").rglob("*.c")) + list((mirror / "src").rglob("*.h")) + list((mirror / "include").rglob("*.h"))):
        texts[p.relative_to(mirror).as_posix()] = p.read_text(encoding="utf-8", errors="surrogateescape")
    td = gp._typedef_classes(list(texts.values()))
    shapes = collections.defaultdict(set)
    sources = list(texts.values()) + [p.read_text(encoding="utf-8", errors="surrogateescape")
                                      for p in sorted((gp.ROOT / "src").rglob("*.c"))]
    for text in sources:
        for name, is_def, ret, params, variadic in gp._scan_functions(gp._strip_literals(text), td):
            if is_def and not variadic:
                shapes[name].add(len([] if params is None else params))
    rows = collections.defaultdict(list)
    forwarders = 0
    stripped = {}
    for f in findings:
        if f["flag"] != "-Wdeprecated-non-prototype" or not f["callee"] or len(shapes.get(f["callee"], ())) != 1:
            continue
        text = texts.get(f["file"])
        if text is None:
            continue
        if f["file"] not in stripped:
            stripped[f["file"]] = gp._strip_literals(text)
        st = stripped[f["file"]]
        offset = sum(len(l) + 1 for l in st.split("\n")[:f["line"] - 1]) + f["col"] - 1
        open_idx = st.find("(", offset)
        if open_idx < 0 or open_idx - offset > 80:
            continue
        spans, _ = gp._split_args_at(st, open_idx)
        passed = 0 if not "".join(st[a:b] for a, b in spans).strip() else len(spans)
        if passed == 4 and [st[a:b].strip() for a, b in spans] == ["a0", "a1", "a2", "a3"]:
            forwarders += 1       # a register forwarder: hands all four argument registers to the native
            continue
        defined = next(iter(shapes[f["callee"]]))
        if passed != defined:
            rows[f["callee"]].append((f["file"], f["line"], passed, defined))
    lines = ["# Calls whose argument count differs from the definition (generated)", "",
             "Generated by `tools/audit_prototypes.py --arity`. Each row is a call through a declaration without a",
             "prototype that passes a different number of arguments than the callee's definition takes. Too few",
             "means the callee reads whatever is on the stack where the argument should be (on the PS2 it read a",
             "register the caller never set, often the caller's own incoming argument: check the retail asm); too many",
             "is harmless on cdecl. Fix a site with a `config/mirror_fixes.txt` row checked against the asm.", "",
             f"{sum(len(v) for v in rows.values())} calls to {len(rows)} functions (not counting {forwarders} register forwarders that",
             "pass `a0, a1, a2, a3` on to a native, which is by design).", "",
             "| Callee | Passed | Defined | Call sites |", "|---|---|---|---|"]
    for name, sites in sorted(rows.items(), key=lambda kv: (-len(kv[1]), kv[0])):
        by = collections.defaultdict(list)
        for file, line, passed, defined in sites:
            by[(passed, defined)].append(f"{file}:{line}")
        for (passed, defined), where in sorted(by.items()):
            shown = ", ".join(where[:6]) + (f" (+{len(where) - 6})" if len(where) > 6 else "")
            lines.append(f"| `{name}` | {passed} | {defined} | {shown} |")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"arity report: {sum(len(v) for v in rows.values())} calls to {len(rows)} functions -> {out}")


def class_audit_init(root, mirror, abi, clang):
    """Each worker owns Clang's AST/source state; it must not be shared between threads."""
    global _class_root, _class_mirror, _class_abi, _class_clang, _gr, _unit64
    _class_root, _class_mirror, _class_clang = root, mirror, clang
    _class_abi = json.loads(Path(abi).read_text())
    sys.path.insert(0, str(Path(root) / "tools" / "port64"))
    import gptr_rewrite
    import unit64
    _gr, _unit64 = gptr_rewrite, unit64


def function_reference(node, declarations, seen=None):
    """A direct function, including asm labels/casts and immutable function-pointer aliases."""
    seen = set() if seen is None else seen
    while node.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr", "UnaryOperator") and node.get("inner"):
        node = node["inner"][0]
    if node.get("kind") != "DeclRefExpr":
        return None
    ref = node.get("referencedDecl", {})
    if ref.get("kind") == "FunctionDecl":
        return ref
    decl = declarations.get(ref.get("id"), {})
    if decl.get("kind") != "VarDecl" or decl.get("id") in seen:
        return None
    # An initializer is not a points-to proof for a mutable variable.
    if not re.search(r"\*\s*const\b", decl.get("type", {}).get("qualType", "")):
        return None
    seen.add(decl["id"])
    for child in decl.get("inner", ()):
        result = function_reference(child, declarations, seen)
        if result:
            return result
    return None


def integer_argument(node):
    """Literal opcode operands, after Clang has expanded macros (unknown values stay unknown)."""
    while node.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr", "ConstantExpr") and node.get("inner"):
        node = node["inner"][0]
    if node.get("kind") == "IntegerLiteral":
        return int(node["value"], 0)
    if node.get("kind") == "UnaryOperator" and node.get("opcode") in ("+", "-"):
        value = integer_argument(node["inner"][0])
        return None if value is None else (-value if node["opcode"] == "-" else value)
    return None

def argument_shape(reader, node, layouts, seen=()):
    """A source argument's concrete shape; records are not scalar GP placeholders."""
    qt = reader.qt(node)
    record = reader.find_record(node)
    if record is None:
        anonymous = re.fullmatch(
            r"(struct|union) \((?:unnamed|anonymous)(?: (?:struct|union))? at (.*):(\d+):(\d+)\)", qt)
        if anonymous:
            site = (anonymous[2], int(anonymous[3]), int(anonymous[4]))
            matches = []
            for declaration in reader.decl_by_id.values():
                if (declaration.get("kind") != "RecordDecl"
                        or declaration.get("tagUsed") != anonymous[1]
                        or not declaration.get("completeDefinition")):
                    continue
                location = _gr.span(declaration)
                if location and tuple(reader.origin_of(location[0], location[1])) == site:
                    matches.append(declaration)
            if len(matches) == 1:
                record = matches[0]
    if record is None:
        kind = reader.pclass(qt)
        c = reader.clq(qt)
        if kind == "ptr":
            return {"kind": "ptr", "bytes": 4, "class": "G"}
        base = reader.builtin_text(c["base"]) if c["ok"] else qt
        size = reader.SCALAR_SIZES.get(base)
        if size is None:
            size = {"long": 4, "unsigned long": 4, "__int128": 16,
                    "unsigned __int128": 16}.get(base)
        if kind in ("int", "float"):
            return {"kind": kind, "bytes": size, "class": "F" if kind == "float" else "G"}
        return None
    if record["id"] in seen:
        return None
    fields = [field for field in record.get("inner", ()) if field.get("kind") == "FieldDecl"]
    if any(field.get("isBitfield") or not field.get("name") for field in fields):
        return None
    names = [field["name"] for field in fields]
    base = reader.clq(qt).get("base", "")
    keys = [qt, base, re.sub(r"^(struct|union) ", "", base),
            record.get("tagUsed", "struct") + " " + qt]
    if record.get("name"):
        keys.insert(0, record.get("tagUsed", "struct") + " " + record["name"])
    else:
        location = _gr.span(record)
        if location:
            file, line, column = reader.origin_of(location[0], location[1])
            keys.insert(0, f"{record.get('tagUsed', 'struct')} (unnamed at {file}:{line}:{column})")
    oracle = layouts()
    candidates = [layout for key in keys for layout in oracle.get(key, ())
                  if [name for _, name in layout["fields"]] == names]
    if not candidates:
        candidates = [layout for variants in oracle.values() for layout in variants
                      if [name for _, name in layout["fields"]] == names]
    unique = {(layout["size"], tuple(layout["fields"])) for layout in candidates
              if layout["size"] is not None}
    if len(unique) != 1:
        return None
    size, offsets = unique.pop()
    lanes = []
    for field, (offset, _) in zip(fields, offsets):
        field_type = reader.clq(reader.qt(field))
        if not field_type["ok"]:
            return None
        dimensions = re.findall(r"\[([^]]*)\]", field_type["dims"])
        if any(not str(dimension).isdigit() for dimension in dimensions):
            return None
        count = 1
        for dimension in dimensions:
            count *= int(dimension)
        element = field
        if dimensions:
            element = dict(field, type=dict(field["type"]))
            element["type"]["qualType"] = field_type["base"] + "*" * field_type["levels"]
        shape = argument_shape(reader, element, layouts, seen + (record["id"],))
        if shape is None or shape["bytes"] is None:
            return None
        for index in range(count):
            start = offset + index * shape["bytes"]
            if shape["kind"] == "record":
                lanes.extend((start + at, width, cls) for at, width, cls in shape["lanes"])
            else:
                lanes.append((start, shape["bytes"], shape["class"]))
    # SysV INTEGER dominates SSE when a union/mixed record overlaps an eightbyte.
    chunks = []
    for start in range(0, size, 8):
        classes = {cls for at, width, cls in lanes if at < start + 8 and at + width > start}
        chunks.append("G" if "G" in classes else "F" if "F" in classes else "_")
    floats = {width for _, width, cls in lanes if cls == "F"}
    hfa = (next(iter(floats)), len(lanes)) if (
        record.get("tagUsed") == "struct" and len(floats) == 1
        and all(cls == "F" for _, _, cls in lanes) and 1 <= len(lanes) <= 4) else None
    return {"kind": "record", "bytes": size, "class": "G", "chunks": "".join(chunks),
            "hfa": hfa, "lanes": lanes,
            "unaligned": any(at % width for at, width, _ in lanes)}


def aggregate_compatible(have, want):
    """Retain whole records; only existing, integer-bit-compatible packing is accepted."""
    if have is None or want is None or want["kind"] != "record":
        return None
    if have["kind"] == "record":
        return (have["bytes"], have["chunks"], have["hfa"], have.get("unaligned", False)) == (
            want["bytes"], want["chunks"], want["hfa"], want.get("unaligned", False))
    return (have["kind"] == "int" and have["bytes"] == want["bytes"]
            and want["bytes"] <= 8 and want["chunks"] == "G" and want["hfa"] is None
            and not want.get("unaligned", False))


def finish_aggregate_calls(deferred, definitions, local_definitions):
    """Resolve opaque ABI slots from actual callee definitions after all units were parsed."""
    counts, findings, unresolved = collections.Counter(), [], []
    for row, abi, arguments, packed in deferred:
        key = (row["unit"], row["linked"])
        choices = local_definitions.get(key) or definitions.get(row["linked"], [])
        unique = {json.dumps(choice, sort_keys=True): choice for choice in choices}
        if len(unique) != 1:
            counts["parameter_class_unresolved"] += 1
            unresolved.append(dict(row, reason="missing or conflicting concrete aggregate definition"))
            continue
        shapes = next(iter(unique.values()))
        if (len(shapes) != len(abi["params"]) or any(shape is None for shape in shapes)
                or any(shape is None for shape in arguments[:len(shapes)])
                or any(parameter is not None and parameter["cls"] not in ("int", "ptr", "float")
                       for parameter in abi["params"])
                or any(parameter is None and shape["kind"] != "record"
                       for parameter, shape in zip(abi["params"], shapes))):
            counts["parameter_class_unresolved"] += 1
            unresolved.append(dict(row, reason="unsupported concrete argument layout"))
            continue
        have, bad_records, widths = [], [], []
        want = ["F" if parameter is not None and parameter["cls"] == "float" else "G"
                for parameter in abi["params"]]
        for index, (argument, parameter, shape) in enumerate(zip(arguments, abi["params"], shapes)):
            actual = argument["class"]
            if parameter is None:
                compatible = aggregate_compatible(argument, shape)
                if compatible is not True:
                    bad_records.append(index)
            else:
                if argument["kind"] == "record":
                    if not abi.get("va") and parameter["cls"] == "int" and packed[index]:
                        actual = "G"
                        counts["packed_record_arguments"] += 1
                    else:
                        bad_records.append(index)
                if parameter["cls"] == "float" and argument["kind"] == "float":
                    if (argument["bytes"] == 8) != (parameter["ct"].strip() == "double"):
                        widths.append(index)
            have.append(actual)
        have.extend(argument["class"] if argument is not None else "?"
                    for argument in arguments[len(abi["params"]):])
        counts["checked"] += 1
        counts["aggregate_calls_checked"] += 1
        if abi.get("va"):
            counts["variadic_fixed_parameters"] += 1
        different = have[:min(len(have), len(want))] != want[:min(len(have), len(want))]
        if bad_records or different or len(arguments) < len(abi["params"]) or widths:
            kind = ("aggregate-abi" if bad_records else
                    ("class-order" if len(have) == len(want) and sorted(have) == sorted(want)
                     else "class-count") if different else
                    "under-passing" if len(arguments) < len(abi["params"]) else "float-width")
            findings.append(dict(row, kind=kind, have="".join(have), want="".join(want),
                                 passed=len(arguments), defined=len(abi["params"]),
                                 float_width_positions=widths, aggregate_positions=bad_records,
                                 argument_shapes=arguments, parameter_shapes=shapes))
    return counts, findings, unresolved


def audit_class_unit(entry):
    path, _, raw_flags = entry.partition("|")
    unit = path.replace("\\", "/").split("/decomp/", 1)[-1]
    counts, findings, unresolved, calls = collections.Counter(), [], [], []
    definitions, local_shapes, deferred = {}, {}, []
    error = None
    try:
        flags = _unit64.T32 + _unit64.base_flags(_class_root, [
            flag for flag in raw_flags.split("\t") if flag and flag != "-ferror-limit=0"])
        with tempfile.TemporaryDirectory(prefix="p4pc-call-classes-") as work:
            preprocessed = str(Path(work) / "unit.i")
            _gr.preprocess(path, flags, preprocessed, _class_clang)
            command = [_class_clang, "-fsyntax-only", "-Wno-everything", "-Xclang", "-ast-dump=json"]
            parsed = subprocess.run(command + _gr.stage2_flags(flags) + ["-x", "cpp-output", preprocessed],
                                    capture_output=True, text=True)
            if parsed.returncode:
                raise RuntimeError(parsed.stderr)
            ast = json.loads(parsed.stdout)
            _gr._MAIN[0] = preprocessed
            reader = _gr.Rewriter(ast, _class_mirror, ifile=preprocessed)
            reader.index(ast)
            layout_cache = None

            def layouts():
                nonlocal layout_cache
                if layout_cache is None:
                    layout_cache = _gr.oracle_layouts(path, flags, _class_clang)
                return layout_cache

            def parameter_shapes(definition):
                return [argument_shape(reader, child, layouts) for child in definition.get("inner", ())
                        if child.get("kind") == "ParmVarDecl"]

            local_nodes = {}
            local = {}
            for definition in ast.get("inner", ()):
                if definition.get("kind") == "FunctionDecl" and any(
                        child.get("kind") == "CompoundStmt" for child in definition.get("inner", ())):
                    linked = definition.get("mangledName") or definition["name"]
                    local[linked] = reader.abi_of(definition)
                    local_nodes[linked] = definition
                    if definition.get("storageClass") != "static" and any(
                            parameter is None for parameter in local[linked]["params"]):
                        definitions[linked] = parameter_shapes(definition)
            stack = [(ast, None)]
            while stack:
                node, caller = stack.pop()
                if node.get("kind") == "FunctionDecl":
                    caller = node.get("name")
                stack.extend((child, caller) for child in node.get("inner", ()) if isinstance(child, dict))
                if node.get("kind") != "CallExpr":
                    continue
                reader.cur = node
                site = reader.site()
                if site is None:
                    continue
                counts["calls"] += 1
                file, line, col = site
                row = {"unit": unit, "file": file.replace("\\", "/"), "line": line, "col": col, "caller": caller}
                call = dict(row, passed=len(node["inner"]) - 1, callee=None, linked=None,
                            constant_args=[integer_argument(argument) for argument in node["inner"][1:]])
                calls.append(call)
                reference = function_reference(node["inner"][0], reader.decl_by_id)
                if reference is None:
                    counts["indirect_unresolved"] += 1
                    unresolved.append(dict(row, reason="dynamic function pointer",
                                           declared_type=reader.qt(node["inner"][0])))
                    continue
                name = reference["name"]
                declaration = reader.decl_by_id.get(reference["id"], {})
                linked = declaration.get("mangledName") or name
                call.update(callee=name, linked=linked)
                definition = local.get(linked) or _class_abi.get(linked)
                if definition is None:
                    counts["definition_unavailable"] += 1
                    unresolved.append(dict(row, callee=name, linked=linked, reason="no compiled definition or ABI row"))
                    continue
                params = definition["params"]
                if any(param is None or param["cls"] not in ("int", "ptr", "float") for param in params):
                    if linked in local_nodes:
                        local_shapes[linked] = parameter_shapes(local_nodes[linked])
                    arguments = node["inner"][1:]
                    packed = []
                    for argument in arguments:
                        value = argument
                        while value.get("kind") in ("ParenExpr", "ImplicitCastExpr") and value.get("inner"):
                            value = value["inner"][0]
                        packed.append(reader.small_record_size(value) is not None)
                    deferred.append((dict(row, callee=name, linked=linked,
                                          declared_type=reader.qt(node["inner"][0])),
                                     definition, [argument_shape(reader, argument, layouts) for argument in arguments],
                                     packed))
                    continue
                arguments = node["inner"][1:]
                normalize = lambda value: "F" if value == "float" else ("G" if value in ("int", "ptr") else "?")
                have = [normalize(reader.pclass(reader.qt(argument))) for argument in arguments]
                want = [normalize(param["cls"]) for param in params]
                if not definition.get("va"):
                    for index, (argument, param) in enumerate(zip(arguments, params)):
                        value = argument
                        while value.get("kind") in ("ParenExpr", "ImplicitCastExpr") and value.get("inner"):
                            value = value["inner"][0]
                        if have[index] == "?" and param["cls"] == "int" and reader.small_record_size(value) is not None:
                            have[index] = "G"
                            counts["packed_record_arguments"] += 1
                counts["checked"] += 1
                if definition.get("va"):
                    counts["variadic_fixed_parameters"] += 1
                    compared = have[:len(want)]
                else:
                    compared = have
                different = compared[:min(len(compared), len(want))] != want[:min(len(compared), len(want))]
                widths = [index for index, (argument, param) in enumerate(zip(arguments, params))
                          if param["cls"] == "float" and reader.pclass(reader.qt(argument)) == "float"
                          and (argument.get("type", {}).get("desugaredQualType", reader.qt(argument)).strip() == "double")
                          != (param["ct"].strip() == "double")]
                if different or len(arguments) < len(params) or widths:
                    kind = ("class-order" if len(compared) == len(want) and sorted(compared) == sorted(want)
                            else "class-count") if different else ("under-passing" if len(arguments) < len(params) else "float-width")
                    findings.append(dict(row, callee=name, linked=linked, kind=kind, have="".join(have),
                                         want="".join(want), passed=len(arguments), defined=len(params),
                                         float_width_positions=widths,
                                         declared_type=reader.qt(node["inner"][0])))
    except (Exception, SystemExit) as exception:
        error = {"unit": unit, "error": str(exception)}
    return {"unit": unit, "counts": dict(counts), "findings": findings, "unresolved": unresolved, "error": error,
            "calls": calls, "definitions": definitions, "local_definitions": local_shapes, "deferred": deferred}


def class_report(args, units):
    root = Path(__file__).resolve().parents[1]
    mirror = (Path(args.gen) / "decomp").resolve()
    counts, findings, unresolved, errors, calls = collections.Counter(), [], [], [], []
    definitions, local_definitions, deferred = collections.defaultdict(list), collections.defaultdict(list), []
    with ProcessPoolExecutor(args.jobs, initializer=class_audit_init,
                             initargs=(str(root), str(mirror), str(Path(args.abi).resolve()), args.clang)) as pool:
        for result in pool.map(audit_class_unit, units):
            counts.update(result["counts"])
            findings.extend(result["findings"])
            unresolved.extend(result["unresolved"])
            calls.extend(result["calls"])
            deferred.extend(result["deferred"])
            for name, shapes in result["definitions"].items():
                definitions[name].append(shapes)
            for name, shapes in result["local_definitions"].items():
                local_definitions[(result["unit"], name)].append(shapes)
            if result["error"]:
                errors.append(result["error"])
    sys.path.insert(0, str(root / "tools"))
    import gen_port as gp
    aliases = gp.retail_function_names(root / "decomp", Path(args.gen))
    address_definitions = collections.defaultdict(list)
    for name, choices in list(definitions.items()):
        if name in aliases:
            address_definitions[aliases[name]].extend(choices)
    for name, address in aliases.items():
        if name not in definitions and address in address_definitions:
            definitions[name].extend(address_definitions[address])
    aggregate_counts, aggregate_findings, aggregate_unresolved = finish_aggregate_calls(
        deferred, definitions, local_definitions)
    counts.update(aggregate_counts)
    findings.extend(aggregate_findings)
    unresolved.extend(aggregate_unresolved)
    # Header-inline sites occur in several translation units. Keep their unit-specific
    # observations: different visible declarations are precisely what this audit checks.
    report = {"units": len(units), "source_layout": "i386", "counts": dict(counts), "findings": findings,
              "unresolved": unresolved, "errors": errors, "calls": calls,
              "active_units": [entry.split("|", 1)[0].replace("\\", "/").split("/decomp/", 1)[-1] for entry in units]}
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=1) + "\n")
    print("call classes:", json.dumps({"units": len(units), **dict(counts),
                                      "findings": len(findings), "parse_errors": len(errors)}, sort_keys=True))
    for row in findings:
        print(f"{row['file']}:{row['line']} {row['caller']} -> {row['callee']} [{row['linked']}]: "
              f"{row['kind']} {row['have']} -> {row['want']}")
    for error in errors:
        print(f"INCOMPLETE {error['unit']}: {error['error']}", file=sys.stderr)
    return 2 if errors else int(bool(findings))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gen", required=True, help="build/gen directory holding units.txt")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--json", help="write the findings as JSON")
    ap.add_argument("--arity", help="write the calls whose argument count differs from the definition (markdown)")
    ap.add_argument("--classes", action="store_true", help="audit every preprocessed call against the compiled ABI, including aliases")
    ap.add_argument("--abi", help="merged obj64/abi.json from the same build (required by --classes)")
    ap.add_argument("--clang", default="clang", help="Clang used by --classes")
    a = ap.parse_args()
    units = [l for l in (Path(a.gen) / "units.txt").read_text().splitlines() if l.strip()]
    if a.classes:
        if not a.abi:
            ap.error("--classes requires --abi from the same compiled build")
        return class_report(a, units)
    findings = []
    with ThreadPoolExecutor(a.jobs) as pool:
        for path, err in pool.map(lambda e: run_unit(e, []), units):
            for line in err.splitlines():
                m = DIAG.match(line)
                if not m:
                    continue
                names = NAME.findall(m["msg"])
                findings.append({"flag": m["flag"], "unit": Path(path).as_posix().split("/decomp/", 1)[-1],
                                 "file": m["file"].split("/decomp/", 1)[-1], "line": int(m["line"]), "col": int(m["col"]),
                                 "callee": names[0] if names else "", "msg": m["msg"]})
    by_flag = collections.Counter(f["flag"] for f in findings)
    print("findings by diagnostic:", dict(by_flag))
    calls = collections.defaultdict(set)
    for f in findings:
        if f["flag"] == "-Wdeprecated-non-prototype" and "passing arguments" in f["msg"]:
            calls[f["callee"]].add((f["file"], f["line"]))
    print(f"calls with arguments through an unprototyped declaration: {sum(len(v) for v in calls.values())} "
          f"in {len(calls)} callees")
    for name, sites in sorted(calls.items(), key=lambda kv: -len(kv[1])):
        print(f"  {name}: {len(sites)}")
    if a.json:
        Path(a.json).write_text(json.dumps(findings, indent=1))
    if a.arity:
        arity_report(Path(a.gen), findings, Path(a.arity))


if __name__ == "__main__":
    sys.exit(main())
