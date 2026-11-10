#!/usr/bin/env python3
"""Developer-only PS2 card/PSU writer and inspector.

Player save importing has moved to the native, checksum-checked importer:
  p4pc --import-save FILE_OR_FOLDER [--slot N]
This module retains build_card for the lockstep harness, not a second importer.

  ps2save.py dir2psu MCDIR/FOLDER OUT.psu
  ps2save.py mkcard MCDIR/FOLDER OUT.ps2 [NAME]
  ps2save.py list SAVE.psu
  ps2save.py mcdlist CARD.ps2
  ps2save.py selftest
"""
import os
import struct
import sys
import time
import mod_witness

MODE_DIR = 0x8427
MODE_FILE = 0x8497
ENTRY = 512
PAD = 1024
NATIVE_SAVE_METADATA = frozenset((".p4mods", ".p4mods-commit", mod_witness.MARKER, mod_witness.RECEIPT))


def _refuse_native_names(names):
    mod_witness.refuse_fidelity()
    for name in names:
        if name.casefold() in NATIVE_SAVE_METADATA or name.casefold().endswith(mod_witness.SIDECAR):
            raise ValueError("cannot export native or witness save metadata to PS2: %s; "
                             "no approved retail-safe migration" % name)


def refuse_ps2_export(folder):
    """Native package state and witness provenance have no approved PS2 migration."""
    mod_witness.refuse_fidelity()
    mod_witness.refuse_reference(folder)
    for root, directories, files in os.walk(folder):
        _refuse_native_names((*directories, *files))
        for name in files:
            mod_witness.refuse_reference(os.path.join(root, name))


def _export_files(folder):
    refuse_ps2_export(folder)
    return sorted(name for name in os.listdir(folder) if os.path.isfile(os.path.join(folder, name)))


def tod(t):
    g = time.gmtime(t)
    return struct.pack("<BBBBBBH", 0, g.tm_sec, g.tm_min, g.tm_hour, g.tm_mday, g.tm_mon, g.tm_year)


def header(mode, length, name, t):
    raw = name.encode("latin-1")
    if len(raw) > 447:
        raise ValueError("name too long: " + name)
    return (struct.pack("<II", mode, length) + tod(t) + struct.pack("<II", 0, 0) + tod(t) + bytes(32)
            + raw + bytes(ENTRY - 0x40 - len(raw)))


def pack_dir(folder):
    folder = os.path.abspath(folder)
    name = os.path.basename(folder)
    files = _export_files(folder)
    t = os.path.getmtime(folder)
    out = [header(MODE_DIR, len(files) + 2, name, t), header(MODE_DIR, 0, ".", t), header(MODE_DIR, 0, "..", t)]
    for f in files:
        data = open(os.path.join(folder, f), "rb").read()
        out.append(header(MODE_FILE, len(data), f, os.path.getmtime(os.path.join(folder, f))))
        out.append(data + bytes(-len(data) % PAD))
    return b"".join(out)


def parse(blob):
    """Returns (folder name, [(file name, data)])."""
    def entry(off):
        if off + ENTRY > len(blob):
            raise ValueError("truncated .psu")
        mode, length = struct.unpack_from("<II", blob, off)
        name = blob[off + 0x40:off + ENTRY].split(b"\0")[0].decode("latin-1")
        return mode, length, name

    mode, count, folder = entry(0)
    if not mode & 0x20:       # sceMcFileAttrSubdir
        raise ValueError("first entry is not a directory (mode %#x)" % mode)
    off = ENTRY
    files = []
    for _ in range(count):
        mode, length, name = entry(off)
        off += ENTRY
        if name in (".", ".."):
            continue
        data = blob[off:off + length]
        if len(data) != length:
            raise ValueError("truncated file %s" % name)
        files.append((name, data))
        off += length + (-length % PAD)
    return folder, files




