#!/usr/bin/env python3
"""Headless qualification of the bundled seeded randomizer (mods/p4pc.randomizer).

Runs main.luau in the real pinned Luau VM through luau_script_runner with a
synthetic p4 provider (tests/mod_randomizer_fixture.luau) and checks:

  vectors    SHA-256, stream derivation, xoshiro128**, below(), Fisher-Yates,
             hi/lo counters and the canonical run definition, each recomputed
             here independently;
  synthetic  generation on synthetic banks: digests pinned below must match on
             every target (Linux i386/x86_64/AArch64, Windows x64), pacing and
             feature switches never change a mapping;
  lifecycle  main.luau's own reset/New Game/save/load/verify/NG+ callbacks;
  corpus     with --iso (or P4PC_ISO), many seeds through generation and the
             validator on the user's own disc banks, extracted to a private
             temporary directory; each feature must change at least half of its
             eligible entries for every seed.

No retail byte is committed or written outside the temporary directory.
"""
import argparse
import hashlib
import os
from pathlib import Path
import statistics
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.randomizer" / "main.luau"
FIXTURE = ROOT / "tests" / "mod_randomizer_fixture.luau"
SCRIPT = ROOT / "tests" / "mod_randomizer_engine_test.luau"
TWO32 = 1 << 32
ALGORITHM = 1

