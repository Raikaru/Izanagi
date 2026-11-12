/* PS2 memory-card FAT and page ECC, following the public-domain mymc format
 * documentation/algorithms by Ross Ridge. Only 8 MiB PCSX2 cards are accepted. */
#include "p4pc/save_import.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t rd(const unsigned char *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int err(char *e,size_t cap,const char *s) { if(cap) snprintf(e,cap,"%s",s); return -1; }
static unsigned parity(unsigned x) { x^=x>>4; x^=x>>2; x^=x>>1; return x&1; }
static int ecc(const unsigned char *p,const unsigned char *stored) {
    unsigned cp=0x77,l0=127,l1=127; const unsigned masks[]={0x55,0x33,15,0,0xaa,0xcc,0xf0};
    for(unsigned i=0;i<128;i++) { for(unsigned j=0;j<7;j++) cp^=parity(p[i]&masks[j])<<j; if(parity(p[i])) { l0^=~i; l1^=i; } }
    return stored[0]==cp && stored[1]==(l0&127) && stored[2]==l1;
}
typedef struct { unsigned char *raw; uint32_t alloc,end,root,ifc[32]; } Card;
static const unsigned char *cluster(Card *c,uint32_t n) { return n<8192 ? c->raw+(size_t)n*1024 : NULL; }
static int fat(Card *c,uint32_t n,uint32_t *v) {
    if(n>=c->end) return -1;
    const unsigned char *ind=cluster(c,c->ifc[(n/256)/256]); if(!ind) return -1;
    const unsigned char *table=cluster(c,rd(ind+((n/256)%256)*4)); if(!table) return -1;
    *v=rd(table+(n%256)*4); return 0;
}
static unsigned char *chain(Card *c,uint32_t start,size_t *size) {
    unsigned char seen[8192]={0}; uint32_t n=start,next; size_t count=0;
    while(1) { if(n>=c->end || seen[n] || !cluster(c,c->alloc+n) || fat(c,n,&next) || !(next&0x80000000)) return NULL; seen[n]=1; count++; if(next==0xffffffff) break; n=next&0x7fffffff; }
    unsigned char *out=malloc(count*1024); if(!out) return NULL;
    n=start; for(size_t i=0;i<count;i++) { memcpy(out+i*1024,cluster(c,c->alloc+n),1024); if(fat(c,n,&next)) { free(out); return NULL; } n=next&0x7fffffff; }
    *size=count*1024; return out;
}
/* Emit directories through the same checked archive writer as other formats. */
int p4save_card(const unsigned char *b,size_t n,int (*visit)(P4SaveArchive *,void *),void *ctx,char *e,size_t cap) {
    Card c={0}; int rc=-1;
    if(n!=8388608 && n!=8650752) return err(e,cap,"Expected an 8 MiB PCSX2 memory card");
    if(memcmp(b,"Sony PS2 Memory Card Format ",28) || b[40]!=0 || b[41]!=2 || b[42]!=2 || b[43]!=0 || rd(b+48)!=8192) return err(e,cap,"Invalid PS2 memory-card geometry");
    c.raw=malloc(8388608); if(!c.raw) return err(e,cap,"Out of memory");
    size_t stride=n==8650752?528:512;
    for(size_t i=0;i<16384;i++) {
        const unsigned char *p=b+i*stride;
        if(stride==528) {
            int erased=1; for(size_t j=0;j<528;j++) if(p[j]!=255) { erased=0; break; }
            if(!erased) for(size_t j=0;j<4;j++) if(!ecc(p+j*128,p+512+j*3)) { err(e,cap,"Memory-card page ECC mismatch"); goto done; }
        }
        memcpy(c.raw+i*512,p,512);
    }
    c.alloc=rd(c.raw+52); c.end=rd(c.raw+56); c.root=rd(c.raw+60);
    if(c.alloc>=8192 || c.end>8192-c.alloc || !c.end) { err(e,cap,"Invalid memory-card allocation range"); goto done; }
    for(size_t i=0;i<32;i++) c.ifc[i]=rd(c.raw+80+i*4);
    size_t rootsize; unsigned char *root=chain(&c,c.root,&rootsize);
    if(!root) { err(e,cap,"Invalid root FAT chain"); goto done; }
    uint32_t roots=rd(root+4); if(roots>rootsize/512) { free(root); err(e,cap,"Invalid root directory length"); goto done; }
    rc=0;
    for(uint32_t i=0;i<roots;i++) {
        const unsigned char *d=root+(size_t)i*512;
        if((rd(d)&0x8020)!=0x8020 || memcmp(d+64,"BASLUS-21782Save",sizeof "BASLUS-21782Save"-1)) continue;
        P4SaveArchive s={0}; size_t ds; unsigned char *dir=chain(&c,rd(d+16),&ds);
        if(!dir || rd(d+4)>ds/512 || rd(d+4)>P4SAVE_MAX_FILES+2 || !memchr(d+64,0,32)) { free(dir); rc=err(e,cap,"Invalid save directory FAT chain"); break; }
        memcpy(s.name,d+64,32); memcpy(s.created,d+8,8); memcpy(s.modified,d+24,8);
        size_t total=0;
        for(uint32_t j=0;j<rd(d+4);j++) {
            const unsigned char *p=dir+(size_t)j*512;
            if(!(rd(p)&0x8000) || p[64]=='.') continue;
            if((rd(p)&0x30)!=0x10 || s.count==P4SAVE_MAX_FILES) { rc=err(e,cap,"Invalid save file directory entry"); break; }
            P4SaveFile *f=&s.files[s.count++]; memcpy(f->name,p+64,32); memcpy(f->created,p+8,8); memcpy(f->modified,p+24,8); f->size=rd(p+4);
            if(f->size>P4SAVE_MAX_BYTES-total) { rc=err(e,cap,"Memory-card save exceeds 8 MiB"); break; }
            total+=f->size;
            size_t bytes=0; f->data=f->size?chain(&c,rd(p+16),&bytes):malloc(1);
            if(!f->data || f->size>bytes || f->size>P4SAVE_MAX_BYTES) { rc=err(e,cap,"Invalid save file FAT chain or size"); break; }
        }
        free(dir);
        if(!rc) rc=visit(&s,ctx);
        p4save_archive_free(&s); if(rc) break;
    }
    free(root);
done: free(c.raw); return rc;
}
