"""Parser for Persona 4's RenderWare 3.7 model data (RMD files and the RW streams inside them).

Read-only: it decodes the chunk tree into plain Python objects and keeps the bytes of every chunk it
does not interpret, so a converter can carry them along. docs/modding/models.md documents each
layout and the decomp function that reads it; the comments below name those functions.

Chunk header: u32 type, u32 payload size, u32 library id. The library id of RW 3.1+ streams encodes
the version as ((v - 0x30000) & 0x3FF00) << 14 | (v & 0x3F) << 16 | build; P4 writes 0x1C020037
(RW 3.7.0.2, build 0x37). Atlus's own RMD chunks (0xF0F000xx) carry 0x40000000.
"""
import base64
import math
import struct

# Core chunk ids (include/rw/inc/rwplcore.h / rwcore.h enums).
STRUCT, STRING, EXTENSION = 0x01, 0x02, 0x03
TEXTURE, MATERIAL, MATLIST, FRAMELIST, GEOMETRY, CLUMP, ATOMIC = 0x06, 0x07, 0x08, 0x0E, 0x0F, 0x10, 0x14
TEXTURENATIVE, TEXDICTIONARY, GEOMETRYLIST, ANIMANIMATION = 0x15, 0x16, 0x1A, 0x1B
PITEXDICTIONARY, UVANIMDICT = 0x23, 0x2B
# Plugin ids seen in P4 extensions (vendor 1 toolkit ids, plus the core right-to-render and bin mesh).
RIGHTTORENDER, SKYMIPMAP, SKIN, COLLISION, HANIM, USERDATA, MATFX, UVANIM, BINMESH = (
    0x1F, 0x110, 0x116, 0x11D, 0x11E, 0x11F, 0x120, 0x135, 0x50E)

# Atlus RMD chunk ids read by mdlReadAuxChunks (decomp src/Graphics/Model/mdlManager.c, FUN_0047B0C0).
RMD_PLACEHOLDER = 0xF0F00001   # slot gets the shared placeholder animation
RMD_NEXT_LAYER = 0xF0F00002    # layer++, slot = 0
RMD_ALIAS = 0xF0F00003         # u16 source slot: copy its animation, matrix, material/UV anims, effects
RMD_NEXT_SLOT = 0xF0F00004     # slot++
RMD_SLOT_MATRIX = 0xF0F00005   # 64-byte RwMatrix stored in the slot
RMD_ATTACH_TABLE = 0xF0F00006  # u16 count, then (s32 id, s32 HAnim node id, RwMatrix) per entry
RMD_START_FRAME_A = 0xF0F00007
RMD_START_FRAME_B = 0xF0F00008
RMD_BASE_ANIMATION = 0xF0F00009
RMD_SLOT_PROPERTIES = 0xF0F00070  # 0x2B8-byte record (mdlBuildScaledSlotRecords)
RMD_MATANIM = (0xF0F00080, 0xF0F00081, 0xF0F00082, 0xF0F00083)  # mdlMatAnimLoadChunk channels
RMD_BLEND_GROUP = 0xF0F000D0
RMD_EFFECT = 0xF0F000E0
RMD_EFFECT_COPY = 0xF0F000E1
RMD_SLOT_COUNT = 0xF0F000F0   # u16 animation slot capacity

PLATFORM_PS2 = 0x00325350  # "PS2\0"

CHUNK_NAMES = {
    STRUCT: "struct", STRING: "string", EXTENSION: "extension", TEXTURE: "texture", MATERIAL: "material",
    MATLIST: "material list", FRAMELIST: "frame list", GEOMETRY: "geometry", CLUMP: "clump",
    ATOMIC: "atomic", TEXTURENATIVE: "native texture", TEXDICTIONARY: "texture dictionary",
    GEOMETRYLIST: "geometry list", ANIMANIMATION: "animation", PITEXDICTIONARY: "PI texture dictionary",
    UVANIMDICT: "UV animation dictionary", RIGHTTORENDER: "right to render", SKYMIPMAP: "sky mipmap",
    SKIN: "skin", COLLISION: "collision", HANIM: "HAnim", USERDATA: "user data", MATFX: "material effects",
    UVANIM: "UV animation", BINMESH: "bin mesh",
    RMD_PLACEHOLDER: "rmd placeholder animation", RMD_NEXT_LAYER: "rmd next layer", RMD_ALIAS: "rmd slot alias",
    RMD_NEXT_SLOT: "rmd next slot", RMD_SLOT_MATRIX: "rmd slot matrix", RMD_ATTACH_TABLE: "rmd attach table",
    RMD_START_FRAME_A: "rmd start frame a", RMD_START_FRAME_B: "rmd start frame b",
    RMD_BASE_ANIMATION: "rmd base animation", RMD_SLOT_PROPERTIES: "rmd slot properties",
    RMD_BLEND_GROUP: "rmd blend group", RMD_EFFECT: "rmd effect", RMD_EFFECT_COPY: "rmd effect copy",
    RMD_SLOT_COUNT: "rmd slot count", 0xF0F00080: "rmd material anim 0", 0xF0F00081: "rmd material anim 1",
    0xF0F00082: "rmd material anim 2", 0xF0F00083: "rmd material anim 3",
}

