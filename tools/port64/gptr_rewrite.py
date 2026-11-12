#!/usr/bin/env python3
"""gptr_rewrite: retype a mirrored decomp unit for a 64-bit host (docs/port64.md, "Mechanism").

The decomp is C for a 32-bit machine: a struct's pointer fields are 4 bytes, the retail image holds
4-byte pointers, and m2c code keeps guest addresses in s32/u32 and casts them to pointers. On a 64-bit
host the same C is rewritten from clang's own AST (`clang -Xclang -ast-dump=json`, so the rewriter sees
every implicit conversion) so that:

  * storage that holds a pointer in guest memory (struct and union fields, globals, arrays, address-taken
    locals) is a 4-byte guest address (`u32`); a pointer-to-pointer value is a host `u32 *` to such a slot;
  * a load of a pointer from guest storage translates to a host pointer (`p4_g2h`), a store translates back
    (`p4_h2g`), and so does every int<->pointer conversion (cast or implicit): integers that stand for
    pointers are guest addresses by construction;
  * function pointers in guest storage are guest code addresses (`p4_fn_g2h` / `p4_fn_h2g`);
  * retail-addressed data (D_xxxxxxxx, iGpxxxxxxxx, symbols with a retail address) is reached as
    `(*(T *)p4_g2h(addr))` instead of through a linker-defined absolute symbol;
  * every record that holds a pointer gets `_Static_assert`s of the i386 layout (size and field offsets).

Registers (locals, parameters, returns) keep host pointers, so pointer arithmetic, comparisons, member access
and calls need no rewriting. See include/p4pc/guest.h for the runtime.

  gptr_rewrite.py --root <mirror root> --out <out root> [--census FILE] [--layouts layouts.json] \\
                  [--symbols decomp-root] unit.c -- <clang flags>

--census only counts rule applications and unsupported constructs (no output files are written)."""
import argparse, bisect, collections, json, os, re, subprocess, sys, threading
import tempfile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import build_cache

sys.setrecursionlimit(1000000)

KEYWORDS_STORAGE = ("static", "extern", "register", "typedef", "inline")


# ------------------------------------------------------------------------------------------ AST loading
_MAIN = [None]


def stage2_flags(flags):
    """the flags that still matter once a unit is preprocessed: target, language and codegen-affecting options"""
    out, skip = [], False
    for f in flags:
        if skip:
            skip = False
        elif f in ("-include", "-isystem", "-I", "-D", "-U", "-MF", "-MT", "-MQ", "-o"):
            skip = True
        elif f.startswith(("-I", "-D", "-U", "-include", "-MD", "-MMD", "-MF", "-MT", "-W", "-ferror-limit")) or f in ("-c", "-E", "-S"):
            continue
        else:
            out.append(f)
    return out


def preprocess(unit, flags, out_i, clang="clang", deps=None):
    """`clang -E` with the unit's own flags: macros expanded, includes inlined, line markers kept. Rewriting this text
    instead of the source removes the macro problem (every node is plain text with one meaning)."""
    dep = ["-MD", "-MF", deps[0], "-MT", deps[1]] if deps else []
    with build_cache.timed("clang.preprocess", source=os.path.abspath(unit)) as trace:
        p = subprocess.run([clang, "-E", "-w"] + dep + flags + [unit, "-o", out_i], capture_output=True)
        trace["returncode"] = p.returncode
    if p.returncode:
        raise SystemExit(f"preprocessing {unit} failed:\n{p.stderr.decode(errors='replace')[:2000]}")


def load_ast(unit, flags, clang="clang"):
    lang = ["-x", "cpp-output"] if unit.endswith(".i") else []
    parsed = []
    def produce():
        with build_cache.timed("clang.ast", source=os.path.abspath(unit)) as trace:
            p = subprocess.run([clang, "-fsyntax-only", "-Wno-everything", "-Xclang", "-ast-dump=json"] + flags + lang + [unit],
                               capture_output=True)
            trace["returncode"] = p.returncode
        if not p.stdout:
            raise SystemExit(f"clang produced no AST for {unit}:\n{p.stderr.decode(errors='replace')[:2000]}")
        with build_cache.timed("ast.decode"):
            parsed.append(json.loads(p.stdout))
        return p.stdout
    def inputs():
        if unit.endswith(".i"):
            checksum = build_cache.file_digest(unit)
        else:
            # A direct C AST (notably host ABI collection) reads headers too.
            with tempfile.TemporaryDirectory(prefix="p4-ast-key-") as work:
                key_source = os.path.join(work, "unit.i")
                preprocess(unit, flags, key_source, clang)
                checksum = build_cache.file_digest(key_source)
        return {"source": os.path.abspath(unit), "preprocessed": checksum,
                "flags": flags + lang, "compiler": build_cache.compiler_identity(clang),
                "versions": build_cache.versions(__file__)}
    data = build_cache.run_bytes("ast", inputs, produce)
    if parsed:
        ast = parsed[0]
    else:
        with build_cache.timed("ast.decode"):
            ast = json.loads(data)
    _MAIN[0] = os.path.abspath(unit)       # a preprocessed unit is one buffer: a location without a file is in it
    return ast




def span(node):
    """(file, begin, end) of a node's source range. The input is preprocessed text (see preprocess()), so there are no
    macro locations: every range is plain text of the one file."""
    r = node.get("range")
    if not r:
        return None
    b, e = r["begin"], r["end"]
    if "offset" not in b or "offset" not in e or e["offset"] < b["offset"]:
        return None
    return b.get("file", _MAIN[0]), b["offset"], e["offset"] + e["tokLen"]


def name_span(node):
    l = node.get("loc", {})
    if "offset" not in l:
        return None
    return l.get("file", _MAIN[0]), l["offset"], l["offset"] + l["tokLen"]


# ------------------------------------------------------------------------------------------ type model
# Printf-family calls retain guest-address/soft-double masks even after the
# libc rename binds their spelling to a retail entry. Values name the shared
# native formatter entry, not a second host-CRT implementation.
PRINTF_FAMILY = {
    "sprintf": "sprintf", "snprintf": "snprintf",
    "printf": "printf", "fprintf": "fprintf",
    "func_00442088": "sprintf", "func_00440b68": "printf",
}

def letter_class(ch):
    """the register class of a signature letter: p (a function pointer `f` too), x (float/double), i"""
    return "p" if ch in "pf" else "x" if ch == "x" else "i"


class Types:
    """Pointer-level classification of clang's printed types, through typedefs."""

    def __init__(self):
        self.typedefs = {}
        self.memo = {}

    def add_typedef(self, name, qt):
        self.typedefs.setdefault(name, qt)

    @staticmethod
    def _strip_quals(s):
        return re.sub(r"\b(const|volatile|restrict|__restrict|_Nonnull|_Nullable|_Null_unspecified)\b", "", s).strip()

    def classify(self, qt, depth=0):
        if depth == 0:
            c = self.memo.get(qt)
            if c is None:
                c = self.memo[qt] = self._classify(qt, 0)
            return c
        return self._classify(qt, depth)

    def _classify(self, qt, depth=0):
        """-> dict(levels=pointer indirections (function pointers count as one), fn=bool, dims=str, ok=bool,
        base=spelled base type)"""
        s = self._strip_quals(re.sub(r"\((?:unnamed|anonymous)[^)]*\)", "ANON", qt))
        s = re.sub(r"\(([\s*]+)\)", lambda m_: "(" + re.sub(r"\s", "", m_.group(1)) + ")", s)      # `(* *)` after const is dropped
        s = re.sub(r"\(\*\((\**)\)\)", lambda m_: "(*" + m_.group(1) + ")", s)         # `(*(*))` is `(**)`
        m = re.match(r"^(.*?)\s*\((\*+)\)$", s)               # `void *(**)`: a parenthesised declarator, no parameters
        if m and "(" not in m.group(1):
            s = m.group(1).strip() + " " + m.group(2)
        dims = ""
        m = re.match(r"^(.*?)\s*\((\*+)\)\s*((?:\[[^\]]*\])+)$", s)
        if m:                                                   # pointer to array: `f32 (*)[16]`
            return dict(levels=len(m.group(2)), fn=False, parr=True, dims="", ok=True, base=m.group(1).strip(),
                        elem_ptr=m.group(1).strip().endswith("*") or (depth < 8 and self.classify(m.group(1).strip(), depth + 1)["levels"] >= 1),
                        adims=m.group(3))
        m = re.match(r"^(.*?)\s*\(\*\((\*+)\)\s*((?:\[[^\]]*\])+)\)\s*\((.*)\)$", s)
        if m:                                                   # pointer to an array of function pointers: `R (*(*)[N])(P)`
            return dict(levels=len(m.group(2)), fn=False, parr=True, dims="", ok=True, base=f"{m.group(1).strip()} (*)({m.group(4)})",
                        elem_ptr=True, adims=m.group(3))
        m = re.match(r"^(.*?)\s*((?:\[[^\]]*\]\s*)+)$", s)
        if m and not re.search(r"\(\*+\s*$", m.group(1)) and not m.group(1).rstrip().endswith(")"):
            s, dims = m.group(1).strip(), m.group(2).strip()
        m = re.match(r"^(.*?)\s*\((\**)\s*((?:\[[^\]]*\])*)\)\s*\((.*)\)$", s)
        if m and m.group(2):
            return dict(levels=len(m.group(2)), fn=True, dims=(m.group(3) or dims), ok=True, base=m.group(1).strip(),
                        params=m.group(4))
        if re.search(r"\(\*+\)", s) or "(" in s:
            return dict(levels=0, fn=False, dims=dims, ok=False, base=s)       # pointer to array and similar
        stars = 0
        while s.endswith("*"):
            stars += 1
            s = self._strip_quals(s[:-1])
        base = s
        if base in PTRSIZE_TYPEDEFS:
            return dict(levels=stars, fn=False, dims=dims, ok=True, base=PTRSIZE_TYPEDEFS[base])
        if depth < 8 and re.fullmatch(r"[A-Za-z_]\w*", base) and base in self.typedefs:
            und = self.typedefs[base]
            if re.fullmatch(r"[^()]*\(.*\)", und.strip()) and not re.search(r"\(\s*\*", und):
                # a function type (`typedef R name(P)`): a pointer to it is a function pointer
                return dict(levels=stars, fn=bool(stars), dims=dims, ok=True, base=base, params="")
            inner = self.classify(und, depth + 1)
            if inner["dims"]:
                if stars == 1 and inner.get("fn") and inner["levels"] == 1 and not dims:
                    # a pointer to a typedef'd array of function pointers (`typedef R (*Slot[N])(P); Slot *p`): the pointer to that array, as spelled out
                    return dict(levels=1, fn=False, parr=True, dims="", ok=True, base=f"{inner['base']} (*)({inner.get('params', '')})", elem_ptr=True,
                                adims=inner["dims"])
                return dict(levels=stars, fn=False, dims=dims, ok=False, base=base)
            inner["levels"] += stars
            inner["dims"] = dims
            return inner
        return dict(levels=stars, fn=False, dims=dims, ok=True, base=base)


# ------------------------------------------------------------------------------------------ edit buffer
class Edits:
    """Text edits of one file. An edit is (pos, group, depth, seq, kind, text, end): at one position closes come first
    (innermost first), then opens (outermost first), then replacements."""

    def __init__(self):
        self.by_file = collections.defaultdict(list)
        self.src_for_extract = {}
        self.seq = 0

    def wrap(self, f, b, e, open_, close, depth):
        self.seq += 1
        self.by_file[f].append((b, 1, depth, self.seq, "open", open_, None))
        self.by_file[f].append((e, 0, -depth, self.seq, "close", close, None))

    def insert(self, f, pos, text):
        self.seq += 1
        self.by_file[f].append((pos, 2, 0, self.seq, "open", text, None))

    def insert_before(self, f, pos, text):
        """text at pos ahead of every other edit that starts there (a replacement of the text that follows it included)"""
        self.seq += 1
        self.by_file[f].append((pos, 1, -(1 << 22), self.seq, "open", text, None))

    def replace(self, f, b, e, text):
        self.seq += 1
        self.by_file[f].append((b, 1, 1 << 20, self.seq, "rep", text, e))

    def extract(self, f, b, e):
        """the text src[b:e] with the edits inside it applied; those edits are removed (the text moves elsewhere)"""
        inside, rest = [], []
        for ev in self.by_file[f]:
            pos, grp, _d, _s, kind, _a, end = ev
            if kind == "rep" and b <= pos and end <= e:
                inside.append(ev)
            elif kind == "open" and grp != 2 and b <= pos < e:
                inside.append(ev)
            elif kind == "close" and b < pos <= e:
                inside.append(ev)
            else:
                rest.append(ev)
        self.by_file[f] = rest
        sub = Edits()
        sub.by_file["x"] = [(ev[0] - b, ev[1], ev[2], ev[3], ev[4], ev[5], None if ev[6] is None else ev[6] - b) for ev in inside]
        return sub.apply("x", self.src_for_extract[f][b:e])

    def apply(self, f, src):
        ev = sorted(self.by_file[f], key=lambda t: (t[0], t[1], t[2], -t[3] if t[1] == 0 else t[3]))
        out, cur, skip_to = [], 0, 0
        for pos, _g, _d, _s, kind, a, e in ev:
            if pos < skip_to:
                raise SystemExit(f"{f}: edit at {pos} inside a replaced span (until {skip_to}): {a!r}")
            out.append(src[cur:pos])
            cur = pos
            out.append(a)
            if kind == "rep":
                cur = skip_to = e
        out.append(src[cur:])
        return "".join(out)


class Census(dict):
    """counts per rule, plus the set of source sites (file relative to the mirror root, offset) each rule hit,
    so a header seen from many units is counted once when the per-unit censuses are merged"""

    def __init__(self, owner):
        super().__init__()
        self.owner = owner
        self.sites = collections.defaultdict(set)

    def __missing__(self, k):
        return 0

    def __setitem__(self, k, v):
        super().__setitem__(k, v)
        s = self.owner.site()
        if s:
            self.sites[k].add(s)


# ------------------------------------------------------------------------------------------ the rewriter
ABS_RE = re.compile(r"^(?:D|DAT|PTR_DAT|jtbl)_([0-9A-Fa-f]{8})(?:_abs)?$")
GP_RE = re.compile(r"^(?:[if]Gp|gp0x)([0-9A-Fa-f]{8})$")
GP_VALUE = 0x007690F0


def ret_text(fn_qt):
    """the return type of a function type `RET (PARAMS)`, or None when it cannot be told apart from a function-pointer return"""
    s = fn_qt.strip()
    if not s.endswith(")"):
        return None
    m = re.match(r"^([^()]*?)\s*\((\*+)\((.*)\)\)\s*\(([^()]*)\)$", s)     # `R (**(PARAMS))(ARGS)`: returns R (**)(ARGS)
    if m and len(m[2]) >= 2:      # a function returning a function pointer stays unclassified: it needs p4_fn_*, not p4_h2g
        return f"{m[1]} ({m[2]})({m[4]})"
    depth = 0
    for i in range(len(s) - 1, -1, -1):
        if s[i] == ")":
            depth += 1
        elif s[i] == "(":
            depth -= 1
            if depth == 0:
                r = s[:i].strip()
                return r if "(" not in r else None
    return None


