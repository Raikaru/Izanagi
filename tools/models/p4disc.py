"""Read-only access to the user's own Persona 4 disc image: the five CVM containers and PAC packs.

The layout is the one src/platform/files/vfs.c indexes: the ISO 9660 root holds DATA.CVM, BTL.CVM,
BGM.CVM, ENV.CVM and ZZZZZ.BIN; each CVM starts with a 0x1800-byte CVMH header followed by its own
ISO 9660 volume whose extent LBAs are relative to that base. Keys are the VFS keys
("DATA/MODEL/WEAPON/WP000.RMD"); a PAC member is addressed as "<pac key>#<member name>".

PAC (and the field .P00/.P01 packs) are flat: a 252-byte NUL-padded name, a little-endian u32 size,
the payload, then padding to the next 64-byte boundary (retail packs pad to 64 or 128; both are
found by scanning for the next plausible header).

Nothing here writes to the disc or copies it; callers read single files into memory.
"""
import os
import struct

CONTAINERS = ("DATA.CVM", "BTL.CVM", "BGM.CVM", "ENV.CVM", "ZZZZZ.BIN")
CVM_HEADER = 0x1800
SECTOR = 2048


def default_iso():
    """Return the existing disc image named by P4PC_ISO, or None."""
    path = os.environ.get("P4PC_ISO")
    return path if path and os.path.isfile(path) else None


class Disc:
    def __init__(self, iso_path):
        self.path = iso_path
        self._fp = open(iso_path, "rb")
        self.files = {}
        top = self._walk(0)
        missing = [name for name in CONTAINERS if name not in top]
        if missing:
            raise ValueError("%s is not a complete Persona 4 disc (missing %s)" % (iso_path, ", ".join(missing)))
        for name in CONTAINERS:
            base, _size = top[name]
            if self._read(base, 4) != b"CVMH":
                raise ValueError("%s has no CVMH header" % name)
            label = name.split(".")[0]
            for key, extent in self._walk(base + CVM_HEADER).items():
                self.files[label + "/" + key] = extent

    def close(self):
        self._fp.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def _read(self, offset, size):
        self._fp.seek(offset)
        return self._fp.read(size)

    def _walk(self, base):
        pvd = self._read(base + 16 * SECTOR, SECTOR)
        if pvd[1:6] != b"CD001":
            raise ValueError("no ISO 9660 volume at offset 0x%x" % base)
        root = pvd[156:156 + 34]
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
                flags, name_length = record[25], record[32]
                name = record[33:33 + name_length]
                if name in (b"\0", b"\1"):
                    continue
                name = name.decode("ascii").split(";")[0]
                key = prefix + name
                if flags & 2:
                    visit(extent, extent_size, key + "/")
                else:
                    out[key] = (base + extent * SECTOR, extent_size)

        visit(struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0], "")
        return out

    def keys(self, prefix=""):
        prefix = prefix.upper()
        return sorted(key for key in self.files if key.startswith(prefix))

    def read(self, key):
        """Bytes of a disc file, or of a PAC member for "<key>#<member>" (member match is case-insensitive)."""
        key, _, member = key.partition("#")
        key = key.replace("\\", "/").upper()
        if key not in self.files:
            raise KeyError("not on the disc: %s" % key)
        offset, size = self.files[key]
        data = self._read(offset, size)
        if member:
            return pac_member(data, member)
        return data


def pac_entries(data):
    """[(name, offset, size)] of a flat pack."""
    entries = []
    offset = 0
    while offset + 256 <= len(data):
        raw = data[offset:offset + 252].split(b"\0", 1)[0]
        if not raw or any(c < 0x20 or c > 0x7E for c in raw):
            break
        size = struct.unpack_from("<I", data, offset + 252)[0]
        start = offset + 256
        if start + size > len(data):
            raise ValueError("pack entry %r overruns the pack" % raw)
        entries.append((raw.decode("ascii"), start, size))
        offset = (start + size + 63) // 64 * 64
        # Some packs align to 128: skip zero padding up to the next nonzero name byte.
        while offset + 256 <= len(data) and data[offset] == 0 and offset % 128 != 0:
            offset += 64
    return entries


def pac_member(data, member):
    for name, start, size in pac_entries(data):
        if name.lower() == member.lower():
            return data[start:start + size]
    raise KeyError("pack has no member %s" % member)


def load(spec, disc=None):
    """Bytes for a local file path, a "<file>#<member>" pack member, or a disc key."""
    path, _, member = spec.partition("#")
    if os.path.isfile(path):
        with open(path, "rb") as stream:
            data = stream.read()
        return pac_member(data, member) if member else data
    if disc is None:
        raise FileNotFoundError("%s is neither a local file nor (without a disc) a disc key" % spec)
    return disc.read(spec)
