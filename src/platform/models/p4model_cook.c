/* glTF 2.0 -> cooked model (p4model.h). Parses with cgltf (third_party/cgltf), decodes PNG textures with
 * LodePNG, converts textures to P4's paletted or 32-bit formats, bakes every static mesh into the space
 * of the frame it attaches to, and checks the target's budget. Messages name the glTF object and the
 * exceeded bound. docs/modding/models.md ("Runtime glTF models") is the contract. */
#define CGLTF_IMPLEMENTATION
#include "../../../third_party/cgltf/cgltf.h"
#ifndef LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_DISK
#endif
#ifndef LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ENCODER
#endif
#ifndef LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#endif
#ifndef LODEPNG_NO_COMPILE_ALLOCATORS
#define LODEPNG_NO_COMPILE_ALLOCATORS
#endif
#include "../../../third_party/lodepng/lodepng.h"
/* The port's budgeted LodePNG allocators (src/platform/mods/resource_png.c). */
void lodepng_free(void *pointer);

#include "p4model.h"
#include "p4pc/content_digest.h"

#include <fenv.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const P4ModelBudget budgets[] = {
    /* category, triangles, meshes, vertices/mesh, textures, texture size, texture bytes, source bytes */
    {"weapon", 4096u, 8u, 65535u, 8u, 256u, 512u * 1024u, 16u * 1024u * 1024u},
    {"character", 16384u, 64u, 65535u, 32u, 512u, 2u * 1024u * 1024u, 64u * 1024u * 1024u},
    {"field", 32768u, 256u, 65535u, 64u, 512u, 4u * 1024u * 1024u, 64u * 1024u * 1024u},
};

static int prefix(const char *key, const char *p)
{
    return strncmp(key, p, strlen(p)) == 0;
}

const P4ModelBudget *p4_model_budget_for(const char *key)
{
    if (!key) return NULL;
    if (prefix(key, "DATA/MODEL/WEAPON/")) return &budgets[0];
    if (prefix(key, "DATA/MODEL/PACK/") || prefix(key, "BTL/MODEL_BTL/") || prefix(key, "DATA/MODEL/NPC") ||
        prefix(key, "DATA/MODEL/PERSONA/") || prefix(key, "DATA/MODEL/SYMBOL/"))
        return &budgets[1];
    return &budgets[2];
}

/* ------------------------------------------------------------------------------------------------
 * Output blob */

typedef struct Blob {
    uint8_t *data;
    size_t size, capacity;
    int failed;
} Blob;

static uint32_t crc32_of(const uint8_t *bytes, size_t size);

static size_t blob_reserve(Blob *b, size_t bytes, size_t align)
{
    size_t start = (b->size + align - 1) / align * align;
    if (b->failed || start < b->size || bytes > (size_t)UINT32_MAX - start) {
        b->failed = 1;
        return 0;
    }
    size_t end = start + bytes;
    if (end > b->capacity) {
        size_t next = b->capacity ? b->capacity : 4096;
        while (next < end) next *= 2;
        uint8_t *grown = realloc(b->data, next);
        if (!grown) {
            b->failed = 1;
            return 0;
        }
        b->data = grown;
        b->capacity = next;
    }
    memset(b->data + b->size, 0, end - b->size);
    b->size = end;
    return start;
}

static uint32_t blob_put(Blob *b, const void *bytes, size_t size)
{
    size_t at = blob_reserve(b, size, 4);
    if (!b->failed && size) memcpy(b->data + at, bytes, size);
    return (uint32_t)at;
}

/* ------------------------------------------------------------------------------------------------
 * Errors */

typedef struct Cook {
    const char *name;
    const P4ModelBudget *budget;
    char *error;
    size_t error_size;
    P4ModelReadFn read;
    void *user;
    cgltf_data *data;
    char source[68];
    /* collected records */
    P4ModelCookedTexture *textures;
    size_t texture_count;
    const cgltf_texture **texture_keys;
    uint8_t **texture_data;   /* palette (depth 8) + pixels, owned */
    size_t *texture_data_size;
    P4ModelCookedMaterial *materials;
    const cgltf_material **material_keys;
    size_t material_count;
    uint32_t triangles, texture_bytes;
} Cook;

static int refuse(Cook *c, const char *format, ...)
{
    if (c->error && c->error_size) {
        int used = snprintf(c->error, c->error_size, "%s: ", c->name ? c->name : "glTF");
        if (used < 0) used = 0;
        if ((size_t)used < c->error_size) {
            va_list args;
            va_start(args, format);
            vsnprintf(c->error + used, c->error_size - (size_t)used, format, args);
            va_end(args);
        }
    }
    return -1;
}

/* ------------------------------------------------------------------------------------------------
 * File access through the caller (package plans); cgltf never opens host files. */

static cgltf_result read_file(const struct cgltf_memory_options *memory, const struct cgltf_file_options *file,
                              const char *path, cgltf_size *size, void **data)
{
    (void)memory;
    Cook *c = file->user_data;
    uint8_t *bytes = NULL;
    size_t length = 0;
    if (!c->read || c->read(c->user, path, &bytes, &length) != 0) return cgltf_result_file_not_found;
    *size = length;
    *data = bytes;
    return cgltf_result_success;
}

static void release_file(const struct cgltf_memory_options *memory, const struct cgltf_file_options *file, void *data)
{
    (void)memory;
    (void)file;
    free(data);
}

static const char *result_text(cgltf_result r)
{
    switch (r) {
    case cgltf_result_data_too_short: return "data too short";
    case cgltf_result_unknown_format: return "not glTF 2.0 JSON or GLB";
    case cgltf_result_invalid_json: return "invalid JSON";
    case cgltf_result_invalid_gltf: return "invalid glTF";
    case cgltf_result_invalid_options: return "invalid options";
    case cgltf_result_file_not_found: return "a file it names is missing from the package";
    case cgltf_result_io_error: return "I/O error";
    case cgltf_result_out_of_memory: return "out of memory";
    case cgltf_result_legacy_gltf: return "glTF 1.0 (only 2.0 is supported)";
    default: return "unreadable";
    }
}

