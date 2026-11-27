/* Golden-only model source -> genuine RW hierarchy -> existing native atomic renderer.
 * No PS2 asset/skeleton fallback and no glTF intermediate. U05 policy is provisional:
 * source diffuse RGBA, straight embedded alpha, repeat/linear sampling. Unknown
 * extra layers, nonidentity UV transforms and tool-only draw state are refused.
 * Raw material commands are retained in the owned source for owner BBV/M8c-2. */
#include "p4pc/golden_model.h"
#include "p4pc/game_mode.h"
#include "p4pc/gmem.h"
#include "golden_model_rw.h"
#include "../files/vfs.h"
#include "../../render/p4gl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static uint32_t u32(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static float f32(const uint8_t *p) { uint32_t u=u32(p); float f; memcpy(&f,&u,4); return f; }
static void *pointer(const void *p, size_t off) { uint32_t g; memcpy(&g,(const uint8_t *)p+off,4); return P4_G2H(g); }
static void put_pointer(void *p, size_t off, const void *value) { uint32_t g=P4_H2G(value); memcpy((uint8_t *)p+off,&g,4); }
static void matrix(float *rw, const float *m)
{
    memcpy(rw,m,64);
    /* RW affine rows occupy the same scalar slots; their fourth words are flags/padding. */
    uint32_t zero=0; memcpy(rw+3,&zero,4); memcpy(rw+7,&zero,4); memcpy(rw+11,&zero,4); memcpy(rw+15,&zero,4);
}
static void named(char *error, size_t capacity, const P4GoldenModel *m, uint32_t node, const char *why)
{
    const P4GoldenModelNode *n=&m->nodes[node];
    if(error && capacity) snprintf(error,capacity,"%s: %.*s at +0x%x",why,(int)n->name_size,(const char *)m->bytes+n->name,n->offset);
}
static void *texture(const P4GoldenModel *m, uint32_t node, char *error, size_t capacity)
{
    uint32_t image=p4_golden_model_child(m,node,0x8013,0);
    if(image==NONE) { named(error,capacity,m,node,"external-only texture U06"); return NULL; }
    const uint8_t *p=m->bytes+m->nodes[image].args; P4GoldenTexture t={0};
    if(p4_golden_model_image(p+4,u32(p),&t)) { named(error,capacity,m,node,"unsupported embedded image U06"); return NULL; }
    const P4GoldenTextureLevel *l=&t.levels[0];
    void *raster=RwRasterCreate((int32_t)l->width,(int32_t)l->height,32,0x504u);
    if(!raster) { p4_golden_texture_free(&t); return NULL; }
    uint8_t *dest=RwRasterLock(raster,0,1); int32_t stride=0; memcpy(&stride,(uint8_t *)raster+24,4);
    if(!dest || stride<(int32_t)(l->width*4)) {
        if(dest) RwRasterUnlock(raster);
        func_003ec330(raster); p4_golden_texture_free(&t); return NULL;
    }
    for(uint32_t y=0;y<l->height;++y) memcpy(dest+(size_t)y*(size_t)stride,l->rgba+(size_t)y*l->width*4,(size_t)l->width*4);
    RwRasterUnlock(raster); p4_golden_texture_free(&t);
    void *tex=func_003ef2e0(raster);
    if(!tex) { func_003ec330(raster); return NULL; }
    char name[32]; const P4GoldenModelNode *n=&m->nodes[node];
    snprintf(name,sizeof name,"%.*s",(int)n->name_size,(const char *)m->bytes+n->name);
    func_003ef470(tex,name); uint32_t sampler=0x1102; memcpy((uint8_t *)tex+80,&sampler,4); return tex;
}
static void *material(const P4GoldenModel *m, uint32_t node, void **textures, char *error, size_t capacity)
{
    uint32_t layer=p4_golden_model_child(m,node,9,0);
    if(p4_golden_model_child(m,node,9,1)!=NONE) { named(error,capacity,m,node,"multiple material layers U05"); return NULL; }
    void *tex=NULL;
    if(layer!=NONE) {
        uint32_t matrix_node=p4_golden_model_child(m,layer,0x809a,0);
        if(matrix_node!=NONE) {
            const uint8_t *p=m->bytes+m->nodes[matrix_node].args;
            for(unsigned j=0;j<16;++j) if(f32(p+4*j)!=(j%5==0?1.0f:0.0f)) { named(error,capacity,m,node,"nonidentity UV transform U05"); return NULL; }
        }
        uint32_t ref=p4_golden_model_child(m,layer,0x8091,0);
        if(ref!=NONE) {
            uint32_t image=p4_golden_model_resolve(m,layer,u32(m->bytes+m->nodes[ref].args));
            if(image==NONE || m->nodes[image].type!=10) { named(error,capacity,m,node,"texture reference U02"); return NULL; }
            if(!textures[image]) textures[image]=texture(m,image,error,capacity);
            tex=textures[image]; if(!tex) return NULL;
        }
    }
    void *mat=RpMaterialCreate(); if(!mat) return NULL;
    uint8_t color[4]={255,255,255,255};
    uint32_t rgba=p4_golden_model_child(m,node,0x8082,0);
    if(rgba!=NONE) {
        const uint8_t *p=m->bytes+m->nodes[rgba].args;
        for(unsigned j=0;j<4;++j) { float f=f32(p+4*j); if(!isfinite(f) || f<0 || f>1) { RpMaterialDestroy(mat); named(error,capacity,m,node,"diffuse range U05"); return NULL; } color[j]=(uint8_t)(f*255+0.5f); }
    }
    memcpy((uint8_t *)mat+4,color,4);
    const float surface[3]={1,0,1}; memcpy((uint8_t *)mat+12,surface,12);
    if(tex) RpMaterialSetTexture(mat,tex);
    return mat;
}
typedef struct NativeFrames { void **bones, **geometries; } NativeFrames;
static void *build(const P4GoldenModelScene *s, char *error, size_t capacity, void **helper_frames, NativeFrames *native)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN || !s || !s->source.bytes) { if(error && capacity) snprintf(error,capacity,"Golden-only model source"); return NULL; }
    if(p4_golden_model_static_check(s,error,capacity)) return NULL;
    const P4GoldenModel *m=&s->source;
    void **frames=calloc(s->bone_count?s->bone_count:1,sizeof *frames);
    void **materials=calloc(m->count?m->count:1,sizeof *materials), **textures=calloc(m->count?m->count:1,sizeof *textures);
    void *clump=NULL, *root=NULL;
    if(!frames || !materials || !textures) goto done;
    clump=RpClumpCreate(); if(!clump) goto done;
    root=RwFrameCreate(); if(!root) goto failed;
    put_pointer(clump,4,root);
    /* Each bone is a real independent frame; no traversal-index/HAnim-id alias. */
    for(uint32_t i=0;i<s->bone_count;++i) {
        frames[i]=RwFrameCreate(); if(!frames[i]) goto failed;
        RwFrameAddChild(root,frames[i]);
        float rw[16]; matrix(rw,s->bones[i].local); func_003e9cb0(frames[i],rw,0);
    }
    for(uint32_t i=0;i<s->bone_count;++i) if(s->bones[i].parent!=NONE) RwFrameAddChild(frames[s->bones[i].parent],frames[i]);
    /* Helpers remain distinct attachment frames, with their id/TRQ retained by the source resource. */
    for(uint32_t i=0;i<s->helper_count;++i) {
        const P4GoldenModelHelper *h=&s->helpers[i];
        const uint8_t *p=m->bytes+m->nodes[h->node].args;
        float q[4], rw[16]={0}; for(unsigned j=0;j<4;++j) q[j]=f32(p+28+j*4);
        float x=q[0],y=q[1],z=q[2],w=q[3];
        rw[0]=1-2*(y*y+z*z); rw[1]=2*(x*y+z*w); rw[2]=2*(x*z-y*w);
        rw[4]=2*(x*y-z*w); rw[5]=1-2*(x*x+z*z); rw[6]=2*(y*z+x*w);
        rw[8]=2*(x*z+y*w); rw[9]=2*(y*z-x*w); rw[10]=1-2*(x*x+y*y);
        for(unsigned j=0;j<3;++j) rw[12+j]=f32(p+16+j*4);
        void *frame=RwFrameCreate(); if(!frame) goto failed;
        RwFrameAddChild(frames[h->bone],frame); func_003e9cb0(frame,rw,0);
        if(helper_frames) helper_frames[i]=frame;
    }
    for(uint32_t i=0;i<s->mesh_count;++i) {
        const P4GoldenModelMesh *mesh=&s->meshes[i];
        if(mesh->vertices>65535 || mesh->triangles>65535 || mesh->material>=m->count) { named(error,capacity,m,mesh->node,"native geometry count bound"); goto failed; }
        if(!materials[mesh->material]) materials[mesh->material]=material(m,mesh->material,textures,error,capacity);
        void *mat=materials[mesh->material]; if(!mat) goto failed;
        uint32_t format=0x5au | (mesh->uv_sets<<16);
        void *geometry=func_003c2630((int32_t)mesh->vertices,(int32_t)mesh->triangles,format);
        if(!geometry) goto failed;
        void *morph=pointer(geometry,92); float *position=morph?pointer(morph,20):NULL, *normal=morph?pointer(morph,24):NULL;
        uint8_t *colors=pointer(geometry,48), *triangles=pointer(geometry,44); float *uv[4]={0};
        for(uint32_t set=0;set<mesh->uv_sets;++set) uv[set]=pointer(geometry,52+4*set);
        if(!position || !normal || !colors || !triangles) { func_003c2a80(geometry); goto failed; }
        for(uint32_t v=0;v<mesh->vertices;++v) {
            memcpy(position+v*3,mesh->vertex[v].position,12); memcpy(normal+v*3,mesh->vertex[v].normal,12); memcpy(colors+v*4,mesh->vertex[v].color,4);
            for(uint32_t set=0;set<mesh->uv_sets;++set) if(uv[set]) memcpy(uv[set]+v*2,mesh->vertex[v].uv[set],8);
        }
        for(uint32_t t=0;t<mesh->triangles;++t) {
            const uint32_t *ix=mesh->indices+t*3;
            RpGeometryTriangleSetVertexIndices(geometry,triangles+t*8,(uint16_t)ix[0],(uint16_t)ix[1],(uint16_t)ix[2]);
            RpGeometryTriangleSetMaterial(geometry,triangles+t*8,mat);
        }
        float sphere[4]; RpMorphTargetCalcBoundingSphere(morph,sphere); memcpy((uint8_t *)morph+4,sphere,16); func_003c22f0(geometry);
        void *atomic=func_003c00e0();
        if(!atomic) { func_003c2a80(geometry); goto failed; }
        func_003c1b90(atomic,mesh->bone==NONE?root:frames[mesh->bone]);
        func_003c0210(atomic,geometry,0); func_003c2a80(geometry); RpClumpAddAtomic(clump,atomic);
        if(native) native->geometries[i]=geometry; /* atomic owns the geometry reference */
    }
    RwFrameUpdateObjects(root);
    goto done;
