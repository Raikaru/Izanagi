"""Decode P4's PS2 native textures (rwID_TEXTURENATIVE "PS2\\0" rasters) to straight RGBA8.

This is the port's reader (src/render/p4gl_nativetex.c, retail 003fd940 readTEXFromStream) in Python:

* Header word 3: low 16 bits are the RwRasterCreate flags (0x2504 PAL8 | 8888 | texture, 0x4504 PAL4,
  0x0504 32-bit, 0x8000 mipmaps), high 16 bits the data kind. Every P4 model texture is kind 2: the data
  chunk is the GIF stream the sky texture cache replays (A+D TRXPOS/TRXREG/TRXDIR then an IMAGE per
  upload). The first IMAGE is mip level 0, the last one the CLUT.
* PSMT8 and PSMT4 pixels were uploaded through PSMCT32 / PSMCT16 (the usual pre-swizzle); the formulas
  below are the ones in p4gl_nativetex.c's header.
* PS2 alpha is 0..0x80; it is doubled (0x80 and above read 0xFF) as the port does.

encode_*() write the same layouts, so tests can build synthetic textures that the decoder (and the
port's C decoder) reads back.
"""
import struct

KIND_GIF = 2
FLAG_PAL8, FLAG_PAL4, FLAG_MIPMAP = 0x2000, 0x4000, 0x8000

_BLOCK16 = ((0, 2, 8, 10), (1, 3, 9, 11), (4, 6, 12, 14), (5, 7, 13, 15),
            (16, 18, 24, 26), (17, 19, 25, 27), (20, 22, 28, 30), (21, 23, 29, 31))


class TextureError(ValueError):
    pass


def gif_images(data):
    """Payloads of the IMAGE (FLG=2) packets of a GIF stream, in order."""
    images = []
    offset = 0
    while offset + 16 <= len(data):
        tag = struct.unpack_from("<Q", data, offset)[0]
        offset += 16
        nloop = tag & 0x7FFF
        flg = (tag >> 58) & 3
        nreg = (tag >> 60) & 15 or 16
        if flg == 0:
            offset += 16 * nloop * nreg
        elif flg == 1:
            offset += (nloop * nreg + 1) // 2 * 16
        elif flg == 2:
            if offset + 16 * nloop > len(data):
                raise TextureError("GIF IMAGE overruns the data chunk")
            images.append(data[offset:offset + 16 * nloop])
            offset += 16 * nloop
        else:
            break
    return images


def _ct16_address(x, y, pages_across):
    page = (y >> 6) * pages_across + (x >> 6)
    blk = _BLOCK16[(y >> 3) & 7][(x >> 4) & 3]
    g = 2 * (x & 1) + 8 * ((x >> 1) & 3) + ((x & 15) >> 3)
    return page * 4096 + blk * 128 + ((y >> 1) & 3) * 32 + (y & 1) * 4 + g


def _t4_nibble(x, y):
    col = (y >> 2) & 3
    yy = y & 3
    rot = (yy >> 1) * 4
    if col & 1:
        rot = (rot + 4) & 7
    b = ((x & 7) + rot) & 7
    f = (b & 1) * 8 + (b >> 1) * 32
    return col * 128 + 2 * ((x >> 3) & 3) + f + (yy >> 1) + (yy & 1) * 16