WORD_BASES = {"unsigned long": "u32", "long": "s32", "long unsigned int": "u32", "long int": "s32"}
# the pointer-sized typedefs are 32-bit words in the i386 original whatever the host makes of them (`unsigned long long` for size_t on Windows x64)
PTRSIZE_TYPEDEFS = {"size_t": "unsigned long", "__size_t": "unsigned long", "uintptr_t": "unsigned long", "__uintptr_t": "unsigned long",
                    "ssize_t": "long", "__ssize_t": "long", "ptrdiff_t": "long", "intptr_t": "long", "__intptr_t": "long"}


def leading_dims(s):
    """the leading `[..][..]` groups of a declarator suffix (brackets and parentheses inside a dimension may nest)"""
    i = 0
    while True:
        j = i
        while j < len(s) and s[j] in " \t\n":
            j += 1
        if j >= len(s) or s[j] != "[":
            return s[:i]
        depth = 0
        while j < len(s):
            if s[j] in "[(":
                depth += 1
            elif s[j] in "])":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        i = j + 1


class Origin:
    """Where each offset of a preprocessed file came from, read from its line markers (`# 12 "file.h" 1`)."""

    def __init__(self, text):
        self.text = text
        self.starts, self.files, self.lines = [0], [None], [1]
        for m in re.finditer(r'^# (\d+) "((?:[^"\\\n]|\\.)*)"[^\n]*\n', text, re.M):
            self.starts.append(m.end())
            self.files.append(os.path.abspath(m.group(2)) if m.group(2)[:1] not in "<" else m.group(2))
            self.lines.append(int(m.group(1)))

    def at(self, off):
        """(file, line, column) in the original source"""
        i = bisect.bisect_right(self.starts, off) - 1
        line = self.lines[i] + self.text.count("\n", self.starts[i], off)
        col = off - (self.text.rfind("\n", 0, off) + 1) + 1
        return self.files[i], line, col


