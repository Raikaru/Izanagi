#include "failure_native.h"
#include "p4pc/package_boot.h"
#include "p4pc/mods.h"
#include "p4pc/packs.h"
#include "p4pc/os.h"
#include "p4pc/crt.h"
#include "package_boot_internal.h"
#include "../config/p4cfg.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int message(const char *title,const char *text)
{
    fprintf(stderr,"p4pc: %s: %s\n",title,text);
    if(p4_mod_witness_enabled()){
        fprintf(stderr,"p4pc: native mod message mod_witness=true reference_eligible=false; scripted acknowledgement\n");
        return 1;
    }
    if(p4_packages_automation_requested())return 0;
    if(SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,title,text,NULL))return 1;
    fprintf(stderr,"p4pc: native message presentation failed: %s\n",SDL_GetError());
    return 0;
}
int p4_package_native_refuse(P4PackageKind kind,const char *id,const char *reason)
{
    char text[3072];
    snprintf(text,sizeof text,"%s %s was refused before any replacement files or code were exposed:\n%s\n\nStarting without this package and its replacement files.",
             kind==P4_PACKAGE_ENGINE_PACK?"Engine pack":"Mod",id,reason);
    message("Persona 4 package refused",text);
    return 0;
}
static int refuse(const char *id,const char *reason,void *user)
{
    (void)user;return p4_package_native_refuse(P4_PACKAGE_MOD,id,reason);
}
static int refuse_pack(const char *id,const char *reason,void *user)
{
    (void)user;return p4_package_native_refuse(P4_PACKAGE_ENGINE_PACK,id,reason);
}
static _Noreturn void recover(const P4PackageProvider *provider,const char *reason,int unknown_origin,int startup_crash_loop);
static int concurrent_continue(void)
{
    const char *text="Another process owns this settings directory. You may continue with all mods and content packs off for THIS INSTANCE ONLY; your stored selections and existing startup history are unchanged. Close the other instance and restart to use your selected packages.";
    fprintf(stderr,"p4pc: %s\n",text);
    if(p4_mod_witness_enabled()){
        fprintf(stderr,"p4pc: concurrent startup instance-only packages-off mod_witness=true reference_eligible=false\n");
        return 1;
    }
    if(p4_packages_automation_requested())return 0;
    const SDL_MessageBoxButtonData buttons[]={
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,"Continue with mods off"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Quit"}
    };
    const SDL_MessageBoxData box={.flags=SDL_MESSAGEBOX_WARNING,
        .title="Persona 4 settings directory is in use",.message=text,
        .numbuttons=2,.buttons=buttons};
    int choice=0;
    if(!SDL_ShowMessageBox(&box,&choice)){
        fprintf(stderr,"p4pc: concurrent-startup message failed: %s\n",SDL_GetError());
        return 0;
    }
    return choice==1;
}
int p4_mod_native_failure_init(void)
{
    if(p4_mod_witness_validate())return -1;
    if(p4_mods_set_refusal_handler(refuse,NULL) || p4_packs_set_refusal_handler(refuse_pack,NULL))return -1;
    int health=p4_package_boot_health_begin();
    if(health>0)recover(NULL,"Two unclean startups before the interactive title screen.",1,1);
    if(health==-2){
        if(!concurrent_continue())exit(EXIT_FAILURE);
        return 0;
    }
    if(health<0)
        message("Persona 4 optional packages disabled for this launch","Startup crash protection could not be protected or saved. Optional packages will not be prepared for this launch; your stored selections are unchanged. Fix the settings location before enabling them again.");
    const char *notice=p4_package_boot_health_notice();
    if(notice&&p4_package_boot_health_notice_complete(message("Persona 4 started in safe mode",notice)))
        fprintf(stderr,"p4pc: safe-mode notice could not be marked as shown\n");
    if(p4_package_boot_waiting_stable&&p4pc_on_normal_exit(p4_package_boot_clean_exit))return -1;
    return 0;
}
int p4_mod_native_startup_refused(const char *reason)
{
    p4_package_boot_disable();
    message("Persona 4 optional packages unavailable",reason);
    return !p4_packages_automation_requested()||p4_mod_witness_enabled();
}
static int recovery_choice(const char *id,const char *text)
{
    if(p4_mod_witness_enabled()){
        const char *choice=getenv("P4PC_MOD_RECOVERY_CHOICE");
        if(choice&&!strcmp(choice,"restart"))return 1;
        if(choice&&strcmp(choice,"quit"))fprintf(stderr,"p4pc: invalid scripted recovery choice; quitting safely\n");
        return 0;
    }
    if(p4_packages_automation_requested())return 0;
    char restart[128];
    if(id)snprintf(restart,sizeof restart,"Restart without %s",id);
    else strcpy(restart,"Restart with all mods disabled");
    const SDL_MessageBoxButtonData buttons[]={
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,restart},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Quit"}
    };
    const SDL_MessageBoxData box={
        .flags=SDL_MESSAGEBOX_ERROR,.title="Persona 4 package failed",.message=text,
        .numbuttons=(int)(sizeof buttons/sizeof buttons[0]),.buttons=buttons
    };
    int choice=0;
    if(!SDL_ShowMessageBox(&box,&choice)){
        fprintf(stderr,"p4pc: native recovery presentation failed: %s\n",SDL_GetError());return 0;
    }
    return choice==1;
}
static _Noreturn void recover(const P4PackageProvider *provider,const char *reason,int unknown_origin,int startup_crash_loop)
{
    const P4PackageInfo *info=provider?p4_package_provider_info(provider):NULL;
    const char *id=info?info->id:NULL;
    char text[4096];
    const char *detail=reason&&reason[0]?reason:"No further diagnostic was available";
    if(info)
        snprintf(text,sizeof text,"%s %s failed after replacement files/native work were exposed:\n%s\n\nThe game must restart. This package will be disabled in your settings before restarting or quitting.",
            info->kind==P4_PACKAGE_ENGINE_PACK?"Engine pack":"Mod",id,detail);
    else
        snprintf(text,sizeof text,"%s\n%s\n\nNo individual mod has been identified. Restart with all mods disabled turns off every mod and content pack selector for the next launch. Quit leaves your selections unchanged. Native game data files are not modified.",
            startup_crash_loop?"The game ended unexpectedly twice before its interactive title screen.":
            unknown_origin?"An unidentified mod or native service failed.":"An authenticated native source failed.",detail);
    fprintf(stderr,"p4pc: %s\n",text);
    int restart=recovery_choice(id,text);
    if(p4_mod_witness_enabled())
        fprintf(stderr,"p4pc: package recovery mod_witness=true reference_eligible=false action=%s\n",
            restart?(id?"restart-without-owner":"restart-all-disabled"):"quit");
    int persisted=info?p4_package_recovery_persist(info->kind,id,restart):
        restart?p4_package_recovery_safe_mode(startup_crash_loop):p4_package_recovery_quit(startup_crash_loop);
    if(persisted){
        message("Persona 4 recovery could not be saved","The recovery choice could not be saved atomically. Automatic restart is unavailable. Quit, fix the settings file permissions or disable packages manually, then start the game again.");
        exit(EXIT_FAILURE);
    }
    p4_package_boot_health_recorded();
    if(restart){
        if(!p4_os_request_restart(detail))p4pc_exit(EXIT_SUCCESS);
        message("Persona 4 restart unavailable","Your recovery settings were saved, but this platform could not schedule a fresh process. Quit and start the game again manually.");
    }
    /* No fake rollback/early destructor: native work may still retain these
     * VMs, provider pins, handles and guest allocations. Drain owns their exit. */
    exit(EXIT_FAILURE);
}
_Noreturn void p4_mod_native_runtime_failed(const P4PackageProvider *provider,const char *reason)
{
    if(provider&&!p4_package_provider_selected(provider))recover(NULL,reason,1,0);
    recover(provider,reason,0,0); /* NULL here is explicitly known-native only. */
}
_Noreturn void p4_mod_native_unattributed_failed(const char *reason)
{
    recover(NULL,reason,1,0);
}
