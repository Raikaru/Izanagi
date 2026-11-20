#!/usr/bin/env python3
"""Convert Persona 4 RenderWare models (RMD) from the user's own disc to glTF 2.0 for Blender.

    python tools/models/rw_to_gltf.py --out OUT DATA/MODEL/WEAPON/WP001.RMD
    python tools/models/rw_to_gltf.py --out OUT "DATA/MODEL/PACK/BC001.PAC#bc001.RMD" --glb
    python tools/models/rw_to_gltf.py --out OUT "DATA/MODEL/WEAPON/WP0*.RMD"     # disc glob
    python tools/models/rw_to_gltf.py --list DATA/MODEL/WEAPON                   # disc keys
    python tools/models/rw_to_gltf.py --out OUT some/local/file.RMD

The disc is P4PC_ISO (--iso overrides). Every model gets OUT/<name>/:
<name>.gltf + <name>.bin + one PNG per texture (or one <name>.glb with --glb) and
<name>.p4model.json, the sidecar with the P4 metadata glTF has no place for (animation slot layout,
attach table, Atlus chunks, textures' PS2 formats). Converted files derive from the user's disc: they are
for local editing and never belong in the repository.

What is written follows the glTF convention in docs/modding/models.md: 1 unit = 1 P4 unit (about a
centimetre), Y up, RW frames as nodes (TRS, exact RW matrix in extras.p4.matrix), HAnim bones as the
skin joints in HAnim node order, RpSkin inverse matrices as inverseBindMatrices, each RtAnim animation as
one glTF animation (LINEAR rotation/translation per bone), materials as metallic 0 / roughness 1 PBR
(KHR_materials_unlit for prelit geometry without the light flag), prelit colours as COLOR_0, and the
P4-specific material state (draw category, fog, tint, MatFX environment map, UV animation) in
extras.p4.
"""
import argparse
import base64
import fnmatch
import hashlib
import json
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gltfio  # noqa: E402
import p4disc  # noqa: E402
import ps2tex  # noqa: E402
import rwmodel as rw  # noqa: E402

SIDECAR_FORMAT = "p4model-sidecar"
SIDECAR_VERSION = 1
CONVENTION = 1

GL_NEAREST, GL_LINEAR = 9728, 9729
GL_FILTERS = {  # RwTextureFilterMode -> (mag, min)
    1: (GL_NEAREST, GL_NEAREST), 2: (GL_LINEAR, GL_LINEAR), 3: (GL_NEAREST, 9984), 4: (GL_NEAREST, 9986),
    5: (GL_LINEAR, 9985), 6: (GL_LINEAR, 9987)}
GL_WRAPS = {1: 10497, 2: 33648, 3: 33071, 4: 33071}  # RwTextureAddressMode -> wrap
FRAME_NAMES = ("per3HelperId",)


class ConvertError(Exception):
    pass


def _b64(data):
    return base64.b64encode(data).decode("ascii")


def _hexcolor(rgba):
    return "%02x%02x%02x%02x" % tuple(rgba)


def _user_json(arrays):
    return [array.to_json() for array in arrays]


def _normalize(v, fallback=(0.0, 0.0, 1.0)):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    if length < 1e-12:
        return fallback
    return (v[0] / length, v[1] / length, v[2] / length)


SHEAR_LIMIT = 0.05


def decompose(rotation, position, limit=SHEAR_LIMIT):
    """(translation, rotation quat, scale) of an RW frame, or None when the rows are degenerate or
    sheared beyond limit. Non-orthogonal rows (retail has up to ~1e-3 on ordinary frames) are
    orthonormalised (Gram-Schmidt from the right vector); the exact RW rows go to extras.p4.matrix."""
    rows = [rotation[0:3], rotation[3:6], rotation[6:9]]
    scale = [math.sqrt(sum(c * c for c in row)) for row in rows]
    if min(scale) < 1e-8:
        return None
    unit = [[c / s for c in row] for row, s in zip(rows, scale)]
    for i in range(3):
        for j in range(i + 1, 3):
            if abs(sum(unit[i][k] * unit[j][k] for k in range(3))) > limit:
                return None
    right = unit[0]
    d = sum(a * b for a, b in zip(unit[1], right))
    up = _normalize([unit[1][k] - d * right[k] for k in range(3)])
    at = (right[1] * up[2] - right[2] * up[1], right[2] * up[0] - right[0] * up[2], right[0] * up[1] - right[1] * up[0])
    if sum(a * b for a, b in zip(at, unit[2])) < 0:  # mirrored: fold the reflection into the scale
        scale[2] = -scale[2]
    return tuple(position), gltfio.quat_from_rows([right, up, at]), tuple(scale)