# Geometry format flags (rpGEOMETRY*).
GEO_TRISTRIP, GEO_POSITIONS, GEO_TEXTURED, GEO_PRELIT, GEO_NORMALS, GEO_LIGHT, GEO_MODULATE, GEO_TEXTURED2 = (
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80)
GEO_NATIVE = 0x01000000


class FormatError(ValueError):
    pass


class UnsupportedAnimation(FormatError):
    """An RtAnim animation of a registered type other than the two HAnim keyframe schemes."""


def library_version(libid):
    """(version, build) of a chunk library id."""
    if libid & 0xFFFF0000:
        return (((libid >> 14) & 0x3FF00) + 0x30000) | ((libid >> 16) & 0x3F), libid & 0xFFFF
    return libid << 8, 0


def chunk_list(data, start, end, where="stream"):
    """[(type, libid, payload start, payload end)] of the chunks between start and end."""
    out = []
    offset = start
    while offset + 12 <= end:
        kind, size, libid = struct.unpack_from("<III", data, offset)
        if offset + 12 + size > end:
            raise FormatError("%s: chunk 0x%x at 0x%x overruns its parent (size 0x%x)" % (where, kind, offset, size))
        out.append((kind, libid, offset + 12, offset + 12 + size))
        offset += 12 + size
    return out


class Reader:
    def __init__(self, data, start, end, where):
        self.data, self.pos, self.end, self.where = data, start, end, where

    def need(self, size):
        if self.pos + size > self.end:
            raise FormatError("%s: truncated at 0x%x (need %d bytes)" % (self.where, self.pos, size))

    def take(self, fmt):
        size = struct.calcsize(fmt)
        self.need(size)
        value = struct.unpack_from(fmt, self.data, self.pos)
        self.pos += size
        return value

    def one(self, fmt):
        return self.take(fmt)[0]

    def rows(self, code, width, count):
        """count tuples of width values of struct code (one bulk unpack)."""
        flat = self.take("<%d%s" % (width * count, code))
        return [flat[i:i + width] for i in range(0, len(flat), width)]

    def raw(self, size):
        self.need(size)
        value = self.data[self.pos:self.pos + size]
        self.pos += size
        return value

    def rest(self):
        return self.raw(self.end - self.pos)


def _expect(chunk, kind, where):
    if chunk[0] != kind:
        raise FormatError("%s: expected %s chunk, found 0x%x" % (where, CHUNK_NAMES.get(kind, hex(kind)), chunk[0]))


def _string(data, chunk):
    return data[chunk[2]:chunk[3]].split(b"\0", 1)[0].decode("latin-1")


# ---------------------------------------------------------------------------------------------------
# Plugins

class UserDataArray:
    """RpUserDataArray (rpusrdat.c): name, format (1 int, 2 float, 3 string) and elements."""
    INT, FLOAT, STRING = 1, 2, 3

    def __init__(self, name, fmt, values):
        self.name, self.format, self.values = name, fmt, values

    def to_json(self):
        return {"name": self.name, "format": {1: "int", 2: "float", 3: "string"}.get(self.format, self.format),
                "values": self.values}


def parse_user_data(data, start, end, where):
    r = Reader(data, start, end, where)
    arrays = []
    for _ in range(r.one("<i")):
        length = r.one("<i")
        name = r.raw(length).split(b"\0", 1)[0].decode("latin-1")
        fmt, count = r.take("<ii")
        values = []
        for _ in range(count):
            if fmt == UserDataArray.INT:
                values.append(r.one("<i"))
            elif fmt == UserDataArray.FLOAT:
                values.append(r.one("<f"))
            elif fmt == UserDataArray.STRING:
                values.append(r.raw(r.one("<i")).split(b"\0", 1)[0].decode("latin-1"))
            else:
                raise FormatError("%s: user data format %d" % (where, fmt))
        arrays.append(UserDataArray(name, fmt, values))
    return arrays


class Extension:
    """An extension chunk: the plugins it holds, decoded where known, raw bytes kept for the rest."""

    def __init__(self):
        self.plugins = []  # [(plugin id, libid, payload bytes)] in stream order

    def raw(self, plugin):
        for pid, _libid, payload in self.plugins:
            if pid == plugin:
                return payload
        return None

    def ids(self):
        return [pid for pid, _, _ in self.plugins]

    def opaque_json(self, known):
        return [{"plugin": "0x%x" % pid, "bytes": base64.b64encode(payload).decode("ascii")}
                for pid, _libid, payload in self.plugins if pid not in known]


