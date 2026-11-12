#include "p4pc/save_import.h"
#include "p4pc/os.h"
#include "../config/p4cfg.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int p4save_default_card(char *card,size_t cap) {
    char root[2048]; const char *env=getenv("P4PC_SAVE_DIR");
    if(env && *env) {
        if(strlen(env)>=sizeof root) return -1;
        strcpy(root,env);
    } else {
        const char *cfg=p4cfg_path();
        const char *explicit_cfg=getenv("P4PC_CONFIG");
        if(!cfg && explicit_cfg && strcmp(explicit_cfg,"none")) cfg=explicit_cfg;
        if(cfg) {
            if(strlen(cfg)>=sizeof root) return -1;
            strcpy(root,cfg); char *a=strrchr(root,'/'),*b=strrchr(root,'\\'); if(!a || (b && b>a)) a=b;
            if(a) a[1]=0; else root[0]=0;
        } else {
            char *pref=SDL_GetPrefPath("p4pc","Persona4"); if(!pref) return -1;
            int n=snprintf(root,sizeof root,"%s",pref); SDL_free(pref); if(n<0 || (size_t)n>=sizeof root) return -1;
        }
        if(strlen(root)+5>=sizeof root) return -1;
        strcat(root,"save");
    }
    if(p4_os_extended_path(root,sizeof root) || (!p4_os_is_dir_nofollow(root) && p4_os_mkdir(root))) return -1;
    int n=snprintf(card,cap,"%s/mc0",root); return n<0 || (size_t)n>=cap ? -1:0;
}
int p4save_import_cli(int argc,char **argv) {
    if(argc<2 || strcmp(argv[1],"--import-save")) return -1;
    int slot=-1; char card[2048],message[2304];
    if(argc!=3 && argc!=5) { fprintf(stderr,"Usage: p4pc --import-save FILE_OR_FOLDER [--slot N]\nSlots are zero-based (0..15); --slot explicitly replaces that slot.\n"); return 2; }
    if(argc==5) {
        char *end; long value=strtol(argv[4],&end,10);
        if(strcmp(argv[3],"--slot") || !*argv[4] || *end || value<0 || value>15) { fprintf(stderr,"p4pc: --slot must be 0 through 15\n"); return 2; }
        slot=(int)value;
    }
    if(p4save_default_card(card,sizeof card)) { fprintf(stderr,"p4pc: cannot locate/create save folder\n"); return 1; }
    int rc=p4save_import(argv[2],card,slot,message,sizeof message);
    fprintf(stderr,"p4pc: %s\n",message); return rc?1:0;
}
