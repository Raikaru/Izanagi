/* Test driver for the glTF model cooker (src/platform/models): cooks a .gltf/.glb file the way a
 * [resource-model] declaration does and writes the cooked blob, or prints the refusal.
 *
 *   model_cook_cli <model.gltf|model.glb> <replaces disc key> <out.p4mc> [cache directory]
 *   model_cook_cli --stream <cooked.p4mc> <mesh> <node ids> <out.bin>
 *   model_cook_cli --anim-stream <cooked.p4mc> L<layer>S<slot> <node ids> <out.bin>
 *
 * Exit 0 when cooked (prints "cooked <bytes> <hash> <cache hit|miss>"), 1 with the reason on stderr when
 * refused. External buffers and images resolve next to the glTF file. --stream writes the RenderWare
 * geometry chunk the runtime builder reads for skinned mesh <mesh>, --anim-stream the RtAnimAnimation
 * chunk it reads for that slot's animation, for a hierarchy whose nodes have the comma-separated HAnim
 * ids <node ids> in index order (prints "stream <bytes>"). */
#include "../src/platform/models/p4model.h"

#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LodePNG is built without its allocators (LODEPNG_NO_COMPILE_ALLOCATORS): plain heap here. */
void *lodepng_malloc(size_t size) { return malloc(size); }
void *lodepng_realloc(void *p, size_t size) { return realloc(p, size); }
void lodepng_free(void *p) { free(p); }

static int read_all(const char *path, uint8_t **data, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END)) {
        fclose(f);
        return -1;
    }
    long n = ftell(f);
    if (n < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return -1;
    }
    uint8_t *buffer = malloc(n ? (size_t)n : 1);
    if (!buffer || fread(buffer, 1, (size_t)n, f) != (size_t)n) {
        free(buffer);
        fclose(f);
        return -1;
    }
    fclose(f);
    *data = buffer;
    *size = (size_t)n;
    return 0;
}

static char base[1024];

static int read_beside(void *user, const char *uri, uint8_t **data, size_t *size)
{
    (void)user;
    char path[2048];
    if (strstr(uri, "..")) return -1;
    snprintf(path, sizeof path, "%s%s", base, uri);
    return read_all(path, data, size);
}

static int stream_main(int argc, char **argv)
{
    const int animation = !strcmp(argv[1], "--anim-stream");
    uint8_t *blob;
    size_t size;
    if (argc != 6 || read_all(argv[2], &blob, &size) || p4_model_cooked_check(blob, size)) {
        fprintf(stderr, "usage: model_cook_cli --stream <cooked.p4mc> <mesh> <node ids> <out.bin>\n"
                        "       model_cook_cli --anim-stream <cooked.p4mc> L<layer>S<slot> <node ids> <out.bin>\n");
        return 2;
    }
    int32_t ids[256];
    uint32_t nodes = 0;
    for (const char *p = argv[4]; *p && nodes < 256;) {
        char *end;
        ids[nodes++] = (int32_t)strtol(p, &end, 10);
        if (end == p) break;
        p = *end == ',' ? end + 1 : end;
    }
    const P4ModelCookedMesh *m = NULL;
    const P4ModelCookedAnimation *a = NULL;
    if (animation) {
        unsigned layer = 0, slot = 0;
        if (sscanf(argv[3], "L%uS%u", &layer, &slot) == 2)
            for (uint32_t i = 0; i < p4_model_header(blob)->animations && !a; ++i)
                if (p4_model_animation(blob, i)->layer == layer && p4_model_animation(blob, i)->slot == slot) a = p4_model_animation(blob, i);
    } else {
        uint32_t mesh = (uint32_t)strtoul(argv[3], NULL, 10);
        if (mesh < p4_model_header(blob)->meshes && p4_model_mesh(blob, mesh)->skinOffset) m = p4_model_mesh(blob, mesh);
    }
    if (!m && !a) {
        fprintf(stderr, "model_cook_cli: %s is not a %s of %s\n", argv[3], animation ? "slot animation" : "skinned mesh", argv[2]);
        return 2;
    }
    char error[256];
    size_t bytes = a ? p4_model_animation_stream(blob, a, ids, nodes, NULL, 0, error, sizeof error)
                     : p4_model_geometry_stream(blob, m, ids, nodes, NULL, 0, error, sizeof error);
    if (!bytes) {
        fprintf(stderr, "refused: %s\n", error);
        return 1;
    }
    uint8_t *out = malloc(bytes);
    FILE *f = out ? fopen(argv[5], "wb") : NULL;
    size_t written = !out ? 0
                     : a ? p4_model_animation_stream(blob, a, ids, nodes, out, bytes, NULL, 0)
                         : p4_model_geometry_stream(blob, m, ids, nodes, out, bytes, NULL, 0);
    if (!f || written != bytes || fwrite(out, 1, bytes, f) != bytes || fclose(f)) {
        fprintf(stderr, "model_cook_cli: cannot write %s\n", argv[5]);
        return 2;
    }
    printf("stream %zu\n", bytes);
    free(out);
    free(blob);
    return 0;
}

int main(int argc, char **argv)
{
    int round_zero = getenv("P4_MODEL_TEST_ROUND_ZERO") != NULL;
    if (round_zero && fesetround(FE_TOWARDZERO) != 0) return 2;
    if (argc > 1 && (!strcmp(argv[1], "--stream") || !strcmp(argv[1], "--anim-stream"))) return stream_main(argc, argv);
    if (argc < 4) {
        fprintf(stderr, "usage: model_cook_cli <model.gltf|.glb> <replaces key> <out.p4mc> [cache dir]\n");
        return 2;
    }
    const char *slash = strrchr(argv[1], '/');
    const char *back = strrchr(argv[1], '\\');
    if (back && (!slash || back > slash)) slash = back;
    snprintf(base, sizeof base, "%.*s", slash ? (int)(slash - argv[1] + 1) : 0, argv[1]);
    uint8_t *gltf;
    size_t size;
    if (read_all(argv[1], &gltf, &size)) {
        fprintf(stderr, "model_cook_cli: cannot read %s\n", argv[1]);
        return 2;
    }
    const P4ModelBudget *budget = p4_model_budget_for(argv[2]);
    char error[512];
    char source[68];
    const char *cache = argc > 4 ? argv[4] : NULL;
    uint8_t *blob = NULL;
    size_t blob_size = 0;
    int hit = 0;
    if (cache && p4_model_source_hash(argv[1], gltf, size, read_beside, NULL, source, error, sizeof error) == 0 &&
        p4_model_cache_load(cache, source, &blob, &blob_size) == 0)
        hit = 1;
    if (!hit && p4_model_cook(argv[1], gltf, size, read_beside, NULL, budget, &blob, &blob_size, error, sizeof error)) {
        fprintf(stderr, "refused: %s\n", error);
        free(gltf);
        return 1;
    }
    if (round_zero && fegetround() != FE_TOWARDZERO) {
        fprintf(stderr, "model_cook_cli: cooker did not restore the floating-point rounding mode\n");
        return 2;
    }
    free(gltf);
    if (!hit && cache && p4_model_cache_store(cache, blob, blob_size)) fprintf(stderr, "model_cook_cli: cache store failed\n");
    FILE *out = fopen(argv[3], "wb");
    if (!out || fwrite(blob, 1, blob_size, out) != blob_size || fclose(out)) {
        fprintf(stderr, "model_cook_cli: cannot write %s\n", argv[3]);
        return 2;
    }
    printf("cooked %zu %s %s %s\n", blob_size, p4_model_header(blob)->source, budget->category, hit ? "hit" : "miss");
    free(blob);
    return 0;
}