def parse_extension(data, chunk, where):
    _expect(chunk, EXTENSION, where)
    ext = Extension()
    for kind, libid, start, end in chunk_list(data, chunk[2], chunk[3], where + " extension"):
        ext.plugins.append((kind, libid, data[start:end]))
    return ext


class HAnimPlugin:
    """RpHAnim frame extension (rphanim.c): every bone frame has an id; the hierarchy root also lists
    the nodes (id, index, flags: 1 pop, 2 push) in depth-first order."""

    def __init__(self, payload, where):
        r = Reader(payload, 0, len(payload), where)
        self.version, self.node_id, count = r.take("<iii")
        self.flags = self.keyframe_size = None
        self.nodes = []
        if count:
            self.flags, self.keyframe_size = r.take("<ii")
            self.nodes = [r.take("<iii") for _ in range(count)]


# ---------------------------------------------------------------------------------------------------
# Frames, geometry, materials, atomics, clump (baframe.c, bageomet.c, bamateri.c, baclump.c)

class Frame:
    def __init__(self, rotation, position, parent, flags):
        self.rotation = rotation  # right, up, at rows (9 floats)
        self.position = position
        self.parent = parent
        self.flags = flags
        self.extension = Extension()
        self.hanim = None
        self.user_data = []


class Texture:
    def __init__(self):
        self.filter_addressing = 0
        self.name = ""
        self.mask = ""
        self.extension = Extension()

    @property
    def filter(self):
        return self.filter_addressing & 0xFF

    @property
    def address_u(self):
        return (self.filter_addressing >> 8) & 0xF

    @property
    def address_v(self):
        return (self.filter_addressing >> 12) & 0xF


class MatFx:
    """rpMatFX material data (RpMatFX material stream): the material's effect type, then two passes,
    each an effect type and its data: bump map (coefficient, optional bumped texture, optional texture),
    environment map (coefficient, frame-buffer alpha flag, optional texture), dual (source and
    destination blend modes, optional texture), UV transform (no data; the matrices come from the UV
    animation plugin)."""
    NAMES = {0: "null", 1: "bumpmap", 2: "envmap", 3: "bumpenvmap", 4: "dual", 5: "uvtransform", 6: "dualuvtransform"}

    def __init__(self, payload, where="matfx"):
        self.payload = payload
        r = Reader(payload, 0, len(payload), where)
        self.effect = r.one("<I")
        self.passes = []
        for _ in range(2):
            kind = r.one("<I")
            info = {"effect": self.NAMES.get(kind, kind)}
            if kind == 1:
                info["coefficient"] = r.one("<f")
                info["bumpedTexture"] = self._texture(r, where)
                info["texture"] = self._texture(r, where)
            elif kind == 2:
                info["coefficient"] = r.one("<f")
                info["frameBufferAlpha"] = r.one("<i")
                info["texture"] = self._texture(r, where)
            elif kind == 4:
                info["srcBlend"], info["dstBlend"] = r.take("<ii")
                info["texture"] = self._texture(r, where)
            elif kind not in (0, 5):
                raise FormatError("%s: MatFX pass effect %d" % (where, kind))
            self.passes.append(info)
        if r.pos != r.end:
            raise FormatError("%s: %d trailing MatFX bytes" % (where, r.end - r.pos))

    @staticmethod
    def _texture(r, where):
        if not r.one("<i"):
            return None
        kind, size, libid = r.take("<III")
        start = r.pos
        r.raw(size)
        return parse_texture(r.data, (kind, libid, start, start + size), where + " texture")

    @property
    def name(self):
        return self.NAMES.get(self.effect, str(self.effect))


class Material:
    def __init__(self):
        self.flags = 0
        self.color = (255, 255, 255, 255)
        self.unused = 0
        self.textured = 0
        self.surface = (1.0, 1.0, 1.0)  # ambient, specular, diffuse
        self.texture = None
        self.extension = Extension()
        self.user_data = []
        self.matfx = None
        self.uv_animations = []  # (slot bit, UV animation name) from the UV animation plugin

    def user_value(self, name, default=None):
        for array in self.user_data:
            if array.name == name and array.values:
                return array.values[0]
        return default


