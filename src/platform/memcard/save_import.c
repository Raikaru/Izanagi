#include "p4pc/save_import.h"
#include "p4pc/os.h"
#include "save_image.h"
#include "mod_save_format.h"
#include "mod_save_hash.h"
#include "mod_save_custody.h"
#include "save_transaction.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

int p4save_card(const unsigned char *,size_t,int (*)(P4SaveArchive *,void *),void *,char *,size_t);
static uint32_t rd(const unsigned char *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int fail(char *e,size_t n,const char *s) { if(n) snprintf(e,n,"%s",s); return -1; }
static int path(char *out,size_t cap,const char *a,const char *b) { int n=snprintf(out,cap,"%s/%s",a,b); return n<0 || (size_t)n>=cap || p4_os_extended_path(out,cap); }
static size_t trim_separators(char *p,size_t n) {
    while(n>1 && (p[n-1]=='/' || p[n-1]=='\\')) {
        if((n==3 && p[1]==':') ||
           (n==7 && !memcmp(p,"\\\\?\\",4) && p[5]==':')) break;
        p[--n]=0;
    }
    return n;
}
static int safe(const char *n) {
    size_t len=strlen(n); if(!len || len>32 || n[0]=='.' || n[len-1]=='.' || n[len-1]==' ') return 0;
    for(size_t i=0;i<len;i++) if((unsigned char)n[i]<32 || (unsigned char)n[i]>126 || strchr("<>:\"/\\|?*",n[i])) return 0;
    return 1;
}
static unsigned char *readfile(const char *p,size_t *n,size_t limit) {
    FILE *f=p4_os_fopen_rb(p); if(!f) return NULL;
    if(fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long end=ftell(f); if(end<0 || (size_t)end>limit || fseek(f,0,SEEK_SET)) { fclose(f); return NULL; }
    unsigned char *b=malloc(end? (size_t)end:1); if(!b) { fclose(f); return NULL; }
    int read_ok=fread(b,1,(size_t)end,f)==(size_t)end;
    if(fclose(f) || !read_ok) { free(b); return NULL; }
    *n=(size_t)end; return b;
}
/* All supported game filenames are ASCII. Use Windows collision semantics on
 * every host, so an archive cannot silently replace icon.sys via ICON.SYS. */
static int same_name(const char *a,const char *b) {
    while(*a && *b) {
        int x=*a++,y=*b++;
        if(x>='A' && x<='Z') x+=32;
        if(y>='A' && y<='Z') y+=32;
        if(x!=y) return 0;
    }
    return *a==*b;
}
/* Equivalent to the game's datSaveVerifyChecksum (0x0010e880), called with
 * file+4, size-4 by mc.c; the summary check is h_memcard.c's LOAD GAME check.
 * Unlike the original, every record header/length is bounded before reading. */
static int verify(P4SaveArchive *s,P4SavePairBinding *binding,char *e,size_t cap) {
    if(p4_save_slot_index(s->name)<0 || !s->count) return fail(e,cap,"Not an NTSC-U Persona 4 save directory");
    P4SaveFile *game=NULL; int icon=0,sys=0; size_t total=0;
    for(size_t i=0;i<s->count;i++) {
        P4SaveFile *f=&s->files[i]; total+=f->size;
        if(p4_save_witness_name(f->name))s->witness=1;
        if(!safe(f->name) || total>P4SAVE_MAX_BYTES) return fail(e,cap,"Invalid save filename or size");
        for(size_t j=0;j<i;j++) if(same_name(f->name,s->files[j].name)) return fail(e,cap,"Duplicate save filename");
        if(!strcmp(f->name,s->name)) game=f;
        if(!strcmp(f->name,"icon.sys") && f->size>=964 && !memcmp(f->data,"PS2D",4)) sys=1;
        if(!strcmp(f->name,"icon.ico") && f->size) icon=1;
    }
    if(!game || !sys || !icon) return fail(e,cap,"Missing game data, icon.sys or icon.ico");
    if(p4save_image_validate(game->data,game->size,e,cap))return -1;
    memset(binding,0,sizeof *binding);binding->retail_size=game->size;
    p4_save_hash(game->data,game->size,binding->retail_hash);
    if(!s->mod_sidecar.data&&!s->mod_commit.data)return 0;
    if(!s->mod_sidecar.data||!s->mod_commit.data)return fail(e,cap,"Incomplete .p4mods save transaction; both sidecar and commit marker are required");
    binding->sidecar_size=s->mod_sidecar.size;
    p4_save_hash(s->mod_sidecar.data,s->mod_sidecar.size,binding->sidecar_hash);
    if(p4_save_sidecar_verify(binding->retail_size,binding->retail_hash,s->mod_sidecar.data,s->mod_sidecar.size,e,cap)||
       p4_save_marker_verify(binding->retail_size,binding->retail_hash,binding->sidecar_size,binding->sidecar_hash,
           s->mod_commit.data,s->mod_commit.size,e,cap))return -1;
    if(rd(s->mod_sidecar.data+68)&P4_MOD_SAVE_WITNESS)s->witness=1;
    return 0;
}
static void stamp(const char *p,const unsigned char raw[8]) {
    if(!raw[4] || !raw[5]) return;
    struct tm tm={0}; tm.tm_sec=raw[1]; tm.tm_min=raw[2]; tm.tm_hour=raw[3]; tm.tm_mday=raw[4]; tm.tm_mon=raw[5]-1; tm.tm_year=(raw[6]|raw[7]<<8)-1900; tm.tm_isdst=-1;
    time_t t=mktime(&tm); if(t!=(time_t)-1) { struct utimbuf u={t,t}; utime(p,&u); }
}
static void encode_time(unsigned char out[8],time_t t) { struct tm *tm=localtime(&t); if(!tm) return; out[0]=0; out[1]=tm->tm_sec; out[2]=tm->tm_min; out[3]=tm->tm_hour; out[4]=tm->tm_mday; out[5]=tm->tm_mon+1; out[6]=(tm->tm_year+1900)&255; out[7]=(tm->tm_year+1900)>>8; }
/* PCSX2 _pcsx2_index uses flat filename maps of integer attributes, emitted
 * as block or flow YAML. Read only that data grammar, not tags or aliases.
 * Current $ROOT indexes use UTC epochs; legacy %ROOT indexes used GMT+9.
 * Format reference: pcsx2/SIO/Memcard/MemoryCardFolder.cpp (FromTime/WriteIndex).
 */
static int index_token(const unsigned char **cursor,const unsigned char *end,char out[65]) {
    const unsigned char *p=*cursor;
    while(p<end) {
        if(strchr(" \t\r\n",*p)) { p++; continue; }
        if(*p=='#') { while(p<end && *p!='\n') p++; continue; }
        break;
    }
    if(p==end) { *cursor=p; out[0]=0; return 0; }
    if(strchr("{}:,",*p)) { out[0]=*p++; out[1]=0; *cursor=p; return 1; }
    unsigned char quote=(*p=='"' || *p=='\'')?*p++:0;
    size_t n=0;
    while(p<end && (quote?*p!=quote:!strchr("{}:, \t\r\n#",*p))) {
        if(n==64 || *p=='\\') return -1;
        out[n++]=*p++;
    }
    if(quote) { if(p==end) return -1; p++; }
    if(!n) return -1;
    out[n]=0; *cursor=p; return 1;
}
static int folder_index(const char *dir,P4SaveArchive *s) {
    char pathbuf[2048]; struct stat st;
    if(path(pathbuf,sizeof pathbuf,dir,"_pcsx2_index")) return -1;
    if(stat(pathbuf,&st)) return errno==ENOENT?0:-1;
    if(st.st_size<0 || st.st_size>16384) return -1;
    size_t length; unsigned char *bytes=readfile(pathbuf,&length,16384);
    if(!bytes || length>16384 || memchr(bytes,0,length)) { free(bytes); return -1; }
    int legacy=0,current=0;
    for(size_t i=0;i+5<=length;i++) {
        if(!memcmp(bytes+i,"%ROOT",5)) legacy=1;
        if(!memcmp(bytes+i,"$ROOT",5)) current=1;
    }
    int offset_seconds=legacy && !current?32400:0;
    unsigned char *created=NULL,*modified=NULL;
    const unsigned char *cursor=bytes,*end=bytes+length;
    char key[65],value[65]; int rc=0,depth=0,t;
    while((t=index_token(&cursor,end,key))>0) {
        if(!strcmp(key,"---") || !strcmp(key,"...") || !strcmp(key,",")) continue;
        if(!strcmp(key,"{")) { if(++depth>2) { rc=-1; break; } continue; }
        if(!strcmp(key,"}")) { if(--depth<0) { rc=-1; break; } continue; }
        if(index_token(&cursor,end,value)!=1 || strcmp(value,":")) { rc=-1; break; }
        int is_created=!strcmp(key,"timeCreated"),is_modified=!strcmp(key,"timeModified");
        if(is_created || is_modified || !strcmp(key,"order")) {
            if(index_token(&cursor,end,value)!=1) { rc=-1; break; }
            errno=0; char *tail; long long epoch=strtoll(value,&tail,10);
            if(errno || tail==value || *tail) { rc=-1; break; }
            if(!is_created && !is_modified) continue;
            unsigned char *target=is_created?created:modified;
            if(!target) continue;
            if(epoch>INT64_MAX-offset_seconds) { rc=-1; break; }
            epoch+=offset_seconds;
            time_t time=(time_t)epoch; struct tm *tm=gmtime(&time);
            if((int64_t)time!=epoch || !tm || tm->tm_year+1900<0 || tm->tm_year+1900>65535) { rc=-1; break; }
            target[0]=0; target[1]=tm->tm_sec; target[2]=tm->tm_min; target[3]=tm->tm_hour;
            target[4]=tm->tm_mday; target[5]=tm->tm_mon+1; target[6]=(tm->tm_year+1900)&255; target[7]=(tm->tm_year+1900)>>8;
        } else {
            created=modified=NULL;
            if(!strcmp(key,"%ROOT") || !strcmp(key,"$ROOT")) { created=s->created; modified=s->modified; }
            for(size_t i=0;i<s->count;i++) if(!strcmp(key,s->files[i].name)) { created=s->files[i].created; modified=s->files[i].modified; break; }
        }
    }
    if(t<0 || depth) rc=-1;
    free(bytes); return rc;
}
/* Host filesystems cannot all set PS2 creation times. Preserve both original
 * eight-byte timestamps losslessly beside the files as P4SM v1 records. */
static int metadata(const char *dir,P4SaveArchive *s,const char *renamed) {
    char p[2048]; if(path(p,sizeof p,dir,".p4save-meta")) return -1;
    FILE *f=fopen(p,renamed?"wb":"rb");
    if(!f) return renamed?-1:0;
    unsigned char header[24]={ 'P','4','S','M',1,0,0,0 };
    int ok=1;
    if(renamed) {
        memcpy(header+8,s->created,8); memcpy(header+16,s->modified,8);
        ok=fwrite(header,1,sizeof header,f)==sizeof header;
        for(size_t i=0;i<s->count && ok;i++) {
            unsigned char row[49]={0}; P4SaveFile *file=&s->files[i];
            strcpy((char*)row,!strcmp(file->name,s->name)?renamed:file->name);
            memcpy(row+33,file->created,8); memcpy(row+41,file->modified,8);
            ok=fwrite(row,1,sizeof row,f)==sizeof row;
        }
    } else {
        ok=fread(header,1,sizeof header,f)==sizeof header && !memcmp(header,"P4SM\1\0\0\0",8);
        if(ok) { memcpy(s->created,header+8,8); memcpy(s->modified,header+16,8); }
        unsigned char row[49];
        while(ok) {
            size_t bytes=fread(row,1,sizeof row,f); if(!bytes) break;
            if(bytes!=sizeof row || !memchr(row,0,33)) { ok=0; break; }
            for(size_t i=0;i<s->count;i++) if(!strcmp(s->files[i].name,(char*)row)) {
                memcpy(s->files[i].created,row+33,8); memcpy(s->files[i].modified,row+41,8); break;
            }
        }
    }
    if(fclose(f)) ok=0;
    return ok?0:-1;
}
typedef struct { P4SaveArchive *s; const char *dir; char *error; size_t cap; int failed; size_t total; } Folder;
static void folder_file(const char *name,void *user) {
    Folder *c=user; if(c->failed || !strcmp(name,".") || !strcmp(name,"..") || !strncmp(name,"_pcsx2_",7) || !strcmp(name,".p4save-meta")) return;
    char p[2048]; struct stat st;
    if(path(p,sizeof p,c->dir,name)||!p4_os_is_file_nofollow(p)||stat(p,&st)||!S_ISREG(st.st_mode)) { c->failed=fail(c->error,c->cap,"Invalid or linked folder memory-card file"); return; }
    if(p4_save_witness_name(name)){c->s->witness=1;return;}
    P4SaveFile *f;
    size_t limit;
    if(!strcmp(name,P4_MOD_SAVE_SIDECAR)){f=&c->s->mod_sidecar;limit=P4_MOD_SAVE_MAX_BYTES;}
    else if(!strcmp(name,P4_MOD_SAVE_COMMIT)){f=&c->s->mod_commit;limit=P4_MOD_SAVE_MARKER_BYTES;}
    else{
        if(!safe(name)||c->s->count==P4SAVE_MAX_FILES){c->failed=fail(c->error,c->cap,"Invalid folder memory-card filename or count");return;}
        if(st.st_size<0||(uint64_t)st.st_size>P4SAVE_MAX_BYTES-c->total){c->failed=fail(c->error,c->cap,"Folder save exceeds 8 MiB");return;}
        c->total+=(size_t)st.st_size;f=&c->s->files[c->s->count++];limit=P4SAVE_MAX_BYTES;
    }
    if(f->data){c->failed=fail(c->error,c->cap,"Duplicate native save metadata");return;}
    strcpy(f->name,name);f->data=readfile(p,&f->size,limit);
    if(!f->data) { c->failed=fail(c->error,c->cap,"Cannot read bounded folder memory-card file"); return; }
    encode_time(f->created,st.st_ctime); encode_time(f->modified,st.st_mtime);
}
static int folder(const char *p,const char *name,P4SaveArchive *s,char *e,size_t cap) {
    if(!safe(name)) return fail(e,cap,"Invalid save directory name");
    strcpy(s->name,name); struct stat st; if(stat(p,&st)) return fail(e,cap,"Cannot read save folder metadata");
    int witness=p4_save_witness_input(p,1,e,cap);if(witness<0)return -1;
    if(witness)s->witness=1;
    encode_time(s->created,st.st_ctime); encode_time(s->modified,st.st_mtime);
    Folder ctx={s,p,e,cap,0,0};
    if(p4_os_list_dir(p,folder_file,&ctx) || folder_index(p,s) || metadata(p,s,NULL)) return fail(e,cap,"Cannot enumerate save folder or read timestamps");
    return ctx.failed;
}
typedef struct { P4SaveArchive saves[16];P4SavePairBinding bindings[16];size_t count;char *error;size_t cap; } Collection;
static int collect(P4SaveArchive *s,void *user) {
    Collection *c=user;
    if(c->count==16) return fail(c->error,c->cap,"Too many Persona 4 save directories");
    if(verify(s,&c->bindings[c->count],c->error,c->cap)) return -1;
    for(size_t i=0;i<c->count;i++) if(!strcmp(s->name,c->saves[i].name))
        return fail(c->error,c->cap,"Duplicate Persona 4 save directory");
    c->saves[c->count++]=*s; memset(s,0,sizeof *s); return 0;
}
typedef struct { Collection *c; const char *dir; int failed; } CardFolder;
static void folder_dir(const char *name,void *user) {
    CardFolder *c=user; if(c->failed || strncmp(name,"BASLUS-21782Save",sizeof "BASLUS-21782Save"-1)) return;
    char p[2048]; P4SaveArchive s={0};
    if(path(p,sizeof p,c->dir,name) || !p4_os_is_dir_nofollow(p) || folder(p,name,&s,c->c->error,c->c->cap) || collect(&s,c->c)) c->failed=-1;
    p4save_archive_free(&s);
}
static int exists(const char *p) { struct stat st; return stat(p,&st)==0; }
static int write_import_file(const P4SaveTransaction *transaction,const P4SaveFile *file,const char *name) {
    char p[P4_SAVE_PATH_CAP];
    if(p4_save_transaction_path(transaction,name,p,sizeof p))return -1;
    FILE *out=fopen(p,"wbx");if(!out)return -1;
    int bad=fwrite(file->data,1,file->size,out)!=file->size;
    if(fclose(out))bad=1;
    if(!bad)stamp(p,file->modified);
    return bad?-1:0;
}
static int import_source(const char *source,const char *card,int requested,char *message,size_t cap,
                         int witness_mode,int *output_witness) {
    char normalized[2048];
    size_t source_len=strlen(source);
    if(source_len>=sizeof normalized) return fail(message,cap,"Source path is too long");
    memcpy(normalized,source,source_len+1);
    source_len=trim_separators(normalized,source_len);
    if(!source_len || (strncmp(normalized,"fd:",3) && p4_os_extended_path(normalized,sizeof normalized)))
        return fail(message,cap,"Invalid source path");
    source=normalized;
    fail(message,cap,"Cannot import save source");
    Collection *c=calloc(1,sizeof *c); if(!c) return fail(message,cap,"Out of memory");
    c->error=message; c->cap=cap; int rc=-1; unsigned char *blob=NULL; size_t n=0;
    if(requested < -1 || requested>15) { fail(message,cap,"Slot must be 0 through 15"); goto done; }
    if(p4_os_is_dir_nofollow(source)) {
        const char *base=strrchr(source,'/'),*bs=strrchr(source,'\\'); if(!base || (bs && bs>base)) base=bs; base=base?base+1:source;
        if(p4_save_slot_index(base)>=0) { P4SaveArchive s={0}; int bad=folder(source,base,&s,message,cap) || collect(&s,c); p4save_archive_free(&s); if(bad) goto done; }
        else { CardFolder f={c,source,0}; if(p4_os_list_dir(source,folder_dir,&f) || f.failed) goto done; }
    } else {
        blob=readfile(source,&n,9u*1024u*1024u); if(!blob) { fail(message,cap,"Cannot read save container"); goto done; }
        if(n>=28 && !memcmp(blob,"Sony PS2 Memory Card Format ",28)) { if(p4save_card(blob,n,collect,c,message,cap)) goto done; }
        else { P4SaveArchive s={0}; int bad=p4save_decode(blob,n,&s,message,cap) || collect(&s,c); p4save_archive_free(&s); if(bad) goto done; }
    }
    if(!c->count) { fail(message,cap,"No Persona 4 saves found"); goto done; }
    if(requested>=0 && c->count!=1) { fail(message,cap,"--slot requires a container with exactly one Persona 4 save"); goto done; }
    int inherited=p4_save_witness_input(source,p4_os_is_dir_nofollow(source),message,cap);
    if(inherited<0)goto done;
    if(inherited||witness_mode)for(size_t i=0;i<c->count;i++)c->saves[i].witness=1;
    for(size_t i=0;i<c->count;i++)if(c->saves[i].witness)*output_witness=1;
    char root[2048]; size_t root_len=strlen(card);
    if(root_len>=sizeof root) { fail(message,cap,"Card path is too long"); goto done; }
    memcpy(root,card,root_len+1);
    if(!trim_separators(root,root_len)) { fail(message,cap,"Invalid card path"); goto done; }
    if(p4_os_extended_path(root,sizeof root) || (!p4_os_is_dir_nofollow(root) && p4_os_mkdir(root))) { fail(message,cap,"Cannot create memory-card folder"); goto done; }
    int destination_witness=p4_save_witness_input(root,1,message,cap);
    if(destination_witness<0)goto done;
    if(destination_witness){
        *output_witness=1;
        for(size_t i=0;i<c->count;i++)c->saves[i].witness=1;
    }
    if(p4_save_transactions_recover(root,message,cap))goto done;
    int slots[16]; unsigned occupied=0;
    for(int i=0;i<16;i++) { char nm[33],p[2048]; snprintf(nm,sizeof nm,"BASLUS-21782Save%d",i); if(path(p,sizeof p,root,nm)) { fail(message,cap,"Card path is too long"); goto done; } if(exists(p)) occupied|=1u<<i; }
    /* Reserve every still-free original slot before allocating collisions, so
     * one remapped save cannot steal another save's original free slot. */
    for(size_t i=0;i<c->count;i++) {
        int wanted=requested>=0?requested:p4_save_slot_index(c->saves[i].name);
        slots[i]=-1;
        if(requested>=0 || !(occupied&(1u<<wanted))) {
            slots[i]=wanted;
            occupied|=1u<<wanted;
        }
    }
    for(size_t i=0;i<c->count;i++) if(slots[i]<0) {
        int slot=0;
        while(slot<16 && (occupied&(1u<<slot))) slot++;
        if(slot==16) { fail(message,cap,"No empty save slots (use --slot N to explicitly replace one)"); goto done; }
        slots[i]=slot;
        occupied|=1u<<slot;
    }
    for(size_t i=0;i<c->count;i++) {
        P4SaveArchive *s=&c->saves[i];P4SaveTransaction transaction={0};char stage[P4_SAVE_PATH_CAP];
        if(p4_save_transaction_begin(&transaction,root,(unsigned)slots[i],requested>=0,0,message,cap))goto done;
        if(p4_save_transaction_stage(&transaction,&c->bindings[i],message,cap)){
            p4_save_transaction_abort(&transaction);goto done;
        }
        int bad=p4_save_transaction_path(&transaction,NULL,stage,sizeof stage);
        for(size_t j=0;j<s->count&&!bad;j++){
            P4SaveFile *file=&s->files[j];
            const char *name=!strcmp(file->name,s->name)?transaction.name:file->name;
            bad=write_import_file(&transaction,file,name);
        }
        if(!bad&&s->mod_sidecar.data)bad=write_import_file(&transaction,&s->mod_sidecar,P4_MOD_SAVE_SIDECAR);
        if(!bad&&s->mod_commit.data)bad=write_import_file(&transaction,&s->mod_commit,P4_MOD_SAVE_COMMIT);
        if(!bad)bad=metadata(stage,s,transaction.name);
        if(bad){p4_save_transaction_abort(&transaction);fail(message,cap,"Cannot write complete imported save generation");goto done;}
        if(s->witness&&p4_save_witness_mark(stage,message,cap)){p4_save_transaction_abort(&transaction);goto done;}
        stamp(stage,s->modified);
        if(p4_save_transaction_commit(&transaction,message,cap))goto done;
    }
    if(cap) { size_t used=(size_t)snprintf(message,cap,"Imported %zu save(s) to %s; slots",c->count,card); for(size_t i=0;i<c->count && used<cap;i++) { int added=snprintf(message+used,cap-used," %d",slots[i]); if(added>0) used+=(size_t)added; } }
    rc=0;
done: free(blob); for(size_t i=0;i<c->count;i++) p4save_archive_free(&c->saves[i]); free(c); return rc;
}
/* One common final receipt path also labels partial-batch failures and early
 * source/path errors under the explicit witness mode. Never relabel ordinary
 * player's mod-history metadata as test custody. */
int p4save_import(const char *source,const char *card,int requested,char *message,size_t cap) {
    int witness_mode=p4_save_witness_mode(message,cap);
    if(witness_mode<0)return -1;
    int witness=witness_mode;
    int result=import_source(source,card,requested,message,cap,witness_mode,&witness);
    if(witness&&cap){
        static const char prefix[]="[mod-witness; NOT a retail reference] ";
        size_t start=sizeof prefix-1;
        if(start>=cap){memcpy(message,prefix,cap-1);message[cap-1]=0;}
        else{
            size_t size=strlen(message);
            if(size>cap-start-1)size=cap-start-1;
            memmove(message+start,message,size);memcpy(message,prefix,start);message[start+size]=0;
        }
    }
    return result;
}
