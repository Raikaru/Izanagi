#!/usr/bin/env python3
"""Step adapter for the `zig build` graph (build.zig, zigbuild/core.zig); no build logic of its own.

    zig_build_step.py exec [--stamp FILE] [--depfile FILE (--depfile-from FILE | --depfile-from-out)] -- CMD...
        Runs CMD (its exit status is the step's). On success it copies the depfile CMD wrote beside its output
        (--depfile-from FILE, or --depfile-from-out: the value of CMD's --out argument plus ".d", which is where
        tools/port64/unit64.py writes it) to --depfile, the path the Zig cache reads, and writes --stamp, the cached
        output of a check whose results are files outside the cache (reports) or nothing at all.

    zig_build_step.py stage [--skip-suffix SUFFIX]... -- DEST=SRC...
        Places each SRC at the fixed build-tree path DEST (a file, or a directory whose files go into DEST): the tools
        read objects and their sidecars by the layout of the CMake build tree (tools/crt_guard.py: CMakeFiles/<target>.dir/,
        tools/gen_port.py tables: p4decomp.dir and obj64/units, tools/port64/unit64.py report: the *.census files under one
        directory). A DEST that already has SRC's bytes is left alone; otherwise it is hard-linked to SRC, or copied
        where a link cannot be made. Nothing is ever removed.

    zig_build_step.py alias TARGET OUTDIR NAME
        Makes OUTDIR/NAME a symbolic link to the fixed build-tree directory TARGET: a cached path through which a
        consumer that takes a directory reaches that tree in the graph's dependency order (it is produced after the
        steps that stage TARGET).
"""
import filecmp
import os
import shutil
import subprocess
import sys


def die(message):
    sys.exit(f"zig_build_step: {message}")


def cmd_exec(argv):
    if "--" not in argv:
        die("exec needs -- CMD")
    split = argv.index("--")
    options, command = argv[:split], argv[split + 1:]
    if not command:
        die("exec: empty command")
    stamp = depfile = depfile_from = None
    from_out = False
    i = 0
    while i < len(options):
        option = options[i]
        if option in ("--stamp", "--depfile", "--depfile-from") and i + 1 < len(options):
            value = options[i + 1]
            if option == "--stamp":
                stamp = value
            elif option == "--depfile":
                depfile = value
            else:
                depfile_from = value
            i += 2
        elif option == "--depfile-from-out":
            from_out = True
            i += 1
        else:
            die(f"exec: unknown option {option!r}")
    if from_out:
        outs = [a[len("--out="):] for a in command if a.startswith("--out=")]
        outs += [command[k + 1] for k, a in enumerate(command[:-1]) if a == "--out"]
        if len(outs) != 1:
            die(f"exec --depfile-from-out: the command has {len(outs)} --out arguments")
        depfile_from = outs[0] + ".d"
    if (depfile is None) != (depfile_from is None):
        die("exec: --depfile and its source go together")
    rc = subprocess.call(command)
    if rc != 0:
        sys.exit(rc)
    if depfile:
        if not os.path.isfile(depfile_from):
            die(f"exec: {command[0]} wrote no depfile {depfile_from}")
        shutil.copyfile(depfile_from, depfile)
    if stamp:
        with open(stamp, "w", encoding="utf-8") as f:
            f.write("ok\n")


def same(a, b):
    try:
        sa, sb = os.stat(a), os.stat(b)
    except FileNotFoundError:
        return False
    if (sa.st_dev, sa.st_ino) == (sb.st_dev, sb.st_ino):
        return True
    return sa.st_size == sb.st_size and filecmp.cmp(a, b, shallow=False)


def place(src, dest):
    if same(src, dest):
        return
    os.makedirs(os.path.dirname(dest) or ".", exist_ok=True)
    tmp = f"{dest}.zig-stage-{os.getpid()}"
    try:
        os.link(src, tmp)
    except OSError:
        shutil.copy2(src, tmp)
    os.replace(tmp, dest)


def cmd_stage(argv):
    if "--" not in argv:
        die("stage needs -- DEST=SRC...")
    split = argv.index("--")
    options, entries = argv[:split], argv[split + 1:]
    skip = []
    i = 0
    while i < len(options):
        if options[i] == "--skip-suffix" and i + 1 < len(options):
            skip.append(options[i + 1])
            i += 2
        else:
            die(f"stage: unknown option {options[i]!r}")
    for entry in entries:
        dest, sep, src = entry.partition("=")
        if not sep or not dest or not src:
            die(f"stage: malformed entry {entry!r}")
        if os.path.isdir(src):
            for name in sorted(os.listdir(src)):
                path = os.path.join(src, name)
                if os.path.isfile(path) and not any(name.endswith(s) for s in skip):
                    place(path, os.path.join(dest, name))
        elif os.path.isfile(src):
            place(src, dest)
        else:
            die(f"stage: {src} does not exist")


def cmd_alias(argv):
    if len(argv) != 3:
        die("alias needs TARGET OUTDIR NAME")
    target, outdir, name = argv
    if not os.path.isdir(target):
        die(f"alias: {target} is not a directory")
    os.makedirs(outdir, exist_ok=True)
    link = os.path.join(outdir, name)
    if os.path.lexists(link):
        os.remove(link)
    os.symlink(os.path.abspath(target), link, target_is_directory=True)


def main():
    if len(sys.argv) < 2:
        die("usage: zig_build_step.py exec|stage ...")
    {"exec": cmd_exec, "stage": cmd_stage, "alias": cmd_alias}.get(sys.argv[1], lambda _: die(f"unknown command {sys.argv[1]!r}"))(sys.argv[2:])


if __name__ == "__main__":
    main()
