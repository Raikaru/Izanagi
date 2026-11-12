/* Real INI writer/policy; synthetic settings only, no native UI/game claim. */
#include "../src/platform/mods/failure_native.h"
#include "../src/platform/config/p4cfg.h"
#include "../src/platform/mods/package_boot_internal.h"
#include "p4pc/package_boot.h"
#include "p4pc/mods.h"
#include <stdlib.h>
#include "p4pc/os.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"recovery config:%d: %s\n",__LINE__,#x);++failures;}} while(0)
static int health_case(const char *mode)
{
    p4_os_setenv("P4PC_MODS","1",1);p4_os_setenv("P4PC_PACKS","1",1);
    p4_os_setenv("P4PC_MOD_WITNESS","1",1);
    printf("synthetic startup-health fixture mod_witness=true reference_eligible=false\n");
    if(!strcmp(mode,"health-disabled")||!strcmp(mode,"health-noop")||!strcmp(mode,"health-off-busy")){
        p4_os_setenv("P4PC_MODS","0",1);p4_os_setenv("P4PC_PACKS","0",1);
    }
    if(!strcmp(mode,"health-write-refusal")){
        p4cfg_parse_text("[mods]\nenabled=broken.mod\n[packs]\nenabled=broken.pack\n");
        CHECK(p4_package_boot_health_begin()<0&&p4_package_startup_blocked);
        CHECK(!p4_mods_stage("this-directory-must-not-be-read","broken.mod"));
        CHECK(!p4_mods_count()&&!p4_package_boot_waiting_stable);
        CHECK(!strcmp(p4cfg_str("mods.enabled",""),"broken.mod"));
        return failures?1:0;
    }
    p4cfg_load();
    if(!strcmp(mode,"health-cached")){
        puts("cache-read");fflush(stdout);
        if(fgetc(stdin)==EOF)return 2;
    }
    if(!strcmp(mode,"health-notice")){
        CHECK(!p4_package_boot_health_begin());
        const char *notice=p4_package_boot_health_notice();
        CHECK(notice&&strstr(notice,"disabled every mod"));
        CHECK(!p4_package_boot_health_notice_complete(1)&&!p4_package_boot_health_notice());
        return failures?1:0;
    }
    int result=p4_package_boot_health_begin();
    if(!strcmp(mode,"health-hold")){
        CHECK(result==0&&p4_package_boot_waiting_stable);
        if(failures)return 1;
        puts("lease-held");fflush(stdout);
        if(fgetc(stdin)==EOF)return 2;
        p4_package_boot_clean_exit();
    }else if(!strcmp(mode,"health-busy")){
        CHECK(result==-2&&p4_package_startup_blocked&&!p4_package_boot_waiting_stable);
        CHECK(p4cfg_int("mods.boot_pending",-1)==1&&p4cfg_int("mods.boot_early_crashes",-1)==0);
        CHECK(!p4_mods_stage("this-directory-must-not-be-read","broken.mod")&&!p4_mods_count());
    }else if(!strcmp(mode,"health-off-busy")){
        CHECK(result==0&&!p4_package_startup_blocked&&!p4_package_boot_waiting_stable);
        CHECK(p4cfg_int("mods.boot_pending",-1)==1&&p4cfg_int("mods.boot_early_crashes",-1)==0);
    }else if(!strcmp(mode,"health-cached")){
        CHECK(result==0&&p4_package_boot_waiting_stable&&p4cfg_int("mods.boot_early_crashes",-1)==0);
        p4_package_boot_clean_exit();
    }else if(!strcmp(mode,"health-loop")){
        CHECK(result==1&&!p4_package_boot_waiting_stable);
        CHECK(p4cfg_int("mods.boot_pending",-1)==0&&p4cfg_int("mods.boot_early_crashes",-1)==2);
    }else if(!strcmp(mode,"health-crash")){
        CHECK(result==0&&p4_package_boot_waiting_stable&&p4cfg_int("mods.boot_pending",-1)==1);
        if(failures)return 1;
        fflush(stdout);_Exit(23); /* Genuine process death before the milestone. */
    }else if(!strcmp(mode,"health-stable")||!strcmp(mode,"health-stable-crash")){
        CHECK(result==0&&p4_package_boot_waiting_stable);
        p4_package_boot_stable();
        CHECK(!p4_package_boot_waiting_stable&&p4cfg_int("mods.boot_pending",-1)==0&&p4cfg_int("mods.boot_early_crashes",-1)==0);
        if(!strcmp(mode,"health-stable-crash")&&!failures){fflush(stdout);_Exit(23);}
    }else if(!strcmp(mode,"health-normal")){
        CHECK(result==0&&p4_package_boot_waiting_stable);p4_package_boot_clean_exit();
        CHECK(!p4_package_boot_waiting_stable&&p4cfg_int("mods.boot_pending",-1)==0&&p4cfg_int("mods.boot_early_crashes",-1)==0);
    }else if(!strcmp(mode,"health-empty")){
        CHECK(result==0&&p4_package_boot_waiting_stable);p4_package_boot_admission_complete();
        CHECK(!p4_package_boot_waiting_stable&&p4cfg_int("mods.boot_pending",-1)==0);
    }else if(!strcmp(mode,"health-disabled")||!strcmp(mode,"health-noop")){
        CHECK(result==0&&!p4_package_boot_waiting_stable);
        CHECK(!p4cfg_int("mods.boot_pending",0)&&!p4cfg_int("mods.boot_early_crashes",0));
    }else return 2;
    return failures?1:0;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    p4_os_setenv("P4PC_INPUT","script",1);p4_os_setenv("P4PC_CONFIG",argv[2],1);
    if(!strncmp(argv[1],"health-",7))return health_case(argv[1]);
    if(!strcmp(argv[1],"none")){
        p4cfg_parse_text("[mods]\nenabled=keep.first,broken.mod,keep.last\n[packs]\nenabled=pack.keep,broken.pack\n");
        CHECK(p4_package_recovery_persist(P4_PACKAGE_MOD,"broken.mod",1)==-1);
        CHECK(p4_package_recovery_safe_mode(0)==-1&&p4_package_recovery_quit(0)==-1);
        CHECK(!strcmp(p4cfg_str("mods.enabled",""),"keep.first,broken.mod,keep.last"));
        CHECK(!strcmp(p4cfg_str("packs.enabled",""),"pack.keep,broken.pack"));
        CHECK(p4cfg_get("mods.recovery_choice")==NULL);
    }else{
        p4cfg_load();
        int safe=!strcmp(argv[1],"safe")||!strcmp(argv[1],"safe-loop");
        int unknown=!strcmp(argv[1],"unknown-quit")||!strcmp(argv[1],"loop-quit");
        if(safe){
            int loop=!strcmp(argv[1],"safe-loop");
            CHECK(!p4_package_recovery_safe_mode(loop));
            CHECK(p4cfg_get("mods.enabled")&&!strcmp(p4cfg_get("mods.enabled"),"")&&p4cfg_get("packs.enabled")&&!strcmp(p4cfg_get("packs.enabled"),""));
            CHECK(!strcmp(p4cfg_str("mods.recovery_kind",""),"safe-mode"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_mod",""),"all"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_choice",""),"restart"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_cause",""),loop?"startup-crash-loop":"unattributed-runtime"));
            CHECK(p4cfg_int("mods.recovery_notice",0)==1&&!p4cfg_int("mods.boot_pending",-1)&&!p4cfg_int("mods.boot_early_crashes",-1));
        }else if(unknown){
            int loop=!strcmp(argv[1],"loop-quit");
            CHECK(!p4_package_recovery_quit(loop));
            CHECK(!strcmp(p4cfg_str("mods.enabled",""),"keep.first, broken.mod, keep.last"));
            CHECK(!strcmp(p4cfg_str("packs.enabled",""),"pack.keep, broken.pack"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_kind",""),"unattributed"));
            CHECK(p4cfg_get("mods.recovery_mod")&&!strcmp(p4cfg_get("mods.recovery_mod"),""));
            CHECK(!strcmp(p4cfg_str("mods.recovery_choice",""),"quit"));
            CHECK(!p4cfg_int("mods.boot_pending",-1)&&p4cfg_int("mods.boot_early_crashes",-1)==(loop?2:0));
        }else{
            int pack=!strcmp(argv[1],"pack"),restart=strcmp(argv[1],"quit")!=0;
            const char *id=pack?"broken.pack":!strcmp(argv[1],"absent")?"gone.mod":"broken.mod";
            CHECK(p4_package_recovery_persist(P4_PACKAGE_MOD,NULL,1)==-1);
            CHECK(!p4_package_recovery_persist(pack?P4_PACKAGE_ENGINE_PACK:P4_PACKAGE_MOD,id,restart));
            const char *mods=pack?"keep.first, broken.mod, keep.last":!strcmp(argv[1],"absent")?"keep.first,broken.mod,keep.last":"keep.first,keep.last";
            CHECK(!strcmp(p4cfg_str("mods.enabled",""),mods));
            CHECK(!strcmp(p4cfg_str("packs.enabled",""),pack?"pack.keep":"pack.keep, broken.pack"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_choice",""),restart?"restart":"quit"));
            CHECK(!strcmp(p4cfg_str("mods.recovery_mod",""),id));
            CHECK(!strcmp(p4cfg_str("mods.recovery_kind",""),pack?"engine-pack":"mod"));
        }
        CHECK(!strcmp(p4cfg_str("unrelated.keep",""),"untouched"));
    }
    printf("recovery config %s: %s; synthetic=true native_game=false reference_eligible=false\n",argv[1],failures?"FAIL":"PASS");
    return failures?1:0;
}
