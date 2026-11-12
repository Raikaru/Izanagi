"""Bounded host-adapter checks using the selected compiler's existing AST.

Object relocations authorize (source, symbol, caller) pairs. This complementary
check rejects host results entering guest memory and known function targets
crossing incompatible callback prototypes. It neither collects game units nor
requires ABI sidecars, SDK probes, or unresolved whole-program target closure.
"""
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import struct

from port64.gptr_rewrite import Rewriter, Types

TO_GUEST = {"p4_h2g", "p4_fn_h2g", "p4_fn_h2g_sig"}
FROM_GUEST = {"p4_g2h", "p4_g2h_zero", "p4_g2h_slow"}
GUEST_MACROS = {"P4_G2H", "P4_G8", "P4_G16", "P4_G32", "P4_GF32",
                "P4_GPTR", "P4_GDATA", "P4_GARRAY", "P4_GPDATA"}
ADDRESS_MACROS = {"P4_H2G", "P4_FN_H2G"}
BYTE_RESULT = {"memcpy", "memmove", "memset", "strcpy", "strncpy", "strcat", "strncat", "strstr"}
FORMAT_DEST = {"sprintf", "snprintf", "vsprintf", "vsnprintf"}
FORTIFIED_BASE = {
    "__memcpy_chk": "memcpy", "__memmove_chk": "memmove", "__memset_chk": "memset",
    "__strcpy_chk": "strcpy", "__strncpy_chk": "strncpy", "__strcat_chk": "strcat",
    "__strncat_chk": "strncat", "__strlen_chk": "strlen", "__strchr_chk": "strchr",
    "__sprintf_chk": "sprintf", "__snprintf_chk": "snprintf",
    "__vsprintf_chk": "vsprintf", "__vsnprintf_chk": "vsnprintf",
}


