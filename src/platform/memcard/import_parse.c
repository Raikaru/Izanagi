/* Container layouts: Ross Ridge's public-domain mymc ps2save.py; PSV layout
 * also documented by tools/ps2save.py. No console signing keys are used: PSV's
 * PS3 signature is not an authenticity requirement for loading a PS2 save.
 * Integrity is checked against P4's own summary and record checksums below. */
#include "p4pc/save_import.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int p4save_decode_max(const unsigned char *, size_t, P4SaveArchive *, char *, size_t);
int p4save_decode_cbs(const unsigned char *, size_t, P4SaveArchive *, char *, size_t);
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static unsigned u16(const unsigned char *p) { return p[0] | (unsigned)p[1]<<8; }
static int fail(char *e,size_t n,const char *s) { if(n) snprintf(e,n,"%s",s); return -1; }
static int range(size_t o,size_t n,size_t total) { return o<=total && n<=total-o; }
static int name(char out[33], const unsigned char *p,size_t n) {
    size_t i=0; while(i<n && p[i]) ++i;
    if(!i || i>32) return -1;
    if((i==1 && p[0]=='.') || (i==2 && p[0]=='.' && p[1]=='.')) return -1;
    for(size_t j=0;j<i;j++) if(p[j]<32 || p[j]>126 || strchr("<>:\"/\\|?*",p[j])) return -1;
    if(p[i-1]=='.' || p[i-1]==' ') return -1;
    memcpy(out,p,i); out[i]=0; return 0;
}
void p4save_archive_free(P4SaveArchive *s) {
    for(size_t i=0;i<s->count && i<P4SAVE_MAX_FILES;i++) free(s->files[i].data);
    free(s->mod_sidecar.data);
    free(s->mod_commit.data);
    memset(s,0,sizeof *s);
}
static int add(P4SaveArchive *s,const unsigned char *nm,size_t nn,const unsigned char *data,size_t len,const unsigned char *ct,const unsigned char *mt,char *e,size_t cap) {
    char fn[33]; size_t total=len;
    if(len>P4SAVE_MAX_BYTES) return fail(e,cap,"Save file exceeds 8 MiB");
    if(s->count==P4SAVE_MAX_FILES || name(fn,nm,nn)) return fail(e,cap,"Invalid filename or too many files");
    for(size_t i=0;i<s->count;i++) { if(!strcmp(s->files[i].name,fn)) return fail(e,cap,"Duplicate filename"); total+=s->files[i].size; }
    if(total>P4SAVE_MAX_BYTES) return fail(e,cap,"Save exceeds 8 MiB");
    P4SaveFile *f=&s->files[s->count];
    f->data=malloc(len?len:1); if(!f->data) return fail(e,cap,"Out of memory");
    memcpy(f->data,data,len); f->size=len; strcpy(f->name,fn);
    if(ct) memcpy(f->created,ct,8); if(mt) memcpy(f->modified,mt,8);
    s->count++; return 0;
}
static int psv(const unsigned char *b,size_t n,P4SaveArchive *s,char *e,size_t cap) {
    if(n<160 || u32(b+4)!=0 || u32(b+56)!=44 || u32(b+60)!=2) return fail(e,cap,"Invalid PS2 PSV header");
    size_t count=u32(b+100), table=160+count*60;
    if(count==0 || count>P4SAVE_MAX_FILES || table>n || (u32(b+124)&0x30)!=0x20 || name(s->name,b+128,32)) return fail(e,cap,"Invalid PSV directory table");
    memcpy(s->created,b+104,8); memcpy(s->modified,b+112,8);
    for(size_t i=0;i<count;i++) {
        const unsigned char *p=b+160+i*60; size_t len=u32(p+16), off=u32(p+56);
        if((u32(p+20)&0x30)!=0x10 || off<table || !range(off,len,n)) return fail(e,cap,"Invalid PSV file extent");
        for(size_t j=0;j<i;j++) { const unsigned char *q=b+160+j*60; size_t qo=u32(q+56), qn=u32(q+16); if(len && qn && off<qo+qn && qo<off+len) return fail(e,cap,"Overlapping PSV file extents"); }
        if(add(s,p+24,32,b+off,len,p,p+8,e,cap)) return -1;
    }
    /* icon offsets are redundant but must describe file data inside this file. */
    for(size_t i=68;i<100;i+=8) if(u32(b+i+4) && (u32(b+i)<table || !range(u32(b+i),u32(b+i+4),n))) return fail(e,cap,"Invalid PSV icon extent");
    return 0;
}
static int psu(const unsigned char *b,size_t n,P4SaveArchive *s,char *e,size_t cap) {
    if(n<1536 || (u32(b)&0x30)!=0x20 || (u32(b+512)&0x30)!=0x20 || (u32(b+1024)&0x30)!=0x20 || memcmp(b+576,".\0",2) || memcmp(b+1088,"..\0",3)) return fail(e,cap,"Invalid PSU directory headers");
    size_t count=u32(b+4),off=1536;
    if(count<3 || count>P4SAVE_MAX_FILES+2 || name(s->name,b+64,32)) return fail(e,cap,"Invalid PSU directory");
    memcpy(s->created,b+8,8); memcpy(s->modified,b+24,8);
    for(size_t i=2;i<count;i++) {
        if(!range(off,512,n)) return fail(e,cap,"Truncated PSU header");
        const unsigned char *p=b+off; size_t len=u32(p+4); off+=512;
        size_t padded=(len+1023)&~(size_t)1023;
        if((u32(p)&0x30)!=0x10 || !range(off,padded,n)) return fail(e,cap,"Truncated PSU file");
        if(add(s,p+64,32,b+off,len,p+8,p+24,e,cap)) return -1;
        off+=padded;
    }
    if(off!=n) return fail(e,cap,"Unexpected trailing PSU data");
    return 0;
}
static uint32_t shark_sum(const unsigned char *p,size_t n) { uint32_t h=0; for(size_t i=0;i<n;i++) h+=(uint32_t)p[i]<<(h%24); return h; }
static int xps(const unsigned char *b,size_t n,P4SaveArchive *s,char *e,size_t cap) {
    size_t off=21;
    if(n<25 || u32(b+17)!=0) return fail(e,cap,"Unsupported SharkPort save type");
    for(int i=0;i<3;i++) { if(!range(off,4,n)) return fail(e,cap,"Truncated SharkPort description"); size_t len=u32(b+off); off+=4; if(!range(off,len,n)) return fail(e,cap,"Truncated SharkPort description"); off+=len; }
    if(!range(off,4,n)) return fail(e,cap,"Truncated SharkPort length");
    size_t body=off+4, bodylen=u32(b+off); off+=4;
    if(!range(body,bodylen,n) || body+bodylen+4!=n || shark_sum(b+body,bodylen)!=u32(b+n-4)) return fail(e,cap,"SharkPort checksum or length mismatch");
    if(!range(off,98,n)) return fail(e,cap,"Truncated SharkPort directory");
    size_t count=u32(b+off+66),hdr=u16(b+off);
    if(count<3 || count>P4SAVE_MAX_FILES+2 || hdr<98 || !range(off,hdr,n-4) || (b[off+79]&0x30)!=0x20 || name(s->name,b+off+2,64)) return fail(e,cap,"Invalid SharkPort directory");
    memcpy(s->created,b+off+82,8); memcpy(s->modified,b+off+90,8); off+=hdr;
    for(size_t i=2;i<count;i++) {
        if(!range(off,98,n-4)) return fail(e,cap,"Truncated SharkPort file header");
        const unsigned char *p=b+off; size_t h=u16(p),len=u32(p+66);
        if(h<98 || !range(off,h,n-4) || (p[79]&0x30)!=0x10) return fail(e,cap,"Invalid SharkPort file header");
        off+=h; if(!range(off,len,n-4)) return fail(e,cap,"Truncated SharkPort file");
        if(add(s,p+2,64,b+off,len,p+82,p+90,e,cap)) return -1;
        off+=len;
    }
    if(off!=n-4) return fail(e,cap,"Unexpected SharkPort data");
    return 0;
}
int p4save_decode(const unsigned char *b,size_t n,P4SaveArchive *s,char *e,size_t cap) {
    memset(s,0,sizeof *s);
    if(n<4 || n>P4SAVE_MAX_BYTES+65536) return fail(e,cap,"Invalid container size");
    if(!memcmp(b,"\0VSP",4)) return psv(b,n,s,e,cap);
    if(n>=12 && !memcmp(b,"Ps2PowerSave",12)) return p4save_decode_max(b,n,s,e,cap)?0:-1;
    if(!memcmp(b,"CFU\0",4)) return p4save_decode_cbs(b,n,s,e,cap)?0:-1;
    if(n>=17 && !memcmp(b,"\x0d\0\0\0SharkPortSave",17)) return xps(b,n,s,e,cap);
    return psu(b,n,s,e,cap);
}
