#!/usr/bin/env python3
"""Compiler launcher for the decomp mirror: the EE's add, subtract, divide and square root (docs/porting.md, "EE float mode").

    ee_float_cc.py [--modes=div,sqrt,add] <clang> <args...> -c file.c -o file.o

The game's float code runs with MXCSR round-toward-zero (include/p4pc/hostmath.h), which is right for multiplication but not for the other four operations: the
golden traces (retail code run in PCSX2, tools/mipsdiff/replay_notes.md) show add.s/sub.s as a guard-bit truncating adder and div.s/sqrt.s rounded to nearest, with
x / 0 and sqrt of a negative number defined. The launcher compiles the unit to unoptimised LLVM IR (the type information a source-level rewrite would need), rewrites
exactly those operations, and compiles the IR to the object file with the same flags:

  fadd / fsub float   -> call p4pc_ee_add / p4pc_ee_sub        (include/p4pc/eefloat.h, src/platform/ee_float.c)
  fdiv float          -> call p4pc_ee_div
  sqrtf(x), llvm.sqrt.f32 -> call p4pc_ee_sqrt

The helpers are pure (readnone) so the optimiser can still schedule and combine them. madd.s / msub.s were measured as fused single truncations too and match fewer
golden calls than a plain multiply followed by the add (six functions lose 35 cases, one gains 1), so they are not rewritten beyond their add.
P4_EE_FLOAT_REPORT=<file> appends one "unit function div sqrt add" line per function with something rewritten (tools/ee_float_sites.py compares that with the retail
div.s/sqrt.s counts).
"""
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import build_cache

# options of the original command that the IR compile does not need (they only matter to the front end)
DROP_WITH_ARG = {"-I", "-include", "-isystem", "-MF", "-MT", "-MQ", "-o", "-Xclang", "-iquote", "-idirafter", "-imacros"}
DROP_PREFIX = ("-I", "-D", "-std=", "-MD", "-MMD", "-isystem", "-include", "-iquote")

NAME = r"%[\w.$-]+"
FLAGS = r"(?:(?:fast|nnan|ninf|nsz|arcp|contract|afn|reassoc) )*"
TAIL = r"((?:, !\w+ !\d+)*)\s*$"   # trailing metadata attachments (!dbg with -g)
RE_FDIV = re.compile(rf"^(\s*)({NAME}) = fdiv {FLAGS}float ([^\s,]+), ([^\s,]+){TAIL}")
RE_FADD = re.compile(rf"^(\s*)({NAME}) = fadd {FLAGS}float ([^\s,]+), ([^\s,]+){TAIL}")
RE_FSUB = re.compile(rf"^(\s*)({NAME}) = fsub {FLAGS}float ([^\s,]+), ([^\s,]+){TAIL}")
RE_SQRT = re.compile(rf"^(\s*)({NAME}) = (?:tail )?call (?:[\w()]+ )*float @(?:llvm\.sqrt\.f32|sqrtf)\(float (?:[a-z]+(?:\([^)]*\))? )*([^\s,)]+)\)(?: #\d+)?{TAIL}")
RE_DEFINE = re.compile(r"^define .* @(\"[^\"]+\"|[\w.$-]+)\(")

DECLS = {
    "add": "declare float @p4pc_ee_add(float, float) nounwind readnone willreturn\ndeclare float @p4pc_ee_sub(float, float) nounwind readnone willreturn\n",
    "div": "declare float @p4pc_ee_div(float, float) nounwind readnone willreturn\n",
    "sqrt": "declare float @p4pc_ee_sqrt(float) nounwind readnone willreturn\n",
}


class Fn:
    def __init__(self):
        self.n = 0
        self.div = self.sqrt = self.add = 0

    def tmp(self):
        self.n += 1
        return f"%ee{self.n}"


