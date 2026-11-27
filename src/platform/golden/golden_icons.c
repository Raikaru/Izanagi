#include "golden_icons.h"
#include "golden_mode_boot.h"
#include "golden_mode_sources.h"
#include "p4pc/golden_atlas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char atlas_chain[]="data_e.cpk::init_free.bin::init/camp.arc::c_main01x2.spr";
static const char atlas_hash[]="edca8715c9aadde997fbc72be49f2b779e83cdd31bab4a0dcc3ea2c95b31b141";
static const int indices[P4_GOLDEN_ICON_COUNT]={-1,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x4c,0x4d,0x4f,0x50,0x51,0x52,0x53,0x54,0x6c,0x251,0x252,0x253};
static struct {
    P4GoldenCachePaths paths;
    char path[4][4096];
    P4GoldenCacheIdentity identity;
    P4GoldenCacheIdentityEntry sources[33],table_versions[64],versions[65];
    char names[33][256],hashes[33][65];
    char tables[P4_GOLDEN_ICON_COUNT][32];
    char key[65];
    uint8_t *refs;
    void (*prior_release)(P4GoldenModeSources *);
    int ready;
} cache;
static uint32_t u32(const uint8_t *b) { return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24; }
static void w32(uint8_t *b,uint32_t v) { for(unsigned i=0;i<4;++i) b[i]=(uint8_t)(v>>(8*i)); }
static int fail(char *r,size_t n,const char *s) { if(r && n) snprintf(r,n,"%s",s); return -1; }
static void key(int index,char *out,size_t n)
{ if(index<0) snprintf(out,n,"p4pc.golden:sprite-none"); else snprintf(out,n,"p4pc.golden:sprite-%03x",(unsigned)index); }
static int ref(int index,P4ResourceRef *out)
{
    if(!out) return -1;
    memset(out,0,sizeof *out); strcpy(out->owner,"p4pc.golden"); key(index,out->key,sizeof out->key); out->revision=1; return 0;
}
int p4_golden_icons_item_ref(uint32_t flags,P4ResourceRef *out) { return ref(p4_golden_item_sprite(flags),out); }
int p4_golden_icons_arcana_ref(unsigned arcana,int special,P4ResourceRef *out)
{ int i=p4_golden_arcana_sprite(arcana,special); return i<0?-1:ref(i,out); }
static void release_refs(P4GoldenModeSources *s)
{
    free(cache.refs); cache.refs=NULL;
    if(cache.prior_release) cache.prior_release(s);
    cache.prior_release=NULL;
}
static int order(const void *a,const void *b)
{ return strcmp(((const P4GoldenCacheIdentityEntry *)a)->name,((const P4GoldenCacheIdentityEntry *)b)->name); }
static int configure(P4GoldenModeSources *s)
{
    const char *paths[]={s->paths.pref,s->paths.install,s->paths.program,s->paths.repo};
    const char **dest[]={&cache.paths.pref,&cache.paths.install,&cache.paths.program,&cache.paths.repo};
    for(unsigned i=0;i<4;++i) {
        if(paths[i]) { if(strlen(paths[i])>=sizeof cache.path[i]) return -1; strcpy(cache.path[i],paths[i]); *dest[i]=cache.path[i]; }
        else *dest[i]=NULL;
    }
    P4GoldenModeIdentity *id=&s->identity;
    size_t n=id->identity.source_count,v=id->identity.version_count;
    if(n>=P4_GOLDEN_MODE_SOURCE_MAX || v>=63) return -1;
    strcpy(id->names[n],atlas_chain); strcpy(id->hashes[n],atlas_hash);
    id->sources[n]=(P4GoldenCacheIdentityEntry){id->names[n],id->hashes[n]};
    id->identity.source_count=++n; qsort(id->sources,n,sizeof id->sources[0],order);
    for(size_t i=0;i<n;++i) {
        strcpy(cache.names[i],id->sources[i].name); strcpy(cache.hashes[i],id->sources[i].value);
        cache.sources[i]=(P4GoldenCacheIdentityEntry){cache.names[i],cache.hashes[i]};
    }
    for(size_t i=0;i<v;++i) cache.table_versions[i]=id->identity.versions[i];
    cache.table_versions[v++]=(P4GoldenCacheIdentityEntry){"golden-native-atlas","spr0-tmx-dds-bc1237@3"};
    qsort(cache.table_versions,v,sizeof cache.table_versions[0],order);
    /* Declaration and image caches share source/converter identity, but have
     * different complete object sets and therefore MUST have distinct keys. */
    id->identity.versions=cache.table_versions; id->identity.version_count=v;
    memcpy(cache.versions,cache.table_versions,v*sizeof cache.versions[0]);
    cache.versions[v++]=(P4GoldenCacheIdentityEntry){"purpose","golden-atlas-images@1"};
    qsort(cache.versions,v,sizeof cache.versions[0],order);
    cache.identity=(P4GoldenCacheIdentity){cache.sources,n,cache.versions,v};
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) {
        if(indices[i]<0) strcpy(cache.tables[i],"atlas/000-none");
        else snprintf(cache.tables[i],sizeof cache.tables[i],"atlas/001-%03x",(unsigned)indices[i]);
    }
    return 0;
}
int p4_golden_icons_cache_build(P4GameMode mode,const P4GoldenCachePaths *paths,
    const P4GoldenCacheIdentity *identity,const uint8_t *bytes,size_t size,
    char key_hash[65],char *reason,size_t cap)
{
    P4GoldenAtlas atlas={0}; P4GoldenCacheTable tables[P4_GOLDEN_ICON_COUNT]={0};
    char names[P4_GOLDEN_ICON_COUNT][32];
    if(mode!=P4_GAME_MODE_GOLDEN) return fail(reason,cap,"Golden atlas conversion requires Golden mode");
    if(p4_golden_atlas_open(bytes,size,&atlas)) return fail(reason,cap,"invalid Golden icon atlas");
    int result=-1; size_t total=0;
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) {
        P4GoldenSprite s={0};
        if(indices[i]>=0 && p4_golden_atlas_sprite(&atlas,indices[i],&s)) goto done;
        uint32_t w=indices[i]<0?1u:(uint32_t)(s.x1-s.x0),h=indices[i]<0?1u:(uint32_t)(s.y1-s.y0);
        size_t n=44u+(size_t)w*h*4;
        if(n>P4_GOLDEN_TEXTURE_BUDGET-total) goto done;
        total+=n;
        uint8_t *data=calloc(1,n); if(!data) goto done;
        if(indices[i]<0) strcpy(names[i],"atlas/000-none");
        else snprintf(names[i],sizeof names[i],"atlas/001-%03x",(unsigned)indices[i]);
        tables[i]=(P4GoldenCacheTable){names[i],data,n};
        w32(data,w); w32(data+4,h); w32(data+8,(uint32_t)indices[i]);
        w32(data+12,s.texture_index); w32(data+16,(uint32_t)s.x0); w32(data+20,(uint32_t)s.y0);
        w32(data+24,(uint32_t)s.offset_x); w32(data+28,(uint32_t)s.offset_y);
        w32(data+32,(uint32_t)s.width); w32(data+36,(uint32_t)s.height); w32(data+40,s.flip);
        if(indices[i]>=0 && p4_golden_atlas_crop(&atlas,indices[i],data+44,n-44)) goto done;
    }
    result=p4_golden_cache_store(paths,identity,tables,P4_GOLDEN_ICON_COUNT,key_hash,reason,cap);