class Card:
    """A raw PS2 memory card image (see the module docstring)."""

    def __init__(self, blob):
        if blob[:28] != b"Sony PS2 Memory Card Format ":
            raise ValueError("not a PS2 memory card image (no superblock magic)")
        self.blob = blob
        (self.page, self.ppc, self.ppb, self.clusters, self.alloc, self.alloc_end, self.root) = struct.unpack_from("<HHHxxIIII", blob, 0x28)
        self.ifc = struct.unpack_from("<32I", blob, 0x50)
        self.stride = self.page + (16 if len(blob) == self.clusters * self.ppc * (self.page + 16) else 0)
        if len(blob) != self.clusters * self.ppc * self.stride:
            raise ValueError("card image size %d does not match its superblock" % len(blob))
        self.csize = self.page * self.ppc
        self.epc = self.csize // 4

    def cluster(self, n):
        """The bytes of absolute cluster n."""
        if not 0 <= n < self.clusters:
            raise ValueError("cluster %d out of range" % n)
        return b"".join(self.blob[(n * self.ppc + i) * self.stride:(n * self.ppc + i) * self.stride + self.page] for i in range(self.ppc))

    def fat(self, n):
        fat_cluster = n // self.epc
        ifc = self.ifc[fat_cluster // self.epc]
        fc = struct.unpack_from("<I", self.cluster(ifc), (fat_cluster % self.epc) * 4)[0]
        return struct.unpack_from("<I", self.cluster(fc), (n % self.epc) * 4)[0]

    def chain(self, n):
        """The data of the chain starting at allocatable cluster n."""
        out, seen = [], set()
        while True:
            if n in seen:
                raise ValueError("loop in the FAT chain")
            seen.add(n)
            out.append(self.cluster(self.alloc + n))
            e = self.fat(n)
            n = e & 0x7FFFFFFF
            if n == 0x7FFFFFFF or not e & 0x80000000:      # end of chain (the marker is 0xFFFFFFFF; free entries are 0x7FFFFFFF)
                return b"".join(out)

    @staticmethod
    def entry(raw):
        mode, length = struct.unpack_from("<II", raw, 0)
        cluster, parent = struct.unpack_from("<II", raw, 16)
        return {"mode": mode, "length": length, "cluster": cluster, "parent": parent, "name": raw[64:96].split(b"\0")[0].decode("latin-1")}

    def entries(self, cluster, count=None):
        data = self.chain(cluster)
        if count is None:
            count = self.entry(data[:512])["length"]
        return [self.entry(data[i * 512:(i + 1) * 512]) for i in range(count)]

    def read(self, e):
        if e["length"] == 0 or e["cluster"] == 0xFFFFFFFF:      # an empty file has no cluster
            return b""
        return self.chain(e["cluster"])[:e["length"]]

    def saves(self):
        """(folder name, [(file name, data)]) for every directory in the root."""
        out = []
        for d in self.entries(self.root):
            if not d["mode"] & 0x20 or d["mode"] & 0x8000 == 0 or d["name"].startswith("."):
                continue
            files = [(f["name"], self.read(f)) for f in self.entries(d["cluster"], d["length"]) if f["mode"] & 0x10 and f["mode"] & 0x8000]
            out.append((d["name"], files))
        return out




def build_card(saves, ecc=True, clusters=8192):
    """A formatted card image holding saves = [(folder, [(name, data)])]: the layout PCSX2 and the console write (superblock in
    cluster 0, the indirect FAT in cluster 8, one FAT cluster in 9, data from cluster 41; used by the self test and to prepare cards)."""
    mod_witness.refuse_fidelity()
    for _, files in saves:
        _refuse_native_names(name for name, _ in files)
    page, ppc, csize = 512, 2, 1024
    alloc = 41
    fat = [0x7FFFFFFF] * 256              # one FAT cluster covers 256 clusters; a test card never needs more
    data = {}                              # allocatable cluster -> bytes
    nxt = [0]

    def store(blob):
        blob = blob or b""
        need = max(1, -(-len(blob) // csize))
        first = nxt[0]
        for i in range(need):
            c = nxt[0]
            nxt[0] += 1
            data[c] = blob[i * csize:(i + 1) * csize].ljust(csize, b"\0")
            fat[c] = 0x80000000 | (c + 1 if i + 1 < need else 0x7FFFFFFF)
        return first

    def entry(mode, length, cluster, name, parent=0):
        t = tod(0)
        return struct.pack("<II8sII8sI28s32s", mode, length, t, cluster, parent, t, 0, bytes(28), name.encode("latin-1")) + bytes(0x1A0)

    # reserve the root's cluster first (index 0), fill it last
    root_cluster = nxt[0]
    nxt[0] += 1
    fat[0] = 0xFFFFFFFF
    root_entries = [entry(MODE_DIR, 2 + len(saves), 0, "."), entry(MODE_DIR, 0, 0, "..")]
    for folder, files in saves:
        fe = [entry(MODE_DIR, 2 + len(files), 0, "."), entry(MODE_DIR, 0, 0, "..")]
        for name, blob in files:
            fe.append(entry(MODE_FILE, len(blob), store(blob) if blob else 0xFFFFFFFF, name))
        root_entries.append(entry(MODE_DIR, len(fe), store(b"".join(fe)), folder))
    data[0] = b"".join(root_entries).ljust(csize, b"\0") if len(root_entries) * 512 <= csize else None
    if data[0] is None:                    # a root of more than two entries needs a chain
        blob = b"".join(root_entries)
        need = -(-len(blob) // csize)
        ids = [root_cluster] + [nxt[0] + i for i in range(need - 1)]
        nxt[0] += need - 1
        for i, c in enumerate(ids):
            data[c] = blob[i * csize:(i + 1) * csize].ljust(csize, b"\0")
            fat[c] = 0x80000000 | (ids[i + 1] if i + 1 < need else 0x7FFFFFFF)
        fat[root_cluster] = 0x80000000 | ids[1]
    else:
        fat[root_cluster] = 0xFFFFFFFF
    sb = bytearray(csize)
    sb[0:40] = b"Sony PS2 Memory Card Format 1.2.0.0".ljust(40, b"\0")
    struct.pack_into("<HHHHIIII", sb, 0x28, page, ppc, 16, 0, clusters, alloc, clusters - alloc, root_cluster)
    struct.pack_into("<II", sb, 0x40, 0x3FF, 0x3FE)
    sb[0x50:0x54] = struct.pack("<I", 8)                 # indirect FAT cluster list: cluster 8
    sb[0xD0:0xD4] = b"\xff\xff\xff\xff"
    sb[0x150] = 2
    sb[0x151] = 0x2B
    image = bytearray(clusters * csize)
    image[0:csize] = sb
    ifc = bytearray(csize)
    ifc[0:4] = struct.pack("<I", 9)                      # FAT cluster 0 is cluster 9
    image[8 * csize:9 * csize] = ifc
    image[9 * csize:10 * csize] = struct.pack("<256I", *fat)
    for c, blob in data.items():
        image[(alloc + c) * csize:(alloc + c + 1) * csize] = blob
    if not ecc:
        return bytes(image)
    out = bytearray()
    for p in range(clusters * ppc):
        out += image[p * page:(p + 1) * page] + bytes(16)
    return bytes(out)


def selftest():
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "BASLUS-21782Save0")
        os.makedirs(src)
        blobs = {"BASLUS-21782Save0": bytes(range(256)) * 700 + b"x", "icon.ico": b"\1\2\3" * 5000, "icon.sys": b"PS2D" + bytes(960)}
        for n, d in blobs.items():
            open(os.path.join(src, n), "wb").write(d)
        psu = os.path.join(tmp, "x.psu")
        open(psu, "wb").write(pack_dir(src))
        folder, files = parse(open(psu, "rb").read())
        ok = dict(files) == blobs and folder == "BASLUS-21782Save0" and len(open(psu, "rb").read()) % PAD == 0
        # the same save on a card image (with and without ECC bytes), next to an empty file and a second save
        other = ("BASLUS-21782Save1", [("BASLUS-21782Save1", b"\x55" * 3000), ("icon.sys", b"PS2D" + bytes(960)), ("empty", b"")])
        for ecc in (True, False):
            card = os.path.join(tmp, "card%d.ps2" % ecc)
            open(card, "wb").write(build_card([("BASLUS-21782Save0", list(blobs.items())), other], ecc=ecc))
            saves = dict(Card(open(card, "rb").read()).saves())
            ok = ok and saves.get("BASLUS-21782Save0") == list(blobs.items()) and saves.get("BASLUS-21782Save1") == other[1]
        print("ps2save selftest:", "PASS" if ok else "FAIL", "(%d files, %d bytes)" % (len(files), os.path.getsize(psu)))
        return 0 if ok else 1


def main(argv):
    if len(argv) >= 2 and argv[1] == "selftest":
        return selftest()
    if len(argv) == 4 and argv[1] == "dir2psu":
        blob = pack_dir(argv[2])
        with open(argv[3], "wb") as output:
            output.write(blob)
        return 0
    if len(argv) == 3 and argv[1] == "mcdlist":
        for folder, files in Card(open(argv[2], "rb").read()).saves():
            print(folder)
            for n, d in files:
                print("  %-24s %8d" % (n, len(d)))
        return 0
    if len(argv) in (4, 5) and argv[1] == "mkcard":
        folder = os.path.basename(os.path.abspath(argv[2]))
        new = argv[4] if len(argv) == 5 else folder
        files = [((new if n == folder else n), open(os.path.join(argv[2], n), "rb").read()) for n in _export_files(argv[2])]
        blob = build_card([(new, files)])
        with open(argv[3], "wb") as output:
            output.write(blob)
        return 0
    if len(argv) == 3 and argv[1] == "list":
        folder, files = parse(open(argv[2], "rb").read())
        print(folder)
        for n, d in files:
            print("  %-24s %8d" % (n, len(d)))
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
