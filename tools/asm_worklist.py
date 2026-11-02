#!/usr/bin/env python3
"""Worklist of asm-only functions the port still traps, reachable from the boot entry.

Inputs: the generated tables (gen/p4pc_tables.c, from `gen_port tables`), the compiled
objects, the decomp source tree, and the retail ELF. Output: docs/asm_worklist.md.

Roots: the boot entry (_start) and every function-pointer target gen_port patches.
Edges: a native (compiled) function's calls and address references, from its object's
text relocations; an asm-only function's jal/j targets, from its retail extent (its
address up to the next known function start, ending at the first jr $ra that no earlier
branch crosses).

Library: the decomp file whose `// FUN_<addr>` marker names the function.
Status: "asm-only" when the decomp has no C body for the function; otherwise
"C elsewhere" with the file that holds the body.
Excluded from the list: addresses claimed in overrides/CLAIMS.md, addresses with an
overrides/<addr>.c file, and sky2 units (PortGL).

--coverage FILE (repeatable): retail block coverage merged from PCSX2 hook runs of the real game by retail_coverage.py
(part of the private PCSX2 trace tooling, which is not in this repository; put it on PYTHONPATH). Rows gain a Retail
column (the function's entry count in retail, "-" when it never ran), the summary counts executed rows, and a section
lists every trap stub that ran in retail, whether or not the static walk reaches it: those are the ground-truth gaps.

--reach FILE: a TSV of the static reachability of the overrides that never ran in retail (from the same tooling):
the doc gains the section "Overrides never executed in retail", which marks the ones no retail EE path reaches
(dead) so they stop counting as coverage gaps.

Run under WSL Python: the build paths in gen/units.txt and gen/objects.txt are WSL paths.
"""

import argparse
import bisect
import re
import struct
import subprocess
import sys
from collections import defaultdict, deque
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_port as gp  # noqa: E402  (shared readers: symbols, markers, segments, nm)

BOOT_ENTRY = 0x00100008  # _start
JR_RA = 0x03E00008
# Retail code: the main image text, and the SDK code blob that gen_port maps at 0x0070C850.
CODE_RANGES = [(0x00100000, 0x0052DA00), (0x0070C850, 0x0070E140)]
BRANCH_OPS = {4, 5, 6, 7, 20, 21, 22, 23}
REGIMM_BRANCHES = {0, 1, 2, 3, 16, 17, 18, 19}
TRAP_RE = re.compile(r'^int (\w+)\([^)]*\) \{.*?p4pc_trap(?:_args)?\("(\w+)", 0x([0-9a-fA-F]{8})')
PATCH_RE = re.compile(r'^\s*\{\s*0x([0-9a-fA-F]{8}),\s*\(void\s*\*\)&(\w+),\s*(?:0|"[ipxf]*")\s*\},$')
RELOC_ROW = re.compile(r"^([0-9a-fA-F]{8,16})\s+(R_\w+)\s+(\S+?)(?:[+-]0x[0-9a-fA-F]+)?$")
OBJ_HDR = re.compile(r"^(.+?):\s+file format")
SECTION_HDR = re.compile(r"^RELOCATION RECORDS FOR \[(.+)\]:")
CLAIM_RE = re.compile(r"^([0-9a-fA-F]{8})\s+(\S+)")
SKY2_RE = re.compile(r"^src/rw/(basky|baskytran|skinskycommon|skyblit|skyconv|skyinst)\.c$")
# DMA/GIF/VIF register code (0x1000xxxx MMIO): replaced at the sky2 seam by the renderer lane.
HARDWARE_RE = re.compile(r"^src/rw/badma\.c$")
# Audio middleware is deferred: the CRI data paths come first.
AUDIO_RE = re.compile(r"^(adx|ahx|aix|acs|mwsfd|mps|mpv|dtx|dtr)", re.I)

