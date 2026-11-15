#!/usr/bin/env python3
"""Best-effort static inventory of selected campaign battle script commands as JSON.

    python tools/campaign_battles.py [--iso P4.ISO] [--elf SLUS_217.82] [--out FILE] [--summary]

This scans candidate FLW0 blocks outside excluded media formats, and linearly
approximates the value stack in each procedure. It does not follow control flow:
arguments, reachability and completeness must be checked independently. The
output contains retail-derived names and script identifiers; keep it private.
The ISO and ELF are read-only inputs. --out creates a new file, never overwrites
an existing one, and must be outside this checkout and the configured retail area.
Without --out, JSON goes to stdout, which must also be kept private.

Sources (NTSC-U guest layout, docs/platform/playthrough_coverage.md):

- Flowscripts: candidate FLW0 blocks in non-media files, including event and
  field scripts, field packs, battle BOSS/E*.BIN scripts and AICALC.TBL.
  A linear stack approximation identifies constants and nonconstant values;
  it is not a control-flow-aware replay.
- The script command table (D_00748530, eight-byte {commands, count} groups of
  eight-byte {handler, parameter count} rows; command id = group << 12 | index)
  is read from the ELF to estimate each known command's stack effect.
- BTL/BATTLE/ENCOUNT.TBL block 0 (0x18-byte records: flags at +0, bit 0 set when
  the battle can be escaped; five enemy IDs at +8; field and variant at +0x12 and
  +0x14) and BTL/BATTLE/MSG.TBL block 2 (21-byte enemy names) describe each
  encounter a script starts. A BOSS/E<hex>.BIN script belongs to encounter <hex>.

What a script cannot show is not inferred: field symbol encounters start in
native code (FldEvent_BeginEncounter), and the native tutorial, advantage and
results paths are not scripts. Fr/En in a unit check are relative to the unit
the script runs for; the inventory keeps the raw side and unit arguments.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import sys
from pathlib import Path

SECTOR = 2048
CVM_HEADER = 0x1800
CONTAINERS = ("DATA.CVM", "BTL.CVM", "BGM.CVM", "ENV.CVM", "ZZZZZ.BIN")
# Media and model formats never hold flowscripts.
MEDIA = {"ADX", "AFS", "AHX", "ACX", "ANM", "BD", "BED", "BVP", "CPK", "EPL", "FNT", "HD", "PM1", "PM2", "PM3",
         "RMD", "SE", "SEQ", "SFD", "SQ", "TMX"}
COMMAND_GROUPS = 0x00748530  # D_00748530 (decomp docs/dispatch_tables.md, "Script command groups")
ENCOUNTER_STRIDE = 0x18
ENEMY_NAME_STRIDE = 21

# Commands that start a battle; the argument names are the parameters' roles.
BATTLE_STARTS = {
    0x20DC: ("btlScriptStartEncounter", ("encounter",)),
    0x1016: ("CALL_BATTLE", ("encounter", "argument1")),
    0x1029: ("Cmd_PickEncounter", ("argument0",)),
}
# Commands that name a party member, a party slot or a battle unit.
PARTY_COMMANDS = {
    0x0038: ("PARTY_IN", ("member",)),
    0x0039: ("PARTY_OUT", ("member",)),
    0x005A: ("GetPartyMember", ("slot",)),
}
UNIT_CHECKS = {
    0x200E: ("btlAICommandScriptQueryFrId", ("unit",)),
    0x200F: ("btlAICommandScriptQueryEnId", ("unit",)),
    0x205D: ("btlAICommandSideIdQueryMyBad", ("unit", "ailments")),
    0x2069: ("btlAICommandScriptUnitIsIdOnSide", ("side", "unit")),
    0x207B: ("btlEffectScriptPickUnitByKind", ("side", "kind")),
    0x20C8: ("btlEffectScriptCheckSideAndId", ("side", "unit")),
    0x20FD: ("btlEffectScriptApplyToSideUnit", ("side", "unit", "argument2", "argument3", "argument4")),
}
PARTY_NAMES = {1: "YU", 2: "YOSUKE", 3: "CHIE", 4: "YUKIKO", 5: "RISE", 6: "KANJI", 7: "NAOTO", 8: "TEDDIE"}

# Flowscript opcodes (FLW0 code words: u16 opcode, u16 operand).
PUSHI, PUSHF, PUSHIX, PUSHIF, PUSHREG, POPIX, POPFX, PROC, COMM, END = range(10)
PUSHIS, PUSHLIX, PUSHLFX, POPLIX, POPLFX, PUSHSTR = 29, 30, 31, 32, 33, 34
BINARY = {14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 27}  # ADD SUB MUL DIV OR AND EQ NEQ S L SE LE
UNARY = {18, 19}  # MINUS NOT
POPS = {POPIX, POPFX, POPLIX, POPLFX, 28}  # 28 IF pops its condition


class Disc:
    """Read-only index of the disc's five containers (each CVM is an ISO 9660
    volume after its 0x1800-byte CVMH header); keys are "DATA/FIELD/...". """

    def __init__(self, path):
        self._fp = open(path, "rb")
        self.files = {}
        top = self._walk(0)
        missing = [name for name in CONTAINERS if name not in top]
        if missing:
            raise ValueError("%s is not a complete Persona 4 disc (missing %s)" % (path, ", ".join(missing)))
        for name in CONTAINERS:
            base, _ = top[name]
            if self._read(base, 4) != b"CVMH":
                raise ValueError("%s has no CVMH header" % name)
            for key, extent in self._walk(base + CVM_HEADER).items():
                self.files[name.split(".")[0] + "/" + key] = extent

    def close(self):
        self._fp.close()

    def _read(self, offset, size):
        self._fp.seek(offset)
        return self._fp.read(size)

    def _walk(self, base):
        pvd = self._read(base + 16 * SECTOR, SECTOR)
        if pvd[1:6] != b"CD001":
            raise ValueError("no ISO 9660 volume at offset 0x%x" % base)
        out = {}

        def visit(lba, size, prefix):
            data = self._read(base + lba * SECTOR, size)
            i = 0
            while i < len(data):
                length = data[i]
                if length == 0:
                    i = (i // SECTOR + 1) * SECTOR
                    continue
                record = data[i:i + length]
                i += length
                extent, extent_size = struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0]
                name = record[33:33 + record[32]]
                if name in (b"\0", b"\1"):
                    continue
                key = prefix + name.decode("ascii").split(";")[0]
                if record[25] & 2:
                    visit(extent, extent_size, key + "/")
                else:
                    out[key] = (base + extent * SECTOR, extent_size)

        visit(struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0], "")
        return out

    def read(self, key):
        offset, size = self.files[key]
        return self._read(offset, size)


def pack_members(data):
    """[(name, start, size)] of a flat pack (252-byte name, u32 size, payload, 64/128 alignment), else []."""
    entries, offset = [], 0
    while offset + 256 <= len(data):
        raw = data[offset:offset + 252].split(b"\0", 1)[0]
        if not raw or any(c < 0x20 or c > 0x7E for c in raw):
            break
        size = struct.unpack_from("<I", data, offset + 252)[0]
        start = offset + 256
        if start + size > len(data):
            break
        entries.append((raw.decode("ascii"), start, size))
        offset = (start + size + 63) // 64 * 64
        while offset + 256 <= len(data) and data[offset] == 0 and offset % 128 != 0:
            offset += 64
    return entries


def size_blocks(data):
    """The u32-size-prefixed, 16-byte-aligned blocks of a battle .TBL file."""
    out, offset = [], 0
    while offset + 4 <= len(data):
        size = struct.unpack_from("<I", data, offset)[0]
        if size == 0 and offset + 16 >= len(data):
            break
        out.append((offset + 4, data[offset + 4:offset + 4 + size]))
        offset += (4 + size + 15) & ~15
    return out


def command_table(elf):
    """{command id: (handler, parameter count)} from the retail ELF's script command groups."""
    if elf[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    phoff, = struct.unpack_from("<I", elf, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", elf, 0x2A)
    segments = []
    for i in range(phnum):
        kind, offset, vaddr, _, filesz = struct.unpack_from("<5I", elf, phoff + i * phentsize)
        if kind == 1:
            segments.append((vaddr, offset, filesz))

    def words(address, count):
        for vaddr, offset, filesz in segments:
            if vaddr <= address and address + count * 4 <= vaddr + filesz:
                return struct.unpack_from("<%dI" % count, elf, offset + address - vaddr)
        return None

    table = {}
    for group in range(16):
        record = words(COMMAND_GROUPS + group * 8, 2)
        if record is None or record[1] == 0 or record[1] > 0x1000:
            break
        rows = words(record[0], record[1] * 2)
        if rows is None:
            break
        for index in range(record[1]):
            table[group << 12 | index] = (rows[index * 2], rows[index * 2 + 1])
    if not table:
        raise ValueError("no script command table at 0x%08x" % COMMAND_GROUPS)
    return table


def parse_flowscript(data):
    """(procedures [(name, word)], code [(opcode, operand)], words [u32]) of a FLW0 block, or None."""
    if len(data) < 0x20 or data[8:12] != b"FLW0":
        return None
    count, = struct.unpack_from("<I", data, 0x10)
    if not 0 < count <= 8 or 0x20 + count * 16 > len(data):
        return None
    sections = {}
    for i in range(count):
        kind, size, entries, offset = struct.unpack_from("<4I", data, 0x20 + i * 16)
        if offset + size * entries > len(data):
            return None
        sections[kind] = (size, entries, offset)
    if 0 not in sections or 2 not in sections or sections[2][0] != 4:
        return None
    size, entries, offset = sections[0]
    if size <= 8:
        return None
    procedures = []
    for i in range(entries):
        row = data[offset + i * size:offset + (i + 1) * size]
        procedures.append((row[:size - 8].split(b"\0", 1)[0].decode("latin-1"), struct.unpack_from("<I", row, size - 8)[0]))
    _, entries, offset = sections[2]
    words = list(struct.unpack_from("<%dI" % entries, data, offset))
    return procedures, [(word & 0xFFFF, word >> 16) for word in words], words


def script_calls(script, table, wanted):
    """Approximate calls linearly; arguments may be wrong across control-flow edges."""
    procedures, code, words = script
    starts = sorted((start, name) for name, start in procedures if start < len(code))
    calls = []
    for index, (start, name) in enumerate(starts):
        end = starts[index + 1][0] if index + 1 < len(starts) else len(code)
        stack, i = [], start
        while i < end:
            opcode, operand = code[i]
            if opcode == PUSHI:
                stack.append(words[i + 1] if i + 1 < len(words) else {"kind": "unknown"})
                i += 2
                continue
            if opcode == PUSHF:
                stack.append({"kind": "float"})
                i += 2
                continue
            if opcode == PUSHIS:
                stack.append(operand - 0x10000 if operand & 0x8000 else operand)
            elif opcode in (PUSHIX, PUSHLIX):
                stack.append({"kind": "variable", "scope": "global" if opcode == PUSHIX else "local", "index": operand})
            elif opcode in (PUSHIF, PUSHLFX):
                stack.append({"kind": "variable", "scope": "global" if opcode == PUSHIF else "local", "float": True,
                              "index": operand})
            elif opcode == PUSHREG:
                stack.append({"kind": "return"})
            elif opcode == PUSHSTR:
                stack.append({"kind": "string"})
            elif opcode in POPS:
                if stack:
                    stack.pop()
            elif opcode in BINARY or opcode in UNARY:
                for _ in range(2 if opcode in BINARY else 1):
                    if stack:
                        stack.pop()
                stack.append({"kind": "expression"})
            elif opcode == COMM:
                if operand not in table:
                    if operand in wanted:
                        raise ValueError("no parameter count for command 0x%04X" % operand)
                    stack.clear()
                    i += 1
                    continue
                count = table[operand][1]
                if operand in wanted:
                    roles = (BATTLE_STARTS.get(operand) or PARTY_COMMANDS.get(operand) or UNIT_CHECKS.get(operand))
                    if roles and count != len(roles[1]):
                        raise ValueError("unexpected parameter count for command 0x%04X" % operand)
                arguments = [stack.pop() if stack else {"kind": "unknown"} for _ in range(count)]
                if operand in wanted:
                    calls.append((name, i, operand, arguments))
            elif opcode in (PROC, END):
                stack = []
            i += 1
    return calls


def flowscripts(disc):
    """Yield candidate FLW0 blocks found in non-media files."""
    for key in sorted(disc.files):
        leaf = key.rsplit("/", 1)[-1]
        if leaf.rsplit(".", 1)[-1] in MEDIA:
            continue
        data = disc.read(key)
        members = pack_members(data)
        blocks = size_blocks(data) if leaf.endswith(".TBL") else []
        at = data.find(b"FLW0")
        while at != -1:
            start = at - 8
            if start >= 0:
                # The header's size stops before the string section, so a script is bounded by
                # the pack member or table block that holds it, else by the file.
                holder = next(((name, first + length) for name, first, length in members
                               if first <= start < first + length), None)
                if holder is None:
                    holder = next((("block %d" % n, first + len(body)) for n, (first, body) in enumerate(blocks)
                                   if first <= start < first + len(body)), (None, len(data)))
                script = parse_flowscript(data[start:holder[1]])
                if script:
                    yield key, holder[0], script
            at = data.find(b"FLW0", at + 4)


def encounter_records(block, names):
    def record(encounter):
        if not 0 <= encounter < len(block) // ENCOUNTER_STRIDE:
            return None
        base = encounter * ENCOUNTER_STRIDE
        flags, = struct.unpack_from("<I", block, base)
        enemies = struct.unpack_from("<5H", block, base + 8)
        field, variant = struct.unpack_from("<2H", block, base + 0x12)
        return {"flags": flags, "escapable": bool(flags & 1),
                "enemies": [{"id": e, "name": names[e] if e < len(names) else None} for e in enemies if e],
                "field": field, "variant": variant}
    return record


def boss_encounter(key):
    match = re.fullmatch(r"BTL/BATTLE/BOSS/E([0-9A-F]{3})\.BIN", key)
    return int(match.group(1), 16) if match else None


def inventory(scripts, table, record):
    """The JSON inventory of (key, member, script) flowscripts; record(encounter) describes one."""
    wanted = set(BATTLE_STARTS) | set(PARTY_COMMANDS) | set(UNIT_CHECKS)
    battles, party, units = [], [], []
    for key, member, script in scripts:
        for procedure, word, command, arguments in script_calls(script, table, wanted):
            row = {"file": key, "member": member, "procedure": procedure, "word": word,
                   "command": "0x%04X" % command}
            if command in BATTLE_STARTS:
                name, roles = BATTLE_STARTS[command]
                row.update(name=name, arguments=dict(zip(roles, arguments)))
                encounter = arguments[0] if command != 0x1029 and isinstance(arguments[0], int) else None
                row["encounter"] = encounter
                row["record"] = record(encounter) if encounter is not None else None
                battles.append(row)
            elif command in PARTY_COMMANDS:
                name, roles = PARTY_COMMANDS[command]
                row.update(name=name, arguments=dict(zip(roles, arguments)))
                member_id = arguments[0]
                if command != 0x005A and isinstance(member_id, int) and member_id in PARTY_NAMES:
                    row["party_member"] = PARTY_NAMES[member_id]
                party.append(row)
            else:
                name, roles = UNIT_CHECKS[command]
                row.update(name=name, arguments=dict(zip(roles, arguments)), encounter=boss_encounter(key))
                units.append(row)
    started = sorted({b["encounter"] for b in battles if b["encounter"] is not None})
    return {
        "schema": 1,
        "battles": battles,
        "boss_unit_checks": units,
        "party_commands": party,
        "encounters": [dict(encounter=e, **(record(e) or {})) for e in started],
    }


def summary(result):
    lines = []
    seen = {}
    for battle in result["battles"]:
        seen.setdefault(battle["encounter"], set()).add(battle["procedure"])
    checks = {}
    for check in result["boss_unit_checks"]:
        if check["encounter"] is None:
            continue
        arguments = check["arguments"]
        if isinstance(arguments.get("unit"), int):
            checks.setdefault(check["encounter"], set()).add("%s(%s)" % (
                check["name"].replace("btlAICommandScript", "").replace("btlEffectScript", "").replace("btlAICommand", ""),
                ",".join(str(v) if isinstance(v, int) else v["kind"] for v in arguments.values())))
    for encounter in sorted(seen, key=lambda e: (e is None, e or 0)):
        record = next((r for r in result["encounters"] if r["encounter"] == encounter), None)
        enemies = ", ".join(e["name"] or str(e["id"]) for e in record["enemies"]) if record and "enemies" in record else "-"
        # A missing record is not evidence for either escape state.
        escape = ("escapable" if record["escapable"] else "no-escape") if record and "escapable" in record else "unknown"
        lines.append("%-5s %-9s %-40s %s" % (encounter if encounter is not None else "?", escape, enemies[:40],
                                              ", ".join(sorted(seen[encounter]))))
        if encounter in checks:
            lines.append("      unit checks: " + " ".join(sorted(checks[encounter])))
    for encounter in sorted(set(checks) - set(seen)):
        lines.append("%-5s boss script only; unit checks: %s" % (encounter, " ".join(sorted(checks[encounter]))))
    return "\n".join(lines)


def output_path(path):
    """Reject output in this checkout or the local read-only retail area."""
    destination = Path(path).resolve()
    repo = Path(__file__).resolve().parent.parent
    if destination.is_relative_to(repo):
        raise ValueError("--out must be outside the repository checkout")
    # local_work is private infrastructure and is absent in public exports.
    try:
        import local_work
    except ModuleNotFoundError as error:
        if error.name != "local_work":
            raise
    else:
        retail = (local_work.work_root() / local_work.RETAIL_AREA).resolve()
        if destination.is_relative_to(retail):
            raise ValueError("--out must be outside the read-only retail area")
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--iso", default=os.environ.get("P4PC_ISO"))
    parser.add_argument("--elf", default=os.environ.get("P4PC_ELF"))
    parser.add_argument("--out", help="create a new private JSON file outside this checkout and retail area")
    parser.add_argument("--summary", action="store_true", help="also print one line per started encounter")
    args = parser.parse_args()
    if not args.iso or not os.path.isfile(args.iso) or not args.elf or not os.path.isfile(args.elf):
        parser.error("--iso and --elf (or P4PC_ISO and P4PC_ELF) must name the player's disc image and SLUS_217.82")
    if args.out:
        try:
            destination = output_path(args.out)
        except ValueError as error:
            parser.error(str(error))
    with open(args.elf, "rb") as stream:
        elf = stream.read()
    table = command_table(elf)
    disc = Disc(args.iso)
    try:
        block = size_blocks(disc.read("BTL/BATTLE/ENCOUNT.TBL"))[0][1]
        messages = [body for _, body in size_blocks(disc.read("BTL/BATTLE/MSG.TBL"))]
        names = [messages[2][i:i + ENEMY_NAME_STRIDE].split(b"\0", 1)[0].decode("latin-1")
                 for i in range(0, len(messages[2]) - ENEMY_NAME_STRIDE + 1, ENEMY_NAME_STRIDE)]
        result = inventory(flowscripts(disc), table, encounter_records(block, names))
    finally:
        disc.close()
    result["elf_sha256"] = hashlib.sha256(elf).hexdigest()
    text = json.dumps(result, indent=1, sort_keys=True) + "\n"
    if args.out:
        fd = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            stream.write(text)
    else:
        sys.stdout.write(text)
    if args.summary:
        print(summary(result), file=sys.stderr if not args.out else sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
