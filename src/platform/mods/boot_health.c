#include "p4pc/package_boot.h"
#include "package_boot_internal.h"
#include "../config/p4cfg.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int p4_package_boot_waiting_stable;
static uintptr_t startup_lease[2];

static int requested(const char *environment,const char *key)
{
    const char *value=getenv(environment);
    return (!value||strcmp(value,"0"))&&p4cfg_str(key,"")[0];
}
static int lease_settings(void)
{
    const char *path=p4cfg_path();
    if(!path){errno=EINVAL;return -1;}
    char directory[1024],resolved[4096];
    size_t length=strlen(path);
    if(length>=sizeof directory){errno=ENAMETOOLONG;return -1;}
    memcpy(directory,path,length+1);
    char *separator=strrchr(directory,'/');
#ifdef _WIN32
    char *back=strrchr(directory,'\\');
    if(back&&(!separator||back>separator))separator=back;
#endif
    if(separator)separator[1]=0;else memcpy(directory,".",2);
    if(p4_os_resolved_path(directory,resolved,sizeof resolved))return -1;
    return p4_os_directory_lock(resolved,startup_lease);
}
int p4_package_boot_health_begin(void)
{
    p4cfg_load();
    int enabled=requested("P4PC_MODS","mods.enabled")||requested("P4PC_PACKS","packs.enabled");
    int pending=p4cfg_int("mods.boot_pending",0)!=0;
    int crashes=p4cfg_int("mods.boot_early_crashes",0);
    int notice=p4cfg_int("mods.recovery_notice",0)!=0;
    if(!enabled&&!pending&&!crashes&&!notice)return 0; /* No vanilla work/artifact. */
    if(lease_settings()){
        if(enabled){
            p4_package_startup_blocked=1;
            return errno==EBUSY?-2:-1;
        }
        return 0; /* Never clear another live startup's marker/notice. */
    }
    /* Loading the path precedes ownership. Re-read under the REAL lease so a
     * previous owner's healthy exit cannot look like a crash in cached data. */
    if(p4cfg_refresh_startup()){
        p4_os_directory_unlock(startup_lease);
        if(enabled)p4_package_startup_blocked=1;
        return -1;
    }
    enabled=requested("P4PC_MODS","mods.enabled")||requested("P4PC_PACKS","packs.enabled");
    pending=p4cfg_int("mods.boot_pending",0)!=0;
    crashes=p4cfg_int("mods.boot_early_crashes",0);
    if(crashes<0)crashes=0;
    if(crashes>2)crashes=2;
    if(!enabled){
        int result=0;
        if(pending||crashes){
            const P4CfgChange clear[]={{"mods.boot_pending","0"},{"mods.boot_early_crashes","0"}};
            result=p4cfg_set_batch(clear,sizeof clear/sizeof clear[0]);
        }
        if(result||!p4cfg_int("mods.recovery_notice",0))p4_os_directory_unlock(startup_lease);
        return result;
    }
    if(!pending&&crashes==2)return 1; /* Keep lease through the safe-mode choice. */
    if(pending&&crashes<2)++crashes;
    const char *count=crashes==2?"2":crashes==1?"1":"0";
    const P4CfgChange changes[]={{"mods.boot_pending",crashes==2?"0":"1"},{"mods.boot_early_crashes",count}};
    if(p4cfg_set_batch(changes,sizeof changes/sizeof changes[0])){
        p4_os_directory_unlock(startup_lease);
        p4_package_startup_blocked=1;
        return -1;
    }
    if(crashes==2)return 1;
    p4_package_boot_waiting_stable=1;
    return 0;
}
void p4_package_boot_clean_exit(void)
{
    if(!p4_package_boot_waiting_stable)return;
    const P4CfgChange changes[]={{"mods.boot_pending","0"},{"mods.boot_early_crashes","0"}};
    if(p4cfg_set_batch(changes,sizeof changes/sizeof changes[0]))
        fprintf(stderr,"p4pc: startup reached a normal/stable boundary but its crash marker could not be cleared\n");
    /* Do not turn a settings I/O failure into one disk retry per game frame. */
    p4_package_boot_waiting_stable=0;
    p4_os_directory_unlock(startup_lease);
}
void p4_package_boot_stable(void)
{
    /* Called only for the actual interactive title task state, never a timer,
     * preview, scripted acknowledgement or CPU/native-materialization stamp. */
    p4_package_boot_clean_exit();
}
void p4_package_boot_admission_complete(void)
{
    if(!p4_packages_count())p4_package_boot_clean_exit();
}
void p4_package_boot_health_recorded(void)
{
    p4_package_boot_waiting_stable=0; /* Recovery already committed atomically. */
    p4_os_directory_unlock(startup_lease);
}
const char *p4_package_boot_health_notice(void)
{
    if((!startup_lease[0]&&!startup_lease[1])||!p4cfg_int("mods.recovery_notice",0))return NULL;
    if(!strcmp(p4cfg_str("mods.recovery_cause",""),"startup-crash-loop"))
        return "The game failed twice before its interactive title screen with optional packages enabled. Safe-mode recovery disabled every mod and content pack selector. Review your modifications before enabling them again.";
    return "An unidentified mod or native-service failure requested safe mode. Recovery disabled every mod and content pack selector. Review your modifications before enabling them again.";
}
int p4_package_boot_health_notice_complete(int shown)
{
    if(!startup_lease[0]&&!startup_lease[1])return -1;
    int result=shown?p4cfg_set_str("mods.recovery_notice","0"):0;
    if(!p4_package_boot_waiting_stable)p4_os_directory_unlock(startup_lease);
    return result;
}
