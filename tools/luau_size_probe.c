/* Same link flags for both builds: their stripped-file delta includes the
 * interpreter, in-process compiler and the retained static C++ runtime. */
#include "p4pc/luau_c.h"
#ifndef P4_LUAU_SIZE_BASELINE
static const char source[] = "return 6 * 7";
#endif
int main(void)
{
#ifdef P4_LUAU_SIZE_BASELINE
    return 0;
#else
    p4_luau_config config = {1024 * 1024, 0, 0, 0, 1024};
    p4_luau *vm = p4_luau_create(&config);
    if (!vm) return 1;
    int status = p4_luau_load_source(vm, "@size-probe", source, sizeof(source) - 1);
    if (!status) status = p4_luau_run(vm, 0, 1);
    if (!status && lua_tointeger(p4_luau_state(vm), -1) != 42) status = 1;
    p4_luau_destroy(vm);
    return status;
#endif
}