def command_arguments(command):
    if os.name != "nt":
        return shlex.split(command)
    import ctypes
    count = ctypes.c_int()
    split = ctypes.windll.shell32.CommandLineToArgvW
    split.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    split.restype = ctypes.POINTER(ctypes.c_wchar_p)
    argv = split(command, ctypes.byref(count))
    if not argv:
        raise ValueError("cannot parse native Windows compiler command")
    try:
        return [argv[i] for i in range(count.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(ctypes.cast(argv, ctypes.c_void_p))


def compiler_command(entry):
    """The real front-end argv, without launchers, codegen outputs or depflags."""
    args = entry.get("arguments") or command_arguments(entry["command"])
    # Ninja compdb may include ccache/the EE IR launcher; the check needs the
    # actual C front end, with the exact same target and include flags.
    for index, argument in enumerate(args):
        if re.fullmatch(r"(?:.*-)?clang(?:-[0-9]+)?(?:\.exe)?", Path(argument).name, re.IGNORECASE):
            args = args[index:]
            break
    command, skip = [], False
    for arg in args:
        if skip:
            skip = False
        elif arg in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
        elif arg not in ("-c", "-MD", "-MMD", "-MP"):
            command.append(arg)
    return command


def compiler_ast(entry, deps=None):
    """Use the profile's real includes, macros and ABI, not a guessed target."""
    command = compiler_command(entry)
    command += ["-Xclang", "-ast-dump=json", "-fsyntax-only"]
    if deps:
        path, target = deps
        command += ["-MD", "-MF", str(path), "-MQ", str(target)]
    result = subprocess.run(command, cwd=entry["directory"], text=True,
                            capture_output=True, check=True)
    return json.loads(result.stdout)


def defined_functions(ast, source):
    """Share actual function ownership, including macro-renamed platform entries."""
    functions, current_file = {}, str(source)
    for node in ast.get("inner", []):
        loc = node.get("loc", {})
        current_file = (loc.get("expansionLoc", {}).get("file")
                        or loc.get("spellingLoc", {}).get("file")
                        or loc.get("file", current_file))
        if node.get("kind") == "FunctionDecl":
            body = next((x for x in node.get("inner", []) if x.get("kind") == "CompoundStmt"), None)
            if body:
                functions[node["name"]] = (node, body, current_file)
                functions[node.get("mangledName", node["name"])] = (node, body, current_file)
    return functions


def check(entry, host_names):
    ast = compiler_ast(entry)
    source = Path(entry["file"]).resolve()
    global_ids = {node["id"] for node in ast.get("inner", []) if node.get("kind") == "VarDecl"}
    # Explicit spelling locations preserve Android's macro-renamed main owner.
    functions = defined_functions(ast, source)
    texts, summaries, globals_, errors = {}, {}, {}, set()
    initializers, current_file = [], str(source)
    for node in ast.get("inner", []):
        loc = node.get("loc", {})
        current_file = (loc.get("expansionLoc", {}).get("file")
                        or loc.get("spellingLoc", {}).get("file") or loc.get("file", current_file))
        if node.get("kind") == "VarDecl" and node.get("init") and Path(current_file).resolve() == source:
            initializers.append(node)
    declarations, typedefs, types, requirements = {}, {}, Types(), {}
    stack = [ast]
    while stack:
        node = stack.pop()
        stack.extend(node.get("inner", ()))
        if node.get("id"):
            declarations[node["id"]] = node
        if node.get("kind") == "TypedefDecl":
            typedefs[node["name"]] = node.get("type", {}).get("qualType", "")
            types.add_typedef(node["name"], re.sub(r"\s*__attribute__\s*\(\(.*?\)\)", "",
                                                  typedefs[node["name"]]))
    command = compiler_command(entry)
    # LLVM's target-prefixed drivers select an ABI without --target flags
    # (the production Win32 toolchain uses i686-w64-mingw32-clang).
    driver = re.fullmatch(r"(.+)-clang(?:-[0-9]+)?(?:\.exe)?",
                          Path(command[0]).name, re.IGNORECASE)
    target = driver[1] if driver else ""
    pointer_mode = None
    arguments = iter(command[1:])
    for argument in arguments:
        if argument in ("-target", "--target"):
            target = next(arguments, "")
        elif argument.startswith(("--target=", "-target=")):
            target = argument.split("=", 1)[1]
        elif argument in ("-m32", "-m64"):
            pointer_mode = int(argument[2:])
    # Clang selects the last target and the last width override independently:
    # inherited -m32 must not defeat a later -m64 (or vice versa).
    bits = pointer_mode or (32 if re.match(r"i[3-6]86-", target) else (
        64 if re.match(r"(?:x86_64|aarch64|arm64)-", target) else struct.calcsize("P") * 8))
    long_bits = 32 if bits == 32 or re.search(r"windows|mingw|msvc", target) or (
        not target and os.name == "nt") else 64
    x86_32 = bits == 32 and (not target or re.match(r"(?:i[3-6]86|x86_64)-", target))

    def qt(node):
        type_ = node.get("type", {})
        return type_.get("desugaredQualType", type_.get("qualType", ""))

    def clean_type(text):
        convention = "cdecl"
        for attribute in re.finditer(r"__attribute__\s*\(\((.*?)\)\)", text):
            prefix = text[:attribute.start()]
            if prefix.count("(") == prefix.count(")"):
                match = re.search(r"\b(stdcall|fastcall|thiscall|vectorcall|ms_abi|sysv_abi)\b",
                                  attribute[1])
                if match:
                    convention = match[1]
        if not x86_32 and convention in ("stdcall", "fastcall", "thiscall"):
            convention = "cdecl"
        return re.sub(r"\s*__attribute__\s*\(\(.*?\)\)", "", text).strip(), convention

    def shape(text):
        text = Types._strip_quals(clean_type(text)[0])
        seen = set()
        while text in typedefs and text not in seen:
            seen.add(text)
            text, _ = clean_type(typedefs[text])
        # Clang builtin signatures can spell size_t without emitting a typedef
        # declaration. The rewriter's fallback maps it to EE unsigned long;
        # native LLP64 instead requires the selected ABI's pointer-sized word.
        if text in ("size_t", "__size_t", "ssize_t", "__ssize_t", "intptr_t", "__intptr_t",
                    "uintptr_t", "__uintptr_t", "ptrdiff_t"):
            return ("G", bits)
        classified = types.classify(text)
        if classified["levels"] or classified["dims"]:
            return ("G", bits)
        base = classified["base"]
        if base in ("void", "float", "double", "long double"):
            return ({"void": "V", "float": "F", "double": "F", "long double": "long double"}[base],
                    {"void": 0, "float": 32, "double": 64, "long double": 0}[base])
        if base.startswith("enum "):
            return ("G", 32)
        plain = re.sub(r"\b(?:signed|unsigned)\b", "", base)
        plain = " ".join(plain.split())
        width = {"": 32, "int": 32, "char": 8, "_Bool": 8, "bool": 8, "short": 16,
                 "short int": 16, "long": long_bits, "long int": long_bits,
                 "long long": 64, "long long int": 64, "__int128": 128}.get(plain)
        return ("G", width) if width is not None else ("type", base)

    def callback(text):
        raw = text
        text, convention = clean_type(text)
        classified = types.classify(text)
        if not classified["fn"] or classified["levels"] != 1 or classified["dims"]:
            return None
        # Typedef spellings may hide the calling convention, but not its
        # selected-compiler canonical desugared function-pointer spelling.
        if raw in typedefs:
            _, convention = clean_type(typedefs[raw])
        result = classified["base"]
        if result in typedefs and classified.get("params") == "":
            function_type, convention = clean_type(typedefs[result])
            params = Rewriter.split_params(function_type)
            result = function_type.split("(", 1)[0].strip()
        else:
            params = Rewriter.split_params("void (" + classified.get("params", "") + ")")
        return {"return": result, "params": params, "cc": convention}

    def actual_signature(declaration):
        text, convention = clean_type(declaration.get("type", {}).get("qualType", ""))
        result = text.split("(", 1)[0].strip()
        if re.search(r"\(\s*\*\s*\(", text):
            result = "void *"  # Function returning a function pointer.
        params = [qt(child) for child in children(declaration) if child.get("kind") == "ParmVarDecl"]
        parsed = Rewriter.split_params(text)
        variadic = bool(declaration.get("variadic")) or bool(parsed and parsed[-1] == "...")
        if not params and parsed:
            params = parsed[:-1] if variadic else parsed
        return {"return": result, "params": params, "variadic": variadic, "cc": convention}

    def callback_targets(tags, expected, fn, context):
        signature = callback(expected)
        if signature is None:
            return
        for tag in tags:
            if tag.startswith("param:"):
                requirements.setdefault(fn, set()).add((int(tag[6:]), expected))
        for tag in tags:
            if not tag.startswith("function:"):
                continue
            declaration = declarations.get(tag[len("function:"):])
            if declaration is None:
                continue
            actual = actual_signature(declaration)
            differences = []
            if signature["cc"] != actual["cc"]:
                differences.append("calling convention")
            if shape(signature["return"]) != shape(actual["return"]):
                differences.append("return width/class")
            parameters = signature["params"]
            if parameters is not None:
                variadic = bool(parameters and parameters[-1] == "...")
                parameters = parameters[:-1] if variadic else parameters
                if variadic != actual["variadic"]:
                    differences.append("variadic prototype")
                if len(parameters) != len(actual["params"]):
                    differences.append("parameter count")
                for index, (wanted, have) in enumerate(zip(parameters, actual["params"])):
                    if shape(wanted) != shape(have):
                        differences.append(f"parameter {index} width/class")
            if differences:
                errors.add(f"{source}: {fn}: indirect callback {declaration['name']} at {context}: "
                           + ", ".join(differences))

    def children(node):
        return node.get("inner", [])

    def macro(node, file):
        loc = node.get("range", {}).get("begin", {}).get("expansionLoc")
        if not loc or "offset" not in loc:
            return ""
        path = loc.get("file", file)
        if path not in texts:
            texts[path] = Path(path).read_bytes().decode("latin-1")
        match = re.match(r"[A-Za-z_]\w*", texts[path][loc["offset"]:])
        return match[0] if match else ""

    def callee(node):
        while node.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and children(node):
            node = children(node)[0]
        return node.get("referencedDecl", {}).get("name", "")

    def key(node):
        while node.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and children(node):
            node = children(node)[0]
        if node.get("kind") == "DeclRefExpr":
            return node.get("referencedDecl", {}).get("id")
        if node.get("kind") == "MemberExpr":
            return "field:" + str(node.get("referencedMemberDecl", node.get("name")))
        if node.get("kind") == "ArraySubscriptExpr" and children(node):
            base = key(children(node)[0])
            return "elements:" + base if base else None
        return None

    def union(nodes, env, fn, file, escapes):
        result = set()
        for node in nodes:
            result |= value(node, env, fn, file, escapes)
        return result

    def expand(tags, args):
        result = set()
        for tag in tags:
            if tag.startswith("addr:"):
                result.update("addr:" + item for item in expand({tag[5:]}, args))
            elif tag.startswith("param:"):
                index = int(tag[6:])
                if index < len(args):
                    result.update(args[index])
            else:
                result.add(tag)
        return result

    def remember(target, tags):
        writes.setdefault(target, set()).update(tags)
        globals_.setdefault(target, set()).update(
            tag for tag in tags if tag == "guest" or tag.startswith(("host:", "function:")) or ":host:" in tag)

    def escape(tags, fn, sink, escapes):
        for tag in tags:
            while tag.startswith("addr:"):
                tag = tag[5:]
            if tag.startswith("host:"):
                errors.add(f"{source}: {fn}: {tag[5:]} result reaches guest-memory boundary {sink}")
            elif tag.startswith("param:"):
                escapes.add(int(tag[6:]))

    def output_result(arg, tags, env):
        while arg.get("kind") in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and children(arg):
            arg = children(arg)[0]
        addressed = arg.get("kind") == "UnaryOperator" and arg.get("opcode") == "&"
        target = key(children(arg)[0]) if addressed and children(arg) else key(arg)
        if target:
            if not addressed:
                target = "elements:" + target
            env[target] = set(tags)
            root = target.removeprefix("elements:")
            if root.startswith("field:") or root in global_ids:
                remember(target, tags)

    def value(node, env, fn, file, escapes):
        kind, inner = node.get("kind"), children(node)
        spelling = macro(node, file)
        if kind == "ImplicitCastExpr" and node.get("castKind") == "ArrayToPointerDecay" and inner:
            base = key(inner[0])
            tags = set(env.get("elements:" + base, globals_.get("elements:" + base, ()))) if base else set()
            tags |= value(inner[0], env, fn, file, escapes)
            result = {"addr:" + tag for tag in tags}
            if guest_lvalue(inner[0], env, fn, file, escapes):
                result.add("guest")
            return result
        if kind == "ArraySubscriptExpr" and inner:
            tags = value(inner[0], env, fn, file, escapes)
            result = {tag[5:] for tag in tags if tag.startswith("addr:")}
            if "guest" in tags:
                result.add("guest")
            target = key(node)
            result.update(env.get(target, globals_.get(target, ())))
            return result | union(inner[1:], env, fn, file, escapes)
        if kind == "DeclRefExpr":
            declaration = node.get("referencedDecl", {})
            if declaration.get("name") in host_names and (
                    declaration.get("kind") == "FunctionDecl" or
                    declaration.get("name") in {"stdin", "stdout", "stderr", "errno"}):
                return ({"host:" + declaration["name"], "function:" + declaration["id"]}
                        if declaration.get("kind") == "FunctionDecl" else {"host:" + declaration["name"]})
            if declaration.get("kind") == "FunctionDecl":
                return {"function:" + declaration["id"]}
        if kind in ("DeclRefExpr", "MemberExpr"):
            name = key(node)
            tags = set(env.get(name, globals_.get(name, ())))
            type_text = qt(node)
            if name and "*" in type_text and not type_text.endswith("]"):
                tags.update("addr:" + tag for tag in env.get("elements:" + name, globals_.get("elements:" + name, ())))
            return tags
        if kind == "CallExpr" and inner:
            name = callee(inner[0])
            targets = value(inner[0], env, fn, file, escapes)
            callback_targets(targets, qt(inner[0]), fn, "call")
            args = [value(arg, env, fn, file, escapes) for arg in inner[1:]]
            for tag in targets:
                declaration = declarations.get(tag.removeprefix("function:")) if tag.startswith("function:") else None
                if declaration is not None:
                    for expected, tags_ in zip(actual_signature(declaration)["params"], args):
                        callback_targets(tags_, expected, fn, "argument")
            base = re.sub(r"^__(?:isoc99|isoc23|mingw|builtin)_", "", name)
            base = FORTIFIED_BASE.get(base, base)
            tags = set().union(*args) if args else set()
            if name in TO_GUEST:
                escape(tags, fn, name, escapes)
                return set()
            if name in FROM_GUEST:
                return {"guest"} | tags
            if base in BYTE_RESULT:
                if base not in ("strstr", "memset") and args and "guest" in args[0] and len(args) > 1:
                    escape({tag[5:] for tag in args[1] if tag.startswith("addr:")}, fn, base, escapes)
                if base == "memset" and args and "guest" in args[0] and len(args) > 1:
                    escape(args[1], fn, name, escapes)
                return args[0] if args else set()
            if base in FORMAT_DEST and args:
                if "guest" in args[0]:
                    errors.add(f"{source}: {fn}: host {base} formats directly into guest memory")
                output_result(inner[1], {"host:" + base}, env)
            if base in ("sscanf", "scanf"):
                for tags_ in args[2 if base == "sscanf" else 1:]:
                    if "guest" in tags_:
                        errors.add(f"{source}: {fn}: host {base} writes directly into a guest destination")
                for arg in inner[3 if base == "sscanf" else 2:]:
                    output_result(arg, {"host:" + base}, env)
            if base.startswith("strto") and len(args) > 1:
                if "guest" in args[1]:
                    errors.add(f"{source}: {fn}: host {base} writes an end pointer into guest memory")
                output_result(inner[2], {"host:" + base + ":end"}, env)
            if name in functions and name not in TO_GUEST | FROM_GUEST:
                returned, sinks, effects = summaries.get(name, (set(), set(), {}))
                for index, expected in tuple(requirements.get(name, ())):
                    if index < len(args):
                        callback_targets(args[index], expected, fn, "argument via " + name)
                for index in sinks:
                    if index < len(args):
                        escape(args[index], fn, name, escapes)
                for target, effect in effects.items():
                    remember(target, expand(effect, args))
                return expand(returned, args)
            if name in {"__errno_location", "__errno", "_errno"}:
                return {"addr:host:errno"}
            if name in host_names or base in host_names:
                return {"host:" + base}
            return set()
        if kind == "UnaryOperator" and inner:
            tags = value(inner[0], env, fn, file, escapes)
            if node.get("opcode") == "&":
                operand = inner[0]
                while operand.get("kind") == "ParenExpr" and children(operand):
                    operand = children(operand)[0]
                if operand.get("kind") == "DeclRefExpr" and operand.get("referencedDecl", {}).get("kind") == "FunctionDecl":
                    return tags
                if operand.get("kind") == "ArraySubscriptExpr":
                    return union(children(operand), env, fn, file, escapes)
                if operand.get("kind") == "UnaryOperator" and operand.get("opcode") == "*":
                    return union(children(operand), env, fn, file, escapes)
                result = {"addr:" + tag for tag in tags}
                if operand.get("kind") == "MemberExpr":
                    result |= union(children(operand), env, fn, file, escapes)
                if guest_lvalue(operand, env, fn, file, escapes):
                    result.add("guest")
                return result
            if node.get("opcode") == "*":
                return ({tag[5:] for tag in tags if tag.startswith("addr:")}
                        | {tag for tag in tags if tag.startswith("function:") and callback(qt(inner[0]))})
            return tags
        if kind in ("BinaryOperator", "CompoundAssignOperator") and len(inner) == 2 and node.get("opcode") in ("=", "+=", "-=", "|=", "&="):
            tags = value(inner[1], env, fn, file, escapes)
            lhs = inner[0]
            lhs_tags = value(lhs, env, fn, file, escapes)
            callback_targets(tags, qt(lhs), fn, "store")
            if "guest" in lhs_tags or guest_lvalue(lhs, env, fn, file, escapes):
                escape(tags, fn, "guest store", escapes)
                escape(lhs_tags, fn, "guest store address", escapes)
            target = key(lhs)
            if target:
                env[target] = tags
                if target.startswith("field:") or target.removeprefix("elements:") in global_ids:
                    remember(target, tags)
            return tags
        if kind == "BinaryOperator" and node.get("opcode") == "," and len(inner) == 2:
            value(inner[0], env, fn, file, escapes)
            return value(inner[1], env, fn, file, escapes)
        tags = union(inner, env, fn, file, escapes)
        if kind in ("ImplicitCastExpr", "CStyleCastExpr", "CXXReinterpretCastExpr", "CXXStaticCastExpr"):
            if node.get("castKind") != "FunctionToPointerDecay":
                callback_targets(tags, qt(node), fn, "cast")
        if kind in ("InitListExpr", "CompoundLiteralExpr") and callback(qt(node)) is None:
            # An aggregate is not one union-valued code pointer. Opaque raw
            # dispatch loads remain outside this bounded known-target check.
            tags = {tag for tag in tags if not tag.startswith("function:")}
        if spelling in ADDRESS_MACROS:
            escape(tags, fn, spelling, escapes)
            return set()
        if spelling in GUEST_MACROS:
            return {"guest"} | tags
        return tags

    def guest_lvalue(node, env, fn, file, escapes):
        if macro(node, file) in GUEST_MACROS:
            return True
        if node.get("kind") == "UnaryOperator" and node.get("opcode") == "*":
            return bool(children(node) and "guest" in value(children(node)[0], env, fn, file, escapes))
        if node.get("kind") in ("MemberExpr", "ArraySubscriptExpr"):
            return bool(children(node) and "guest" in value(children(node)[0], env, fn, file, escapes))
        if node.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr") and children(node):
            return guest_lvalue(children(node)[0], env, fn, file, escapes)
        return False

    def statement(node, env, fn, file, returned, escapes):
        kind = node.get("kind")
        if kind == "VarDecl":
            env[node["id"]] = union(children(node), env, fn, file, escapes)
            callback_targets(env[node["id"]], qt(node), fn, "initializer")
        elif kind == "ReturnStmt":
            tags = union(children(node), env, fn, file, escapes)
            returned.update(tags)
            declaration = functions.get(fn, (None,))[0]
            if declaration is not None:
                callback_targets(tags,
                                 actual_signature(declaration)["return"], fn, "return")
        elif kind == "IfStmt":
            parts = children(node)
            value(parts[0], env, fn, file, escapes)
            branches = []
            for child in parts[1:]:
                branch = {key: set(tags) for key, tags in env.items()}
                statement(child, branch, fn, file, returned, escapes)
                branches.append(branch)
            if len(parts) == 2:
                branches.append(env)
            joined = {}
            for branch in branches:
                for key_, tags in branch.items():
                    joined.setdefault(key_, set()).update(tags)
            env.clear()
            env.update(joined)
        elif kind in ("CompoundStmt", "DeclStmt", "ForStmt", "WhileStmt", "DoStmt", "SwitchStmt", "CaseStmt", "DefaultStmt", "LabelStmt"):
            for child in children(node):
                statement(child, env, fn, file, returned, escapes)
        else:
            value(node, env, fn, file, escapes)

    # Analyze source functions and the inline helpers they actually call, not
    # every unused body in a system or unrelated project header.
    pending = [name for name, (_, _, file) in functions.items() if Path(file).resolve() == source]
    if not pending:
        raise ValueError(f"{source}: no source function bodies in compiler AST")
    reachable = set()
    while pending:
        name = pending.pop()
        if name in reachable or name not in functions:
            continue
        reachable.add(name)
        stack = [functions[name][1]]
        while stack:
            node = stack.pop()
            stack.extend(children(node))
            if node.get("kind") == "CallExpr" and children(node):
                pending.append(callee(children(node)[0]))
    functions = {name: info for name, info in functions.items() if name in reachable}

    def snapshot():
        return ({name: (frozenset(ret), frozenset(sinks),
                        {key: frozenset(tags) for key, tags in effects.items()})
                 for name, (ret, sinks, effects) in summaries.items()},
                {key: frozenset(tags) for key, tags in globals_.items()},
                {key: frozenset(contracts) for key, contracts in requirements.items()})

    # Parameterized summaries include project/inline helpers and native caches.
    while True:
        previous = snapshot()
        writes = {}
        for node in initializers:
            tags = union(children(node), globals_, "<global>", str(source), set())
            callback_targets(tags, qt(node), "<global>", "initializer")
            globals_.setdefault(node["id"], set()).update(tags)
        for name, (decl, body, file) in functions.items():
            if name in TO_GUEST | FROM_GUEST:
                continue
            params = [x for x in children(decl) if x.get("kind") == "ParmVarDecl"]
            env = {param["id"]: {f"param:{index}"} for index, param in enumerate(params)}
            returned, escapes, writes = set(), set(), {}
            statement(body, env, name, file, returned, escapes)
            summaries[name] = (returned, escapes, writes)
        if snapshot() == previous:
            break
    return sorted(errors)