static int parse(Cook *c, const uint8_t *gltf, size_t size)
{
    cgltf_options options;
    memset(&options, 0, sizeof options);
    options.file.read = read_file;
    options.file.release = release_file;
    options.file.user_data = c;
    /* glTF JSON uses round-to-nearest, independently of the game's EE floating-point mode. */
    fenv_t saved;
    fegetenv(&saved);
    fesetround(FE_TONEAREST);
    cgltf_result r = cgltf_parse(&options, gltf, size, &c->data);
    fesetenv(&saved);
    if (r != cgltf_result_success) return refuse(c, "cannot parse: %s", result_text(r));
    r = cgltf_load_buffers(&options, c->data, "");
    if (r != cgltf_result_success) return refuse(c, "cannot load buffers: %s", result_text(r));
    r = cgltf_validate(c->data);
    if (r != cgltf_result_success) return refuse(c, "fails glTF validation (accessor, buffer or index out of range)");
    for (cgltf_size i = 0; i < c->data->extensions_required_count; ++i) {
        const char *ext = c->data->extensions_required[i];
        if (strcmp(ext, "KHR_materials_unlit") && strcmp(ext, "KHR_texture_transform"))
            return refuse(c, "requires extension %s, which the game cannot use", ext);
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Hash of every input byte (cache key) */

static int image_bytes(Cook *c, const cgltf_image *image, const uint8_t **bytes, size_t *size, uint8_t **owned);

int p4_model_source_hash(const char *name, const uint8_t *gltf, size_t size, P4ModelReadFn read, void *user,
                         char hex[68], char *error, size_t error_size)
{
    Cook c;
    memset(&c, 0, sizeof c);
    c.name = name ? name : "glTF";
    c.read = read;
    c.user = user;
    c.error = error;
    c.error_size = error_size;
    if (parse(&c, gltf, size)) {
        cgltf_free(c.data);
        return -1;
    }
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    p4_content_digest_update(&digest, P4_MODEL_COOKER_ID, sizeof P4_MODEL_COOKER_ID);
    p4_content_digest_update(&digest, gltf, size);
    for (cgltf_size i = 0; i < c.data->buffers_count; ++i) {
        const cgltf_buffer *buffer = &c.data->buffers[i];
        if (buffer->uri && buffer->data) p4_content_digest_update(&digest, buffer->data, buffer->size);
    }
    for (cgltf_size i = 0; i < c.data->images_count; ++i) {
        const cgltf_image *image = &c.data->images[i];
        if (image->uri && strncmp(image->uri, "data:", 5)) {
            const uint8_t *bytes;
            size_t length;
            uint8_t *owned = NULL;
            if (image_bytes(&c, image, &bytes, &length, &owned)) {
                cgltf_free(c.data);
                return -1;
            }
            p4_content_digest_update(&digest, bytes, length);
            free(owned);
        }
    }
    char text[P4_CONTENT_DIGEST_HEX + 1];
    p4_content_digest_end(&digest, text);
    memcpy(hex, text, sizeof text);
    cgltf_free(c.data);
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Math */

static void mat_mul(const float a[16], const float b[16], float out[16])
{
    /* column-major, out = a * b */
    float r[16];
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += a[k * 4 + row] * b[col * 4 + k];
            r[col * 4 + row] = s;
        }
    memcpy(out, r, sizeof r);
}

static int mat_invert_affine(const float m[16], float out[16])
{
    float a = m[0], b = m[4], cc = m[8], d = m[1], e = m[5], f = m[9], g = m[2], h = m[6], i = m[10];
    float det = a * (e * i - f * h) - b * (d * i - f * g) + cc * (d * h - e * g);
    if (!(fabsf(det) > 1e-12f)) return -1;
    float inv[9] = {(e * i - f * h) / det, -(b * i - cc * h) / det, (b * f - cc * e) / det,
                    -(d * i - f * g) / det, (a * i - cc * g) / det, -(a * f - cc * d) / det,
                    (d * h - e * g) / det, -(a * h - b * g) / det, (a * e - b * d) / det};
    /* inv is row-major of the 3x3 */
    float r[16] = {inv[0], inv[3], inv[6], 0, inv[1], inv[4], inv[7], 0, inv[2], inv[5], inv[8], 0, 0, 0, 0, 1};
    float t[3] = {m[12], m[13], m[14]};
    for (int row = 0; row < 3; ++row)
        r[12 + row] = -(r[row] * t[0] + r[4 + row] * t[1] + r[8 + row] * t[2]);
    memcpy(out, r, sizeof r);
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Frames */

static int numbered(const char *name, const char *stem, int32_t *id)
{
    size_t n = strlen(stem);
    if (!name || strncmp(name, stem, n)) return 0;
    const char *p = name + n;
    if (*p < '0' || *p > '9') return 0;
    long value = 0;
    while (*p >= '0' && *p <= '9') {
        value = value * 10 + (*p - '0');
        if (value > 0x7FFFFFFF) return 0;
        ++p;
    }
    /* Blender may append ".001" to a duplicated name; anything else is not a frame name. */
    if (*p && *p != '.') return 0;
    *id = (int32_t)value;
    return 1;
}

/* The frame a mesh node attaches to: the nearest ancestor-or-self named bone_<HAnim id>,
 * helper_<per3HelperId> or frame_0; without one, the clump root frame in scene space. */
static const cgltf_node *anchor_of(const cgltf_node *node, int32_t *kind, int32_t *id)
{
    for (const cgltf_node *n = node; n; n = n->parent) {
        int32_t value;
        if (numbered(n->name, "bone_", &value)) {
            *kind = P4_MODEL_FRAME_HANIM;
            *id = value;
            return n;
        }
        if (numbered(n->name, "helper_", &value)) {
            *kind = P4_MODEL_FRAME_HELPER;
            *id = value;
            return n;
        }
        if (numbered(n->name, "frame_", &value) && value == 0) {
            *kind = P4_MODEL_FRAME_ROOT;
            *id = 0;
            return n;
        }
    }
    *kind = P4_MODEL_FRAME_ROOT;
    *id = 0;
    return NULL;
}

/* ------------------------------------------------------------------------------------------------
 * Textures */

static int base64_value(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return -1;
}

static int image_bytes(Cook *c, const cgltf_image *image, const uint8_t **bytes, size_t *size, uint8_t **owned)
{
    *owned = NULL;
    const char *label = image->name ? image->name : (image->uri ? image->uri : "(embedded)");
    if (image->buffer_view) {
        const uint8_t *view = cgltf_buffer_view_data(image->buffer_view);
        if (!view) return refuse(c, "image %s: its buffer view has no data", label);
        *bytes = view;
        *size = image->buffer_view->size;
        return 0;
    }
    if (!image->uri) return refuse(c, "image %s has neither a URI nor a buffer view", label);
    if (!strncmp(image->uri, "data:", 5)) {
        const char *comma = strchr(image->uri, ',');
        if (!comma || !strstr(image->uri, ";base64")) return refuse(c, "image %s: data URI is not base64", label);
        size_t length = strlen(comma + 1);
        uint8_t *out = malloc(length / 4 * 3 + 3);
        if (!out) return refuse(c, "out of memory");
        size_t n = 0;
        unsigned acc = 0;
        int bits = 0;
        for (const char *p = comma + 1; *p && *p != '='; ++p) {
            int v = base64_value((unsigned char)*p);
            if (v < 0) {
                free(out);
                return refuse(c, "image %s: invalid base64", label);
            }
            acc = (acc << 6) | (unsigned)v;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out[n++] = (uint8_t)(acc >> bits);
            }
        }
        *owned = out;
        *bytes = out;
        *size = n;
        return 0;
    }
    char uri[1024];
    snprintf(uri, sizeof uri, "%s", image->uri);
    cgltf_decode_uri(uri);
    uint8_t *data = NULL;
    size_t length = 0;
    if (!c->read || c->read(c->user, uri, &data, &length) != 0)
        return refuse(c, "image %s: file %s is missing from the package", label, uri);
    *owned = data;
    *bytes = data;
    *size = length;
    return 0;
}

static uint32_t filter_addressing(const cgltf_sampler *sampler)
{
    /* RwTextureFilterMode: 1 nearest, 2 linear (no mip levels are built). Addressing: 1 wrap, 2 mirror, 3 clamp. */
    uint32_t filter = 2, u = 1, v = 1;
    if (sampler) {
        if (sampler->mag_filter == 9728) filter = 1;
        u = sampler->wrap_s == 33648 ? 2 : sampler->wrap_s == 33071 ? 3 : 1;
        v = sampler->wrap_t == 33648 ? 2 : sampler->wrap_t == 33071 ? 3 : 1;
    }
    return filter | (u << 8) | (v << 12);
}

static int power_of_two(uint32_t v)
{
    return v && !(v & (v - 1));
}

/* Index of the cooked texture for a glTF texture, converting it on first use. */
static int cook_texture(Cook *c, const cgltf_texture *texture, int32_t *out)
{
    for (size_t i = 0; i < c->texture_count; ++i)
        if (c->texture_keys[i] == texture) {
            *out = (int32_t)i;
            return 0;
        }
    const cgltf_image *image = texture->image;
    const char *label = texture->name ? texture->name : (image && image->name ? image->name : "(unnamed)");
    if (!image) return refuse(c, "texture %s has no image", label);
    if (c->texture_count >= c->budget->textures)
        return refuse(c, "more than %u textures (the %s budget)", (unsigned)c->budget->textures, c->budget->category);
    if (image->mime_type && strcmp(image->mime_type, "image/png"))
        return refuse(c, "texture %s is %s; only PNG textures are supported", label, image->mime_type);
    const uint8_t *bytes;
    size_t size;
    uint8_t *owned;
    if (image_bytes(c, image, &bytes, &size, &owned)) return -1;
    if (size < 8 || memcmp(bytes, "\x89PNG\r\n\x1a\n", 8)) {
        free(owned);
        return refuse(c, "texture %s is not a PNG file", label);
    }
    unsigned width = 0, height = 0;
    uint8_t *rgba = NULL;
    unsigned code = lodepng_decode32(&rgba, &width, &height, bytes, size);
    free(owned);
    if (code || !rgba) {
        lodepng_free(rgba);
        return refuse(c, "texture %s: invalid PNG (%s)", label, lodepng_error_text(code));
    }
    if (!power_of_two(width) || !power_of_two(height) || width < 8 || height < 8) {
        lodepng_free(rgba);
        return refuse(c, "texture %s is %ux%u; P4 textures are powers of two from 8", label, width, height);
    }
    if (width > c->budget->textureSize || height > c->budget->textureSize) {
        lodepng_free(rgba);
        return refuse(c, "texture %s is %ux%u; at most %u for a %s", label, width, height,
                      (unsigned)c->budget->textureSize, c->budget->category);
    }
    /* Exact palette when the image has at most 256 distinct colours (PAL8, as nearly all retail model
     * textures), else straight 32-bit RGBA (as retail's few 32-bit textures). Never lossy. */
    size_t pixels = (size_t)width * height;
    uint32_t palette[256];
    unsigned colours = 0;
    uint8_t *indices = malloc(pixels);
    int paletted = indices != NULL;
    for (size_t p = 0; paletted && p < pixels; ++p) {
        uint32_t v;
        memcpy(&v, rgba + 4 * p, 4);
        unsigned k = 0;
        while (k < colours && palette[k] != v) ++k;
        if (k == colours) {
            if (colours == 256) {
                paletted = 0;
                break;
            }
            palette[colours++] = v;
        }
        indices[p] = (uint8_t)k;
    }
    size_t payload = paletted ? 1024 + pixels : pixels * 4;
    if ((uint64_t)c->texture_bytes + payload > c->budget->textureBytes) {
        free(indices);
        lodepng_free(rgba);
        return refuse(c, "textures exceed %u KiB (the %s budget) at texture %s", (unsigned)(c->budget->textureBytes / 1024),
                      c->budget->category, label);
    }
    uint8_t *data = malloc(payload);
    if (!data) {
        free(indices);
        lodepng_free(rgba);
        return refuse(c, "out of memory");
    }
    if (paletted) {
        memset(data, 0, 1024);
        memcpy(data, palette, (size_t)colours * 4);
        memcpy(data + 1024, indices, pixels);
    } else {
        memcpy(data, rgba, pixels * 4);
    }
    free(indices);
    lodepng_free(rgba);
    size_t n = c->texture_count;
    P4ModelCookedTexture *records = realloc(c->textures, (n + 1) * sizeof *records);
    const cgltf_texture **keys = realloc(c->texture_keys, (n + 1) * sizeof *keys);
    uint8_t **datas = realloc(c->texture_data, (n + 1) * sizeof *datas);
    size_t *sizes = realloc(c->texture_data_size, (n + 1) * sizeof *sizes);
    if (records) c->textures = records;
    if (keys) c->texture_keys = keys;
    if (datas) c->texture_data = datas;
    if (sizes) c->texture_data_size = sizes;
    if (!records || !keys || !datas || !sizes) {
        free(data);
        return refuse(c, "out of memory");
    }
    P4ModelCookedTexture *t = &c->textures[n];
    memset(t, 0, sizeof *t);
    snprintf(t->name, sizeof t->name, "m%.8s_%02u", c->source, (unsigned)n);
    t->width = width;
    t->height = height;
    t->depth = paletted ? 8u : 32u;
    t->filterAddressing = filter_addressing(texture->sampler);
    c->texture_keys[n] = texture;
    c->texture_data[n] = data;
    c->texture_data_size[n] = payload;
    c->texture_bytes += (uint32_t)payload;
    c->texture_count = n + 1;
    *out = (int32_t)n;
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Materials */

/* Value text of "key" inside the "p4" object of an extras JSON string, or NULL. Tolerant scanner for
 * the flat fields this cooker reads; the glTF itself was already validated by cgltf. */
static const char *extras_field(const char *json, const char *key)
{
    if (!json) return NULL;
    const char *p4 = strstr(json, "\"p4\"");
    if (!p4) return NULL;
    char pattern[64];
    snprintf(pattern, sizeof pattern, "\"%s\"", key);
    const char *at = strstr(p4, pattern);
    if (!at) return NULL;
    at += strlen(pattern);
    while (*at == ' ' || *at == '\t' || *at == '\n' || *at == '\r') ++at;
    if (*at != ':') return NULL;
    ++at;
    while (*at == ' ' || *at == '\t' || *at == '\n' || *at == '\r') ++at;
    return at;
}
static int json_floats(const char *p, float *out, int count);

static uint8_t unit_byte(float v)
{
    if (!(v > 0.0f)) return 0;
    if (v >= 1.0f) return 255;
    return (uint8_t)(v * 255.0f + 0.5f);
}

static int cook_material(Cook *c, const cgltf_material *material, uint32_t *out)
{
    for (size_t i = 0; i < c->material_count; ++i)
        if (c->material_keys[i] == material) {
            *out = (uint32_t)i;
            return 0;
        }
    P4ModelCookedMaterial m;
    memset(&m, 0, sizeof m);
    m.color[0] = m.color[1] = m.color[2] = m.color[3] = 255;
    m.ambient = m.specular = m.diffuse = 1.0f;
    m.texture = -1;
    m.drawCategory = -1;
    if (material) {
        const char *label = material->name ? material->name : "(unnamed)";
        snprintf(m.name, sizeof m.name, "%s", material->name ? material->name : "");
        if (material->has_pbr_metallic_roughness) {
            const cgltf_pbr_metallic_roughness *pbr = &material->pbr_metallic_roughness;
            for (int k = 0; k < 4; ++k) m.color[k] = unit_byte(pbr->base_color_factor[k]);
            if (pbr->base_color_texture.texture) {
                if (pbr->base_color_texture.texcoord != 0)
                    return refuse(c, "material %s samples its texture with TEXCOORD_%d; only TEXCOORD_0 is drawn", label,
                                  pbr->base_color_texture.texcoord);
                if (cook_texture(c, pbr->base_color_texture.texture, &m.texture)) return -1;
            }
        }
        if (material->unlit) m.flags |= P4_MODEL_MATERIAL_UNLIT;
        const char *json = material->extras.data;
        const char *v;
        if ((v = extras_field(json, "drawCategory")) != NULL) {
            long category = strtol(v, NULL, 10);
            if (category < 0 || category > 9) return refuse(c, "material %s: extras.p4.drawCategory %ld is outside 0-9", label, category);
            m.drawCategory = (int32_t)category;
        }
        if ((v = extras_field(json, "fogDisable")) != NULL && !strncmp(v, "true", 4)) m.flags |= P4_MODEL_MATERIAL_FOG_DISABLE;
        if ((v = extras_field(json, "tint")) != NULL && *v == '"') {
            m.tint = (uint32_t)strtoul(v + 1, NULL, 16);
            m.flags |= P4_MODEL_MATERIAL_TINT;
        }
        if ((v = extras_field(json, "surfaceProperties")) != NULL && *v == '[') {
            float s[3];
            if (json_floats(v, s, 3) == 3) {
                m.ambient = s[0];
                m.specular = s[1];
                m.diffuse = s[2];
            }
        }
    } else {
        snprintf(m.name, sizeof m.name, "default");
    }
    size_t n = c->material_count;
    P4ModelCookedMaterial *records = realloc(c->materials, (n + 1) * sizeof *records);
    const cgltf_material **keys = realloc(c->material_keys, (n + 1) * sizeof *keys);
    if (records) c->materials = records;
    if (keys) c->material_keys = keys;
    if (!records || !keys) return refuse(c, "out of memory");
    c->materials[n] = m;
    c->material_keys[n] = material;
    c->material_count = n + 1;
    *out = (uint32_t)n;
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Meshes */

#define SHARED_SETS 16

typedef struct MeshBuild {
    float *positions, *normals, *uv0, *uv1;
    uint8_t *colors;
    uint8_t *joints;    /* skinned: 4 joint slots per vertex */
    float *weights;     /* skinned: 4 weights per vertex */
    uint16_t *triangles;
    uint32_t *materials;
    uint32_t vertices, triangle_count, triangle_capacity, material_count, max_weights;
    int has_normals, has_uv0, has_uv1, has_colors, lit;
    /* Primitives that name the same attribute accessors (one vertex array split by material, as
     * rw_to_gltf writes and Blender exports) share one copy of the vertices. */
    const cgltf_accessor *shared[SHARED_SETS][7];
    uint32_t shared_base[SHARED_SETS];
    int shared_count;
} MeshBuild;

static void mesh_free(MeshBuild *m)
{
    free(m->positions);
    free(m->normals);
    free(m->uv0);
    free(m->uv1);
    free(m->colors);
    free(m->joints);
    free(m->weights);
    free(m->triangles);
    free(m->materials);
    memset(m, 0, sizeof *m);
}

static int grow(void **p, size_t count, size_t size)
{
    void *q = realloc(*p, count * size);
    if (!q && count) return -1;
    *p = q;
    return 0;
}

static int add_triangle(Cook *c, MeshBuild *m, const char *label, uint32_t a, uint32_t b, uint32_t d, uint32_t slot)
{
    if (a == b || b == d || a == d) return 0; /* degenerate: never drawn */
    if (m->triangle_count >= 0xFFFFFFu) return refuse(c, "mesh %s has too many triangles", label);
    if (m->triangle_count == m->triangle_capacity) {
        uint32_t next = m->triangle_capacity ? m->triangle_capacity * 2 : 256;
        if (grow((void **)&m->triangles, (size_t)next * 4, sizeof *m->triangles)) return refuse(c, "out of memory");
        m->triangle_capacity = next;
    }
    uint16_t *t = m->triangles + (size_t)m->triangle_count * 4;
    t[0] = (uint16_t)a;
    t[1] = (uint16_t)b;
    t[2] = (uint16_t)d;
    t[3] = (uint16_t)slot;
    m->triangle_count++;
    return 0;
}

/* One vertex's skin weights: the non-zero ones first in their glTF order, renormalised when they do not
 * already sum to 1 (within 1e-4, the precision retail weights keep through rw_to_gltf). */
static int skin_vertex(Cook *c, const char *label, const cgltf_skin *skin, const uint32_t joint[4], const float weight[4],
                       uint8_t slot[4], float out[4], uint32_t *max_weights)
{
    uint32_t n = 0;
    float sum = 0.0f;
    memset(slot, 0, 4);
    memset(out, 0, 4 * sizeof *out);
    for (int k = 0; k < 4; ++k) {
        if (!(weight[k] > 0.0f)) continue;
        if (joint[k] >= skin->joints_count)
            return refuse(c, "mesh %s weights joint %u; its skin has %u joints", label, (unsigned)joint[k],
                          (unsigned)skin->joints_count);
        slot[n] = (uint8_t)joint[k];
        out[n] = weight[k];
        sum += weight[k];
        ++n;
    }
    if (!n) return refuse(c, "mesh %s has a vertex without a joint weight", label);
    if (fabsf(sum - 1.0f) > 1e-4f)
        for (uint32_t k = 0; k < n; ++k) out[k] /= sum;
    if (n > *max_weights) *max_weights = n;
    return 0;
}

static int cook_primitive(Cook *c, MeshBuild *m, const char *label, const cgltf_primitive *prim, const cgltf_skin *skin,
                          const float matrix[16], const float normal_matrix[9])
{
    if (prim->type != cgltf_primitive_type_triangles && prim->type != cgltf_primitive_type_triangle_strip &&
        prim->type != cgltf_primitive_type_triangle_fan)
        return refuse(c, "mesh %s has a primitive of points or lines; only triangles are drawn", label);
    if (prim->targets_count) return refuse(c, "mesh %s has morph targets, which P4 models do not use", label);
    const cgltf_accessor *pos = NULL, *nrm = NULL, *uv0 = NULL, *uv1 = NULL, *col = NULL, *jnt = NULL, *wgt = NULL;
    for (cgltf_size i = 0; i < prim->attributes_count; ++i) {
        const cgltf_attribute *a = &prim->attributes[i];
        if (a->type == cgltf_attribute_type_position) pos = a->data;
        else if (a->type == cgltf_attribute_type_normal) nrm = a->data;
        else if (a->type == cgltf_attribute_type_texcoord && a->index == 0) uv0 = a->data;
        else if (a->type == cgltf_attribute_type_texcoord && a->index == 1) uv1 = a->data;
        else if (a->type == cgltf_attribute_type_color && a->index == 0) col = a->data;
        else if (a->type == cgltf_attribute_type_joints && a->index == 0) jnt = a->data;
        else if (a->type == cgltf_attribute_type_weights && a->index == 0) wgt = a->data;
        else if (a->type == cgltf_attribute_type_joints || a->type == cgltf_attribute_type_weights)
            return refuse(c, "mesh %s has more than 4 weights per vertex (JOINTS_1/WEIGHTS_1); P4 skins take 4", label);
    }
    if (!skin && (jnt || wgt)) return refuse(c, "mesh %s has JOINTS_0/WEIGHTS_0 but its node has no skin", label);
    if (skin && (!jnt || !wgt)) return refuse(c, "skinned mesh %s has a primitive without JOINTS_0 and WEIGHTS_0", label);
    if (!pos || pos->type != cgltf_type_vec3) return refuse(c, "mesh %s has a primitive without VEC3 POSITION", label);
    uint32_t count = (uint32_t)pos->count;
    uint32_t slot;
    {
        uint32_t global = 0;
        if (cook_material(c, prim->material, &global)) return -1;
        for (slot = 0; slot < m->material_count && m->materials[slot] != global; ++slot) {
        }
        if (slot == m->material_count) {
            if (grow((void **)&m->materials, m->material_count + 1, sizeof *m->materials)) return refuse(c, "out of memory");
            m->materials[m->material_count++] = global;
        }
        if (!(c->materials[global].flags & P4_MODEL_MATERIAL_UNLIT)) m->lit = 1;
    }
    const cgltf_accessor *set[7] = {pos, nrm, uv0, uv1, col, jnt, wgt};
    uint32_t base = m->vertices;
    int reused = 0;
    for (int s = 0; s < m->shared_count; ++s)
        if (!memcmp(m->shared[s], set, sizeof set)) {
            base = m->shared_base[s];
            reused = 1;
            break;
        }
    if (!reused) {
        if (m->vertices && (m->has_normals != (nrm != NULL)))
            return refuse(c, "mesh %s mixes primitives with and without NORMAL", label);
        uint64_t total = (uint64_t)base + count;
        if (total > c->budget->vertices)
            return refuse(c, "mesh %s has %llu vertices; at most %u per mesh", label, (unsigned long long)total,
                          (unsigned)c->budget->vertices);
        if (grow((void **)&m->positions, (size_t)total * 3, sizeof(float)) || grow((void **)&m->normals, (size_t)total * 3, sizeof(float)) ||
            grow((void **)&m->uv0, (size_t)total * 2, sizeof(float)) || grow((void **)&m->uv1, (size_t)total * 2, sizeof(float)) ||
            grow((void **)&m->colors, (size_t)total * 4, 1) ||
            (skin && (grow((void **)&m->joints, (size_t)total * 4, 1) || grow((void **)&m->weights, (size_t)total * 4, sizeof(float)))))
            return refuse(c, "out of memory");
        for (uint32_t v = 0; v < count; ++v) {
            float p[3] = {0, 0, 0}, n[3] = {0, 0, 1}, t0[2] = {0, 0}, t1[2] = {0, 0}, rgba[4] = {1, 1, 1, 1};
            cgltf_accessor_read_float(pos, v, p, 3);
            float *out = m->positions + (size_t)(base + v) * 3;
            for (int r = 0; r < 3; ++r) out[r] = matrix[r] * p[0] + matrix[4 + r] * p[1] + matrix[8 + r] * p[2] + matrix[12 + r];
            if (nrm) {
                cgltf_accessor_read_float(nrm, v, n, 3);
                float x = normal_matrix[0] * n[0] + normal_matrix[3] * n[1] + normal_matrix[6] * n[2];
                float y = normal_matrix[1] * n[0] + normal_matrix[4] * n[1] + normal_matrix[7] * n[2];
                float z = normal_matrix[2] * n[0] + normal_matrix[5] * n[1] + normal_matrix[8] * n[2];
                float len = sqrtf(x * x + y * y + z * z);
                if (len > 1e-12f) {
                    x /= len;
                    y /= len;
                    z /= len;
                }
                n[0] = x;
                n[1] = y;
                n[2] = z;
            }
            memcpy(m->normals + (size_t)(base + v) * 3, n, sizeof n);
            if (uv0) cgltf_accessor_read_float(uv0, v, t0, 2);
            if (uv1) cgltf_accessor_read_float(uv1, v, t1, 2);
            memcpy(m->uv0 + (size_t)(base + v) * 2, t0, sizeof t0);
            memcpy(m->uv1 + (size_t)(base + v) * 2, t1, sizeof t1);
            if (col) cgltf_accessor_read_float(col, v, rgba, col->type == cgltf_type_vec3 ? 3 : 4);
            uint8_t *cv = m->colors + (size_t)(base + v) * 4;
            for (int k = 0; k < 4; ++k) cv[k] = unit_byte(rgba[k]);
            if (skin) {
                uint32_t j[4] = {0, 0, 0, 0};
                float w[4] = {0, 0, 0, 0};
                cgltf_accessor_read_uint(jnt, v, j, 4);
                cgltf_accessor_read_float(wgt, v, w, 4);
                if (skin_vertex(c, label, skin, j, w, m->joints + (size_t)(base + v) * 4, m->weights + (size_t)(base + v) * 4,
                                &m->max_weights))
                    return -1;
            }
        }
        m->has_normals = nrm != NULL;
        m->has_uv0 |= uv0 != NULL;
        m->has_uv1 |= uv1 != NULL;
        m->has_colors |= col != NULL;
        m->vertices = (uint32_t)total;
        if (m->shared_count < SHARED_SETS) {
            memcpy(m->shared[m->shared_count], set, sizeof set);
            m->shared_base[m->shared_count++] = base;
        }
    }
    /* Triangles, in the primitive's order and winding (glTF and P4 both treat counter-clockwise as front). */
    size_t indices = prim->indices ? prim->indices->count : count;
    uint32_t prev[2] = {0, 0};
    uint32_t first = 0;
    for (size_t i = 0; i < indices; ++i) {
        size_t raw = prim->indices ? cgltf_accessor_read_index(prim->indices, i) : i;
        if (raw >= count) return refuse(c, "mesh %s has an index past its vertices", label);
        uint32_t index = base + (uint32_t)raw;
        if (prim->type == cgltf_primitive_type_triangles) {
            if (i % 3 == 2 && add_triangle(c, m, label, prev[0], prev[1], index, slot)) return -1;
            if (i % 3 == 0) prev[0] = index;
            else if (i % 3 == 1) prev[1] = index;
        } else if (prim->type == cgltf_primitive_type_triangle_strip) {
            if (i >= 2) {
                int odd = (int)(i & 1);
                if (add_triangle(c, m, label, odd ? prev[1] : prev[0], odd ? prev[0] : prev[1], index, slot)) return -1;
            }
            prev[0] = prev[1];
            prev[1] = index;
        } else {
            if (i == 0) first = index;
            else if (i >= 2 && add_triangle(c, m, label, first, prev[1], index, slot)) return -1;
            prev[1] = index;
        }
    }
    if (prim->type == cgltf_primitive_type_triangles && indices % 3)
        return refuse(c, "mesh %s has a triangle list whose index count is not a multiple of 3", label);
    return 0;
}

/* extras.p4.matrixPads of a skin (rw_to_gltf): per joint, the four words of the RwMatrix's fourth column
 * (flags and pads) as "0x" hex strings. Applied only when there is exactly one quadruple per joint. */
static void matrix_pads(const char *p, uint32_t joints, uint32_t *words)
{
    uint32_t pads[256 * 4];
    uint32_t n = 0;
    int depth = 0;
    if (!p || *p != '[') return;
    for (; *p; ++p) {
        if (*p == '[') {
            ++depth;
        } else if (*p == ']') {
            if (--depth == 0) break;
        } else if (*p == '"' && depth == 2) {
            if (n == joints * 4) return;
            pads[n++] = (uint32_t)strtoul(p + 1, NULL, 16);
            p = strchr(p + 1, '"');
            if (!p) return;
        }
    }
    if (n != joints * 4) return;
    for (uint32_t j = 0; j < joints; ++j)
        for (uint32_t k = 0; k < 4; ++k) words[16 * j + 4 * k + 3] = pads[4 * j + k];
}

/* A skinned mesh's P4ModelCookedSkin: the HAnim node id of each joint (a node named bone_<id>), its
 * inverse bind matrix as an RwMatrix, and the mesh's per-vertex slots and weights. */
static int cook_skin(Cook *c, Blob *blob, const char *label, const cgltf_skin *skin, const MeshBuild *m, const char *mesh_extras,
                     uint32_t *offset)
{
    const uint32_t joints = (uint32_t)skin->joints_count;
    if (!joints || joints > 256) return refuse(c, "the skin of mesh %s has %u joints; P4 skins take 1 to 256", label, (unsigned)joints);
    int32_t ids[256];
    uint8_t used[256];
    memset(used, 0, sizeof used);
    for (size_t v = 0; v < (size_t)m->vertices * 4; ++v)
        if (m->weights[v] > 0.0f) used[m->joints[v]] = 1;
    uint32_t *words = malloc((size_t)joints * 64);
    if (!words) return refuse(c, "out of memory");
    for (uint32_t j = 0; j < joints; ++j) {
        const cgltf_node *joint = skin->joints[j];
        ids[j] = -1;
        if (!joint || !numbered(joint->name, "bone_", &ids[j])) {
            ids[j] = -1;
            if (used[j]) {
                free(words);
                return refuse(c, "the skin of mesh %s weights joint %s, which is not an HAnim bone (a node named bone_<id>)",
                              label, joint && joint->name ? joint->name : "(unnamed)");
            }
        }
        for (uint32_t k = 0; k < j; ++k)
            if (ids[j] >= 0 && ids[k] == ids[j]) {
                free(words);
                return refuse(c, "the skin of mesh %s names bone_%d twice", label, (int)ids[j]);
            }
        float ibm[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        if (skin->inverse_bind_matrices && !cgltf_accessor_read_float(skin->inverse_bind_matrices, j, ibm, 16)) {
            free(words);
            return refuse(c, "the skin of mesh %s has an unreadable inverse bind matrix %u", label, (unsigned)j);
        }
        memcpy(words + 16 * j, ibm, sizeof ibm);
        words[16 * j + 3] = words[16 * j + 7] = words[16 * j + 11] = words[16 * j + 15] = 0;
    }
    matrix_pads(extras_field(skin->extras.data, "matrixPads"), joints, words);
    P4ModelCookedSkin s;
    memset(&s, 0, sizeof s);
    s.joints = joints;
    s.maxWeights = m->max_weights;
    s.jointIdsOffset = blob_put(blob, ids, (size_t)joints * 4);
    s.matricesOffset = blob_put(blob, words, (size_t)joints * 64);
    s.indicesOffset = blob_put(blob, m->joints, (size_t)m->vertices * 4);
    s.weightsOffset = blob_put(blob, m->weights, (size_t)m->vertices * 16);
    /* rw_to_gltf keeps a retail skin's used-bone list (extras.p4.skin.usedBones: hierarchy node indices, which
     * are its joint slots); the stream writer keeps that order when it still lists exactly the used bones. */
    const char *order = extras_field(mesh_extras, "usedBones");
    if (order && *order == '[') {
        uint8_t slots[256];
        uint32_t n = 0;
        const char *p = order + 1;
        while (n < 256) {
            char *end;
            long v = strtol(p, &end, 10);
            if (end == p || v < 0 || v >= (long)joints) break;
            slots[n++] = (uint8_t)v;
            while (*end == ' ' || *end == ',' || *end == '\n' || *end == '\r' || *end == '\t') ++end;
            p = end;
            if (*p == ']') break;
        }
        if (n && *p == ']') {
            s.orderCount = n;
            s.orderOffset = blob_put(blob, slots, n);
        }
    }
    free(words);
    *offset = blob_put(blob, &s, sizeof s);
    return 0;
}

/* A copy (malloc'd, NUL-terminated) of an object's extras JSON, or NULL: cgltf keeps some objects' extras as
 * a span of the document only. */
static char *extras_json(Cook *c, const cgltf_extras *extras)
{
    char *json = NULL;
    cgltf_size json_size = 0;
    if (extras->end_offset > extras->start_offset && cgltf_copy_extras_json(c->data, extras, NULL, &json_size) == cgltf_result_success &&
        (json = malloc(json_size)) != NULL && cgltf_copy_extras_json(c->data, extras, json, &json_size) != cgltf_result_success) {
        free(json);
        json = NULL;
    }
    return json;
}

static int cook_meshes(Cook *c, Blob *blob, P4ModelCookedMesh **records, uint32_t *record_count)
{
    *records = NULL;
    *record_count = 0;
    for (cgltf_size ni = 0; ni < c->data->nodes_count; ++ni) {
        const cgltf_node *node = &c->data->nodes[ni];
        if (!node->mesh) continue;
        const char *label = node->mesh->name ? node->mesh->name : (node->name ? node->name : "(unnamed)");
        if (*record_count >= c->budget->meshes)
            return refuse(c, "more than %u meshes (the %s budget)", (unsigned)c->budget->meshes, c->budget->category);
        int32_t kind = P4_MODEL_FRAME_SKIN, id = 0;
        /* A skinned mesh is in skin space (glTF ignores its node's transform); a static mesh in its anchor's
         * space: the local transforms from below the anchor down to the mesh node, composed in order
         * (identity, exactly, for a mesh on its frame's own node), or the world transform when there is no
         * anchor. */
        float matrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        float normal3[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        if (!node->skin) {
            const cgltf_node *anchor = anchor_of(node, &kind, &id);
            for (const cgltf_node *n = node; n && n != anchor; n = n->parent) {
                float local[16];
                cgltf_node_transform_local(n, local);
                mat_mul(local, matrix, matrix);
            }
            float inv[16];
            if (mat_invert_affine(matrix, inv)) return refuse(c, "mesh %s has a singular transform", label);
            /* normal matrix = inverse transpose of the 3x3 (column-major 3x3 out) */
            for (int col = 0; col < 3; ++col)
                for (int row = 0; row < 3; ++row) normal3[col * 3 + row] = inv[row * 4 + col];
        }
        MeshBuild m;
        memset(&m, 0, sizeof m);
        for (cgltf_size pi = 0; pi < node->mesh->primitives_count; ++pi)
            if (cook_primitive(c, &m, label, &node->mesh->primitives[pi], node->skin, matrix, normal3)) {
                mesh_free(&m);
                return -1;
            }
        if (!m.triangle_count) {
            mesh_free(&m);
            continue;
        }
        if ((uint64_t)c->triangles + m.triangle_count > c->budget->triangles) {
            unsigned long long have = (unsigned long long)c->triangles + m.triangle_count;
            mesh_free(&m);
            return refuse(c, "%llu triangles at mesh %s; at most %u for a %s", have, label, (unsigned)c->budget->triangles,
                          c->budget->category);
        }
        c->triangles += m.triangle_count;
        P4ModelCookedMesh r;
        memset(&r, 0, sizeof r);
        r.frameKind = kind;
        r.frameId = id;
        r.vertices = m.vertices;
        r.triangles = m.triangle_count;
        r.uvSets = m.has_uv1 ? 2u : m.has_uv0 ? 1u : 0u;
        int modulate = 0;
        for (uint32_t s = 0; s < m.material_count; ++s) {
            const uint8_t *col = c->materials[m.materials[s]].color;
            if (col[0] != 255 || col[1] != 255 || col[2] != 255 || col[3] != 255) modulate = 1;
        }
        int lit = m.lit && m.has_normals;
        /* No rpGEOMETRYTRISTRIP: RpGeometryUnlock then builds a triangle-list bin mesh instead of running the
         * tri-stripper at load (retail files store strips; the port draws the triangle array either way). */
        r.format = P4_MODEL_GEO_POSITIONS | (m.has_normals ? P4_MODEL_GEO_NORMALS : 0) |
                   (lit ? P4_MODEL_GEO_LIGHT : 0) | (m.has_colors ? P4_MODEL_GEO_PRELIT : 0) |
                   (r.uvSets ? P4_MODEL_GEO_TEXTURED : 0) | (r.uvSets == 2 ? P4_MODEL_GEO_TEXTURED2 : 0) |
                   (modulate ? P4_MODEL_GEO_MODULATE : 0) | (r.uvSets << 16);
        r.positionsOffset = blob_put(blob, m.positions, (size_t)m.vertices * 12);
        if (m.has_normals) r.normalsOffset = blob_put(blob, m.normals, (size_t)m.vertices * 12);
        if (r.uvSets >= 1) r.uv0Offset = blob_put(blob, m.uv0, (size_t)m.vertices * 8);
        if (r.uvSets == 2) r.uv1Offset = blob_put(blob, m.uv1, (size_t)m.vertices * 8);
        if (m.has_colors) r.colorsOffset = blob_put(blob, m.colors, (size_t)m.vertices * 4);
        r.trianglesOffset = blob_put(blob, m.triangles, (size_t)m.triangle_count * 8);
        r.materialCount = m.material_count;
        r.materialsOffset = blob_put(blob, m.materials, (size_t)m.material_count * 4);
        if (node->skin) {
            char *json = extras_json(c, &node->mesh->extras);
            int failed = cook_skin(c, blob, label, node->skin, &m, json, &r.skinOffset);
            free(json);
            if (failed) {
                mesh_free(&m);
                return -1;
            }
        }
        mesh_free(&m);
        P4ModelCookedMesh *grown = realloc(*records, (*record_count + 1) * sizeof *grown);
        if (!grown) return refuse(c, "out of memory");
        *records = grown;
        (*records)[(*record_count)++] = r;
    }
    if (!*record_count) return refuse(c, "has no triangle meshes");
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * Animations. A glTF animation named L<layer>S<slot> (rw_to_gltf's name for a slot's animation) replaces
 * that slot's animation. Each HAnim bone (a node named bone_<id>) gets a track: its rotation and translation
 * channels sampled at the union of their key times, or its rest pose when the animation leaves it alone.
 * Values are packed as RtCompressedKeyFrames, the scheme retail animations use. */

#define MAX_ANIMATION_KEYS (1u << 20) /* keys over every track of one model */
#define HALF_SMALLEST 3.0517578125e-05f /* 2^-15: smaller magnitudes pack as zero */
#define HALF_LARGEST 1.99951171875f   /* 2^0 * (1 + 2047/2048) */

/* RtCompressedKeyFrame's 16-bit float: sign, a 4-bit exponent for 2^-15..2^0, an 11-bit mantissa
 * (rounded to nearest; magnitudes outside the range clamp). */
static uint16_t pack_half(float x)
{
    uint32_t bits;
    memcpy(&bits, &x, sizeof bits);
    const uint32_t sign = (bits >> 16) & 0x8000u;
    float a = fabsf(x);
    if (!(a >= HALF_SMALLEST)) return (uint16_t)sign;
    if (a > HALF_LARGEST) a = HALF_LARGEST;
    memcpy(&bits, &a, sizeof bits);
    uint32_t exponent = (bits >> 23) - 112u, mantissa = ((bits & 0x7FFFFFu) + 0x800u) >> 12;
    if (mantissa == 0x800u) {
        mantissa = 0;
        ++exponent;
    }
    if (exponent > 15u) {
        exponent = 15u;
        mantissa = 0x7FFu;
    }
    return (uint16_t)(sign | exponent << 11 | mantissa);
}

static int slot_name(const char *name, uint32_t *layer, uint32_t *slot)
{
    if (!name || name[0] != 'L' || name[1] < '0' || name[1] > '9') return 0;
    char *end;
    unsigned long l = strtoul(name + 1, &end, 10);
    if (*end != 'S' || end[1] < '0' || end[1] > '9') return 0;
    unsigned long s = strtoul(end + 1, &end, 10);
    if (*end || l > 0xFFFFu || s > 0xFFFFu) return 0;
    *layer = (uint32_t)l;
    *slot = (uint32_t)s;
    return 1;
}

static float key_time(const cgltf_accessor *input, cgltf_size i)
{
    float t = 0.0f;
    cgltf_accessor_read_float(input, i, &t, 1);
    return t;
}

static void normalize4(float q[4])
{
    float n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (n > 0.0f)
        for (int k = 0; k < 4; ++k) q[k] /= n;
    else
        q[0] = q[1] = q[2] = 0.0f, q[3] = 1.0f;
}

/* A channel's value at time t: its first or last key outside its keys, the earlier key for STEP, else
 * linear (spherical for rotations). */
static void sample(const cgltf_animation_sampler *s, float t, int rotation, float out[4])
{
    const int comps = rotation ? 4 : 3;
    cgltf_size lo = 0, hi = s->input->count - 1;
    if (t <= key_time(s->input, 0) || hi == 0) {
        cgltf_accessor_read_float(s->output, 0, out, comps);
    } else if (t >= key_time(s->input, hi)) {
        cgltf_accessor_read_float(s->output, hi, out, comps);
    } else {
        while (hi - lo > 1) {
            cgltf_size mid = lo + (hi - lo) / 2;
            if (key_time(s->input, mid) <= t) lo = mid;
            else hi = mid;
        }
        float a[4] = {0, 0, 0, 1}, b[4] = {0, 0, 0, 1};
        cgltf_accessor_read_float(s->output, lo, a, comps);
        cgltf_accessor_read_float(s->output, hi, b, comps);
        const float t0 = key_time(s->input, lo), t1 = key_time(s->input, hi);
        const float u = s->interpolation == cgltf_interpolation_type_step || !(t1 > t0) ? 0.0f : (t - t0) / (t1 - t0);
        float wa = 1.0f - u, wb = u;
        if (rotation) {
            float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
            if (d < 0.0f) {
                d = -d;
                wb = -wb;
            }
            if (d < 0.9995f) {
                const float theta = acosf(d), sine = sinf(theta);
                wa = sinf((1.0f - u) * theta) / sine;
                wb = (wb < 0.0f ? -1.0f : 1.0f) * sinf(u * theta) / sine;
            }
        }
        for (int k = 0; k < comps; ++k) out[k] = wa * a[k] + wb * b[k];
    }
    if (rotation) normalize4(out);
}

/* A node's local rotation and translation (its matrix's, scale removed, when it has one). */
static void rest_pose(const cgltf_node *node, float rotation[4], float translation[3])
{
    if (!node->has_matrix) {
        const float identity[4] = {0, 0, 0, 1}, zero[3] = {0, 0, 0};
        memcpy(rotation, node->has_rotation ? node->rotation : identity, 4 * sizeof(float));
        memcpy(translation, node->has_translation ? node->translation : zero, 3 * sizeof(float));
        normalize4(rotation);
        return;
    }
    const float *m = node->matrix;
    float axis[3][3];
    for (int col = 0; col < 3; ++col) {
        float n = sqrtf(m[col * 4] * m[col * 4] + m[col * 4 + 1] * m[col * 4 + 1] + m[col * 4 + 2] * m[col * 4 + 2]);
        for (int row = 0; row < 3; ++row) axis[col][row] = n > 0.0f ? m[col * 4 + row] / n : (float)(row == col);
    }
    /* r(row, col) = axis[col][row] */
    const float r00 = axis[0][0], r11 = axis[1][1], r22 = axis[2][2], trace = r00 + r11 + r22;
    float *q = rotation;
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q[3] = 0.25f * s, q[0] = (axis[1][2] - axis[2][1]) / s, q[1] = (axis[2][0] - axis[0][2]) / s, q[2] = (axis[0][1] - axis[1][0]) / s;
    } else if (r00 > r11 && r00 > r22) {
        float s = sqrtf(1.0f + r00 - r11 - r22) * 2.0f;
        q[3] = (axis[1][2] - axis[2][1]) / s, q[0] = 0.25f * s, q[1] = (axis[1][0] + axis[0][1]) / s, q[2] = (axis[2][0] + axis[0][2]) / s;
    } else if (r11 > r22) {
        float s = sqrtf(1.0f + r11 - r00 - r22) * 2.0f;
        q[3] = (axis[2][0] - axis[0][2]) / s, q[0] = (axis[1][0] + axis[0][1]) / s, q[1] = 0.25f * s, q[2] = (axis[2][1] + axis[1][2]) / s;
    } else {
        float s = sqrtf(1.0f + r22 - r00 - r11) * 2.0f;
        q[3] = (axis[0][1] - axis[1][0]) / s, q[0] = (axis[2][0] + axis[0][2]) / s, q[1] = (axis[2][1] + axis[1][2]) / s, q[2] = 0.25f * s;
    }
    normalize4(q);
    translation[0] = m[12], translation[1] = m[13], translation[2] = m[14];
}

/* Up to `count` numbers of the JSON array at `p` ("[1, 2.5, ...]"). Returns how many were read. */
static int json_floats(const char *p, float *out, int count)
{
    if (!p || *p != '[') return 0;
    int n = 0;
    fenv_t saved;
    fegetenv(&saved);
    fesetround(FE_TONEAREST);
    ++p;
    while (n < count) {
        char *end;
        volatile double v = strtod(p, &end);
        if (end == p) break;
        out[n++] = (float)v;
        while (*end == ' ' || *end == ',' || *end == '\n' || *end == '\r' || *end == '\t') ++end;
        p = end;
    }
    fesetenv(&saved);
    return n;
}

typedef struct Bone {
    int32_t id;
    const cgltf_node *node;
    const cgltf_animation_sampler *rotation, *translation;
    const cgltf_accessor *packed_rotation, *rotation_snapshot, *time_snapshot;
} Bone;

typedef struct Key {
    float time, rotation[4], translation[3];
    uint16_t packed_rotation[4];
    int has_packed_rotation;
} Key;

/* The exporter's optional snapshot preserves original packed rotations only for untouched keys. */
static const cgltf_accessor *snapshot_accessor(Cook *c, const char *json, const char *name,
                                               cgltf_component_type component, cgltf_type type, cgltf_size count)
{
    const char *value = extras_field(json, name);
    if (!value || *value < '0' || *value > '9') return NULL;
    char *end;
    unsigned long index = strtoul(value, &end, 10);
    if (end == value || index >= c->data->accessors_count) return NULL;
    const cgltf_accessor *a = &c->data->accessors[index];
    return a->component_type == component && a->type == type && a->count == count && !a->normalized ? a : NULL;
}

static int original_rotation(const Bone *bone, float time, uint16_t packed[4])
{
    if (!bone->rotation || !bone->packed_rotation || !bone->rotation_snapshot || !bone->time_snapshot) return 0;
    const cgltf_animation_sampler *s = bone->rotation;
    cgltf_size lo = 0, hi = s->input->count;
    while (lo < hi) {
        cgltf_size mid = lo + (hi - lo) / 2;
        if (key_time(s->input, mid) < time) lo = mid + 1;
        else hi = mid;
    }
    if (lo == s->input->count || key_time(s->input, lo) != time || key_time(bone->time_snapshot, lo) != time) return 0;
    float value[4], snapshot[4];
    cgltf_uint raw[4];
    if (!cgltf_accessor_read_float(s->output, lo, value, 4) ||
        !cgltf_accessor_read_float(bone->rotation_snapshot, lo, snapshot, 4) ||
        memcmp(value, snapshot, sizeof value) ||
        !cgltf_accessor_read_uint(bone->packed_rotation, lo, raw, 4)) return 0;
    for (int i = 0; i < 4; ++i) packed[i] = (uint16_t)raw[i];
    return 1;
}

/* The sorted, de-duplicated union of a bone's channel times with 0 and `duration`, clipped to them. */
static float *track_times(const Bone *bone, float duration, size_t *count)
{
    const cgltf_size a = bone->rotation ? bone->rotation->input->count : 0;
    const cgltf_size b = bone->translation ? bone->translation->input->count : 0;
    float *times = malloc((a + b + 2) * sizeof *times);
    if (!times) return NULL;
    size_t n = 0;
    times[n++] = 0.0f;
    times[n++] = duration;
    for (cgltf_size i = 0; i < a; ++i) times[n++] = key_time(bone->rotation->input, i);
    for (cgltf_size i = 0; i < b; ++i) times[n++] = key_time(bone->translation->input, i);
    /* insertion sort: channel inputs are already ascending */
    for (size_t i = 1; i < n; ++i) {
        float t = times[i];
        size_t j = i;
        for (; j > 0 && times[j - 1] > t; --j) times[j] = times[j - 1];
        times[j] = t;
    }
    /* Times within 1e-6 of the previous kept one merge into the later (the first stays 0, the last is then
     * exactly `duration`, the largest). */
    size_t kept = 0;
    for (size_t i = 0; i < n; ++i) {
        float t = times[i] < 0.0f ? 0.0f : times[i] > duration ? duration : times[i];
        if (kept && t - times[kept - 1] <= 1e-6f) {
            if (kept > 1) times[kept - 1] = t;
            continue;
        }
        times[kept++] = t;
    }
    /* a still pose (duration 0) keeps two keys, as RtAnim needs */
    if (kept == 1) times[kept++] = duration;
    *count = kept;
    return times;
}

static int cook_animation(Cook *c, Blob *blob, const cgltf_animation *anim, Bone *bones, size_t bone_count, uint32_t layer,
                          uint32_t slot, uint64_t *total_keys, P4ModelCookedAnimation *record)
{
    const char *label = anim->name;
    float duration = 0.0f;
    for (size_t b = 0; b < bone_count; ++b) {
        bones[b].rotation = bones[b].translation = NULL;
        bones[b].packed_rotation = bones[b].rotation_snapshot = bones[b].time_snapshot = NULL;
    }
    for (cgltf_size i = 0; i < anim->channels_count; ++i) {
        const cgltf_animation_channel *ch = &anim->channels[i];
        const cgltf_animation_sampler *s = ch->sampler;
        if (!s || !s->input || !s->output || !s->input->count) return refuse(c, "animation %s has an empty channel", label);
        if (s->interpolation == cgltf_interpolation_type_cubic_spline)
            return refuse(c, "animation %s uses CUBICSPLINE interpolation; export it with Linear or Constant", label);
        if (s->output->count != s->input->count) return refuse(c, "animation %s has a channel whose keys and values differ in count", label);
        float last = -1.0f;
        for (cgltf_size k = 0; k < s->input->count; ++k) {
            float t = key_time(s->input, k);
            if (!(t >= last) || !(t < 1e6f)) return refuse(c, "animation %s has key times that are negative, unordered or not finite", label);
            last = t;
        }
        if (last > duration) duration = last;
        Bone *bone = NULL;
        for (size_t b = 0; b < bone_count && !bone; ++b)
            if (bones[b].node == ch->target_node) bone = &bones[b];
        if (!bone) continue; /* only HAnim bones animate */
        if (ch->target_path == cgltf_animation_path_type_scale) {
            for (cgltf_size k = 0; k < s->output->count; ++k) {
                float v[3] = {1, 1, 1};
                cgltf_accessor_read_float(s->output, k, v, 3);
                if (fabsf(v[0] - 1.0f) > 1e-3f || fabsf(v[1] - 1.0f) > 1e-3f || fabsf(v[2] - 1.0f) > 1e-3f)
                    return refuse(c, "animation %s scales bone_%d; P4 animations carry rotation and translation only", label,
                                  (int)bone->id);
            }
            continue;
        }
        const cgltf_animation_sampler **into = ch->target_path == cgltf_animation_path_type_rotation      ? &bone->rotation
                                               : ch->target_path == cgltf_animation_path_type_translation ? &bone->translation
                                                                                                          : NULL;
        if (!into) continue;
        if (*into) return refuse(c, "animation %s has two %s channels for bone_%d", label, into == &bone->rotation ? "rotation" : "translation", (int)bone->id);
        *into = s;
        if (into == &bone->rotation) {
            char *metadata = extras_json(c, &s->extras);
            bone->packed_rotation = snapshot_accessor(c, metadata, "packedRotation", cgltf_component_type_r_16u, cgltf_type_vec4, s->input->count);
            bone->rotation_snapshot = snapshot_accessor(c, metadata, "rotationSnapshot", cgltf_component_type_r_32f, cgltf_type_vec4, s->input->count);
            bone->time_snapshot = snapshot_accessor(c, metadata, "timeSnapshot", cgltf_component_type_r_32f, cgltf_type_scalar, s->input->count);
            free(metadata);
        }
    }
    char *json = extras_json(c, &anim->extras);
    const char *field = extras_field(json, "duration");
    if (field) {
        fenv_t saved;
        fegetenv(&saved);
        fesetround(FE_TONEAREST);
        float stored = strtof(field, NULL);
        fesetenv(&saved);
        if (stored > duration && stored < 1e6f) duration = stored;
    }
    field = extras_field(json, "flags");
    record->flags = field ? (uint32_t)strtoul(field, NULL, 10) : 0u;
    float stored_offset[3], stored_scale[3];
    int stored_range = json_floats(extras_field(json, "translationOffset"), stored_offset, 3) == 3 &&
                       json_floats(extras_field(json, "translationScale"), stored_scale, 3) == 3;
    free(json);
    record->layer = layer;
    record->slot = slot;
    record->duration = duration;
    record->tracks = (uint32_t)bone_count;

    Key **keys = calloc(bone_count, sizeof *keys);
    size_t *counts = calloc(bone_count, sizeof *counts);
    P4ModelCookedTrack *tracks = calloc(bone_count, sizeof *tracks);
    P4ModelCookedKey *packed = NULL;
    int failed = !keys || !counts || !tracks ? refuse(c, "out of memory") : 0;
    float low[3] = {0, 0, 0}, high[3] = {0, 0, 0};
    for (size_t b = 0; b < bone_count && !failed; ++b) {
        size_t n = 0;
        float *times = track_times(&bones[b], duration, &n);
        if (!times || !(keys[b] = malloc(n * sizeof **keys))) {
            free(times);
            failed = refuse(c, "out of memory");
            break;
        }
        if ((*total_keys += n) > MAX_ANIMATION_KEYS) {
            free(times);
            failed = refuse(c, "more than %u animation keys", MAX_ANIMATION_KEYS);
            break;
        }
        float rest_rotation[4], rest_translation[3];
        rest_pose(bones[b].node, rest_rotation, rest_translation);
        for (size_t k = 0; k < n; ++k) {
            Key *key = &keys[b][k];
            key->time = times[k];
            key->has_packed_rotation = original_rotation(&bones[b], times[k], key->packed_rotation);
            if (bones[b].rotation) sample(bones[b].rotation, times[k], 1, key->rotation);
            else memcpy(key->rotation, rest_rotation, sizeof rest_rotation);
            float t[4];
            if (bones[b].translation) sample(bones[b].translation, times[k], 0, t);
            else memcpy(t, rest_translation, sizeof rest_translation);
            for (int a = 0; a < 3; ++a) {
                key->translation[a] = t[a];
                if ((b == 0 && k == 0) || t[a] < low[a]) low[a] = t[a];
                if ((b == 0 && k == 0) || t[a] > high[a]) high[a] = t[a];
            }
        }
        counts[b] = n;
        free(times);
    }
    /* Translations pack as (value - offset) / scale: rw_to_gltf's stored range when it still covers them
     * (a zero scale when every value is the offset, as retail stores still translations), else the keys'
     * own range mapped onto -1..1. */
    for (int a = 0; a < 3 && stored_range; ++a) {
        if (stored_scale[a] == 0.0f ? low[a] != stored_offset[a] || high[a] != stored_offset[a]
                                    : !(fabsf((low[a] - stored_offset[a]) / stored_scale[a]) <= HALF_LARGEST) ||
                                          !(fabsf((high[a] - stored_offset[a]) / stored_scale[a]) <= HALF_LARGEST))
            stored_range = 0;
    }
    for (int a = 0; a < 3; ++a) {
        record->translationOffset[a] = stored_range ? stored_offset[a] : (low[a] + high[a]) * 0.5f;
        float half = (high[a] - low[a]) * 0.5f;
        record->translationScale[a] = stored_range ? stored_scale[a] : half > 1e-6f ? half : 1.0f;
    }
    for (size_t b = 0; b < bone_count && !failed; ++b) {
        P4ModelCookedKey *grown = realloc(packed, counts[b] * sizeof *packed);
        if (!grown) {
            failed = refuse(c, "out of memory");
            break;
        }
        packed = grown;
        for (size_t k = 0; k < counts[b]; ++k) {
            const Key *key = &keys[b][k];
            P4ModelCookedKey *out = &packed[k];
            memset(out, 0, sizeof *out);
            out->time = key->time;
            for (int a = 0; a < 4; ++a)
                out->rotation[a] = key->has_packed_rotation ? key->packed_rotation[a] : pack_half(key->rotation[a]);
            for (int a = 0; a < 3; ++a)
                out->translation[a] = record->translationScale[a] == 0.0f
                                          ? 0
                                          : pack_half((key->translation[a] - record->translationOffset[a]) / record->translationScale[a]);
        }
        tracks[b].nodeId = bones[b].id;
        tracks[b].keys = (uint32_t)counts[b];
        tracks[b].keysOffset = blob_put(blob, packed, counts[b] * sizeof *packed);
    }
    if (!failed) record->tracksOffset = blob_put(blob, tracks, bone_count * sizeof *tracks);
    for (size_t b = 0; keys && b < bone_count; ++b) free(keys[b]);
    free(keys);
    free(counts);
    free(tracks);
    free(packed);
    return failed;
}

static int cook_animations(Cook *c, Blob *blob, uint32_t *count, uint32_t *offset)
{
    *count = 0;
    *offset = 0;
    const cgltf_data *d = c->data;
    uint32_t layer, slot, wanted = 0;
    for (cgltf_size i = 0; i < d->animations_count; ++i) wanted += (uint32_t)slot_name(d->animations[i].name, &layer, &slot);
    if (!wanted) return 0;
    Bone *bones = calloc(d->nodes_count ? d->nodes_count : 1, sizeof *bones);
    P4ModelCookedAnimation *records = calloc(wanted, sizeof *records);
    if (!bones || !records) {
        free(bones);
        free(records);
        return refuse(c, "out of memory");
    }
    size_t bone_count = 0;
    int failed = 0;
    for (cgltf_size i = 0; i < d->nodes_count && !failed; ++i) {
        int32_t id;
        if (!numbered(d->nodes[i].name, "bone_", &id)) continue;
        for (size_t b = 0; b < bone_count; ++b)
            if (bones[b].id == id) failed = refuse(c, "two nodes are HAnim bone_%d; an animated model names each bone once", (int)id);
        bones[bone_count].id = id;
        bones[bone_count++].node = &d->nodes[i];
    }
    if (!failed && !bone_count) failed = refuse(c, "has slot animations but no HAnim bones (nodes named bone_<id>)");
    uint64_t total_keys = 0;
    for (cgltf_size i = 0; i < d->animations_count && !failed; ++i) {
        if (!slot_name(d->animations[i].name, &layer, &slot)) continue;
        for (uint32_t k = 0; k < *count; ++k)
            if (records[k].layer == layer && records[k].slot == slot)
                failed = refuse(c, "two animations replace layer %u slot %u (%s)", (unsigned)layer, (unsigned)slot, d->animations[i].name);
        if (!failed) failed = cook_animation(c, blob, &d->animations[i], bones, bone_count, layer, slot, &total_keys, &records[*count]);
        if (!failed) ++*count;
    }
    if (!failed && *count) *offset = blob_put(blob, records, *count * sizeof *records);
    free(bones);
    free(records);
    return failed;
}

/* ------------------------------------------------------------------------------------------------ */

static void cook_free(Cook *c)
{
    for (size_t i = 0; i < c->texture_count; ++i) free(c->texture_data[i]);
    free(c->texture_data);
    free(c->texture_data_size);
    free(c->textures);
    free(c->texture_keys);
    free(c->materials);
    free(c->material_keys);
    cgltf_free(c->data);
}

int p4_model_cook(const char *name, const uint8_t *gltf, size_t size, P4ModelReadFn read, void *user,
                  const P4ModelBudget *budget, uint8_t **out, size_t *out_size, char *error, size_t error_size)
{
    Cook c;
    memset(&c, 0, sizeof c);
    c.name = name;
    c.budget = budget ? budget : &budgets[2];
    c.error = error;
    c.error_size = error_size;
    c.read = read;
    c.user = user;
    if (error && error_size) error[0] = 0;
    *out = NULL;
    *out_size = 0;
    if (size > c.budget->sourceBytes)
        return refuse(&c, "%zu bytes; at most %u MiB for a %s", size, (unsigned)(c.budget->sourceBytes >> 20), c.budget->category);
    if (p4_model_source_hash(name, gltf, size, read, user, c.source, error, error_size)) return -1;
    if (parse(&c, gltf, size)) {
        cook_free(&c);
        return -1;
    }
    Blob blob;
    memset(&blob, 0, sizeof blob);
    blob_reserve(&blob, sizeof(P4ModelCookedHeader), 4);
    P4ModelCookedMesh *meshes = NULL;
    uint32_t mesh_count = 0;
    uint32_t animations = 0, animations_offset = 0;
    if (cook_meshes(&c, &blob, &meshes, &mesh_count) || cook_animations(&c, &blob, &animations, &animations_offset)) {
        free(meshes);
        free(blob.data);
        cook_free(&c);
        return -1;
    }
    for (size_t i = 0; i < c.texture_count; ++i) {
        P4ModelCookedTexture *t = &c.textures[i];
        const uint8_t *data = c.texture_data[i];
        if (t->depth == 8) {
            t->paletteOffset = blob_put(&blob, data, 1024);
            t->pixelsOffset = blob_put(&blob, data + 1024, c.texture_data_size[i] - 1024);
            t->pixelsBytes = (uint32_t)(c.texture_data_size[i] - 1024);
        } else {
            t->pixelsOffset = blob_put(&blob, data, c.texture_data_size[i]);
            t->pixelsBytes = (uint32_t)c.texture_data_size[i];
        }
    }
    P4ModelCookedHeader header;
    memset(&header, 0, sizeof header);
    header.magic = P4_MODEL_COOKED_MAGIC;
    header.version = P4_MODEL_COOKED_VERSION;
    header.textures = (uint32_t)c.texture_count;
    header.materials = (uint32_t)c.material_count;
    header.meshes = mesh_count;
    header.triangles = c.triangles;
    header.textureBytes = c.texture_bytes;
    header.texturesOffset = blob_put(&blob, c.textures, c.texture_count * sizeof *c.textures);
    header.materialsOffset = blob_put(&blob, c.materials, c.material_count * sizeof *c.materials);
    header.meshesOffset = blob_put(&blob, meshes, (size_t)mesh_count * sizeof *meshes);
    header.animations = animations;
    header.animationsOffset = animations_offset;
    memcpy(header.source, c.source, sizeof header.source);
    free(meshes);
    cook_free(&c);
    if (blob.failed) {
        free(blob.data);
        return refuse(&c, "cooked model exceeds 4 GiB or memory is exhausted");
    }
    header.bytes = (uint32_t)blob.size;
    header.bodyCrc = crc32_of(blob.data + sizeof header, blob.size - sizeof header);
    memcpy(blob.data, &header, sizeof header);
    if (p4_model_cooked_check(blob.data, blob.size)) {
        free(blob.data);
        return refuse(&c, "internal error: the cooked model fails its own check");
    }
    *out = blob.data;
    *out_size = blob.size;
    return 0;
}

/* ------------------------------------------------------------------------------------------------ */

static uint32_t crc32_of(const uint8_t *bytes, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static int span_ok(size_t size, uint32_t offset, uint64_t bytes)
{
    return offset % 4 == 0 && (uint64_t)offset + bytes <= size;
}

int p4_model_cooked_check(const uint8_t *blob, size_t size)
{
    if (!blob || size < sizeof(P4ModelCookedHeader) || size > UINT32_MAX) return -1;
    const P4ModelCookedHeader *h = p4_model_header(blob);
    if (h->magic != P4_MODEL_COOKED_MAGIC || h->version != P4_MODEL_COOKED_VERSION || h->bytes != size) return -1;
    if (h->source[64] || strlen(h->source) != 64) return -1;
    if (h->bodyCrc != crc32_of(blob + sizeof *h, size - sizeof *h)) return -1;
    if (!span_ok(size, h->texturesOffset, (uint64_t)h->textures * sizeof(P4ModelCookedTexture)) ||
        !span_ok(size, h->materialsOffset, (uint64_t)h->materials * sizeof(P4ModelCookedMaterial)) ||
        !span_ok(size, h->meshesOffset, (uint64_t)h->meshes * sizeof(P4ModelCookedMesh)) || !h->meshes)
        return -1;
    for (uint32_t i = 0; i < h->textures; ++i) {
        const P4ModelCookedTexture *t = p4_model_texture(blob, i);
        if (t->name[P4_MODEL_NAME_BYTES - 1] || !power_of_two(t->width) || !power_of_two(t->height) ||
            t->width < 8 || t->height < 8 || t->width > 4096 || t->height > 4096)
            return -1;
        uint64_t pixels = (uint64_t)t->width * t->height;
        if (t->depth == 8) {
            if (!span_ok(size, t->paletteOffset, 1024) || t->pixelsBytes != pixels) return -1;
        } else if (t->depth != 32 || t->pixelsBytes != pixels * 4) {
            return -1;
        }
        if ((uint64_t)t->pixelsOffset + t->pixelsBytes > size) return -1;
    }
    for (uint32_t i = 0; i < h->materials; ++i) {
        const P4ModelCookedMaterial *m = p4_model_material(blob, i);
        if (m->name[P4_MODEL_NAME_BYTES - 1] || m->texture < -1 || m->texture >= (int32_t)h->textures ||
            m->drawCategory < -1 || m->drawCategory > 9)
            return -1;
    }
    uint64_t triangles = 0;
    for (uint32_t i = 0; i < h->meshes; ++i) {
        const P4ModelCookedMesh *m = p4_model_mesh(blob, i);
        if (m->frameKind < P4_MODEL_FRAME_ROOT || m->frameKind > P4_MODEL_FRAME_SKIN || !m->vertices ||
            m->vertices > 65535u || !m->triangles || !m->materialCount || m->uvSets > 2)
            return -1;
        if (!span_ok(size, m->positionsOffset, (uint64_t)m->vertices * 12) ||
            (m->normalsOffset && !span_ok(size, m->normalsOffset, (uint64_t)m->vertices * 12)) ||
            (m->uvSets >= 1 && !span_ok(size, m->uv0Offset, (uint64_t)m->vertices * 8)) ||
            (m->uvSets == 2 && !span_ok(size, m->uv1Offset, (uint64_t)m->vertices * 8)) ||
            (m->colorsOffset && !span_ok(size, m->colorsOffset, (uint64_t)m->vertices * 4)) ||
            !span_ok(size, m->trianglesOffset, (uint64_t)m->triangles * 8) ||
            !span_ok(size, m->materialsOffset, (uint64_t)m->materialCount * 4))
            return -1;
        const uint32_t *slots = (const uint32_t *)(const void *)(blob + m->materialsOffset);
        for (uint32_t s = 0; s < m->materialCount; ++s)
            if (slots[s] >= h->materials) return -1;
        const uint16_t *t = (const uint16_t *)(const void *)(blob + m->trianglesOffset);
        for (uint32_t k = 0; k < m->triangles; ++k, t += 4)
            if (t[0] >= m->vertices || t[1] >= m->vertices || t[2] >= m->vertices || t[3] >= m->materialCount) return -1;
        if ((m->frameKind == P4_MODEL_FRAME_SKIN) != (m->skinOffset != 0)) return -1;
        if (m->skinOffset) {
            if (!span_ok(size, m->skinOffset, sizeof(P4ModelCookedSkin))) return -1;
            const P4ModelCookedSkin *s = p4_model_skin(blob, m);
            if (!s->joints || s->joints > 256 || !s->maxWeights || s->maxWeights > 4 ||
                !span_ok(size, s->jointIdsOffset, (uint64_t)s->joints * 4) ||
                !span_ok(size, s->matricesOffset, (uint64_t)s->joints * 64) ||
                !span_ok(size, s->indicesOffset, (uint64_t)m->vertices * 4) ||
                !span_ok(size, s->weightsOffset, (uint64_t)m->vertices * 16))
                return -1;
            const uint8_t *slot = blob + s->indicesOffset;
            for (uint64_t k = 0; k < (uint64_t)m->vertices * 4; ++k)
                if (slot[k] >= s->joints) return -1;
            if (s->orderCount) {
                if (s->orderCount > 256 || !span_ok(size, s->orderOffset, 0) || (uint64_t)s->orderOffset + s->orderCount > size)
                    return -1;
                for (uint32_t k = 0; k < s->orderCount; ++k)
                    if (blob[s->orderOffset + k] >= s->joints) return -1;
            }
        }
        triangles += m->triangles;
    }
    if (triangles != h->triangles) return -1;
    if (h->animations && !span_ok(size, h->animationsOffset, (uint64_t)h->animations * sizeof(P4ModelCookedAnimation))) return -1;
    for (uint32_t i = 0; i < h->animations; ++i) {
        const P4ModelCookedAnimation *a = p4_model_animation(blob, i);
        if (!a->tracks || a->tracks > 0x10000u || !(a->duration >= 0.0f && a->duration < 1e6f) ||
            !span_ok(size, a->tracksOffset, (uint64_t)a->tracks * sizeof(P4ModelCookedTrack)))
            return -1;
        for (int k = 0; k < 3; ++k)
            if (!(fabsf(a->translationScale[k]) < 1e30f) || !(fabsf(a->translationOffset[k]) < 1e30f)) return -1;
        const P4ModelCookedTrack *tracks = (const P4ModelCookedTrack *)(const void *)(blob + a->tracksOffset);
        for (uint32_t t = 0; t < a->tracks; ++t) {
            if (tracks[t].keys < 2 || !span_ok(size, tracks[t].keysOffset, (uint64_t)tracks[t].keys * sizeof(P4ModelCookedKey)))
                return -1;
            const P4ModelCookedKey *keys = (const P4ModelCookedKey *)(const void *)(blob + tracks[t].keysOffset);
            if (keys[0].time != 0.0f || keys[tracks[t].keys - 1].time != a->duration) return -1;
            for (uint32_t k = 1; k < tracks[t].keys; ++k)
                if (!(keys[k].time >= keys[k - 1].time)) return -1;
        }
    }
    return 0;
}
