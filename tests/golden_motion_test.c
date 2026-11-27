/* Original synthetic vectors; no game data or source-text assertions. */
#include "p4pc/golden_model.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define NONE P4_GOLDEN_MODEL_NONE
static uint8_t b[65536]; static uint32_t cursor;
static void w16(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v) { w16(p,v); w16(p+2,v>>16); }
static void wf(uint8_t *p, float f) { uint32_t v; memcpy(&v,&f,4); w32(p,v); }
static uint32_t begin(uint16_t type, const char *name, const uint8_t *args, uint32_t argument_bytes, const uint8_t *data, uint32_t data_bytes)
{
    uint32_t at=cursor, names=name?(uint32_t)strlen(name)+1:0, h=16+((names+3)&~3u);
    memset(b+at,0,h+argument_bytes+data_bytes); w16(b+at,type); w16(b+at+2,h); w32(b+at+12,h+argument_bytes); w32(b+at+8,h+argument_bytes+data_bytes);
    if(name) memcpy(b+at+16,name,names);
    if(argument_bytes) memcpy(b+at+h,args,argument_bytes);
    if(data_bytes) memcpy(b+at+h+argument_bytes,data,data_bytes);
    cursor+=h+argument_bytes+data_bytes; return at;
}
static void end(uint32_t at) { w32(b+at+4,cursor-at); }
static void cmd(uint16_t type, const uint8_t *p, uint32_t n)
{ w16(b+cursor,type); w16(b+cursor+2,0); w32(b+cursor+4,n+8); memcpy(b+cursor+8,p,n); cursor+=n+8; }
static void word(uint16_t type, uint32_t v) { uint8_t p[4]; w32(p,v); cmd(type,p,4); }
static void vector(uint16_t type, float x,float y,float z)
{ uint8_t p[12]; wf(p,x); wf(p+4,y); wf(p+8,z); cmd(type,p,12); }
static uint32_t curve(uint32_t format, uint32_t dimensions, uint32_t keys, const uint16_t *values)
{
    uint8_t a[16]={0}, data[64]={0}; w32(a,format); w32(a+4,dimensions); w32(a+8,keys);
    uint32_t length=2*keys*(dimensions+1); assert(length<=sizeof data);
    for(uint32_t i=0;i<keys*(dimensions+1);++i) w16(data+2*i,values[i]);
    uint32_t at=begin(12,"curve",a,16,data,(length+3)&~3u); end(at); return at;
}
static void binding(uint32_t target, uint32_t command, uint32_t option, uint32_t ordinal)
{ uint8_t p[16]; w32(p,target); w32(p+4,command); w32(p+8,option); w32(p+12,0xc0000|ordinal); cmd(0x80b3,p,16); }
static uint32_t fixture(void)
{
    memset(b,0,sizeof b); memcpy(b,"OMG.00.1PSP",11); cursor=16;
    uint32_t root=begin(2,NULL,NULL,0,NULL,0), model=begin(3,"toy",NULL,0,NULL,0);
    for(uint32_t i=0;i<7;++i) {
        uint32_t bone=begin(4,"bone",NULL,0,NULL,0); vector(0x8048,(float)i,0,0); vector(0x804c,2,1,1);
        if(i) word(0x8041,0x41000);
        if(!i) {
            word(0x804e,0x51000); uint8_t palette[28]={0}, offsets[388]={0}; w32(palette,6); w32(offsets,6);
            for(uint32_t j=0;j<6;++j) { w32(palette+4+4*j,0x41001+j); for(unsigned k=0;k<16;++k) wf(offsets+4+64*j+4*k,k%5==0?1:0); }
            cmd(0x8044,palette,sizeof palette); cmd(0x8045,offsets,sizeof offsets);
        }
        if(i==6) { uint8_t helper[52]={0}; memcpy(helper,"per3Helper",10); w32(helper+12,500); wf(helper+16,1); wf(helper+40,1); cmd(0x80f1,helper,sizeof helper); }
        end(bone);
    }
    uint32_t part=begin(5,"part",NULL,0,NULL,0), mesh=begin(6,"mesh",NULL,0,NULL,0);
    word(0x8061,0x82000); uint8_t subset[28]={0}; w32(subset,6);
    for(uint32_t i=0;i<6;++i) w32(subset+4+4*i,5-i);
    cmd(0x8062,subset,sizeof subset); uint8_t draw[20]={0}; w32(draw,0x71000); w32(draw+4,0x103); w32(draw+8,3); w32(draw+12,1); cmd(0x8066,draw,sizeof draw); end(mesh);
    uint8_t args[16]={0}, data[56]={0}; uint32_t format=(1<<9)|(5<<14)|(2<<5)|(2<<7);
    w32(args,format); w32(args+4,3); w32(args+8,1);
    for(unsigned i=0;i<3;++i) { uint8_t *p=data+18*i; for(unsigned j=0;j<5;++j) p[j]=20; p[5]=27; w16(p+6,16384); w16(p+12,16384); w16(p+14,i==2?16384:0); }
    uint32_t arrays=begin(7,"arrays",args,sizeof args,data,sizeof data); end(arrays); end(part);
    uint32_t mat=begin(8,"material",NULL,0,NULL,0); end(mat);
    for(unsigned slot=0;slot<35;++slot) {
        /* Deliberately irregular names: file order, never suffix parsing. */
        uint32_t motion=begin(11,slot==0?"base":slot==2?"another-base":"odd-0099",NULL,0,NULL,0);
        uint8_t interval[8]={0}; wf(interval+4,1); cmd(0x80b1,interval,8); uint8_t rate[4]; wf(rate,30); cmd(0x80b2,rate,4);
        if(slot!=3) {
            binding(0x41006,0x48,0,0);
            const uint16_t translation[]={0,0x4600,0,0,0x3c00,0x4900,0,0}; curve(0x81,3,2,translation);
        }
        end(motion);
    }
    end(model); end(root); return cursor;
}
static void near(float a, float value) { assert(fabsf(a-value)<0.00002f); }
static void animation_skin(void)
{
    uint32_t size=fixture(); char error[256]=""; P4GoldenModelScene s={0}; P4GoldenMotions bank={0};
    assert(!p4_golden_model_scene(b,size,&s,error,sizeof error)); assert(!p4_golden_model_motions(&s.source,&bank,error,sizeof error));
    assert(bank.clip_count==35 && bank.channel_count==34); assert(s.bone_count==7 && s.meshes[0].weight_count==6);
    uint32_t model=bank.clips[0].model; assert(p4_golden_model_clip(&bank,model,-1)==0);
    for(int slot=0;slot<32;++slot) assert(p4_golden_model_clip(&bank,model,slot)==(uint32_t)slot+1);
    assert(p4_golden_model_clip(&bank,model,32)==NONE);
    assert(p4_golden_model_clip(&bank,model+1,0)==NONE);
    for(unsigned j=0;j<6;++j) assert(s.meshes[0].joints[j]==6-j);
    float expected=0; for(unsigned j=0;j<6;++j) expected+=(j==5?27.0f:20.0f)/128*(2+2*(6-j));
    near(s.meshes[0].vertex[0].position[0],expected);
    near(s.meshes[0].vertex[0].normal[0],0.5f*0.25f*127/128);
    assert(!p4_golden_model_pose(&s,&bank,0,0.5f,error,sizeof error));
    near(s.meshes[0].vertex[0].position[0],expected+4*20.0f/128);
    near(s.helpers[0].world[12],20); /* root scale 2, child translation 8, child scale 2, helper x=1 */
    for(unsigned j=0;j<20;++j) assert(!p4_golden_model_pose(&s,&bank,0,0.5f,error,sizeof error));
    near(s.meshes[0].vertex[0].position[0],expected+4*20.0f/128); /* no cumulative rebake */
    assert(!p4_golden_model_pose(&s,&bank,3,0.5f,error,sizeof error)); near(s.meshes[0].vertex[0].position[0],expected); /* empty is rest, not alias */
    for(int slot=27;slot<32;++slot) { assert(!p4_golden_model_pose(&s,&bank,p4_golden_model_clip(&bank,model,slot),1,error,sizeof error)); near(s.meshes[0].vertex[0].position[0],expected+8*20.0f/128); }
    near(p4_golden_model_motion_frame(&bank.clips[0],1.0/60,0),0.5f);
    near(p4_golden_model_motion_frame(&bank.clips[0],1.0/30,0),1);
    near(p4_golden_model_motion_frame(&bank.clips[0],1.0/30,1),0);
    near(p4_golden_model_motion_frame(&bank.clips[0],-1.0/60,1),0.5f);
    /* Scale is an independent channel; inverse-transpose normals must follow it. */
    bank.channels[0].command=0x4c;
    uint8_t *keys=s.source.bytes+s.source.nodes[bank.channels[0].curve].data;
    w16(keys+2,0x4000); w16(keys+4,0x3c00); w16(keys+6,0x3c00);
    w16(keys+10,0x4400); w16(keys+12,0x3c00); w16(keys+14,0x3c00);
    assert(!p4_golden_model_pose(&s,&bank,0,0.5f,error,sizeof error));
    near(s.meshes[0].vertex[0].position[0],expected+20.0f/128);
    near(s.meshes[0].vertex[0].normal[0],(0.5f/6*20+0.5f/4*107)/128);
    near(s.helpers[0].world[12],18);
    bank.channels[0].flags=P4_GM_CHANNEL_SENTINEL;
    assert(p4_golden_model_pose(&s,&bank,0,0,error,sizeof error)); assert(strstr(error,"exception"));
    p4_golden_model_motions_free(&bank); p4_golden_model_scene_free(&s);
}
static void curves(void)
{
    memset(b,0,sizeof b); memcpy(b,"OMG.00.1PSP",11); cursor=16;
    uint32_t root=begin(2,NULL,NULL,0,NULL,0), model=begin(3,"toy",NULL,0,NULL,0), motion=begin(11,"base",NULL,0,NULL,0);
    const uint16_t linear[]={0,0x3c00,0x4000,0x4200,0x3800,0x4200,0x4400,0x4500}; curve(0x81,3,2,linear);
    const uint16_t antipodes[]={0,0,0,0,0x3c00,0x3c00,0,0,0,0xbc00}; curve(0x84,4,2,antipodes);
    const uint16_t spherical[]={0,0,0,0,0x3c00,0x3c00,0,0,0x3c00,0}; curve(0x84,4,2,spherical);
    const uint16_t constant[]={0,0x4000,0x3c00,0x4400}; curve(0x80,1,2,constant);
    const uint16_t tiny[]={0,1}; curve(0x80,1,1,tiny);
    const uint16_t prestart[]={0xffff,0x3c00,0,0x4000,0x3c00,0x4400}; curve(0x81,1,3,prestart);
    const uint16_t duplicate[]={0,0x3c00,0,0x4000,0x3c00,0x4400}; curve(0x81,1,3,duplicate);
    end(motion); end(model); end(root);
    P4GoldenModel m={0}; P4GoldenMotions bank={0}; char error[256]="";
    assert(!p4_golden_model_parse(b,cursor,&m,error,sizeof error)); assert(!p4_golden_model_motions(&m,&bank,error,sizeof error));
    uint32_t node=bank.clips[0].node, c=p4_golden_model_child(&m,node,12,0); float v[4]={0}, time=0;
    assert(!p4_golden_model_curve_key(&m,c,1,&time,v)); near(time,0.5f); near(v[0],3);
    assert(!p4_golden_model_curve(&m,c,0.25f,v)); near(v[0],2); near(v[1],3); near(v[2],4);
    assert(!p4_golden_model_curve(&m,c,-100,v)); near(v[0],1);
    assert(!p4_golden_model_curve(&m,c,100,v)); near(v[0],3);
    c=p4_golden_model_child(&m,node,12,1); assert(!p4_golden_model_curve(&m,c,0.5f,v)); near(fabsf(v[3]),1);
    c=p4_golden_model_child(&m,node,12,2); assert(!p4_golden_model_curve(&m,c,0.5f,v)); near(v[2],sqrtf(0.5f)); near(v[3],sqrtf(0.5f));
    c=p4_golden_model_child(&m,node,12,3); assert(!p4_golden_model_curve(&m,c,0.5f,v)); near(v[0],2); assert(!p4_golden_model_curve(&m,c,1,v)); near(v[0],4);
    c=p4_golden_model_child(&m,node,12,4); assert(!p4_golden_model_curve(&m,c,0,v)); assert(v[0]==ldexpf(1,-24));
    c=p4_golden_model_child(&m,node,12,5);
    assert(!p4_golden_model_curve_key(&m,c,0,&time,v)); assert(isnan(time)); near(v[0],1);
    assert(!p4_golden_model_curve(&m,c,0.5f,v)); near(v[0],3); /* explicit finite-key default */
    c=p4_golden_model_child(&m,node,12,6); assert(!p4_golden_model_curve(&m,c,0,v)); near(v[0],2);
    p4_golden_model_motions_free(&bank);
    w16(m.bytes+m.nodes[c].data+2,0x7c00); assert(p4_golden_model_motions(&m,&bank,error,sizeof error)); assert(strstr(error,"nonfinite"));
    p4_golden_model_free(&m);
}
int main(void) { curves(); animation_skin(); puts("Golden motion/keyframe/six-lane skinning vectors passed"); return 0; }
