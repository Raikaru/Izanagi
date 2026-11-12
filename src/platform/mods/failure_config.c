#include "failure_native.h"
#include "../config/p4cfg.h"
#include <ctype.h>
#include <string.h>

int p4_package_recovery_persist(P4PackageKind kind,const char *id,int restart)
{
    if(!id||!id[0]||(kind!=P4_PACKAGE_MOD&&kind!=P4_PACKAGE_ENGINE_PACK))return -1;
    const char *selector=kind==P4_PACKAGE_ENGINE_PACK?"packs.enabled":"mods.enabled";
    char enabled[P4_PACKAGE_MAX*65u];
    size_t used=0,target=strlen(id);
    const char *cursor=p4cfg_get(selector);
    while(cursor&&*cursor){
        while(isspace((unsigned char)*cursor))++cursor;
        const char *end=strchr(cursor,',');
        if(!end)end=cursor+strlen(cursor);
        const char *trimmed=end;
        while(trimmed>cursor&&isspace((unsigned char)trimmed[-1]))--trimmed;
        size_t size=(size_t)(trimmed-cursor);
        if(size&&(size!=target||memcmp(cursor,id,size))){
            size_t separator=used!=0;
            if(size>=sizeof enabled-used-separator)return -1;
            if(separator)enabled[used++]=',';
            memcpy(enabled+used,cursor,size);used+=size;
        }
        cursor=*end?end+1:end;
    }
    enabled[used]=0;
    const P4CfgChange changes[]={
        {selector,enabled},
        {"mods.recovery_mod",id},
        {"mods.recovery_kind",kind==P4_PACKAGE_ENGINE_PACK?"engine-pack":"mod"},
        {"mods.recovery_choice",restart?"restart":"quit"},
        {"mods.boot_pending","0"},
        {"mods.boot_early_crashes","0"},
        {"mods.recovery_notice","0"}
    };
    return p4cfg_set_batch(changes,sizeof changes/sizeof changes[0]);
}
int p4_package_recovery_safe_mode(int startup_crash_loop)
{
    const P4CfgChange changes[]={
        {"mods.enabled",""},{"packs.enabled",""},
        {"mods.recovery_mod","all"},{"mods.recovery_kind","safe-mode"},
        {"mods.recovery_choice","restart"},
        {"mods.recovery_cause",startup_crash_loop?"startup-crash-loop":"unattributed-runtime"},
        {"mods.recovery_notice","1"},
        {"mods.boot_pending","0"},{"mods.boot_early_crashes","0"}
    };
    return p4cfg_set_batch(changes,sizeof changes/sizeof changes[0]);
}
int p4_package_recovery_quit(int startup_crash_loop)
{
    const P4CfgChange changes[]={
        {"mods.recovery_mod",""},{"mods.recovery_kind","unattributed"},
        {"mods.recovery_choice","quit"},{"mods.recovery_notice","0"},
        {"mods.boot_pending","0"},{"mods.boot_early_crashes",startup_crash_loop?"2":"0"}
    };
    return p4cfg_set_batch(changes,sizeof changes/sizeof changes[0]);
}
