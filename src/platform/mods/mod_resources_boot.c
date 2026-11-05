#include "p4pc/mod_resources_boot.h"
#include "p4pc/content_source.h"
#include "p4pc/mod_setting_gate.h"
#include "file_index.h"
#include "package_memory_internal.h"
#include "resource_png.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
#define RESOURCE_GPU_BUDGET (128u * 1024u * 1024u)
#else
#define RESOURCE_GPU_BUDGET (512u * 1024u * 1024u)
#endif
#define RESOURCE_FILE_LIMIT (128u * 1024u * 1024u)

typedef struct ImageInput {
    char owner[64], key[96];
    uint32_t revision;
    const P4PackageProvider *provider;
    P4PackageFilePlan *plan;
    P4ContentSource source;
    P4ResourcePng image;
} ImageInput;
struct P4ResourceImagesPrepared {
    ImageInput *images;
    size_t count, capacity;
};
static P4ResourceImagesPrepared prepared;
/* This generation survives any post-exposure failure. Only the core's
 * independently quiesced session end may retire it and release input plans. */
static P4Resources *runtime_resources;
static uint32_t next_revision = 1;

static int identifier(const char *text, size_t size)
{
    if (!text || !size || text[0] == '.' || text[size - 1] == '.') return 0;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}
static int qualified(const char *key, char owner[64])
{
    if (!key) return 0;
    const char *colon = strchr(key, ':');
    if (!colon || colon == key || !colon[1] || strchr(colon + 1, ':')) return 0;
    size_t owner_size = (size_t)(colon - key), key_size = strlen(key);
    if (owner_size >= 64 || key_size >= 96 || !identifier(key, owner_size) ||
        !identifier(colon + 1, key_size - owner_size - 1)) return 0;
    memcpy(owner, key, owner_size);
    owner[owner_size] = 0;
    return 1;
}
static int decimal(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    uint32_t value = 0;
    do {
        unsigned char c = (unsigned char)*text++;
        if (c < '0' || c > '9' || value > (UINT32_MAX - (c - '0')) / 10u) return 0;
        value = value * 10u + c - '0';
    } while (*text);
    if (!value) return 0;
    *out = value;
    return 1;
}
static void image_release(ImageInput *image)
{
    p4_resource_png_free(&image->image);
    p4_package_file_plan_release(image->plan);
    memset(image, 0, sizeof *image);
}
static void prepared_clear(void)
{
    for (size_t i = 0; i < prepared.count; ++i) image_release(&prepared.images[i]);
    p4_package_metadata_free(prepared.images, prepared.capacity * sizeof *prepared.images);
    memset(&prepared, 0, sizeof prepared);
}
size_t p4_resources_images_prepared_count(const P4ResourceImagesPrepared *view)
{
    return view == &prepared ? prepared.count : 0;
}
int p4_resources_images_prepared_find(const P4ResourceImagesPrepared *view,
                                      const char *owner, const char *key,
                                      uint32_t revision)
{
    char actual_owner[64];
    if (view != &prepared || !owner || !revision || !qualified(key, actual_owner) ||
        strcmp(owner, actual_owner)) return -1;
    for (size_t i = 0; i < prepared.count; ++i) {
        const ImageInput *image = &prepared.images[i];
        if (!strcmp(image->key, key))
            return image->revision == revision && !strcmp(image->owner, owner);
    }
    return 0;
}
static int refuse(const P4PackageProvider *provider, const char *key,
                  const char *file, const char *why)
{
    char message[512];
    snprintf(message, sizeof message, "resource-image %.95s (%.160s): %.180s",
             key ? key : "?", file ? file : "?", why);
    p4_package_provider_refuse(provider, message);
    return -1;
}
static int grow(void)
{
    if (prepared.count < prepared.capacity) return 0;
    size_t next = prepared.capacity ? prepared.capacity * 2 : 8;
    if (next < prepared.capacity || next > SIZE_MAX / sizeof *prepared.images) return -1;
    size_t old_bytes = prepared.capacity * sizeof *prepared.images;
    size_t new_bytes = next * sizeof *prepared.images;
    ImageInput *images = p4_package_metadata_resize(prepared.images, old_bytes, new_bytes);
    if (!images) return -1;
    prepared.images = images;
    prepared.capacity = next;
    return 0;
}
static const char *actual_file(const P4PackageProvider *provider, const char *path)
{
    size_t first = 0, last = p4_package_provider_files_count(provider);
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        const char *actual;
        int directory;
        if (p4_package_provider_file(provider, middle, &actual, &directory)) return NULL;
        int order = p4_package_key_compare(actual, path);
        if (!order) return directory ? NULL : actual;
        if (order < 0) first = middle + 1;
        else last = middle;
    }
    return NULL;
}
static int prepare_image(const P4PackageProvider *provider, const P4PackageField *fields,
                         size_t count)
{
    const P4PackageInfo *info = p4_package_provider_info(provider);
    const char *key = fields[0].argument, *file = NULL, *format = NULL, *schema = NULL, *revision = NULL;
    ImageInput image = {0};
    if (!info || !p4_package_provider_selected(provider) || !qualified(key, image.owner))
        return refuse(provider, key, NULL, "expected a bounded lowercase owner:key");
    size_t gates = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(fields[i].key, "file")) file = fields[i].value;
        else if (!strcmp(fields[i].key, "format")) format = fields[i].value;
        else if (!strcmp(fields[i].key, "schema")) schema = fields[i].value;
        else if (!strcmp(fields[i].key, "revision")) revision = fields[i].value;
        else if (!strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD)) ++gates; /* evaluated by prepare() */
        else return refuse(provider, key, file, "unknown declaration field");
    }
    uint32_t schema_number;
    if (gates > 1 || count - gates != 4 || !file || !*file || !format || strcmp(format, "png") ||
        !decimal(schema, &schema_number) || schema_number != 1 || !decimal(revision, &image.revision))
        return refuse(provider, key, file, "requires file, format=png, schema=1 and nonzero revision");
    const char *actual = actual_file(provider, file);
    if (!actual || strlen(actual) >= sizeof image.source.key)
        return refuse(provider, key, file, "contained indexed source is missing or its key is too long");
    P4PackageFile *stream = NULL;
    if (p4_package_file_open(provider, actual, RESOURCE_FILE_LIMIT, &stream))
        return refuse(provider, key, file, "cannot open the selected contained source");
    char error[192] = {0};
    int failed = p4_package_file_plan_acquire(stream, &image.plan) ||
                 p4_resource_png_decode(stream, &image.image, error, sizeof error);
    if (p4_package_file_close(stream)) failed = 1;
    if (!failed) {
        /* Check the same issued object's revision after decoding, before any
         * raw-file exposure; never adopt a replacement by path. */
        P4PackageFile *check = NULL;
        if (p4_package_file_plan_open(image.plan, &check)) failed = 1;
        else if (p4_package_file_close(check)) failed = 1;
    }
    if (failed) {
        image_release(&image);
        return refuse(provider, key, file, error[0] ? error : "source identity or close failed");
    }
    if (p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_COSMETIC)) {
        image_release(&image);
        return refuse(provider, key, file, "cannot claim the selected cosmetic source");
    }
    strcpy(image.key, key);
    image.provider = provider; /* plan pins this exact candidate */
    image.source.origin = P4_TEXTURE_OVERRIDE;
    /* Direct provider-plan I/O has no VFS layer rank. Never relabel the
     * catalog ordinal (or any other invented number) as one. */
    image.source.rank = P4_CONTENT_RANK_PACKAGE_PLAN;
    strcpy(image.source.provider, info->id);
    strcpy(image.source.key, actual);
    for (size_t i = 0; i < prepared.count; ++i) {
        if (!strcmp(prepared.images[i].key, image.key)) {
            fprintf(stderr, "p4pc: resource-image %s: provider %s overrides provider %s\n",
                image.key, image.source.provider, prepared.images[i].source.provider);
            image_release(&prepared.images[i]);
            prepared.images[i] = image; /* catalog order: later provider wins */
            return 0;
        }
    }
    if (grow()) {
        image_release(&image);
        return refuse(provider, key, file, "resource metadata budget exhausted");
    }
    prepared.images[prepared.count++] = image;
    return 0;
}
static int prepare(void *unused)
{
    (void)unused;
    if (runtime_resources || prepared.images || prepared.count) return -1;
    for (size_t i = 0, n = p4_packages_count(); i < n; ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        size_t count = 0;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        for (size_t first = 0; first < count;) {
            size_t last = first + 1;
            while (last < count && !strcmp(fields[first].section, fields[last].section) &&
                   !strcmp(fields[first].argument, fields[last].argument)) ++last;
            if (!strcmp(fields[first].section, "resource-image")) {
                char reason[192];
                const int gate = p4_mod_setting_section_gate(provider, fields + first, last - first, reason, sizeof reason);
                if (gate < 0) return refuse(provider, fields[first].argument, NULL, reason);
                if (gate && prepare_image(provider, fields + first, last - first)) return -1;
            }
            first = last;
        }
    }
    return p4_package_boot_provide(prepared.count ? &prepared : NULL);
}
static int finalize_prepare(void *unused)
{
    (void)unused;
    for (size_t i = 0; i < prepared.count; ++i) {
        const ImageInput *image = &prepared.images[i];
        P4PackageFile *check = NULL;
        if (p4_package_file_plan_open(image->plan, &check))
            return refuse(image->provider, image->key, image->source.key,
                          "selected source changed before publication");
        if (p4_package_file_close(check))
            return refuse(image->provider, image->key, image->source.key,
                          "selected source failed final identity check");
    }
    return 0;
}
static void rollback(void *unused)
{
    (void)unused;
    prepared_clear(); /* early pass only, before raw file exposure */
}
static int runtime_prepare(void *unused)
{
    (void)unused;
    if (runtime_resources) return p4_package_boot_fail(NULL, "Resource generation still active");
    if (!prepared.count) return p4_package_boot_provide(NULL);
    if (!next_revision) return p4_package_boot_fail(NULL, "Resource generation unavailable");
    runtime_resources = p4_resources_create(RESOURCE_GPU_BUDGET);
    if (!runtime_resources) return p4_package_boot_fail(NULL, "Cannot create resource generation");
    for (size_t i = 0; i < prepared.count; ++i) {
        const ImageInput *image = &prepared.images[i];
        uint32_t token;
        if (!p4_package_provider_selected(image->provider))
            return p4_package_boot_fail(NULL, "Selected image provider is no longer active");
        if (p4_resources_image(runtime_resources, image->owner, image->key, &image->source,
                image->image.rgba, image->image.size, image->image.width, image->image.height,
                image->revision, &token) != P4_RESOURCE_OK)
            return p4_package_boot_fail(image->provider, "Cannot materialize the selected authored image");
    }
    if (p4_resources_freeze(runtime_resources, next_revision++) != P4_RESOURCE_OK)
        return p4_package_boot_fail(NULL, "Cannot freeze resource generation");
    return p4_package_boot_provide(runtime_resources);
}
int p4_resources_images_session_end(void)
{
    if (runtime_resources) {
        int result = p4_resources_retire_unpinned(runtime_resources);
        if (result != P4_RESOURCE_OK) return result;
        runtime_resources = NULL;
    }
    prepared_clear();
    return P4_RESOURCE_OK;
}
static const char *const sections[] = {"resource-image", NULL};
/* A resource-image's `when` reads this admission's settings snapshot. */
static const char *const dependencies[] = {P4_MOD_SETTINGS_HANDLER, NULL};
const P4PackageHandler p4_resources_images_boot_provider = {
    .name = P4_RESOURCES_IMAGES_HANDLER,
    .sections = sections,
    .prepare = prepare,
    .rollback = rollback,
    .boot_phase = P4_PACKAGE_BOOT_RESOURCES,
    .dependencies = dependencies,
    .finalize_prepare = finalize_prepare,
    .runtime_prepare = runtime_prepare
};
