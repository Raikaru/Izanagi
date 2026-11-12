#!/usr/bin/env python3
"""Rewrite only i386 game printf-family calls to the shared masked EE GP ABI.

Unlike gptr_rewrite this pass changes no layouts, storage or pointer arithmetic.
Fixed integer addresses become identity pointers; masked integer varargs use
exactly the same uint64-word normalization as the full 64-bit rewriter.
The --full fixture mode uses the production 64-bit rewriter and the same rows.
"""
import argparse
import os

try:
    from . import gptr_rewrite as gr
except ImportError:  # Direct CLI execution, like unit64.py.
    import gptr_rewrite as gr


FIXED = {
    "sprintf": ("void *", "const char *"),
    "snprintf": ("void *", "unsigned int", "const char *"),
    "printf": ("const char *",),
    "fprintf": ("void *", "const char *"),
}


def printf_abi():
    """Canonical family rows, also usable by the full rewriter's fixture."""
    return {
        name: {"ret": "int", "rt": "int", "skip": False, "va": "other",
               "params": [{"cls": "ptr" if "*" in ct else "int", "ct": ct}
                          for ct in FIXED[family]]}
        for name, family in gr.PRINTF_FAMILY.items()
    }


def process(unit, flags, work, root, clang="clang", deps=None, symbol_prefix=""):
    os.makedirs(work, exist_ok=True)
    ipath = os.path.join(work, os.path.basename(unit) + ".printf.i")
    gr.preprocess(unit, flags, ipath, clang, deps)
    return rewrite_preprocessed(ipath, flags, root, clang, symbol_prefix)


def rewrite_preprocessed(ipath, flags, root, clang="clang", symbol_prefix=""):
    """Use the same real AST visitor after a caller has expanded dependencies."""
    ast = gr.load_ast(ipath, gr.stage2_flags(flags), clang)
    rw = gr.Rewriter(ast, root, ifile=ipath, sym_prefix=symbol_prefix)
    rw.index(ast)

    def visit(node, depth):
        if node.get("kind") == "CallExpr":
            callee = node["inner"][0]
            while callee.get("kind") in ("ParenExpr", "ImplicitCastExpr") and callee.get("inner"):
                callee = callee["inner"][0]
            if callee.get("kind") == "DeclRefExpr" and callee.get("referencedDecl", {}).get("kind") == "FunctionDecl":
                cname = callee["referencedDecl"]["name"]
                decl = rw.decl_by_id.get(callee["referencedDecl"]["id"], {})
                name = decl.get("mangledName") or cname
                if symbol_prefix and name.startswith(symbol_prefix):
                    name = name[len(symbol_prefix):]
                family = gr.PRINTF_FAMILY.get(name)
                args = node["inner"][1:]
                if family and cname not in rw.local_defs and name not in rw.local_defs and len(args) > len(FIXED[family]):
                    sp = gr.span(callee)
                    if sp and rw.mine(sp[0], sp[1]):
                        private = f"p4abi_{cname}_{len(args)}"
                        key = (private, rw.cur_func_pos[0])
                        if key not in rw.abi_decls:
                            rw.abi_decls.add(key)
                            proto = ", ".join((*FIXED[family], "..."))
                            rw.edits.insert_before(*rw.cur_func_pos,
                                f'extern int {private}({proto}) __asm__("{symbol_prefix}p4pf_{family}");\n')
                        rw.edits.replace(*sp, private)
                        for arg, ct in zip(args, FIXED[family]):
                            original = arg
                            while original.get("kind") in ("ParenExpr", "ImplicitCastExpr") and original.get("inner"):
                                original = original["inner"][0]
                            if "*" in ct and rw.pclass(rw.qt(original)) == "int":
                                rw.wrap_node(arg, "((void *)(unsigned long)(", "))", depth + 1)
                        rw.cur = node
                        rw.printf_arguments(args[len(FIXED[family]):], depth)
        for child in node.get("inner", ()):
            if isinstance(child, dict):
                visit(child, depth + 1)

    for top in ast.get("inner", ()):
        if isinstance(top, dict) and top.get("kind") == "FunctionDecl" and any(
                c.get("kind") == "CompoundStmt" for c in top.get("inner", ()) if isinstance(c, dict)):
            sp = gr.span(top)
            if sp and rw.mine(sp[0], sp[1]):
                rw.cur_func_pos = (sp[0], sp[1])
                visit(top, 0)
    return rw, rw.edits.apply(ipath, rw.src(ipath))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--work")
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--depfile")
    parser.add_argument("--symbol-prefix", default="")
    parser.add_argument("--full", action="store_true",
                        help="use the full mirror rewriter for a 64-bit typed-caller fixture")
    parser.add_argument("unit")
    parser.add_argument("flags", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    work = args.work or os.path.dirname(os.path.abspath(args.out))
    deps = (args.depfile, args.out) if args.depfile else None
    flags = [f for f in args.flags if f != "--"]
    if args.full:
        _, _, text = gr.process(args.unit, flags, work, args.root,
                                clang=args.clang, abi=printf_abi(), deps=deps)
    else:
        _, text = process(args.unit, flags, work, args.root,
                          args.clang, deps, args.symbol_prefix)
    with open(args.out, "wb") as output:
        output.write(text.encode("latin-1"))


if __name__ == "__main__":
    main()
