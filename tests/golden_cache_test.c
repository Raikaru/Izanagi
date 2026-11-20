#include "p4pc/golden_cache.h"
#include "p4pc/content_digest.h"
#include "p4pc/os.h"
#include "golden_cache_fixture.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
static void cleanup_entry(const char *name, void *user)
{
    const char *directory = user;
    char path[1600];
    if (!strcmp(name,".") || !strcmp(name,"..")) return;
    int n = snprintf(path,sizeof path,"%s/%s",directory,name);
    assert(n > 0 && (size_t)n < sizeof path);
    if (p4_os_is_dir_nofollow(path)) {
        assert(!p4_os_list_dir_utf8(path,cleanup_entry,path));
        assert(!p4_os_rmdir(path));
    } else {
        int result = p4_os_remove_utf8(path);
        if (result) fprintf(stderr,"cannot remove test file %s (errno %d)\n",path,errno);
        assert(!result);
    }
}


int main(int argc, char **argv)
{
    char root[1024], pref[1100], install[1100], program[1100], repo[1100], path[1400];
    char key[65], old_key[65], digest[65], reason[256] = "";
    (void)argv;
    assert(argc == 1);
    assert(!p4_os_fopen_utf8(NULL,"rb") && !p4_os_fopen_utf8("unused","invalid"));
    assert(p4_os_mkdir_utf8(NULL) == -1 && p4_os_remove_utf8(NULL) == -1);
    assert(p4_os_list_dir_utf8(NULL,cleanup_entry,NULL) == -1);
    const char *temp = golden_cache_fixture_temp();
    if (!temp) return 1;
    int root_len = snprintf(root,sizeof root,"%s/golden-cache-test-%llu",temp,(unsigned long long)p4_os_now_ns());
    assert(root_len > 0 && (size_t)root_len < sizeof root);
    assert(!p4_os_mkdir(root));
    assert(p4_os_list_dir_utf8(root,NULL,NULL) == -1);
    snprintf(pref,sizeof pref,"%s/pref",root); assert(!p4_os_mkdir(pref));
    snprintf(install,sizeof install,"%s/install",root); assert(!p4_os_mkdir(install));
    snprintf(program,sizeof program,"%s/program",root); assert(!p4_os_mkdir(program));
    snprintf(repo,sizeof repo,"%s/repo",root); assert(!p4_os_mkdir(repo));
    P4GoldenCachePaths paths = {pref,install,program,repo};
    P4GoldenCacheIdentityEntry sources[] = {{"P4G.exe","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}};
    P4GoldenCacheIdentityEntry versions[] = {{"schema","1"},{"skill@converter","2"}};
    P4GoldenCacheIdentity identity = {sources,1,versions,2};
    uint8_t one[] = {1,2,3}, two[] = {4,5};
    P4GoldenCacheTable output[] = {{"item",one,sizeof one},{"skill",two,sizeof two}};
    P4GoldenCacheTable input[] = {{"item",NULL,0},{"skill",NULL,0}};
    int load_result = p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason);
    if (load_result != 1) {
        fprintf(stderr,"initial cache load: result=%d reason=%s pref=%s\n",load_result,reason,pref);
        return 1;
    }
    if (p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason)) {
        fprintf(stderr,"cache store: %s\n",reason); return 1;
    }
    strcpy(old_key,key);
    assert(!p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason));
    assert(input[0].size == sizeof one && !memcmp(input[0].data,one,sizeof one));
    assert(input[1].size == sizeof two && !memcmp(input[1].data,two,sizeof two));
    p4_golden_cache_free(input,2);
    p4_content_digest(one,sizeof one,digest);
    snprintf(path,sizeof path,"%s/golden-cache/%s/objects/%s",pref,key,digest);
    FILE *f = fopen(path,"wb"); assert(f); assert(fwrite("x",1,1,f) == 1); assert(!fclose(f));
    assert(p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason) == 1);
    assert(!input[0].data && !input[1].data);
    assert(!p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason));
    assert(!p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason));
    p4_golden_cache_free(input,2);
    snprintf(path,sizeof path,"%s/golden-cache/%s/tables-index.json",pref,key);
    f = fopen(path,"ab"); assert(f); assert(fwrite(" ",1,1,f) == 1); assert(!fclose(f));
    assert(p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason) == 1);
    assert(!p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason));
    versions[0].value = "2";
    assert(p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason) == 1);
    assert(strcmp(key,old_key));
    sources[0].value = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    char version_key[65]; strcpy(version_key,key);
    assert(p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason) == 1);
    assert(strcmp(key,version_key));
    assert(!p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason));
    snprintf(path,sizeof path,"%s/golden-cache/%s",pref,old_key);
    assert(!p4_os_is_dir_nofollow(path));
    paths.pref = install; assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    paths.pref = repo; assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    paths.pref = program; assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    snprintf(path,sizeof path,"%s/.git",repo); assert(!p4_os_mkdir(path));
    snprintf(path,sizeof path,"%s/user-pref",repo); assert(!p4_os_mkdir(path));
    paths.pref = path; paths.repo = NULL;
    assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    paths.pref = pref;
    snprintf(path,sizeof path,"%s/golden-cache/install",pref); assert(!p4_os_mkdir(path));
    paths.install = path;
    assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    paths.install = install;
    input[0].data = one; input[1].data = two;
    input[1].name = "item";
    assert(p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason) == -1);
    assert(!input[0].data && !input[1].data);
    input[1].name = "skill";
    paths.pref = pref; versions[0].value = "bad\"json";
    assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    char unicode[1100];
    snprintf(unicode,sizeof unicode,"%s/pref-\xc3\xa9",root);
    assert(!p4_os_mkdir_utf8(unicode));
    paths.pref = unicode; versions[0].value = "3";
    assert(!p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason));
    strcpy(old_key,key);
    assert(!p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason));
    p4_golden_cache_free(input,2);
    versions[0].value = "4";
    assert(!p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason));
    snprintf(path,sizeof path,"%s/golden-cache/%s",unicode,old_key);
    assert(!p4_os_is_dir_nofollow(path));
    char foreign[1400], foreign_file[1500];
    snprintf(foreign,sizeof foreign,"%s/golden-cache/cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",unicode);
    assert(!p4_os_mkdir_utf8(foreign));
    snprintf(foreign_file,sizeof foreign_file,"%s/unexpected",foreign);
    f = p4_os_fopen_utf8(foreign_file,"wb"); assert(f); assert(!fclose(f));
    versions[0].value = "5";
    assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    assert(strstr(reason,"cannot prune") && p4_os_is_file_nofollow(foreign_file));
    assert(!p4_os_remove_utf8(foreign_file) && !p4_os_rmdir(foreign));
    assert(!p4_golden_cache_load(&paths,&identity,input,2,key,reason,sizeof reason));
    p4_golden_cache_free(input,2);
#ifndef _WIN32
    char linked_pref[1100], linked_cache[1200];
    snprintf(linked_pref,sizeof linked_pref,"%s/linked-pref",root); assert(!p4_os_mkdir(linked_pref));
    snprintf(linked_cache,sizeof linked_cache,"%s/golden-cache",linked_pref);
    assert(!symlink(install,linked_cache));
    paths.pref = linked_pref;
    assert(p4_golden_cache_store(&paths,&identity,output,2,key,reason,sizeof reason) == -1);
    assert(!unlink(linked_cache));
#endif
    assert(!p4_os_list_dir_utf8(root,cleanup_entry,root));
    assert(!p4_os_rmdir(root));
    return 0;
}
