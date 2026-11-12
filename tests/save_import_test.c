#include "p4pc/save_import.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
static DWORD WINAPI release_lock(void *arg) {
    Sleep(100);
    CloseHandle((HANDLE)arg);
    return 0;
}
static HANDLE lock_directory(const char *path) {
    WCHAR wide[4096];
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,4096)) return INVALID_HANDLE_VALUE;
    return CreateFileW(wide,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
}
static int path_retry_test(const char *root) {
    char base[2048],source[2048],target[2048],missing[2048];
    if(strlen(root)>=sizeof base) return 1;
    strcpy(base,root);
    if(p4_os_extended_path(base,sizeof base) || p4_os_mkdir(base)) return 1;
    if(snprintf(source,sizeof source,"%s/source",base)>=(int)sizeof source ||
       snprintf(target,sizeof target,"%s/target",base)>=(int)sizeof target ||
       snprintf(missing,sizeof missing,"%s/missing",base)>=(int)sizeof missing ||
       p4_os_extended_path(source,sizeof source) || p4_os_extended_path(target,sizeof target) ||
       p4_os_extended_path(missing,sizeof missing) || p4_os_mkdir(source)) return 1;
    HANDLE lock=lock_directory(source);
    if(lock==INVALID_HANDLE_VALUE) return 1;
    HANDLE thread=CreateThread(NULL,0,release_lock,lock,0,NULL);
    if(!thread) { CloseHandle(lock); return 1; }
    ULONGLONG begin=GetTickCount64();
    int rc=p4_os_rename(source,target);
    ULONGLONG elapsed=GetTickCount64()-begin;
    WaitForSingleObject(thread,INFINITE); CloseHandle(thread);
    if(rc || elapsed<80 || elapsed>4000) return 1;
    lock=lock_directory(target);
    if(lock==INVALID_HANDLE_VALUE) return 1;
    begin=GetTickCount64();
    rc=p4_os_rename(target,source);
    int error=errno; unsigned long native=0; _get_doserrno(&native);
    elapsed=GetTickCount64()-begin;
    CloseHandle(lock);
    if(!rc || error!=EACCES || (native!=ERROR_ACCESS_DENIED && native!=ERROR_SHARING_VIOLATION) ||
       elapsed<2000 || elapsed>5000 || !p4_os_is_dir_nofollow(target)) return 1;
    lock=lock_directory(target);
    if(lock==INVALID_HANDLE_VALUE) return 1;
    thread=CreateThread(NULL,0,release_lock,lock,0,NULL);
    if(!thread) { CloseHandle(lock); return 1; }
    rc=p4_os_rmdir(target);
    WaitForSingleObject(thread,INFINITE); CloseHandle(thread);
    if(rc) return 1;
    begin=GetTickCount64(); rc=p4_os_rename(missing,source);
    elapsed=GetTickCount64()-begin;
    if(!rc || errno!=ENOENT || elapsed>1000) return 1;
    if(p4_os_rmdir(base)) return 1;
    fprintf(stderr,"Windows transient rename/rmdir, bounded denial and non-retryable error PASS\n");
    return 0;
}
#endif

int main(int argc,char **argv) {
#ifdef _WIN32
    if(argc==3 && !strcmp(argv[1],"--path-retry")) return path_retry_test(argv[2]);
#endif
    if(argc==4 || argc==5) {
        char message[4096]; int slot=argc==5?atoi(argv[4]):-1;
        int rc=p4save_import(argv[2],argv[3],slot,message,sizeof message);
        fprintf(stderr,"%s\n",message); return rc?1:0;
    }
    fprintf(stderr,"Usage: save_import_test --import SOURCE CARD [SLOT]\n"); return 2;
}
