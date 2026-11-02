#include "lua_runtime.h"
#include "p4pc/mod_bindings.h"
#include "p4pc/package_boot.h"

static void runtime_failed(size_t owner,const char *reason,void *user)
{
    (void)user;
    const P4PackageProvider *provider=p4_mod_scripts_owner_provider(owner);
    if(provider)p4_package_boot_source_failed(provider,reason);
    p4_package_boot_runtime_failed(reason); /* Unknown is never native by default. */
}
int p4_mod_script_failure_init(void)
{
    return p4_mod_failure_handler(runtime_failed,NULL);
}