failed:
    RpClumpDestroy(clump); clump=NULL;
done:
    if(materials) for(uint32_t i=0;i<m->count;++i) if(materials[i]) RpMaterialDestroy(materials[i]);
    if(textures) for(uint32_t i=0;i<m->count;++i) if(textures[i]) func_003ef3a0(textures[i]);
    if(clump && native) native->bones=frames; else free(frames);
    free(materials); free(textures);
    if(!clump && error && capacity && !error[0]) snprintf(error,capacity,"native model allocation/build failure");
    return clump;
}
void *p4_golden_model_build(const P4GoldenModelScene *s, char *error, size_t capacity)
{
    return build(s,error,capacity,NULL,NULL);
}
/* Host-only resource ownership, never serialized into guest Model slots. Whole-source SHA
 * covers every embedded texture and retained motion/adjunct; chain + converter disambiguate
 * nested assets and conversion revisions. No speculative hrestex dependency is introduced. */
typedef struct Resource {
    struct Resource *next, *parent;
    void *clump, **helpers;
    NativeFrames native;
    P4GoldenModelScene scene;
    P4GoldenMotions motions;
    uint8_t record[10];
    char chain[2048], sha256[65];
} Resource;
static Resource *resources;
void *p4_golden_model_load(const char *path, char *error, size_t capacity)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN) { if(error && capacity) snprintf(error,capacity,"Golden-only model source"); return NULL; }
    int fd=vfs_open(path,0); if(fd<0) { if(error && capacity) snprintf(error,capacity,"Golden VFS model absent: %s",path); return NULL; }
    P4VfsOrigin origin; long long size=vfs_seek(fd,0,2); Resource *r=NULL; uint8_t *bytes=NULL;
    if(vfs_origin(fd,&origin) || origin.kind!=P4_VFS_GOLDEN || !origin.chain || !origin.sha256 || size<4 || size>P4_GOLDEN_MODEL_BUDGET || vfs_seek(fd,0,0)<0) goto done;
    r=calloc(1,sizeof *r); bytes=malloc((size_t)size); if(!r || !bytes) goto done;
    if(strlen(origin.chain)>=sizeof r->chain || strlen(origin.sha256)!=64) goto done;
    memcpy(r->chain,origin.chain,strlen(origin.chain)+1); memcpy(r->sha256,origin.sha256,65);
    if(vfs_read(fd,bytes,(int)size)!=size || p4_golden_model_scene(bytes,(size_t)size,&r->scene,error,capacity) ||
       p4_golden_model_motions(&r->scene.source,&r->motions,error,capacity)) goto done;
    r->helpers=calloc(r->scene.helper_count?r->scene.helper_count:1,sizeof *r->helpers);
    r->native.geometries=calloc(r->scene.mesh_count?r->scene.mesh_count:1,sizeof *r->native.geometries);
    if(!r->helpers || !r->native.geometries) goto done;
    r->clump=build(&r->scene,error,capacity,r->helpers,&r->native);
    if(r->clump) {
        fprintf(stderr,"Golden model %s sha256=%s converter=%s bones=%u meshes=%u helpers=%u\n",r->chain,r->sha256,P4_GOLDEN_MODEL_CONVERTER,r->scene.bone_count,r->scene.mesh_count,r->scene.helper_count);
        r->next=resources; resources=r;
    }
