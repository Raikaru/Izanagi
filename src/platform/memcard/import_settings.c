#include "p4pc/save_import.h"
#include "p4pc/settings.h"
#include "p4pc/os.h"
#include <SDL3/SDL.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

int p4save_default_card(char *,size_t);
static _Atomic(char *) selected;
static atomic_int pending;
static char picker_error[192];
#ifndef __ANDROID__
static int choose_folder;
#endif
void p4save_import_dialog_result(void *user,const char *const *files,int filter) {
    (void)user; (void)filter;
    if(!files) {
        snprintf(picker_error,sizeof picker_error,"Cannot choose save: %s",SDL_GetError());
        atomic_store(&pending,3);
        return;
    }
    char *p=files[0]?SDL_strdup(files[0]):NULL;
    if(files[0] && !p) {
        snprintf(picker_error,sizeof picker_error,"Out of memory choosing save");
        atomic_store(&pending,3);
        return;
    }
    atomic_store(&selected,p); atomic_store(&pending,2);
}
static void value(char *out,size_t cap,void *user) {
    (void)user;
    int paused=p4_settings_active();
    if(paused && atomic_load(&pending)==3) {
        p4_settings_message(picker_error);
        atomic_store(&pending,0);
    }
    if(paused && atomic_load(&pending)==2) {
        char *p=atomic_exchange(&selected,NULL); atomic_store(&pending,0);
        if(p) {
            char card[2048],msg[2304];
            if(p4save_default_card(card,sizeof card)) p4_settings_message("Cannot locate save folder");
            else {
                int rc=p4save_import(p,card,-1,msg,sizeof msg);
                const char *slots=strstr(msg,"; slots");
                if(!rc && slots) {
                    char concise[128];
                    snprintf(concise,sizeof concise,"Imported%s. Use LOAD GAME.",slots+1);
                    p4_settings_message(concise);
                } else p4_settings_message(msg);
            }
            SDL_free(p);
        }
    }
#ifdef __ANDROID__
    snprintf(out,cap,"Read files/import");
#else
    snprintf(out,cap,"%s",atomic_load(&pending)?"Choosing...":choose_folder?"Choose folder":"Choose file");
#endif
}
#ifdef __ANDROID__
/* Android/data is not accessible to adb/MTP on Android 11+. The app-owned
 * media directory is, and needs no storage permission. Ask Android for its
 * actual volume path rather than hardcoding /sdcard or a package name. */
static int android_files_dir(char *out,size_t cap) {
    JNIEnv *env=SDL_GetAndroidJNIEnv();
    if(!env) return -1;
    if((*env)->PushLocalFrame(env,8)<0) { (*env)->ExceptionClear(env); return -1; }
    int rc=-1;
    jobject activity=SDL_GetAndroidActivity();
    if(!activity) goto done;
    jclass activity_class=(*env)->GetObjectClass(env,activity);
    if(!activity_class) goto done;
    jmethodID media=(*env)->GetMethodID(env,activity_class,"getExternalMediaDirs","()[Ljava/io/File;");
    if(!media) goto done;
    jobjectArray dirs=(*env)->CallObjectMethod(env,activity,media);
    if(!dirs || (*env)->ExceptionCheck(env)) goto done;
    jobject dir=NULL;
    for(jsize i=0,n=(*env)->GetArrayLength(env,dirs);i<n && !dir;i++) {
        dir=(*env)->GetObjectArrayElement(env,dirs,i);
        if((*env)->ExceptionCheck(env)) goto done;
    }
    if(!dir) goto done;
    jclass file_class=(*env)->GetObjectClass(env,dir);
    if(!file_class) goto done;
    jmethodID absolute=(*env)->GetMethodID(env,file_class,"getAbsolutePath","()Ljava/lang/String;");
    if(!absolute) goto done;
    jstring text=(*env)->CallObjectMethod(env,dir,absolute);
    if(!text || (*env)->ExceptionCheck(env)) goto done;
    const char *path=(*env)->GetStringUTFChars(env,text,NULL);
    if(!path) goto done;
    int n=snprintf(out,cap,"%s/files",path);
    (*env)->ReleaseStringUTFChars(env,text,path);
    if(n>0 && (size_t)n<cap) rc=0;
done:
    if((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env,NULL);
    return rc;
}
typedef struct { const char *dir; char card[2048]; int count; char message[2304]; } AndroidImport;
static void android_file(const char *name,void *user) {
    AndroidImport *c=user;
    if(!strcmp(name,".") || !strcmp(name,"..") || !strncmp(name,"_pcsx2_",7)) return;
    char path[2048]; int n=snprintf(path,sizeof path,"%s/%s",c->dir,name); if(n<0 || (size_t)n>=sizeof path) return;
    const char *dot=strrchr(name,'.');
    if(!p4_os_is_dir_nofollow(path) && (!dot || (SDL_strcasecmp(dot,".psv") && SDL_strcasecmp(dot,".psu") && SDL_strcasecmp(dot,".max") && SDL_strcasecmp(dot,".cbs") && SDL_strcasecmp(dot,".xps") && SDL_strcasecmp(dot,".sps") && SDL_strcasecmp(dot,".ps2")))) return;
    if(!p4save_import(path,c->card,-1,c->message,sizeof c->message)) c->count++;
}
#endif
static void activate(int direction,void *user) {
    (void)user; if(atomic_load(&pending)) return;
#ifdef __ANDROID__
    if(direction) return;
    char base[2048],dir[2048];
    if(android_files_dir(base,sizeof base)) { p4_settings_message("Cannot locate app media files"); return; }
    if(!p4_os_is_dir_nofollow(base) && p4_os_mkdir(base)) { p4_settings_message("Cannot create app media files"); return; }
    int n=snprintf(dir,sizeof dir,"%s/import",base); if(n<0 || (size_t)n>=sizeof dir) return;
    if(!p4_os_is_dir_nofollow(dir)) {
        if(p4_os_mkdir(dir)) p4_settings_message("Cannot create app files/import");
        else p4_settings_message("Place saves in app files/import, then activate Import save");
        return;
    }
    AndroidImport c={0}; c.dir=dir;
    if(p4save_default_card(c.card,sizeof c.card) || p4_os_list_dir(dir,android_file,&c)) p4_settings_message("Cannot read app files/import");
    else if(c.count) { char msg[128]; snprintf(msg,sizeof msg,"Imported %d container(s). Use LOAD GAME.",c.count); p4_settings_message(msg); }
    else p4_settings_message(c.message[0]?c.message:"No supported saves in app files/import");
#else
    if(direction) { choose_folder=!choose_folder; return; }
    static const SDL_DialogFileFilter filters[]={
        {"PS2 saves","psv;PSV;psu;PSU;max;MAX;cbs;CBS;sps;SPS;xps;XPS;ps2;PS2"},
        {"All files","*"}
    };
    atomic_store(&pending,1);
    if(choose_folder) SDL_ShowOpenFolderDialog(p4save_import_dialog_result,NULL,NULL,NULL,false);
    else SDL_ShowOpenFileDialog(p4save_import_dialog_result,NULL,NULL,filters,2,NULL,false);
#endif
}
void p4save_import_settings_init(void) {
    static int initialized; if(initialized) return; initialized=1;
    static const P4SettingsEntry entry = {
        .label = "Import save", .format = value, .adjust = activate,
        .flags = P4_SETTINGS_ACTION, .description = "Confirm: import a save."
    };
    p4_settings_register(&entry);
}