class Rewriter:
    def __init__(self, ast, root, syms=None, layouts=None, ifile=None, abi=None, sym_prefix="", variadic_on_stack=False):
        self.ast = ast
        self.ifile = os.path.abspath(ifile) if ifile else None
        self.sym_prefix = sym_prefix   # the C symbol prefix of the target ("" on ELF, "_" on Mach-O and i386 PE)
        self.variadic_on_stack = variadic_on_stack   # the target passes variable arguments elsewhere than fixed ones (Apple arm64)
        self.abi = abi or {}           # function name -> its definition's ABI (merge_abi): the call sites follow it
        self.local_defs = set()
        self.origin = None
        self.root = os.path.abspath(root)
        self.syms = syms or {}
        self.layouts = layouts or {}
        self.types = Types()
        self.edits = Edits()
        self.cur = None
        self.census = Census(self)
        self.unsupported = []
        self.decl_by_id = {}
        self.typedef_ids = {}
        self.gp_vars = set()           # VarDecl ids whose storage is guest memory (u32)
        self.addr_taken = set()
        self.srcs = {}
        self.edits.src_for_extract = self.srcs
        self.calls = {}                # (callee, declared type, arg classes) -> first site     (ABI census)
        self.defs = {}                 # function name -> parameter classes of its definition
        self.fn_addr_taken = set()     # functions used as values (they need a function-table entry)
        self.skip = set()              # node ids whose translation a parent folded away
        self.fn_word = {}              # LValueToRValue node id -> the function-pointer type its `void *` read is cast to
        self.fn_adapted = set()        # argument node ids already wrapped by p4_fn_adapt
        self.callee_sig = {}           # node ids of an indirect call's callee expression -> the call site's register signature
        self.groups = {}
        self.static_inits = 0
        self.abi_decls = set()
        self.chain = set()
        self.abi_defs = {}
        self.pending_moves = []
        self.eof_code = []
        self.assert_done = set()

    def linked_name(self, decl, fallback):
        """The C-level name a declaration links as: its asm label, else its own name. Clang's JSON gives the symbol (`mangledName`), which on
        Mach-O carries the target's `_` for an ordinary name but is spelled as written for an asm label."""
        name = decl.get("mangledName")
        if not name:
            return fallback
        if self.sym_prefix and name.startswith(self.sym_prefix):
            return name[len(self.sym_prefix):]
        return name

    # --- helpers
    def site(self):
        """(original file relative to the mirror root, line, column) of the node being visited, for the census"""
        n = self.cur
        if not n:
            return None
        sp = span(n) or name_span(n)
        if not sp or not self.mine(sp[0], sp[1]):
            return None
        f, line, col = self.origin_of(sp[0], sp[1])
        return (os.path.relpath(f, self.root), line, col)

    def origin_of(self, f, off):
        if self.ifile and os.path.abspath(f) == self.ifile:
            if self.origin is None:
                self.origin = Origin(self.src(f))
            return self.origin.at(off)
        return f, 0, off

    def mine(self, f, off=None):
        """is this text part of the mirror (not a system header or a port header)?"""
        if self.ifile and os.path.abspath(f) == self.ifile:
            if off is None:
                return True
            of = self.origin_of(f, off)[0]
            return os.path.abspath(of).startswith(self.root + os.sep)
        return os.path.abspath(f).startswith(self.root + os.sep)

    def src(self, f):
        if f not in self.srcs:
            with open(f, "rb") as stream:
                self.srcs[f] = stream.read().decode("latin-1")   # byte offsets == character offsets
        return self.srcs[f]

    def unsup(self, node, what):
        sp = span(node) or name_span(node)
        if sp:
            f, line, _c = self.origin_of(sp[0], sp[1])
            loc = f"{os.path.relpath(f, self.root)}:{line}"
        else:
            loc = "?"
        self.census["unsupported:" + what] += 1
        self.unsupported.append((what, loc))

    @staticmethod
    def qt(node):
        return node.get("type", {}).get("qualType", "")

    def clq(self, qt):
        return self.types.classify(qt)

    # --- pass 0: indexes
    def find_groups(self, n):
        """declarators that share one declaration (`T *a, *b;`): consecutive siblings with the same range start"""
        stack = [n]
        while stack:
            x = stack.pop()
            kids = [c for c in x.get("inner", ()) if isinstance(c, dict)]
            run = []
            for c in kids + [None]:
                key = None
                if c is not None and c.get("kind") in ("FieldDecl", "VarDecl"):
                    r = c.get("range", {}).get("begin", {})
                    if "spellingLoc" in r:
                        r = r["spellingLoc"]
                    key = (r.get("file"), r.get("offset"))
                if run and key == run[0][0]:
                    run.append((key, c))
                else:
                    if len(run) > 1:
                        grp = [m for _k, m in run]
                        for m in grp:
                            self.groups[m["id"]] = grp
                    run = [(key, c)] if key is not None else []
            stack.extend(kids)






    def index(self, n):
        self.find_groups(n)
        for top in n.get("inner", ()):
            if isinstance(top, dict) and top.get("kind") == "FunctionDecl" and any(
                    isinstance(c, dict) and c.get("kind") == "CompoundStmt" for c in top.get("inner", ())):
                self.local_defs.add(top["name"])
        for top in n.get("inner", ()):
            if isinstance(top, dict) and top.get("kind") == "VarDecl":
                top["_global"] = True
        stack = [n]
        while stack:
            x = stack.pop()
            k = x.get("kind")
            if k == "TypedefDecl":
                self.types.add_typedef(x["name"], x["type"]["qualType"])
                self.typedef_ids.setdefault(x["name"], x["id"])
            if "id" in x:
                self.decl_by_id[x["id"]] = x
            for c in x.get("inner", ()):
                if isinstance(c, dict):
                    stack.append(c)

    def prescan(self, n, parent_kind=None):
        """address-taken variables"""
        noaddr = set()
        stack = [(n, None)]
        while stack:
            x, p = stack.pop()
            if x.get("kind") == "FunctionDecl" and "..." in x.get("type", {}).get("qualType", ""):
                noaddr.update(c["id"] for c in x.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "ParmVarDecl")
            if x.get("kind") == "UnaryOperator" and x.get("opcode") == "&":
                t = self.strip(x["inner"][0])
                if t.get("kind") == "DeclRefExpr":
                    self.addr_taken.add(t["referencedDecl"]["id"])
            for c in x.get("inner", ()):
                if isinstance(c, dict):
                    stack.append((c, x))
        self.addr_taken -= noaddr

    @staticmethod
    def strip(x):
        """through parens and implicit no-op casts"""
        while True:
            k = x.get("kind")
            if k == "ParenExpr":
                x = x["inner"][0]
            elif k == "ImplicitCastExpr" and x.get("castKind") in ("NoOp", "BitCast"):
                x = x["inner"][0]
            else:
                return x

    # --- classification of lvalues and decls
    def storage_decl(self, d):
        """is this VarDecl/FieldDecl/ParmVarDecl's pointer-typed entity guest storage?"""
        k = d.get("kind")
        c = self.clq(self.qt(d))
        if not c["ok"] or (c["levels"] == 0):
            return False
        if k == "FieldDecl":
            return True
        if k == "ParmVarDecl":
            return d["id"] in self.addr_taken and c["levels"] == 1
        if k == "VarDecl":
            if c["dims"]:
                return True
            # automatic: only when address-taken; globals/statics are always storage
            if d.get("storageClass") in ("static", "extern") or d.get("_global"):
                return True
            return d["id"] in self.addr_taken
        return False

    def is_gp_lvalue(self, x):
        x = self.strip_paren(x)
        k = x.get("kind")
        c = self.clq(self.qt(x))
        if not c["ok"] or c["levels"] == 0 or c["dims"]:
            return False
        if k == "MemberExpr":
            return True
        if k == "ArraySubscriptExpr":
            return True
        if k == "UnaryOperator" and x.get("opcode") == "*":
            return True
        if k == "DeclRefExpr":
            rd = x.get("referencedDecl", {})
            if rd.get("kind") not in ("VarDecl", "ParmVarDecl"):
                return False
            d = self.decl_by_id.get(rd["id"], rd)
            return self.storage_decl(d) or self.abs_addr(rd) is not None
        return False

    @staticmethod
    def strip_paren(x):
        while x.get("kind") == "ParenExpr":
            x = x["inner"][0]
        return x

    def abs_addr(self, rd):
        name = rd.get("name", "")
        if rd.get("kind") == "VarDecl":
            # `extern T local_name __asm__("D_00887300");` (an override naming retail data by its asm label): the label is the name
            d = self.decl_by_id.get(rd.get("id"))
            linked = self.linked_name(d, name) if d else name
            if d and linked != name and any(
                    isinstance(c, dict) and c.get("kind") == "AsmLabelAttr" for c in d.get("inner", ())):
                if re.match(r"^(?:func|FUN)_[0-9A-Fa-f]{8}", linked):
                    return None       # the address of a function symbol declared as data (`u8 p4cb_x[] __asm__("func_003bcbc0")`): a host function
                name = linked
        # the symbol lists are the evidence of where retail data is: a handful of D_XXXXXXXX names sit at another address than the
        # one they spell (D_00724840 is 0x00764840), which is also what gen_port's tables step places them at
        m = GP_RE.match(name)
        if m:       # gp-relative: gp plus the sign-extended 16-bit offset in the name, as gen_port's place_undefined computes it
            v = int(m[1], 16)
            return (GP_VALUE + (v - (1 << 32 if v & 0x80000000 else 0))) & 0xFFFFFFFF
        s = self.syms.get(name)
        if s and s[1] == "data":
            return s[0]
        m = ABS_RE.match(name)
        if m:
            return int(m[1], 16)
        return None

    # --- text edits
    def type_sig(self, qt):
        """the register signature (guest.h) a function pointer type declares: None for an unprototyped type, a struct, `...`
        or more than six parameters"""
        s = qt.strip()
        for _ in range(8):
            s2 = re.sub(r"\b(const|volatile)\b", "", s).strip()
            if re.fullmatch(r"[A-Za-z_]\w*", s2) and s2 in self.types.typedefs:
                s = self.types.typedefs[s2]
            else:
                s = s2
                break
        m = re.match(r"^(.*?)\s*\(\s*\*\s*\)\s*\((.*)\)$", s)
        if not m or "..." in m[2] or m[2].strip() == "":
            return None
        ps = self.split_params(f"{m[1].strip()} ({m[2]})")
        if ps is None or len(ps) > 6:
            return None
        sig = 0
        for i, p_ in enumerate(ps):
            cl = self.pclass(p_)
            if cl == "ptr":
                sig |= 1 << (1 + i)
                pc = self.clq(p_)
                if pc["ok"] and pc["fn"] and pc["levels"] == 1:
                    sig |= 1 << (12 + i)
            elif cl == "float":
                sig |= 1 << (18 + i)
            elif cl != "int":
                return None
        ret = m[1].strip()
        if ret != "void":
            cl = self.pclass(ret)
            if cl == "ptr":
                sig |= 1
            elif cl == "float":
                sig |= 1 << 24
            elif cl != "int":
                return None
        return 0x80000000 | len(ps) << 8 | sig

    def wrap_node(self, node, open_, close, depth, ftype=None):
        sig = self.callee_sig.get(node.get("id"))
        if sig is None and "p4_fn_g2h(" in open_:
            sig = self.type_sig(ftype or self.qt(node))      # a function pointer kept for later: the call that uses it has its type
        if sig is not None and "p4_fn_g2h(" in open_ and close.startswith("))"):
            # the callee of an indirect call: the table entry's own register signature is compared with this call site's
            open_ = open_.replace("p4_fn_g2h(", "p4_fn_g2h_sig(", 1)
            close = f"), 0x{sig:x}u)" + close[2:]
            self.census["indirect-call-sig"] += 1
        sp = span(node)
        if not sp or not self.mine(sp[0], sp[1]):
            self.census["skipped:not-editable"] += 1
            return False
        self.edits.wrap(sp[0], sp[1], sp[2], open_, close, depth)
        return True

    def g2h(self, c, qt):
        """(open, close) turning a u32 guest address expression into a host pointer of type qt"""
        ty = "u32 *" if c["levels"] >= 2 else qt
        fn = "p4_fn_g2h" if c["fn"] and c["levels"] == 1 else "p4_g2h"
        return f"(({ty}){fn}((u32)(", ")))"

    def sig_string(self, qt):
        """the register signature of a function or function-pointer type as the letters p4_fn_h2g_sig takes ("pii": a pointer result,
        two integers; `f` a function pointer parameter, `x` a float or double), or None (unprototyped, a struct, `...`, more than six parameters)"""
        s = qt.strip()
        for _ in range(8):
            s2 = re.sub(r"\b(const|volatile)\b", "", s).strip()
            if re.fullmatch(r"[A-Za-z_]\w*", s2) and s2 in self.types.typedefs:
                s = self.types.typedefs[s2]
            else:
                s = s2
                break
        m = re.match(r"^(.*?)\s*\(\s*\*\s*\)\s*(\(.*\))$", s)
        fn = f"{m[1].strip()} {m[2]}" if m else s
        ps = self.split_params(fn)
        rt = ret_text(fn)
        if ps is None or rt is None or len(ps) > 6:
            return None
        out = ""
        r = rt.strip()
        if r == "void":
            out = "v"
        else:
            cl = self.pclass(r)
            if cl == "ptr":
                out = "p"
            elif cl == "int":
                out = "i"
            elif cl == "float":
                out = "x"
            else:
                return None
        for p_ in ps:
            cl = self.pclass(p_)
            if cl == "ptr":
                pc = self.clq(p_)
                out += "f" if pc["ok"] and pc["fn"] and pc["levels"] == 1 else "p"
            elif cl == "int":
                out += "i"
            elif cl == "float":
                out += "x"
            else:
                return None
        return out

    @staticmethod
    def sig_numeric(letters):
        """guest.h's numeric signature of a letter string ("pii")"""
        v = 0x80000000 | (len(letters) - 1) << 8
        for i, ch in enumerate(letters):
            if ch in "pf":
                v |= 1 << i
            if ch == "f":
                v |= 1 << (11 + i)
            if ch == "x":
                v |= 1 << (24 if i == 0 else 17 + i)       # a float or double: the result in xmm0, a parameter in the next xmm register
        return v

    @staticmethod
    def abi_sig_string(d):
        """the signature letters of an ABI-table row ('x': a float or double), or None when it has structs, a variable part or more than six
        parameters"""
        if d.get("skip") or d.get("va") or len(d["params"]) > 6 or d["ret"] not in (None, "ptr", "int", "float"):
            return None
        if any(p_ is None or p_["cls"] not in ("ptr", "int", "float") for p_ in d["params"]):
            return None
        return ("v" if d["ret"] is None else "p" if d["ret"] == "ptr" else "x" if d["ret"] == "float" else "i") + "".join(
            ("f" if p_.get("fn") else "p") if p_["cls"] == "ptr" else "x" if p_["cls"] == "float" else "i" for p_ in d["params"])

    def adapt_fn_cast(self, x, inner, t, depth):
        """A named function converted to a function-pointer type whose calls use another register ABI (a callback defined with m2c
        integers, passed where RenderWare calls a `RpMaterialCallBack(RpMaterial *, void *)`): the pointer becomes the function or its
        adapter (guest.h p4_fn_adapt)."""
        own = self.fn_sig_for(inner)
        dst = self.sig_string(t)
        if not own or not dst:
            return False

        def cls(s):
            return [letter_class(ch) for ch in s]      # result, then parameters; void is an integer slot
        m = min(len(own), len(dst))
        if cls(own)[:m] == cls(dst)[:m]:
            return False
        self.census["fn-cast-adapted"] += 1
        return self.wrap_node(x, f"(({t})p4_fn_adapt((const void *)(", f'), "{own}", "{dst}"))', depth)

    def fn_sig_for(self, node):
        """the signature to register with a function stored into guest memory: only when the expression names the function (`= f`): its own
        type is its signature, and a static function has no ABI-table row, so this is where it is registered. A function pointer
        variable says nothing about the function behind it (its declared type is how it will be called), so it registers nothing"""
        n = node
        while n.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr") and n.get("inner"):
            n = n["inner"][0]
            if n.get("kind") == "DeclRefExpr" and n.get("referencedDecl", {}).get("kind") == "FunctionDecl":
                rd = n["referencedDecl"]
                decl = self.decl_by_id.get(rd["id"], {})
                name = self.linked_name(decl, rd["name"])
                if rd["name"] in self.local_defs and rd["name"] == name:
                    return self.sig_string(rd["type"]["qualType"])        # defined in this unit: its own type is its signature
                d = self.abi.get(name) if self.abi else None
                if d is not None:
                    # defined elsewhere, a function the ABI table knows: its definition says what it takes (an m2c unit may declare it
                    # `void f(void)`)
                    return self.abi_sig_string(d)
                return None
        return None

    def h2g(self, c, node=None):
        if c["fn"] and c["levels"] == 1:
            sig = self.fn_sig_for(node) if node is not None else None
            if sig:
                return ("p4_fn_h2g_sig((", f'), "{sig}")')
            return ("p4_fn_h2g((", "))")
        return ("p4_h2g((", "))")

    # --- the walk
    cur_parent_kind = None

    def walk(self, x, depth=0, parent=None):
        k = x.get("kind")
        self.cur = x
        self.cur_parent_kind = "callee" if (parent and parent.get("kind") == "CallExpr" and parent["inner"][0] is x) else None
        h = getattr(self, "v_" + k, None) if k else None
        if h:
            h(x, depth, parent)
        for c in x.get("inner", ()):
            if isinstance(c, dict):
                self.walk(c, depth + 1, x)

    # ---- declarations
    @staticmethod
    def word_ptr(c):
        """a pointer to a word type (`size_t *`): what it points at is 4 bytes in guest memory"""
        return c["ok"] and c["levels"] == 1 and not c["fn"] and not c.get("parr") and c["base"] in WORD_BASES

    def word_of(self, qt):
        """`u32`/`s32` when qt is size_t, intptr_t, long ... (a word on the PS2, 8 bytes on LP64), else None"""
        c = self.clq(qt)
        if c["ok"] and c["levels"] == 0 and c["base"] in WORD_BASES:
            return WORD_BASES[c["base"]]
        return None

    def decl_plan(self, d):
        """what has to change in the text of one declarator, or None"""
        w = self.word_of(self.qt(d)) if d.get("kind") in ("FieldDecl", "VarDecl") and (
            d.get("kind") == "FieldDecl" or d.get("_global") or d.get("storageClass") in ("static", "extern")) else None
        if w:
            sp, ns = span(d), name_span(d)
            if not sp or not ns or not self.mine(sp[0], sp[1]) or not d.get("name") or not (sp[1] <= ns[1] < sp[2]):
                return None
            f, b, e = sp
            text = self.src(f)
            prefix = text[b:ns[1]]
            sc = " ".join(w_ for w_ in KEYWORDS_STORAGE if re.search(r"\b" + w_ + r"\b", prefix))
            self.census["word-type:" + d["kind"]] += 1
            return dict(storage=True, ty=w + " ", f=f, b=b, e=e, ns=ns, named=True, sfx="", keep_dims="", sc=(sc + " ") if sc else "",
                        name=d["name"])
        c = self.clq(self.qt(d))
        if not c["ok"]:
            if "*" in self.qt(d) or "(*" in self.qt(d):
                self.unsup(d, "decl-type:" + self.qt(d))
            return None
        if c["levels"] == 0:
            return None
        k = d.get("kind")
        storage = self.storage_decl(d)
        if k == "ParmVarDecl":
            if d["id"] in self.addr_taken and c["levels"] == 1:
                return None                   # v_FunctionDecl gives it a guest-storage local
            storage = False
        if not storage and c["levels"] < 2 and not (c.get("parr") and c.get("elem_ptr")):
            return None
        if not storage and c.get("parr") and c.get("elem_ptr") and c["levels"] == 1:
            # a register pointer to an array of guest-stored pointers: `T *(*p)[N]` -> `u32 (*p)[N]`
            sp, ns = span(d), name_span(d)
            if not sp or not ns or not self.mine(sp[0], sp[1]) or not d.get("name") or not (sp[1] <= ns[1] < sp[2]):
                return None
            f, b, e = sp
            prefix = self.src(f)[b:ns[1]]
            sc = " ".join(w_ for w_ in KEYWORDS_STORAGE if re.search(r"\b" + w_ + r"\b", prefix))
            self.census["decl-ptr-to-ptr-array"] += 1
            # spelled out (`T *(*p)[N]`) the declarator's own `)[N]` stays; through a typedef (`Slot *p`) it is added; for function pointers
            # (`R (*(*p)[N])(P)`) the declarator's `)[N])(P)` becomes `)[N]`
            sfx, dims = "", ""
            if "(" not in prefix:
                dims = ")" + c["adims"]
            elif c["base"].endswith(")") and "(*)(" in c["base"]:
                suffix = self.src(f)[ns[2]:e]
                sfx = suffix[:suffix.index("=")].rstrip() if "=" in suffix else suffix
                dims = ")" + c["adims"]
            return dict(storage=False, ty="u32 (*", f=f, b=b, e=e, ns=ns, named=True, sfx=sfx, keep_dims=dims, sc=(sc + " ") if sc else "",
                        name=d["name"])
        sp = span(d)
        ns = name_span(d)
        if not sp or not self.mine(sp[0], sp[1]):
            self.census["skipped:macro-or-foreign"] += 1
            return None
        f, b, e = sp
        text = self.src(f)
        named = bool(d.get("name")) and ns and ns[0] == f and b <= ns[1] < e
        pre_end = ns[1] if named else e
        prefix = text[b:pre_end]
        suffix = text[ns[2]:e] if named else ""
        # initializer: the suffix stops at the '=' that precedes the initializer expression
        init_at = None
        if k == "VarDecl" and d.get("init") and d.get("inner"):
            isp = span(d["inner"][-1])
            if isp and isp[0] == f:
                before = text[:isp[1]].rstrip()
                if before.endswith("="):
                    init_at = len(before) - 1 - ns[2] if named else None
                    if init_at is not None and init_at < 0:
                        init_at = None
        init_text = suffix[init_at:] if init_at is not None else ""
        sfx = suffix[:init_at] if init_at is not None else suffix
        if init_text:
            self.census["init-of-retyped-decl"] += 1
        keep_dims = leading_dims(sfx) if storage else ""
        if storage and not c["fn"] and not c.get("parr") and sfx[len(keep_dims):].strip().strip(")"):
            self.unsup(d, "decl-suffix")
            return None
        if k == "ParmVarDecl":
            keep_dims = ""
        if k == "VarDecl" and init_text and d["inner"][-1].get("kind") == "InitListExpr":
            m_ = re.match(r"\s*\[\s*\]", keep_dims)
            if m_:          # an array sized by its initializer keeps the size once the initializer moves
                keep_dims = f"[{len([c for c in d['inner'][-1]['inner'] if isinstance(c, dict)])}]" + keep_dims[m_.end():]
        sc = " ".join(w for w in KEYWORDS_STORAGE if re.search(r"\b" + w + r"\b", prefix))
        sc = (sc + " ") if sc else ""
        if storage:
            self.census[f"decl-storage:{k}"] += 1
        else:
            self.census[f"decl-ptr2:{k}"] += 1
        return dict(storage=storage, ty="u32 " if storage else "u32 *", f=f, b=b, e=e, ns=ns, named=named, sfx=sfx,
                    keep_dims=keep_dims, sc=sc, name=d.get("name"))

    def emit_decl(self, p, first, prev_e):
        f, ns = p["f"], p["ns"]
        stars = p["ty"]
        if p["named"]:
            if first:
                self.edits.replace(f, p["b"], ns[1], p["sc"] + stars)
            else:
                self.edits.replace(f, prev_e, ns[1], ", " + ("" if p["storage"] else "*"))
            if p["sfx"] != p["keep_dims"]:
                self.edits.replace(f, ns[2], ns[2] + len(p["sfx"]), p["keep_dims"].strip())
        else:
            self.edits.replace(f, p["b"], p["e"], (p["sc"] + stars).rstrip())

    def split_group(self, grp, plans):
        """`T *a, *b[64];` where only some declarators change: the declaration is split at the boundaries
        (`T *a; u32 b[64];`) so every declarator keeps or gets its own type"""
        sps = [span(m) for m in grp]
        nss = [name_span(m) for m in grp]
        if any(not s or not n for s, n in zip(sps, nss)) or any(p and not p["named"] for p in plans):
            self.unsup(grp[0], "declarator-group-mixed")
            return
        f, b = sps[0][0], sps[0][1]
        text = self.src(f)
        if not self.mine(f, b):
            return
        declspec = re.sub(r"[\s*]+$", "", text[b:nss[0][1]])
        for i, m in enumerate(grp):
            p = plans[i]
            if i == 0:
                if p:
                    self.edits.replace(f, b, nss[0][1], p["sc"] + p["ty"])
            else:
                prev_e = sps[i - 1][2]
                own = text[prev_e:nss[i][1]]
                stars = own.split(",", 1)[1] if "," in own else ""
                prev = plans[i - 1]
                same = (p is None) == (prev is None) and (p is None or p["ty"] == prev["ty"])
                if same:
                    if p:
                        self.edits.replace(f, prev_e, nss[i][1], ", " + ("" if p["storage"] else "*"))
                else:
                    head = (p["sc"] + p["ty"]) if p else declspec + " " + stars.lstrip()
                    self.edits.replace(f, prev_e, nss[i][1], "; " + head)
            if p and p["sfx"] != p["keep_dims"]:
                self.edits.replace(f, nss[i][2], nss[i][2] + len(p["sfx"]), p["keep_dims"].strip())
        self.census["declarator-group-split"] += 1

    def decl_text(self, d, depth):
        grp = self.groups.get(d["id"])
        if grp is None:
            p = self.decl_plan(d)
            if p:
                self.emit_decl(p, True, None)
            return
        if grp[0] is not d:
            return                           # edited with the first declarator of its group
        plans = [self.decl_plan(m) for m in grp]
        live = [p for p in plans if p]
        if not live:
            return
        if len(live) != len(plans) or len({p["ty"] for p in plans}) != 1 or any(not p["named"] for p in plans):
            self.split_group(grp, plans)
            return
        self.census["declarator-group"] += 1
        for i, p in enumerate(plans):
            self.emit_decl(p, i == 0, plans[i - 1]["e"] if i else None)

    def v_FieldDecl(self, d, depth, parent):
        self.decl_text(d, depth)

    def v_VarDecl(self, d, depth, parent):
        self.decl_text(d, depth)
        if not (d.get("init") and d.get("inner")):
            return
        init = d["inner"][-1]
        static_like = d.get("_global") or d.get("storageClass") == "static"
        if init.get("kind") == "InitListExpr" or static_like:
            leaves = []
            self.init_leaves(init, leaves)
            nonconst = False
            for leaf in leaves:
                nonconst |= self.store_value(leaf, self.clq(self.qt(leaf)), depth + 1, d)
            if static_like:
                self.pending_moves.append((d, init, depth))      # its children are visited after this handler: decide afterwards
        elif self.storage_decl(d):
            self.store_value(init, self.clq(self.qt(d)), depth + 1, d)

    def init_leaves(self, node, out):
        """the pointer-typed leaves of an initializer: each initialises guest storage, so each holds a guest address"""
        k = node.get("kind")
        if k in ("InitListExpr", "DesignatedInitExpr"):
            for c in node.get("inner", ()):
                if isinstance(c, dict):
                    self.init_leaves(c, out)
        elif k not in ("ImplicitValueInitExpr", "ArrayFiller"):
            c = self.clq(self.qt(node))
            if c["ok"] and c["levels"] >= 1 and not c["dims"]:
                out.append(node)

    def move_static_init(self, d, init, depth):
        """A static object whose initializer now needs the translation cannot be initialised by the compiler (a guest
        address is not a link-time constant). The initializer text, with its rewrites, becomes a copy into the object, run
        once the guest block exists: a registered init function for a file-scope object, a one-shot block after the
        declaration for a function-local static."""
        sp = span(d)
        isp = span(init)
        ns = name_span(d)
        if not sp or not isp or not ns:
            self.unsup(d, "static-pointer-initializer")
            return
        if not self.mine(sp[0], sp[1]):
            return
        f, b, e = sp
        text = self.src(f)
        # only an initializer that now calls the translation (or reads guest memory) is no longer a link-time constant
        if not any("p4_" in ev[5] for ev in self.edits.by_file.get(f, ()) if isp[1] <= ev[0] <= isp[2]):
            return
        before = text[:isp[1]].rstrip()
        if not before.endswith("="):
            self.unsup(d, "static-pointer-initializer")
            return
        eq = len(before) - 1
        name = d["name"]
        aggregate = init.get("kind") == "InitListExpr"
        rendered = self.edits.extract(f, isp[1], isp[2])
        self.edits.replace(f, eq, isp[2], "")
        # an array sized by its initializer keeps its size; a const object must be writable (the init writes it)
        if aggregate:
            m = re.compile(r"\s*\[\s*\]").match(text, ns[2])
            if m and not self.has_edit_at(f, ns[2]):
                self.edits.replace(f, ns[2], m.end(), f"[{len([c for c in init['inner'] if isinstance(c, dict)])}]")
        for m in re.finditer(r"\bconst\b", text[b:ns[1]]):
            if not self.has_edit_at(f, b + m.start()):
                self.edits.replace(f, b + m.start(), b + m.end(), "")
        if aggregate:
            stmt = f"__builtin_memcpy((void *)&{name}, &(__typeof__({name})){rendered}, sizeof {name});"
        else:
            stmt = f"{name} = {rendered};"
        self.static_inits += 1
        n = self.static_inits
        if d.get("_global"):
            self.eof_code.append((f, f"\nstatic void p4_ginit_{n}(void) {{ {stmt} }}\n"
                                 f"static void __attribute__((constructor)) p4_gireg_{n}(void) {{ p4_guest_on_init(p4_ginit_{n}); }}\n"))
        else:
            semi = text.find(";", isp[2])
            self.edits.insert(f, semi + 1, f" {{ static char p4_once; if (!p4_once) {{ p4_once = 1; {stmt} }} }}")
        self.census["static-init-moved"] += 1

    def has_edit_at(self, f, pos):
        return any(ev[0] <= pos < (ev[6] if ev[4] == "rep" else ev[0] + 1) for ev in self.edits.by_file.get(f, ()))

    def v_ParmVarDecl(self, d, depth, parent):
        self.decl_text(d, depth)

    def v_TypedefDecl(self, d, depth, parent):
        c = self.clq(d["type"]["qualType"])
        if c["ok"] and c["levels"] >= 2 and not c["fn"]:
            self.unsup(d, "typedef-pointer-to-pointer")
        elif not c["ok"] and re.search(r"\(\s*\*", d["type"]["qualType"]):
            self.unsup(d, "typedef-type:" + d["type"]["qualType"])

    def v_RecordDecl(self, d, depth, parent):
        if not d.get("completeDefinition"):
            return
        sp = span(d)
        if not sp or not self.mine(sp[0], sp[1]):
            return
        fields = [c for c in d.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "FieldDecl"]
        if not any(self.clq(self.qt(f))["levels"] for f in fields):
            return
        self.census["record-with-pointer"] += 1
        self.pending_records.append(d)

    pending_records = ()

    # ---- expressions
    def v_ImplicitCastExpr(self, x, depth, parent):
        self.cast(x, depth, implicit=True)

    def v_CStyleCastExpr(self, x, depth, parent):
        self.mark_fn_word(x)
        if x.get("inner") and "*" in self.qt(x):
            self.va_list_address(x["inner"][0], depth)
        self.cast(x, depth, implicit=False)

    def v_RecoveryExpr(self, x, depth, parent):
        """clang's stand-in for an expression it could not type: on AArch64 a va_list handed to a pointer parameter, or cast to a pointer, is one"""
        for c in x.get("inner", ()):
            if isinstance(c, dict):
                self.va_list_address(c, depth)

    @staticmethod
    def is_va_list(qt):
        return qt.strip() in ("va_list", "__builtin_va_list", "__gnuc_va_list", "struct __va_list")

    def va_list_address(self, node, depth):
        """A `va_list` object used where a pointer is wanted (the decomp passes `args` on to a `void *` parameter, or casts it: on i386 a va_list is a
        pointer to the arguments, on x86_64 an array that decays to the address of its one element): the address of the object. On AArch64 a va_list is a
        struct, and there is no conversion without the `&`."""
        n = self.peel(node)
        if n.get("kind") != "DeclRefExpr" or not self.is_va_list(self.qt(n)):
            return
        sp = span(n)
        if sp and self.mine(sp[0], sp[1]):
            self.edits.wrap(sp[0], sp[1], sp[2], "(&", ")", depth + 2)
            self.census["va-list-address"] += 1

    def mark_fn_word(self, x):
        """A data-pointer word read from guest storage and cast to a function pointer is a code address, regardless of its declared pointer depth
        (`(Code)table[0]` can read void ** from a void *** table). Convert through the function table, not the data window, and adapt the
        registered function's register classes."""
        if x.get("castKind") not in ("BitCast", "NoOp") or not x.get("inner"):
            return
        c = self.clq(self.qt(x))
        if not (c["ok"] and c["fn"] and c["levels"] == 1):
            return
        n = self.strip(x["inner"][0])
        if n.get("kind") == "ImplicitCastExpr" and n.get("castKind") == "LValueToRValue" and n.get("inner"):
            sc = self.clq(self.qt(n))
            if sc["ok"] and sc["levels"] >= 1 and not (sc["fn"] and sc["levels"] == 1) and not sc["dims"] and self.is_gp_lvalue(n["inner"][0]):
                self.fn_word[n["id"]] = self.qt(x)

    def cast(self, x, depth, implicit):
        ck = x.get("castKind")
        if x["id"] in self.skip:
            return
        inner = x["inner"][0]
        t = self.qt(x)
        c = self.clq(t)
        if ck == "FunctionToPointerDecay":
            tgt = inner
            if tgt.get("kind") == "DeclRefExpr" and tgt.get("referencedDecl", {}).get("kind") == "FunctionDecl":
                if self.cur_parent_kind != "callee":
                    self.fn_addr_taken.add(tgt["referencedDecl"]["name"])
            return
        if not implicit and ck in ("BitCast", "NoOp") and c["ok"] and c["fn"] and c["levels"] == 1:
            # A data-pointer word cast to a function pointer is a code address, including a word declared with multiple data-pointer levels.
            # Do not project it into the guest data window before resolving the callback.
            peeled = self.strip(inner)
            if peeled.get("kind") == "ImplicitCastExpr" and peeled.get("castKind") == "LValueToRValue" and peeled.get("inner"):
                ic = self.clq(self.qt(peeled))
                if ic["ok"] and ic["levels"] >= 1 and not (ic["fn"] and ic["levels"] == 1) and not ic["dims"] and self.is_gp_lvalue(peeled["inner"][0]):
                    self.skip.add(peeled["id"])
                    self.census["fn-cast-of-data-word"] += 1
                    self.wrap_node(inner, "p4_fn_g2h((u32)(", "))", depth + 1, ftype=t)
                    return
        if ck in ("BitCast", "NoOp") and c["ok"] and c["fn"] and c["levels"] == 1 and self.adapt_fn_cast(x, inner, t, depth):
            return
        if ck == "ArrayToPointerDecay":
            t0 = self.strip(inner)
            if t0.get("kind") == "DeclRefExpr" and t0.get("referencedDecl", {}).get("kind") == "VarDecl":
                d = self.decl_by_id.get(t0["referencedDecl"]["id"], t0["referencedDecl"])
                if not d.get("_global") and d.get("storageClass") not in ("static", "extern") and self.abs_addr(t0["referencedDecl"]) is None:
                    self.census["addr-of-local-array"] += 1
            return
        if ck == "LValueToRValue":
            if c["ok"] and c["levels"] >= 1 and not c["dims"] and self.is_gp_lvalue(inner):
                if x["id"] in self.fn_word:
                    self.census["read-ptr:fn-word"] += 1
                    self.wrap_node(x, "((void *)p4_fn_g2h((u32)(", ")))", depth, ftype=self.fn_word[x["id"]])
                    return
                o, cl = self.g2h(c, t)
                self.census["read-ptr" + (":fn" if c["fn"] else "")] += 1
                self.wrap_node(x, o, cl, depth)
            return
        if ck == "IntegralToPointer":
            if not c["ok"]:
                self.unsup(x, "int-to-ptr-type:" + t)
                return
            self.census["int2ptr" + (":fn" if c["fn"] else "") + (":implicit" if implicit else ":explicit")] += 1
            fnk = "p4_fn_g2h" if c["fn"] and c["levels"] == 1 else "p4_g2h"
            # A pointer made from integer arithmetic (`(u8 *)(i * 4)`, an offset that is added to a base afterwards) is not a pointer word: its 0 is an
            # ordinary value, and must stay the block's base, not become NULL
            peeled = self.strip(inner)
            while peeled.get("kind") in ("CStyleCastExpr", "ImplicitCastExpr") and peeled.get("castKind") in ("IntegralCast", "NoOp") and peeled.get("inner"):
                peeled = self.strip(peeled["inner"][0])            # (u8 *)(uintptr_t)(i * 0x2c)
            arith = peeled.get("kind") == "BinaryOperator" and peeled.get("opcode") in ("+", "-", "*", "<<", ">>", "|", "&", "/", "%") and fnk == "p4_g2h"
            if arith:
                fnk = "p4_g2h_off"
                self.census["int2ptr-arith"] += 1
            if implicit:
                o, cl = self.g2h(c, t)
                if arith:
                    o = o.replace("p4_g2h(", "p4_g2h_off(", 1)
                self.wrap_node(x, o, cl, depth)
            else:
                self.wrap_node(inner, f"{fnk}((u32)(", "))", depth + 1, ftype=t)
                self.cast_type_text(x, c)
            return
        if ck == "PointerToIntegral":
            ic = self.clq(self.qt(inner))
            fnk = ic["fn"] and ic["levels"] == 1
            self.census["ptr2int" + (":fn" if fnk else "") + (":implicit" if implicit else ":explicit")] += 1
            peeled = self.strip(inner)
            if (peeled.get("kind") == "ImplicitCastExpr" and peeled.get("castKind") == "LValueToRValue"
                    and self.is_gp_lvalue(peeled["inner"][0])):
                self.skip.add(peeled["id"])                       # the storage already holds the guest address
                self.census["ptr2int-folded-read"] += 1
                return
            if self.null_offset(inner):
                # `(s32)&((T *)0)->field`: a byte offset into T, the same number on both sides; converted as an integer, never by p4_h2g.
                # Through a pointer-wide type (every target of the rewrite is 64-bit), spelled out: the text is compiled as cpp-output.
                self.census["ptr2int-null-offset"] += 1
                if implicit:
                    self.wrap_node(x, f"(({t})(unsigned long long)(", "))", depth)
                else:
                    self.wrap_node(inner, "(unsigned long long)(", ")", depth + 1)
                return
            o, cl = self.h2g(ic, inner)
            if implicit:
                self.wrap_node(x, f"(({t})" + o, cl + ")", depth)
            else:
                self.wrap_node(inner, o, cl, depth + 1)
            return
        if not implicit and ck in ("BitCast", "NoOp") and c["ok"] and (c["levels"] >= 2 or c.get("elem_ptr") or self.word_ptr(c)):
            self.cast_type_text(x, c)

    def cast_type_text(self, x, c):
        """`(T **)` -> `(u32 *)` for casts whose type is a pointer to a pointer (function pointers included)"""
        if c["levels"] < 2 and not c.get("elem_ptr") and not self.word_ptr(c):
            return
        sp_ = span(x)
        inner = x["inner"][0]
        isp = span(inner)
        if not sp_ or not isp or not self.mine(sp_[0], sp_[1]) or isp[0] != sp_[0]:
            self.unsup(x, "cast-type-text")
            return
        text = self.src(sp_[0])
        if text[sp_[1]] != "(":
            self.unsup(x, "cast-type-text")
            return
        depth, i = 0, sp_[1]
        while i < isp[1]:
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        else:
            self.unsup(x, "cast-type-text")
            return
        if self.word_ptr(c):
            new = f"({WORD_BASES[c['base']]} *)"
            self.census["word-type:cast"] += 1
        else:
            new = f"(u32 (*){c['adims']})" if c.get("elem_ptr") and c["levels"] == 1 else "(u32 *)"
        self.edits.replace(sp_[0], sp_[1], i + 1, new)
        self.census["cast-type-ptr2"] += 1

    def v_DeclRefExpr(self, x, depth, parent):
        rd = x.get("referencedDecl", {})
        if rd.get("kind") != "VarDecl":
            return
        a = self.abs_addr(rd)
        if a is None:
            return
        t = self.qt(x)
        c = self.clq(t)
        if not c["ok"]:
            self.unsup(x, "abs-type:" + t)
            return
        addr = f"p4_g2h(0x{a:08X}u)"
        if c["levels"] == 0 and c["base"] in WORD_BASES:
            wt = WORD_BASES[c["base"]]
            self.census["word-type:abs"] += 1
            new = f"(*({wt} (*){c['dims']}){addr})" if c["dims"] else f"(*({wt} *){addr})"
        elif c["dims"]:
            if c["levels"] >= 1:
                elem = "u32"
            else:
                m_ = re.match(r"^(.*?)\s*((?:\[[^\]]*\]\s*)+)$", t)
                elem = m_.group(1) if m_ else c["base"]
                if "unnamed" in elem or "anonymous" in elem or "ANON" in elem:
                    # an anonymous record has no spelling: the extern declaration still names the object
                    sp0 = span(x)
                    if not sp0 or not self.mine(sp0[0], sp0[1]):
                        return
                    nm = self.src(sp0[0])[sp0[1]:sp0[2]]
                    self.census["abs-data:anonymous"] += 1
                    self.edits.replace(sp0[0], sp0[1], sp0[2], f"(*(__typeof__({nm}) *){addr})")
                    return
            self.census["abs-data:" + ("ptr-array" if c["levels"] else "array")] += 1
            new = f"(*({elem} (*){c['dims']}){addr})"
        elif c["levels"] >= 1:
            self.census["abs-data:ptr"] += 1
            new = f"(*(u32 *){addr})"
        else:
            self.census["abs-data:scalar"] += 1
            new = f"(*({t} *){addr})"
        sp_ = span(x)
        if not sp_ or not self.mine(sp_[0], sp_[1]):
            self.census["skipped:not-editable"] += 1
            return
        self.edits.replace(sp_[0], sp_[1], sp_[2], new)

    def store_value(self, rhs, lc, depth, ctx):
        """rhs is about to be stored into guest pointer storage of class lc: translate it to a guest address. Returns
        False when the stored value is a constant (null), True when the text now calls the translation."""
        p = self.strip(rhs)
        # a store of a null pointer constant: the slot holds 0
        if p.get("kind") == "IntegerLiteral" and p.get("value") == "0":
            return False
        if p.get("castKind") == "NullToPointer" or (p.get("kind") == "CStyleCastExpr" and p.get("castKind") == "NullToPointer"):
            sp_ = span(rhs)
            if sp_ and self.mine(sp_[0], sp_[1]):
                if self.src(sp_[0])[sp_[1]:sp_[2]].strip() != "0":
                    self.edits.replace(sp_[0], sp_[1], sp_[2], "0")
                self.census["store-null"] += 1
                return False
        # a chained assignment `a = b = p`: the inner one already yields the guest address
        if p.get("kind") == "BinaryOperator" and p.get("opcode") == "=" and self.is_gp_lvalue(p["inner"][0]):
            self.chain.add(p["id"])
            self.census["store-chain"] += 1
            return True
        # copy of a pointer between guest slots: no translation either way
        if p.get("kind") == "ImplicitCastExpr" and p.get("castKind") == "LValueToRValue" and self.is_gp_lvalue(p["inner"][0]):
            self.skip.add(p["id"])
            self.census["store-copy"] += 1
            return True
        # an integer made into a pointer only to be stored: store the integer
        if p.get("kind") == "ImplicitCastExpr" and p.get("castKind") == "IntegralToPointer":
            self.skip.add(p["id"])
            self.census["store-int"] += 1
            return True
        if p.get("kind") == "CStyleCastExpr" and p.get("castKind") == "IntegralToPointer":
            self.skip.add(p["id"])
            sp_ = span(p)
            isp = span(p["inner"][0])
            if sp_ and isp and self.mine(sp_[0], sp_[1]):
                text = self.src(sp_[0])
                seg = text[sp_[1]:isp[1]]
                m = re.match(r"^\(\s*[^()]*\)\s*", seg)
                if m and "(" not in m.group(0)[1:]:
                    self.edits.replace(sp_[0], sp_[1], sp_[1] + m.end(), "")
                    self.census["store-int-cast"] += 1
                    return True
            self.skip.discard(p["id"])
        ic = self.clq(self.qt(rhs))
        if p.get("kind") == "StringLiteral" or (p.get("kind") == "ImplicitCastExpr" and p.get("inner") and
                                                   self.strip(p["inner"][0]).get("kind") == "StringLiteral"):
            self.census["string-literal-into-guest"] += 1
        o, cl = self.h2g(lc if lc["fn"] else ic, rhs)
        self.census["store-ptr" + (":fn" if lc["fn"] else "")] += 1
        self.wrap_node(rhs, o, cl, depth)
        return True

    def v_BinaryOperator(self, x, depth, parent):
        op = x.get("opcode")
        lhs, rhs = x["inner"]
        if op == "=":
            if self.is_gp_lvalue(lhs):
                self.store_value(rhs, self.clq(self.qt(lhs)), depth + 1, x)
                # the value of the assignment is a pointer in C and the guest address now: a user needs the pointer
                if x["id"] not in self.chain and parent is not None and parent.get("kind") != "CompoundStmt":
                    c = self.clq(self.qt(lhs))
                    o, cl = self.g2h(c, self.qt(lhs))
                    self.wrap_node(x, o, cl, depth)
                    self.census["assign-value-used"] += 1
        elif op in ("+", "-"):
            self.signed_offset(x, lhs, rhs, depth)
            if op == "+":
                self.guest_sum(x, lhs, rhs, depth)

    def v_ArraySubscriptExpr(self, x, depth, parent):
        if len(x.get("inner", ())) == 2:
            self.signed_offset(x, x["inner"][0], x["inner"][1], depth, subscript=True)

    @staticmethod
    def is_u32(node):
        t = node.get("type", {})
        return (t.get("desugaredQualType") or t.get("qualType", "")) == "unsigned int"

    def signed_offset(self, x, lhs, rhs, depth, subscript=False, compound=False):
        """`ptr + u32` / `ptr - u32` / `ptr[u32]`: on the PS2 the sum wraps mod 2^32, so an offset that is a negative number held in an unsigned
        word (a `(u32)offset` the code adds to a base) reaches an address below the base. On a 64-bit host the same u32 widens to 2^32 + the offset and
        misses the block. The guest block is 64 MB, so a real offset never reaches 2^31: reading the word as signed is the wrapped sum."""
        lp = self.clq(self.qt(lhs))
        if not (lp["ok"] and lp["levels"] >= 1 and not lp["dims"]) or not self.is_u32(rhs):
            return
        if not subscript and not compound and not self.clq(self.qt(x))["levels"]:
            return                                   # pointer - pointer, comparisons never reach here with an integer result
        sp = span(rhs)
        if not sp or not self.mine(sp[0], sp[1]):
            return
        self.edits.wrap(sp[0], sp[1], sp[2], "(s32)(", ")", depth + 1)
        self.census["signed-offset"] += 1

    @staticmethod
    def peel(n):
        while n.get("kind") in ("ParenExpr", "ImplicitCastExpr") and n.get("inner"):
            n = n["inner"][0]
        return n

    def is_ptr_as_int(self, node):
        """an explicit cast of a pointer to an integer (m2c's `(s32)base`): the guest address of the pointer; a null-based offset is not one"""
        n = self.peel(node)
        if n.get("kind") != "CStyleCastExpr" or not n.get("inner") or self.pclass(self.qt(n)) != "int":
            return False
        c = self.clq(self.qt(self.peel(n["inner"][0])))
        return bool(c["ok"] and c["levels"] >= 1 and not c["dims"]) and not self.null_offset(n["inner"][0])

    def null_offset(self, node):
        """an address computed from a null pointer constant: `&((T *)0)->field` (MEMBER_OFFSET, the decomp's offsetof), through any member,
        subscript, no-op cast or `+`/`-` step. Its value is a byte offset into T, not a host pointer; p4_h2g could only guess that from the number."""
        n = node
        while True:
            k, inner = n.get("kind"), n.get("inner") or []
            if not inner:
                return False
            if n.get("castKind") == "NullToPointer":
                return True
            if k == "ParenExpr" or (k in ("ImplicitCastExpr", "CStyleCastExpr") and n.get("castKind") in ("NoOp", "BitCast", "ArrayToPointerDecay")):
                n = inner[0]
            elif (k == "UnaryOperator" and n.get("opcode") == "&") or k == "MemberExpr":
                n = inner[0]                              # `->`: the base pointer; `.`: the base record, itself null-based or not
            elif (k == "ArraySubscriptExpr" or (k == "BinaryOperator" and n.get("opcode") in ("+", "-"))) and len(inner) == 2:
                n = inner[1] if self.clq(self.qt(inner[1]))["levels"] and not self.clq(self.qt(inner[0]))["levels"] else inner[0]
            else:
                return False

    def guest_sum(self, x, lhs, rhs, depth, compound=False):
        """`ptr + (s32)other_ptr`: an m2c base-plus-offset kept in a pointer (`p = (u8 *)(index * 4); p += (s32)base;`) is a sum of two
        guest addresses. On the host a NULL (the pointer form of 0, which p4_g2h keeps as NULL) plus a host-translated address is not that
        sum: the sum is formed on guest addresses and translated once, `p4_g2h(p4_h2g(ptr) + (u32)(int))`."""
        lp = self.clq(self.qt(lhs))
        if not (lp["ok"] and lp["levels"] >= 1 and not lp["dims"]) or not self.is_ptr_as_int(rhs):
            return
        o, cl = self.g2h(lp, self.qt(lhs))
        ls, rs = span(lhs), span(rhs)
        if not ls or not rs or not self.mine(ls[0], ls[1]):
            return
        if compound:
            n = self.peel(lhs)
            sp = span(x)
            if n.get("kind") != "DeclRefExpr" or not sp or self.is_gp_lvalue(lhs):
                return
            text = self.src(ls[0])[ls[1]:ls[2]]
            self.edits.replace(ls[0], ls[2], rs[1], f" = {o}p4_h2g({text}) + (u32)(")
            self.edits.wrap(rs[0], rs[1], rs[2], "", ")" + cl, depth + 1)
        else:
            sp = span(x)
            if not sp:
                return
            self.edits.wrap(sp[0], sp[1], sp[2], o, cl, depth)
            self.edits.wrap(ls[0], ls[1], ls[2], "p4_h2g(", ")", depth + 1)
            self.edits.wrap(rs[0], rs[1], rs[2], "(u32)(", ")", depth + 1)
        self.census["guest-sum"] += 1

    def v_CompoundAssignOperator(self, x, depth, parent):
        lhs = x["inner"][0]
        if x.get("opcode") in ("+=", "-=") and len(x["inner"]) == 2:
            self.signed_offset(x, lhs, x["inner"][1], depth, compound=True)
        if x.get("opcode") == "+=" and len(x["inner"]) == 2:
            self.guest_sum(x, lhs, x["inner"][1], depth, compound=True)
        if self.is_gp_lvalue(lhs):
            if x.get("opcode") in ("+=", "-="):
                self.rmw_pointer(x, lhs, x["opcode"][0], x["inner"][1], depth)
            else:
                self.unsup(x, "compound-assign-on-guest-pointer")

    def rmw_pointer(self, x, lv, op, rhs, depth, postfix=False):
        """`lv++`, `--lv`, `lv += n` on a pointer in guest storage: a statement expression that translates the slot, does
        the arithmetic on the host pointer and stores the guest address back. The value is the old pointer for a postfix
        operator and the new one otherwise, as in C."""
        c = self.clq(self.qt(lv))
        t = "u32 *" if c["levels"] >= 2 else self.qt(lv)       # a pointer to a guest slot steps by 4, as a PS2 pointer does
        sp, lsp = span(x), span(lv)
        if sp and os.path.abspath(sp[0]) == self.ifile and \
                not os.path.abspath(self.origin_of(sp[0], sp[1])[0]).startswith(self.root + os.sep):
            # a system or port header's own inline code (macOS's __sputc in <_stdio.h> steps FILE's buffer pointer): nothing outside the
            # mirror is rewritten, and its structs are the host's
            return
        if (c["fn"] and c["levels"] == 1) or c["levels"] < 1 or not sp or not lsp or not self.mine(sp[0], sp[1]):
            self.unsup(x, "pointer-rmw-shape")
            return
        f, text = sp[0], self.src(sp[0])
        head = "({ u32 *__p4s = &("
        if rhs is None:                                  # ++ / -- : the amount is 1
            amount = "1"
            if postfix:
                self.edits.wrap(f, lsp[1], sp[2], head, "})", depth)
                self.edits.replace(f, lsp[2], sp[2], f"); {t} __p4o = ({t})p4_g2h(*__p4s); *__p4s = p4_h2g(__p4o {op} {amount}); __p4o; ")
            else:
                self.edits.replace(f, sp[1], lsp[1], head)
                self.edits.wrap(f, lsp[1], lsp[2], "", f"); {t} __p4n = ({t})p4_g2h(*__p4s) {op} {amount}; *__p4s = p4_h2g(__p4n); __p4n; }})", depth)
        else:
            rsp = span(rhs)
            if not rsp:
                self.unsup(x, "pointer-rmw-shape")
                return
            self.edits.wrap(f, lsp[1], rsp[2], head, f"); *__p4s = p4_h2g(__p4n); __p4n; }})", depth)
            self.edits.replace(f, lsp[2], rsp[1], f"); {t} __p4n = ({t})p4_g2h(*__p4s) {op} (")
        self.census["pointer-rmw"] += 1

    def v_UnaryExprOrTypeTraitExpr(self, x, depth, parent):
        if x.get("name") == "sizeof" and "argType" in x and not x.get("inner"):
            c = self.clq(x["argType"]["qualType"])
            if c["ok"] and not c["dims"] and (c["levels"] >= 1 or c["base"] in WORD_BASES):
                self.census["sizeof-pointer"] += 1
                sp_ = span(x)
                if sp_ and self.mine(sp_[0], sp_[1]):
                    self.edits.replace(sp_[0], sp_[1], sp_[2], "4")

    def v_InitListExpr(self, x, depth, parent):
        pass

    # ---- ABI census: what the rewriter cannot see from one unit
    FLOATS = {"f32", "f64", "float", "double", "Float32", "Float64", "RwReal", "long double"}

    def pclass(self, qt):
        c = self.clq(qt)
        if not c["ok"]:
            return "ptr" if "*" in qt else "other"
        if c["levels"] >= 1 or c["dims"]:
            return "ptr"
        if c["base"] in self.FLOATS:
            return "float"
        if c["base"].startswith(("struct ", "union ")):
            return "other"
        return "int"

    @staticmethod
    def split_params(ty):
        """parameter type strings of a function type `R (A, B)`; None when it has no prototype (`R ()`)"""
        s = ty.strip()
        if not s.endswith(")"):
            return None
        depth, i = 0, len(s) - 1
        while i >= 0:
            if s[i] == ")":
                depth += 1
            elif s[i] == "(":
                depth -= 1
                if depth == 0:
                    break
            i -= 1
        inner = s[i + 1:-1].strip()
        if inner == "":
            return None
        if inner == "void":
            return []
        out, depth, cur = [], 0, ""
        for ch in inner:
            if ch in "([":
                depth += 1
            elif ch in ")]":
                depth -= 1
            if ch == "," and depth == 0:
                out.append(cur.strip())
                cur = ""
            else:
                cur += ch
        out.append(cur.strip())
        return out

    def builtin_text(self, qt):
        """a spelling of an integer or float type that is valid anywhere: typedef names resolved, enums as int"""
        s = re.sub(r"\b(const|volatile|restrict|__restrict|_Nonnull|_Nullable|_Null_unspecified)\b", "", qt).strip()
        for _ in range(8):
            if re.fullmatch(r"[A-Za-z_]\w*", s) and s in self.types.typedefs:
                s = re.sub(r"\b(const|volatile)\b", "", self.types.typedefs[s]).strip()
            else:
                break
        # (a host header's own typedef, `__size_t`, is not always in the AST: the LP64 values)
        s = {"__size_t": "unsigned long", "__ssize_t": "long"}.get(s, s)
        return "int" if s.startswith("enum ") else s

    INT_SIZES = {"char": 1, "signed char": 1, "unsigned char": 1, "_Bool": 1, "bool": 1, "short": 2, "signed short": 2, "unsigned short": 2,
                 "short int": 2, "unsigned short int": 2, "int": 4, "signed int": 4, "unsigned int": 4, "unsigned": 4, "signed": 4}

    def int_size(self, qt):
        """the size in bytes of an integer type below or at 32 bits, by its (typedef-resolved) spelling; None otherwise"""
        return self.INT_SIZES.get(self.builtin_text(qt))

    SCALAR_SIZES = {"char": 1, "signed char": 1, "unsigned char": 1, "_Bool": 1, "bool": 1, "short": 2, "signed short": 2, "unsigned short": 2,
                    "short int": 2, "unsigned short int": 2, "int": 4, "signed int": 4, "unsigned int": 4, "unsigned": 4, "signed": 4,
                    "long long": 8, "unsigned long long": 8, "long long int": 8, "unsigned long long int": 8, "float": 4, "double": 8}

    def complete_record(self, r):
        """the RecordDecl with the body for a declaration found first (`typedef struct S S;` names the struct before it is defined)"""
        if r.get("completeDefinition") or not r.get("name"):
            return r if r.get("completeDefinition") else None
        for o in self.decl_by_id.values():
            if o.get("kind") == "RecordDecl" and o.get("name") == r["name"] and o.get("completeDefinition") and o.get("tagUsed") == r.get("tagUsed"):
                return o
        return None

    def find_record(self, node):
        """the complete RecordDecl a by-value parameter (or field) of record type names, through typedefs; None for anything else"""
        qt = (node.get("type") or {}).get("qualType", "")
        if "*" in qt or "[" in qt or "(" in qt:
            return None
        seen = 0
        cur = self.decl_by_id.get(node.get("type", {}).get("typeAliasDeclId", ""))
        if cur is None:
            cur = self.decl_by_id.get(self.typedef_ids.get(re.sub(r"\b(const|volatile)\b", "", qt).strip(), ""))
        stack = [cur] if cur else []
        while stack and seen < 64:
            n = stack.pop()
            seen += 1
            if not isinstance(n, dict):
                continue
            dd = n.get("decl")
            if n.get("kind") == "RecordType" and dd and dd.get("id") in self.decl_by_id:
                return self.complete_record(self.decl_by_id[dd["id"]])
            if n.get("kind") == "TypedefType" and dd and dd.get("id") in self.decl_by_id:
                stack.append(self.decl_by_id[dd["id"]])
            og = n.get("ownedTagDecl")
            if og and og.get("id") in self.decl_by_id and self.decl_by_id[og["id"]].get("kind") == "RecordDecl":
                return self.complete_record(self.decl_by_id[og["id"]])
            stack.extend(c for c in n.get("inner", ()) if isinstance(c, dict))
        m = re.fullmatch(r"(?:const\s+)?(struct|union)\s+(\w+)", qt.strip())
        if m:
            for r in self.decl_by_id.values():
                if r.get("kind") == "RecordDecl" and r.get("name") == m.group(2) and r.get("completeDefinition") and r.get("tagUsed") == m.group(1):
                    return r
        return None

    def small_record_size(self, node):
        """the size of a by-value struct argument of at most 8 bytes whose fields are integers and floats (what a caller whose declaration says
        `s64`, or a struct of other field types, passes where the definition takes the packed integer); None for any other expression"""
        r = self.find_record(node)
        if r is None or r.get("tagUsed") != "struct":
            return None
        off = 0
        for f in r.get("inner", ()):
            if not isinstance(f, dict) or f.get("kind") != "FieldDecl":
                continue
            if f.get("isBitfield"):
                return None
            ft = (f.get("type") or {}).get("qualType", "")
            m = re.fullmatch(r"(.*?)\s*((?:\[\d+\])*)", ft.strip())
            n = 1
            for d_ in re.findall(r"\[(\d+)\]", m.group(2)):
                n *= int(d_)
            sz = self.SCALAR_SIZES.get(self.builtin_text(m.group(1).strip()))
            if sz is None:
                return None
            if off % sz:
                off += sz - off % sz
            off += sz * n
        return off if 0 < off <= 8 else None

    def float_record(self, node):
        """(size, text) of a by-value parameter whose type is a struct of at most 8 bytes made only of floats (and float arrays): the System V and
        AAPCS64 conventions pass it in floating-point registers, where an `s64` of the same bytes (the m2c declaration of the same parameter) is in
        an integer register. None for any other parameter."""
        r = self.find_record(node)
        if r is None or r.get("tagUsed") != "struct":
            return None
        off, flo = 0, True
        for f in r.get("inner", ()):
            if not isinstance(f, dict) or f.get("kind") != "FieldDecl":
                continue
            if f.get("isBitfield"):
                return None
            ft = (f.get("type") or {}).get("qualType", "")
            m = re.fullmatch(r"(.*?)\s*((?:\[\d+\])*)", ft.strip())
            base, dims = m.group(1).strip(), m.group(2)
            n = 1
            for d_ in re.findall(r"\[(\d+)\]", dims):
                n *= int(d_)
            bt = self.builtin_text(base)
            if bt != "float":
                return None
            off += 4 * n
        if off == 0 or off > 8:
            return None
        return off, (node.get("type") or {}).get("qualType", "").strip()

    def calls_through_params(self, fn):
        """parameter id -> register signature letters of the first call `param(args)` in the body of `fn`, from the classes of the arguments and the result"""
        out = {}
        stack = [fn]
        while stack:
            n = stack.pop()
            if n.get("kind") == "CallExpr" and n.get("inner"):
                c = n["inner"][0]
                while c.get("kind") in ("ParenExpr", "ImplicitCastExpr") and c.get("inner"):
                    c = c["inner"][0]
                rd = c.get("referencedDecl") if c.get("kind") == "DeclRefExpr" else None
                if rd and rd.get("kind") == "ParmVarDecl" and rd.get("id") not in out:
                    args = n["inner"][1:]
                    letters = ""
                    ok = len(args) <= 6
                    rt = self.qt(n).strip()
                    if rt == "void":
                        letters = "v"
                    else:
                        cl = self.pclass(rt)
                        letters = {"ptr": "p", "int": "i", "float": "x"}.get(cl, "")
                        ok = ok and bool(letters)
                    for a in args:
                        cl = self.pclass(self.qt(a))
                        ch = {"ptr": "p", "int": "i", "float": "x"}.get(cl)
                        if ch is None:
                            ok = False
                            break
                        ac = self.clq(self.qt(a))
                        if ch == "p" and ac["ok"] and ac["fn"] and ac["levels"] == 1:
                            ch = "f"
                        letters += ch
                    if ok:
                        out[rd["id"]] = letters
            stack.extend(c for c in n.get("inner", ()) if isinstance(c, dict))
        return out

    def abi_of(self, x):
        """the register ABI of a function definition: {"ret": cls|None, "rt": C text, "params": [{"cls","ct"}], "skip"}"""
        qt = self.qt(x)
        rt = ret_text(qt)
        ps = [c for c in x.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "ParmVarDecl"]
        skip = rt is None
        va = None
        if "..." in qt:
            # a variadic definition: the arguments past the named ones are read by va_arg. All integer reads (m2c's `va_arg(ap, s32)`:
            # guest addresses) means a pointer passed there must be converted; any other mix (printf's %s) is left alone
            kinds, stack = set(), list(x.get("inner", ()))
            while stack:
                n = stack.pop()
                if isinstance(n, dict):
                    if n.get("kind") == "VAArgExpr":
                        kinds.add(self.pclass(self.qt(n)))
                    stack.extend(n.get("inner", ()))
            # (the fixed parameters of a printf-like function are adapted as usual: `printf((s32)fmt, ...)` passes the format as a pointer;
            # the variable part is left as the caller wrote it)
            va = "int" if kinds == {"int"} else "other"

        def abi_text(text):
            # The table is also consumed by another host ABI. Do not flatten a
            # fixed int64_t to LP64 `long`, which becomes 32 bits on LLP64.
            fixed = {"int64_t": "long long", "uint64_t": "unsigned long long"}
            host = {"size_t": "size_t", "ssize_t": "intptr_t", "ptrdiff_t": "ptrdiff_t",
                    "intptr_t": "intptr_t", "uintptr_t": "uintptr_t"}
            plain, seen = text.strip(), set()
            while plain not in seen:
                seen.add(plain)
                if plain in fixed:
                    return fixed[plain]
                if plain in host:
                    return host[plain]
                plain = self.types.typedefs.get(plain, "")
                if not plain:
                    break
            return self.builtin_text(text)

        def one(tp):
            q = tp.get("desugaredQualType") or tp.get("qualType", "")
            cl = self.pclass(tp.get("qualType", ""))
            if cl == "ptr":
                c = self.clq(tp.get("qualType", ""))
                isfn = bool(c["ok"] and c["fn"] and c["levels"] == 1)
                out = {"cls": "ptr", "ct": "void *", "fn": isfn}
                if isfn:
                    fsig = self.sig_string(tp.get("qualType", ""))      # how the callee calls this callback
                    if fsig:
                        out["fsig"] = fsig
                return out
            if cl in ("int", "float"):
                return {"cls": cl, "ct": abi_text(tp.get("qualType") or q)}
            return None
        body_calls = None
        params = []
        for c in ps:
            row = one(c.get("type", {}))
            if row is not None and row.get("fn") and "fsig" not in row and c.get("id"):
                # an unprototyped function-pointer parameter (`s32 (*)()`): how the body calls it says what the callee passes it
                if body_calls is None:
                    body_calls = self.calls_through_params(x)
                fs = body_calls.get(c["id"])
                if fs:
                    row["fsig"] = fs
            fr = self.float_record(c) if row is None else None
            if fr is not None:
                row = {"cls": "int", "ct": "unsigned long long", "sx": fr[1]}
            params.append(row)
        if any(p_ is None for p_ in params):
            skip = True
        rcls, rct = None, "void"
        if rt and rt.strip() != "void":
            rcls = self.pclass(rt)
            rct = "void *" if rcls == "ptr" else abi_text(rt)
            if rcls == "other":
                skip = True
        out = {"ret": rcls, "rt": rct, "params": params, "skip": skip}
        if va:
            out["va"] = va
        return out

    cur_func_pos = None

    def v_FunctionDecl(self, x, depth, parent):
        if any(isinstance(c, dict) and c.get("kind") == "CompoundStmt" for c in x.get("inner", ())):
            sp_ = span(x)
            self.cur_func_pos = (sp_[0], sp_[1]) if sp_ else None
        if any(isinstance(c, dict) and c.get("kind") == "CompoundStmt" for c in x.get("inner", ())):
            ps = [c for c in x["inner"] if isinstance(c, dict) and c.get("kind") == "ParmVarDecl"]
            self.defs[x["name"]] = [self.pclass(self.qt(c)) for c in ps]
            if x.get("storageClass") != "static":
                self.abi_defs[x["name"]] = self.abi_of(x)
        # a by-value struct of floats (at most 8 bytes) is an integer parameter on every target: the m2c callers declare the same two floats as an
        # `s64`, and the System V and AAPCS64 conventions pass the struct in floating-point registers. The parameter becomes an `unsigned long long`
        # (the ABI row says so) and a definition copies it into a local of the struct's type; the calls pack their struct arguments (P4_SX)
        if self.abi:
            row = self.abi.get(x["name"])
            ps_ = [c for c in x.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "ParmVarDecl"]
            bsp_ = span(body) if (body := next((c for c in x.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "CompoundStmt"), None)) else None
            if row:
                for i_, c in enumerate(ps_):
                    if i_ >= len(row["params"]) or not row["params"][i_] or "sx" not in row["params"][i_] or self.float_record(c) is None:
                        continue
                    sp_c, ns_c = span(c), name_span(c)
                    if not sp_c or not self.mine(sp_c[0], sp_c[1]):
                        continue
                    nm = c.get("name")
                    if bsp_ is not None and nm and ns_c:
                        ty = re.sub(r"\b(const|volatile)\b", "", (c.get("type") or {}).get("qualType", "")).strip()
                        self.edits.replace(sp_c[0], sp_c[1], ns_c[2], f"unsigned long long {nm}_p4sx")
                        self.edits.insert(sp_c[0], bsp_[1] + 1, f" {ty} {nm}; __builtin_memcpy(&{nm}, &{nm}_p4sx, sizeof {nm});")
                    elif nm and ns_c:
                        self.edits.replace(sp_c[0], sp_c[1], ns_c[2], f"unsigned long long {nm}")
                    else:
                        self.edits.replace(sp_c[0], sp_c[1], sp_c[2], "unsigned long long")
                    self.census["struct-param-int"] += 1
        # a pointer parameter whose address is taken: the parameter keeps its host ABI under a new name and the body starts
        # with a guest-storage copy under the old one (the address of the copy is what the code takes)
        body = next((c for c in x.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "CompoundStmt"), None)
        if body is not None:
            for c in x.get("inner", ()):
                if isinstance(c, dict) and c.get("kind") == "ParmVarDecl" and c["id"] in self.addr_taken and c.get("name"):
                    cl = self.clq(self.qt(c))
                    ns, bsp = name_span(c), span(body)
                    if cl["ok"] and cl["levels"] == 1 and ns and bsp and self.mine(ns[0], ns[1]):
                        self.edits.replace(ns[0], ns[1], ns[2], c["name"] + "_p4arg")
                        # Leading assertions inspect the original parameter's
                        # ABI type, not the guest-storage copy that shadows it.
                        start = bsp[1] + 1
                        assertions = re.match(r"(?:\s*_Static_assert\([^\n]*\);)+", self.src(ns[0])[start:])
                        if assertions:
                            for ref in re.finditer(r"\b" + re.escape(c["name"]) + r"\b", assertions.group()):
                                self.edits.replace(ns[0], start + ref.start(), start + ref.end(), c["name"] + "_p4arg")
                            stack = [body]
                            while stack:
                                node = stack.pop()
                                if node.get("kind") == "ImplicitCastExpr" and node.get("castKind") == "LValueToRValue":
                                    child = self.strip(node["inner"][0])
                                    nspan = span(node)
                                    if (child.get("kind") == "DeclRefExpr" and
                                            child.get("referencedDecl", {}).get("id") == c["id"] and
                                            nspan and nspan[0] == ns[0] and
                                            start <= nspan[1] < start + assertions.end()):
                                        self.skip.add(node["id"])
                                stack.extend(child for child in node.get("inner", ()) if isinstance(child, dict))
                        self.edits.insert(ns[0], start, f" u32 {c['name']} = " + (
                            "p4_fn_h2g" if cl["fn"] else "p4_h2g") + f"({c['name']}_p4arg);")
                        self.census["addr-taken-param"] += 1
        # a function that returns a pointer to a pointer returns a host pointer to a guest slot: `T **` is `u32 *`
        rt = ret_text(self.qt(x))
        if rt:
            c = self.clq(rt)
            if c["ok"] and c["levels"] >= 2:
                sp, ns = span(x), name_span(x)
                if sp and ns and ns[0] == sp[0] and self.mine(sp[0], sp[1]) and sp[1] < ns[1]:
                    text = self.src(sp[0])
                    prefix = text[sp[1]:ns[1]]
                    sc = " ".join(w for w in KEYWORDS_STORAGE if re.search(r"\b" + w + r"\b", prefix))
                    if "(" in prefix:
                        # `s32 (**f(PARAMS))(ARGS)`: the parenthesised declarator goes with the type, `u32 *f(PARAMS)` is left
                        def balanced(i):
                            d = 0
                            for j in range(i, len(text)):
                                d += (text[j] == "(") - (text[j] == ")")
                                if d == 0:
                                    return j + 1
                            return None
                        pe = balanced(ns[2]) if text[ns[2]:ns[2] + 1] == "(" else None
                        m_ = re.compile(r"\s*\)\s*\(").match(text, pe) if pe else None
                        pe2 = balanced(m_.end() - 1) if m_ else None
                        if not pe2:
                            self.unsup(x, "return-type-declarator")
                            return
                        self.edits.replace(sp[0], pe, pe2, "")
                    self.edits.replace(sp[0], sp[1], ns[1], (sc + " " if sc else "") + "u32 *")
                    self.census["return-ptr2"] += 1

    def call_sig(self, x):
        """the register signature of a call site (guest.h, P4_SIG_KNOWN): None when a struct or more than six arguments are involved"""
        args = x["inner"][1:]
        if len(args) > 6:
            return None
        sig = 0
        for i, a in enumerate(args):
            cl = self.pclass(self.qt(a))
            if cl == "ptr":
                sig |= 1 << (1 + i)
                ac = self.clq(self.qt(a))
                if ac["ok"] and ac["fn"] and ac["levels"] == 1:
                    sig |= 1 << (12 + i)
            elif cl == "float":
                sig |= 1 << (18 + i)
            elif cl != "int":
                return None
        rt = self.qt(x).strip()
        if rt != "void":
            cl = self.pclass(rt)
            if cl == "ptr":
                sig |= 1
            elif cl == "float":
                sig |= 1 << 24
            elif cl != "int":
                return None
        return 0x80000000 | len(args) << 8 | sig

    def v_CallExpr(self, x, depth, parent):
        callee = x["inner"][0]
        decl_ps = self.split_params(self.qt(callee))
        for i_, a_ in enumerate(x["inner"][1:]):
            pt = decl_ps[i_] if decl_ps and i_ < len(decl_ps) else None
            if pt is not None and "*" in pt and "va_list" not in pt:
                self.va_list_address(a_, depth)
        n = callee
        chain = []
        while True:
            chain.append(n)
            if n.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr") and n.get("inner"):
                n = n["inner"][0]
            else:
                break
        callee = x["inner"][0]
        while callee.get("kind") in ("ParenExpr", "ImplicitCastExpr") and callee.get("inner"):
            callee = callee["inner"][0]
        if callee.get("kind") != "DeclRefExpr" or callee.get("referencedDecl", {}).get("kind") != "FunctionDecl":
            sig = self.call_sig(x)
            if sig is not None:
                for n in chain:
                    self.callee_sig[n["id"]] = sig
            return
        classes = []
        for a in x["inner"][1:]:
            while a.get("kind") in ("ParenExpr", "ImplicitCastExpr") and a.get("inner"):
                a = a["inner"][0]
            classes.append(self.pclass(self.qt(a)))
        self.pack_struct_args(x, callee, depth)
        key = (callee["referencedDecl"]["name"], self.qt(callee), tuple(classes))
        if key not in self.calls:
            self.calls[key] = self.site() or ("?", 0)
        # a value read from a call of a function the mirror defines as void (m2c leaves the result in $v0): what the register holds is up to the
        # compiler, which differs between targets. P4_PORT64_VOIDUSE=<file> lists those calls (tools/port64/unit64.py reports nothing else about them)
        vf = os.environ.get("P4_PORT64_VOIDUSE")
        pk = parent.get("kind") if parent is not None else None
        used = pk not in ("CompoundStmt", "ForStmt", "DoStmt", "CaseStmt", "DefaultStmt", "LabelStmt", "IfStmt", "WhileStmt", "SwitchStmt") or (
            pk in ("IfStmt", "WhileStmt", "SwitchStmt") and parent.get("inner") and parent["inner"][0].get("id") == x.get("id"))
        if vf and parent is not None and used:
            rd = self.abi.get(callee["referencedDecl"]["name"]) if self.abi else None
            rt = self.qt(x).strip()
            if rd is not None and rd.get("ret") is None and not rd.get("skip") and rt != "void":
                s = self.site() or ("?", 0)
                with open(vf, "a") as f:
                    f.write(f"{callee['referencedDecl']['name']}\t{s[0]}:{s[1]}\t{parent.get('kind')}\n")
        self.adapt_call(x, callee, depth)

    def names_function(self, n):
        """an expression that is a function (a function name, possibly cast and parenthesised): what it converts to is a code address"""
        while isinstance(n, dict) and n.get("kind") in ("ParenExpr", "ImplicitCastExpr", "CStyleCastExpr") and n.get("inner"):
            n = n["inner"][0]
        return isinstance(n, dict) and n.get("kind") == "DeclRefExpr" and (n.get("referencedDecl") or {}).get("kind") == "FunctionDecl"

    def pack_struct_args(self, x, callee, depth):
        """a by-value struct of floats (at most 8 bytes) passed where the callee's definition takes an integer (see v_FunctionDecl): its bytes in an
        integer"""
        decl = self.decl_by_id.get(callee["referencedDecl"]["id"], {})
        d = self.abi.get(self.linked_name(decl, callee["referencedDecl"]["name"])) if self.abi else None
        if not d or d.get("va"):
            return
        args = x["inner"][1:]
        for i, a in enumerate(args):
            if i >= len(d["params"]) or not d["params"][i] or d["params"][i]["cls"] != "int":
                continue
            n = a
            while n.get("kind") in ("ParenExpr", "ImplicitCastExpr") and n.get("inner"):
                n = n["inner"][0]
            if self.pclass(self.qt(n)) != "other" or self.small_record_size(n) is None:
                continue
            sp = span(a)
            if sp and self.mine(sp[0], sp[1]):
                self.edits.wrap(sp[0], sp[1], sp[2], "({ __auto_type p4sx_v = (",
                                "); unsigned long long p4sx_r = 0; __builtin_memcpy(&p4sx_r, &p4sx_v, sizeof p4sx_v); p4sx_r; })", depth + 1)
                self.census["struct-arg-packed"] += 1

    def printf_arguments(self, args, depth):
        """One EE GP word per masked integer, independent of host int/long width.

        Casting the promoted expression directly preserves its signedness:
        negative signed32 becomes a sign-extended word; unsigned32 zero-extends.
        Native pointers and real doubles keep their original varargs classes.
        The narrow i386 pass uses this same operation.
        """
        mask = 0
        for k, a in enumerate(args[:32]):
            if self.pclass(self.qt(a)) == "int":
                mask |= 1 << k
                self.wrap_node(a, "((unsigned long long)(", "))", depth + 1)
                self.census["printf-gp-word"] += 1
        asp = span(args[0])
        if asp and self.mine(asp[0], asp[1]):
            self.edits.insert_before(asp[0], asp[1], f"0x{mask:x}u, ")
            self.census["printf-masked"] += 1

    def adapt_call(self, x, callee, depth):
        """The callee's definition decides which arguments are host pointers and which are guest addresses; the
        declaration the caller saw (an m2c `int f();`, a prototype with `u32`) may say otherwise. Convert the argument, and
        the result, where the two disagree about pointer versus integer."""
        cname = callee["referencedDecl"]["name"]
        decl = self.decl_by_id.get(callee["referencedDecl"]["id"], {})
        # `extern T p4_name(...) __asm__("func_003df890")` names a function by another symbol: the symbol is what has an ABI
        name = self.linked_name(decl, cname)
        d = self.abi.get(name)
        # a function defined in this unit is called through the declaration the compiler checked against the definition; only a
        # call through another name for the same symbol (an asm label) can disagree with it
        # (a call through an unprototyped declaration, `void f();`, is not checked against the definition: it is adapted like any other)
        prototyped = self.split_params(self.qt(callee)) is not None
        # a definition with a struct parameter by value has no complete row (`skip`): its other parameters are still pointer or integer, and the
        # struct keeps the caller's own type in the call's declaration below
        partial = bool(d and d.get("skip") and d["ret"] != "other" and not d.get("va") and prototyped
                       and any(p_ is None for p_ in d["params"]) and all(p_ is None or p_["cls"] != "other" for p_ in d["params"]))
        if not d or (d.get("skip") and not partial) or (cname == name and name in self.local_defs and prototyped) or (cname in self.local_defs and prototyped):
            return
        args = x["inner"][1:]
        params = [p_ or {"cls": "other", "ct": None} for p_ in d["params"]]
        todo = []
        for i in range(min(len(args), len(params))):
            have, want = self.pclass(self.qt(args[i])), params[i]["cls"]
            if (have == "ptr") != (want == "ptr") and "float" not in (have, want) and "other" not in (have, want):
                ac = self.clq(self.qt(args[i]))
                fnarg = params[i].get("fn", False) if want == "ptr" else bool(ac["ok"] and ac["fn"] and ac["levels"] == 1)
                if want == "ptr" and not fnarg and self.names_function(args[i]):
                    fnarg = True        # a function passed as an integer (`(s32)func`) to a `void *` parameter: a code address, not a data address
                todo.append((args[i], want == "ptr", fnarg, params[i].get("fsig") if want == "ptr" else None))
        # a callback passed where the callee calls it by another register ABI than the function has (m2c integers vs RenderWare pointers)
        for i in range(min(len(args), len(params))):
            fs = params[i].get("fsig")
            own = self.fn_sig_for(args[i]) if fs else None
            if own:
                m = min(len(own), len(fs))
                if [letter_class(ch) for ch in own][:m] != [letter_class(ch) for ch in fs][:m]:
                    self.census["fn-arg-adapted"] += 1
                    self.wrap_node(args[i], "((void *)p4_fn_adapt((const void *)(", f'), "{own}", "{fs}"))', depth + 1)
                    self.fn_adapted.add(args[i]["id"])
        if d.get("va") == "int":
            for a in args[len(params):]:
                ac = self.clq(self.qt(a))
                if self.pclass(self.qt(a)) == "ptr":
                    todo.append((a, False, bool(ac["ok"] and ac["fn"] and ac["levels"] == 1), None))
        # printf-family: the runtime formats guest strings; it learns which variable arguments are integer-class from a leading mask
        pf = name in PRINTF_FAMILY and name not in self.local_defs and len(args) > len(params)
        rt = self.qt(x)
        have, want = (self.pclass(rt) if rt.strip() != "void" else None), d["ret"]
        ret_fix = bool(have and want and (have == "ptr") != (want == "ptr") and "float" not in (have, want)
                       and "other" not in (have, want))
        # A parameter the caller's declaration makes 8 or 16 bits wide and the definition 32 (m2c declares `s16`, the definition takes `s32`):
        # whether the upper bits of the register are defined depends on the target's convention (clang's x86-64 System V callers extend, Windows
        # x64 callers do not), so the call goes through the definition's types and the compiler converts. Likewise a result the definition makes
        # narrow and the caller wide.
        decl_params = self.split_params(self.qt(callee))
        narrow = False
        if decl_params is not None:
            for i in range(min(len(args), len(params), len(decl_params))):
                dsz, wsz = self.int_size(decl_params[i]), params[i]["cls"] == "int" and self.int_size(params[i]["ct"])
                if dsz and wsz and dsz < wsz:
                    narrow = True
        ret_narrow = False
        if not ret_fix and have == "int" and want == "int":
            dsz, wsz = self.int_size(d["rt"]), self.int_size(rt)
            ret_narrow = bool(dsz and wsz and dsz < wsz)
        # Apple's arm64 ABI passes a variadic function's variable arguments on the stack and its fixed ones in registers: a call through a
        # declaration that disagrees with the definition about `...` (an m2c `int f();` for a printf-like definition, a `...` prototype for a
        # fixed one) puts the arguments where the callee does not read them, so it goes through the definition's prototype (elsewhere the two
        # conventions put every argument in the same place)
        va_fix = self.variadic_on_stack and bool(d.get("va")) != ("..." in self.qt(callee))
        if va_fix and any(p_["ct"] is None for p_ in params[:len(args)]):
            self.census["abi-variadic-unadapted"] += 1
            va_fix = False
        if not todo and not ret_fix and not pf and not narrow and not ret_narrow and not va_fix:
            return
        # the call goes through a declaration with the definition's types, under a private name bound to the same symbol
        # (an asm label): the converted arguments and the result are then what the callee takes and gives, and nothing
        # depends on how, or whether, the function was declared where it is called
        sp = span(callee)
        if not sp or not self.mine(sp[0], sp[1]) or self.cur_func_pos is None:
            return
        n_args = len(args)
        if partial:
            for i_ in range(min(n_args, len(params))):
                if params[i_]["ct"] is None:
                    params = list(params)
                    params[i_] = dict(params[i_], ct=decl_params[i_] if i_ < len(decl_params) else None)
            if any(p_["ct"] is None for p_ in params[:n_args]):
                return
            self.census["abi-partial-row"] += 1
        if params[:n_args]:
            proto = ", ".join([p_["ct"] for p_ in params[:n_args]] + (["..."] if n_args > len(params) or d.get("va") else []))
        else:
            proto = "" if n_args else "void"
        # a definition that returns nothing, called as if it returned a value (the value is in $v0): keep the caller's type
        # the result type is the caller's own unless the definition's pointer/integer class has to replace it
        rct = d["rt"] if (ret_fix or ret_narrow) else ("void" if rt.strip() == "void" else rt)
        private = f"p4abi_{cname}_{n_args}"
        if (private, self.cur_func_pos[0]) not in self.abi_decls:
            self.abi_decls.add((private, self.cur_func_pos[0]))
            self.edits.insert_before(self.cur_func_pos[0], self.cur_func_pos[1],
                                     f'extern {rct} {private}({proto}) __asm__("{self.sym_prefix}{("p4pf_" + PRINTF_FAMILY[name]) if pf else name}");\n')
        self.edits.replace(sp[0], sp[1], sp[2], private)
        if pf:
            self.printf_arguments(args[len(params):], depth)
        for a, to_ptr, fnarg, fsig in todo:
            if to_ptr and fnarg and fsig:
                # a guest code address handed to a callee that calls it by `fsig` (RenderWare's callback typedef): the table adapts it to
                # the register signature the function was registered with
                self.wrap_node(a, "((void *)p4_fn_g2h_sig((u32)(", f"), 0x{self.sig_numeric(fsig):x}u))", depth + 1)
            elif to_ptr:
                self.wrap_node(a, "((void *)" + ("p4_fn_g2h" if fnarg else "p4_g2h") + "((u32)(", ")))", depth + 1)
            else:
                self.wrap_node(a, "((u32)" + ("p4_fn_h2g" if fnarg else "p4_h2g") + "((", ")))", depth + 1)
            self.census["abi-arg-adapted"] += 1
        if ret_fix:
            if want == "ptr":
                self.wrap_node(x, "((u32)p4_h2g((", ")))", depth)
            else:
                self.wrap_node(x, f"(({rt})p4_g2h((u32)(", ")))", depth)
            self.census["abi-ret-adapted"] += 1
        if narrow or ret_narrow:
            self.census["abi-narrow-adapted"] += 1
        if va_fix:
            self.census["abi-variadic-adapted"] += 1
        self.census["abi-call-adapted"] += 1

    def v_UnaryOperator(self, x, depth, parent):
        if x.get("opcode") in ("++", "--") and self.is_gp_lvalue(x["inner"][0]):
            self.rmw_pointer(x, x["inner"][0], x["opcode"][0], None, depth, bool(x.get("isPostfix")))
        if x.get("opcode") == "&":
            t = self.strip(x["inner"][0])
            while t.get("kind") in ("MemberExpr", "ArraySubscriptExpr") and t.get("inner"):
                t = self.strip(t["inner"][0])
            if t.get("kind") == "DeclRefExpr":
                rd = t.get("referencedDecl", {})
                if rd.get("kind") == "VarDecl":
                    d = self.decl_by_id.get(rd["id"], rd)
                    if not d.get("_global") and d.get("storageClass") not in ("static", "extern") and self.abs_addr(rd) is None:
                        self.census["addr-of-local"] += 1
                elif rd.get("kind") == "FunctionDecl":
                    self.fn_addr_taken.add(rd["name"])

    # ---- layout asserts
    def record_asserts(self):
        for d in self.pending_records:
            self.cur = d
            sp_ = span(d)
            if not sp_:
                continue
            f, b, e = sp_
            if not self.mine(f, b):
                continue
            of, oline, _oc = self.origin_of(f, b)
            key = f"{os.path.relpath(of, self.root)}:{oline}"
            name = d.get("name")
            tag = d.get("tagUsed")
            cands = None
            if name and tag:
                cands = self.layouts.get(f"{tag} {name}")
            if not cands:
                cands = self.layouts.get(f"@{key}")
            if not cands:
                self.census["record-without-oracle"] += 1
                continue
            fnames = [c["name"] for c in d.get("inner", ()) if isinstance(c, dict) and c.get("kind") == "FieldDecl" and c.get("name")]
            lay = next((c for c in cands if [n for _o, n in c["fields"]] == fnames), None)
            if lay is None:
                self.census["record-oracle-ambiguous"] += 1
                continue
            text = self.src(f)
            m = re.compile(r"\s*;?").match(text, e)
            end = e
            # the statement ends at the next ';' (the declaration may carry a typedef name)
            semi = text.find(";", e)
            if semi < 0:
                continue
            tail = text[e:semi]
            tname = None
            if not name:
                mt = re.search(r"\b([A-Za-z_]\w*)\s*$", tail)
                if not mt or "typedef" not in text[max(0, b - 40):b]:
                    self.census["record-anon-without-typedef"] += 1
                    continue
                tname = mt.group(1)
            ty = f"{tag} {name}" if name else tname
            lines = [f"\n_Static_assert(sizeof({ty}) == {lay['size']}, \"layout {ty}\");"]
            for off, fname in lay["fields"]:
                lines.append(f"_Static_assert(__builtin_offsetof({ty}, {fname}) == {off}, \"layout {ty}.{fname}\");")
            self.edits.insert(f, semi + 1, "\n".join(lines))
            self.census["layout-asserts"] += 1 + len(lay["fields"])

    def run(self):
        self.index(self.ast)
        self.prescan(self.ast)
        self.pending_records = []
        self.walk(self.ast)
        for d, init, depth in self.pending_moves:
            self.move_static_init(d, init, depth)
        self.record_asserts()
        for f, code in self.eof_code:
            self.edits.insert(f, len(self.src(f)), code)
        if self.ifile:
            text = self.src(self.ifile)
            for m in re.finditer(r"sizeof\s*\(\s*void\s*\*\s*\)\s*==\s*4\b", text):
                if self.mine(self.ifile, m.start()):       # the decomp's guest-layout checks: pointers in guest memory are 4 bytes
                    self.edits.replace(self.ifile, m.start(), m.end(), "4 == 4")
                    self.census["sizeof-pointer-check"] += 1


