/* Private, content-addressed Golden tables. Nothing in this directory is a package input. */
#include "p4pc/golden_cache.h"
#include "p4pc/content_digest.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAP 4096
#define META_CAP (1024u * 1024u)
#define OBJECT_CAP (256u * 1024u * 1024u)
#define ENTRY_CAP 4096u

typedef struct Buffer { char *p; size_t used; } Buffer;
static int fail(char *reason, size_t cap, const char *message)
{
    if (reason && cap) snprintf(reason, cap, "%s", message);
    return -1;
}
static int add(Buffer *b, const char *text)
{
    size_t n = strlen(text);
    if (n >= META_CAP - b->used) return -1;
    memcpy(b->p + b->used, text, n + 1); b->used += n; return 0;
}
static int token(const char *s)
{
    size_t n = s ? strlen(s) : 0;
    if (!n || n > 1024) return 0;
    for (; *s; ++s) if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
        (*s >= '0' && *s <= '9') || *s == '_' || *s == '-' || *s == '.' ||
        *s == '/' || *s == ':' || *s == '@')) return 0;
    return 1;
}
static int hash_valid(const char *s)
{
    if (!s || strlen(s) != 64) return 0;
    for (; *s; ++s) if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f'))) return 0;
    return 1;
}
static int entries(Buffer *b, const P4GoldenCacheIdentityEntry *e, size_t count, int hashes)
{
    if (count > ENTRY_CAP || (count && !e)) return -1;
    for (size_t i = 0; i < count; ++i) {
        if (!token(e[i].name) || !token(e[i].value) || (hashes && !hash_valid(e[i].value)) ||
            (i && strcmp(e[i-1].name, e[i].name) >= 0) ||
            (i && add(b, ",")) || add(b, "[\"") || add(b, e[i].name) ||
            add(b, "\",\"") || add(b, e[i].value) || add(b, "\"]")) return -1;
    }
    return 0;
}
static char *manifest(const P4GoldenCacheIdentity *id, char key[65])
{
    if (!id || !key || !id->source_count || !id->version_count) return NULL;
    Buffer b = {malloc(META_CAP), 0};
    if (!b.p) return NULL;
    b.p[0] = 0;
    if (add(&b, "{\"format\":1,\"sources\":[") || entries(&b,id->sources,id->source_count,1) ||
        add(&b, "],\"versions\":[") || entries(&b,id->versions,id->version_count,0) || add(&b,"]}\n")) {
        free(b.p); return NULL;
    }
    p4_content_digest(b.p, b.used, key); return b.p;
}
static int join(char out[PATH_CAP], const char *a, const char *b)
{
#ifdef _WIN32
    int n = snprintf(out, PATH_CAP, "%s\\%s", a, b);
#else
    int n = snprintf(out, PATH_CAP, "%s/%s", a, b);
#endif
    return n > 0 && n < PATH_CAP ? 0 : -1;
}
static char fold(char c)
{
#ifdef _WIN32
    if (c == '\\') return '/';
    if (c >= 'A' && c <= 'Z') return (char)(c + ('a' - 'A'));
#endif
    return c;
}
static int contains(const char *base, const char *path)
{
    size_t n = strlen(base), i = 0;
    while (n > 1 && (base[n-1] == '/' || base[n-1] == '\\')) --n;
    for (; i < n && path[i] && fold(base[i]) == fold(path[i]); ++i) {}
    return i == n && (!path[i] || path[i] == '/' || path[i] == '\\' || (n == 1 && base[0] == '/'));
}
static int plain_dir(const char *path, int create)
{
    if (p4_os_is_dir_nofollow(path)) return 0;
    if (!create || (p4_os_mkdir_utf8(path) && errno != EEXIST)) return -1;
    return p4_os_is_dir_nofollow(path) ? 0 : -1;
}
/* Refuse a checkout even when the caller did not know its root. Worktrees have
 * a .git file; normal checkouts have a .git directory. Resolve both, including
 * aliases, rather than treating a link as absence. */