# Rows another lane owns, with the reason (PortPromotedB batch 5). Listed in a separate
# section of the worklist instead of the asm-only rows. Owners: see docs/decomp_followups.md.
RECLASSIFIED_GROUPS = [
    ("PortCriC", "CRI: ACSS audio (acssnd)", [0x004BD720]),
    ("PortCriC", "CRI: H_Cdvd / adxstm / ADXT family (code1_004c)", [0x004C74F8, 0x004C85A0, 0x004C8708, 0x004C8A60, 0x004C9010, 0x004C95F8, 0x004C9670, 0x004C9820, 0x004C9D48, 0x004CA720, 0x004CB2D0, 0x004CCC98, 0x004CD3F0]),
    ("PortCriC", "CRI: aixp / ADXT / ACSSBADX (code1_004d)", [0x004D3850, 0x004D3BC0, 0x004D3CD8, 0x004D4308, 0x004DE2C0, 0x004DE4C0, 0x004DE540, 0x004DE860, 0x004DEB38, 0x004DEB58, 0x004DECB8, 0x004DED60, 0x004DEEC0, 0x004DF068, 0x004DF220, 0x004DFD78]),
    ("PortCriC", "CRI: htCi file layer (code1_004e)", [0x004E3A10, 0x004E3C78, 0x004E47F8, 0x004E48B0, 0x004E49E8, 0x004E76F8, 0x004E77C0, 0x004E7A58, 0x004E7BD0, 0x004E7C70, 0x004E7D08, 0x004E7D40, 0x004E7ED0, 0x004E8008, 0x004E8058, 0x004E8110]),
    ("PortCriC", "CRI: SRD (code1_004f; tail-calls into cri_srd)", [0x004F13C0, 0x004F1460, 0x004F14E0, 0x004F2370, 0x004F9A58, 0x004F9A70]),
    ("PortCriC", "CRI movie: MPS / SFMPV / MW DMA (code1_0050)", [0x00503488, 0x00503A68, 0x00506C30, 0x005097E8, 0x0050B8F0, 0x0050C008, 0x0050C940, 0x0050C9D0, 0x0050CD00]),
    ("PortCriC", "CRI: SFLIB / SFMPV (code1_0051)", [0x00513410, 0x00513948, 0x00517AD0, 0x00517CF0, 0x00519D40, 0x0051F608]),
    ("PortSkeleton-2", "RW-init path: kwlnInitSubsystems caller", [0x00418B18, 0x004CC200, 0x004CC218, 0x004D32F8, 0x004F34B0, 0x004F5BF0, 0x0050B3F8]),
    ("PortAsmC", "RW core: RxHeapAlloc, RxPipelineExecute, heap helper", [0x00414C10, 0x00414E10, 0x00414EF0]),
    ("unassigned: SCE MPEG (hardware)", "SCE MPEG decoder (named _sceMpeg*/_sequence*/_picture*/_copyright* in the same unit; unnamed rows between them are [INFERENCE])", [0x0041BE30, 0x0041C2F8, 0x0041C370, 0x0041C408, 0x0041C4C0, 0x0041C590, 0x0041C640, 0x0041C6F8, 0x0041C7F0, 0x0041C8F0, 0x0041C990, 0x0041CA40, 0x0041CB20, 0x0041CC08, 0x0041CCE0, 0x0041CDB0, 0x0041CED0, 0x0041D9D8, 0x0041DB18, 0x0041DC20, 0x0041E1E8, 0x0041E278, 0x0041E310, 0x0041E408, 0x00420CC8]),
    ("unassigned: SCE DMA (hardware)", "sceDmaSendN/RecvN and MWSPR_SaveSprByDma DMA sender", [0x00418DF8, 0x00418F50]),
    ("unassigned: memcard SDK", "memcard step, sceMc2CheckAsync, and the libmc client init and command queue (0x00430E28 waits on the IOP mc server, 0x00430F80 queues a command; both use the RPC client block at 0x008952C0)", [0x00430E28, 0x00430F80, 0x00431B58, 0x00432EC0]),
    ("unassigned: sound seam", "sdkSnd driver (0043bb70-0043c470)", [0x0043BB70, 0x0043BCB8, 0x0043C010, 0x0043C0C0, 0x0043C308, 0x0043C470]),
    ("PortGL", "VIF/DMA buffer control (renderer lane)", [0x00520338, 0x00520438]),
    ("unassigned: libc scanf (host-bound)", "newlib __svfscanf_r over a FILE: reached only through the retail sscanf (0x00442100, which the port replaces by the host sscanf; rwstring and the CRI callers bind the name `sscanf`) and the unreferenced wrappers func_004460c0/func_004460f0. Its callees (__srefill, _ungetc_r, strtol/strtoul/strtod and fread: 0x00441b40, 0x00444570, 0x00443fd0, 0x00444240, 0x00442088, 0x00443068, 0x0043e060) are not ported", [0x00446108]),
    ("unassigned: SIF DMA (hardware)", "SifSetDma wrapper (syscall 0x77, EE to IOP DMA descriptors)", [0x00424708]),
    ("PortCriC", "CRI: ADXT lock wrappers (code1_004c, DIntr-style lock around func_004c7f30 / func_004c8b70)", [0x004C7EF8, 0x004C8B48]),
    ("unassigned: SCE ROFS (hardware)", "ROFS init/finish and the RSU key scrambler (ROFS_Init/Finish state at 0x00743A50 and rsu_ScrambleVariKey, which needs the rofs_if rsu_ConvVarToAry helpers and func_004f8b18)", [0x004F34E8, 0x004F3780, 0x004F8F60]),
    ("PortGL", "renderer lane (PortGL-2): rpPTankSky instancing; scratchpad DMA channel registers (0x1000D000/0x1000D400) plus VU0 macro math writing VU1 UNPACK data", [0x003AA370, 0x003AA9F0, 0x003AB100, 0x003ABF60, 0x003ACB10, 0x003ADC40, 0x003AED60, 0x003AF990, 0x003B0B80, 0x003B12A0, 0x003B1A10, 0x003B3880]),
    ("PortGL", "renderer lane (PortGL-2): PTank instancers (strided vertex/colour/uv gathers and VIF packet writes); the port draws particle arrays straight to GL (src/render/p4gl_ptank.c) and its create callback never selects an instancer, so nothing calls these", [0x003A45C0, 0x003A5280, 0x003A5740, 0x003A5940, 0x003A5B50, 0x003A5DC0, 0x003A5FE0, 0x003A6230, 0x003A63B0, 0x003A6FB0, 0x003A72A0, 0x003A76C0, 0x003A8340, 0x003B4470]),
    ("PortGL", "renderer lane (PortGL-2): GIF/VIF packet builders; DMA chain written at the packet cursor (gp-0x477C) with DMA tags and VIF UNPACK tags", [0x0039C730, 0x0039C900, 0x003A14F0, 0x003A2E60, 0x003A3050, 0x003CD720, 0x003D2C40, 0x003A6650, 0x003A7A30, 0x003CC560, 0x003CC680, 0x003CC720, 0x0039CB80, 0x0039D360, 0x0039DCC0, 0x0039E740, 0x0039F160, 0x0039F9D0, 0x003A03A0, 0x003A0AA0, 0x003A8840, 0x003D0970, 0x003D2300]),
]
RECLASSIFIED = {addr: (owner, why) for owner, why, addrs in RECLASSIFIED_GROUPS for addr in addrs}


def in_code(va):
    return any(lo <= va < hi for lo, hi in CODE_RANGES)


def _parse_generated_tables(text):
    """Read actual patch rows, not similarly shaped rows of the native table."""
    traps, patches = {}, {}
    in_patches = seen_patches = False
    declared_count = None
    for lineno, line in enumerate(text.splitlines(), 1):
        m = TRAP_RE.match(line)
        if m:
            if m[2] in traps:
                raise ValueError(f"duplicate trap {m[2]} at table line {lineno}")
            traps[m[2]] = int(m[3], 16)
        if re.fullmatch(r"const\s+p4pc_patch_t\s+p4pc_patch_table\[\]\s*=\s*\{", line.strip()):
            if seen_patches:
                raise ValueError("duplicate p4pc_patch_table")
            in_patches = seen_patches = True
            continue
        if in_patches:
            if line.strip() == "};":
                in_patches = False
                continue
            if not line.strip():
                continue
            m = PATCH_RE.fullmatch(line)
            if not m:
                raise ValueError(f"unrecognized patch row at table line {lineno}: {line}")
            address = int(m[1], 16)
            if address in patches:
                raise ValueError(f"duplicate patch location 0x{address:08x}")
            patches[address] = m[2]
        else:
            m = re.fullmatch(r"\s*const\s+size_t\s+p4pc_patch_count\s*=\s*(\d+);", line)
            if m:
                if declared_count is not None:
                    raise ValueError("duplicate p4pc_patch_count")
                declared_count = int(m[1])
    if not seen_patches or in_patches or declared_count is None:
        raise ValueError("missing or incomplete generated patch table/count")
    if declared_count != len(patches):
        raise ValueError(f"patch count mismatch: declared {declared_count}, parsed {len(patches)}")
    return traps, patches