# ------------------------------------------------------------------------------------------ CLI
# -fdump-record-layouts-complete dumps each record when it is completed and ignores an `aligned` attribute that a typedef
# adds afterwards (RwMatrix: sizeof(struct Model) is 720 and its dump says 712). The dump without -complete is of the layout
# the compiler actually uses, so the unit is code-generated (-O0, no output) to make it lay out every record the code touches.
LAYOUT_FLAGS = ["-S", "-emit-llvm", "-O0", "-w", "-o", os.devnull, "-Xclang", "-fdump-record-layouts"]


def oracle_layouts(unit, flags, clang="clang", deps=None):
    """The original i386 layout oracle, cached by its exact preprocessed input."""
    def produce():
        with build_cache.timed("clang.layout", source=os.path.abspath(unit)) as trace:
            if symbol_prefix(flags):
                # the Apple oracle (i686-apple-macos): the unit's text with what a Mach-O target rejects removed (macho_text), compiled with
                # the unit's own diagnostic flags (its -Wno-... keep a decomp error that is a warning there from ending the dump); the line
                # markers keep the anonymous records' file:line labels those of the original unit
                with tempfile.TemporaryDirectory(prefix="p4-layout-macho-") as work:
                    ipath = os.path.join(work, os.path.basename(unit) + ".i")
                    preprocess(unit, flags, ipath, clang)
                    macho_text(ipath, symbol_prefix(flags))
                    diagnostics = [f for f in flags if f.startswith(("-W", "-ferror-limit"))]
                    p = subprocess.run([clang] + LAYOUT_FLAGS + stage2_flags(flags) + diagnostics + ["-x", "cpp-output", ipath],
                                       capture_output=True, text=True, errors="replace")
            else:
                p = subprocess.run([clang] + LAYOUT_FLAGS + flags + [unit], capture_output=True, text=True, errors="replace")
            trace["returncode"] = p.returncode
        return json.dumps(parse_layouts(p.stdout + p.stderr), sort_keys=True).encode("utf-8")
    if build_cache.enabled():
        # Only the key uses preprocessing. The producer still compiles the
        # original unit, preserving anonymous record labels and diagnostics.
        with tempfile.TemporaryDirectory(prefix="p4-layout-key-") as work:
            key_source = os.path.join(work, "unit.i")
            preprocess(unit, flags, key_source, clang, deps)
            data = build_cache.run_bytes("layout", {
                "source": os.path.abspath(unit), "preprocessed": build_cache.file_digest(key_source),
                "flags": LAYOUT_FLAGS + flags, "compiler": build_cache.compiler_identity(clang),
                "versions": build_cache.versions(__file__)}, produce)
    else:
        with build_cache.timed("layout"):
            data = produce()
    recs = json.loads(data)
    for variants in recs.values():
        for record in variants:
            record["fields"] = [tuple(field) for field in record["fields"]]
    return recs


