"""Reads the user's own retail battle tables from the disc at test time.

Mod-witness fixtures derive their packages from these bytes in a private work
directory; no retail byte is ever bundled or committed."""
import struct

SECTOR = 2048
CVM_HEADER = 0x1800


class Volume:
    """An ISO9660 volume at a byte offset of an open image (the disc or a CVM)."""

    def __init__(self, handle, base):
        self.handle, self.base = handle, base

    def read(self, lba, size):
        self.handle.seek(self.base + lba * SECTOR)
        data = self.handle.read(size)
        if len(data) != size:
            raise ValueError("truncated ISO9660 extent")
        return data

    def _entries(self, lba, size):
        data, offset, entries = self.read(lba, size), 0, {}
        while offset < len(data):
            length = data[offset]
            if not length:
                offset = (offset // SECTOR + 1) * SECTOR
                continue
            extent, extent_size = struct.unpack_from("<I", data, offset + 2)[0], struct.unpack_from("<I", data, offset + 10)[0]
            name = data[offset + 33:offset + 33 + data[offset + 32]]
            if name not in (b"\0", b"\1"):
                entries[name.decode("ascii").split(";")[0].upper()] = (extent, extent_size)
            offset += length
        return entries

    def find(self, path):
        descriptor = self.read(16, SECTOR)
        if descriptor[1:6] != b"CD001":
            raise ValueError("not an ISO9660 volume")
        lba, size = struct.unpack_from("<I", descriptor, 158)[0], struct.unpack_from("<I", descriptor, 166)[0]
        for part in path.upper().split("/"):
            lba, size = self._entries(lba, size)[part]
        return lba, size

    def file(self, path):
        return self.read(*self.find(path))


def battle_files(iso, names):
    """{name: bytes} for BTL.CVM BATTLE/<name> files of the disc image."""
    with open(iso, "rb") as handle:
        lba, _ = Volume(handle, 0).find("BTL.CVM")
        container = Volume(handle, lba * SECTOR + CVM_HEADER)
        return {name: container.file("BATTLE/" + name) for name in names}


def data_file(iso, path):
    """The bytes of DATA.CVM <path> (for example INIT/ITEMTBL.BIN) of the disc image."""
    with open(iso, "rb") as handle:
        lba, _ = Volume(handle, 0).find("DATA.CVM")
        return Volume(handle, lba * SECTOR + CVM_HEADER).file(path)


def blocks(packed):
    """The size-prefixed, 16-byte aligned blocks of a packed battle table."""
    result, offset = [], 0
    while offset + 4 <= len(packed):
        size = struct.unpack_from("<I", packed, offset)[0]
        if size > len(packed) - offset - 4:
            break
        result.append(packed[offset + 4:offset + 4 + size])
        offset += (size + 4 + 15) & ~15
    return result
