/* Same-card directory publication for imports and live tagged saves.
 * A journal owns only bounded private basenames; never follow slot/reparse links
 * or delete an ambiguous generation to pretend rollback succeeded. */
#include "mod_save_custody.h"
#include "save_transaction.h"
#include "mod_save_format.h"
#include "mod_save_hash.h"
#include "p4pc/os.h"
#include "p4pc/save_import.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAP P4_SAVE_PATH_CAP
#define JOURNAL_BYTES 256u
#define RETAIL_MAX_BYTES P4SAVE_MAX_BYTES
static unsigned unique_counter;
static P4SaveTransaction *active_transaction; /* callbacks/CLI run on the game thread */
enum { TX_LOCKED=1,TX_STAGED=2 };
static int fail(char *error,size_t cap,const char *format,...)
{
    if(cap){va_list args;va_start(args,format);vsnprintf(error,cap,format,args);va_end(args);}
    return -1;
}
static int join(char out[PATH_CAP],const char *root,const char *name)
{
    size_t len=strlen(root);
    const char *separator=len&&(root[len-1]=='/'||root[len-1]=='\\')?"":"/";
    int n=snprintf(out,PATH_CAP,"%s%s%s",root,separator,name);
    return n<0||(size_t)n>=PATH_CAP||p4_os_extended_path(out,PATH_CAP);
}
static uint32_t get32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const unsigned char *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static void put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
int p4_save_slot_index(const char *name)
{
    static const char prefix[]="BASLUS-21782Save";
    if(!name||strncmp(name,prefix,sizeof prefix-1))return -1;
    const char *suffix=name+sizeof prefix-1;
    if(suffix[0]>='0'&&suffix[0]<='9'&&!suffix[1])return suffix[0]-'0';
    if(suffix[0]=='1'&&suffix[1]>='0'&&suffix[1]<='5'&&!suffix[2])return 10+suffix[1]-'0';
    return -1;
}
static int owned_name(const char *name)
{
    if(!name||(strncmp(name,".p4mod-stage-",13)&&strncmp(name,".p4mod-backup-",14)))return 0;
    size_t i=0;
    for(;i<64&&name[i];i++){
        unsigned char c=(unsigned char)name[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return 0;
    }
    return i&&i<64;
}
typedef struct EntryProbe { const char *wanted;int present; } EntryProbe;
static void find_entry(const char *name,void *user)
{
    EntryProbe *probe=user;if(!strcmp(name,probe->wanted))probe->present=1;
}
static int entry_exists(const char *root,const char *name)
{
    EntryProbe probe={.wanted=name};
    if(p4_os_list_dir(root,find_entry,&probe))return -1;
    return probe.present;
}
static int stage_make(const char *card,char *name,size_t cap,char *error,size_t error_cap)
{
    if(!card||!name||cap<64||!p4_os_is_dir_nofollow(card))return fail(error,error_cap,"Invalid memory-card staging root");
    for(unsigned i=0;i<100;i++){
        int n=snprintf(name,cap,".p4mod-stage-%llu-%u",(unsigned long long)p4_os_now_ns(),unique_counter++);
        char path[PATH_CAP];
        if(n<0||(size_t)n>=cap||join(path,card,name))return fail(error,error_cap,"Save staging path is too long");
        if(!p4_os_mkdir(path))return 0;
        if(errno!=EEXIST)break;
    }
    return fail(error,error_cap,"Cannot create save staging directory: %s",strerror(errno));
}
typedef struct Remove { const char *path;int failed; } Remove;
static void remove_entry(const char *name,void *user)
{
    Remove *state=user;
    if(!strcmp(name,".")||!strcmp(name,".."))return;
    char path[PATH_CAP];
    if(join(path,state->path,name)||!p4_os_is_file_nofollow(path)||remove(path))state->failed=1;
}
static int stage_remove(const char *card,const char *name)
{
    char path[PATH_CAP];
    if(!owned_name(name)||join(path,card,name)||!p4_os_is_dir_nofollow(path))return -1;
    Remove state={.path=path};
    if(p4_os_list_dir(path,remove_entry,&state)||state.failed||p4_os_rmdir(path))return -1;
    return 0;
}
static int hash_file(const char *path,uint64_t expected,unsigned char hash[32])
{
    if(!p4_os_is_file_nofollow(path))return -1;
    FILE *file=p4_os_fopen_rb(path);
    if(!file)return -1;
    P4OSFileIdentity before,after;
    int bad=p4_os_file_identity(file,&before)||before.size!=expected;
    P4SaveHash state;p4_save_hash_init(&state);
    unsigned char buffer[16384];uint64_t total=0;
    while(!bad){
        size_t n=fread(buffer,1,sizeof buffer,file);
        if(!n)break;
        if((uint64_t)n>expected-total){bad=1;break;}
        p4_save_hash_update(&state,buffer,n);total+=n;
    }
    if(ferror(file)||total!=expected||p4_os_file_identity(file,&after)||memcmp(&before,&after,sizeof before))bad=1;
    if(fclose(file))bad=1;
    if(bad)return -1;
    p4_save_hash_final(&state,hash);return 0;
}
static int binding_valid(const P4SavePairBinding *binding)
{
    return binding&&binding->retail_size>=4100&&binding->retail_size<=RETAIL_MAX_BYTES&&
        (!binding->sidecar_size||(binding->sidecar_size>=P4_MOD_SAVE_HEADER_BYTES+32&&binding->sidecar_size<=P4_MOD_SAVE_MAX_BYTES));
}
static int verify_generation(const char *card,const char *directory,const char *slot,const P4SavePairBinding *binding,char *error,size_t cap)
{
    char root[PATH_CAP],path[PATH_CAP];unsigned char hash[32];
    if(!binding_valid(binding)||join(root,card,directory)||!p4_os_is_dir_nofollow(root)||join(path,root,slot)||
       hash_file(path,binding->retail_size,hash)||memcmp(hash,binding->retail_hash,32))
        return fail(error,cap,"Save generation's retail image is missing/changed: %s",directory);
    if(!binding->sidecar_size){
        if(entry_exists(root,P4_MOD_SAVE_SIDECAR)!=0||entry_exists(root,P4_MOD_SAVE_COMMIT)!=0)
            return fail(error,cap,"Untagged save generation contains unbound/unreadable mod metadata");
        return 0;
    }
    if(join(path,root,P4_MOD_SAVE_SIDECAR)||hash_file(path,binding->sidecar_size,hash)||memcmp(hash,binding->sidecar_hash,32))
        return fail(error,cap,"Save generation's sidecar is missing/changed: %s",directory);
    if(join(path,root,P4_MOD_SAVE_COMMIT)||!p4_os_is_file_nofollow(path))return fail(error,cap,"Save generation lacks its commit marker");
    FILE *file=p4_os_fopen_rb(path);
    if(!file)return fail(error,cap,"Cannot read mod-save commit marker");
    unsigned char marker[P4_MOD_SAVE_MARKER_BYTES];
    int bad=fread(marker,1,sizeof marker,file)!=sizeof marker;
    if(fgetc(file)!=EOF||ferror(file))bad=1;
    if(fclose(file))bad=1;
    if(bad)return fail(error,cap,"Truncated/oversized mod-save commit marker");
    return p4_save_marker_verify(binding->retail_size,binding->retail_hash,binding->sidecar_size,binding->sidecar_hash,marker,sizeof marker,error,cap);
}
int p4_save_pair_verify(const char *card,const char *slot,const P4SavePairBinding *binding,char *error,size_t cap)
{
    if(p4_save_slot_index(slot)<0)return fail(error,cap,"Invalid save slot");
    return verify_generation(card,slot,slot,binding,error,cap);
}
static int journal_name(char out[96],const char *slot)
{
    int n=snprintf(out,96,".p4mod-%s.txn",slot);return n<0||n>=96;
}
static int remove_named(const char *card,const char *name)
{
    int exists=entry_exists(card,name);
    if(exists<=0)return exists;
    char path[PATH_CAP];
    return join(path,card,name)||!p4_os_is_file_nofollow(path)||remove(path)?-1:0;
}
static int remove_records(const char *card,const char *slot)
{
    char name[96],next[104];journal_name(name,slot);
    snprintf(next,sizeof next,"%s.next",name);
    return remove_named(card,next)||remove_named(card,name)?-1:0;
}
static void journal_encode(unsigned char out[JOURNAL_BYTES],const P4SaveTransaction *t,int publishing)
{
    static const unsigned char magic[8]={'P','4','S','T','X','N',0,0};
    memset(out,0,JOURNAL_BYTES);memcpy(out,magic,sizeof magic);put32(out+8,1);
    put32(out+12,(t->old?1u:0u)|(t->binding.sidecar_size?2u:0u)|(publishing?4u:0u));
    strcpy((char *)out+16,t->stage);strcpy((char *)out+80,t->backup);
    memcpy(out+144,t->binding.retail_hash,32);memcpy(out+176,t->binding.sidecar_hash,32);
    put64(out+208,t->binding.retail_size);put64(out+216,t->binding.sidecar_size);p4_save_hash(out,224,out+224);
}
static int journal_decode(const unsigned char in[JOURNAL_BYTES],P4SaveTransaction *t)
{
    static const unsigned char magic[8]={'P','4','S','T','X','N',0,0};
    unsigned char hash[32];p4_save_hash(in,224,hash);
    if(memcmp(in,magic,sizeof magic)||get32(in+8)!=1||(get32(in+12)&~7u)||memcmp(hash,in+224,32)||
       !memchr(in+16,0,64)||!memchr(in+80,0,64))return -1;
    memcpy(t->stage,in+16,64);memcpy(t->backup,in+80,64);
    if(strncmp(t->stage,".p4mod-stage-",13)||!owned_name(t->stage)||
       strncmp(t->backup,".p4mod-backup-",14)||!owned_name(t->backup)||!strcmp(t->stage,t->backup))return -1;
    memset(&t->binding,0,sizeof t->binding);
    memcpy(t->binding.retail_hash,in+144,32);memcpy(t->binding.sidecar_hash,in+176,32);
    t->binding.retail_size=get64(in+208);t->binding.sidecar_size=get64(in+216);
    t->old=(get32(in+12)&1u)!=0;t->publishing=(get32(in+12)&4u)!=0;
    return !binding_valid(&t->binding)||!!t->binding.sidecar_size!=!!(get32(in+12)&2u)?-1:0;
}
static int journal_write(P4SaveTransaction *t,int initial,int publishing)
{
    char name[96],next[104],path[PATH_CAP],next_path[PATH_CAP];
    if(journal_name(name,t->name)||join(path,t->card,name))return -1;
    snprintf(next,sizeof next,"%s.next",name);
    if(join(next_path,t->card,next))return -1;
    if(!initial&&remove_named(t->card,next))return -1;
    unsigned char bytes[JOURNAL_BYTES];journal_encode(bytes,t,publishing);
    FILE *file=fopen(initial?path:next_path,"wbx");
    if(!file)return -1;
    if(initial)t->journal=1; /* even partial bytes are ours, not another writer */
    int bad=fwrite(bytes,1,sizeof bytes,file)!=sizeof bytes;
    if(fclose(file))bad=1;
    if(bad)return -1;
    if(!initial&&p4_os_replace_file(next_path,path))return -1;
    t->publishing=(unsigned)publishing;return 0;
}
static int discard_staging(P4SaveTransaction *t)
{
    int exists=entry_exists(t->card,t->stage);
    if(exists<0||(exists&&stage_remove(t->card,t->stage)))return -1;
    return remove_records(t->card,t->name);
}
static int cleanup_committed(P4SaveTransaction *t)
{
    int exists=entry_exists(t->card,t->backup);
    if(exists<0||(exists&&stage_remove(t->card,t->backup)))return -1;
    return discard_staging(t);
}
static int recover_one(const char *card,const char *slot,char *error,size_t cap)
{
    char journal[96],path[PATH_CAP],destination[PATH_CAP],stage_path[PATH_CAP],backup_path[PATH_CAP];
    if(journal_name(journal,slot)||join(path,card,journal))
        return fail(error,cap,"Invalid save recovery journal path");
    if(!p4_os_is_file_nofollow(path))
        return fail(error,cap,"Invalid save recovery journal: %s",path);
    FILE *file=p4_os_fopen_rb(path);
    if(!file)return fail(error,cap,"Cannot read save recovery journal");
    unsigned char bytes[JOURNAL_BYTES];P4SaveTransaction t={.card=card,.journal=1};
    strcpy(t.name,slot);
    int bad=fread(bytes,1,sizeof bytes,file)!=sizeof bytes;
    if(fgetc(file)!=EOF||ferror(file))bad=1;
    if(fclose(file))bad=1;
    if(bad||journal_decode(bytes,&t)||join(destination,card,slot)||join(stage_path,card,t.stage)||join(backup_path,card,t.backup))
        return fail(error,cap,"Corrupt save journal; generations retained for manual recovery: %s",path);
    int dest_exists=entry_exists(card,slot),stage_exists=entry_exists(card,t.stage),backup_exists=entry_exists(card,t.backup);
    if(dest_exists<0||stage_exists<0||backup_exists<0)return fail(error,cap,"Cannot inspect interrupted save generations");
    if(!t.publishing){
        /* The durable pending record precedes payload writes and every rename. */
        if(backup_exists||!!dest_exists!=!!t.old||(dest_exists&&!p4_os_is_dir_nofollow(destination)))
            return fail(error,cap,"Pending save has ambiguous external changes; generations retained: %s",path);
        if(discard_staging(&t))return fail(error,cap,"Original save unchanged; staging cleanup retained: %s",path);
        return 0;
    }
    if(dest_exists&&!verify_generation(card,slot,slot,&t.binding,NULL,0)){
        if(cleanup_committed(&t))fprintf(stderr,"p4pc: committed save retained with deferred journal cleanup: %s\n",path);
        return 0;
    }
    if(backup_exists){
        if(!t.old||dest_exists||!p4_os_is_dir_nofollow(backup_path))
            return fail(error,cap,"Ambiguous interrupted save; original/new generations retained: %s",path);
        if(p4_os_rename(backup_path,destination))return fail(error,cap,"Cannot restore previous save generation: %s",backup_path);
        if(journal_write(&t,0,0)||discard_staging(&t))
            return fail(error,cap,"Previous save restored; journal/staging retained: %s",path);
        return 0;
    }
    if(dest_exists&&t.old&&stage_exists){
        /* Before the first rename, or after real restoration. Mark pending
         * before deleting stage, so interrupted cleanup remains unambiguous. */
        if(!p4_os_is_dir_nofollow(destination)||journal_write(&t,0,0)||discard_staging(&t))
            return fail(error,cap,"Cannot abort interrupted save staging: %s",path);
        return 0;
    }
    if(!dest_exists&&stage_exists&&!t.old&&!verify_generation(card,t.stage,slot,&t.binding,NULL,0)){
        if(p4_os_rename(stage_path,destination)||remove_records(card,slot))
            return fail(error,cap,"Cannot finish complete interrupted save: %s",path);
        return 0;
    }
    return fail(error,cap,"Interrupted save has no provable complete publication; generations retained: %s",path);
}
typedef struct JournalScan { unsigned slots; } JournalScan;
static void find_journal(const char *name,void *user)
{
    JournalScan *scan=user;
    for(unsigned slot=0;slot<16;slot++){
        char save[33],journal[96];snprintf(save,sizeof save,"BASLUS-21782Save%u",slot);journal_name(journal,save);
        if(!strcmp(name,journal)){scan->slots|=1u<<slot;break;}
    }
}
static int recover_locked(const char *card,char *error,size_t cap)
{
    JournalScan scan={0};
    if(p4_os_list_dir(card,find_journal,&scan))return fail(error,cap,"Cannot inspect memory-card recovery journals");
    for(unsigned slot=0;slot<16;slot++)if(scan.slots&(1u<<slot)){
        char name[33];snprintf(name,sizeof name,"BASLUS-21782Save%u",slot);
        if(recover_one(card,name,error,cap))return -1;
    }
    return 0;
}
int p4_save_transactions_recover(const char *card,char *error,size_t cap)
{
    if(active_transaction)return fail(error,cap,"Memory-card transaction is busy");
    uintptr_t lock[2]={0};
    if(p4_os_directory_lock(card,lock))return fail(error,cap,"Cannot own memory-card recovery root: %s",strerror(errno));
    int result=recover_locked(card,error,cap);p4_os_directory_unlock(lock);return result;
}
static void release_transaction(P4SaveTransaction *t)
{
    p4_os_directory_unlock(t->lock);
    if(active_transaction==t)active_transaction=NULL;
    t->state=0;t->card=NULL;
}
int p4_save_transaction_begin(P4SaveTransaction *t,const char *card,unsigned preferred,
    int replace,int choose_free,char *error,size_t cap)
{
    if(!t||!card||preferred>=16||t->state||t->lock[0]||t->lock[1]||active_transaction||(replace&&choose_free))
        return fail(error,cap,"Invalid/busy memory-card transaction");
    if(p4_os_directory_lock(card,t->lock))return fail(error,cap,"Memory card is busy/unavailable: %s",strerror(errno));
    t->card=card;t->state=TX_LOCKED;t->old=t->journal=t->stage_owned=t->publishing=0;
    t->stage[0]=t->backup[0]=0;active_transaction=t;
    if(recover_locked(card,error,cap)){release_transaction(t);return -1;}
    snprintf(t->name,sizeof t->name,"BASLUS-21782Save%u",preferred);
    int old=entry_exists(card,t->name);
    if(old<0)goto inspect_fail;
    if(old&&!replace){
        if(!choose_free){fail(error,cap,"Save slot is occupied; not replaced");goto fail;}
        unsigned slot=0;
        for(;slot<16;slot++){
            snprintf(t->name,sizeof t->name,"BASLUS-21782Save%u",slot);
            old=entry_exists(card,t->name);if(old<0)goto inspect_fail;if(!old)break;
        }
        if(slot==16){fail(error,cap,"Memory card has no free save slot");goto fail;}
    }
    char destination[PATH_CAP];
    if(join(destination,card,t->name)||(old&&!p4_os_is_dir_nofollow(destination))){
        fail(error,cap,"Existing save destination is linked/non-directory");goto fail;
    }
    t->old=(unsigned)old;return 0;
inspect_fail:
    fail(error,cap,"Cannot inspect memory-card save slots");
fail:
    release_transaction(t);return -1;
}
int p4_save_transaction_tagged(const P4SaveTransaction *t)
{
    if(!t||!t->state||active_transaction!=t)return -1;
    if(!t->old)return 0;
    char root[PATH_CAP];if(join(root,t->card,t->name))return -1;
    int sidecar=entry_exists(root,P4_MOD_SAVE_SIDECAR),marker=entry_exists(root,P4_MOD_SAVE_COMMIT);
    return sidecar<0||marker<0?-1:sidecar||marker;
}
int p4_save_transaction_path(const P4SaveTransaction *t,const char *file,char *out,size_t capacity)
{
    if(!t||!t->state||!out||!capacity||active_transaction!=t)return -1;
    if(file){
        size_t length=strlen(file);
        if(!length||length>32||!strcmp(file,".")||!strcmp(file,".."))return -1;
        for(size_t i=0;i<length;i++)if((unsigned char)file[i]<32||(unsigned char)file[i]>=127||strchr("/\\:",file[i]))return -1;
    }
    int n=file?snprintf(out,capacity,"%s/%s/%s",t->card,t->state==TX_STAGED?t->stage:t->name,file):
        snprintf(out,capacity,"%s/%s",t->card,t->state==TX_STAGED?t->stage:t->name);
    return n<0||(size_t)n>=capacity||p4_os_extended_path(out,capacity)?-1:0;
}
int p4_save_transaction_stage(P4SaveTransaction *t,const P4SavePairBinding *binding,char *error,size_t cap)
{
    if(!t||t->state!=TX_LOCKED||active_transaction!=t||!binding_valid(binding))
        return fail(error,cap,"Invalid save stage/binding");
    if(stage_make(t->card,t->stage,sizeof t->stage,error,cap))return -1;
    t->stage_owned=1;t->binding=*binding;t->state=TX_STAGED;
    snprintf(t->backup,sizeof t->backup,".p4mod-backup-%llu-%u",(unsigned long long)p4_os_now_ns(),unique_counter++);
    char journal[96],next[104];journal_name(journal,t->name);snprintf(next,sizeof next,"%s.next",journal);
    if(entry_exists(t->card,t->backup)!=0||entry_exists(t->card,next)!=0||journal_write(t,1,0))
        return fail(error,cap,"Cannot record exclusive save staging intent: %s",strerror(errno));
    return 0;
}
typedef struct Clone {
    P4SaveTransaction *t;
    char source[PATH_CAP];
    uint64_t bytes;
    unsigned count;
    int failed;
} Clone;
static void clone_extra(const char *name,void *user)
{
    Clone *c=user;
    if(c->failed||!strcmp(name,".")||!strcmp(name,"..")||!strcmp(name,c->t->name)||
       !strcmp(name,"icon.ico")||!strcmp(name,"icon.sys")||!strcmp(name,P4_MOD_SAVE_SIDECAR)||!strcmp(name,P4_MOD_SAVE_COMMIT))return;
    char source[PATH_CAP],destination[PATH_CAP];
    if(join(source,c->source,name)||!p4_os_is_file_nofollow(source)){c->failed=1;return;}
    if(p4_save_witness_name(name))return; /* old-generation custody is not current state */
    if(++c->count>P4SAVE_MAX_FILES||p4_save_transaction_path(c->t,name,destination,sizeof destination)){
        c->failed=1;return;
    }
    FILE *in=p4_os_fopen_rb(source);
    if(!in){c->failed=1;return;}
    P4OSFileIdentity before,after;
    if(p4_os_file_identity(in,&before)||before.size>P4SAVE_MAX_BYTES-c->bytes){fclose(in);c->failed=1;return;}
    FILE *out=fopen(destination,"wbx");
    if(!out){fclose(in);c->failed=1;return;}
    unsigned char buffer[16384];uint64_t copied=0;
    while(copied<before.size){
        size_t n=(size_t)(before.size-copied);if(n>sizeof buffer)n=sizeof buffer;
        if(fread(buffer,1,n,in)!=n||fwrite(buffer,1,n,out)!=n){c->failed=1;break;}
        copied+=n;
    }
    if(ferror(in)||p4_os_file_identity(in,&after)||memcmp(&before,&after,sizeof before))c->failed=1;
    if(fclose(in))c->failed=1;
    if(fclose(out))c->failed=1;
    c->bytes+=copied;
}
int p4_save_transaction_clone_extra(P4SaveTransaction *t,char *error,size_t cap)
{
    if(!t||t->state!=TX_STAGED||active_transaction!=t)return fail(error,cap,"Save clone requires an owned stage");
    if(!t->old)return 0;
    Clone c={.t=t};
    if(join(c.source,t->card,t->name)||p4_os_list_dir(c.source,clone_extra,&c)||c.failed)
        return fail(error,cap,"Cannot preserve extra save files (linked/subdirectory/oversize/changed input)");
    return 0;
}
void p4_save_transaction_abort(P4SaveTransaction *t)
{
    if(!t||!t->state||active_transaction!=t)return;
    if(t->stage_owned){
        /* Never erase the only stage while a publishing record still needs it. */
        if(t->publishing&&journal_write(t,0,0)){
            fprintf(stderr,"p4pc: save generations/journal retained: %s/%s\n",t->card,t->name);
            release_transaction(t);return;
        }
        int bad=stage_remove(t->card,t->stage);
        if(!bad&&t->journal)bad=remove_records(t->card,t->name);
        if(bad)fprintf(stderr,"p4pc: save staging cleanup retained: %s/%s\n",t->card,t->stage);
    }
    release_transaction(t);
}
int p4_save_transaction_commit(P4SaveTransaction *t,char *error,size_t cap)
{
    if(!t||!t->state||active_transaction!=t)return fail(error,cap,"No owned save transaction");
    if(t->state==TX_LOCKED){release_transaction(t);return 0;}
    if(verify_generation(t->card,t->stage,t->name,&t->binding,error,cap)){
        p4_save_transaction_abort(t);return -1;
    }
    if(journal_write(t,0,1)){
        fail(error,cap,"Cannot record save publication: %s",strerror(errno));
        p4_save_transaction_abort(t);return -1;
    }
    char source[PATH_CAP],destination[PATH_CAP],backup[PATH_CAP];
    if(join(source,t->card,t->stage)||join(destination,t->card,t->name)||join(backup,t->card,t->backup)){
        fail(error,cap,"Save publication path too long");p4_save_transaction_abort(t);return -1;
    }
    if(t->old&&p4_os_rename(destination,backup)){
        fail(error,cap,"Cannot retain previous save generation: %s",strerror(errno));
        p4_save_transaction_abort(t);return -1;
    }
    if(p4_os_rename(source,destination)){
        int saved=errno;
        if(t->old&&p4_os_rename(backup,destination)){
            fail(error,cap,"Cannot publish/restore save; original retained at %s; stage %s",backup,source);
            release_transaction(t);return -1;
        }
        fail(error,cap,"Cannot publish save generation: %s",strerror(saved));
        p4_save_transaction_abort(t);return -1;
    }
    t->stage_owned=0;
    if(cleanup_committed(t))fprintf(stderr,"p4pc: save published; backup/journal cleanup deferred: %s/%s\n",t->card,t->name);
    release_transaction(t);return 0;
}