def parse_layouts(text):
    recs, cur = {}, None
    for ln in text.splitlines():
        if ln.startswith("*** Dumping AST Record Layout"):
            cur = None
            continue
        m = re.match(r"^\s*0 \| (\S.*)$", ln)
        if m and cur is None:
            cur = {"size": None, "fields": []}
            recs.setdefault(m.group(1), []).append(cur)      # a tag redefined in several blocks keeps every layout
            continue
        if cur is None:
            continue
        m = re.match(r"^\s+\| \[sizeof=(\d+), (?:dsize=\d+, )?align=(\d+)", ln)
        if m:
            cur["size"] = int(m.group(1))
            cur = None
            continue
        m = re.match(r"^\s*(\d+) \|   (\S.*)$", ln)
        if m:
            nm = re.search(r"([A-Za-z_]\w*)(?:\[[^\]]*\])*$", m.group(2))
            if nm:
                cur["fields"].append((int(m.group(1)), nm.group(1)))
    return recs




SIG_CT = {"b": "signed char", "B": "unsigned char", "h": "short", "H": "unsigned short", "i": "int", "q": "long long",
          "f": "float", "d": "double"}


def sigs_abi(path):
    """ABI of the port natives and overrides from tools/mipsdiff/sigs.txt (origin `o`, generated from their definitions)"""
    out = {}
    for ln in open(path):
        if ln.startswith("#") or not ln.strip():
            continue
        f = ln.split()
        if len(f) < 5 or f[4] != "o":
            continue
        name, ret, args = f[1], f[2], f[3]
        skip = "?" in args or ret == "?"
        params = []
        for ch in ("" if args == "-" else args):
            if ch == "p":
                params.append({"cls": "ptr", "ct": "void *"})
            elif ch in SIG_CT:
                params.append({"cls": "float" if ch in "fd" else "int", "ct": SIG_CT[ch]})
            else:
                skip = True
        rcls = None if ret == "v" else ("ptr" if ret == "p" else ("float" if ret in "fd" else "int"))
        rct = "void" if ret == "v" else ("void *" if ret == "p" else SIG_CT.get(ret, "int"))
        out[name] = {"ret": rcls, "rt": rct, "params": params, "skip": skip}
    return out


