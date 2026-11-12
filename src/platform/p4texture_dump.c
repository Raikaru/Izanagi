/* Opt-in original texture capture. Encoding and filesystem writes stay off the
 * game thread. A failed dump is incomplete, never a successful missing frame. */
#include "p4pc/texture_dump.h"
#include "p4pc/os.h"
#include "../viewer/p4png.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DUMP_BYTES (24u * 1024u * 1024u)
#define KEY_LIMIT 131072u
#define PATH_CAP 1200

typedef struct DumpKey { char hex[65]; } DumpKey;
typedef struct DumpJob {
    struct DumpJob *next;
    P4TexturePixels pixels;
    size_t bytes;
    enum P4TextureOrigin origin;
    char key[65], name[512];
    uint8_t data[];
} DumpJob;
static struct {
    SDL_Mutex *mutex;
    SDL_Condition *condition;
    SDL_Thread *thread;
    FILE *inventory;
    DumpJob *first, *last;
    DumpKey *keys;
    size_t key_count, key_cap;
    char directory[PATH_CAP];
    int active, stopping, failed, synthetic;
    uint64_t excluded[8];
    P4TextureDumpStats stats;
} dump;

static const char *origin_name(enum P4TextureOrigin origin)
{
    static const char *const names[]={"unknown","disc","font-disc","override",
        "framebuffer","movie","procedural","synthetic"};
    return (unsigned)origin<sizeof names/sizeof *names?names[origin]:"unknown";
}
static int make_directories(const char *path)
{
    char copy[PATH_CAP];
    size_t length=strlen(path);
    if (!length || length>=sizeof copy) return -1;
    memcpy(copy,path,length+1);
    for (size_t i=1;i<=length;++i) {
        if (copy[i]=='/' || copy[i]=='\\' || copy[i]==0) {
            char saved=copy[i];
            /* A drive prefix itself is not a directory to create. */
            if (copy[i-1]!=':') {
                copy[i]=0;
                if (p4_os_mkdir(copy)!=0 && errno!=EEXIST) return -1;
                copy[i]=saved;
            }
        }
    }
    /* Explicit output roots may traverse normal OS aliases (/var on macOS,
     * AppData junctions on Windows). The final generated directory is real. */
    return p4_os_is_dir_nofollow(path)?0:-1;
}
static uint32_t key_hash(const char *key)
{
    uint32_t hash=2166136261u;
    for (unsigned i=0;i<64;++i) hash=(hash^(uint8_t)key[i])*16777619u;
    return hash;
}
/* Called under the queue lock. -1 failure, 0 already queued, 1 inserted. */
static int remember_key(const char *key)
{
    size_t index;
    if (dump.key_cap) {
        index=key_hash(key)&(dump.key_cap-1);
        while (dump.keys[index].hex[0]) {
            if (strcmp(dump.keys[index].hex,key)==0) return 0;
            index=(index+1)&(dump.key_cap-1);
        }
    }
    if (!dump.key_cap || (dump.key_count+1)*2>=dump.key_cap) {
        size_t cap=dump.key_cap?dump.key_cap*2:1024;
        DumpKey *grown;
        if (cap>KEY_LIMIT) return -1;
        grown=calloc(cap,sizeof *grown);
        if (!grown) return -1;
        for (size_t i=0;i<dump.key_cap;++i) if (dump.keys[i].hex[0]) {
            size_t slot=key_hash(dump.keys[i].hex)&(cap-1);
            while (grown[slot].hex[0]) slot=(slot+1)&(cap-1);
            grown[slot]=dump.keys[i];
        }
        free(dump.keys); dump.keys=grown; dump.key_cap=cap;
    }
    index=key_hash(key)&(dump.key_cap-1);
    while (dump.keys[index].hex[0]) {
        if (strcmp(dump.keys[index].hex,key)==0) return 0;
        index=(index+1)&(dump.key_cap-1);
    }
    memcpy(dump.keys[index].hex,key,65); ++dump.key_count;
    return 1;
}
static int write_string(FILE *fp, const char *text)
{
    if (fputc('"',fp)==EOF) return -1;
    for (const unsigned char *p=(const unsigned char *)text;*p;++p) {
        if (*p=='"' || *p=='\\') { if (fputc('\\',fp)==EOF) return -1; }
        if (*p<32) { if (fprintf(fp,"\\u%04x",*p)<0) return -1; }
        else if (fputc(*p,fp)==EOF) return -1;
    }
    return fputc('"',fp)==EOF?-1:0;
}
static int same_file(const char *a, const char *b)
{
    unsigned char x[4096], y[4096];
    FILE *left=p4_os_fopen_rb(a), *right=p4_os_fopen_rb(b);
    int same=1;
    if (!left || !right) same=0;
    while (same) {
        size_t n=fread(x,1,sizeof x,left), m=fread(y,1,sizeof y,right);
        if (n!=m || memcmp(x,y,n)!=0 || ferror(left) || ferror(right)) same=0;
        if (!same || n<sizeof x) break;
    }
    if (left && fclose(left)!=0) same=0;
    if (right && fclose(right)!=0) same=0;
    return same;
}
static int write_job(const DumpJob *job)
{
    static const char digits[]="0123456789abcdef";
    char path[PATH_CAP], temporary[PATH_CAP];
    FILE *existing;
    const P4TexturePixels *p=&job->pixels;
    int n=snprintf(path,sizeof path,"%s/%s.png",dump.directory,job->key);
    int m=snprintf(temporary,sizeof temporary,"%s/%s.png.tmp",dump.directory,job->key);
    if (n<0 || m<0 || (size_t)n>=sizeof path || (size_t)m>=sizeof temporary) return -1;
    if (p4png_write_rgba(temporary,p->width,p->height,p->rgba)!=0) return -1;
    existing=p4_os_fopen_rb(path);
    if (existing) {
        fclose(existing);
        if (!same_file(path,temporary)) {
            fprintf(stderr,"texture dump: existing PNG differs: %s\n",path);
            remove(temporary);
            return -1;
        }
        if (remove(temporary)!=0) return -1;
    } else if (errno!=ENOENT || p4_os_rename(temporary,path)!=0) {
        remove(temporary); return -1;
    }
    if (fprintf(dump.inventory,"{\"schema\":\"p4tex-v1\",\"hash\":\"%s\",\"width\":%u,\"height\":%u,\"format\":%u,\"palette_format\":%u,\"palette_entries\":%u,\"palette_rgba_hex\":\"",
        job->key,p->width,p->height,p->format,p->palette_format,p->palette_entries)<0) return -1;
    for (size_t i=0;i<(size_t)p->palette_entries*4;++i) {
        if (fputc(digits[p->palette[i]>>4],dump.inventory)==EOF ||
            fputc(digits[p->palette[i]&15],dump.inventory)==EOF) return -1;
    }
    if (fprintf(dump.inventory,"\",\"origin\":\"%s\",\"replaceable\":%s,\"source\":",
        origin_name(job->origin),job->origin==P4_TEXTURE_FONT_DISC?"false":"true")<0 ||
        write_string(dump.inventory,job->name)!=0 ||
        fputs("}\n",dump.inventory)==EOF || fflush(dump.inventory)!=0) return -1;
    return 0;
}
static int SDLCALL dump_worker(void *unused)
{
    (void)unused;
    for (;;) {
        DumpJob *job;
        int failed;
        SDL_LockMutex(dump.mutex);
        while (!dump.first && !dump.stopping) SDL_WaitCondition(dump.condition,dump.mutex);
        job=dump.first;
        if (!job) { SDL_UnlockMutex(dump.mutex); break; }
        dump.first=job->next;
        if (!dump.first) dump.last=NULL;
        failed=dump.failed;
        SDL_UnlockMutex(dump.mutex);
        if (!failed) failed=write_job(job)!=0;
        SDL_LockMutex(dump.mutex);
        if (failed) { dump.failed=1; ++dump.stats.errors; }
        else ++dump.stats.completed;
        dump.stats.queued_bytes-=job->bytes;
        SDL_BroadcastCondition(dump.condition);
        SDL_UnlockMutex(dump.mutex);
        free(job);
    }
    return 0;
}
int p4_texture_dump_start(const char *root, int allow_synthetic)
{
    char inventory[PATH_CAP];
    int n;
    if (dump.active) return -1;
    memset(&dump,0,sizeof dump);
    if (!root || !*root) return 0;
    n=snprintf(dump.directory,sizeof dump.directory,"%s/textures/p4tex-v1",root);
    if (n<0 || (size_t)n>=sizeof dump.directory || make_directories(dump.directory)!=0) goto fail;
    n=snprintf(inventory,sizeof inventory,"%s/inventory.jsonl",root);
    if (n<0 || (size_t)n>=sizeof inventory) goto fail;
    dump.inventory=fopen(inventory,"ab");
    if (!dump.inventory) goto fail;
    dump.mutex=SDL_CreateMutex(); dump.condition=SDL_CreateCondition();
    if (!dump.mutex || !dump.condition) goto fail;
    dump.synthetic=allow_synthetic;
    dump.active=1;
    dump.thread=SDL_CreateThread(dump_worker,"texture-dump",NULL);
    if (!dump.thread) goto fail;
    fprintf(stderr,"texture dump: original player-disc assets only; do not distribute Atlus assets. Output: %s\n",root);
    return 0;
fail:
    if (dump.inventory) fclose(dump.inventory);
    if (dump.condition) SDL_DestroyCondition(dump.condition);
    if (dump.mutex) SDL_DestroyMutex(dump.mutex);
    memset(&dump,0,sizeof dump); dump.failed=1; dump.stats.errors=1;
    fprintf(stderr,"texture dump: cannot start output at %s\n",root);
    return -1;
}
int p4_texture_dump_active(void) { return dump.active; }
int p4_texture_dump_submit(const P4TexturePixels *p, enum P4TextureOrigin origin, const char *name, char key_out[65])
{
    char key[65];
    size_t image_size, palette_size, bytes;
    DumpJob *job;
    int remembered;
    if (!dump.active) return 0;
    if (origin!=P4_TEXTURE_DISC && origin!=P4_TEXTURE_FONT_DISC &&
        !(origin==P4_TEXTURE_SYNTHETIC && dump.synthetic)) {
        SDL_LockMutex(dump.mutex);
        ++dump.stats.excluded;
        ++dump.excluded[(unsigned)origin<8?(unsigned)origin:0];
        SDL_UnlockMutex(dump.mutex);
        return 0;
    }
    if (p4_texture_key(p,key)!=0) goto fail;
    if (key_out) memcpy(key_out,key,65);
    image_size=(size_t)p->width*p->height*4;
    palette_size=(size_t)p->palette_entries*4;
    if (image_size>DUMP_BYTES-sizeof *job-palette_size) goto fail;
    bytes=sizeof *job+image_size+palette_size;
    SDL_LockMutex(dump.mutex);
    if (dump.failed || dump.stopping) { SDL_UnlockMutex(dump.mutex); return -1; }
    remembered=remember_key(key);
    if (remembered<=0) {
        if (remembered<0) { dump.failed=1; ++dump.stats.errors; }
        SDL_UnlockMutex(dump.mutex);
        return remembered<0?-1:0;
    }
    while (!dump.failed && dump.stats.queued_bytes>DUMP_BYTES-bytes)
        SDL_WaitCondition(dump.condition,dump.mutex);
    if (dump.failed) { SDL_UnlockMutex(dump.mutex); return -1; }
    /* Account before allocation so active worker + queued jobs share one cap. */
    dump.stats.queued_bytes+=bytes;
    if (dump.stats.peak_bytes<dump.stats.queued_bytes) dump.stats.peak_bytes=dump.stats.queued_bytes;
    SDL_UnlockMutex(dump.mutex);
    job=malloc(bytes);
    if (!job) {
        SDL_LockMutex(dump.mutex); dump.stats.queued_bytes-=bytes;
        SDL_UnlockMutex(dump.mutex); goto fail;
    }
    memset(job,0,sizeof *job);
    job->pixels=*p; job->bytes=bytes; job->origin=origin;
    memcpy(job->key,key,sizeof key);
    snprintf(job->name,sizeof job->name,"%s",name?name:"");
    memcpy(job->data,p->rgba,image_size); job->pixels.rgba=job->data;
    if (palette_size) memcpy(job->data+image_size,p->palette,palette_size);
    job->pixels.palette=palette_size?job->data+image_size:NULL;
    SDL_LockMutex(dump.mutex);
    if (dump.last) dump.last->next=job; else dump.first=job;
    dump.last=job; ++dump.stats.submitted;
    SDL_SignalCondition(dump.condition);
    SDL_UnlockMutex(dump.mutex);
    return 0;
fail:
    SDL_LockMutex(dump.mutex); dump.failed=1; ++dump.stats.errors;
    SDL_BroadcastCondition(dump.condition); SDL_UnlockMutex(dump.mutex);
    fprintf(stderr,"texture dump: incomplete (invalid image, index limit, or allocation failure)\n");
    return -1;
}
void p4_texture_dump_stats(P4TextureDumpStats *stats)
{
    if (!stats) return;
    if (dump.active) SDL_LockMutex(dump.mutex);
    *stats=dump.stats;
    if (dump.active) SDL_UnlockMutex(dump.mutex);
}
int p4_texture_dump_finish(void)
{
    int failed;
    if (!dump.active) return dump.failed?-1:0;
    SDL_LockMutex(dump.mutex); dump.stopping=1;
    SDL_BroadcastCondition(dump.condition); SDL_UnlockMutex(dump.mutex);
    SDL_WaitThread(dump.thread,NULL);
    for (unsigned i=0;i<8;++i) if (dump.excluded[i] &&
        fprintf(dump.inventory,"{\"excluded_origin\":\"%s\",\"count\":%llu}\n",
            origin_name((enum P4TextureOrigin)i),(unsigned long long)dump.excluded[i])<0) dump.failed=1;
    if (fflush(dump.inventory)!=0) dump.failed=1;
    if (fclose(dump.inventory)!=0) dump.failed=1;
    failed=dump.failed;
    if (failed && !dump.stats.errors) ++dump.stats.errors;
    fprintf(stderr,"texture dump: %s submitted=%llu completed=%llu excluded=%llu errors=%llu\n",
        failed?"INCOMPLETE":"complete",(unsigned long long)dump.stats.submitted,
        (unsigned long long)dump.stats.completed,(unsigned long long)dump.stats.excluded,
        (unsigned long long)dump.stats.errors);
    SDL_DestroyCondition(dump.condition); SDL_DestroyMutex(dump.mutex);
    free(dump.keys); dump.keys=NULL; dump.active=0;
    return failed?-1:0;
}