def _alpha_mode(rgba):
    alphas = set(rgba[3::4])
    if alphas <= {255}:
        return "OPAQUE"
    if alphas <= {0, 255}:
        return "MASK"
    return "BLEND"


class Converter:
    def __init__(self, data, name, source=None, glb=False):
        self.data = data
        self.name = name
        self.source = source or name
        self.glb = glb
        self.rmd = rw.Rmd(data)
        self.clump = self.rmd.clump
        if self.clump is None:
            raise ConvertError("%s has no clump (not a model)" % self.source)
        self.b = gltfio.Builder()
        self.files = {}  # relative file name -> bytes (PNGs in .gltf mode)
        self.warnings = list(self.rmd.warnings)
        self.sampler_index = {}
        self.texture_index = {}
        self.image_index = {}
        self.tex_info = []

    # -- textures -------------------------------------------------------------------------------
    def _images(self):
        txd = self.rmd.texdictionary
        if txd is None:
            return
        for tex in txd.textures:
            entry = {"name": tex.name, "mask": tex.mask, "width": tex.width, "height": tex.height,
                     "depth": tex.depth, "rasterFlags": "0x%04x" % tex.raster_flags, "kind": tex.kind,
                     "filterAddressing": "0x%x" % tex.filter_addressing,
                     "mipmaps": bool(tex.raster_flags & ps2tex.FLAG_MIPMAP),
                     "header": ["0x%x" % v for v in tex.header],
                     "plugins": tex.extension.opaque_json(())}
            try:
                w, h, rgba, _palette = ps2tex.decode(tex)
            except ps2tex.TextureError as error:
                self.warnings.append(str(error))
                entry["image"] = None
                self.tex_info.append(entry)
                continue
            png = gltfio.png_rgba(w, h, rgba)
            image = {"name": tex.name}
            if self.glb:
                image["bufferView"] = self.b.view(png)
                image["mimeType"] = "image/png"
            else:
                file_name = "%s_%s.png" % (self.name, _safe(tex.name))
                self.files[file_name] = png
                image["uri"] = file_name
            index = self.b.add("images", image)
            self.image_index[tex.name.lower()] = (index, _alpha_mode(rgba))
            entry["image"] = index
            entry["alphaMode"] = _alpha_mode(rgba)
            self.tex_info.append(entry)

    def _sampler(self, filter_addressing):
        if filter_addressing not in self.sampler_index:
            mag, mini = GL_FILTERS.get(filter_addressing & 0xFF, (GL_LINEAR, GL_LINEAR))
            u = GL_WRAPS.get((filter_addressing >> 8) & 0xF, 10497)
            v = GL_WRAPS.get((filter_addressing >> 12) & 0xF, u)
            self.sampler_index[filter_addressing] = self.b.add("samplers", {
                "magFilter": mag, "minFilter": mini, "wrapS": u, "wrapT": v})
        return self.sampler_index[filter_addressing]

    def _texture(self, name, filter_addressing):
        """glTF texture index for a TXD texture name, or None when the TXD lacks it."""
        image = self.image_index.get(name.lower())
        if image is None:
            return None, None
        key = (image[0], filter_addressing)
        if key not in self.texture_index:
            self.texture_index[key] = self.b.add("textures", {"source": image[0], "sampler": self._sampler(filter_addressing),
                                                              "name": name})
        return self.texture_index[key], image[1]

    # -- materials ------------------------------------------------------------------------------
    def _material(self, mat, geo, gi, mi, used_ext):
        name = mat.user_value("name") or (mat.texture.name if mat.texture else "g%d_m%d" % (gi, mi))
        lit = bool(geo.format & rw.GEO_LIGHT) and bool(geo.morph_targets and geo.morph_targets[0].normals)
        out = {"name": name, "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}}
        factor = [c / 255.0 for c in mat.color]
        if factor != [1.0, 1.0, 1.0, 1.0]:
            out["pbrMetallicRoughness"]["baseColorFactor"] = factor
        p4 = {"color": list(mat.color), "surfaceProperties": list(mat.surface), "flags": mat.flags,
              "lighting": "lit" if lit else "prelit", "userData": _user_json(mat.user_data)}
        alpha = "OPAQUE"
        if mat.texture is not None:
            p4["texture"] = {"name": mat.texture.name, "mask": mat.texture.mask,
                             "filterAddressing": "0x%x" % mat.texture.filter_addressing,
                             "plugins": mat.texture.extension.opaque_json(())}
            index, mode = self._texture(mat.texture.name, mat.texture.filter_addressing)
            if index is None:
                self.warnings.append("material %s: texture %s is not in the model's dictionary" % (name, mat.texture.name))
            else:
                out["pbrMetallicRoughness"]["baseColorTexture"] = {"index": index}
                alpha = mode
        if mat.color[3] < 255:
            alpha = "BLEND"
        if alpha != "OPAQUE":
            out["alphaMode"] = alpha
            if alpha == "MASK":
                out["alphaCutoff"] = 0.5
        category = mat.user_value("per3MatAlpha")
        if category is not None:
            p4["drawCategory"] = category
        if any(a.name == "per3FogDisable" for a in mat.user_data):
            p4["fogDisable"] = True
        tint = mat.user_value("per3modelMatColor")
        if tint is not None:
            p4["tint"] = "%08x" % (tint & 0xFFFFFFFF)
        if mat.matfx is not None:
            passes = []
            for info in mat.matfx.passes:
                info = dict(info)
                for key in ("texture", "bumpedTexture"):
                    tex = info.get(key)
                    if tex is not None:
                        index, _mode = self._texture(tex.name, tex.filter_addressing)
                        info[key] = {"name": tex.name, "filterAddressing": "0x%x" % tex.filter_addressing,
                                     "index": index}
                passes.append(info)
            p4["matfx"] = {"effect": mat.matfx.name, "passes": passes}
        if mat.uv_animations:
            p4["uvAnimations"] = [{"slot": bit, "name": n} for bit, n in mat.uv_animations]
        opaque = mat.extension.opaque_json((rw.USERDATA, rw.MATFX, rw.UVANIM))
        if opaque:
            p4["plugins"] = opaque
        if not lit:
            out["extensions"] = {"KHR_materials_unlit": {}}
            used_ext.add("KHR_materials_unlit")
        out["extras"] = {"p4": p4}
        return self.b.add("materials", out)

    # -- frames ---------------------------------------------------------------------------------
    def _nodes(self):
        clump = self.clump
        root = clump.hierarchy_root()
        hanim_order = {}
        if root is not None:
            for index, (frame, node_id, flags) in enumerate(clump.hanim_frames()):
                hanim_order[frame] = (index, node_id, flags)
        self.frame_node = []
        for fi, frame in enumerate(clump.frames):
            helper = None
            for array in frame.user_data:
                if array.name == "per3HelperId" and array.values:
                    helper = array.values[0]
            if frame.hanim is not None:
                name = "bone_%d" % frame.hanim.node_id
            elif helper is not None:
                name = "helper_%d" % helper
            else:
                name = "frame_%d" % fi
            node = {"name": name}
            exact = list(frame.rotation) + list(frame.position)
            trs = decompose(frame.rotation, frame.position)
            if trs is None and fi in hanim_order:
                # An HAnim bone: its animation replaces the frame matrix with a rotation and a
                # translation every update, so the bind matrix's shear never survives in game.
                trs = decompose(frame.rotation, frame.position, limit=1.0)
                if trs is None:
                    raise ConvertError("bone frame %d has a degenerate matrix" % fi)
                self.warnings.append("bone frame %d: sheared bind matrix approximated (exact rows in extras.p4.matrix)" % fi)
            p4 = {"frame": fi, "flags": frame.flags}
            if trs is None:
                node["matrix"] = [frame.rotation[0], frame.rotation[1], frame.rotation[2], 0.0,
                                  frame.rotation[3], frame.rotation[4], frame.rotation[5], 0.0,
                                  frame.rotation[6], frame.rotation[7], frame.rotation[8], 0.0,
                                  frame.position[0], frame.position[1], frame.position[2], 1.0]
            else:
                t, q, s = trs
                if any(t):
                    node["translation"] = list(t)
                if q != (0.0, 0.0, 0.0, 1.0):
                    node["rotation"] = list(q)
                if any(abs(c - 1.0) > 1e-6 for c in s):
                    node["scale"] = list(s)
                p4["matrix"] = exact
            if frame.hanim is not None:
                p4["hanimId"] = frame.hanim.node_id
                if fi in hanim_order:
                    p4["hanimIndex"], _id, p4["hanimFlags"] = hanim_order[fi]
                if frame.hanim.nodes:
                    p4["hierarchy"] = {"flags": frame.hanim.flags, "keyFrameSize": frame.hanim.keyframe_size,
                                       "nodes": [list(n) for n in frame.hanim.nodes], "version": frame.hanim.version}
            if frame.user_data:
                p4["userData"] = _user_json(frame.user_data)
            if helper is not None:
                p4["helperId"] = helper
            opaque = frame.extension.opaque_json((rw.HANIM, rw.USERDATA))
            if opaque:
                p4["plugins"] = opaque
            node["extras"] = {"p4": p4}
            self.frame_node.append(self.b.add("nodes", node))
        for fi, frame in enumerate(clump.frames):
            if frame.parent >= 0:
                parent = self.b.doc["nodes"][self.frame_node[frame.parent]]
                parent.setdefault("children", []).append(self.frame_node[fi])
        return [self.frame_node[fi] for fi, frame in enumerate(clump.frames) if frame.parent < 0]

    # -- geometry -------------------------------------------------------------------------------
    def _skin(self, geo, gi):
        clump = self.clump
        bones = clump.hanim_frames()
        skin = geo.skin
        if len(bones) != skin.num_bones:
            raise ConvertError("geometry %d skins %d bones but the HAnim hierarchy has %d nodes" % (gi, skin.num_bones, len(bones)))
        key = tuple(tuple(m) for m in skin.matrices)
        cache = getattr(self, "_skins", {})
        self._skins = cache
        if key in cache:
            return cache[key]
        ibm = []
        for m in skin.matrices:
            ibm.append((m[0], m[1], m[2], 0.0, m[4], m[5], m[6], 0.0, m[8], m[9], m[10], 0.0, m[12], m[13], m[14], 1.0))
        pads = [["0x%08x" % w for w in words] for words in skin.matrix_pads]
        root = clump.hierarchy_root()
        index = self.b.add("skins", {
            "name": "hanim", "joints": [self.frame_node[f] for f, _id, _fl in bones],
            "inverseBindMatrices": self.b.accessor(ibm, gltfio.FLOAT, "MAT4"),
            "skeleton": self.frame_node[root],
            "extras": {"p4": {"matrixPads": pads}}})
        cache[key] = index
        return index

    def _mesh(self, geo, gi, used_ext):
        target = geo.morph_targets[0]
        attrs = {"POSITION": self.b.accessor(target.positions, gltfio.FLOAT, "VEC3", gltfio.ARRAY_BUFFER, minmax=True)}
        if target.normals is not None:
            attrs["NORMAL"] = self.b.accessor([_normalize(n) for n in target.normals], gltfio.FLOAT, "VEC3",
                                              gltfio.ARRAY_BUFFER)
        for set_index, uvs in enumerate(geo.uvs):
            attrs["TEXCOORD_%d" % set_index] = self.b.accessor(uvs, gltfio.FLOAT, "VEC2", gltfio.ARRAY_BUFFER)
        if geo.prelit is not None:
            attrs["COLOR_0"] = self.b.accessor(geo.prelit, gltfio.UNSIGNED_BYTE, "VEC4", gltfio.ARRAY_BUFFER,
                                               normalized=True)
        skin = None
        if geo.skin is not None:
            joints, weights = [], []
            idx, wts = geo.skin.indices, geo.skin.weights
            for v in range(geo.num_vertices):
                w = list(wts[4 * v:4 * v + 4])
                j = list(idx[4 * v:4 * v + 4])
                total = sum(w)
                if total <= 0:
                    w, j = [1.0, 0.0, 0.0, 0.0], [j[0], 0, 0, 0]
                elif abs(total - 1.0) > 1e-4:
                    w = [c / total for c in w]
                j = [jj if ww > 0 else 0 for jj, ww in zip(j, w)]
                joints.append(tuple(j))
                weights.append(tuple(w))
            attrs["JOINTS_0"] = self.b.accessor(joints, gltfio.UNSIGNED_BYTE, "VEC4", gltfio.ARRAY_BUFFER)
            attrs["WEIGHTS_0"] = self.b.accessor(weights, gltfio.FLOAT, "VEC4", gltfio.ARRAY_BUFFER)
            skin = self._skin(geo, gi)
        materials = [self._material(mat, geo, gi, mi, used_ext) for mi, mat in enumerate(geo.materials)]
        primitives = []
        for mi in range(len(geo.materials)):
            tris = [t for t in geo.triangles if t[3] == mi and len({t[0], t[1], t[2]}) == 3]
            if not tris:
                continue
            indices = [v for t in tris for v in t[:3]]
            component = gltfio.UNSIGNED_SHORT if geo.num_vertices <= 0xFFFF else gltfio.UNSIGNED_INT
            primitives.append({"attributes": attrs, "material": materials[mi], "mode": 4,
                               "indices": self.b.accessor(indices, component, "SCALAR", gltfio.ELEMENT_ARRAY_BUFFER)})
        p4 = {"geometry": gi, "format": "0x%x" % geo.format, "boundingSphere": list(target.sphere),
              "morphTargets": len(geo.morph_targets), "materialSlots": geo.material_slots,
              "triangles": len(geo.triangles)}
        if geo.binmesh is not None:
            p4["binMesh"] = {"strips": geo.binmesh.strips, "meshes": len(geo.binmesh.meshes),
                             "indices": geo.binmesh.total}
        if geo.skin is not None:
            p4["skin"] = {"bones": geo.skin.num_bones, "usedBones": geo.skin.used_bones,
                          "maxWeights": geo.skin.max_weights, "split": _b64(geo.skin.tail)}
        if geo.user_data:
            p4["userData"] = _user_json(geo.user_data)
        opaque = geo.extension.opaque_json((rw.BINMESH, rw.SKIN, rw.USERDATA))
        if opaque:
            p4["plugins"] = opaque
        if len(geo.morph_targets) > 1:
            self.warnings.append("geometry %d: %d morph targets, only the first is exported" % (gi, len(geo.morph_targets)))
        mesh = self.b.add("meshes", {"name": "geometry_%d" % gi, "primitives": primitives, "extras": {"p4": p4}})
        return mesh, skin

    # -- animations -----------------------------------------------------------------------------
    def _animations(self, slot_names):
        bones = self.clump.hanim_frames()
        anims = [chunk for chunk in self.rmd.chunks if chunk.kind == rw.ANIMANIMATION]
        for ai, chunk in enumerate(anims):
            anim = chunk.obj
            if anim is None:
                continue  # not HAnim keyframes: kept raw in the sidecar (rwmodel's warning says so)
            channels, samplers = [], []
            if bones:
                tracks = anim.tracks(len(bones))
                for bi, track in enumerate(tracks):
                    keys = []
                    for t, q, tr in track:
                        if keys and keys[-1][0] == t:
                            keys[-1] = (t, q, tr)
                        else:
                            keys.append((t, q, tr))
                    if not keys:
                        continue
                    times = self.b.accessor([k[0] for k in keys], gltfio.FLOAT, "SCALAR", minmax=True)
                    rot = []
                    for _t, q, _tr in keys:
                        n = math.sqrt(sum(c * c for c in q)) or 1.0
                        rot.append(tuple(c / n for c in q))
                    node = self.frame_node[bones[bi][0]]
                    rotation_sampler = {"input": times, "output": self.b.accessor(rot, gltfio.FLOAT, "VEC4"),
                                        "interpolation": "LINEAR"}
                    if anim.scheme == rw.Animation.COMPRESSED:
                        # Retain packed retail quaternions only while the editable glTF keys remain unchanged.
                        def packed(value):
                            bits = struct.unpack("<I", struct.pack("<f", value))[0]
                            sign = (bits >> 16) & 0x8000
                            if not bits & 0x7fffffff:
                                return sign
                            return sign | (((bits >> 23) & 0xff) - 112) << 11 | ((bits >> 12) & 0x7ff)
                        rotation_sampler["extras"] = {"p4": {
                            "packedRotation": self.b.accessor([tuple(packed(c) for c in k[1]) for k in keys],
                                                               gltfio.UNSIGNED_SHORT, "VEC4"),
                            "rotationSnapshot": self.b.accessor(rot, gltfio.FLOAT, "VEC4"),
                            "timeSnapshot": self.b.accessor([k[0] for k in keys], gltfio.FLOAT, "SCALAR")}}
                    samplers.append(rotation_sampler)
                    channels.append({"sampler": len(samplers) - 1, "target": {"node": node, "path": "rotation"}})
                    samplers.append({"input": times, "output": self.b.accessor([k[2] for k in keys], gltfio.FLOAT, "VEC3"),
                                     "interpolation": "LINEAR"})
                    channels.append({"sampler": len(samplers) - 1, "target": {"node": node, "path": "translation"}})
            p4 = {"chunk": self.rmd.chunks.index(chunk), "scheme": anim.scheme, "duration": anim.duration,
                  "flags": anim.flags, "keyframes": len(anim.keyframes), "version": anim.version}
            if anim.scheme == rw.Animation.COMPRESSED:
                p4["translationOffset"] = list(anim.offset)
                p4["translationScale"] = list(anim.scale)
            if not channels:
                self.warnings.append("animation %d has no bones to drive" % ai)
                continue
            self.b.add("animations", {"name": slot_names.get(ai, "anim_%02d" % ai), "channels": channels,
                                      "samplers": samplers, "extras": {"p4": p4}})

    # -- whole model ----------------------------------------------------------------------------
    def run(self):
        used_ext = set()
        self._images()
        roots = self._nodes()
        clump = self.clump
        mesh_on = {}
        for ai, atomic in enumerate(clump.atomics):
            geo = clump.geometries[atomic.geometry]
            mesh, skin = self._mesh(geo, atomic.geometry, used_ext)
            frame_node = self.frame_node[atomic.frame]
            rtr = atomic.extension.raw(rw.RIGHTTORENDER)
            p4 = {"atomic": ai, "flags": atomic.flags, "geometry": atomic.geometry}
            if rtr is not None and len(rtr) == 8:
                p4["rightToRender"] = ["0x%x" % v for v in struct.unpack("<II", rtr)]
            matfx = atomic.extension.raw(rw.MATFX)
            if matfx is not None:
                p4["matfx"] = struct.unpack("<I", matfx)[0]
            opaque = atomic.extension.opaque_json((rw.RIGHTTORENDER, rw.MATFX))
            if opaque:
                p4["plugins"] = opaque
            p4["frame"] = atomic.frame
            node = self.b.doc["nodes"][frame_node]
            if skin is not None:
                # Skinned atomics: a scene-root node (glTF ignores a skinned mesh's node transform and
                # parents); extras.p4.atomic.frame keeps the RW frame the atomic hangs from.
                target_index = self.b.add("nodes", {"name": "skinned_%d" % ai})
                roots.append(target_index)
                target = self.b.doc["nodes"][target_index]
                target["skin"] = skin
            elif "mesh" not in node:
                target = node
            else:
                child = self.b.add("nodes", {"name": "atomic_%d" % ai})
                node.setdefault("children", []).append(child)
                target = self.b.doc["nodes"][child]
            target["mesh"] = mesh
            target.setdefault("extras", {}).setdefault("p4", {})["atomic"] = p4
        slots = self.rmd.slots()
        slot_names = {}
        for entry in slots:
            if entry["animation"] is not None and entry["animation"] not in slot_names:
                slot_names[entry["animation"]] = "L%dS%02d" % (entry["layer"], entry["slot"])
        self._animations(slot_names)
        self.b.add("scenes", {"name": self.name, "nodes": roots,
                              "extras": {"p4": {"convention": CONVENTION, "units": "P4 (about 1 cm), Y up",
                                                "source": self.source}}})
        self.b.doc["scene"] = 0
        if used_ext:
            self.b.doc["extensionsUsed"] = sorted(used_ext)
        self.sidecar = self._sidecar(slots)
        return self

    def _sidecar(self, slots):
        chunks = []
        represented = {rw.CLUMP: "gltf scene", rw.TEXDICTIONARY: "gltf images", rw.ANIMANIMATION: "gltf animation"}
        for index, chunk in enumerate(self.rmd.chunks):
            entry = {"index": index, "type": "0x%x" % chunk.kind, "name": chunk.name, "libid": "0x%x" % chunk.libid}
            if chunk.kind in represented and chunk.obj is not None:
                entry["in"] = represented[chunk.kind]
            else:
                entry["bytes"] = _b64(chunk.payload)
            if chunk.kind == rw.RMD_SLOT_COUNT:
                entry["slots"] = int.from_bytes(chunk.payload[:2], "little")
            chunks.append(entry)
        anim_names = [a["name"] for a in self.b.doc["animations"]]
        anim_chunk_to_gltf = {}
        for gi, anim in enumerate(self.b.doc["animations"]):
            anim_chunk_to_gltf[anim["extras"]["p4"]["chunk"]] = gi
        anim_chunks = [i for i, c in enumerate(self.rmd.chunks) if c.kind == rw.ANIMANIMATION]
        for entry in slots:
            if entry["animation"] is not None:
                entry["gltfAnimation"] = anim_chunk_to_gltf.get(anim_chunks[entry["animation"]])
            entry["blendGltfAnimations"] = [anim_chunk_to_gltf.get(anim_chunks[a]) for a in entry["blend"]]
        bases = {str(k): v for k, v in getattr(self.rmd, "slot_bases", {}).items()}
        return {
            "format": SIDECAR_FORMAT, "version": SIDECAR_VERSION, "convention": CONVENTION,
            "source": {"name": self.source, "bytes": len(self.data), "sha256": hashlib.sha256(self.data).hexdigest()},
            "rw": {"libid": "0x%x" % self.clump.libid, "version": "0x%x" % rw.library_version(self.clump.libid)[0]},
            "chunks": chunks,
            "trailingBytes": len(self.rmd.trailing),
            "slots": slots, "slotBases": bases, "animations": anim_names,
            "attach": [{"id": ident, "hanimId": node, "matrix": list(m)} for ident, node, m in self.rmd.attach_table()],
            "textures": self.tex_info,
            "clumpPlugins": self.clump.extension.opaque_json(()),
            "warnings": self.warnings,
        }

    def write(self, out_dir):
        os.makedirs(out_dir, exist_ok=True)
        written = []
        if self.glb:
            path = os.path.join(out_dir, self.name + ".glb")
            with open(path, "wb") as stream:
                stream.write(self.b.glb())
            written.append(path)
        else:
            path = os.path.join(out_dir, self.name + ".gltf")
            binary = self.b.write_gltf(path, self.name + ".bin")
            written.append(path)
            if binary:
                with open(os.path.join(out_dir, self.name + ".bin"), "wb") as stream:
                    stream.write(binary)
            for file_name, payload in self.files.items():
                with open(os.path.join(out_dir, file_name), "wb") as stream:
                    stream.write(payload)
        side = os.path.join(out_dir, self.name + ".p4model.json")
        self.sidecar["gltf"] = os.path.basename(written[0])
        with open(side, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(gltfio.finite(self.sidecar), stream, indent=1, allow_nan=False)
            stream.write("\n")
        written.append(side)
        return written

    def summary(self):
        doc = self.b.doc
        tris = sum(acc_count(doc, p["indices"]) // 3 for m in doc["meshes"] for p in m["primitives"])
        bones = len(self.clump.hanim_frames())
        return "%s: %d nodes, %d meshes, %d triangles, %d bones, %d animations, %d images" % (
            self.name, len(doc["nodes"]), len(doc["meshes"]), tris, bones, len(doc["animations"]), len(doc["images"]))


def acc_count(doc, index):
    return doc["accessors"][index]["count"]


def _safe(name):
    return "".join(c if c.isalnum() or c in "-_." else "_" for c in name)


def model_name(spec):
    """Output name: the file stem, "<pack stem>-<member stem>" for a pack member (so WP070.PAC#wp070.RMD
    and WP070.RMD stay apart on case-insensitive file systems)."""
    def stem(path):
        base = path.replace("\\", "/").split("/")[-1]
        if "." not in base:
            return base
        name, ext = base.rsplit(".", 1)
        # ES001.RMD_ and ES001.RMD are different files: keep any extension but the plain ones.
        return name if ext.upper() in ("RMD", "PAC", "P00", "P01") else name + "_" + ext
    path, _, member = spec.partition("#")
    return _safe(stem(path) + "-" + stem(member)) if member else _safe(stem(path))


def expand(specs, disc):
    """Disc globs and "pack#*" members expanded to concrete specs."""
    out = []
    for spec in specs:
        path, _, member = spec.partition("#")
        if os.path.isfile(path):
            data = None
            if member == "*":
                with open(path, "rb") as stream:
                    data = stream.read()
                out.extend(path + "#" + n for n, _o, _s in p4disc.pac_entries(data) if n.upper().endswith(".RMD"))
            else:
                out.append(spec)
            continue
        if disc is None:
            raise SystemExit("rw_to_gltf: %s is not a local file and no disc image is available (set P4PC_ISO or --iso)" % spec)
        keys = [k for k in disc.files if fnmatch.fnmatchcase(k, path.replace("\\", "/").upper())]
        if not keys:
            raise SystemExit("rw_to_gltf: nothing on the disc matches %s" % path)
        for key in sorted(keys):
            if member == "*":
                out.extend(key + "#" + n for n, _o, _s in p4disc.pac_entries(disc.read(key)) if n.upper().endswith(".RMD"))
            elif member:
                out.append(key + "#" + member)
            else:
                out.append(key)
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("specs", nargs="*", help="disc keys or globs, pack members (key#member, key#*), or local files")
    parser.add_argument("--iso", help="disc image (default: P4PC_ISO)")
    parser.add_argument("--out", help="output directory (one subdirectory per model)")
    parser.add_argument("--glb", action="store_true", help="write one .glb per model instead of .gltf + .bin + PNGs")
    parser.add_argument("--list", metavar="PREFIX", help="list the disc's RMD and PAC keys under PREFIX")
    parser.add_argument("--check", action="store_true", help="convert in memory only: report what would be written")
    args = parser.parse_args(argv)
    iso = args.iso or p4disc.default_iso()
    disc = p4disc.Disc(iso) if iso else None
    if args.list is not None:
        if disc is None:
            raise SystemExit("rw_to_gltf: no disc image")
        for key in disc.keys(args.list):
            if ".RMD" in key or key.endswith(".PAC"):
                print(key)
        return 0
    if not args.specs or not (args.out or args.check):
        parser.error("give model specs and --out (or --check)")
    failures = converted = 0
    for spec in expand(args.specs, disc):
        name = model_name(spec)
        try:
            conv = Converter(p4disc.load(spec, disc), name, spec, args.glb).run()
        except (rw.FormatError, ConvertError, ps2tex.TextureError, KeyError, ValueError) as error:
            print("rw_to_gltf: %s: %s" % (spec, error), file=sys.stderr)
            failures += 1
            continue
        converted += 1
        if not args.check:
            conv.write(os.path.join(args.out, name))
        print(conv.summary())
        for warning in conv.warnings:
            print("  warning: %s" % warning)
    print("rw_to_gltf: %d converted, %d failed" % (converted, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
