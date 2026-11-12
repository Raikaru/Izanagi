/* SHA-256, FIPS180-4 section6.2. Integrity binding, not a signature/MAC.
 * Native save-only code: no heap allocation, guest state or platform crypto. */
#include "mod_save_hash.h"
#include <string.h>

static uint32_t rotate(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }
static uint32_t be32(const unsigned char *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static void compress(P4SaveHash *hash, const unsigned char *block)
{
    static const uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t words[16];
    uint32_t a=hash->state[0],b=hash->state[1],c=hash->state[2],d=hash->state[3];
    uint32_t e=hash->state[4],f=hash->state[5],g=hash->state[6],h=hash->state[7];
    for (unsigned i=0;i<64;i++) {
        unsigned j=i&15u;
        if (i<16) words[j]=be32(block+4*i);
        else {
            uint32_t x=words[(i-15)&15u],y=words[(i-2)&15u];
            words[j]+=(rotate(x,7)^rotate(x,18)^(x>>3))+words[(i-7)&15u]+(rotate(y,17)^rotate(y,19)^(y>>10));
        }
        uint32_t t1=h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+words[j];
        uint32_t t2=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    hash->state[0]+=a;hash->state[1]+=b;hash->state[2]+=c;hash->state[3]+=d;
    hash->state[4]+=e;hash->state[5]+=f;hash->state[6]+=g;hash->state[7]+=h;
}
void p4_save_hash_init(P4SaveHash *hash)
{
    static const uint32_t initial[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    memcpy(hash->state,initial,sizeof initial);
    hash->bytes=0;hash->used=0;
}
void p4_save_hash_update(P4SaveHash *hash, const void *data, size_t size)
{
    if(!size)return;
    const unsigned char *p=data;
    hash->bytes+=(uint64_t)size;
    if (hash->used) {
        size_t n=64-hash->used;
        if(n>size)n=size;
        if(n)memcpy(hash->block+hash->used,p,n);
        hash->used+=n;p+=n;size-=n;
        if(hash->used==64){compress(hash,hash->block);hash->used=0;}
    }
    while(size>=64){compress(hash,p);p+=64;size-=64;}
    if(size){memcpy(hash->block,p,size);hash->used=size;}
}
void p4_save_hash_final(P4SaveHash *hash, unsigned char digest[32])
{
    uint64_t bits=hash->bytes<<3;
    hash->block[hash->used++]=0x80;
    if(hash->used>56){memset(hash->block+hash->used,0,64-hash->used);compress(hash,hash->block);hash->used=0;}
    memset(hash->block+hash->used,0,56-hash->used);
    for(unsigned i=0;i<8;i++)hash->block[63-i]=(unsigned char)(bits>>(8*i));
    compress(hash,hash->block);
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)digest[4*i+j]=(unsigned char)(hash->state[i]>>(24-8*j));
    hash->used=0;
}
void p4_save_hash(const void *data, size_t size, unsigned char digest[32])
{
    P4SaveHash hash;
    p4_save_hash_init(&hash);p4_save_hash_update(&hash,data,size);p4_save_hash_final(&hash,digest);
}
