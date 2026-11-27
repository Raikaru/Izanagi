/* Original synthetic model-format vectors; never game bytes or source-text assertions. */
#include "p4pc/golden_model.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static void w16(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v) { w16(p,v); w16(p+2,v>>16); }
static void wf(uint8_t *p, float f) { uint32_t v; memcpy(&v,&f,4); w32(p,v); }
static uint8_t b[65536]; static uint32_t cursor;
static uint32_t begin(uint16_t type, const char *name, const uint8_t *args, uint32_t argument_bytes, const uint8_t *data, uint32_t data_bytes)
{
    uint32_t at=cursor, name_bytes=name?(uint32_t)strlen(name)+1:0, h=16+((name_bytes+3)&~3u);
    memset(b+at,0,h+argument_bytes+data_bytes); w16(b+at,type); w16(b+at+2,h); w32(b+at+12,h+argument_bytes); w32(b+at+8,h+argument_bytes+data_bytes);
    if(name) memcpy(b+at+16,name,name_bytes);
    if(argument_bytes) memcpy(b+at+h,args,argument_bytes);
    if(data_bytes) memcpy(b+at+h+argument_bytes,data,data_bytes);
    cursor+=h+argument_bytes+data_bytes; return at;
}
static void end(uint32_t at) { w32(b+at+4,cursor-at); }
static void cmd(uint16_t type, const uint8_t *data, uint32_t size)
{ w16(b+cursor,type); w16(b+cursor+2,0); w32(b+cursor+4,8+size); if(size) memcpy(b+cursor+8,data,size); cursor+=8+size; }
static void word(uint16_t type, uint32_t value) { uint8_t p[4]; w32(p,value); cmd(type,p,4); }
static void vector(uint16_t type, float x, float y, float z)
{ uint8_t p[12]; wf(p,x); wf(p+4,y); wf(p+8,z); cmd(type,p,12); }
static uint32_t fixture(int weighted, int cycle, uint32_t mode)
{
    memset(b,0,sizeof b); memcpy(b,"OMG.00.1PSP",11); cursor=16;
    uint32_t root=begin(2,NULL,NULL,0,NULL,0), model=begin(3,"toy",NULL,0,NULL,0), bone=begin(4,"root",NULL,0,NULL,0);
    vector(0x8048,10,20,30); vector(0x804c,1,1,1);
    if(cycle) word(0x8041,0x00041000);
    word(0x804e,0x00051000);
    if(weighted) {
        uint8_t pal[8]={0}; w32(pal,1); w32(pal+4,0x00041000); cmd(0x8044,pal,8);
        uint8_t bind[68]={0}; w32(bind,1); for(unsigned j=0;j<16;++j) wf(bind+4+4*j,j%5==0?1:0); cmd(0x8045,bind,68);
    }
    uint8_t helper[52]={0}; memcpy(helper,"per3Helper",10); w32(helper+12,500); wf(helper+16,1); wf(helper+20,2); wf(helper+24,3); wf(helper+40,1); cmd(0x80f1,helper,52); end(bone);
    uint32_t part=begin(5,"part",NULL,0,NULL,0), mesh=begin(6,"mesh",NULL,0,NULL,0);
    word(0x8061,0x00082000);
    if(weighted) { uint8_t sub[28]={0}; w32(sub,6); cmd(0x8062,sub,28); }
    uint8_t draw[24]={0}; w32(draw,0x00071000); w32(draw+4,mode); w32(draw+8,3); w32(draw+12,1);
    if(!(mode&0x100)) { w16(draw+16,0); w16(draw+18,1); w16(draw+20,2); }
    cmd(0x8066,draw,mode&0x100?20:24); end(mesh);
    uint8_t args[16]={0}, data[68]={0};
    uint32_t format=2|(2<<5)|(2<<7)|(weighted?((1<<9)|(5<<14)):0), stride=weighted?22:16;
    w32(args,format); w32(args+4,3); w32(args+8,1);
    for(unsigned v=0;v<3;++v) {
        uint8_t *p=data+v*stride; if(weighted) { for(unsigned j=0;j<5;++j) p[j]=21; p[5]=23; p+=6; }
        w16(p,16384); w16(p+2,32768); w16(p+4,0); w16(p+6,0); w16(p+8,32767);
        w16(p+10,v==0?0x8000:v==1?32767:0); w16(p+12,v==2?32767:0); w16(p+14,0);
    }
    uint32_t array=begin(7,"arrays",args,16,data,(stride*3+3)&~3u); end(array); end(part);
    uint32_t material=begin(8,"material",NULL,0,NULL,0);
    uint8_t rgba[16]; for(unsigned j=0;j<4;++j) wf(rgba+4*j,1); cmd(0x8082,rgba,16); end(material);
    uint32_t motion=begin(11,"base",NULL,0,NULL,0); uint8_t ca[16]={0}, keys[16]={0}; w32(ca,0x81); w32(ca+4,3); w32(ca+8,2); w16(keys+8,0x3c00);
    uint32_t curve=begin(12,"curve",ca,16,keys,16); end(curve); end(motion); end(model); end(root); return cursor;
}
static void graph_vectors(void)
{
    P4GoldenModel m={0}; P4GoldenModelScene s={0}; char error[256];
    uint32_t size=fixture(0,0,0x103); memset(b+size,0xa5,8);
    assert(!p4_golden_model_parse(b,size+8,&m,error,sizeof error));
    assert(m.nodes[0].suffix==size); uint32_t model=p4_golden_model_child(&m,1,3,0), bone=p4_golden_model_child(&m,model,4,0);
    assert(p4_golden_model_resolve(&m,bone,0x00051000)!=NONE);
    assert(p4_golden_model_resolve(&m,bone,0x00051001)==NONE); assert(p4_golden_model_resolve(&m,bone,UINT32_MAX)==NONE);
    uint32_t part=p4_golden_model_child(&m,model,5,0), arrays=p4_golden_model_child(&m,part,7,0); P4GoldenVertex v;
    assert(!p4_golden_model_vertex(&m,arrays,0,&v)); assert(v.position[0]==-1 && v.uv[0][0]==0.5f && v.uv[0][1]==1);
    p4_golden_model_free(&m);
    assert(!p4_golden_model_scene(b,size,&s,error,sizeof error));
    assert(s.bone_count==1 && s.mesh_count==1 && s.helper_count==1); assert(s.helpers[0].id==500);
    assert(s.helpers[0].world[12]==11 && s.helpers[0].world[13]==22 && s.helpers[0].world[14]==33);
    assert(s.meshes[0].bone==0 && s.meshes[0].triangles==1); p4_golden_model_scene_free(&s);
    for(uint32_t n=0;n<size;++n) assert(p4_golden_model_parse(b,n,&m,error,sizeof error));
    size=fixture(1,0,0x104); assert(!p4_golden_model_scene(b,size,&s,error,sizeof error));
    assert(s.meshes[0].weight_count==6 && s.meshes[0].bone==NONE); assert(fabsf(s.meshes[0].vertex[0].position[0]-9)<1e-6f);
    assert(s.meshes[0].vertex[0].weights[5]==23.0f/128); p4_golden_model_scene_free(&s);
    size=fixture(0,1,0x103); assert(p4_golden_model_scene(b,size,&s,error,sizeof error)); assert(strstr(error,"cycle"));
    size=fixture(0,0,5); assert(!p4_golden_model_scene(b,size,&s,error,sizeof error)); p4_golden_model_scene_free(&s);
    size=fixture(0,0,3); assert(!p4_golden_model_scene(b,size,&s,error,sizeof error)); p4_golden_model_scene_free(&s);
    uint8_t gmo[32]={0}; memcpy(gmo,"OMG.00.1PSP",11); w16(gmo+16,2); w16(gmo+18,16); w32(gmo+20,16); w32(gmo+24,16); w32(gmo+28,16);
    uint8_t amd[64]={0}; memcpy(amd,"CHNK",4); w32(amd+4,1); memcpy(amd+8,"MODEL_DATA",10); w32(amd+24,0x101); w32(amd+28,32); memcpy(amd+32,gmo,32);
    assert(!p4_golden_model_parse(amd,64,&m,error,sizeof error)); p4_golden_model_free(&m);
    uint8_t pac[104]={0}; w32(pac,1); memcpy(pac+4,"toy.AMD",7); w32(pac+36,64); memcpy(pac+40,amd,64);
    assert(!p4_golden_model_parse(pac,104,&m,error,sizeof error)); assert(m.nodes[0].type==P4_GM_PAC2); p4_golden_model_free(&m);
    w32(pac+36,UINT32_MAX); assert(p4_golden_model_parse(pac,104,&m,error,sizeof error));
    uint8_t pak1[320]={0}; memcpy(pak1,"toy.GMO",7); w32(pak1+252,32); memcpy(pak1+256,gmo,32);
    assert(!p4_golden_model_parse(pak1,320,&m,error,sizeof error)); assert(m.nodes[0].type==P4_GM_PAC1); p4_golden_model_free(&m);
    uint8_t neck[100]={0}; memcpy(neck,"CHNK",4); w32(neck+4,1); memcpy(neck+8,"NECK_BENDER",11); w32(neck+24,0x2d0); w32(neck+28,68);
    w32(neck+32,1); memcpy(neck+36,"neck.GMO",8); w32(neck+68,32); memcpy(neck+72,gmo,28);
    assert(p4_golden_model_parse(neck,sizeof neck,&m,error,sizeof error)); /* declared GMO is truncated */
    P4GoldenVertexLayout layout; assert(!p4_golden_model_layout(2|(2<<5)|(2<<7)|(1<<9)|(5<<14),3,0,&layout)); assert(layout.stride==22);
    assert(!p4_golden_model_layout(2|(2<<5)|(2<<7),3,3,&layout)); assert(layout.uv_sets==4 && layout.position==22 && layout.stride==28);
}
static void images(void)
{
    uint8_t image[2048]; P4GoldenTexture t={0};
    for(unsigned type=2;type<=5;++type) {
        memset(image,0,sizeof image); memcpy(image,"TIM2",4); image[4]=3; w16(image+6,1);
        unsigned pixels=type==2?192:type==3?256:type==4?32:64, palette=type==4?64:type==5?1024:0, size=64+pixels+palette;
        w32(image+16,size-16); w32(image+20,palette); w32(image+24,pixels); w16(image+28,48); w16(image+30,palette/4);
        image[33]=1; image[34]=palette?3:0; image[35]=(uint8_t)type; w16(image+36,8); w16(image+38,8);
        if(type==2) { image[64]=7; image[65]=19; image[66]=31; }
        if(type==3) { image[64]=7; image[65]=19; image[66]=31; image[67]=85; }
        if(type==4) { image[64]=0x21; image[64+pixels+4]=7; image[64+pixels+7]=85; image[64+pixels+8]=19; }
        if(type==5) { image[64]=8; image[64+pixels+16*4]=7; image[64+pixels+16*4+3]=85; }
        assert(!p4_golden_model_image(image,size,&t)); assert(t.levels[0].rgba[0]==7);
        assert(t.levels[0].rgba[3]==(type==2?255:85)); if(type==4) assert(t.levels[0].rgba[4]==19);
        free(t.storage); memset(&t,0,sizeof t);
        for(unsigned n=0;n<size;++n) assert(p4_golden_model_image(image,n,&t));
        image[33]=2; assert(p4_golden_model_image(image,size,&t));
    }
    memset(image,0,sizeof image); image[0]='B'; image[1]='M'; w32(image+2,78); w32(image+10,54); w32(image+14,40); w32(image+18,3); w32(image+22,2); w16(image+26,1); w16(image+28,24);
    image[54]=31; image[55]=19; image[56]=7; image[66]=3; image[67]=2; image[68]=1;
    assert(!p4_golden_model_image(image,78,&t)); assert(t.levels[0].rgba[0]==1 && t.levels[0].rgba[12]==7 && t.levels[0].rgba[3]==255); free(t.storage); memset(&t,0,sizeof t);
    w32(image+22,0x80000000); assert(p4_golden_model_image(image,78,&t));
}
static void scalar_vectors(void)
{
    for(unsigned kind=1;kind<=3;++kind) for(unsigned color=4;color<=7;++color) {
        uint8_t bytes[128]={0};
        P4GoldenModelNode node={.type=7,.parent=NONE,.first=NONE,.next=NONE,.args=0,.data=16};
        P4GoldenModel m={bytes,sizeof bytes,&node,1};
        uint32_t format=kind|(color<<2)|(kind<<5)|(kind<<7)|(1<<9)|(5<<14);
        P4GoldenVertexLayout l; assert(!p4_golden_model_layout(format,1,3,&l));
        w32(bytes,format); w32(bytes+4,1); w32(bytes+8,1); w32(bytes+12,3);
        uint8_t *v=bytes+16;
        for(unsigned j=0;j<5;++j) v[l.weight+j]=21; v[l.weight+5]=22; /* sum 127 */
        if(kind==1) { for(unsigned s=0;s<4;++s) v[l.uv+2*s]=128; v[l.position]=128; }
        if(kind==2) { for(unsigned s=0;s<4;++s) w16(v+l.uv+4*s,32768); w16(v+l.position,0x8000); }
        if(kind==3) { for(unsigned s=0;s<4;++s) wf(v+l.uv+8*s,1); wf(v+l.position,-1); }
        if(color==7) { v[l.color]=17; v[l.color+1]=34; v[l.color+2]=51; v[l.color+3]=85; }
        else w16(v+l.color,color==4?31:color==5?0x801f:0x4321);
        P4GoldenVertex decoded, batch;
        assert(!p4_golden_model_vertex(&m,0,0,&decoded)); assert(!p4_golden_model_vertices(&m,0,&batch,1));
        assert(!memcmp(&decoded,&batch,sizeof batch)); assert(decoded.position[0]==-1);
        for(unsigned s=0;s<4;++s) assert(decoded.uv[s][0]==1);
        float sum=0; for(unsigned j=0;j<6;++j) sum+=decoded.weights[j]; assert(sum==127.0f/128);
        assert(decoded.color[0]==(color==6 || color==7?17:255));
        if(color==6) assert(decoded.color[3]==68);
        if(color==7) assert(decoded.color[3]==85);
    }
}
static void primitive_vectors(void)
{
    uint8_t bytes[64]={0}; P4GoldenModelNode nodes[4];
    for(unsigned i=0;i<4;++i) nodes[i]=(P4GoldenModelNode){.parent=NONE,.first=NONE,.next=NONE};
    nodes[0].type=5; nodes[0].first=1;
    nodes[1].type=6; nodes[1].parent=0; nodes[1].first=3; nodes[1].next=2;
    nodes[2].type=7; nodes[2].parent=0; nodes[2].args=32;
    nodes[3].type=0x8066; nodes[3].parent=1; nodes[3].args=0;
    P4GoldenModel m={bytes,sizeof bytes,nodes,4}; uint32_t indices[6]; size_t triangles=0;
    w32(bytes,0x00071000); w32(bytes+4,0x104); w32(bytes+8,4); w32(bytes+12,1); w32(bytes+36,4);
    assert(!p4_golden_model_triangles(&m,3,indices,6,&triangles)); assert(triangles==2);
    const uint32_t strip[]={0,1,2,2,1,3}; assert(!memcmp(indices,strip,sizeof strip));
    w32(bytes+4,5); w16(bytes+16,3); w16(bytes+18,2); w16(bytes+20,1); w16(bytes+22,0);
    const uint32_t fan[]={3,2,1,3,1,0};
    assert(!p4_golden_model_triangles(&m,3,indices,6,&triangles)); assert(!memcmp(indices,fan,sizeof fan));
    w16(bytes+22,4); assert(p4_golden_model_triangles(&m,3,indices,6,&triangles));
    w16(bytes+22,0); assert(p4_golden_model_triangles(&m,3,indices,5,&triangles));
    w32(bytes+4,0x101); assert(p4_golden_model_triangles(&m,3,indices,6,&triangles));
}
int main(void) { graph_vectors(); scalar_vectors(); primitive_vectors(); images(); puts("Golden model grammar/hierarchy/primitive/decoder vectors passed"); return 0; }