static int checkout_ancestor(const char *pref)
{
    char dir[PATH_CAP], probe[PATH_CAP], resolved[PATH_CAP];
    if (strlen(pref) >= sizeof dir) return 1;
    size_t floor = 1;
#ifdef _WIN32
    if (!strncmp(pref,"\\\\?\\UNC\\",8)) {
        const char *server_end = strchr(pref+8,'\\');
        if (!server_end || server_end == pref+8 || !server_end[1]) return 1;
        const char *share_end = strchr(server_end+1,'\\');
        floor = share_end ? (size_t)(share_end-pref)+1 : strlen(pref);
    } else if (!strncmp(pref,"\\\\?\\",4) && pref[4] && pref[5] == ':' && pref[6] == '\\') {
        floor = 7;
    } else return 1;
#endif
    strcpy(dir,pref);
    for (;;) {
        if (join(probe,dir,".git")) return 1;
        if (!p4_os_resolved_path(probe,resolved,sizeof resolved)) return 1;
        if (errno != ENOENT) return 1;
        size_t n = strlen(dir);
        if (n <= floor) break; /* The volume/share root was already checked. */
        while (n && dir[n-1] != '/' && dir[n-1] != '\\') --n;
        if (n <= floor) dir[floor] = 0;
        else dir[n-1] = 0;
    }
    return 0;
}
static int root_path(const P4GoldenCachePaths *paths, char root[PATH_CAP], int create, char *reason, size_t cap)
{
    char pref[PATH_CAP], other[PATH_CAP];
    if (!paths || !paths->pref || !paths->install || !paths->program ||
        !p4_os_is_dir_nofollow(paths->pref) || p4_os_resolved_path(paths->pref,pref,sizeof pref))
        return fail(reason,cap,"invalid Golden cache preference directory");
    if (checkout_ancestor(pref)) return fail(reason,cap,"Golden cache cannot be inside a checkout");
    if (join(root,pref,"golden-cache")) return fail(reason,cap,"Golden cache path too long");
    const char *excluded[] = {paths->install, paths->program, paths->repo};
    for (size_t i = 0; i < 3; ++i) {
        if (!excluded[i]) continue;
        if (p4_os_resolved_path(excluded[i],other,sizeof other) || contains(other,root) || contains(root,other))
            return fail(reason,cap,"Golden cache preference directory is unsafe");
    }
    if (!p4_os_is_dir_nofollow(root)) {
        if (!create) {
            if (p4_os_resolved_path(root,other,sizeof other) && errno == ENOENT) return 1;
            return fail(reason,cap,"unsafe Golden cache directory");
        }
        if (plain_dir(root,1)) return fail(reason,cap,"cannot open Golden cache directory");
    }
    return 0;
}
static int valid_tables(const P4GoldenCacheTable *tables, size_t count, int storing)
{
    if (!count || count > ENTRY_CAP || !tables) return 0;
    for (size_t i = 0; i < count; ++i)
        if (!token(tables[i].name) || (i && strcmp(tables[i-1].name,tables[i].name) >= 0) ||
            (storing && ((!tables[i].data && tables[i].size) || tables[i].size > OBJECT_CAP))) return 0;
    return 1;
}
static int write_file(const char *path, const void *bytes, size_t size)
{
    FILE *f = p4_os_fopen_utf8(path,"wb");
    if (!f) return -1;
    int bad = fwrite(bytes,1,size,f) != size || p4_os_sync_file(f);
    if (fclose(f)) bad = 1;
    return bad ? -1 : 0;
}
static uint8_t *read_file(const char *path, size_t max, size_t *size)
{
    long n;
    if (!p4_os_is_file_nofollow(path)) return NULL;
    FILE *f = p4_os_fopen_utf8(path,"rb");
    if (!f) return NULL;
    if (fseek(f,0,SEEK_END) || (n = ftell(f)) < 0 || (unsigned long)n > max || fseek(f,0,SEEK_SET)) {
        fclose(f); return NULL;
    }
    uint8_t *p = malloc((size_t)n + 1);
    if (!p || fread(p,1,(size_t)n,f) != (size_t)n) { free(p); fclose(f); return NULL; }
    fclose(f); p[n] = 0; *size = (size_t)n; return p;
}
void p4_golden_cache_free(P4GoldenCacheTable *tables, size_t count)
{
    if (!tables) return;
    for (size_t i = 0; i < count; ++i) { free(tables[i].data); tables[i].data = NULL; tables[i].size = 0; }
}
/* The index is canonical: one table per line, with only restricted tokens. */
static int index_line(Buffer *b, const char *name, const char *hash, size_t size)
{
    char num[32]; snprintf(num,sizeof num,"%zu",size);
    return add(b,"[\"") || add(b,name) || add(b,"\",\"") || add(b,hash) ||
           add(b,"\",") || add(b,num) || add(b,"]");
}
static int load_at(const char *dir, const char *expected, P4GoldenCacheTable *tables, size_t count)
{
    char path[PATH_CAP], objects[PATH_CAP], digest[65]; size_t size, manifest_size;
    if (!p4_os_is_dir_nofollow(dir) || join(path,dir,"manifest.json")) return 1;
    uint8_t *m = read_file(path,META_CAP,&manifest_size);
    if (!m || memchr(m,0,manifest_size) || join(path,dir,"tables-index.json")) { free(m); return 1; }
    uint8_t *index = read_file(path,META_CAP,&size);
    if (!index || memchr(index,0,size) || join(objects,dir,"objects") || !p4_os_is_dir_nofollow(objects)) {
        free(m); free(index); return 1;
    }
    p4_content_digest(index,size,digest);
    size_t identity_size = strlen(expected);
    char suffix[96];
    int suffix_size = snprintf(suffix,sizeof suffix,",\"index_sha256\":\"%s\"}\n",digest);
    /* The key hashes the canonical identity; the manifest also seals the index. */
    if (!identity_size || expected[identity_size-1] != '\n' || suffix_size < 0 ||
        manifest_size != identity_size-2+(size_t)suffix_size ||
        memcmp(m,expected,identity_size-2) ||
        memcmp(m+identity_size-2,suffix,(size_t)suffix_size)) {
        free(m); free(index); return 1;
    }
    free(m);
    char *cursor = (char *)index;
    if (strncmp(cursor,"[\n",2)) { free(index); return 1; }
    cursor += 2;
    for (size_t i = 0; i < count; ++i) {
        size_t name_len = strlen(tables[i].name), object_size = 0;
        if (i && strncmp(cursor,",\n",2)) goto bad;
        if (i) cursor += 2;
        if (strncmp(cursor,"[\"",2) || strncmp(cursor+2,tables[i].name,name_len)) goto bad;
        cursor += 2 + name_len;
        if (strncmp(cursor,"\",\"",3) || strlen(cursor) < 69) goto bad;
        cursor += 3; memcpy(digest,cursor,64); digest[64] = 0; cursor += 64;
        if (!hash_valid(digest) || strncmp(cursor,"\",",2)) goto bad;
        cursor += 2;
        if (*cursor < '0' || *cursor > '9') goto bad;
        do { unsigned d = (unsigned)(*cursor++ - '0'); if (object_size > (OBJECT_CAP-d)/10) goto bad; object_size = object_size*10+d; } while (*cursor >= '0' && *cursor <= '9');
        if (*cursor++ != ']') goto bad;
        if (join(path,objects,digest)) goto bad;
        tables[i].data = read_file(path,OBJECT_CAP,&tables[i].size);
        if (!tables[i].data || tables[i].size != object_size) goto bad;
        char actual[65]; p4_content_digest(tables[i].data,tables[i].size,actual);
        if (strcmp(actual,digest)) goto bad;
    }
    if (strcmp(cursor,"\n]\n")) goto bad;
    free(index); return 0;
bad:
    free(index); p4_golden_cache_free(tables,count); return 1;
}
int p4_golden_cache_load(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
                         P4GoldenCacheTable *tables, size_t count, char key[65], char *reason, size_t cap)
{
    char root[PATH_CAP], dir[PATH_CAP];
    if (!tables || !count || count > ENTRY_CAP) return fail(reason,cap,"invalid Golden cache tables");
    for (size_t i = 0; i < count; ++i) { tables[i].data = NULL; tables[i].size = 0; }
    if (!valid_tables(tables,count,0)) return fail(reason,cap,"invalid Golden cache tables");
    char *m = manifest(identity,key);
    if (!m) return fail(reason,cap,"invalid Golden cache identity");
    int result = root_path(paths,root,0,reason,cap);
    if (!result) result = join(dir,root,key) ? fail(reason,cap,"Golden cache path too long") : load_at(dir,m,tables,count);
    free(m); return result;
}
/* Remove only the exact directory shape this cache writes, never follow links or
 * recursively delete unknown files. Unknown entries make pruning fail closed. */