class Skin:
    """RpSkin geometry data, non-native stream form (skingeom stream read, rpskin.c): bone count, used bone
    list, 4 bone indices and 4 weights per vertex, one inverse bone matrix per bone, then the split
    data (bone limit, mesh count, RLE count and their tables) kept raw."""

    def __init__(self, payload, vertices, where):
        r = Reader(payload, 0, len(payload), where)
        self.num_bones, num_used, self.max_weights, self.pad = r.take("<4B")
        if num_used == 0 and self.num_bones:
            raise FormatError("%s: pre-3.4 skin layout (no used-bone list) is not used by P4" % where)
        self.used_bones = list(r.raw(num_used))
        self.indices = r.raw(vertices * 4)
        self.weights = r.take("<%df" % (vertices * 4))
        start = r.pos
        self.matrices = [r.take("<16f") for _ in range(self.num_bones)]
        # The fourth column of each RwMatrix (flags and pads) is whatever the exporter's memory held;
        # keep its exact words (some are NaN patterns).
        words = struct.unpack_from("<%dI" % (16 * self.num_bones), payload, start)
        self.matrix_pads = [words[16 * i + 3:16 * i + 16:4] for i in range(self.num_bones)]
        self.tail = r.rest()


class BinMesh:
    def __init__(self, payload, where):
        r = Reader(payload, 0, len(payload), where)
        self.flags, count, self.total = r.take("<III")
        self.meshes = []
        for _ in range(count):
            n, material = r.take("<II")
            self.meshes.append((material, r.take("<%dI" % n)))
        if r.pos != r.end:
            raise FormatError("%s: %d trailing bytes" % (where, r.end - r.pos))

    @property
    def strips(self):
        return bool(self.flags & 1)


class MorphTarget:
    def __init__(self):
        self.sphere = (0.0, 0.0, 0.0, 0.0)
        self.positions = None
        self.normals = None


class Geometry:
    def __init__(self):
        self.format = 0
        self.prelit = None
        self.uvs = []
        self.triangles = []  # (v0, v1, v2, material)
        self.morph_targets = []
        self.materials = []
        self.material_slots = []
        self.extension = Extension()
        self.binmesh = None
        self.skin = None
        self.user_data = []

    @property
    def num_vertices(self):
        return self._num_vertices

    @property
    def tex_sets(self):
        count = (self.format >> 16) & 0xFF
        if count == 0:
            count = 2 if self.format & GEO_TEXTURED2 else (1 if self.format & GEO_TEXTURED else 0)
        return count


def parse_texture(data, chunk, where):
    _expect(chunk, TEXTURE, where)
    parts = chunk_list(data, chunk[2], chunk[3], where)
    if len(parts) < 4:
        raise FormatError("%s: texture has %d chunks" % (where, len(parts)))
    tex = Texture()
    _expect(parts[0], STRUCT, where)
    tex.filter_addressing = struct.unpack_from("<I", data, parts[0][2])[0]
    tex.name = _string(data, parts[1])
    tex.mask = _string(data, parts[2])
    tex.extension = parse_extension(data, parts[3], where)
    return tex


def parse_material(data, chunk, where):
    _expect(chunk, MATERIAL, where)
    parts = chunk_list(data, chunk[2], chunk[3], where)
    mat = Material()
    _expect(parts[0], STRUCT, where)
    r = Reader(data, parts[0][2], parts[0][3], where)
    mat.flags = r.one("<i")
    mat.color = r.take("<4B")
    mat.unused, mat.textured = r.take("<ii")
    if r.end - r.pos >= 12:
        mat.surface = r.take("<3f")
    index = 1
    if mat.textured:
        mat.texture = parse_texture(data, parts[index], where + " texture")
        index += 1
    mat.extension = parse_extension(data, parts[index], where)
    payload = mat.extension.raw(USERDATA)
    if payload is not None:
        mat.user_data = parse_user_data(payload, 0, len(payload), where + " user data")
    payload = mat.extension.raw(MATFX)
    if payload is not None:
        mat.matfx = MatFx(payload, where + " MatFX")
    payload = mat.extension.raw(UVANIM)
    if payload is not None:
        # A struct chunk: a u32 slot mask, then one 32-byte animation name per set bit.
        if struct.unpack_from("<I", payload, 0)[0] != STRUCT:
            raise FormatError("%s: UV animation plugin without its struct chunk" % where)
        mask = struct.unpack_from("<I", payload, 12)[0]
        offset = 16
        for bit in range(32):
            if mask & (1 << bit):
                name = payload[offset:offset + 32].split(b"\0", 1)[0].decode("latin-1")
                mat.uv_animations.append((bit, name))
                offset += 32
    return mat