# Generation on the synthetic banks, identical on every target:
# (seed hex, options) -> (run digest, output hash).
PINNED_SYNTHETIC_INPUT = "d5cec1718c7a269d4718e79fbda19e811fa995e9a8d01f7fd2fece6dccf8cab1"
PINNED = {
    ('00000000000000000000000000000000', 1): ('4d54562bafa26d1bd4ab08207430988251024866706208bccd23068b036137de', '5613491b49d402944c5903ad05667afab8c2f8d73ef98a6fded95b87efafd788'),
    ('00000000000000000000000000000000', 2): ('e7d9e3281d34a1186dc2427d69ce7af54224d92baf2eb9fc4ad6caf5edfb3e75', '4ed648fd90fc3cb12d12d1c0c17f629449a5207c334941e518f02e091a55769a'),
    ('00000000000000000000000000000000', 4): ('080067a5131a62d65a8126a0ddb56162e951ad5a291c097b36f75475723dc71b', '01f1e4b0ebf938f770486163e67255a3a66c7037516f98a2db1a1c8444720669'),
    ('00000000000000000000000000000000', 7): ('68630b590c60f5e97555316002936823ade793a33685f2bdc27e2d9a60f11fb8', '1b67f2ac6a9c7ce4e95d20479895dad551b76a144cf8029edc2e5d341d44fdba'),
    ('00000000000000000000000000000000', 8): ('f6685370ed096461fc19842334d334ff37d83d4fb07df29a4ed13503e5dcb7ac', '9c432bd68639eb81a25f97fde68f45117f7ea53bb513efa32a7e623521d44239'),
    ('00000000000000000000000000000000', 16): ('e157849568f4055b60fe72f92625b0191be0bc35c0e94a2910ce9362ad65b5f8', 'ac94dc9d56200e80796479f11fa01dd2234d2143177a3dcaf542fc92d8297600'),
    ('00000000000000000000000000000000', 32): ('180f00cafc9cc2014ae82685fda3a96c9533e7a8a71940d50852c6ef235e29f3', '56be0916a1ac91a330f1f30a8b6f6d7bf804d6066f17d7d7e15be67484f9f83e'),
    ('00000000000000000000000000000000', 64): ('4b74746babf93699ab358d79a02f4d7f33531845531d2e1c4ac7851867bda438', '8850468437c5d060ed242c6c88d38d69a836bd02fd4c6dda708b3be815f138fb'),
    ('00000000000000000000000000000000', 128): ('f0dbd3f3b950db845eb8f3a779b88c1b6473bcccdc69870f00aba5bfd031917f', 'ca992e0a1a7f36b759e35eaacfeb467161b6fff1b17d3ba8dce8fa673d61824f'),
    ('00000000000000000000000000000000', 256): ('f3716c007736e252eb7cb14df93a35dd1ec1283fc0ee166ca792d440ca7822a3', '01d4d1e2998d6f38309d47ba2ae96620d771f9c240d2fc524111a0b5c8c84618'),
    ('00000000000000000000000000000000', 4095): ('85c685709af5583ad94141916756b891be9fb8baf9414184add6a68e5c7a69af', 'c76ff7f3cb066b24a959e7c16028f13cf09f49a5e0cc7946d15b0d7e73910425'),
    ('00000000000000000000000000000001', 1): ('1d7a65567a39687d4a91a65d74a564119ee3176ace9d9ce9c6fa5461f2bfe526', 'a673675a9f454006611bc6e99bc53cc9eae6c139ff3e4a776a7469b4107456fe'),
    ('00000000000000000000000000000001', 2): ('48b844cc09b1167ac5cd6f756a346b18f4eb8382a42a88657dfb206b2d2cea0a', '532bfcf1aedf7e9d93b64b2870d17dbae46119712d0b264f37be4eab1a076dc8'),
    ('00000000000000000000000000000001', 4): ('d11645b00362a2bb34166170397023d72725a4eaea803c9d1bc70a0de21149ca', '56c47097e0cc589e290f9b44addeedce362fce6f24732cbdf2655783b285ed96'),
    ('00000000000000000000000000000001', 7): ('ffd7f98acb9155925c027233aaef10a7c3aba2c1fee497de8a968d010c78ba86', 'bae312164160070fae6aaaf4839c66fe4f04a2bee8f5b24b3d8a58252986bfab'),
    ('00000000000000000000000000000001', 8): ('877b4533f38b6250617422e642867d348963b5e0eac93dd99a04a312fe43ddce', 'c46bf94bd98b8e95a52bae828b1a1da74be7868f841deab50b1dbbd920e4b470'),
    ('00000000000000000000000000000001', 16): ('337fb08b65f5894ac63faf1c10833b572a060dabe6115a91daf56d6d50ae67bd', '8f3545ccc354de7c64596fc8a08f26b6d2d4739ca98bf4faf4f1781273717294'),
    ('00000000000000000000000000000001', 32): ('ee51cc65aec3c68358db2d9850b239cdb3ffcccb7271aa9087a0e8cb1a9822b4', 'd00a3530fc20c80aaa578dccd0c12f66c9a15555afaabae03f8269c3e2db102f'),
    ('00000000000000000000000000000001', 64): ('701c920857e23cc88adf902bf2e00a3bb9da46be8304c75679d594b2f75edfa8', '0a187e94ec0eee6f41abc96888a3cb7e80825a7f7a3b723986c090f6c6bbe043'),
    ('00000000000000000000000000000001', 128): ('01af1686f1099329bd00b24fcb49f40a3808c899cb54367951308c2715a7c997', '59cde63767aed07b01d8cfecdbbe5e6ba111fcb24edf1507f984e08a4cf5dd67'),
    ('00000000000000000000000000000001', 256): ('1f506087d7c967fb0a3437ea0bd8c71e9ba055553e530fc1c3ce4a84bfc925f5', '1916d6d767ec0f15e42a173e9e6489a246e0aa31e40d628ab6a542533826752b'),
    ('00000000000000000000000000000001', 4095): ('563ecb8636518574b824f9808bcce78ac04026d7c5afcf385cbaf6b8ce97ea88', '2204dede7cd1dd6635f1861c3228c10129a25fd1d237d278f130f2d52e14a0cf'),
    ('ffffffffffffffffffffffffffffffff', 1): ('8517c6f335a544ddcab93e8c1a34cd1f3ca931bddeddeea2ab396daa0262af1a', '2d352b15d1490e352c24aa544366f414a99a1f56c5a33e4f6814d6d4dfc6bf60'),
    ('ffffffffffffffffffffffffffffffff', 2): ('8772f35535bbfeabd8a924d4398126f0fdbc2c1bb9d0cf7278cf72bf7483d6f2', '444e80825327f9cff9b8ec64dc29e7306ed2556984bb298cddf08692a4c8f54e'),
    ('ffffffffffffffffffffffffffffffff', 4): ('5a6e0f56870e6c401f448046d4747d3f1a5793d1c8ac1acc7cc9747f2b7bf471', '927f74139c153f478b3e4b8422441994bfce75803c09e1e05b250f1844325f4b'),
    ('ffffffffffffffffffffffffffffffff', 7): ('d506c2dfced1057ae613b78f3523c28b91b19f606a9ae70327f4f3816af152e8', '229d0f71fce201631ebe30faf312f8b243c731adb2c25d617badec105fd84f4d'),
    ('ffffffffffffffffffffffffffffffff', 8): ('d5dc1b649f3a7511d9f924bdf1356c29fef261038e09401a0e8eef730c677ccd', '7246ddf554245607ef075f9de6a1d1925ac73e3175b6b1acd5ba266701f71ff9'),
    ('ffffffffffffffffffffffffffffffff', 16): ('a9589d8ef5d4b8ec9fcb1cdc396faa9b26561bfddf09fde42d84121abf9f9bcd', '7bdffed66db6c8a58dc9395a4842b5fbdecf21b8277b8eb367cd530758f59882'),
    ('ffffffffffffffffffffffffffffffff', 32): ('1f0952fda26648d89f17a654f489bd36e6fb5dba4d4472cd3ee3f9f643b60aef', 'ba156f3db2e774fb2ea1bb3634c5199b33b33eccc34bb756e2fbe01d7b1bdeb6'),
    ('ffffffffffffffffffffffffffffffff', 64): ('c38604d067a951497931adff457f9aaff8303573a06b4c778d599cbaa0369ee3', '41371ceb4d1fbfbecff740dd9164d975b7c71e788bbc76b06d7076f64e88a8d2'),
    ('ffffffffffffffffffffffffffffffff', 128): ('2c24f913b79df8577620290a1c02426dee7b3fbe566a1920f87b5f1ed9f1ba15', '5f9f0c1acbb443a12a6f6a47db5906a5a0cd7e978889ba8d9b15b2359d77afef'),
    ('ffffffffffffffffffffffffffffffff', 256): ('161cd57038813e6ed8c8bbb95fe0a139bda0273edd9114ff76f887b0b8ce2bd3', 'b6cbdb732ce3b8116e31bd1cfb84220fbbf5c9650184caddb5f93208d6e39a65'),
    ('ffffffffffffffffffffffffffffffff', 4095): ('4dfc937251af89c07778442e816fd31741cd3fb0945162f7cd0fb90695f8ecb9', 'bade12e24a339e6295e6a5e087f48d0e8af3819ba32e2d410a2193718d404533'),
}