done:
    vfs_close(fd); free(bytes);
    if(r && r->clump) {
        /* Explicit owner-only frame capture controls. Ordinary loads stay at rest;
         * no shared viewer/PS2 path or host clock drives game animation. */
        const char *slot_text=getenv("P4PC_GOLDEN_MODEL_SLOT"), *time_text=getenv("P4PC_GOLDEN_MODEL_SECONDS");
        if(slot_text || time_text) {
            char *slot_end=NULL, *time_end=NULL;
            long slot=slot_text?strtol(slot_text,&slot_end,10):32;
            double seconds=time_text?strtod(time_text,&time_end):NAN;
            if(!slot_text || !time_text || slot_end==slot_text || *slot_end || time_end==time_text || *time_end ||
               slot< -1 || slot>31 || !isfinite(seconds) ||
               p4_golden_model_play(r->clump,0,(int32_t)slot,NULL,seconds,0,error,capacity)) {
                if(error && capacity && !error[0]) snprintf(error,capacity,"invalid Golden motion capture controls");
                p4_golden_model_destroy(r->clump); return NULL;
            }
            fprintf(stderr,"Golden motion slot=%ld seconds=%.17g nonlooped U07/U08 defaults\n",slot,seconds);
        }
        return r->clump;
    }
    if(r) { free(r->helpers); free(r->native.bones); free(r->native.geometries); p4_golden_model_motions_free(&r->motions); p4_golden_model_scene_free(&r->scene); free(r); }
    if(error && capacity && !error[0]) snprintf(error,capacity,"Golden VFS model identity/read refusal: %s",path);
    return NULL;
}
void p4_golden_model_destroy(void *clump)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN || !clump) return;
    Resource **slot=&resources;
    while(*slot && (*slot)->clump!=clump) slot=&(*slot)->next;
    if(*slot) {
        Resource *r=*slot; *slot=r->next;
        if(r->parent) RwFrameRemoveChild(pointer(r->clump,4));
        for(Resource *child=resources;child;child=child->next) if(child->parent==r) {
            RwFrameRemoveChild(pointer(child->clump,4)); child->parent=NULL;
        }
        RpClumpDestroy(r->clump); free(r->helpers); free(r->native.bones); free(r->native.geometries);
        p4_golden_model_motions_free(&r->motions); p4_golden_model_scene_free(&r->scene); free(r);
    }
    else RpClumpDestroy(clump); /* A direct p4_golden_model_build result has no source registry. */
}
int p4_golden_model_attach(void *owner, uint32_t bone, int32_t helper_id, void *child, char *error, size_t capacity)
{
    Resource *a=NULL, *b=NULL;
    if(p4_game_mode()==P4_GAME_MODE_GOLDEN) for(Resource *r=resources;r;r=r->next) {
        if(r->clump==owner) a=r;
        if(r->clump==child) b=r;
    }
    const char *reason="attachment requires two owned Golden resources";
    if(a && b) {
        for(Resource *p=a;p;p=p->parent) if(p==b) { reason="attachment resource cycle"; goto failed; }
        uint32_t found=NONE;
        for(uint32_t i=0;i<a->scene.helper_count;++i) if(a->scene.helpers[i].bone==bone && a->scene.helpers[i].id==helper_id) {
            if(found!=NONE) { reason="ambiguous helper id on owner bone"; goto failed; }
            found=i;
        }
        if(found==NONE) { reason="helper id absent on owner bone"; goto failed; }
        RwFrameAddChild(a->helpers[found],pointer(child,4)); b->parent=a;
        RwFrameUpdateObjects(a->helpers[found]); return 0;
    }
failed:
    if(error && capacity) snprintf(error,capacity,"%s: bone=%u helper=%d",reason,bone,helper_id);
    return -1;
}
static int play(void *clump, uint32_t model_ordinal, uint32_t physical, const uint8_t record[10], double seconds, int loop, char *error, size_t capacity)
{
    Resource *r=resources;
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN || !isfinite(seconds)) return -1;
    while(r && r->clump!=clump) r=r->next;
    if(!r) { if(error && capacity) snprintf(error,capacity,"playback requires owned Golden resource"); return -1; }
    uint32_t model=NONE;
    for(uint32_t i=0;i<r->scene.source.count;++i) if(r->scene.source.nodes[i].type==3) {
        uint32_t ancestor=i; int neck=0;
        for(;ancestor!=NONE;ancestor=r->scene.source.nodes[ancestor].parent) if(r->scene.source.nodes[ancestor].type==P4_GM_NECK) neck=1;
        if(!neck && model_ordinal--==0) { model=i; break; }
    }
    uint32_t clip=NONE;
    for(uint32_t i=0;i<r->motions.clip_count;++i) if(r->motions.clips[i].model==model && r->motions.clips[i].ordinal==physical) { clip=i; break; }
    if(clip==NONE) { if(error && capacity) snprintf(error,capacity,"physical motion absent: ordinal=%u U08",physical); return -1; }
    float frame=p4_golden_model_motion_frame(&r->motions.clips[clip],seconds,loop);
    if(p4_golden_model_pose(&r->scene,&r->motions,clip,frame,error,capacity)) return -1;
    if(record) memcpy(r->record,record,10); else memset(r->record,0,10);
    for(uint32_t i=0;i<r->scene.bone_count;++i) {
        float rw[16]; matrix(rw,r->scene.bones[i].local); func_003e9cb0(r->native.bones[i],rw,0);
    }
    for(uint32_t i=0;i<r->scene.mesh_count;++i) {
        const P4GoldenModelMesh *mesh=&r->scene.meshes[i];
        if(!mesh->weight_count) continue;
        void *geometry=r->native.geometries[i], *morph=pointer(geometry,92);
        float *positions=pointer(morph,20), *normals=pointer(morph,24);
        for(uint32_t v=0;v<mesh->vertices;++v) {
            memcpy(positions+v*3,mesh->vertex[v].position,12); memcpy(normals+v*3,mesh->vertex[v].normal,12);
        }
        float sphere[4]; RpMorphTargetCalcBoundingSphere(morph,sphere); memcpy((uint8_t *)morph+4,sphere,16);
        /* The existing native atomic cache keys this u16 together with vertex pointers.
         * A new revision prevents stale static VBO reuse, without changing the PS2 path. */
        uint16_t revision; memcpy(&revision,(uint8_t *)geometry+12,2); ++revision; memcpy((uint8_t *)geometry+12,&revision,2);
    }
    RwFrameUpdateObjects(pointer(r->clump,4));
    return 0;
}
int p4_golden_model_play(void *clump, uint32_t model_ordinal, int32_t slot, const uint8_t record[10], double seconds, int loop, char *error, size_t capacity)
{
    if(slot< -1 || slot>31) { if(error && capacity) snprintf(error,capacity,"MODEL0 slot outside -1..31 U08"); return -1; }
    return play(clump,model_ordinal,(uint32_t)(slot+1),record,seconds,loop,error,capacity);
}
int p4_golden_model_play_physical(void *clump, uint32_t model_ordinal, uint32_t physical, double seconds, int loop, char *error, size_t capacity)
{
    return play(clump,model_ordinal,physical,NULL,seconds,loop,error,capacity);
}
static void draw_resource(Resource *r)
{
    mdlDrawVisibleAtomics(r->clump);
    for(Resource *child=resources;child;child=child->next) if(child->parent==r) draw_resource(child);
}
void p4_golden_model_draw(void *clump)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN || !clump) return;
    const uint32_t states[]={P4GL_RS_ZTESTENABLE,P4GL_RS_ZWRITEENABLE,P4GL_RS_VERTEXALPHAENABLE,P4GL_RS_FOGENABLE,P4GL_RS_CULLMODE};
    const uint32_t values[]={1,1,1,0,1};
    uint32_t saved[5], written=p4gl_render_state_written(), blend=0, test=0;
    RpSkyRenderStateGet(2,&blend); RpSkyRenderStateGet(3,&test);
    for(unsigned i=0;i<5;++i) { saved[i]=p4gl_render_state(states[i]); p4gl_render_state_restore(states[i],values[i],UINT32_MAX); }
    /* Standard straight-alpha over; discard zero alpha instead of writing invisible Z.
     * These are adapter defaults, not an asserted mapping of Golden's 8081 enums. */
    RpSkyRenderStateSet(2,0x44); RpSkyRenderStateSet(3,0x0d);
    Resource *resource=resources;
    while(resource && resource->clump!=clump) resource=resource->next;
    if(resource) draw_resource(resource); else mdlDrawVisibleAtomics(clump);
    RpSkyRenderStateSet(2,blend); RpSkyRenderStateSet(3,test);
    for(unsigned i=0;i<5;++i) p4gl_render_state_restore(states[i],saved[i],written);
}
