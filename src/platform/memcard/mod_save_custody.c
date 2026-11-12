#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "mod_save_custody.h"
#include "save_transaction.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fail(char *error,size_t cap,const char *text)
{
    if(cap)snprintf(error,cap,"%s",text);
    return -1;
}
int p4_save_witness_mode(char *error,size_t cap)
{
    const char *mode=getenv("P4PC_MOD_WITNESS");
    if(!mode)return 0;
    const char *input=getenv("P4PC_INPUT");
    if(strcmp(mode,"1")||!input||strcmp(input,"script"))
        return fail(error,cap,"Save mod-witness requires exactly P4PC_MOD_WITNESS=1 and P4PC_INPUT=script");
    return 1;
}
int p4_save_witness_name(const char *name)
{
    size_t n=strlen(name),suffix=sizeof P4_SAVE_MODE_SUFFIX-1;
    return !strcmp(name,P4_SAVE_MODE_MARKER)||(n>=suffix&&!strcmp(name+n-suffix,P4_SAVE_MODE_SUFFIX));
}
static int separator(char c)
{
#ifdef _WIN32
    return c=='/'||c=='\\';
#else
    return c=='/';
#endif
}
static size_t root_length(const char *path)
{
#ifdef _WIN32
    size_t n=strlen(path);
    if(n>=8&&!memcmp(path,"\\\\?\\UNC\\",8)){
        size_t at=8;while(path[at]&&!separator(path[at]))++at;
        if(path[at])++at;
        while(path[at]&&!separator(path[at]))++at;
        return at;
    }
    if(n>=7&&!memcmp(path,"\\\\?\\",4)&&path[5]==':')return 7;
    if(n>=4&&!memcmp(path,"\\\\?\\",4)){
        size_t at=4;while(path[at]&&!separator(path[at]))++at;
        return path[at]?at+1:at;
    }
    return n>=3&&path[1]==':'?3:0;
#else
    return path[0]=='/'?1:0;
#endif
}
static int absolute(char out[P4_SAVE_PATH_CAP],const char *path)
{
#ifdef _WIN32
    if(strlen(path)>=P4_SAVE_PATH_CAP)return -1;
    strcpy(out,path);return p4_os_extended_path(out,P4_SAVE_PATH_CAP);
#else
    if(path[0]=='/'){
        if(strlen(path)>=P4_SAVE_PATH_CAP)return -1;
        strcpy(out,path);
    }else{
        if(!getcwd(out,P4_SAVE_PATH_CAP))return -1;
        size_t n=strlen(out),m=strlen(path);
        if(n+1+m>=P4_SAVE_PATH_CAP)return -1;
        out[n++]='/';memcpy(out+n,path,m+1);
    }
    return 0; /* Keep named components; resolving ".." across a link is OS work. */
#endif
}
static int parent(char *path,size_t root)
{
    size_t n=strlen(path);
    while(n>root&&separator(path[n-1]))path[--n]=0;
    if(n<=root)return 0;
    while(n>root&&!separator(path[n-1]))--n;
    while(n>root&&separator(path[n-1]))--n;
    path[n]=0;return 1;
}
#ifdef _WIN32
typedef struct Probe { const char *name;int exists; } Probe;
static void find(const char *name,void *user)
{
    Probe *p=user;if(!strcmp(p->name,name))p->exists=1;
}
#endif
static int marker(const char *path)
{
#ifndef _WIN32
    struct stat info;
    if(lstat(path,&info)==0)return S_ISREG(info.st_mode)?1:-1;
    if(errno!=ENOENT)return -1;
#else
    if(p4_os_is_file_nofollow(path))return 1;
#endif
    char directory[P4_SAVE_PATH_CAP];size_t n=strlen(path);
    if(n>=sizeof directory)return -1;
    memcpy(directory,path,n+1);
    size_t at=n;while(at&&!separator(directory[at-1]))--at;
    if(!at)return -1;
#ifndef _WIN32
    /* ENOENT can also mean a missing parent. Require a traversable parent,
     * without requiring read permission to enumerate execute-only directories. */
#else
    Probe probe={.name=path+at};
#endif
    size_t root=root_length(directory);
    if(at>root)directory[at-1]=0;else directory[root]=0;
#ifndef _WIN32
    return stat(directory,&info)==0&&S_ISDIR(info.st_mode)?0:-1;
#else
    if(p4_os_list_dir(directory,find,&probe)||probe.exists)return -1;
    return 0;
#endif
}
static int ancestry(char *current,int directory,char *error,size_t cap)
{
    char tag[P4_SAVE_PATH_CAP];
    size_t root=root_length(current);
    if(!root)return fail(error,cap,"Save input custody has no absolute root");
    for(unsigned depth=0;depth<128;depth++){
        int n=snprintf(tag,sizeof tag,"%s%s",current,P4_SAVE_MODE_SUFFIX);
        if(n<0||(size_t)n>=sizeof tag||p4_os_extended_path(tag,sizeof tag))return fail(error,cap,"Save custody path is too long");
        int present=marker(tag);
        if(present<0)return fail(error,cap,"Cannot inspect portable save custody marker");
        if(present)return 1;
        if(!directory){directory=1;if(!parent(current,root))return 0;continue;}
        n=snprintf(tag,sizeof tag,"%s/%s",current,P4_SAVE_MODE_MARKER);
        if(n<0||(size_t)n>=sizeof tag||p4_os_extended_path(tag,sizeof tag))return fail(error,cap,"Save custody path is too long");
        present=marker(tag);
        if(present<0)return fail(error,cap,"Cannot inspect ancestor save custody marker");
        if(present)return 1;
        if(!parent(current,root))return 0;
    }
    return fail(error,cap,"Save input custody ancestry exceeds supported depth");
}
int p4_save_witness_input(const char *path,int directory,char *error,size_t cap)
{
    if(!strncmp(path,"fd:",3))return 0; /* Content URI has no host sibling path. */
    char named[P4_SAVE_PATH_CAP],resolved[P4_SAVE_PATH_CAP];
    if(absolute(named,path))return fail(error,cap,"Cannot resolve save input custody path");
    size_t root=root_length(named),length=strlen(named);
    while(length>root&&separator(named[length-1]))named[--length]=0;
    if(p4_os_resolved_path(path,resolved,sizeof resolved)){
        /* The old generation can lack its data file. Resolve its REAL parent,
         * not a guessed final path, and preserve the missing basename. */
        if(directory||errno!=ENOENT)return fail(error,cap,"Cannot inspect resolved save custody scope");
        strcpy(resolved,named);
        const char *file=named+strlen(named);
        while(file>named&&!separator(file[-1]))--file;
        if(!*file||!parent(resolved,root)||p4_os_resolved_path(resolved,resolved,sizeof resolved))
            return fail(error,cap,"Cannot inspect save custody parent");
        size_t n=strlen(resolved),m=strlen(file);
        if(n+1+m>=sizeof resolved)return fail(error,cap,"Resolved save custody path is too long");
        resolved[n++]='/';memcpy(resolved+n,file,m+1);
        if(p4_os_extended_path(resolved,sizeof resolved))return fail(error,cap,"Resolved save custody path is invalid");
    }
    int same=!strcmp(named,resolved);
    int result=ancestry(named,directory,error,cap);
    return result||same?result:ancestry(resolved,directory,error,cap);
}
int p4_save_witness_mark(const char *directory,char *error,size_t cap)
{
    static const char json[]={"{\"schema\":1,\"run_mode\":\"mod-witness\",\"mod_witness\":true,\"reference_eligible\":false}\n"};
    char target[P4_SAVE_PATH_CAP],temporary[P4_SAVE_PATH_CAP];
    int a=snprintf(target,sizeof target,"%s/%s",directory,P4_SAVE_MODE_MARKER);
    int b=snprintf(temporary,sizeof temporary,"%s/.p4pc-mode-write.tmp",directory);
    if(a<0||(size_t)a>=sizeof target||b<0||(size_t)b>=sizeof temporary||
       p4_os_extended_path(target,sizeof target)||p4_os_extended_path(temporary,sizeof temporary))
        return fail(error,cap,"Save witness marker path is too long");
    FILE *file=fopen(temporary,"wbx");
    if(!file)return fail(error,cap,"Cannot create save witness marker");
    int bad=fwrite(json,1,sizeof json-1,file)!=sizeof json-1;
    if(fclose(file))bad=1;
    if(!bad&&p4_os_replace_file(temporary,target))bad=1;
    if(bad){remove(temporary);return fail(error,cap,"Cannot publish non-reference save custody marker");}
    return 0;
}
