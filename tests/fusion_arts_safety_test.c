/* Run the shipped Fusion Arts script in the real Luau VM with explicit test
 * providers. This checks its page state, not native item or UI qualification. */
#include "p4pc/luau_c.h"
#include <stdio.h>
#include <stdlib.h>

static int run_file(p4_luau *vm, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) { fprintf(stderr, "cannot open %s\n", path); return 1; }
    if (fseek(file, 0, SEEK_END)) { fclose(file); return 1; }
    long length = ftell(file);
    if (length < 0 || length > 1024 * 1024 || fseek(file, 0, SEEK_SET)) { fclose(file); return 1; }
    char *source = malloc((size_t)length + 1);
    if (!source) { fclose(file); return 1; }
    size_t count = fread(source, 1, (size_t)length, file);
    fclose(file);
    if (count != (size_t)length) { free(source); return 1; }
    source[count] = 0;
    int status = p4_luau_load_source(vm, path, source, count);
    free(source);
    if (status == LUA_OK) status = p4_luau_run(vm, 0, 0);
    if (status != LUA_OK) { fprintf(stderr, "%s: %s\n", path, p4_luau_error(vm)); return 1; }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    p4_luau_config config = {8 * 1024 * 1024, NULL, NULL, NULL, 1024 * 1024};
    p4_luau *vm = p4_luau_create(&config);
    if (!vm) return 1;
    int failed = run_file(vm, argv[1]) || run_file(vm, argv[2]) || run_file(vm, argv[3]);
    p4_luau_destroy(vm);
    return failed ? 1 : 0;
}
