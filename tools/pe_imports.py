#!/usr/bin/env python3
"""Lists the DLLs a Windows exe imports (its import table and delay-load table), lower case, one per line: python tools/pe_imports.py p4pc.exe"""
import struct
import sys


def imports(path):
    d = open(path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    assert d[pe:pe + 4] == b"PE\0\0", "not a PE file"
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    opt = pe + 24
    magic = struct.unpack_from("<H", d, opt)[0]
    dd = opt + (96 if magic == 0x10B else 112)  # data directories
    secs = []
    sec0 = opt + optsz
    for i in range(nsec):
        o = sec0 + 40 * i
        vsz, va, rsz, raw = struct.unpack_from("<IIII", d, o + 8)
        secs.append((va, max(vsz, rsz), raw))

    def off(rva):
        for va, sz, raw in secs:
            if va <= rva < va + sz:
                return raw + rva - va
        return None

    names = set()
    for idx, size in ((1, 20), (13, 32)):  # import table, delay-load table
        rva, tsz = struct.unpack_from("<II", d, dd + 8 * idx)
        o = off(rva) if rva else None
        while o is not None:
            ent = d[o:o + size]
            if len(ent) < size or ent == b"\0" * size:
                break
            nrva = struct.unpack_from("<I", ent, 12 if idx == 1 else 4)[0]
            no = off(nrva)
            if no is None:
                break
            names.add(d[no:d.index(b"\0", no)].decode("ascii").lower())
            o += size
    return sorted(names)


if __name__ == "__main__":
    for n in imports(sys.argv[1]):
        print(n)