def chunks(items, n=400):
    for i in range(0, len(items), n):
        yield [str(o) for o in items[i:i + n]]

def non_matching_gated(text, marker_end):
    """True when the marker's definition sits under `#ifdef NON_MATCHING`. The port build never
    defines NON_MATCHING, so that C is not compiled; `#ifndef NON_MATCHING` is compiled."""
    for line in text[gp.line_end(text, marker_end):].split("\n", 40):
        s = line.strip()
        if s.startswith("#if") and "NON_MATCHING" in s and not s.startswith("#ifndef"):
            return True
        if s and not s.startswith(("#", "//", "/*")):
            return False
    return False


# ---------------------------------------------------------------- compiled objects

def object_calls(objs):
    """Compiled function name -> symbols its text relocations reference (calls and
    address-taken functions). Functions come from nm symbol ranges, relocations from objdump -r."""
    funcs = {}  # object path -> ([offsets], [names]) sorted by offset
    for chunk in chunks(objs):
        out = subprocess.run(["nm", "-P", "-S", *chunk], capture_output=True, text=True, check=True).stdout
        cur, found = None, defaultdict(list)
        for line in out.splitlines():
            if line.endswith(":"):
                cur = line[:-1]
                continue
            parts = line.split()
            if cur and len(parts) >= 3 and parts[1] in ("t", "T", "W", "i"):
                found[cur].append((int(parts[2], 16), parts[0]))
        for obj, syms in found.items():
            syms.sort()
            funcs[obj] = ([v for v, _ in syms], [n for _, n in syms])

    calls = defaultdict(set)
    for chunk in chunks(objs):
        out = subprocess.run(["objdump", "-r", *chunk], capture_output=True, text=True, check=True).stdout
        obj, in_text = None, False
        for line in out.splitlines():
            m = OBJ_HDR.match(line)
            if m:
                obj, in_text = m[1], False
                continue
            m = SECTION_HDR.match(line.strip())
            if m:
                in_text = m[1].startswith(".text")
                continue
            if not in_text or obj not in funcs:
                continue
            m = RELOC_ROW.match(line.strip())
            if not m or m[3].startswith("."):
                continue
            offsets, names = funcs[obj]
            i = bisect.bisect_right(offsets, int(m[1], 16)) - 1
            if i >= 0:
                calls[names[i]].add(m[3])
    # Calls to a function of the same object are resolved by the assembler and leave no relocation, so the helpers of an object
    # (statics the decomp gives no address) are tied to it: the walk treats them as called by every reached function of the object.
    objfuncs, fnobj = {}, defaultdict(set)
    for obj, (_, names) in funcs.items():
        objfuncs[obj] = names
        for n in names:
            fnobj[n].add(obj)
    return calls, objfuncs, fnobj


# ---------------------------------------------------------------- retail image

class Retail:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        self.segs = gp.load_segments(self.data)

    def u32(self, va):
        for vaddr, filesz, offset in self.segs:
            if vaddr <= va and va + 4 <= vaddr + filesz:
                return struct.unpack_from("<I", self.data, offset + va - vaddr)[0]
        return None


def cond_target(pc, w):
    """Target of a conditional branch at pc, or None."""
    op = w >> 26
    if op in BRANCH_OPS or (op == 1 and ((w >> 16) & 0x1F) in REGIMM_BRANCHES):
        imm = w & 0xFFFF
        if imm & 0x8000:
            imm -= 0x10000
        return pc + 4 + imm * 4
    return None


def extent(ret, a, nxt):
    """End of the function starting at a: the first jr $ra (plus delay slot) that no
    earlier branch jumps past, and never beyond the next known function start."""
    maxbr = a
    pc = a
    while pc < nxt:
        w = ret.u32(pc)
        if w is None:
            return pc
        t = cond_target(pc, w)
        if t is not None and t > maxbr:
            maxbr = t
        if w == JR_RA and maxbr <= pc + 8:
            return pc + 8
        pc += 4
    return nxt


def retail_callees(ret, a, starts, starts_set):
    """(size, callee addresses) of an asm function, from its retail jal/j targets."""
    i = bisect.bisect_right(starts, a)
    nxt = starts[i] if i < len(starts) else CODE_RANGES[0][1]
    end = extent(ret, a, nxt)
    out = set()
    hi = {}                  # register -> value of a pending lui
    for pc in range(a, end, 4):
        w = ret.u32(pc)
        if w is None:
            break
        op = w >> 26
        rs, rt, imm = (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
        # A function address built with lui + addiu / ori is a function pointer (task and callback creation): an edge of the walk.
        if op == 0x0F:       # lui
            hi[rt] = imm << 16
        elif op in (0x09, 0x0D) and rs in hi:   # addiu / ori on a pending lui
            v = (hi[rs] + (imm - 0x10000 if op == 0x09 and imm & 0x8000 else imm)) & 0xFFFFFFFF
            if v in starts_set and not (a <= v < end):
                out.add(v)
            hi.pop(rt, None)
        elif op == 3:  # jal
            t = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            if in_code(t):
                out.add(t)
        elif op == 2:  # j to another function (tail call)
            t = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            if t in starts_set and not (a <= t < end):
                out.add(t)
    return end - a, out


# ---------------------------------------------------------------- classification

def library_of(rel):
    if rel is None:
        return 6, "no decomp unit"
    if rel.startswith("src/renderware/core/"):
        return 0, "RW core"
    if rel.startswith(("src/renderware/plcore/", "src/renderware/world/")):
        return 0, "RW plcore/world"
    if rel.startswith(("src/renderware/p2/", "src/renderware/worldp2/", "src/rw/")):
        return 1, "RW p2/src-rw"
    if rel.startswith("src/cri/"):
        if AUDIO_RE.match(Path(rel).name):
            return 9, "CRI audio (deferred)"
        return 2, "CRI data paths"
    if rel.startswith("src/middleware/"):
        return 3, "middleware"
    if rel.startswith("src/sce/"):
        return 8, "SCE SDK (hardware)"
    parts = rel.split("/")
    return 4, "game/other: " + (parts[1] if len(parts) > 2 else "src")


def read_unbound_reasons():
    """config/unbound_natives.txt rows: address -> (name, reason). The reader in gen_port keeps only
    the names; the comment is the evidence this report prints."""
    out = {}
    path = gp.ROOT / "config" / "unbound_natives.txt"
    if path.is_file():
        for raw in path.read_text().splitlines():
            line, _, why = raw.partition("#")
            parts = line.split()
            if len(parts) == 2:
                out[int(parts[0], 16)] = (parts[1], why.strip())
    return out


def unbound_markdown(unbound, seen, executed, traps):
    out = ["## Linked bodies that are not the retail function\n",
           "`config/unbound_natives.txt`: a compiled body that a marker or symbol had placed at a retail "
           "address although it differs from the retail function there. The address is a trap (or a port "
           "translation named `func_<addr>`) instead. `Static walk` is this report's walk from the roots over "
           "the build's own calls. It over-approximates (every unaddressed helper of a compiled object counts "
           "as called by each function in that object), so a `yes` is a chain through such a helper, not a call "
           "in the retail graph: the evidence column says what retail actually does, and a boot with "
           "`P4PC_TRAP_CONTINUE=1` logs any of these traps that is hit.\n",
           "| Address | Name | Static walk | Retail | Evidence |", "|---|---|---|---|---|"]
    for addr in sorted(unbound):
        name, why = unbound[addr]
        ret = "n/a" if not executed else "-" if executed.get(addr) is None else f"executed ({executed[addr]})"
        out.append(f"| 0x{addr:08x} | {name} | {'yes' if addr in seen else 'no'} | {ret} | {why} |")
    out.append("")
    return out


def read_claims(path):
    claimed = {}
    if path.is_file():
        for line in path.read_text().splitlines():
            m = CLAIM_RE.match(line)
            if m:
                claimed[int(m[1], 16)] = m[2]
    return claimed


# ---------------------------------------------------------------- main

# ---------------------------------------------------------------------------- census
MOVIE_RE = re.compile(r"^(sf|mps|mpv|mwsfd|sj_|sud|uty)", re.I)
UNKNOWN_DOC = gp.ROOT / "docs" / "unknown_address_stubs.md"


def unknown_address_owners():
    """name -> owner from docs/unknown_address_stubs.md (`| Name | Owner | ... |` rows)."""
    out = {}
    if UNKNOWN_DOC.is_file():
        for line in UNKNOWN_DOC.read_text(encoding="utf-8").splitlines():
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) >= 2 and line.startswith("|") and cells[0] not in ("Name", "---", ""):
                out[cells[0]] = cells[1]
    return out