def _psmt8_word(x, y, w):
    """(word index, byte) of PSMT8 pixel (x, y) in the (w/2 x h/2)-word PSMCT32 upload."""
    swap = (((y + 2) >> 2) & 1) * 4
    pos_y = (((y & ~3) >> 1) + (y & 1)) & 7
    row = (y >> 4) * 8 + pos_y
    col = (x >> 4) * 8 + ((x + swap) & 7)
    return row * (w // 2) + col, ((y >> 1) & 1) + ((x >> 2) & 2)


def unswizzle8(src, w, h):
    if w % 16 or h % 16 or len(src) < w * h:
        raise TextureError("PSMT8 image %dx%d does not fit its %d-byte upload" % (w, h, len(src)))
    out = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            word, byte = _psmt8_word(x, y, w)
            out[y * w + x] = src[word * 4 + byte]
    return bytes(out)


def _psmt4_map(w, h):
    """For each PSMT4 pixel: (half word index into the PSMCT16 upload or -1, nibble shift)."""
    cw, chh = w // 2, h // 2
    across, down = (cw + 63) // 64, (chh + 63) // 64
    inverse = {}
    for y in range(chh):
        for x in range(cw):
            inverse[_ct16_address(x, y, across)] = y * cw + x
    pages_across = (w + 127) // 128
    out = []
    for y in range(h):
        for x in range(w):
            page = (y >> 7) * pages_across + (x >> 7)
            blk = _BLOCK16[(y >> 4) & 7][(x >> 5) & 3]
            n = _t4_nibble(x, y)
            hw = 2 * (page * 2048 + blk * 64 + (n >> 3)) + ((n & 7) >> 2)
            out.append((inverse.get(hw, -1), 4 * (n & 3)))
    return out


def unswizzle4(src, w, h):
    if w % 16 or h % 8 or len(src) < w * h // 2:
        raise TextureError("PSMT4 image %dx%d does not fit its %d-byte upload" % (w, h, len(src)))
    out = bytearray(w * h)
    for i, (hw, shift) in enumerate(_psmt4_map(w, h)):
        if hw >= 0:
            v = src[2 * hw] | (src[2 * hw + 1] << 8)
            out[i] = (v >> shift) & 15
    return bytes(out)


def clut_index(i):
    """Position of logical palette entry i in a 256-entry CSM1 CLUT (bits 3 and 4 swapped)."""
    return ((i & 7) | ((i & 16) >> 1)) + (((i >> 5) << 1) | ((i >> 3) & 1)) * 16


def ps2_alpha(a):
    return 0xFF if a >= 0x80 else a * 2


def rgba_alpha_to_ps2(a):
    return (a + 1) // 2


def decode(tex):
    """(width, height, RGBA bytes, palette or None) of a NativeTexture (rwmodel), level 0 only."""
    w, h, depth = tex.width, tex.height, tex.depth
    if tex.kind != KIND_GIF:
        raise TextureError("texture %s: data kind %d (P4 model textures are kind 2)" % (tex.name, tex.kind))
    images = gif_images(tex.data)
    if depth == 32:
        if not images or len(images[0]) < w * h * 4:
            raise TextureError("texture %s: 32-bit upload too short" % tex.name)
        px = images[0]
        out = bytearray(px[:w * h * 4])
        for i in range(3, len(out), 4):
            out[i] = ps2_alpha(out[i])
        return w, h, bytes(out), None
    if depth not in (4, 8) or len(images) < 2:
        raise TextureError("texture %s: depth %d with %d uploads" % (tex.name, depth, len(images)))
    block = 16 if depth == 8 else 32
    pw = (w + block - 1) & ~(block - 1)
    if pw != w and len(images[0]) * 2 < pw * h * (2 if depth == 8 else 1):
        pw = w
    indices = unswizzle8(images[0], pw, h) if depth == 8 else unswizzle4(images[0], pw, h)
    if pw != w:
        indices = b"".join(indices[y * pw:y * pw + w] for y in range(h))
    clut = images[-1]
    entries = 256 if depth == 8 else 16
    if len(clut) < entries * 4:
        raise TextureError("texture %s: CLUT too short" % tex.name)
    palette = []
    for i in range(entries):
        src = clut_index(i) if depth == 8 else i
        r, g, b, a = clut[4 * src:4 * src + 4]
        palette.append((r, g, b, ps2_alpha(a)))
    out = bytearray(w * h * 4)
    for i, index in enumerate(indices):
        out[4 * i:4 * i + 4] = bytes(palette[index])
    return w, h, bytes(out), palette


# ---------------------------------------------------------------------------------------------------
# Encoders (synthetic fixtures; the inverse of the layouts above)

def _gif_tag(nloop, flg, nreg=1, eop=0, regs=0):
    return struct.pack("<QQ", (nloop & 0x7FFF) | (eop << 15) | (flg << 58) | ((nreg & 15) << 60), regs)


def _upload(payload, dbp_w, dbp_h):
    """One A+D TRXPOS/TRXREG/TRXDIR packet and the IMAGE carrying payload (a multiple of 16 bytes)."""
    out = _gif_tag(3, 0, 1, 0, 0xE)
    out += struct.pack("<QQ", 0, 0x51)
    out += struct.pack("<QQ", dbp_w | (dbp_h << 32), 0x52)
    out += struct.pack("<QQ", 0, 0x53)
    out += _gif_tag(len(payload) // 16, 2)
    return out + payload


def swizzle8(indices, w, h):
    out = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            word, byte = _psmt8_word(x, y, w)
            out[word * 4 + byte] = indices[y * w + x]
    return bytes(out)


def swizzle4(indices, w, h):
    out = bytearray(w * h // 2)
    for i, (hw, shift) in enumerate(_psmt4_map(w, h)):
        if hw < 0:
            raise TextureError("PSMT4 %dx%d has pixels outside its upload" % (w, h))
        v = (out[2 * hw] | (out[2 * hw + 1] << 8)) & ~(15 << shift)
        v |= (indices[i] & 15) << shift
        out[2 * hw], out[2 * hw + 1] = v & 0xFF, v >> 8
    return bytes(out)


def encode_paletted(indices, palette, w, h, depth):
    """GIF data chunk for a PAL8/PAL4 texture; palette entries are straight RGBA (alpha 0..255)."""
    if depth == 8:
        pixels = swizzle8(indices, w, h)
        clut = bytearray(256 * 4)
        for i, (r, g, b, a) in enumerate(palette[:256]):
            clut[4 * clut_index(i):4 * clut_index(i) + 4] = bytes((r, g, b, rgba_alpha_to_ps2(a)))
        return _upload(pixels, w // 2, h // 2) + _upload(bytes(clut), 16, 16)
    pixels = swizzle4(indices, w, h)
    clut = bytearray(16 * 4 * 4)
    for i, (r, g, b, a) in enumerate(palette[:16]):
        clut[4 * i:4 * i + 4] = bytes((r, g, b, rgba_alpha_to_ps2(a)))
    return _upload(pixels, w // 2, h // 2) + _upload(bytes(clut), 8, 2)


def encode_rgba32(rgba, w, h):
    px = bytearray(rgba)
    for i in range(3, len(px), 4):
        px[i] = rgba_alpha_to_ps2(px[i])
    return _upload(bytes(px), w, h)
