#!/usr/bin/env python3
"""Runs the guest-pointer rewriter (gptr_rewrite.py) over every unit of a mirror and merges how often each rule fired
(docs/port64.md, "Inventory" and "Mechanism"). A site is counted once however many units include its header.

    census.py <i386 build dir (gen/units.txt, gen/decomp)> <out dir> [--abi abi.json] [--jobs N] [--limit N]

Writes census.json (merged counts, unsupported list) and census.txt (the table the doc quotes). Pass the abi.json the
build made (gen/obj64/abi.json) to count the calls adapted to their callee's definition. Read-only."""
import argparse, collections, json, multiprocessing as mp, os, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import gptr_rewrite as gr  # noqa: E402

ARGS = {}
T64 = ["--target=x86_64-linux-gnu"]


def init(a):
    ARGS.update(a)


def one(job):
    unit, flags = job
    t0 = time.time()
    work = os.path.join(ARGS["out"], "work", os.path.dirname(os.path.relpath(unit, ARGS["root"])))   # units share basenames
    try:
        rw, ipath, _text = gr.process(unit, T64 + flags, work, ARGS["root"], ARGS["syms"], abi=ARGS["abi"])
        os.remove(ipath)
        return unit, {"sites": {k: sorted(v) for k, v in rw.census.sites.items()}, "unsupported": rw.unsupported,
                      "fn_addr_taken": sorted(rw.fn_addr_taken), "secs": time.time() - t0}
    except BaseException as e:   # noqa: BLE001
        return unit, {"error": f"{type(e).__name__}: {str(e)[:300]}", "secs": time.time() - t0}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("build")
    ap.add_argument("out")
    ap.add_argument("--abi")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    mirror = os.path.join(a.build, "gen/decomp")
    from pathlib import Path
    import gen_port
    syms = gen_port.read_symbols(Path(a.root) / "decomp")
    syms.update(gen_port.read_port_symbols())
    abi = json.load(open(a.abi)) if a.abi else None
    jobs = []
    for ln in open(os.path.join(a.build, "gen/units.txt")):
        path, _, fl = ln.rstrip("\n").partition("|")
        flags = [x for x in fl.split("\t") if x and x != "-ferror-limit=0" and not x.startswith("-malign") and x != "-mno-implicit-float"]
        jobs.append((path, ["-DP4_PC=1", "-DP4_DECOMP_UNIT=1", "-include", "p4pc/compat.h", f"-I{a.root}/include"] + flags))
    if a.limit:
        jobs = jobs[:a.limit]
    sites = collections.defaultdict(set)
    unsup, failed, fnaddr, secs = {}, {}, set(), 0.0
    with mp.Pool(a.jobs, initializer=init, initargs=({"root": mirror, "syms": syms, "out": a.out, "abi": abi},), maxtasksperchild=25) as pool:
        for i, (unit, r) in enumerate(pool.imap_unordered(one, jobs, chunksize=1), 1):
            secs += r["secs"]
            if "error" in r:
                failed[unit] = r["error"]
                continue
            for k, v in r["sites"].items():
                sites[k].update(map(tuple, v))
            for what, loc in r["unsupported"]:
                unsup[(what, loc)] = 1
            fnaddr.update(r["fn_addr_taken"])
            if i % 200 == 0:
                print(f"{i}/{len(jobs)} units", file=sys.stderr)
    counts = {k: len(v) for k, v in sorted(sites.items())}
    counts["address-taken-functions"] = len(fnaddr)
    unsup_counts = collections.Counter(w for w, _ in unsup)
    json.dump({"units": len(jobs), "failed": failed, "counts": counts, "unsupported": unsup_counts.most_common(),
               "unsupported_sites": sorted(f"{w} @ {l}" for w, l in unsup)[:5000]}, open(os.path.join(a.out, "census.json"), "w"), indent=1)
    with open(os.path.join(a.out, "census.txt"), "w") as f:
        f.write(f"units: {len(jobs)}; the rewriter failed on {len(failed)}; {secs:.0f} cpu-seconds\n\nrule sites (a header site counted once):\n")
        for k, v in sorted(counts.items(), key=lambda kv: -kv[1]):
            f.write(f"  {k}: {v}\n")
        f.write("\nunsupported constructs (each needs a rule or a manual fix):\n")
        for w, n in unsup_counts.most_common():
            f.write(f"  {w}: {n}\n")
    print(open(os.path.join(a.out, "census.txt")).read())


if __name__ == "__main__":
    main()
