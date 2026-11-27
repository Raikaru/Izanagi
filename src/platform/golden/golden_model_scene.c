/* Golden skeleton/skin conversion. U01/U09 defaults are explicitly provisional:
 * source coordinates; column-vector T*Rz*Ry*Rx*S; quaternion XYZW.
 * Skinning preserves every lane and the original 127/128 sums. */
#include "p4pc/golden_model.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static uint32_t u32(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static float f32(const uint8_t *p) { uint32_t u=u32(p); float f; memcpy(&f,&u,4); return f; }
static int fail(char *error, size_t capacity, uint32_t at, const char *reason)
{ if(error && capacity) snprintf(error,capacity,"%s at +0x%x",reason,(unsigned)at); return -1; }
static void identity(float *m) { memset(m,0,64); m[0]=m[5]=m[10]=m[15]=1; }
static void multiply(float *out, const float *a, const float *b)
{
    float r[16];
    for(unsigned c=0;c<4;++c) for(unsigned row=0;row<4;++row) {
        float sum=0; for(unsigned k=0;k<4;++k) sum+=a[k*4+row]*b[c*4+k]; r[c*4+row]=sum;
    }
    memcpy(out,r,64);
}
static void transform(float *out, const float *m, const float *v, int point)
{ for(unsigned j=0;j<3;++j) out[j]=m[j]*v[0]+m[4+j]*v[1]+m[8+j]*v[2]+(point?m[12+j]:0); }
static void quaternion(float *m, const float *q)
{
    float x=q[0],y=q[1],z=q[2],w=q[3]; identity(m);
    m[0]=1-2*(y*y+z*z); m[1]=2*(x*y+z*w); m[2]=2*(x*z-y*w);
    m[4]=2*(x*y-z*w); m[5]=1-2*(x*x+z*z); m[6]=2*(y*z+x*w);
    m[8]=2*(x*z+y*w); m[9]=2*(y*z-x*w); m[10]=1-2*(x*x+y*y);
}
static int local(const P4GoldenModel *m, uint32_t node, float *out, float *rest)
{
    float t[3]={0}, scale[3]={1,1,1}, e[3]={0}, q[4]={0,0,0,1}; int have_q=0;
    for(uint32_t i=m->nodes[node].first;i!=NONE;i=m->nodes[i].next) {
        uint32_t type=m->nodes[i].type; const uint8_t *p=m->bytes+m->nodes[i].args;
        float *v=type==0x8048?t:type==0x804c?scale:type==0x8049?e:type==0x804b?q:NULL;
        if(!v) continue;
        unsigned count=type==0x804b?4:3; if(type==0x804b) have_q=1;
        for(unsigned k=0;k<count;++k) { v[k]=f32(p+k*4); if(!isfinite(v[k])) return -1; }
    }
    if(have_q) quaternion(out,q);
    else {
        identity(out);
        for(unsigned axis=0;axis<3;++axis) {
            float r[16]; identity(r); unsigned a=(axis+1)%3,b=(axis+2)%3;
            r[a*4+a]=r[b*4+b]=cosf(e[axis]); r[a*4+b]=sinf(e[axis]); r[b*4+a]=-sinf(e[axis]);
            multiply(out,r,out);
        }
    }
    if(!have_q) {
        float cx=cosf(e[0]*0.5f),sx=sinf(e[0]*0.5f),cy=cosf(e[1]*0.5f),sy=sinf(e[1]*0.5f),cz=cosf(e[2]*0.5f),sz=sinf(e[2]*0.5f);
        q[0]=sx*cy*cz-cx*sy*sz; q[1]=cx*sy*cz+sx*cy*sz;
        q[2]=cx*cy*sz-sx*sy*cz; q[3]=cx*cy*cz+sx*sy*sz;
    }
    memcpy(rest,t,12); memcpy(rest+3,q,16); memcpy(rest+7,scale,12);
    for(unsigned c=0;c<3;++c) for(unsigned row=0;row<3;++row) out[c*4+row]*=scale[c];
    memcpy(out+12,t,12); return 0;
}
static int supplemental(const P4GoldenModel *m, uint32_t i)
{
    for(;i!=NONE;i=m->nodes[i].parent) if(m->nodes[i].type==P4_GM_NECK) return 1;
    return 0;
}
static int world(P4GoldenModelScene *s, uint32_t i, uint8_t *state, unsigned depth, int initialize)
{
    if(state[i]==2) return 0;
    if(state[i]==1 || depth>s->bone_count) return -1;
    state[i]=1;
    P4GoldenModelBone *b=&s->bones[i];
    if(initialize) {
        if(local(&s->source,b->node,b->local,b->rest)) return -1;
        memcpy(b->pose,b->rest,sizeof b->pose);
    } else {
        quaternion(b->local,b->pose+3);
        for(unsigned c=0;c<3;++c) for(unsigned row=0;row<3;++row) b->local[c*4+row]*=b->pose[7+c];
        memcpy(b->local+12,b->pose,12);
    }
    if(b->parent!=NONE) {
        if(world(s,b->parent,state,depth+1,initialize)) return -1;
        multiply(b->world,s->bones[b->parent].world,b->local);
    } else memcpy(b->world,b->local,64);
    state[i]=2; return 0;
}
static int skin_binding(const P4GoldenModelScene *s, P4GoldenModelMesh *mesh, uint32_t draw_bone, char *error, size_t capacity)
{
    const P4GoldenModel *m=&s->source;
    uint32_t subset=p4_golden_model_child(m,mesh->node,0x8062,0);
    uint32_t palette=p4_golden_model_child(m,draw_bone,0x8044,0);
    uint32_t offsets=p4_golden_model_child(m,draw_bone,0x8045,0);
    if(subset==NONE || palette==NONE || offsets==NONE) return fail(error,capacity,m->nodes[mesh->node].offset,"missing static skin binding U04");
    const uint8_t *sub=m->bytes+m->nodes[subset].args, *pal=m->bytes+m->nodes[palette].args, *bind=m->bytes+m->nodes[offsets].args;
    uint32_t lanes=u32(sub), count=u32(pal);
    if(lanes!=mesh->weight_count || lanes>8 || u32(bind)!=count) return fail(error,capacity,m->nodes[mesh->node].offset,"static palette/subset mismatch U04");
    mesh->rest=malloc((size_t)(mesh->vertices?mesh->vertices:1)*sizeof *mesh->rest);
    if(!mesh->rest) return fail(error,capacity,0,"skin rest allocation");
    memcpy(mesh->rest,mesh->vertex,(size_t)mesh->vertices*sizeof *mesh->rest);
    for(uint32_t lane=0;lane<lanes;++lane) {
        uint32_t slot=u32(sub+4+4*lane);
        if(slot>=count) return fail(error,capacity,m->nodes[subset].offset,"static palette index U04");
        uint32_t ref=p4_golden_model_resolve(m,draw_bone,u32(pal+4+slot*4));
        uint32_t bone=ref==NONE?NONE:s->bone_index[ref];
        if(bone==NONE) return fail(error,capacity,m->nodes[palette].offset,"static palette reference U04");
        mesh->joints[lane]=bone;
        for(unsigned j=0;j<16;++j) { mesh->offsets[lane][j]=f32(bind+4+slot*64+j*4); if(!isfinite(mesh->offsets[lane][j])) return -1; }
    }
    mesh->bone=NONE; return 0;
}
static int skin(const P4GoldenModelScene *s, P4GoldenModelMesh *mesh, char *error, size_t capacity)
{
    float matrices[8][16], normals[8][9];
    uint32_t lanes=mesh->weight_count;
    for(uint32_t lane=0;lane<lanes;++lane) {
        multiply(matrices[lane],s->bones[mesh->joints[lane]].world,mesh->offsets[lane]);
        const float *a=matrices[lane];
        /* Inverse transpose for nonuniform static scale, without normalizing weights. */
        float c0[3]={a[5]*a[10]-a[9]*a[6],a[9]*a[2]-a[1]*a[10],a[1]*a[6]-a[5]*a[2]};
        float det=a[0]*c0[0]+a[4]*c0[1]+a[8]*c0[2];
        if(!isfinite(det) || fabsf(det)<1e-20f) return fail(error,capacity,s->source.nodes[mesh->node].offset,"singular skin matrix U04");
        float *n=normals[lane];
        n[0]=c0[0]/det; n[1]=(a[8]*a[6]-a[4]*a[10])/det; n[2]=(a[4]*a[9]-a[8]*a[5])/det;
        n[3]=c0[1]/det; n[4]=(a[0]*a[10]-a[8]*a[2])/det; n[5]=(a[8]*a[1]-a[0]*a[9])/det;
        n[6]=c0[2]/det; n[7]=(a[4]*a[2]-a[0]*a[6])/det; n[8]=(a[0]*a[5]-a[4]*a[1])/det;
    }
    for(uint32_t v=0;v<mesh->vertices;++v) {
        P4GoldenVertex *vertex=&mesh->vertex[v]; const P4GoldenVertex *rest=&mesh->rest[v]; float pos[3]={0}, nrm[3]={0};
        for(uint32_t lane=0;lane<lanes;++lane) {
            float p[3]; transform(p,matrices[lane],rest->position,1);
            const float *n=normals[lane];
            for(unsigned j=0;j<3;++j) { pos[j]+=rest->weights[lane]*p[j]; nrm[j]+=rest->weights[lane]*(n[j]*rest->normal[0]+n[3+j]*rest->normal[1]+n[6+j]*rest->normal[2]); }
        }
        memcpy(vertex->position,pos,12); memcpy(vertex->normal,nrm,12);
    }
    return 0;
}
void p4_golden_model_scene_free(P4GoldenModelScene *s)
{
    if(!s) return;
    for(uint32_t i=0;i<s->mesh_count;++i) { free(s->meshes[i].vertex); free(s->meshes[i].rest); free(s->meshes[i].indices); }
    free(s->meshes); free(s->bones); free(s->helpers); free(s->bone_state); free(s->bone_index); p4_golden_model_free(&s->source); memset(s,0,sizeof *s);
}
int p4_golden_model_scene(const uint8_t *bytes, size_t size, P4GoldenModelScene *out, char *error, size_t capacity)
{
    if(!out || out->source.bytes || out->meshes || out->bones || out->helpers) return -1;
    P4GoldenModelScene s={0};
    if(p4_golden_model_parse(bytes,size,&s.source,error,capacity)) return -1;
    const P4GoldenModel *m=&s.source; uint32_t mesh_count=0, bone_count=0, helpers=0;
    for(uint32_t i=0;i<m->count;++i) if(!supplemental(m,i)) {
        uint32_t type=m->nodes[i].type;
        if(type==4) ++bone_count;
        if(type==0x80f1 && m->nodes[i].size==60 && !memcmp(m->bytes+m->nodes[i].args,"per3Helper\0",11)) ++helpers;
        if(type==0x804e) {
            uint32_t part=p4_golden_model_resolve(m,m->nodes[i].parent,u32(m->bytes+m->nodes[i].args));
            if(part==NONE) { fail(error,capacity,m->nodes[i].offset,"unresolved DrawPart U02"); goto invalid; }
            for(uint32_t child=m->nodes[part].first;child!=NONE;child=m->nodes[child].next) if(m->nodes[child].type==6) ++mesh_count;
        }
    }
    uint64_t allocated=(uint64_t)bone_count*(sizeof *s.bones+1)+(uint64_t)mesh_count*sizeof *s.meshes+(uint64_t)helpers*sizeof *s.helpers+(uint64_t)m->count*sizeof *s.bone_index;
    if(allocated>P4_GOLDEN_MODEL_BUDGET) { fail(error,capacity,0,"decoded hierarchy budget"); goto invalid; }
    s.bones=calloc(bone_count?bone_count:1,sizeof *s.bones); s.meshes=calloc(mesh_count?mesh_count:1,sizeof *s.meshes); s.helpers=calloc(helpers?helpers:1,sizeof *s.helpers);
    s.bone_index=malloc((size_t)m->count*sizeof *s.bone_index);
    if(!s.bones || !s.meshes || !s.helpers || !s.bone_index) goto allocation;
    for(uint32_t i=0;i<m->count;++i) {
        s.bone_index[i]=NONE;
        if(m->nodes[i].type==4 && !supplemental(m,i)) {
            s.bone_index[i]=s.bone_count;
            s.bones[s.bone_count++]=(P4GoldenModelBone){.node=i,.parent=NONE};
        }
    }
    for(uint32_t i=0;i<s.bone_count;++i) {
        uint32_t p=p4_golden_model_child(m,s.bones[i].node,0x8041,0);
        if(p==NONE) continue;
        uint32_t parent=p4_golden_model_resolve(m,s.bones[i].node,u32(m->bytes+m->nodes[p].args));
        s.bones[i].parent=parent==NONE?NONE:s.bone_index[parent];
        if(s.bones[i].parent==NONE) { fail(error,capacity,m->nodes[p].offset,"unresolved bone parent U02"); goto invalid; }
    }
    s.bone_state=calloc(s.bone_count?s.bone_count:1,1); if(!s.bone_state) goto allocation;
    for(uint32_t i=0;i<s.bone_count;++i) if(world(&s,i,s.bone_state,0,1)) { fail(error,capacity,m->nodes[s.bones[i].node].offset,"bone cycle/nonfinite transform U01"); goto invalid; }
    for(uint32_t bone=0;bone<s.bone_count;++bone) {
        uint32_t bn=s.bones[bone].node;
        for(uint32_t command=m->nodes[bn].first;command!=NONE;command=m->nodes[command].next) {
            const P4GoldenModelNode *cmd=&m->nodes[command]; const uint8_t *p=m->bytes+cmd->args;
            if(cmd->type==0x80f1 && cmd->size==60 && !memcmp(p,"per3Helper\0",11)) {
                P4GoldenModelHelper *h=&s.helpers[s.helper_count++]; h->node=command; h->bone=bone; h->id=(int32_t)u32(p+12);
                float q[4], offset[16]; for(unsigned j=0;j<4;++j) q[j]=f32(p+28+4*j);
                quaternion(offset,q); for(unsigned j=0;j<3;++j) offset[12+j]=f32(p+16+4*j);
                multiply(h->world,s.bones[bone].world,offset);
            }
            if(cmd->type!=0x804e) continue;
            uint32_t part=p4_golden_model_resolve(m,bn,u32(p));
            for(uint32_t mesh=m->nodes[part].first;mesh!=NONE;mesh=m->nodes[mesh].next) if(m->nodes[mesh].type==6) {
                P4GoldenModelMesh *dst=&s.meshes[s.mesh_count++]; dst->node=mesh; dst->bone=bone;
                uint32_t mat=p4_golden_model_child(m,mesh,0x8061,0), arrays=NONE; size_t triangles=0;
                dst->material=mat==NONE?NONE:p4_golden_model_resolve(m,mesh,u32(m->bytes+m->nodes[mat].args));
                if(dst->material==NONE) { fail(error,capacity,m->nodes[mesh].offset,"unresolved material U02"); goto invalid; }
                for(uint32_t d=m->nodes[mesh].first;d!=NONE;d=m->nodes[d].next) if(m->nodes[d].type==0x8066) {
                    uint32_t a=p4_golden_model_resolve(m,mesh,u32(m->bytes+m->nodes[d].args)); size_t n=0;
                    if((arrays!=NONE && a!=arrays) || p4_golden_model_triangles(m,d,NULL,0,&n)) { fail(error,capacity,m->nodes[d].offset,"unsupported primitive/reference U03"); goto invalid; }
                    arrays=a; triangles+=n;
                }
                if(arrays==NONE || triangles>UINT32_MAX || triangles>SIZE_MAX/12) { fail(error,capacity,m->nodes[mesh].offset,"empty/oversized draw U03"); goto invalid; }
                const uint8_t *a=m->bytes+m->nodes[arrays].args; P4GoldenVertexLayout layout;
                if(p4_golden_model_layout(u32(a),u32(a+4),u32(a+12),&layout)) goto invalid;
                dst->vertices=layout.count; dst->triangles=(uint32_t)triangles; dst->uv_sets=layout.uv_sets; dst->weight_count=layout.weights;
                uint64_t need=(uint64_t)dst->vertices*sizeof *dst->vertex*(dst->weight_count?2:1)+(uint64_t)triangles*12;
                if(need>P4_GOLDEN_MODEL_BUDGET-allocated) { fail(error,capacity,m->nodes[mesh].offset,"decoded scene budget"); goto invalid; }
                allocated+=need;
                dst->vertex=malloc((size_t)(dst->vertices?dst->vertices:1)*sizeof *dst->vertex); dst->indices=malloc(triangles?triangles*12:1);
                if(!dst->vertex || !dst->indices) goto allocation;
                if(p4_golden_model_vertices(m,arrays,dst->vertex,dst->vertices)) { fail(error,capacity,m->nodes[arrays].offset,"nonfinite vertex U03"); goto invalid; }
                size_t cursor=0;
                for(uint32_t d=m->nodes[mesh].first;d!=NONE;d=m->nodes[d].next) if(m->nodes[d].type==0x8066) {
                    size_t n=0; if(p4_golden_model_triangles(m,d,dst->indices+3*cursor,3*(triangles-cursor),&n)) goto invalid; cursor+=n;
                }
                if(dst->weight_count && (skin_binding(&s,dst,bn,error,capacity) || skin(&s,dst,error,capacity))) goto invalid;
            }
        }
    }
    *out=s; return 0;
allocation:
    fail(error,capacity,0,"scene allocation");
invalid:
    p4_golden_model_scene_free(&s); return -1;
}
int p4_golden_model_pose(P4GoldenModelScene *s, const P4GoldenMotions *bank, uint32_t index, float frame, char *error, size_t capacity)
{
    if(!s || !bank || index>=bank->clip_count || !isfinite(frame)) return fail(error,capacity,0,"invalid motion request");
    const P4GoldenModel *m=&s->source; const P4GoldenMotion *clip=&bank->clips[index];
    if(clip->flags&P4_GM_MOTION_NECK) return fail(error,capacity,m->nodes[clip->node].offset,"supplemental neck execution OPEN U10");
    /* Check before mutating: unresolved sentinels/target semantics never disappear. */
    for(uint32_t i=0;i<clip->count;++i) {
        const P4GoldenChannel *c=&bank->channels[clip->first+i];
        if(c->flags) return fail(error,capacity,m->nodes[c->node].offset,"motion target exception OPEN U02/U07/U10");
        uint32_t type=m->nodes[c->target].type, dimensions=u32(m->bytes+m->nodes[c->curve].args+4);
        int width=c->command==0x4b?4:3;
        if(type!=4 || (c->command!=0x48 && c->command!=0x4b && c->command!=0x4c))
            return fail(error,capacity,m->nodes[c->node].offset,"material/layer motion semantics OPEN U05/U07");
        if((dimensions!=(uint32_t)width || c->option) && (dimensions!=1 || c->option>=(uint32_t)width))
            return fail(error,capacity,m->nodes[c->node].offset,"motion component semantics OPEN U07");
        if(s->bone_index[c->target]==NONE) return fail(error,capacity,m->nodes[c->node].offset,"bone target outside skeleton U02");
    }
    for(uint32_t i=0;i<s->bone_count;++i) if(m->nodes[s->bones[i].node].parent==clip->model) memcpy(s->bones[i].pose,s->bones[i].rest,sizeof s->bones[i].pose);
    for(uint32_t i=0;i<clip->count;++i) {
        const P4GoldenChannel *c=&bank->channels[clip->first+i]; uint32_t bone=s->bone_index[c->target];
        float value[4]; if(p4_golden_model_curve(m,c->curve,frame,value)) return fail(error,capacity,m->nodes[c->curve].offset,"invalid spherical curve U15");
        uint32_t dimensions=u32(m->bytes+m->nodes[c->curve].args+4);
        float *dest=s->bones[bone].pose+(c->command==0x48?0:c->command==0x4b?3:7);
        if(dimensions==1) dest[c->option]=value[0]; else memcpy(dest,value,dimensions*4);
    }
    memset(s->bone_state,0,s->bone_count);
    for(uint32_t i=0;i<s->bone_count;++i) if(world(s,i,s->bone_state,0,0)) return fail(error,capacity,0,"animated hierarchy cycle");
    for(uint32_t i=0;i<s->mesh_count;++i) if(s->meshes[i].weight_count && skin(s,&s->meshes[i],error,capacity)) return -1;
    for(uint32_t i=0;i<s->helper_count;++i) {
        P4GoldenModelHelper *h=&s->helpers[i]; const uint8_t *p=m->bytes+m->nodes[h->node].args;
        float q[4], offset[16]; for(unsigned j=0;j<4;++j) q[j]=f32(p+28+4*j);
        quaternion(offset,q); for(unsigned j=0;j<3;++j) offset[12+j]=f32(p+16+4*j);
        multiply(h->world,s->bones[h->bone].world,offset);
    }
    return 0;
}
int p4_golden_model_static_check(const P4GoldenModelScene *s, char *error, size_t capacity)
{
    if(!s || !s->source.bytes) return fail(error,capacity,0,"missing Golden scene");
    const P4GoldenModel *m=&s->source;
    for(uint32_t i=0;i<m->count;++i) {
        const P4GoldenModelNode *n=&m->nodes[i];
        if(n->type==P4_GM_MEMBER && n->name_size>=7 && !memcmp(m->bytes+n->name,"EFFECT_",7)) {
            if(error && capacity) snprintf(error,capacity,"effect adjunct requires M8f U11: %.*s at +0x%x",(int)n->name_size,(const char *)m->bytes+n->name,n->offset);
            return -1;
        }
    }
    if(!s->mesh_count) return fail(error,capacity,0,"no static draw geometry");
    for(uint32_t i=0;i<s->mesh_count;++i) {
        const P4GoldenModelMesh *mesh=&s->meshes[i];
        uint32_t mat=mesh->material;
        const char *why=NULL;
        if(mesh->uv_sets>1) why="additional UV layer binding U05";
        if(p4_golden_model_child(m,mat,9,1)!=NONE) why="multiple material layers U05";
        uint32_t layer=p4_golden_model_child(m,mat,9,0);
        if(layer!=NONE) {
            uint32_t matrix=p4_golden_model_child(m,layer,0x809a,0);
            if(matrix!=NONE) {
                const uint8_t *p=m->bytes+m->nodes[matrix].args;
                for(unsigned j=0;j<16;++j) if(f32(p+4*j)!=(j%5==0?1.0f:0.0f)) why="nonidentity UV transform U05";
            }
            uint32_t ref=p4_golden_model_child(m,layer,0x8091,0);
            if(ref!=NONE) {
                uint32_t tex=p4_golden_model_resolve(m,layer,u32(m->bytes+m->nodes[ref].args));
                if(tex==NONE || m->nodes[tex].type!=10) why="texture reference U02";
                else if(p4_golden_model_child(m,tex,0x8013,0)==NONE) why="external-only texture U06";
            }
        }
        if(why) {
            const P4GoldenModelNode *n=&m->nodes[mat];
            if(error && capacity) snprintf(error,capacity,"%s: %.*s at +0x%x",why,(int)n->name_size,(const char *)m->bytes+n->name,n->offset);
            return -1;
        }
    }
    return 0;
}
