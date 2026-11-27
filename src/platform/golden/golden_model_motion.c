/* Clean-room GMO motion bank. The committed spec is the sole format source.
 * U07/U08 defaults: clamp endpoints, step constant, shortest normalized slerp,
 * file-order clips (base is distinct). Unresolved semantics remain explicit flags. */
#include "p4pc/golden_model.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static uint32_t u32(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static float f32(const uint8_t *p) { uint32_t u=u32(p); float f; memcpy(&f,&u,4); return f; }
static int fail(char *error, size_t capacity, uint32_t at, const char *why)
{ if(error && capacity) snprintf(error,capacity,"%s at +0x%x",why,at); return -1; }
static float half(const uint8_t *p)
{
    unsigned h=p[0]|(unsigned)p[1]<<8, exponent=(h>>10)&31, fraction=h&1023;
    float f=exponent==31?(fraction?NAN:INFINITY):ldexpf((float)(exponent?1024+fraction:fraction),exponent?(int)exponent-25:-24);
    return h&0x8000?-f:f;
}
static float scalar(const P4GoldenModel *m, const P4GoldenModelNode *n, uint32_t key, uint32_t component)
{
    uint32_t dimensions=u32(m->bytes+n->args+4);
    return half(m->bytes+n->data+2*((dimensions+1)*key+component));
}
int p4_golden_model_curve_key(const P4GoldenModel *m, uint32_t curve, uint32_t key, float *time, float value[4])
{
    if(!m || curve>=m->count || m->nodes[curve].type!=12 || !time || !value) return -1;
    const P4GoldenModelNode *n=&m->nodes[curve]; const uint8_t *a=m->bytes+n->args;
    if(key>=u32(a+8)) return -1;
    *time=scalar(m,n,key,0);
    for(uint32_t j=0;j<u32(a+4);++j) value[j]=scalar(m,n,key,j+1);
    return 0;
}
int p4_golden_model_curve(const P4GoldenModel *m, uint32_t curve, float frame, float value[4])
{
    if(!m || curve>=m->count || m->nodes[curve].type!=12 || !isfinite(frame) || !value) return -1;
    const P4GoldenModelNode *n=&m->nodes[curve]; const uint8_t *a=m->bytes+n->args;
    uint32_t format=u32(a), dimensions=u32(a+4), keys=u32(a+8);
    const uint8_t *first=m->bytes+n->data;
    uint32_t initial=(keys>1 && first[0]==255 && first[1]==255)?1:0, lo=initial, hi=keys;
    /* Upper bound gives the last duplicate key at an exact half-time endpoint. */
    while(lo<hi) { uint32_t mid=lo+(hi-lo)/2; if(scalar(m,n,mid,0)<=frame) lo=mid+1; else hi=mid; }
    uint32_t left=lo>initial?lo-1:initial, right=lo<keys?lo:keys-1;
    float t0=scalar(m,n,left,0), t1=scalar(m,n,right,0), blend=t1>t0?(frame-t0)/(t1-t0):0;
    if(blend<0) blend=0;
    if(blend>1) blend=1;
    float x[4]={0}, y[4]={0};
    for(uint32_t j=0;j<dimensions;++j) { x[j]=scalar(m,n,left,j+1); y[j]=scalar(m,n,right,j+1); }
    if(format==0x84) {
        if(dimensions!=4) return -1;
        float nx=0,ny=0,dot=0;
        for(unsigned j=0;j<4;++j) { nx+=x[j]*x[j]; ny+=y[j]*y[j]; }
        if(nx<1e-20f || ny<1e-20f) return -1;
        nx=1/sqrtf(nx); ny=1/sqrtf(ny);
        for(unsigned j=0;j<4;++j) { x[j]*=nx; y[j]*=ny; dot+=x[j]*y[j]; }
        if(dot<0) { dot=-dot; for(unsigned j=0;j<4;++j) y[j]=-y[j]; }
        if(dot>1) dot=1;
        float wx=1-blend, wy=blend;
        if(dot<0.9995f) { float angle=acosf(dot), divisor=sinf(angle); wx=sinf((1-blend)*angle)/divisor; wy=sinf(blend*angle)/divisor; }
        float norm=0;
        for(unsigned j=0;j<4;++j) { value[j]=wx*x[j]+wy*y[j]; norm+=value[j]*value[j]; }
        if(norm<1e-20f) return -1;
        norm=1/sqrtf(norm); for(unsigned j=0;j<4;++j) value[j]*=norm;
    } else if(format==0x80 || format==0x81) {
        for(uint32_t j=0;j<dimensions;++j) value[j]=format==0x80?x[j]:x[j]+blend*(y[j]-x[j]);
    } else return -1;
    return 0;
}
static int supplemental(const P4GoldenModel *m, uint32_t node)
{ for(;node!=NONE;node=m->nodes[node].parent) if(m->nodes[node].type==P4_GM_NECK) return 1; return 0; }
void p4_golden_model_motions_free(P4GoldenMotions *bank)
{ if(bank) { free(bank->clips); free(bank->channels); memset(bank,0,sizeof *bank); } }
int p4_golden_model_motions(const P4GoldenModel *m, P4GoldenMotions *out, char *error, size_t capacity)
{
    if(!m || !out || out->clips || out->channels) return -1;
    P4GoldenMotions bank={0};
    for(uint32_t i=0;i<m->count;++i) {
        if(m->nodes[i].type==11) ++bank.clip_count;
        if(m->nodes[i].type==0x80b3) ++bank.channel_count;
        if(m->nodes[i].type==12) {
            const uint8_t *a=m->bytes+m->nodes[i].args; float previous=-INFINITY;
            for(uint32_t k=0;k<u32(a+8);++k) {
                float time=scalar(m,&m->nodes[i],k,0);
                /* Steam measurement: leading time FFFF exists in material curves.
                 * Preserve it as an OPEN pre-start key, not a corrupt whole resource. */
                const uint8_t *raw=m->bytes+m->nodes[i].data;
                int sentinel=k==0 && raw[0]==255 && raw[1]==255;
                if(!sentinel && (!isfinite(time) || time<previous)) return fail(error,capacity,m->nodes[i].offset,"nonfinite/decreasing curve time U15");
                if(!sentinel) previous=time;
                for(uint32_t j=1;j<=u32(a+4);++j) if(!isfinite(scalar(m,&m->nodes[i],k,j))) return fail(error,capacity,m->nodes[i].offset,"nonfinite curve value U15");
            }
        }
    }
    if((uint64_t)bank.clip_count*sizeof *bank.clips+(uint64_t)bank.channel_count*sizeof *bank.channels>P4_GOLDEN_MODEL_BUDGET) return fail(error,capacity,0,"motion bank budget");
    bank.clips=calloc(bank.clip_count?bank.clip_count:1,sizeof *bank.clips);
    bank.channels=calloc(bank.channel_count?bank.channel_count:1,sizeof *bank.channels);
    if(!bank.clips || !bank.channels) { p4_golden_model_motions_free(&bank); return fail(error,capacity,0,"motion bank allocation"); }
    uint32_t ci=0, channel=0, previous_model=NONE, ordinal=0;
    for(uint32_t i=0;i<m->count;++i) if(m->nodes[i].type==11) {
        P4GoldenMotion *clip=&bank.clips[ci++]; clip->node=i; clip->model=m->nodes[i].parent;
        if(clip->model!=previous_model) ordinal=0;
        previous_model=clip->model; clip->ordinal=ordinal++; clip->first=channel; clip->fps=30;
        if(supplemental(m,i)) clip->flags|=P4_GM_MOTION_NECK;
        for(uint32_t n=m->nodes[i].first;n!=NONE;n=m->nodes[n].next) {
            const uint8_t *p=m->bytes+m->nodes[n].args;
            switch(m->nodes[n].type) {
            case 0x80b1: clip->start=f32(p); clip->end=f32(p+4); break;
            case 0x80b2: clip->fps=f32(p); break;
            case 0x80b3: {
                P4GoldenChannel *c=&bank.channels[channel++]; c->node=n; c->reference=u32(p); c->command=u32(p+4); c->option=u32(p+8);
                c->curve=p4_golden_model_resolve(m,i,u32(p+12)); c->target=p4_golden_model_resolve(m,i,c->reference);
                if(c->curve==NONE || m->nodes[c->curve].type!=12) { fail(error,capacity,m->nodes[n].offset,"unresolved motion curve U02"); goto invalid; }
                if(c->reference==NONE) c->flags|=P4_GM_CHANNEL_SENTINEL;
                else if(c->target==NONE) c->flags|=P4_GM_CHANNEL_EXTERNAL;
                if(clip->flags&P4_GM_MOTION_NECK) c->flags|=P4_GM_CHANNEL_NECK;
                const uint8_t *first=m->bytes+m->nodes[c->curve].data;
                if(first[0]==255 && first[1]==255) c->flags|=P4_GM_CHANNEL_TIME_SENTINEL;
                ++clip->count; break;
            }
            default: break;
            }
        }
        if(!isfinite(clip->start) || !isfinite(clip->end) || clip->end<clip->start || !isfinite(clip->fps) || clip->fps<=0) { fail(error,capacity,m->nodes[i].offset,"invalid motion interval/rate U07"); goto invalid; }
    }
    *out=bank; return 0;
invalid:
    p4_golden_model_motions_free(&bank); return -1;
}
uint32_t p4_golden_model_clip(const P4GoldenMotions *bank, uint32_t model, int32_t slot)
{
    if(!bank || slot< -1 || slot>31) return NONE;
    /* Provisional U08: base=-1, slot s is physical ordinal s+1. Never parse suffixes. */
    for(uint32_t i=0;i<bank->clip_count;++i) if(bank->clips[i].model==model && bank->clips[i].ordinal==(uint32_t)(slot+1)) return i;
    return NONE;
}
float p4_golden_model_motion_frame(const P4GoldenMotion *clip, double seconds, int loop)
{
    double span=(double)clip->end-clip->start, frame=seconds*clip->fps;
    if(loop && span>0) { frame=fmod(frame,span); if(frame<0) frame+=span; }
    else { if(frame<0) frame=0; if(frame>span) frame=span; }
    return clip->start+(float)frame;
}