# The hardware rows, each assigned to the lane that models the device (reason in the category name).
HARDWARE_LANE = {
    "unassigned: SCE MPEG (hardware)": ("Movie/IPU: sceMpeg decoder API", "PortMovie2"),
    "unassigned: SCE DMA (hardware)": ("Renderer: SPR/DMA senders", "PortGL-2"),
    "unassigned: memcard SDK": ("Audio: libsd init and remote command (mislabelled memcard)", "PortAudio"),
    "unassigned: sound seam": ("Audio: sdkSnd IOP seam", "PortAudio"),
    "unassigned: SIF DMA (hardware)": ("Kernel/EE: SifSetDma host model", "PortAsmC-2"),
    # The ROFS driver is reached only through its own function tables (`--why-outside 004f1000-004fa000`
    # over every row lists no caller outside the group); the host VFS (src/platform/files) serves reads.
    "unassigned: SCE ROFS (hardware)": ("Replaced by the host VFS: ROFS driver (no entry point outside its own tables)", "port"),
}


def census_category(name, addr, seen, claimed, overridden, native_name, where, unknown_owner):
    """(category, owner) of one trap stub. First match wins; the order is the table order."""
    if not addr:
        owner = unknown_owner.get(name, "unlisted")
        if "CRI" in owner:
            return "No retail address: CRI (GameCube-origin API)", owner
        if "PortAsmC" in owner:
            return "No retail address: RW core", owner
        return "No retail address: other", owner
    if addr in native_name:
        return "Aliased to a compiled native", "-"
    if addr in claimed:
        return "Claimed, override in progress", claimed[addr]
    if addr in overridden:
        return "Overridden (file exists, its unit not compiled)", "-"
    unit = where.get(addr, (None, None))[0]
    if unit and (SKY2_RE.match(unit) or HARDWARE_RE.match(unit)):
        return "Renderer seam: PS2 sky2/DMA driver code", "PortGL-2"
    if addr in RECLASSIFIED:
        owner, why = RECLASSIFIED[addr]
        if owner == "PortGL":
            return "Renderer seam: GIF/VIF/DMA packet code", "PortGL-2"
        if owner == "PortCriC":
            if re.search(r"MPS|SFMPV|SFLIB|movie|SFD", why):
                return "Movie lane: CRI SFD/MPV", "PortCriC-2"
            return "Audio/CD lane: CRI ADX/ACSS/H_Cdvd", "PortCriC-2"
        if "scanf (host-bound)" in owner:
            return "Replaced by the host libc (no runtime caller)", "port"
        if owner in HARDWARE_LANE:
            return HARDWARE_LANE[owner]
        if owner.startswith("unassigned"):
            return "Hardware (no owner): " + owner.split(": ", 1)[-1], owner
        if owner == "PortAsmC":
            return "Reachable asm: RenderWare", "PortAsmC-2"
        if owner in ("PortSkeleton-2", "PortSkeleton"):
            return "Boot/kernel path", "PortSkeleton-3"
        return "Other reclassified: " + why[:40], owner
    reach = addr in seen
    lib = library_of(unit)[1]
    if not reach:
        return "Unreachable from boot and patch roots", lib
    if unit and unit.startswith("src/cri/"):
        if MOVIE_RE.match(Path(unit).name):
            return "Movie lane: CRI SFD/MPV", "PortCriC-2"
        return "Audio/CD lane: CRI ADX/ACSS/H_Cdvd", "PortCriC-2"
    if unit and unit.startswith("src/sce/"):
        return HARDWARE_LANE["unassigned: SCE ROFS (hardware)"]
    if unit and unit.startswith("src/promoted/"):
        return "Reachable asm: promoted code", "PortPromotedA2" if addr < 0x003DF9F0 else "PortPromotedB-2"
    if lib.startswith("RW"):
        return "Reachable asm: RenderWare", "PortAsmC-2"
    return "Reachable asm: " + lib, "unassigned"