def rewrite_function(lines, modes):
    """lines: the body of one function (after `define`, up to and including `}`)."""
    st = Fn()
    do_div, do_sqrt, do_add = "div" in modes, "sqrt" in modes, "add" in modes
    out = []
    for line in lines:
        if do_add:
            hit = False
            for rx, fn in ((RE_FADD, "p4pc_ee_add"), (RE_FSUB, "p4pc_ee_sub")):
                m = rx.match(line)
                if m:
                    ind, res, a, b, tail = m.groups()
                    out.append(f"{ind}{res} = call float @{fn}(float {a}, float {b}){tail}\n")
                    st.add += 1
                    hit = True
                    break
            if hit:
                continue
        m = RE_FDIV.match(line) if do_div else None
        if m:
            ind, res, a, b, tail = m.groups()
            out.append(f"{ind}{res} = call float @p4pc_ee_div(float {a}, float {b}){tail}\n")
            st.div += 1
            continue
        m = RE_SQRT.match(line) if do_sqrt else None
        if m:
            ind, res, a, tail = m.groups()
            out.append(f"{ind}{res} = call float @p4pc_ee_sqrt(float {a}){tail}\n")
            st.sqrt += 1
            continue
        out.append(line)
    return out, st


def rewrite_ir(text, modes, report):
    lines = text.splitlines(keepends=True)
    out = []
    i = 0
    used = set()
    while i < len(lines):
        line = lines[i]
        if line.startswith("define "):
            j = i + 1
            while not lines[j].startswith("}"):
                j += 1
            body, st = rewrite_function(lines[i + 1:j + 1], modes)
            out.append(line)
            out += body
            for key, n in (("add", st.add), ("div", st.div), ("sqrt", st.sqrt)):
                if n:
                    used.add(key)
            if report is not None and (st.div or st.sqrt or st.add):
                m = RE_DEFINE.match(line)
                report.append(f"{m.group(1).strip(chr(34)) if m else '?'} {st.div} {st.sqrt} {st.add}\n")
            i = j + 1
            continue
        out.append(line)
        i += 1
    for key in sorted(used):
        for decl in DECLS[key].splitlines(keepends=True):
            name = re.search(r"@(\w+)\(", decl).group(1)
            if not re.search(rf"^declare .*@{name}\(", text, re.M):      # the unit's own C may declare it (eefloat.h), clang already emitted that
                out.append(decl)
    return "".join(out)


# the source: C, or (tools/port64/unit64.py) the rewritten unit as preprocessed C
SRC_EXT = (".c", ".i")


def split_command(cmd):
    """-> (compiler, args, source, output)"""
    src = out = None
    args = []
    i = 1
    while i < len(cmd):
        a = cmd[i]
        if a == "-c":
            i += 1
            continue
        if a == "-o":
            out = cmd[i + 1]
            i += 2
            continue
        if not a.startswith("-") and a.endswith(SRC_EXT):
            src = a
            i += 1
            continue
        args.append(a)
        if a in DROP_WITH_ARG and i + 1 < len(cmd):
            args.append(cmd[i + 1])
            i += 1
        i += 1
    return cmd[0], args, src, out


def ir_flags(args):
    keep = []
    i = 0
    while i < len(args):
        a = args[i]
        if a in DROP_WITH_ARG:
            i += 2
            continue
        if a.startswith(DROP_PREFIX) or a.startswith('"-D'):
            i += 1
            continue
        keep.append(a)
        i += 1
    return keep