def run(command, mode, *extra):
    result = subprocess.run([*command, str(FIXTURE), str(MOD), str(SCRIPT), "--", mode, *extra],
                            capture_output=True, text=True, timeout=3600)
    if result.returncode != 0:
        raise AssertionError(f"{mode}: runner failed ({result.returncode}): {result.stderr}{result.stdout[-2000:]}")
    return result.stdout.splitlines()


def rotl(x, k):
    return ((x << k) | (x >> (32 - k))) & 0xFFFFFFFF


class Xoshiro:
    def __init__(self, words):
        self.s = list(words)

    def next(self):
        s = self.s
        result = (rotl((s[1] * 5) & 0xFFFFFFFF, 7) * 9) & 0xFFFFFFFF
        t = (s[1] << 9) & 0xFFFFFFFF
        s[2] ^= s[0]
        s[3] ^= s[1]
        s[1] ^= s[2]
        s[0] ^= s[3]
        s[2] ^= t
        s[3] = rotl(s[3], 11)
        return result

    def below(self, n):
        limit = (TWO32 // n) * n
        while True:
            x = self.next()
            if x < limit:
                return x % n


def stream(seed, label):
    digest = hashlib.sha256(b"P4PC-RANDO\0" + struct.pack("<I", ALGORITHM) + seed
                            + struct.pack("<I", len(label)) + label).digest()
    words = struct.unpack("<4I", digest[:16])
    if words == (0, 0, 0, 0):
        words = (1, 0, 0, 0)
    return Xoshiro(words)


def check_vectors(lines):
    facts = 0
    seeds = {}
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "SHA":
            size, message, digest = rest.split(" ")
            assert len(bytes.fromhex(message)) == int(size), line
            assert hashlib.sha256(bytes.fromhex(message)).hexdigest() == digest, line
        elif kind == "STREAM":
            parts = rest.split(" ")
            seed = bytes.fromhex(parts[0])
            assert len(parts) == 10, line
            label = bytes.fromhex(parts[1])
            outputs = parts[-8:]
            r = stream(seed, label)
            assert [f"{r.next():08x}" for _ in range(8)] == outputs, line
            seeds[parts[0]] = seed
        elif kind in ("ZEROSTATE", "HIGHSTATE"):
            words = (1, 0, 0, 0) if kind == "ZEROSTATE" else (0xFFFFFFFF, 0xFFFFFFFE, 0x80000000, 0xFFFFFFFF)
            r = Xoshiro(words)
            assert [f"{r.next():08x}" for _ in range(8)] == rest.split(" "), line
        elif kind == "BELOW":
            parts = rest.split(" ")
            n = int(parts[0])
            r = stream(bytes.fromhex("0123456789abcdeffedcba9876543210"), b"below")
            assert [str(r.below(n)) for _ in range(12)] == parts[1:], line
        elif kind == "BELOW_REFUSED":
            assert rest.endswith(" true"), line
        elif kind == "SHUFFLE":
            r = stream(bytes.fromhex("0123456789abcdeffedcba9876543210"), b"shuffle")
            values = list(range(1, 13))
            for i in range(len(values), 1, -1):
                j = r.below(i)
                values[i - 1], values[j] = values[j], values[i - 1]
            assert " ".join(map(str, values)) == rest, line
        elif kind == "COUNTER":
            left, result = rest.split(" ")
            start, amount = left.split("+")
            assert f"{int(start, 16) + int(amount):016x}" == result, line
        elif kind == "COUNTER_OVERFLOW_REFUSED":
            assert rest == "true", line
        elif kind == "CANONICAL":
            seed = bytes.fromhex("0123456789abcdeffedcba9876543210")
            expected = (b"P4PC-RANDO-RUN\0" + struct.pack("<II", 1, 1) + seed + struct.pack("<I", 5)
                        + hashlib.sha256(b"fixture input").digest())
            assert expected.hex() == rest, line
            canonical = expected
        elif kind == "DIGEST":
            assert hashlib.sha256(canonical).hexdigest() == rest, line
        elif kind == "CODE":
            fingerprint = hashlib.sha256(b"fixture input").hexdigest()
            assert rest == f"P4R1-0123456789abcdeffedcba9876543210-005-{fingerprint[:8]}", line
        else:
            raise AssertionError("unexpected vector line: " + line)
        facts += 1
    assert facts == 38 and seeds, facts
    return facts


def check_synthetic(lines, record):
    generated = {}
    flags = {}
    for line in lines:
        parts = line.split(" ")
        if parts[0] == "SYNTH_INPUT":
            if record:
                print(f"PINNED_SYNTHETIC_INPUT = \"{parts[1]}\"")
            else:
                assert parts[1] == PINNED_SYNTHETIC_INPUT, line
        elif parts[0] == "GEN":
            generated[(parts[1], int(parts[2]))] = (parts[3], parts[4], parts[5])
        elif parts[0] == "PACED":
            assert parts[3] == "true", line
            assert int(parts[2]) > 0 or parts[1] == "100", line
        elif parts[0] in ("INDEPENDENT", "WRONG_ALGORITHM_REFUSED"):
            flags[parts[0]] = parts[1]
    assert flags == {"INDEPENDENT": "true", "WRONG_ALGORITHM_REFUSED": "true"}, flags
    assert len(generated) == 33, generated
    if record:
        for key, value in sorted(generated.items()):
            print(f"    {key!r}: {value[:2]!r},")
        return generated
    for key, value in generated.items():
        assert PINNED.get(key) == value[:2], (key, value, PINNED.get(key))
    return generated


def check_lifecycle(lines):
    facts = dict(line.split(" ", 1) for line in lines)
    worst = facts.pop("CALLBACK_WORST", "missing")
    assert worst.startswith("true "), f"a lifecycle callback used {worst} interpreter units"
    expected = {
        "OFF_FRAMES": "0", "OFF_FIELD_CALLBACKS": "0", "OFF_OVERLAYS": "0", "TITLE_FRAMES_DONE": "true true",
        "NEW_GAME_APPLIED": "true true", "SPOILER_WRITTEN": "true", "SPOILER_REEXPORTED": "true", "TIGHT_PUBLISHED": "true", "REARM": "false", "LOAD_AUTHORIZED": "true",
        "LOAD_APPLIED": "true", "LOAD_VERIFIED": "true", "TAMPERED_REFUSED": "true true", "LOCKED": "true",
        "NG_PLUS_REFUSED": "true RANDOMIZER: NO NG+", "SEED_PENDING": "00112233445566778899aabbccddeeff",
        "SEED_PERSISTED": "0" * 32, "R6_LIVE": "true true true", "VARIETY_NOTE": "true false false", "R6_HOOKS_OFF": "true", "R10_LIVE": "true true true", "R11_LIVE": "true true true",
    }
    assert facts == expected, {k: (facts.get(k), v) for k, v in expected.items() if facts.get(k) != v}


BANKS = {"SKILL0": ("SKILL.TBL", 0), "SKILL1": ("SKILL.TBL", 1), "PERSONA0": ("PERSONA.TBL", 0),
         "PERSONA1": ("PERSONA.TBL", 1), "UNIT2": ("UNIT.TBL", 2), "MSG1": ("MSG.TBL", 1), "MSG3": ("MSG.TBL", 3),
         "UNIT0": ("UNIT.TBL", 0), "MSG2": ("MSG.TBL", 2), "ENCOUNT0": ("ENCOUNT.TBL", 0),
         "ENCOUNT3": ("ENCOUNT.TBL", 3), "ENCOUNT4": ("ENCOUNT.TBL", 4)}


# Static ELF data inputs: the four arcana charts' cells (1,1)..(22,22) and the fusion gates.
ELF_SPANS = {"FUSION0": (0x00642147 + 25, 526), "FUSION1": (0x00642387 + 25, 526),
             "FUSION2": (0x00642727 + 25, 526), "FUSION3": (0x00642967 + 25, 526), "GATES": (0x006420A0, 184)}


def elf_span(elf, address, size):
    """Bytes of a loaded ELF virtual address range, through its PT_LOAD headers."""
    phoff, = struct.unpack_from("<I", elf, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", elf, 0x2A)
    for i in range(phnum):
        kind, offset, vaddr, _, filesz = struct.unpack_from("<IIIII", elf, phoff + i * phentsize)
        if kind == 1 and vaddr <= address and address + size <= vaddr + filesz:
            return elf[offset + address - vaddr:offset + address - vaddr + size]
    raise ValueError(f"0x{address:08x} is not in a loaded ELF segment")


def extract(iso, directory):
    sys.path.insert(0, str(ROOT / "tests"))
    import retail_tables
    files = retail_tables.battle_files(iso, sorted({name for name, _ in BANKS.values()}))
    for key, (name, block) in BANKS.items():
        (Path(directory) / f"{key}.bin").write_bytes(retail_tables.blocks(files[name])[block])
    with open(iso, "rb") as handle:
        elf = retail_tables.Volume(handle, 0).file("SLUS_217.82")
    for key, (address, size) in ELF_SPANS.items():
        (Path(directory) / f"{key}.bin").write_bytes(elf_span(elf, address, size))
    # The item table as the game loads it: INIT/ITEMTBL.BIN's rows after its 16-byte header.
    (Path(directory) / "ITEMS.bin").write_bytes(retail_tables.data_file(iso, "INIT/ITEMTBL.BIN")[16:16 + 1536 * 68])


def check_corpus(lines, count):
    rows = [line.split(" ") for line in lines if line.startswith("CORPUS ")]
    failed = [line for line in lines if line.startswith("CORPUS_FAILED")]
    assert not failed, failed[:5]
    assert len(rows) == count, (len(rows), count)
    hashes = {row[3] for row in rows}
    assert len(hashes) == count, "two seeds generated the same banks"
    percents = {}
    music_pools = set()
    starter_pools = set()
    for row in rows:
        for part in row[4].split(","):
            code, numbers = part.split("=")
            changed, eligible = map(int, numbers.split("/"))
            if code == "R12" and eligible < 2:
                # One qualified ordinary battle track: it maps to itself (stated in the spoiler).
                assert changed == 0, row
                music_pools.add(eligible)
                continue
            if code == "R11":
                # One starter drawn from the pool: coverage of the pool, not a changed share.
                assert changed == 1 and eligible >= 2, row
                starter_pools.add(eligible)
                continue
            percents.setdefault(code, []).append(100.0 * changed / eligible)
    starters = [int(line.split(" ")[2]) for line in lines if line.startswith("STARTER ")]
    if starter_pools:
        assert len(starter_pools) == 1 and len(starters) == count, (starter_pools, len(starters))
        pool = starter_pools.pop()
        chosen = {starter: starters.count(starter) for starter in sorted(set(starters))}
        if count >= 10 * pool:
            assert len(chosen) == pool, f"R11 drew only {sorted(chosen)} from a pool of {pool}"
        print(f"mod_randomizer_corpus: R11 starter pool of {pool} ordinary level 1..3 Personas; drawn "
              + ", ".join(f"{starter:03d} x{times}" for starter, times in chosen.items()) + f" over {count} seeds")
    if music_pools:
        print(f"mod_randomizer_corpus: R12 ordinary battle-music pool of {sorted(music_pools)} track(s) on this disc: "
              "it maps to itself, no battle's music changes")
    # R4: the triangle closure is a superset of the pair closure and every
    # Persona it adds is the exact result of its recorded fusion.
    triangle = [line.split(" ") for line in lines if line.startswith("TRIANGLE ")]
    assert triangle and all(row[2] == "true" for row in triangle), triangle
    stock = next(row for row in triangle if row[1] == "stock")
    print(f"mod_randomizer_corpus: R4 fusion closure with triangle fusions: stock {stock[4][5:]} Personas "
          f"({stock[3][5:]} by pairs alone); " + ", ".join(f"seed {row[1]} {row[4][5:]}" for row in triangle if row[1] != "stock")
          + "; every added Persona is its recorded fusion's exact result")
    alone = [line for line in lines if line.startswith("ALONE ")]
    assert alone and all(line.split(" ")[3] == "true" for line in alone), [l for l in alone if " true" not in l][:5]
    print(f"mod_randomizer_corpus: {len(alone)} single-feature runs validated and identical to the all-features banks")
    # R5 without R6 moves a formation only where its Shadows already fit the slot's
    # budget (the design's rule), so alone it changes far fewer slots: reported, not gated.
    r5_alone = [100.0 * int(c) / int(e) for c, e in (line.split(" ")[1:3] for line in lines if line.startswith("ALONE_R5 "))]
    if r5_alone:
        print(f"mod_randomizer_corpus: R5 without R6 changed min {min(r5_alone):.1f}% median "
              f"{statistics.median(r5_alone):.1f}% max {max(r5_alone):.1f}% over {len(r5_alone)} seeds (budget rule)")
    for line in lines:
        if line.startswith("BUDGET "):
            fields = dict(part.split("=") for part in line.split(" ")[2:])
            if "growth" in fields:
                # The game's mod VM holds 8 MiB: generation may add at most 4 MiB
                # (live and not yet collected) on top of the stock banks and the mod.
                assert int(fields["growth"]) < 4 * 1024 * 1024, line
            else:
                # The game gives one callback 4096 units: the longest resume must leave
                # the frame handler headroom, and no stretch between two checks may
                # exceed the pacing margin (2048) the mod keeps.
                assert int(fields["worst"]) < 3500 and int(fields["gap"]) < 1200, line
            print("mod_randomizer_corpus: " + line)
    for code, values in sorted(percents.items()):
        assert min(values) >= 50.0, f"{code} changed only {min(values):.1f}% of eligible entries for some seed"
        print(f"mod_randomizer_corpus: {code} changed min {min(values):.1f}% median {statistics.median(values):.1f}% "
              f"max {max(values):.1f}% over {len(values)} seeds")
    return percents


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--iso", default=os.environ.get("P4PC_ISO"), help="the user's disc image for the corpus")
    parser.add_argument("--seeds", type=int, default=100, help="corpus seeds 0..N-1 (canonical 128-bit values)")
    parser.add_argument("--record", action="store_true", help="print the synthetic digests instead of checking")
    parser.add_argument("--corpus-only", action="store_true")
    parser.add_argument("--spoiler", type=Path, help="write seed 0's spoiler log here (private output)")
    parser.add_argument("runner", nargs="+", help="luau_script_runner and an optional emulator before it")
    args = parser.parse_args()
    command = args.runner
    if not args.corpus_only:
        facts = check_vectors(run(command, "vectors"))
        check_synthetic(run(command, "synthetic"), args.record)
        check_lifecycle(run(command, "lifecycle"))
        print(f"mod_randomizer_engine: {facts} independent vectors, pinned synthetic generation, pacing and "
              "feature independence, and the run lifecycle PASS")
    if args.iso:
        if not Path(args.iso).is_file():
            print(f"mod_randomizer_corpus: {args.iso} is not readable", file=sys.stderr)
            return 77 if args.corpus_only else 0
        with tempfile.TemporaryDirectory(prefix="p4-rando-corpus-") as directory:
            extract(args.iso, directory)
            extra = [directory, str(args.seeds)] + (["spoiler"] if args.spoiler else [])
            lines = run(command, "corpus", *extra)
            check_corpus(lines, args.seeds)
            if args.spoiler:
                begin, end = lines.index("SPOILER_BEGIN"), lines.index("SPOILER_END")
                args.spoiler.write_text("\n".join(lines[begin + 1:end]) + "\n", encoding="utf-8")
            print(f"mod_randomizer_corpus: {args.seeds} seeds generated and validated on this disc's banks "
                  f"({[line for line in lines if line.startswith('CORPUS_INPUT')][0]}) PASS")
    elif args.corpus_only:
        print("mod_randomizer_corpus: no P4PC_ISO", file=sys.stderr)
        return 77
    return 0


if __name__ == "__main__":
    sys.exit(main())
