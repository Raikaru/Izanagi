"""Small deterministic ISO/CVM fixtures containing only caller-supplied test bytes."""
import struct

SECTOR = 2048
CVM_HEADER = 0x1800
CONTAINERS = {"DATA": "DATA.CVM", "BTL": "BTL.CVM", "BGM": "BGM.CVM", "ENV": "ENV.CVM", "ZZZZZ": "ZZZZZ.BIN"}


def _record(name, extent, size, directory):
    identifier = name if isinstance(name, bytes) else name.encode("ascii")
    length = 33 + len(identifier)
    length += length & 1
    result = bytearray(length)
    result[0] = length
    result[2:10] = struct.pack("<I", extent) + struct.pack(">I", extent)
    result[10:18] = struct.pack("<I", size) + struct.pack(">I", size)
    result[18:25] = bytes((100, 1, 1, 0, 0, 0, 0))
    result[25] = 2 if directory else 0
    result[28:32] = struct.pack("<H", 1) + struct.pack(">H", 1)
    result[32] = len(identifier)
    result[33:33 + len(identifier)] = identifier
    return result


def volume(files):
    tree = {}
    for path, data in sorted(files.items()):
        node = tree
        pieces = path.split("/")
        for part in pieces[:-1]:
            node = node.setdefault(part, {})
        node[pieces[-1]] = data
    directories = [("", tree, "")]
    for prefix, node, _ in directories:
        for name, child in sorted(node.items()):
            if isinstance(child, dict):
                directories.append((f"{prefix}/{name}".strip("/"), child, prefix))
    extents = {prefix: 20 + index for index, (prefix, _, _) in enumerate(directories)}
    cursor = 20 + len(directories)
    for name, data in sorted(files.items()):
        extents[name] = cursor
        cursor += (len(data) + SECTOR - 1) // SECTOR
    image = bytearray(cursor * SECTOR)
    for prefix, node, parent in directories:
        records = _record(b"\0", extents[prefix], SECTOR, True)
        records += _record(b"\1", extents[parent], SECTOR, True)
        for name, child in sorted(node.items()):
            path = f"{prefix}/{name}".strip("/")
            directory = isinstance(child, dict)
            records += _record(name if directory else name + ";1", extents[path], SECTOR if directory else len(child), directory)
        if len(records) > SECTOR:
            raise ValueError("synthetic fixture directory exceeds one sector")
        start = extents[prefix] * SECTOR
        image[start:start + len(records)] = records
    for name, data in files.items():
        start = extents[name] * SECTOR
        image[start:start + len(data)] = data
    pvd = memoryview(image)[16 * SECTOR:17 * SECTOR]
    pvd[0:7] = b"\1CD001\1"
    pvd[80:88] = struct.pack("<I", cursor) + struct.pack(">I", cursor)
    pvd[128:132] = struct.pack("<H", SECTOR) + struct.pack(">H", SECTOR)
    pvd[156:190] = _record(b"\0", extents[""], SECTOR, True)
    image[17 * SECTOR:17 * SECTOR + 7] = b"\xffCD001\1"
    return bytes(image)


def disc(containers=None, root_files=None):
    containers = containers or {}
    root = dict(root_files or {})
    for label, filename in CONTAINERS.items():
        root[filename] = b"CVMH" + bytes(CVM_HEADER - 4) + volume(containers.get(label, {}))
    return volume(root)