typedef struct Remove { char dir[PATH_CAP]; int bad; } Remove;
static void remove_object(const char *name, void *user)
{
    Remove *r = user; char path[PATH_CAP];
    if (!strcmp(name,".") || !strcmp(name,"..")) return;
    if (!hash_valid(name) || join(path,r->dir,name) || !p4_os_is_file_nofollow(path) || p4_os_remove_utf8(path)) r->bad = 1;
}
static int remove_entry(const char *dir)
{
    char path[PATH_CAP]; Remove r = {{0},0};
    if (!p4_os_is_dir_nofollow(dir) || join(r.dir,dir,"objects")) return -1;
    if (p4_os_is_dir_nofollow(r.dir)) {
        if (p4_os_list_dir_utf8(r.dir,remove_object,&r) || r.bad || p4_os_rmdir(r.dir)) return -1;
    }
    const char *files[] = {"manifest.json","tables-index.json"};
    for (size_t i = 0; i < 2; ++i) {
        if (join(path,dir,files[i])) return -1;
        if (p4_os_is_file_nofollow(path) && p4_os_remove_utf8(path)) return -1;
    }
    return p4_os_rmdir(dir);
}
typedef struct Prune { const char *root, *keep; int failed; } Prune;
static void prune(const char *name, void *user)
{
    Prune *p = user; char path[PATH_CAP];
    if (hash_valid(name) && strcmp(name,p->keep) &&
        (join(path,p->root,name) || remove_entry(path))) p->failed = 1;
}
int p4_golden_cache_store(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
                          const P4GoldenCacheTable *tables, size_t count, char key[65], char *reason, size_t cap)
{
    char root[PATH_CAP], stage[PATH_CAP], dir[PATH_CAP], objects[PATH_CAP], path[PATH_CAP], digest[65];
    uintptr_t lock[2] = {0,0}; int result = -1, locked = 0;
    if (!valid_tables(tables,count,1)) return fail(reason,cap,"invalid Golden cache tables");
    char *m = manifest(identity,key), *index = malloc(META_CAP);
    if (!m || !index) { free(m); free(index); return fail(reason,cap,"invalid Golden cache identity or out of memory"); }
    Buffer b = {index,0}; index[0] = 0;
    if (root_path(paths,root,1,reason,cap) || join(stage,root,"staging") || join(dir,root,key) ||
        join(objects,stage,"objects")) goto done;
    if (p4_os_directory_lock(root,lock)) { fail(reason,cap,"Golden cache is busy"); goto done; }
    locked = 1;
    if (p4_os_is_dir_nofollow(stage) && remove_entry(stage)) { fail(reason,cap,"cannot remove Golden cache staging"); goto done; }
    if (plain_dir(stage,1) || plain_dir(objects,1) || add(&b,"[\n")) goto io;
    for (size_t i = 0; i < count; ++i) {
        p4_content_digest(tables[i].data,tables[i].size,digest);
        if (join(path,objects,digest) || (!p4_os_is_file_nofollow(path) && write_file(path,tables[i].data,tables[i].size)) ||
            (i && add(&b,",\n")) || index_line(&b,tables[i].name,digest,tables[i].size)) goto io;
    }
    if (add(&b,"\n]\n")) goto io;
    p4_content_digest(index,b.used,digest);
    size_t msize = strlen(m);
    char suffix[96];
    int suffix_size = snprintf(suffix,sizeof suffix,",\"index_sha256\":\"%s\"}\n",digest);
    if (msize < 2 || suffix_size < 0 || msize - 2 + (size_t)suffix_size >= META_CAP) goto io;
    memcpy(m+msize-2,suffix,(size_t)suffix_size+1);
    if (join(path,stage,"manifest.json") || write_file(path,m,strlen(m)) ||
        join(path,stage,"tables-index.json") || write_file(path,index,b.used)) goto io;
    if (p4_os_is_dir_nofollow(dir) && remove_entry(dir)) goto io;
    if (p4_os_rename(stage,dir)) goto io;
    {
        Prune p = {root,key,0};
        if (p4_os_list_dir_utf8(root,prune,&p) || p.failed) {
            fail(reason,cap,"published Golden cache but cannot prune an old entry"); goto done;
        }
    }
    if (reason && cap) reason[0] = 0;
    result = 0; goto done;
io:
    fail(reason,cap,"cannot publish Golden cache");
done:
    if (locked) p4_os_directory_unlock(lock);
    free(index); free(m); return result;
}