done:
    p4_golden_cache_free(tables,P4_GOLDEN_ICON_COUNT); p4_golden_atlas_free(&atlas);
    return result;
}
static int prepare(void *context,char *reason,size_t cap)
{
    P4GoldenModeSources *s=context;
    if(!s || cache.refs || configure(s)) return fail(reason,cap,"invalid Golden icon source context");
    cache.ready=0;
    char path[2048]; P4GoldenCpk cpk={0}; P4GoldenCpkEntry entry;
    uint8_t *bin=NULL; size_t bin_size=0;
    const uint8_t *camp,*atlas; size_t camp_size,atlas_size;
    if(snprintf(path,sizeof path,"%s/data_e.cpk",s->directory)>=(int)sizeof path ||
       p4_golden_cpk_open(&cpk,path,reason,cap)) return -1;
    int result=p4_golden_cpk_find(&cpk,"init_free.bin",&entry) ||
        p4_golden_cpk_read(&cpk,&entry,&bin,&bin_size,reason,cap);
    p4_golden_cpk_close(&cpk);
    if(result) { free(bin); return fail(reason,cap,"cannot read Golden atlas source"); }
    char hash[65];
    result=p4_golden_bin_find(bin,bin_size,"init/camp.arc",&camp,&camp_size) ||
        p4_golden_pak2_find(camp,camp_size,"c_main01x2.spr",&atlas,&atlas_size);
    if(!result) { p4_content_digest(atlas,atlas_size,hash); result=strcmp(hash,atlas_hash); }
    if(!result) {
        P4GoldenCacheTable hit[P4_GOLDEN_ICON_COUNT]={0};
        for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) hit[i].name=cache.tables[i];
        int loaded=p4_golden_cache_load(&cache.paths,&cache.identity,hit,P4_GOLDEN_ICON_COUNT,cache.key,reason,cap);
        p4_golden_cache_free(hit,P4_GOLDEN_ICON_COUNT);
        result=loaded==1?p4_golden_icons_cache_build(P4_GAME_MODE_GOLDEN,&cache.paths,&cache.identity,
            atlas,atlas_size,cache.key,reason,cap):loaded;
    }
    free(bin);
    if(result) return fail(reason,cap,"Golden atlas identity or conversion refused");
    size_t rows=s->map.shape.items;
    if(rows>2560 || s->projection.item_metadata.size!=rows*68) return fail(reason,cap,"invalid Golden icon item rows");
    cache.refs=calloc(rows,164); if(!cache.refs) return -1;
    for(size_t i=0;i<rows;++i) {
        P4ResourceRef icon; p4_golden_icons_item_ref(u32(s->projection.item_metadata.data+i*68+4),&icon);
        memcpy(cache.refs+i*164,icon.owner,64); memcpy(cache.refs+i*164+64,icon.key,96); w32(cache.refs+i*164+160,icon.revision);
    }
    if(p4_golden_mode_outputs_set(&s->outputs,"decl/item-icons",cache.refs,rows*164)) {
        free(cache.refs); cache.refs=NULL; return fail(reason,cap,"cannot publish Golden icon references");
    }
    cache.prior_release=s->release_outputs; s->release_outputs=release_refs; cache.ready=1; return 0;
}
int p4_golden_icons_register(P4GameMode mode)
{
    if(mode==P4_GAME_MODE_PS2) return 0;
    if(mode!=P4_GAME_MODE_GOLDEN) return -1;
    return p4_golden_mode_adapter_register("expanded item icon sources",prepare);
}
int p4_golden_icons_images(P4GameMode mode,P4GoldenIconImage out[P4_GOLDEN_ICON_COUNT],char *reason,size_t cap)
{
    if(mode!=P4_GAME_MODE_GOLDEN || !out || !cache.ready) return fail(reason,cap,"Golden icon cache is not admitted");
    P4GoldenCacheTable tables[P4_GOLDEN_ICON_COUNT]={0}; char key_hash[65];
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) tables[i].name=cache.tables[i];
    if(p4_golden_cache_load(&cache.paths,&cache.identity,tables,P4_GOLDEN_ICON_COUNT,key_hash,reason,cap) || strcmp(key_hash,cache.key)) goto invalid;
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) {
        if(tables[i].size<48) goto invalid;
        uint32_t w=u32(tables[i].data),h=u32(tables[i].data+4);
        if(!w || !h || w>16384 || h>16384 || tables[i].size>P4_GOLDEN_TEXTURE_BUDGET ||
           (uint64_t)w*h*4!=tables[i].size-44 || (int32_t)u32(tables[i].data+8)!=indices[i]) goto invalid;
    }
    memset(out,0,sizeof *out*P4_GOLDEN_ICON_COUNT);
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) {
        P4GoldenIconImage *image=&out[i]; key(indices[i],image->key,sizeof image->key);
        image->object=tables[i].data; image->size=tables[i].size; image->width=u32(image->object); image->height=u32(image->object+4);
        p4_content_fingerprint_seal(&image->identity,image->object,image->size);
        image->source.origin=P4_TEXTURE_OVERRIDE; image->source.rank=P4_CONTENT_RANK_GOLDEN_CACHE;
        strcpy(image->source.provider,"p4pc.golden");
        snprintf(image->source.key,sizeof image->source.key,"golden-cache:%s:%s",cache.key,image->identity.hex);
    }
    return 0;
invalid:
    p4_golden_cache_free(tables,P4_GOLDEN_ICON_COUNT); return fail(reason,cap,"Golden icon cache hash or framing refused");
}
