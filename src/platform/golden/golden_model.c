/* Clean-room AMD/GMO/PAC resource grammar, from committed model-formats*.md.
 * Semantic unknowns stay in the graph. Parsing success is not render readiness. */
#include "p4pc/golden_model.h"
#include "p4pc/golden_source.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static uint32_t u16(const uint8_t *p) { return p[0]|(uint32_t)p[1]<<8; }
static uint32_t u32(const uint8_t *p) { return u16(p)|u16(p+2)<<16; }
static float f32(const uint8_t *p) { uint32_t word=u32(p); float f; memcpy(&f,&word,4); return f; }
static uint32_t align_to(uint32_t p, uint32_t a) { return (p+a-1)&~(a-1); }
int p4_golden_model_layout(uint32_t fmt, uint32_t count, uint32_t extra, P4GoldenVertexLayout *out)
{
    static const uint32_t widths[]={0,1,2,4};
    P4GoldenVertexLayout l={0}; uint32_t p=0, alignment=1;
    l.format=fmt; l.count=count;
    l.weight=l.uv=l.color=l.normal=l.position=NONE;
    if(extra>3) return -1;
    uint32_t kind=(fmt>>9)&3, width=widths[kind];
    if(width) { l.weights=((fmt>>14)&7)+1; l.weight=p; p+=width*l.weights; alignment=width; }
    kind=fmt&3; width=widths[kind];
    if(width) { l.uv_sets=extra+1; p=align_to(p,width); l.uv=p; p+=width*2*l.uv_sets; if(width>alignment) alignment=width; }
    else if(extra) return -1;
    kind=(fmt>>2)&7;
    if(kind) { if(kind<4) return -1; width=kind==7?4:2; p=align_to(p,width); l.color=p; p+=width; if(width>alignment) alignment=width; }
    kind=(fmt>>5)&3; width=widths[kind];
    if(width) { p=align_to(p,width); l.normal=p; p+=width*3; if(width>alignment) alignment=width; }
    kind=(fmt>>7)&3; width=widths[kind];
    if(width) { p=align_to(p,width); l.position=p; p+=width*3; if(width>alignment) alignment=width; }
    if(!p) return -1;
    l.stride=align_to(p,alignment); *out=l; return 0;
}
/* Exact measured parent/child set. Unknown extensions are a named refusal, not skipped. */
static int edge(uint32_t p, uint32_t t)
{
    switch(p) {
    case P4_GM_GMO: return t==2;
    case 2: return t==3 || t==0x8021 || t==0x8022 || t==0x8023 || t==0x80f1;
    case 3: return t==4 || t==5 || t==8 || t==10 || t==11 || t==0x8014 || t==0x8015;
    case 4: return t==0x8014 || t==0x8041 || t==0x8044 || t==0x8045 || t==0x8048 || t==0x8049 || t==0x804b || t==0x804c || t==0x804e || t==0x80e2 || t==0x80f1;
    case 5: return t==6 || t==7 || t==0x8014;
    case 6: return t==0x8061 || t==0x8062 || t==0x8066 || t==0x806b;
    case 8: return t==9 || (t>=0x8081 && t<=0x8088) || t==0x80f1;
    case 9: return (t>=0x8091 && t<=0x8094) || (t>=0x8097 && t<=0x809a);
    case 10: return t==0x8012 || t==0x8013;
    case 11: return t==12 || (t>=0x80b1 && t<=0x80b3);
    default: return 0;
    }
}
typedef struct Walk {
    const uint8_t *b; P4GoldenModelNode *nodes; uint32_t count, limit;
    char *error; size_t capacity;
} Walk;
static int bad(Walk *w, uint32_t at, const char *reason)
{ if(w->error && w->capacity) snprintf(w->error,w->capacity,"%s at +0x%x",reason,(unsigned)at); return -1; }
static uint32_t add(Walk *w, uint32_t type, uint32_t parent, uint32_t at, uint32_t size)
{
    uint32_t i=w->count++;
    if(w->nodes) {
        P4GoldenModelNode *n=&w->nodes[i];
        *n=(P4GoldenModelNode){type,parent,NONE,NONE,at,size,at,0,at,at,at,at+size};
    }
    return i;
}
static void link(Walk *w, uint32_t parent, uint32_t previous, uint32_t child)
{
    if(!w->nodes) return;
    if(previous==NONE) w->nodes[parent].first=child; else w->nodes[previous].next=child;
}
static int command(Walk *w, uint32_t at, uint32_t t, uint32_t size)
{
    const uint8_t *p=w->b+at+8; uint32_t n=size-8, expected=0;
    switch(t) {
    case 0x8012: if(!memchr(p,0,n)) return bad(w,at,"unterminated filename"); return 0;
    case 0x8013: if(n<4 || u32(p)>n-4 || n-4-u32(p)>3) return bad(w,at,"embedded image bound"); return 0;
    case 0x8014: expected=24; break;
    case 0x8015: if(n<4) return bad(w,at,"short quantization"); expected=u32(p)==3?20:u32(p)==384?28:0; break;
    case 0x8021: expected=16; break; case 0x8022: expected=12; break; case 0x8023: expected=24; break;
    case 0x8044: case 0x8045: case 0x8062:
        if(n<4 || (uint64_t)u32(p)*(t==0x8045?64:4)+4!=n) return bad(w,at,"counted payload bound"); return 0;
    case 0x8048: case 0x8049: case 0x804c: expected=12; break;
    case 0x804b: case 0x8082: case 0x8084: case 0x8085: case 0x8098: case 0x80b3: expected=16; break;
    case 0x8066: {
        if(n<20) return bad(w,at,"short draw");
        uint64_t count=(uint64_t)u32(p+8)*u32(p+12);
        uint64_t need=u32(p+4)&0x100?20:(16+count*2+3)&~UINT64_C(3);
        if(need!=n) return bad(w,at,"draw payload bound"); return 0;
    }
    case 0x8081: case 0x8097: case 0x80b1: case 0x80e2: expected=8; break;
    case 0x8083: expected=20; break; case 0x8094: expected=12; break; case 0x809a: expected=64; break;
    case 0x80f1:
        if(!memchr(p,0,n)) return bad(w,at,"unterminated custom tag");
        if(n>=11 && !memcmp(p,"per3Helper\0",11) && n!=52) return bad(w,at,"helper payload bound");
        return 0;
    default: expected=4; break;
    }
    return expected && expected==n?0:bad(w,at,"command payload size");
}
static int chunks(Walk *w, uint32_t at, uint32_t end, uint32_t parent, uint32_t parent_type, unsigned depth)
{
    uint32_t previous=NONE;
    if(depth>64) return bad(w,at,"GMO nesting limit");
    while(at<end) {
        if(end-at<8) return bad(w,at,"short GMO header");
        uint32_t t=u16(w->b+at), h=u16(w->b+at+2), s=u32(w->b+at+4);
        if(s<8 || s>end-at || (at|h|s)&3) return bad(w,at,"GMO size/alignment bound");
        if(!edge(parent_type,t)) return bad(w,at,"unmeasured GMO parent/type");
        uint32_t i=add(w,t,parent,at,s); link(w,parent,previous,i); previous=i;
        if(t<0x8000) {
            if(s<16 || h<16 || h>s) return bad(w,at,"short GMO block");
            uint32_t c=u32(w->b+at+8), a=u32(w->b+at+12);
            if(h>a || a>c || c>s || (a|c)&3) return bad(w,at,"GMO region bound");
            if(h>16 && !memchr(w->b+at+16,0,h-16)) return bad(w,at,"unterminated block name");
            if(t==7 || t==12) {
                if(a-h!=16) return bad(w,at,"array/curve argument size");
                const uint8_t *args=w->b+at+h;
                if(t==7) {
                    P4GoldenVertexLayout layout;
                    if(u32(args+8)!=1 || p4_golden_model_layout(u32(args),u32(args+4),u32(args+12),&layout) ||
                       (((uint64_t)layout.stride*layout.count+3)&~UINT64_C(3))!=c-a) return bad(w,at,"vertex buffer bound");
                } else {
                    uint32_t f=u32(args), d=u32(args+4), keys=u32(args+8);
                    if((f!=0x80 && f!=0x81 && f!=0x84) || (d!=1 && d!=3 && d!=4) || !keys || u32(args+12) ||
                       (((uint64_t)keys*(d+1)*2+3)&~UINT64_C(3))!=c-a) return bad(w,at,"curve buffer bound");
                }
            } else if(a!=h || c!=a) return bad(w,at,"unmeasured block argument/data");
            if(w->nodes) {
                P4GoldenModelNode *n=&w->nodes[i]; n->name=at+16; n->name_size=h-16;
                n->args=at+h; n->data=at+a; n->children=at+c;
            }
            if(chunks(w,at+c,at+s,i,t,depth+1)) return -1;
        } else {
            if(h || command(w,at,t,s)) return bad(w,at,"invalid GMO command");
            if(w->nodes) { w->nodes[i].args=at+8; w->nodes[i].data=at+s; w->nodes[i].children=at+s; }
        }
        at+=s;
    }
    return 0;
}
static int gmo(Walk *w, uint32_t at, uint32_t size, uint32_t parent, uint32_t *index)
{
    static const uint8_t signature[16]="OMG.00.1PSP\0\0\0\0";
    if(size<32 || memcmp(w->b+at,signature,16)) return bad(w,at,"GMO signature");
    uint32_t span=u32(w->b+at+20);
    if(u16(w->b+at+16)!=2 || span<16 || span>size-16) return bad(w,at,"GMO root bound");
    uint32_t i=add(w,P4_GM_GMO,parent,at,size); *index=i;
    if(w->nodes) w->nodes[i].suffix=at+16+span;
    return chunks(w,at+16,at+16+span,i,P4_GM_GMO,0);
}
static int archive(Walk *w, uint32_t at, uint32_t size, uint32_t parent, int neck, uint32_t *index)
{
    const uint8_t *b=w->b+at, *unused=NULL; size_t ignored=0;
    uint32_t kind=P4_GM_PAC2, header=36, count=0, pos=4, previous=NONE;
    if(size<4) return bad(w,at,"short counted archive");
    if(neck) { kind=P4_GM_NECK; count=u32(b); }
    else if(p4_golden_pak2_find(b,size,"",&unused,&ignored)>=0) count=u32(b);
    else {
        if(p4_golden_bin_find(b,size,"",&unused,&ignored)<0) return bad(w,at,"unclassified PAC");
        kind=P4_GM_PAC1; header=256; pos=0; count=UINT32_MAX;
    }
    if(count!=UINT32_MAX && count>(size-pos)/header) return bad(w,at,"archive count bound");
    uint32_t i=add(w,kind,parent,at,size); *index=i;
    for(uint32_t k=0;k<count;++k) {
        if(kind==P4_GM_PAC1) {
            int tail=1; for(uint32_t q=pos;q<size;++q) if(b[q]) { tail=0; break; }
            if(tail) break;
        }
        if(header>size-pos) return bad(w,at+pos,"short archive member");
        uint32_t ns=header-4, n=u32(b+pos+ns), body=pos+header;
        if(!memchr(b+pos,0,ns) || n>size-body) return bad(w,at+pos,"archive member bound/name");
        uint32_t child;
        if(neck && n>=4 && !memcmp(b+body,"OMG.",4)) {
            if(gmo(w,at+body,n,i,&child)) return -1;
        } else child=add(w,P4_GM_MEMBER,i,at+body,n);
        if(w->nodes) { w->nodes[child].name=at+pos; w->nodes[child].name_size=ns; }
        link(w,i,previous,child); previous=child; pos=body+n;
        if(kind==P4_GM_PAC1) { uint32_t next=align_to(pos,64); if(next>size) return bad(w,at+pos,"PAK alignment"); pos=next; }
        if(neck && k+1==count) break;
    }
    if(w->nodes) w->nodes[i].suffix=at+pos;
    return 0;
}
static int walk(Walk *w, uint32_t size)
{
    uint32_t root;
    if(size>=4 && !memcmp(w->b,"OMG.",4)) return gmo(w,0,size,NONE,&root);
    if(size<8 || memcmp(w->b,"CHNK",4)) return archive(w,0,size,NONE,0,&root);
    uint32_t count=u32(w->b+4), pos=8, previous=NONE;
    if(count>(size-8)/24) return bad(w,0,"AMD count bound");
    uint32_t i=add(w,P4_GM_AMD,NONE,0,size);
    for(uint32_t k=0;k<count;++k) {
        if(size-pos<24) return bad(w,pos,"short AMD member");
        uint32_t type=u32(w->b+pos+16), n=u32(w->b+pos+20), body=pos+24, child;
        if(!memchr(w->b+pos,0,16) || n>size-body) return bad(w,pos,"AMD member bound/name");
        if(type==0x101 && !memcmp(w->b+pos,"MODEL_DATA\0",11)) {
            if(gmo(w,body,n,i,&child)) return -1;
        } else if(type==0x2d0 && !memcmp(w->b+pos,"NECK_BENDER\0",12)) {
            if(archive(w,body,n,i,1,&child)) return -1;
        } else {
            const char *name=type==0x2e0 || type==0x2e2?"EFFECT_EPL":type==0x2e3?"EFFECT_TRACK":type==0x2e4?"EFFECT_ADDMOT":type==0x2e5?"EFFECT_EDGE":NULL;
            if(!name || memcmp(w->b+pos,name,strlen(name)+1)) return bad(w,pos,"unmeasured AMD member");
            child=add(w,P4_GM_MEMBER,i,body,n);
            if(w->nodes) w->nodes[child].args=pos+16; /* retains distinct AMD tag */
        }
        if(w->nodes) { w->nodes[child].name=pos; w->nodes[child].name_size=16; }
        link(w,i,previous,child); previous=child; pos=body+n;
    }
    if(w->nodes) w->nodes[i].suffix=pos;
    return 0;
}
int p4_golden_model_parse(const uint8_t *b, size_t size, P4GoldenModel *out, char *error, size_t capacity)
{
    if(!b || !out || out->bytes || out->nodes || size<4 || size>P4_GOLDEN_MODEL_BUDGET) return -1;
    Walk w={b,NULL,0,0,error,capacity};
    if(walk(&w,(uint32_t)size)) return -1;
    if(w.count>(P4_GOLDEN_MODEL_BUDGET-size)/sizeof(P4GoldenModelNode)) return bad(&w,0,"model graph budget");
    P4GoldenModel m={0}; m.bytes=malloc(size); m.nodes=malloc((size_t)w.count*sizeof *m.nodes);
    if(!m.bytes || !m.nodes) { p4_golden_model_free(&m); return bad(&w,0,"model allocation"); }
    memcpy(m.bytes,b,size); m.size=size; m.count=w.count;
    w.b=m.bytes; w.nodes=m.nodes; w.limit=m.count; w.count=0;
    if(walk(&w,(uint32_t)size)) { p4_golden_model_free(&m); return -1; }
    *out=m; return 0;
}
void p4_golden_model_free(P4GoldenModel *m)
{ if(m) { free(m->bytes); free(m->nodes); memset(m,0,sizeof *m); } }
uint32_t p4_golden_model_child(const P4GoldenModel *m, uint32_t parent, uint32_t type, uint32_t ordinal)
{
    if(!m || parent>=m->count) return NONE;
    for(uint32_t i=m->nodes[parent].first;i!=NONE;i=m->nodes[i].next)
        if(m->nodes[i].type==type && !ordinal--) return i;
    return NONE;
}
uint32_t p4_golden_model_resolve(const P4GoldenModel *m, uint32_t scope, uint32_t ref)
{
    if(!m || ref==UINT32_MAX || scope>=m->count) return NONE;
    for(uint32_t level=(ref>>12)&15;level && scope!=NONE;--level) scope=m->nodes[scope].parent;
    return p4_golden_model_child(m,scope,ref>>16,ref&4095);
}
static float scalar(const uint8_t *p, unsigned kind, int signed_value)
{
    if(kind==3) return f32(p);
    if(kind==2) return signed_value?(float)(int16_t)u16(p)/32768.0f:(float)u16(p)/32768.0f;
    return signed_value?(float)(int8_t)*p/128.0f:(float)*p/128.0f;
}
typedef struct VertexContext {
    P4GoldenVertexLayout layout;
    const uint8_t *data;
    float position_offset[3], position_scale[3], uv_offset[2], uv_scale[2];
} VertexContext;
static int vertex_context(const P4GoldenModel *m, uint32_t arrays, VertexContext *context)
{
    if(!m || arrays>=m->count || m->nodes[arrays].type!=7) return -1;
    const P4GoldenModelNode *n=&m->nodes[arrays]; const uint8_t *a=m->bytes+n->args;
    memset(context,0,sizeof *context); context->data=m->bytes+n->data;
    for(unsigned j=0;j<3;++j) context->position_scale[j]=1;
    for(unsigned j=0;j<2;++j) context->uv_scale[j]=1;
    if(p4_golden_model_layout(u32(a),u32(a+4),u32(a+12),&context->layout)) return -1;
    int position=0, uv=0;
    for(uint32_t scope=n->parent;scope!=NONE;scope=m->nodes[scope].parent) {
        for(uint32_t i=m->nodes[scope].first;i!=NONE;i=m->nodes[i].next) if(m->nodes[i].type==0x8015) {
            const uint8_t *q=m->bytes+m->nodes[i].args; uint32_t mask=u32(q);
            if(mask==384 && !position) {
                for(unsigned j=0;j<3;++j) { context->position_offset[j]=f32(q+4+j*4); context->position_scale[j]=f32(q+16+j*4); }
                position=1;
            }
            if(mask==3 && !uv) {
                for(unsigned j=0;j<2;++j) { context->uv_offset[j]=f32(q+4+j*4); context->uv_scale[j]=f32(q+12+j*4); }
                uv=1;
            }
        }
        if(m->nodes[scope].type==3) break;
    }
    return 0;
}
static int decode_vertex(const VertexContext *context, uint32_t index, P4GoldenVertex *out)
{
    static const unsigned width[]={0,1,2,4};
    const P4GoldenVertexLayout l=context->layout;
    const uint8_t *v=context->data+(size_t)index*l.stride;
    P4GoldenVertex o={0}; o.color[0]=o.color[1]=o.color[2]=o.color[3]=255;
    unsigned kind=(l.format>>9)&3;
    for(uint32_t j=0;j<l.weights;++j) o.weights[j]=scalar(v+l.weight+j*width[kind],kind,0);
    kind=l.format&3;
    for(uint32_t s=0;s<l.uv_sets;++s) for(unsigned j=0;j<2;++j) o.uv[s][j]=scalar(v+l.uv+(s*2+j)*width[kind],kind,0);
    kind=(l.format>>2)&7;
    if(kind==7) memcpy(o.color,v+l.color,4);
    else if(kind) {
        uint32_t c=u16(v+l.color), r=c&31, g=(c>>5)&(kind==4?63:31), b=c>>(kind==4?11:10)&31;
        if(kind==6) { for(unsigned j=0;j<4;++j) o.color[j]=(uint8_t)(((c>>(4*j))&15)*17); }
        else { o.color[0]=(uint8_t)(r*255/31); o.color[1]=(uint8_t)(g*255/(kind==4?63:31)); o.color[2]=(uint8_t)(b*255/31); o.color[3]=kind==5?(c&0x8000?255:0):255; }
    }
    kind=(l.format>>5)&3;
    if(kind) for(unsigned j=0;j<3;++j) o.normal[j]=scalar(v+l.normal+j*width[kind],kind,1);
    kind=(l.format>>7)&3;
    if(kind) for(unsigned j=0;j<3;++j) o.position[j]=scalar(v+l.position+j*width[kind],kind,1);
    for(unsigned j=0;j<3;++j) o.position[j]=context->position_offset[j]+context->position_scale[j]*o.position[j];
    for(uint32_t s=0;s<l.uv_sets;++s) for(unsigned j=0;j<2;++j) o.uv[s][j]=context->uv_offset[j]+context->uv_scale[j]*o.uv[s][j];
    for(unsigned j=0;j<3;++j) if(!isfinite(o.position[j]) || !isfinite(o.normal[j])) return -1;
    for(unsigned s=0;s<4;++s) for(unsigned j=0;j<2;++j) if(!isfinite(o.uv[s][j])) return -1;
    *out=o; return 0;
}
int p4_golden_model_vertex(const P4GoldenModel *m, uint32_t arrays, uint32_t index, P4GoldenVertex *out)
{
    VertexContext context;
    if(!out || vertex_context(m,arrays,&context) || index>=context.layout.count) return -1;
    return decode_vertex(&context,index,out);
}
int p4_golden_model_vertices(const P4GoldenModel *m, uint32_t arrays, P4GoldenVertex *out, uint32_t count)
{
    VertexContext context;
    if(!out || vertex_context(m,arrays,&context) || count!=context.layout.count) return -1;
    for(uint32_t v=0;v<count;++v) if(decode_vertex(&context,v,&out[v])) return -1;
    return 0;
}
int p4_golden_model_triangles(const P4GoldenModel *m, uint32_t draw, uint32_t *out, size_t capacity, size_t *triangles)
{
    if(!m || draw>=m->count || m->nodes[draw].type!=0x8066 || !triangles) return -1;
    const uint8_t *p=m->bytes+m->nodes[draw].args;
    uint32_t arrays=p4_golden_model_resolve(m,m->nodes[draw].parent,u32(p));
    if(arrays==NONE || m->nodes[arrays].type!=7) return -1;
    uint32_t vertices=u32(m->bytes+m->nodes[arrays].args+4), mode=u32(p+4), unit=u32(p+8), groups=u32(p+12), low=mode&255;
    if(mode!=0x103 && mode!=0x104 && mode!=3 && mode!=5) return -1;
    if(unit<3 || (low==3 && unit%3)) return -1;
    uint64_t count=(uint64_t)groups*(low==3?unit/3:unit-2);
    if(count>SIZE_MAX/3 || (out && count>capacity/3)) return -1;
    uint32_t start=u32(p+16); size_t cursor=0;
    for(uint32_t group=0;group<groups;++group) {
        for(uint32_t v=0;v<unit;++v) {
            uint64_t ordinal=(uint64_t)group*unit+v;
            uint64_t ix=mode&0x100?(uint64_t)start+ordinal:u16(p+16+(size_t)ordinal*2);
            if(ix>=vertices) return -1;
        }
        uint32_t total=low==3?unit/3:unit-2;
        for(uint32_t t=0;t<total;++t) {
            uint32_t ord[3];
            if(low==3) { ord[0]=3*t; ord[1]=3*t+1; ord[2]=3*t+2; }
            else if(low==4) { ord[0]=t+(t&1); ord[1]=t+1-(t&1); ord[2]=t+2; }
            else { ord[0]=0; ord[1]=t+1; ord[2]=t+2; }
            for(unsigned k=0;k<3;++k) { size_t o=(size_t)group*unit+ord[k]; if(out) out[cursor]=(mode&0x100)?start+(uint32_t)o:u16(p+16+o*2); ++cursor; }
        }
    }
    *triangles=(size_t)count; return 0;
}