def parse_geometry(data, chunk, where):
    _expect(chunk, GEOMETRY, where)
    parts = chunk_list(data, chunk[2], chunk[3], where)
    geo = Geometry()
    _expect(parts[0], STRUCT, where)
    r = Reader(data, parts[0][2], parts[0][3], where + " struct")
    geo.format, num_tris, num_verts, num_morph = r.take("<Iiii")
    geo._num_vertices = num_verts
    version = library_version(chunk[1])[0]
    if version < 0x34000:
        r.take("<3f")
    if geo.format & GEO_NATIVE:
        raise FormatError("%s: pre-instanced native geometry (not used by P4 models)" % where)
    if geo.format & GEO_PRELIT:
        geo.prelit = r.rows("B", 4, num_verts)
    for _ in range(geo.tex_sets):
        geo.uvs.append(r.rows("f", 2, num_verts))
    words = r.take("<%dI" % (2 * num_tris))
    geo.triangles = [(words[i] >> 16, words[i] & 0xFFFF, words[i + 1] >> 16, words[i + 1] & 0xFFFF)
                     for i in range(0, len(words), 2)]
    for _ in range(num_morph):
        target = MorphTarget()
        target.sphere = r.take("<4f")
        has_positions, has_normals = r.take("<ii")
        if has_positions:
            target.positions = r.rows("f", 3, num_verts)
        if has_normals:
            target.normals = r.rows("f", 3, num_verts)
        geo.morph_targets.append(target)
    if r.pos != r.end:
        raise FormatError("%s: %d trailing struct bytes" % (where, r.end - r.pos))
    _expect(parts[1], MATLIST, where)
    mparts = chunk_list(data, parts[1][2], parts[1][3], where + " material list")
    mr = Reader(data, mparts[0][2], mparts[0][3], where + " material list")
    count = mr.one("<i")
    geo.material_slots = list(mr.take("<%di" % count))
    fresh = iter(mparts[1:])
    for slot, reuse in enumerate(geo.material_slots):
        if reuse < 0:
            geo.materials.append(parse_material(data, next(fresh), "%s material %d" % (where, slot)))
        else:
            geo.materials.append(geo.materials[reuse])
    geo.extension = parse_extension(data, parts[2], where)
    payload = geo.extension.raw(BINMESH)
    if payload is not None:
        geo.binmesh = BinMesh(payload, where + " bin mesh")
    payload = geo.extension.raw(SKIN)
    if payload is not None:
        geo.skin = Skin(payload, num_verts, where + " skin")
    payload = geo.extension.raw(USERDATA)
    if payload is not None:
        geo.user_data = parse_user_data(payload, 0, len(payload), where + " user data")
    return geo


class Atomic:
    def __init__(self, frame, geometry, flags, unused):
        self.frame, self.geometry, self.flags, self.unused = frame, geometry, flags, unused
        self.extension = Extension()


class Clump:
    def __init__(self):
        self.libid = 0
        self.frames = []
        self.frame_list_extension_libids = []
        self.geometries = []
        self.atomics = []
        self.extension = Extension()

    def hierarchy_root(self):
        """Index of the frame carrying the HAnim node list, or None."""
        for index, frame in enumerate(self.frames):
            if frame.hanim is not None and frame.hanim.nodes:
                return index
        return None

    def children(self, index):
        return [i for i, frame in enumerate(self.frames) if frame.parent == index]

    def hanim_frames(self):
        """[(frame index, node id, node flags)] in HAnim node order (depth-first under the root)."""
        root = self.hierarchy_root()
        if root is None:
            return []
        by_id = {}
        for index, frame in enumerate(self.frames):
            if frame.hanim is not None:
                by_id.setdefault(frame.hanim.node_id, index)
        out = []
        for node_id, node_index, flags in self.frames[root].hanim.nodes:
            if node_id not in by_id:
                raise FormatError("HAnim node id %d has no frame" % node_id)
            out.append((by_id[node_id], node_id, flags))
        return out