def prepare_printf(compiler, args, src, out, root, symbol_prefix):
    """Normalize EE GP varargs with the real compile flags and original depfile."""
    from port64 import printf_rewrite
    directory = os.path.dirname(os.path.abspath(out))
    with tempfile.TemporaryDirectory(prefix="p4printf-", dir=directory) as work:
        if build_cache.enabled():
            ipath = os.path.join(work, os.path.basename(src) + ".printf.i")
            printf_rewrite.gr.preprocess(src, args, ipath, compiler)
            def produce():
                _, rewritten = printf_rewrite.rewrite_preprocessed(ipath, args, root, compiler, symbol_prefix)
                return rewritten.encode("latin-1")
            text = build_cache.run_bytes("printf", {
                "source": os.path.abspath(src), "preprocessed": build_cache.file_digest(ipath),
                "flags": args, "root": os.path.abspath(root), "symbol_prefix": symbol_prefix,
                "compiler": build_cache.compiler_identity(compiler),
                "versions": build_cache.versions(__file__, printf_rewrite.__file__, printf_rewrite.gr.__file__)},
                produce).decode("latin-1")
        else:
            _, text = printf_rewrite.process(src, args, work, root, clang=compiler, symbol_prefix=symbol_prefix)
    normalized = os.path.abspath(out) + ".printf.i"
    with open(normalized, "w", encoding="latin-1", newline="") as output:
        output.write(text)
    # The preprocessor already emitted the original source/header dependency
    # file. Compiling the .i must not replace it with temporary-file dependencies.
    front_flags = ir_flags(args)
    # Unlike the IR backend, this remains a C frontend: retain its language
    # standard and explicit frontend options, in their original relative order.
    for i, argument in enumerate(args):
        if argument.startswith("-std="):
            front_flags.append(argument)
        elif argument == "-Xclang":
            front_flags.extend((argument, args[i + 1]))
    return normalized, front_flags


def profiled_call(command, phase, source):
    with build_cache.timed(phase, source=source) as trace:
        result = subprocess.call(command)
        trace["returncode"] = result
    return result


def main():
    cmd = sys.argv[1:]
    modes_arg = printf_root = None
    printf_prefix = ""
    while cmd and cmd[0].startswith(("--modes=", "--printf-root=", "--printf-symbol-prefix=")):
        option = cmd.pop(0)
        if option.startswith("--modes="):
            modes_arg = option[len("--modes="):]
        elif option.startswith("--printf-root="):
            printf_root = option[len("--printf-root="):]
        else:
            printf_prefix = option[len("--printf-symbol-prefix="):]
    if "-c" not in cmd or not any(a.endswith(SRC_EXT) for a in cmd):
        sys.exit(subprocess.call(cmd))
    mode_string = modes_arg or os.environ.get("EE_FLOAT_MODE") or "div,sqrt,add"
    modes = set() if mode_string == "none" else set(mode_string.split(","))
    compiler, args, src, out = split_command(cmd)
    if src is None or out is None:
        sys.exit(subprocess.call(cmd))
    original_src = src
    normalized = ll = None
    try:
        if printf_root:
            with build_cache.timed("printf.rewrite", source=src):
                normalized, args = prepare_printf(compiler, args, src, out, printf_root, printf_prefix)
            src = normalized
        cache = ([shutil.which("ccache")] if shutil.which("ccache") and not os.environ.get("CCACHE_DISABLE") else [])
        if not modes:
            sys.exit(profiled_call(cache + [compiler] + args + [src, "-c", "-o", out], "compile.direct", original_src))
        fd, ll = tempfile.mkstemp(suffix=".ll", dir=os.path.dirname(os.path.abspath(out)) or None)
        os.close(fd)
        # The front end is the expensive step: cache it when present. The stable
        # normalized filename retains cache hits across repeated object builds.
        front = cache + [compiler] + args + ["-S", "-emit-llvm", "-Xclang", "-disable-llvm-passes", src, "-o", ll]
        rc = profiled_call(front, "compile.frontend", original_src)
        if rc != 0:
            sys.exit(rc)
        with open(ll, "r", encoding="utf-8", errors="surrogateescape", newline="") as f:
            text = f.read()
        report = [] if os.environ.get("P4_EE_FLOAT_REPORT") else None
        with build_cache.timed("float.lower-ir", source=original_src):
            text = rewrite_ir(text, modes, report)
        if report:
            with open(os.environ["P4_EE_FLOAT_REPORT"], "a", encoding="utf-8") as f:
                f.writelines(f"{os.path.basename(original_src)} {r}" for r in report)
        with open(ll, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
            f.write(text)
        back = [compiler] + ir_flags(args) + ["-Wno-override-module", "-x", "ir", ll, "-c", "-o", out]
        sys.exit(profiled_call(back, "compile.backend", original_src))
    finally:
        for temporary in (ll, normalized):
            if temporary:
                try:
                    os.unlink(temporary)
                except OSError:
                    pass


if __name__ == "__main__":
    main()