def census_markdown(traps, seen, claimed, overridden, native_name, where):
    owners = unknown_address_owners()
    groups = defaultdict(list)
    for name, addr in sorted(traps.items()):
        cat, owner = census_category(name, addr, seen, claimed, overridden, native_name, where, owners)
        groups[(cat, owner)].append((name, addr))
    out = ["## Census of the trap stubs by category\n",
           "Every trap stub in the generated tables, counted once. The category is the first rule that "
           "matches (no retail address; aliased; claimed; overridden; renderer seam; reclassified group; "
           "unreachable; then the library of its unit). `Reachable` is the walk described above.\n",
           "| Category | Owner | Stubs |", "|---|---|---|"]
    total = 0
    for (cat, owner), items in sorted(groups.items(), key=lambda kv: (kv[0][0], -len(kv[1]))):
        out.append(f"| {cat} | {owner} | {len(items)} |")
        total += len(items)
    out.append(f"| **Total** | | **{total}** |\n")
    by_cat = defaultdict(int)
    for (cat, _o), items in groups.items():
        by_cat[cat.split(":")[0]] += len(items)
    out.append("By family:\n")
    out.append("| Family | Stubs |\n|---|---|")
    for fam, n in sorted(by_cat.items(), key=lambda kv: -kv[1]):
        out.append(f"| {fam} | {n} |")
    out.append("")
    out.append("Names per category (first 10 by address; every name for the hardware, reachable-asm and RenderWare rows):\n")
    for (cat, owner), items in sorted(groups.items(), key=lambda kv: (kv[0][0], -len(kv[1]))):
        everything = cat.startswith(("Hardware", "Movie/IPU", "Renderer: SPR", "Audio: ", "Kernel/EE", "File system", "Reachable asm", "No retail address: RW"))
        shown = sorted(items, key=lambda it: (it[1], it[0]))[: None if everything else 10]
        names = ", ".join(f"`{n}`" + (f" (0x{a:08x})" if a else "") for n, a in shown)
        out.append(f"- **{cat}** ({owner}, {len(items)}): {names}" + (" ..." if len(items) > 10 and not everything else ""))
    out.append("")
    return out



def coverage_markdown(labels, executed, traps, trap_at, seen, claimed, overridden, native_name, where, asm_rows,
                      display_name):
    """Section for --coverage: the trap stubs that ran in retail (ground truth), and how much of the worklist ran."""
    owners = unknown_address_owners()
    hit = []  # (category, owner, name, addr, entry count, reached by the static walk)
    for name, addr in sorted(traps.items()):
        if not addr or addr not in executed or addr in native_name:
            continue
        cat, owner = census_category(name, addr, seen, claimed, overridden, native_name, where, owners)
        if cat.startswith("Overridden") or cat.startswith("Aliased"):
            continue
        hit.append((cat, owner, name, addr, executed[addr], addr in seen))
    out = ["## Executed in retail (ground truth)\n",
           f"Block coverage of the real game under pcsx2hook ({', '.join(labels)}), mapped to decomp function starts: "
           f"{len(executed)} functions executed. Entry count: how often the block at the function's first instruction ran "
           f"(0: entered only mid-function). A trap stub listed here is on a path the retail game really runs; "
           f"`static` says whether the call-graph walk above also reaches it (no: it is called indirectly).\n",
           f"Trap stubs executed in retail (native, overridden and claimed functions excluded): **{len(hit)}**; "
           f"asm-only worklist rows executed: **{sum(1 for r in asm_rows if r['retail'] is not None)}** of {len(asm_rows)}.\n",
           "| Category | Owner | Name | Address | Entry count | Static |", "|---|---|---|---|---|---|"]
    for cat, owner, name, addr, n, static in sorted(hit, key=lambda h: (h[0], h[1], -h[4], h[3])):
        out.append(f"| {cat} | {owner} | {name} | 0x{addr:08x} | {n} | {'yes' if static else 'no'} |")
    out.append("")
    return out


REACH_REGIONS = [(0x100000, 0x180000, "boot / kernel"), (0x180000, 0x3A0000, "game code (field, battle, menus)"),
                 (0x3A0000, 0x400000, "RenderWare / HAnim"), (0x400000, 0x480000, "RenderWare / libc / kernel"),
                 (0x480000, 0x4C0000, "effects"), (0x4C0000, 0x500000, "CRI audio (ADX / AHX / ACS)"), (0x500000, 0x540000, "movie / Sofdec")]