def parse_clump(data, chunk, where="clump"):
    _expect(chunk, CLUMP, where)
    parts = chunk_list(data, chunk[2], chunk[3], where)
    clump = Clump()
    clump.libid = chunk[1]
    _expect(parts[0], STRUCT, where)
    r = Reader(data, parts[0][2], parts[0][3], where + " struct")
    num_atomics = r.one("<i")
    num_lights = num_cameras = 0
    if r.end - r.pos >= 8:
        num_lights, num_cameras = r.take("<ii")
    if num_lights or num_cameras:
        raise FormatError("%s: lights/cameras in a model clump are not used by P4" % where)
    _expect(parts[1], FRAMELIST, where)
    fparts = chunk_list(data, parts[1][2], parts[1][3], where + " frame list")
    fr = Reader(data, fparts[0][2], fparts[0][3], where + " frame list")
    num_frames = fr.one("<i")
    for _ in range(num_frames):
        values = fr.take("<12fiI")
        clump.frames.append(Frame(values[0:9], values[9:12], values[12], values[13]))
    if len(fparts) != 1 + num_frames:
        raise FormatError("%s: %d frame extensions for %d frames" % (where, len(fparts) - 1, num_frames))
    for index, frame in enumerate(clump.frames):
        frame.extension = parse_extension(data, fparts[1 + index], "%s frame %d" % (where, index))
        payload = frame.extension.raw(HANIM)
        if payload is not None:
            frame.hanim = HAnimPlugin(payload, "%s frame %d HAnim" % (where, index))
        payload = frame.extension.raw(USERDATA)
        if payload is not None:
            frame.user_data = parse_user_data(payload, 0, len(payload), "%s frame %d user data" % (where, index))
    _expect(parts[2], GEOMETRYLIST, where)
    gparts = chunk_list(data, parts[2][2], parts[2][3], where + " geometry list")
    num_geometries = struct.unpack_from("<i", data, gparts[0][2])[0]
    for index in range(num_geometries):
        clump.geometries.append(parse_geometry(data, gparts[1 + index], "%s geometry %d" % (where, index)))
    index = 3
    for a in range(num_atomics):
        achunk = parts[index]
        index += 1
        _expect(achunk, ATOMIC, where)
        aparts = chunk_list(data, achunk[2], achunk[3], "%s atomic %d" % (where, a))
        frame, geometry, flags, unused = struct.unpack_from("<iiIi", data, aparts[0][2])
        atomic = Atomic(frame, geometry, flags, unused)
        if len(aparts) > 1 and aparts[1][0] == GEOMETRY:
            raise FormatError("%s atomic %d: inline geometry is not used by P4" % (where, a))
        atomic.extension = parse_extension(data, aparts[-1], "%s atomic %d" % (where, a))
        clump.atomics.append(atomic)
    clump.extension = parse_extension(data, parts[index], where)
    return clump


# ---------------------------------------------------------------------------------------------------
# Animation (rtanim.c / rphanim.c keyframe schemes)

_F32 = struct.Struct("<f")


def f32(value):
    """value rounded to the nearest float32."""
    return _F32.unpack(_F32.pack(value))[0]


def decompress_float(value):
    """RtCompressedKeyFrame 16-bit float: sign, 4-bit exponent biased to 2^-15..2^0, 11-bit mantissa."""
    bits = (value & 0x8000) << 16
    if value & 0x7FFF:
        bits |= (((value & 0x7800) << 12) + 0x38000000) | ((value & 0x07FF) << 12)
    return struct.unpack("<f", struct.pack("<I", bits))[0]


_DECOMPRESSED = None


def _decompressed():
    global _DECOMPRESSED
    if _DECOMPRESSED is None:
        _DECOMPRESSED = [decompress_float(v) for v in range(0x10000)]
    return _DECOMPRESSED


