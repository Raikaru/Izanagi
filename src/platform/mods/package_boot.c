#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
#include "p4pc/game_tables_native.h"
#include "package_boot_internal.h"
#include "failure_native.h"
#include <stdio.h>
#ifdef P4_MOD_LUAU
#include "p4pc/mod_script.h"
#endif

static int disabled=1;
void p4_package_boot_disable(void){disabled=1;}
void p4_package_boot_installed(void){disabled=0;}

static int completion_prepare(void *user)
{
    (void)user;
    int result=p4_game_tables_battle_prepare();
    if(!result)return 0;
    char reason[192];
    snprintf(reason,sizeof reason,"Native family-view preparation failed (%d). No native completion was committed.",result);
    return p4_package_boot_fail(NULL,reason);
}
static void completion_publish(void *user)
{
    (void)user;p4_game_tables_battle_commit();
}
/* This is the existing coordinator's actual completion, not a new registry.
 * Every source/UI participant registers during runtime_prepare. Its preparation
 * runs only after ALL such callbacks, before ANY runtime publication. */
const P4PackageHandler p4_package_tables_completion_boot_provider={
    .name="native.tables.completion",
    .boot_phase=P4_PACKAGE_BOOT_UI,
    .runtime_finalize_prepare=completion_prepare,
    .runtime_publish=completion_publish
};
int p4_package_boot_materialize(void)
{
    if(disabled)return 0;
    const P4PackageProvider *failed=NULL;const char *reason=NULL;
    int result=p4_package_registry_runtime_materialize(&failed,&reason);
    if(result<0){
        if(failed)p4_mod_native_runtime_failed(failed,reason);
        p4_mod_native_unattributed_failed(reason);
    }
    if(result>0)return 0; /* A real reload still runs ordinary native completion. */
#ifdef P4_MOD_LUAU
    p4_mod_scripts_start(); /* All CPU admission and late native publication won. */
#endif
    return 1; /* This invocation already committed; never complete twice. */
}
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *actual_provider,const char *reason)
{
    p4_mod_native_runtime_failed(actual_provider,reason);
}
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    p4_mod_native_unattributed_failed(reason);
}
