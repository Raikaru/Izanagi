#!/usr/bin/env python3
"""Summarises PROF samples with the exact executable's nm symbols.
usage: profile_report.py boot.log symbols.txt [top] [--image p4pc64w.exe]
--image reads the PE preferred base to resolve Windows ASLR. Full-width PROF/MOD
addresses and older low-word x64 logs are both supported."""
import argparse
import bisect
import collections
import struct


def pe_image_base(path):
    with open(path, "rb") as image:
        header = image.read(64)
        if header[:2] != b"MZ" or len(header) != 64:
            raise ValueError("--image must name the sampled PE executable")
        image.seek(struct.unpack_from("<I", header, 60)[0])
        header = image.read(56)
    if header[:4] != b"PE\0\0" or len(header) != 56:
        raise ValueError("invalid PE executable")
    magic = struct.unpack_from("<H", header, 24)[0]
    if magic == 0x20B:
        return struct.unpack_from("<Q", header, 48)[0]
    if magic == 0x10B:
        return struct.unpack_from("<I", header, 52)[0]
    raise ValueError("unknown PE optional-header format")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log")
    parser.add_argument("symbols")
    parser.add_argument("top", type=int, nargs="?", default=40)
    parser.add_argument("--image", help="the exact Windows executable, for PE ASLR relocation")
    args = parser.parse_args()
    log, symf, top = args.log, args.symbols, args.top
    preferred = pe_image_base(args.image) if args.image else None
    symbol_rows = []
    win = False
    mods = {}
    for line in open(log, errors="replace"):
        if line.startswith("MOD "):
            _, base, size, name = line.rstrip("\n").split(" ", 3)
            mods[(int(base, 16), int(size, 16))] = name
            win = win or name.lower().endswith(".exe")
    own = [(b, s, n) for (b, s), n in mods.items() if n.replace("\\", "/").split("/")[-1].lower().startswith("p4pc")]
    others = [(b, s, n.replace("\\", "/").split("/")[-1]) for (b, s), n in mods.items()
              if (b, s, n) not in own and not n.replace("\\", "/").split("/")[-1].lower().startswith("p4pc")]
    for line in open(symf, errors="replace"):
        p = line.split()
        if len(p) == 3:
            if preferred is None and p[2] in ("__ImageBase", "__ehdr_start"):
                preferred = int(p[0], 16)
            if p[1] in "tTwW":
                symbol_rows.append((int(p[0], 16), p[2][1:] if p[2].startswith("_") and not p[2].startswith("__") and win else p[2]))
    symbol_rows.sort()
    addrs = [a for a, _ in symbol_rows]
    names = [n for _, n in symbol_rows]
    if win and own and preferred is None and addrs and not any(b <= addrs[0] < b + s for b, s, _ in own):
        parser.error("relocated Windows image: pass --image with the sampled executable")

    def fn(a):
        for b, s, n in others:
            if b <= a < b + s:
                return "[" + n + "]"
        image = next(((b, s) for b, s, _ in own if b <= a < b + s), None)
        if own and image is None:
            return "?"
        if image is not None and preferred is not None:
            a = a - image[0] + preferred
        i = bisect.bisect_right(addrs, a) - 1
        return names[i] if i >= 0 else "?"

    self_c, incl_c, total = collections.Counter(), collections.Counter(), 0
    for line in open(log, errors="replace"):
        if not line.startswith("PROF "):
            continue
        frames = [int(x, 16) for x in line.split()[1:]]
        total += 1
        self_c[fn(frames[0])] += 1
        for name in {fn(a) for a in frames}:
            incl_c[name] += 1
    # Samples inside DLLs and shared objects (GL driver, window system): who in the exe called them. The first frame of the
    # chain that is exe code; the chain stops where a library without frame pointers breaks it.
    by_caller, ext_total = collections.Counter(), 0
    for line in open(log, errors="replace"):
        if not line.startswith("PROF "):
            continue
        frames = [int(x, 16) for x in line.split()[1:]]
        if not fn(frames[0]).startswith("["):
            continue
        ext_total += 1
        caller = next((fn(a) for a in frames[1:] if not fn(a).startswith("[") and fn(a) != "?"), "(chain lost)")
        by_caller[(fn(frames[0]), caller)] += 1
    print(f"{total} samples")
    if not total:
        return
    print("self:")
    for n, c in self_c.most_common(top):
        print(f"  {100.0 * c / total:5.1f}%  {n}")
    print("inclusive:")
    for n, c in incl_c.most_common(top):
        print(f"  {100.0 * c / total:5.1f}%  {n}")
    print(f"in libraries ({100.0 * ext_total / total:.1f}%), by module and first exe caller:")
    for (m, o), c in by_caller.most_common(top):
        print(f"  {100.0 * c / total:5.1f}%  {m} <- {o}")


if __name__ == "__main__":
    main()