def merge_abi(per_unit, sigs=None):
    """one ABI per function name: every unit's non-static definitions (equal classes, or the name is dropped as
    ambiguous), then the port natives of sigs.txt for the names no C definition gives. -> (table, ambiguous names)"""
    table, bad = {}, set()
    for defs in per_unit:
        for n, d in defs.items():
            sig = (d["ret"], [(p_["cls"], p_.get("fn", False)) if p_ else None for p_ in d["params"]], d["skip"], d.get("va"))
            if n in table:
                o = table[n]
                if (o["ret"], [(p_["cls"], p_.get("fn", False)) if p_ else None for p_ in o["params"]], o["skip"], o.get("va")) != sig:
                    bad.add(n)
            else:
                table[n] = d
    for n in bad:
        del table[n]
    for n, d in (sigs or {}).items():
        table.setdefault(n, d)
    return table, sorted(bad)


def collect_abi(unit, flags, work, root, clang="clang", deps=None):
    """the ABI of the non-static functions this unit defines (preprocess, parse, read the top-level definitions)"""
    os.makedirs(work, exist_ok=True)
    ipath = os.path.join(work, os.path.basename(unit) + ".abi.i")
    preprocess(unit, flags, ipath, clang, deps)
    try:
        parsed = []
        def produce():
            ast = load_ast(ipath, stage2_flags(flags), clang)
            with build_cache.timed("abi.extract", source=os.path.abspath(unit)):
                rw = Rewriter(ast, root, ifile=ipath)
                rw.index(ast)
                out = {}
                for top in ast.get("inner", ()):
                    if isinstance(top, dict) and top.get("kind") == "FunctionDecl" and top.get("storageClass") != "static" and any(
                            isinstance(c, dict) and c.get("kind") == "CompoundStmt" for c in top.get("inner", ())):
                        sp = span(top)
                        if sp and rw.mine(sp[0], sp[1]):
                            out[top["name"]] = rw.abi_of(top)
            parsed.append(out)
            return json.dumps(out, sort_keys=True).encode("utf-8")
        data = build_cache.run_bytes("abi", lambda: {
            "source": os.path.abspath(unit), "preprocessed": build_cache.file_digest(ipath),
            "flags": flags, "root": os.path.abspath(root), "compiler": build_cache.compiler_identity(clang),
            "versions": build_cache.versions(__file__)}, produce)
        _MAIN[0] = os.path.abspath(ipath)
        return parsed[0] if parsed else json.loads(data)
    finally:
        os.remove(ipath)


