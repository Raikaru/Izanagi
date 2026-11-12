#include "p4pc/mod_save_native.h"
#include "mod_save_transport.h"
#include "p4pc/package_boot.h"
#include "p4pc/mod_fidelity.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static char restore_error[1024];
static void native_error(const char *operation,const char *reason)
{
    fprintf(stderr,"p4pc: %s: %s\n",operation,reason);
    /* Never invent an acknowledgement for an unattended mod witness. The
     * actual MC result refuses the operation; interactive players get native UI. */
    if(!p4_packages_automation_requested())SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,operation,reason,NULL);
}
/* Bound each real native dialog, not the diagnostic. Every byte remains
 * reachable through Details, even a save with thousands of missing owners. */
static size_t warning_page(const char *text,size_t remaining)
{
    size_t length=remaining<1000?remaining:1000,last_line=0;
    unsigned lines=0;
    for(size_t i=0;i<length;i++)if(text[i]=='\n'){
        last_line=i+1;
        if(++lines==8){length=last_line;break;}
    }
    if(length<remaining){
        if(last_line)length=last_line;
        else while(length&&((unsigned char)text[length]&0xc0)==0x80)--length;
    }
    return length;
}
static int warning_details(const char *warning,size_t length)
{
    size_t offset=0;
    while(offset<length){
        size_t count=warning_page(warning+offset,length-offset);
        if(!count)return -1;
        char text[1001];memcpy(text,warning+offset,count);text[count]=0;
        int more=offset+count<length,choice=0;
        const SDL_MessageBoxButtonData buttons[]={
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Back to load choice"},
            {0,1,"Next warnings"}
        };
        const SDL_MessageBoxData message={SDL_MESSAGEBOX_WARNING,NULL,
            "Saved optional state - details",text,more?2:1,buttons,NULL};
        if(!SDL_ShowMessageBox(&message,&choice))return -1;
        if(!more||choice!=1)return 0;
        offset+=count;
    }
    return 0;
}
static int native_confirm(const char *warning)
{
    fprintf(stderr,"p4pc: saved-state warning: %s\n",warning);
    if(p4_packages_automation_requested())return 0;
    size_t length=strlen(warning),preview=warning_page(warning,length);
    if(!preview)return 0;
    char text[1536];
    snprintf(text,sizeof text,"%.*s%s\n\nContinuing may change game behavior. Unavailable optional extension bytes remain preserved. Details lists every warning.",
        (int)preview,warning,preview<length?"\nMore warnings are available in Details.":"");
    const SDL_MessageBoxButtonData buttons[]={
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Cancel load"},
        {0,1,"Continue with optional state unavailable"},
        {0,2,"Details"}
    };
    const SDL_MessageBoxData message={SDL_MESSAGEBOX_WARNING,NULL,
        "Saved optional state or sources are unavailable",text,3,buttons,NULL};
    for(;;){
        int choice=0;
        if(!SDL_ShowMessageBox(&message,&choice))return 0;
        if(choice!=2)return choice==1;
        if(warning_details(warning,length)){
            fprintf(stderr,"p4pc: cannot display complete saved-state warnings; load cancelled\n");
            return 0;
        }
    }
}
void p4_mod_save_native_init(void)
{
    p4_mod_save_transport_ui(native_confirm,native_error);
}
int p4_mod_save_restore_begin(int32_t version,const void *payload,int32_t size)
{
    restore_error[0]=0;
    if(size<0){snprintf(restore_error,sizeof restore_error,"Negative full-save restore length");return -1;}
    return p4_mod_save_transport_restore(version,payload,(size_t)size,restore_error,sizeof restore_error);
}
void p4_mod_save_restore_commit(void)
{
    p4_mod_save_transport_restore_commit();
}
int p4_mod_save_new_game_begin(void)
{
    restore_error[0]=0;
    return p4_mod_save_transport_reset(restore_error,sizeof restore_error);
}
void p4_mod_save_new_game_commit(void)
{
    p4_mod_save_transport_reset_commit();
}
_Noreturn void p4_mod_save_restore_failed(void)
{
    p4_package_boot_runtime_failed(restore_error[0]?restore_error:"Native save transaction invariant failed");
}
