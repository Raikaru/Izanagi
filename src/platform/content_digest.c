/* Allocation-free SHA-256. Shared native core; no renderer or package runtime. */
#include "p4pc/content_digest.h"
#include <string.h>

static uint32_t ror(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }
static void sha_block(P4ContentDigest *s, const uint8_t *p)
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
    uint32_t w[64];
    uint32_t a=s->h[0], b=s->h[1], c=s->h[2], d=s->h[3];
    uint32_t e=s->h[4], f=s->h[5], g=s->h[6], h=s->h[7];
    for (unsigned i=0; i<16; ++i)
        w[i] = (uint32_t)p[4*i]<<24 | (uint32_t)p[4*i+1]<<16 | (uint32_t)p[4*i+2]<<8 | p[4*i+3];
    for (unsigned i=16; i<64; ++i) {
        uint32_t x=w[i-15], y=w[i-2];
        w[i] = w[i-16] + (ror(x,7)^ror(x,18)^(x>>3)) + w[i-7] + (ror(y,17)^ror(y,19)^(y>>10));
    }
    for (unsigned i=0; i<64; ++i) {
        uint32_t t1=h+(ror(e,6)^ror(e,11)^ror(e,25))+((e&f)^(~e&g))+k[i]+w[i];
        uint32_t t2=(ror(a,2)^ror(a,13)^ror(a,22))+((a&b)^(a&c)^(b&c));
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d;
    s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}
void p4_content_digest_begin(P4ContentDigest *s)
{
    static const uint32_t initial[8]={
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    memcpy(s->h,initial,sizeof initial);
    s->bytes=0;
    s->used=0;
}
void p4_content_digest_update(P4ContentDigest *s, const void *data, size_t size)
{
    const uint8_t *p=data;
    s->bytes += size;
    while (size) {
        if (s->used == 0 && size >= 64) {
            sha_block(s,p); p+=64; size-=64;
        } else {
            size_t n=64-s->used;
            if (n>size) n=size;
            memcpy(s->block+s->used,p,n);
            s->used+=n; p+=n; size-=n;
            if (s->used==64) { sha_block(s,s->block); s->used=0; }
        }
    }
}
void p4_content_digest_end(P4ContentDigest *s, char hex[P4_CONTENT_DIGEST_HEX+1])
{
    static const char digits[]="0123456789abcdef";
    uint64_t bits=s->bytes*8u;
    uint8_t tail[72]={0x80};
    size_t padding=s->used<56?56-s->used:120-s->used;
    for (unsigned i=0;i<8;++i) tail[padding+i]=(uint8_t)(bits>>(56-8*i));
    p4_content_digest_update(s,tail,padding+8);
    for (unsigned i=0;i<32;++i) {
        uint8_t byte=(uint8_t)(s->h[i/4]>>(24-8*(i%4)));
        hex[2*i]=digits[byte>>4]; hex[2*i+1]=digits[byte&15];
    }
    hex[64]=0;
}
void p4_content_digest(const void *data, size_t size, char hex[P4_CONTENT_DIGEST_HEX+1])
{
    P4ContentDigest s;
    p4_content_digest_begin(&s);
    p4_content_digest_update(&s,data,size);
    p4_content_digest_end(&s,hex);
}

int p4_content_fingerprint_seal(P4ContentFingerprint *out, const void *bytes, size_t size)
{
    P4ContentFingerprint result;
    if (!out || !bytes || !size || size>UINTPTR_MAX-(uintptr_t)bytes) return -1;
    result.size=size;
    p4_content_digest(bytes,size,result.hex);
    *out=result;
    return 0;
}

int p4_content_fingerprint_matches(const P4ContentFingerprint *expected,
                                   const void *bytes, size_t size)
{
    char actual[P4_CONTENT_DIGEST_HEX+1];
    if (!expected || !bytes || !size || size!=expected->size ||
        size>UINTPTR_MAX-(uintptr_t)bytes) return 0;
    p4_content_digest(bytes,size,actual);
    return memcmp(actual,expected->hex,sizeof actual)==0;
}