def reach_markdown(path):
    """Section for --reach: the overrides that never ran in the corpus runs, by whether any retail EE path can reach them."""
    rows = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        parts = line.split("\t")
        if len(parts) >= 3:
            rows.append((int(parts[0], 16), parts[1], parts[2]))
    out = ["## Overrides never executed in retail (static reachability)\n",
           "Overrides (`tools/mipsdiff/sigs.txt`) that never ran in any pcsx2hook run of the real game (corpus savestates, scripted and "
           "random input, boot, New Game), classified by the PCSX2 trace tooling's retail_reach.py from the retail ELF: every `jal` / `j`, built "
           "address (`lui` + `addiu` / `ori`) and function-pointer table word, followed from the functions that did run. **dead** = no retail "
           "EE path reaches it (no reference, or only dead callers): no golden call can exist, so it is not a coverage gap. **reachable** = a "
           "call chain from code that ran leads to it (the path was not taken: content would exercise it). **table / pointer only** = only a "
           "callback slot or table names it and the consumer never ran. An address computed by arithmetic is not seen, so `dead` is "
           "conservative only for such pointers.\n"]
    classes = ["reachable (called from code that ran)", "table / pointer only (consumer never ran)", "dead (only dead callers)", "dead (no reference)"]
    counts = defaultdict(lambda: defaultdict(int))
    for addr, _name, cls in rows:
        for lo, hi, label in REACH_REGIONS:
            if lo <= addr < hi:
                counts[label][cls] += 1
                break
    out.append("| Region | " + " | ".join(c.split(" (")[0] + (" (" + c.split(" (")[1] if " (" in c else "") for c in classes) + " | Total |")
    out.append("|---|" + "---|" * (len(classes) + 1))
    tot = defaultdict(int)
    for _lo, _hi, label in REACH_REGIONS:
        if label in counts:
            out.append(f"| {label} | " + " | ".join(str(counts[label][c]) for c in classes) + f" | {sum(counts[label].values())} |")
            for c in classes:
                tot[c] += counts[label][c]
    out.append("| **Total** | " + " | ".join(str(tot[c]) for c in classes) + f" | {sum(tot.values())} |")
    out.append("")
    dead = [r for r in rows if r[2].startswith("dead")]
    out.append(f"### Dead: {len(dead)} overrides no retail EE path reaches\n")
    out.append("| Address | Name | Class |\n|---|---|---|")
    for addr, name, cls in sorted(dead):
        out.append(f"| 0x{addr:08x} | {name} | {cls} |")
    out.append("")
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--decomp", required=True, help="decomp checkout (src/, config/)")
    p.add_argument("--gen", required=True, help="build/gen from the build (tables, units.txt)")
    p.add_argument("--tables", help="generated table source (default: GEN/p4pc_tables.c; use p4pc_tables64.c with objects64.txt for x64)")
    p.add_argument("--objects-file", required=True)
    p.add_argument("--retail", required=True, help="retail SLUS_217.82 ELF")
    p.add_argument("--why", default="", help="comma-separated hex addresses: print the shortest call chain "
                   "from a root to each (stderr)")
    p.add_argument("--why-outside", default="", help="LO-HI hex: with --why, also print every reached "
                   "function outside that range that can call (transitively) into the --why addresses")
    p.add_argument("--unreach-callers", action="store_true", help="print every static caller of each unreached trap stub (stderr)")
    p.add_argument("--marker-audit", action="store_true", help="list compiled functions whose retail callees share nothing with the compiled ones (stderr)")
    p.add_argument("--coverage", action="append", default=[], metavar="FILE",
                   help="retail coverage JSON from the PCSX2 trace tooling's retail_coverage.py (repeatable; blocks are unioned)")
    p.add_argument("--reach", default="", metavar="FILE",
                   help="static reachability of the never-run overrides, from the PCSX2 trace tooling's retail_reach.py --tsv")
    p.add_argument("--out", default=str(gp.ROOT / "docs" / "asm_worklist.md"))
    a = p.parse_args()
    decomp, gen = Path(a.decomp), Path(a.gen)

    # 1. Trap stubs and actual patched locations, from this ABI's generated tables.
    tables = Path(a.tables) if a.tables else gen / "p4pc_tables.c"
    traps, patch_locations = _parse_generated_tables(tables.read_text(encoding="utf-8", errors="surrogateescape"))

    # 2. Names: the same sources as gen_port tables (curated symbols, compiled markers, excluded markers).
    syms = gp.read_symbols(decomp)
    syms.update(gp.read_port_symbols())
    compiled = []  # (absolute compiled copy, decomp-relative path)
    for line in (gen / "units.txt").read_text().splitlines():
        if line.strip():
            path = line.split("|", 1)[0]
            compiled.append((Path(path), path.split("/gen/decomp/", 1)[1]))
    _, excluded = gp.read_units(gp.ROOT)
    marker_names = {}
    for path, _rel in compiled:
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for mk in gp.MARKER_RE.finditer(text):
            name = gp.marker_body_name(text, mk.end(), int(mk[1], 16), syms)
            if name:
                marker_names.setdefault(int(mk[1], 16), name)
    for rel in excluded:
        f = decomp / rel
        if f.is_file():
            text = f.read_text(encoding="utf-8", errors="surrogateescape")
            for mk in gp.MARKER_RE.finditer(text):
                name = gp.marker_body_name(text, mk.end(), int(mk[1], 16), syms)
                if name:
                    marker_names.setdefault(int(mk[1], 16), name)
    for addr, name in marker_names.items():
        syms.setdefault(name, (addr, "func"))
    func_names, func_addr = {}, {}
    for name, (addr, kind) in sorted(syms.items()):
        if kind == "func":
            func_names.setdefault(addr, name)
            func_addr.setdefault(name, addr)

    # 3. Decomp-wide marker index: which file each function's marker sits in, and whether
    #    that file has a strict C body there (the body name must sit at the marker address).
    where = {}  # addr -> (decomp-relative file, body name or None)
    for f in sorted((decomp / "src").rglob("*.c")):
        rel = f.relative_to(decomp).as_posix()
        text = f.read_text(encoding="utf-8", errors="surrogateescape")
        for mk in gp.MARKER_RE.finditer(text):
            addr = int(mk[1], 16)
            body = gp.marker_body_name(text, mk.end(), addr, syms)
            if body is not None and non_matching_gated(text, mk.end()):
                body = None  # C under #ifdef NON_MATCHING: the port build never compiles it
            prev = where.get(addr)
            if prev is None or (prev[1] is None and body is not None):
                where[addr] = (rel, body)

    # 4. Native (compiled) names by address, and the function starts the walk uses.
    objs = [Path(x) for x in Path(a.objects_file).read_text(encoding="utf-8").splitlines() if x.strip()]
    _, _undefined, function_defs = gp.nm_symbols(objs)
    native_name = {}
    unbound = read_unbound_reasons()
    for addr in set(func_names) | set(marker_names):
        for cand in (marker_names.get(addr), func_names.get(addr), "func_%08x" % addr):
            if cand and cand in function_defs and cand != unbound.get(addr, (None,))[0]:
                native_name[addr] = cand
                break
    trap_addrs = {v for v in traps.values() if v}
    root_starts = set(func_names) | set(marker_names)
    starts_set = root_starts | trap_addrs
    starts = sorted(starts_set)

    # Retail coverage: function start -> entry count, for every function with an executed block.
    cov_labels, executed = [], {}
    if a.coverage:
        try:
            import retail_coverage as rcov
        except ImportError:
            sys.exit("asm_worklist: --coverage needs retail_coverage.py from the PCSX2 trace tooling on PYTHONPATH")
        blocks = {}
        for path in a.coverage:
            data = rcov.load(path)
            cov_labels.append(data.get("label", Path(path).stem))
            for start, end, count in data["blocks"]:
                old = blocks.get(start)
                blocks[start] = (max(end, old[0]) if old else end, (old[1] if old else 0) + count)
        executed = rcov.executed_functions([(s_, e_, c_) for s_, (e_, c_) in sorted(blocks.items())], starts)

    def addr_of(sym):
        m = gp.FUNC_NAME_RE.match(sym)
        if m:
            return int(m[1], 16)
        if sym in traps:
            return traps[sym] or None
        return func_addr.get(sym)

    calls, objfuncs, fnobj = object_calls(objs)

    # 5. Roots: _start and the function-pointer targets gen_port patches.
    ret = Retail(a.retail)
    roots = {BOOT_ENTRY}
    for va in patch_locations:
        target = ret.u32(va)
        if target not in root_starts:
            raise ValueError(f"patched location 0x{va:08x} has unknown retail function target {target!r}")
        roots.add(target)

    # 6. Walk.
    seen, edges, size = set(), defaultdict(set), {}
    seen_all = set(native_name)
    parent = {r: None for r in roots}
    queue = deque(roots)
    while queue:
        addr = queue.popleft()
        if addr in seen or not in_code(addr):
            continue
        seen.add(addr)
        if addr in native_name:
            syms = set(calls.get(native_name[addr], ()))
            for obj in fnobj.get(native_name[addr], ()):
                for helper in objfuncs[obj]:
                    if helper != native_name[addr] and not gp.FUNC_NAME_RE.match(helper) and helper not in func_addr and helper not in traps:
                        syms |= calls.get(helper, set())
            outs = {addr_of(s) for s in syms}
            outs = {t for t in outs if t and in_code(t)}
        else:
            size[addr], outs = retail_callees(ret, addr, starts, starts_set)
        edges[addr] = outs
        for t in outs:
            if t not in seen:
                parent.setdefault(t, addr)
                queue.append(t)

    if a.marker_audit:
        # A compiled function whose retail body calls functions the compiled code never references (and shares none of them) sits on the
        # wrong marker: the decomp C is for another function. Reported as: address name | retail-only callees | native-only callees.
        for addr in sorted(native_name):
            if addr not in seen_all:
                continue
            nat = set()
            for s in calls.get(native_name[addr], ()):
                v = addr_of(s)
                if v and in_code(v):
                    nat.add(v)
            nat0 = set(nat)
            # a call to a function of the same object leaves no relocation: every addressed function of the object counts as called
            for obj in fnobj.get(native_name[addr], ()):
                for helper in objfuncs[obj]:
                    v = addr_of(helper)
                    if v and in_code(v) and v != addr:
                        nat.add(v)
            ret_out = {x for x in retail_callees(ret, addr, starts, starts_set)[1] if x in starts_set}
            where = sorted(fnobj.get(native_name[addr], ()))
            if where and not any("/gen/decomp/" in str(o).replace("\\", "/") for o in where):
                continue                     # port source (src/): written to differ from retail
            if ret_out and nat0 and not (ret_out & nat):
                print("marker? %08x %s [%s] retail=%s native=%s" % (addr, native_name[addr], Path(str(where[0])).name if where else "?",
                      " ".join("%x" % x for x in sorted(ret_out)[:4]), " ".join("%x" % x for x in sorted(nat)[:4])), file=sys.stderr)
    for hexaddr in filter(None, a.why.split(",")):
        node, chain = int(hexaddr, 16), []
        while node is not None and node in parent:
            chain.append(f"{node:08x}")
            node = parent[node]
        print(f"why {hexaddr}: " + (" <- ".join(chain) if chain and int(hexaddr, 16) in seen else "not reached"),
              file=sys.stderr)
        # Every reached caller, not only the shortest chain's: a function-pointer root hides its callers.
        who = sorted(f"{c:08x}" for c in seen if int(hexaddr, 16) in edges[c])
        print(f"  callers {hexaddr}: " + (" ".join(who) if who else "none"), file=sys.stderr)

    callers = defaultdict(set)
    for addr in seen:
        for t in edges[addr]:
            callers[t].add(addr)

    if a.why and a.why_outside:
        lo, hi = (int(x, 16) for x in a.why_outside.split("-"))
        todo, anc = [int(x, 16) for x in a.why.split(",") if x], set()
        while todo:
            n = todo.pop()
            if n not in anc:
                anc.add(n)
                todo.extend(callers.get(n, ()))
        outside = sorted(n for n in anc if not lo <= n < hi)
        print("why-outside: " + " ".join(f"{n:08x}" for n in outside), file=sys.stderr)

    # 7. Which trap stubs are reachable, and why some are not in the worklist.
    trap_at = defaultdict(list)
    for name, addr in traps.items():
        if addr:
            trap_at[addr].append(name)
    reached = sorted(addr for addr in trap_at if addr in seen)
    if a.unreach_callers:
        # Every static caller (retail jal / j / lui+addiu pointer, any function) of each trap stub the walk did not reach, and whether that caller was reached.
        refs = defaultdict(set)
        for lo_, hi_ in CODE_RANGES:
            hi_regs = {}
            for pc in range(lo_, hi_, 4):
                w = ret.u32(pc)
                if w is None:
                    break
                op = w >> 26
                rs_, rt_, im_ = (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
                tgt = None
                if op in (2, 3):
                    tgt = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                elif op == 0x0F:
                    hi_regs[rt_] = im_ << 16
                elif op in (0x09, 0x0D) and rs_ in hi_regs:
                    tgt = (hi_regs[rs_] + (im_ - 0x10000 if op == 0x09 and im_ & 0x8000 else im_)) & 0xFFFFFFFF
                    hi_regs.pop(rt_, None)
                if tgt in starts_set and tgt not in seen or tgt in trap_at:
                    j = bisect.bisect_right(starts, pc) - 1
                    refs[tgt].add(starts[j] if j >= 0 else 0)
        for addr in sorted(trap_at):
            if addr in seen:
                continue
            cs = sorted(c for c in refs.get(addr, ()) if c != addr)
            print("unreach %08x %s callers: %s" % (addr, trap_at[addr][0], " ".join("%08x%s" % (c, "+" if c in seen else "-") for c in cs) or "none"), file=sys.stderr)
            # climb the unreached callers to the functions nothing calls at all
            todo, anc, top, via = list(cs), set(), set(), set()
            while todo:
                n = todo.pop()
                if n in seen:
                    via.add(n)      # a reached function that references the stub in retail but whose compiled code does not
                    continue
                if n in anc:
                    continue
                anc.add(n)
                up = [c for c in refs.get(n, ()) if c != n]
                if not up:
                    top.add(n)
                todo.extend(up)
            print("   top-ancestors %08x: %s" % (addr, " ".join("%08x" % n for n in sorted(top)) or "none"), file=sys.stderr)
            if via:
                print("   reached-callers %08x: %s" % (addr, " ".join("%08x" % n for n in sorted(via))), file=sys.stderr)
            if not top:
                print("   cluster %08x: %s" % (addr, " ".join("%08x" % n for n in sorted(anc))), file=sys.stderr)
    trap_total = len(traps)
    unknown_addr = sum(1 for v in traps.values() if not v)
    aliased = sum(1 for addr in reached if addr in native_name)
    claimed = read_claims(gp.ROOT / "overrides" / "CLAIMS.md")
    overridden = {int(gp.OVERRIDE_RE.match(f.name)[1], 16)
                  for f in (gp.ROOT / "overrides").glob("*.c") if gp.OVERRIDE_RE.match(f.name)}

    def display_name(addr):
        names = trap_at.get(addr) or []
        if names:
            return sorted(names, key=lambda n: (bool(gp.FUNC_NAME_RE.match(n)), n))[0]
        return marker_names.get(addr) or func_names.get(addr) or f"func_{addr:08x}"

    rows, excluded_counts = [], defaultdict(int)
    reclassified = []
    for addr in reached:
        if addr in native_name:
            continue  # aliased to its compiled native by gen_port
        if addr in claimed:
            excluded_counts["claimed by " + claimed[addr]] += 1
            continue
        if addr in overridden:
            excluded_counts["overridden"] += 1
            continue
        if addr in RECLASSIFIED:
            owner_, why_ = RECLASSIFIED[addr]
            excluded_counts["reclassified"] += 1
            reclassified.append((owner_, why_, addr, display_name(addr), where.get(addr, (None, None))[0] or "-"))
            continue
        unit = where.get(addr, (None, None))[0]
        if unit and SKY2_RE.match(unit):
            excluded_counts["sky2 (PortGL)"] += 1
            continue
        if unit and HARDWARE_RE.match(unit):
            excluded_counts["hardware: renderer lane (DMA/GIF/VIF)"] += 1
            continue
        names = set(trap_at[addr]) | {n for n in (func_names.get(addr), marker_names.get(addr)) if n}
        body = where.get(addr, (None, None))[1]
        if body is not None:
            status = f"C elsewhere: {unit}"
        else:
            status = "asm-only"
        rank, lib = library_of(unit)
        callee_kinds = [0, 0, 0]  # native, trap, other
        for t in edges[addr]:
            if t in native_name:
                callee_kinds[0] += 1
            elif t in trap_at:
                callee_kinds[1] += 1
            else:
                callee_kinds[2] += 1
        rows.append({
            "rank": rank, "lib": lib, "addr": addr, "name": display_name(addr), "unit": unit or "-",
            "size": size.get(addr, 0), "callers": sorted(callers[addr]), "callees": callee_kinds,
            "status": status, "retail": executed.get(addr),
        })

    rows.sort(key=lambda r: (r["status"] != "asm-only", r["rank"], -len(r["callers"]), r["addr"]))
    asm_rows = [r for r in rows if r["status"] == "asm-only"]
    elsewhere_rows = [r for r in rows if r["status"] != "asm-only"]

    # 8. Markdown.
    out = []
    out.append("# Asm-only worklist (generated)\n")
    out.append("Generated by `tools/asm_worklist.py`. Do not edit by hand; regenerate after each batch.\n")
    out.append("Reachable means: from the boot entry (`_start`, 0x00100008) or a function-pointer target that "
               "`gen_port` patches, through calls (native functions: compiled relocations; asm functions: "
               "retail `jal`/`j` targets).\n")
    out.append("## Summary\n")
    out.append("| Metric | Count |\n|---|---|")
    out.append(f"| Trap stubs in gen_port tables | {trap_total} |")
    out.append(f"| Trap stubs with unknown address (not listed) | {unknown_addr} |")
    out.append(f"| Trap stubs reachable from the roots | {len(reached)} |")
    out.append(f"| Reachable but aliased to a compiled native | {aliased} |")
    for why in sorted(excluded_counts):
        out.append(f"| Excluded: {why} | {excluded_counts[why]} |")
    out.append(f"| Listed: asm-only | {len(asm_rows)} |")
    out.append(f"| Listed: C elsewhere (gen_port aliasing) | {len(elsewhere_rows)} |")
    if executed:
        out.append(f"| Listed asm-only rows executed in retail | {sum(1 for r in asm_rows if r['retail'] is not None)} |")
    out.append("")
    out.extend(census_markdown(traps, seen, claimed, overridden, native_name, where))
    out.extend(unbound_markdown(unbound, seen, executed, traps))
    if executed:
        out.extend(coverage_markdown(cov_labels, executed, traps, trap_at, seen, claimed, overridden, native_name,
                                     where, asm_rows, display_name))
    if a.reach:
        out.extend(reach_markdown(a.reach))
    out.append("## Promoted batch split\n")
    out.append("The promoted game rows (batch 5) are split by address: PortPromotedA takes everything below "
               "0x003DF9F0, PortPromotedB takes 0x003DF9F0 and above (src/cri, src/sce and src/rw rows belong to "
               "their own lanes).\n")
    out.append("## Reclassified (PortPromotedB batch 5; not listed as asm-only)\n")
    out.append("| Owner | Address | Name | Unit | Reason |\n|---|---|---|---|---|")
    for owner_, why_, addr_, name_, unit_ in sorted(reclassified, key=lambda r: (r[0], r[2])):
        out.append(f"| {owner_} | 0x{addr_:08x} | {name_} | {unit_} | {why_} |")
    out.append("")
    out.append("## By library\n")
    out.append("| Library | Listed asm-only | Listed C elsewhere |\n|---|---|---|")
    libs = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        libs[(r["rank"], r["lib"])][0 if r["status"] == "asm-only" else 1] += 1
    for (rank, lib) in sorted(libs):
        asm_n, else_n = libs[(rank, lib)][0], libs[(rank, lib)][1]
        out.append(f"| {lib} | {asm_n} | {else_n} |")
    out.append("")
    out.append("## Worklist (asm-only first, in translation order)\n")
    out.append("Callers: reachable functions that call this one. Callees: native / trap / other, "
               "from the function's own calls. Size: retail bytes of the asm body.\n")
    retail_head = " Retail |" if executed else ""
    out.append("| Library | Address | Name | Unit | Size | Callers | Callees n/t/o | Status |" + retail_head)
    out.append("|---|---|---|---|---|---|---|---|" + ("---|" if executed else ""))
    for r in rows:
        who = ", ".join(display_name(c) for c in r["callers"][:2])
        more = f" (+{len(r['callers']) - 2})" if len(r["callers"]) > 2 else ""
        out.append(f"| {r['lib']} | 0x{r['addr']:08x} | {r['name']} | {r['unit']} | {r['size']} | "
                   f"{len(r['callers'])}{' ' + who + more if who else ''} | "
                   f"{'/'.join(str(c) for c in r['callees'])} | {r['status']} |"
                   + (f" {'-' if r['retail'] is None else 'executed (' + str(r['retail']) + ')'} |" if executed else ""))
    out.append("")

    out_path = Path(a.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(out))
    print(f"asm_worklist: {trap_total} trap stubs, {len(reached)} reachable, "
          f"{len(asm_rows)} asm-only listed, {len(elsewhere_rows)} C elsewhere listed -> {out_path}")


if __name__ == "__main__":
    main()