class Animation:
    """RtAnimAnimation with HAnim keyframes (header: version 0x100, scheme id, keyframe count, flags,
    duration in seconds). Scheme 1 (RpHAnimStdKeyFrame, 36 bytes in memory): time, quat xyzw,
    translation, previous. Scheme 2 (RtCompressedKeyFrame, 24 bytes in memory): time, quat and
    translation as packed 16-bit floats, previous; then the translation offset and scale (6 floats).
    "previous" is stored as a byte offset into the in-memory keyframe array (index * keyframe size);
    keyframes of a node's first time carry no previous (retail leaves garbage there)."""
    STANDARD, COMPRESSED = 1, 2
    KEYFRAME_BYTES = {STANDARD: 36, COMPRESSED: 24}

    def __init__(self, data, start, end, where):
        r = Reader(data, start, end, where)
        self.version, self.scheme, count, self.flags, self.duration = r.take("<iiiif")
        self.keyframes = []  # (time, (qx, qy, qz, qw), (tx, ty, tz), previous index or None)
        self.packed = None
        self.offset = (0.0, 0.0, 0.0)
        self.scale = (1.0, 1.0, 1.0)
        if self.scheme == self.STANDARD:
            flat = r.take("<" + "8fi" * count)
            for i in range(0, len(flat), 9):
                t, qx, qy, qz, qw, tx, ty, tz, prev = flat[i:i + 9]
                self.keyframes.append((t, (qx, qy, qz, qw), (tx, ty, tz), prev))
        elif self.scheme == self.COMPRESSED:
            flat = r.take("<" + "f7Hi" * count)
            self.offset = r.take("<3f")
            self.scale = r.take("<3f")
            self.packed = [flat[i:i + 9] for i in range(0, len(flat), 9)]
            table = _decompressed()
            (ox, oy, oz), (sx, sy, sz) = self.offset, self.scale
            for t, qx, qy, qz, qw, tx, ty, tz, prev in self.packed:
                quat = (table[qx], table[qy], table[qz], table[qw])
                trans = (f32(f32(table[tx] * sx) + ox), f32(f32(table[ty] * sy) + oy), f32(f32(table[tz] * sz) + oz))
                self.keyframes.append((t, quat, trans, prev))
        else:
            raise UnsupportedAnimation("%s: RtAnim type 0x%x is not an HAnim keyframe scheme" % (where, self.scheme & 0xFFFFFFFF))
        if r.pos != r.end:
            raise FormatError("%s: %d trailing bytes" % (where, r.end - r.pos))
        unit = self.KEYFRAME_BYTES[self.scheme]
        self.keyframes = [(t, q, tr, prev // unit if prev >= 0 and prev % unit == 0 and prev // unit < count else None)
                          for t, q, tr, prev in self.keyframes]

    def tracks(self, num_nodes):
        """Per node [(time, quat, translation)]: the first num_nodes keyframes are node 0..n-1 at their
        first time, the next num_nodes their second, and each later keyframe continues the node of the
        keyframe it names as previous (the RtAnim keyframe order rule)."""
        node_of = []
        for index, (_t, _q, _tr, prev) in enumerate(self.keyframes):
            if index < 2 * num_nodes:
                node_of.append(index % num_nodes)
            else:
                if prev is None or not 0 <= prev < index:
                    raise FormatError("keyframe %d names previous keyframe %s" % (index, prev))
                node_of.append(node_of[prev])
        tracks = [[] for _ in range(num_nodes)]
        for index, (t, q, tr, _prev) in enumerate(self.keyframes):
            tracks[node_of[index]].append((t, q, tr))
        for track in tracks:
            track.sort(key=lambda key: key[0])
        return tracks


# ---------------------------------------------------------------------------------------------------
# Texture dictionary with PS2 native textures (p4gl_native.c func_003fd940 layout)

class NativeTexture:
    def __init__(self):
        self.platform = 0
        self.filter_addressing = 0
        self.name = ""
        self.mask = ""
        self.header = ()
        self.data = b""
        self.extension = Extension()

    @property
    def width(self):
        return self.header[0]

    @property
    def height(self):
        return self.header[1]

    @property
    def depth(self):
        return self.header[2]

    @property
    def raster_flags(self):
        return self.header[3] & 0xFFFF

    @property
    def kind(self):
        return self.header[3] >> 16


class TexDictionary:
    def __init__(self):
        self.device = 0
        self.textures = []
        self.extension = Extension()

    def find(self, name):
        for tex in self.textures:
            if tex.name.lower() == name.lower():
                return tex
        return None


def parse_texdictionary(data, chunk, where="texture dictionary"):
    _expect(chunk, TEXDICTIONARY, where)
    parts = chunk_list(data, chunk[2], chunk[3], where)
    txd = TexDictionary()
    count, txd.device = struct.unpack_from("<HH", data, parts[0][2])
    for index in range(count):
        tchunk = parts[1 + index]
        twhere = "%s texture %d" % (where, index)
        _expect(tchunk, TEXTURENATIVE, twhere)
        tparts = chunk_list(data, tchunk[2], tchunk[3], twhere)
        tex = NativeTexture()
        tex.platform, tex.filter_addressing = struct.unpack_from("<II", data, tparts[0][2])
        if tex.platform != PLATFORM_PS2:
            raise FormatError("%s: platform 0x%x is not PS2" % (twhere, tex.platform))
        tex.name = _string(data, tparts[1])
        tex.mask = _string(data, tparts[2])
        rparts = chunk_list(data, tparts[3][2], tparts[3][3], twhere + " raster")
        if len(rparts) != 2 or rparts[0][3] - rparts[0][2] != 64:
            raise FormatError("%s: raster is not a 64-byte header and a data chunk" % twhere)
        tex.header = struct.unpack_from("<16I", data, rparts[0][2])
        tex.data = data[rparts[1][2]:rparts[1][3]]
        tex.extension = parse_extension(data, tparts[4], twhere)
        txd.textures.append(tex)
    txd.extension = parse_extension(data, parts[1 + count], where)
    return txd


# ---------------------------------------------------------------------------------------------------
# RMD container

class RmdChunk:
    def __init__(self, kind, libid, payload):
        self.kind, self.libid, self.payload = kind, libid, payload
        self.obj = None

    @property
    def name(self):
        return CHUNK_NAMES.get(self.kind, "0x%x" % self.kind)


class Rmd:
    """An RMD: Atlus's sequence of top-level chunks (mdlReadAuxChunks reads them in order until a type-0
    chunk or the end). Typical order: slot count, texture dictionary, clump, attach table, then per
    animation slot: slot matrix, animation, material/UV animations, next slot (or alias/placeholder)."""

    def __init__(self, data):
        self.chunks = []
        self.trailing = b""
        self.warnings = []
        offset = 0
        while offset + 12 <= len(data):
            kind, size, libid = struct.unpack_from("<III", data, offset)
            if kind == 0:
                break
            if kind == RMD_EFFECT and offset + 12 + 20 <= len(data):
                # The loader reads the 20-byte effect header, skips `skip` bytes and the `length`-byte
                # payload; it never uses the chunk size, which retail writes without the skip.
                _first, _last, length, skip = struct.unpack_from("<HHII", data, offset + 12)
                size = 20 + skip + length
            if offset + 12 + size > len(data):
                raise FormatError("RMD chunk 0x%x at 0x%x overruns the file" % (kind, offset))
            chunk = RmdChunk(kind, libid, data[offset + 12:offset + 12 + size])
            chunk.offset = offset
            self.chunks.append(chunk)
            offset += 12 + size
        self.trailing = data[offset:]
        for index, chunk in enumerate(self.chunks):
            where = "chunk %d (%s)" % (index, chunk.name)
            whole = (chunk.kind, chunk.libid, 0, len(chunk.payload))
            if chunk.kind == CLUMP:
                chunk.obj = parse_clump(chunk.payload, whole, where)
            elif chunk.kind == TEXDICTIONARY:
                chunk.obj = parse_texdictionary(chunk.payload, whole, where)
            elif chunk.kind == ANIMANIMATION:
                try:
                    chunk.obj = Animation(chunk.payload, 0, len(chunk.payload), where)
                except UnsupportedAnimation as error:
                    # Another registered RtAnim type (one field prop has one): kept raw, not decoded.
                    self.warnings.append(str(error))
                    chunk.obj = None

    def of(self, kind):
        return [chunk.obj for chunk in self.chunks if chunk.kind == kind]

    @property
    def clump(self):
        clumps = self.of(CLUMP)
        return clumps[0] if clumps else None

    @property
    def texdictionary(self):
        txds = self.of(TEXDICTIONARY)
        return txds[0] if txds else None

    def attach_table(self):
        """[(attach id, HAnim node id, 16-float RwMatrix)] from the 0xF0F00006 chunk (FUN_0047B0C0)."""
        for chunk in self.chunks:
            if chunk.kind == RMD_ATTACH_TABLE:
                r = Reader(chunk.payload, 0, len(chunk.payload), "attach table")
                out = []
                for _ in range(r.one("<H")):
                    ident, node = r.take("<ii")
                    out.append((ident, node, r.take("<16f")))
                return out
        return []

    def slots(self):
        """The animation slot layout mdlReadAuxChunks builds, per layer and slot: "animation" (index into
        this file's animation chunks), "blend" (the four animations a blend group chunk reads right
        after itself), "alias" (u16 source slot), "placeholder", "matrix" (slot RwMatrix), "chunks"
        (indices of the material/UV animation, effect and other per-slot chunks). Animations read while
        the base-animation flag is set and the layer has no base yet go to the layer's "base"."""
        layer, slot = 0, 0
        table = {}
        bases = {}
        anim_index = 0
        base_flag = False
        blend_left = 0
        seen_clump = False

        def entry():
            return table.setdefault((layer, slot), {"layer": layer, "slot": slot, "animation": None, "blend": [],
                                                    "alias": None, "placeholder": False, "matrix": None,
                                                    "chunks": []})
        for index, chunk in enumerate(self.chunks):
            if chunk.kind == CLUMP:
                seen_clump = True
            elif chunk.kind == RMD_NEXT_SLOT:
                slot += 1
            elif chunk.kind == RMD_NEXT_LAYER:
                layer, slot = layer + 1, 0
            elif chunk.kind == ANIMANIMATION:
                if blend_left:
                    entry()["blend"].append(anim_index)
                    blend_left -= 1
                elif base_flag and layer not in bases:
                    bases[layer] = anim_index
                else:
                    entry()["animation"] = anim_index
                anim_index += 1
            elif chunk.kind == RMD_BASE_ANIMATION:
                base_flag = True
            elif chunk.kind == RMD_BLEND_GROUP:
                entry()["chunks"].append(index)
                blend_left = 4
            elif chunk.kind == RMD_ALIAS:
                entry()["alias"] = struct.unpack_from("<H", chunk.payload, 0)[0]
            elif chunk.kind == RMD_PLACEHOLDER:
                entry()["placeholder"] = True
            elif chunk.kind == RMD_SLOT_MATRIX:
                entry()["matrix"] = list(struct.unpack_from("<16f", chunk.payload, 0))
            elif chunk.kind == UVANIMDICT and not seen_clump:
                bases.setdefault("uv", index)
            elif chunk.kind in RMD_MATANIM or chunk.kind in (UVANIMDICT, RMD_EFFECT, RMD_EFFECT_COPY,
                                                              RMD_START_FRAME_A, RMD_START_FRAME_B):
                entry()["chunks"].append(index)
        self.slot_bases = bases
        return [table[key] for key in sorted(table)]