def collect_host_abi(headers, flags, work, clang="clang"):
    """the ABI of the host library functions the headers declare (strcpy, memcpy, strlen, ...): the mirror calls some of them directly,
    with whatever its m2c declaration says (`strcpy(s32, s32)`), and the call is adapted to the prototype the library really has"""
    os.makedirs(work, exist_ok=True)
    src = os.path.join(work, "host_headers.c")
    with open(src, "w") as f:
        f.write("".join(f"#include <{h}>\n" for h in headers))
    ast = load_ast(src, flags, clang)
    rw = Rewriter(ast, work, ifile=src)
    rw.index(ast)
    out = {}
    for top in ast.get("inner", ()):
        if isinstance(top, dict) and top.get("kind") == "FunctionDecl" and top.get("storageClass") != "static" and top.get("name") \
                and not top["name"].startswith("__") and not any(isinstance(c, dict) and c.get("kind") == "CompoundStmt" for c in top.get("inner", ())):
            d = rw.abi_of(top)
            if not d["skip"]:
                out[top["name"]] = d
    return out


def va_list_casts(path):
    """AArch64 only: a va_list is a struct there, so clang drops a statement that casts one to a pointer (`(s32 *)args`: the decomp's way of handing the
    argument area to the bound vprintf) from its AST, and no visitor can see it. The cast of a variable declared `va_list` is made a cast of its address
    in the text before the AST is read; on x86_64 and i386 (an array that decays, a pointer) the same text is unchanged."""
    with open(path, "rb") as stream:
        text = stream.read().decode("latin-1")
    names = set(re.findall(r"\bva_list\s+(\w+)\s*;", text))
    if not names:
        return
    pat = re.compile(r"\(\s*([A-Za-z_][\w\s]*?\*+)\s*\)\s*(" + "|".join(sorted(names)) + r")\b")
    new = pat.sub(lambda m: f"({m.group(1)})&{m.group(2)}", text)
    if new != text:
        open(path, "wb").write(new.encode("latin-1"))


def process(unit, flags, work, root, syms=None, oracle_unit=None, oracle_flags=None, clang="clang", abi=None, deps=None):
    """Preprocess `unit`, rewrite it, return (rewriter, preprocessed path, rewritten text). `work` is a directory for the
    preprocessed file; the i386 layouts of `oracle_unit` (the same unit, i386 flags) become the layout assertions."""
    os.makedirs(work, exist_ok=True)
    ipath = os.path.join(work, os.path.basename(unit) + ".i")
    preprocess(unit, flags, ipath, clang, deps)
    return rewrite_preprocessed(ipath, flags, root, syms, oracle_unit, oracle_flags, clang, abi)


def compile_target(flags):
    return next((f.split("=", 1)[1] for f in reversed(flags) if f.startswith("--target=")), "")


def symbol_prefix(flags):
    """The C symbol prefix of the compile target in `flags`: `_` on Apple's Mach-O, none on ELF (the asm labels the rewriter writes
    name symbols, not C names)."""
    return "_" if re.search(r"-apple-|-darwin", compile_target(flags)) else ""


def variadic_on_stack(flags):
    """Apple's arm64 ABI (the compile target in `flags`) passes the variable arguments of a call on the stack, the fixed ones in registers."""
    return bool(re.match(r"(?:arm64|aarch64)\w*-apple-", compile_target(flags)))



# a declaration's asm label spelled for ELF: after a declarator (`)`, `]` or a name), one string literal naming a C identifier; an asm statement
# (`__asm__("sync")` after `;` or `{`) and a label that already carries an underscore (the SDK's own, `__asm("_" "fopen" ...)`) do not match
ASM_LABEL_RE = re.compile(r'([\w\])]\s*)(__asm__|__asm)(\s*\(\s*")([A-Za-z][\w$.]*)("\s*\))')
# an ELF section placement (the decomp's MWCC `.sdata` hint on an extern declaration); Mach-O names a section `segment,section`
ELF_SECTION_RE = re.compile(r'__attribute__\s*\(\(\s*(?:__section__|section)\s*\(\s*"[^",]*"\s*\)\s*\)\)')


def macho_text(path, prefix):
    """Mach-O (macOS): an asm label names the symbol exactly as written, while every C name carries the target's `_`, so the mirror's and
    the overrides' labels (`__asm__("func_003df4d0")`, written for ELF) get the prefix; and ELF section attributes, which mean nothing to the
    PC build and which a Mach-O target rejects, are dropped. On the preprocessed text, before the AST is read (like va_list_casts)."""
    with open(path, "rb") as stream:
        text = stream.read().decode("latin-1")
    new = ASM_LABEL_RE.sub(lambda m: m[1] + m[2] + m[3] + prefix + m[4] + m[5], text)
    new = ELF_SECTION_RE.sub("", new)
    if new != text:
        open(path, "wb").write(new.encode("latin-1"))


def rewrite_preprocessed(ipath, flags, root, syms=None, oracle_unit=None, oracle_flags=None,
                         clang="clang", abi=None, layouts=None):
    """Rewrite an already expanded input without running its preprocessor twice."""
    if any(f.startswith("--target=aarch64") for f in flags):
        va_list_casts(ipath)
    if symbol_prefix(flags):
        macho_text(ipath, symbol_prefix(flags))
    ast = load_ast(ipath, stage2_flags(flags), clang)
    lay = {}
    if oracle_unit:
        for name, rs in (layouts if layouts is not None else oracle_layouts(oracle_unit, oracle_flags, clang)).items():
            m = re.match(r"^(struct|union) \(unnamed at (.*):(\d+):\d+\)$", name)
            key = f"@{os.path.relpath(m.group(2), root)}:{m.group(3)}" if m else name
            lay[key] = [{"size": r["size"], "fields": r["fields"]} for r in rs]
    rw = Rewriter(ast, root, syms, lay, ifile=ipath, abi=abi, sym_prefix=symbol_prefix(flags), variadic_on_stack=variadic_on_stack(flags))
    with build_cache.timed("rewrite.visit"):
        rw.run()
    with build_cache.timed("rewrite.apply"):
        text = rw.src(ipath)
        return rw, ipath, rw.edits.apply(ipath, text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="mirror root (text under it is rewritten)")
    ap.add_argument("--out", help="the rewritten preprocessed unit (.i) is written here")
    ap.add_argument("--census", help="write the census JSON here")
    ap.add_argument("--oracle-unit", help="the same unit in the i386 mirror; its layouts become the _Static_assert oracle")
    ap.add_argument("--oracle-flags", default="", help="i386 compile flags for --oracle-unit (one string)")
    ap.add_argument("--symbols", help="decomp checkout (config/symbol_*.txt) for retail-addressed data names")
    ap.add_argument("--abi", help="ABI table (JSON, merge_abi) the call sites are adapted to")
    ap.add_argument("--work", default=None, help="directory for the preprocessed intermediate (default: next to --out)")
    ap.add_argument("--clang", default="clang")
    ap.add_argument("unit")
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    syms = {}
    if a.symbols:
        sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
        import gen_port
        from pathlib import Path
        syms = gen_port.read_symbols(Path(a.symbols))
        syms.update(gen_port.read_port_symbols())
    import shlex
    flags = [f for f in a.flags if f != "--"]
    work = a.work or (os.path.dirname(os.path.abspath(a.out)) if a.out else ".")
    abi = json.load(open(a.abi)) if a.abi else None
    rw, ipath, text = process(a.unit, flags, work, a.root, syms, a.oracle_unit, shlex.split(a.oracle_flags), a.clang, abi)
    res = {"census": dict(rw.census), "unsupported": rw.unsupported, "defs": rw.defs, "calls": [[*k, v] for k, v in rw.calls.items()],
           "fn_addr_taken": sorted(rw.fn_addr_taken),
           "sites": {k: sorted(v) for k, v in rw.census.sites.items()}}
    if a.census:
        json.dump(res, open(a.census, "w"), indent=1)
    else:
        print(json.dumps(res["census"], indent=1))
    if a.out:
        open(a.out, "wb").write(text.encode("latin-1"))


if __name__ == "__main__":
    main()
