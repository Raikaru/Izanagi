/* Shared on-disk integrity rules for runtime loads and offline save imports.
 * All fields are LE; hash binding is not a signature or a trust attestation. */
#include "mod_save_format.h"
#include "mod_save_hash.h"
#include <stdio.h>
#include <string.h>
static uint32_t get32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const unsigned char *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static void put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
static int fail(char *error,size_t cap,const char *message){if(cap)snprintf(error,cap,"%s",message);return -1;}
int p4_save_sidecar_verify(uint64_t retail_size,const unsigned char retail_hash[32],
    const void *sidecar,size_t size,char *error,size_t cap)
{
    const unsigned char *data=sidecar;
    static const unsigned char magic[8]={'P','4','M','O','D','S',0,0};
    if(!data||!retail_hash||size<P4_MOD_SAVE_HEADER_BYTES+32||size>P4_MOD_SAVE_MAX_BYTES||
       memcmp(data,magic,sizeof magic)||get32(data+8)!=1||get32(data+12)!=P4_MOD_SAVE_HEADER_BYTES||
       (get32(data+68)&~P4_MOD_SAVE_WITNESS)||get64(data+80)||get64(data+88)||
       get64(data+72)!=size-P4_MOD_SAVE_HEADER_BYTES-32||get32(data+56)>P4_MOD_SAVE_MAX_BLOCKS||
       get32(data+60)>P4_MOD_SAVE_MAX_REQUIREMENTS||get32(data+64)>P4_MOD_SAVE_MAX_HISTORY)
        return fail(error,cap,"Unsupported or malformed .p4mods envelope");
    unsigned char digest[32];
    p4_save_hash(data,size-32,digest);
    if(memcmp(digest,data+size-32,32))return fail(error,cap,"Save sidecar integrity mismatch");
    if(get64(data+16)!=retail_size||memcmp(data+24,retail_hash,32))
        return fail(error,cap,"Save sidecar belongs to different retail data");
    return 0;
}
void p4_save_marker_make(uint64_t retail_size,const unsigned char retail_hash[32],
    uint64_t sidecar_size,const unsigned char sidecar_hash[32],unsigned char marker[P4_MOD_SAVE_MARKER_BYTES])
{
    static const unsigned char magic[8]={'P','4','M','C','M','T',0,0};
    memcpy(marker,magic,sizeof magic);put32(marker+8,1);put32(marker+12,P4_MOD_SAVE_MARKER_BYTES);
    put64(marker+16,retail_size);memcpy(marker+24,retail_hash,32);
    put64(marker+56,sidecar_size);memcpy(marker+64,sidecar_hash,32);
    p4_save_hash(marker,96,marker+96);
}
int p4_save_marker_verify(uint64_t retail_size,const unsigned char retail_hash[32],
    uint64_t sidecar_size,const unsigned char sidecar_hash[32],const void *bytes,size_t size,
    char *error,size_t cap)
{
    const unsigned char *marker=bytes;
    static const unsigned char magic[8]={'P','4','M','C','M','T',0,0};
    if(!marker||!retail_hash||!sidecar_hash||size!=P4_MOD_SAVE_MARKER_BYTES||memcmp(marker,magic,sizeof magic)||
       get32(marker+8)!=1||get32(marker+12)!=P4_MOD_SAVE_MARKER_BYTES)
        return fail(error,cap,"Malformed or missing mod-save commit marker");
    unsigned char digest[32];p4_save_hash(marker,96,digest);
    if(memcmp(digest,marker+96,32)||get64(marker+16)!=retail_size||memcmp(marker+24,retail_hash,32)||
       get64(marker+56)!=sidecar_size||memcmp(marker+64,sidecar_hash,32))
        return fail(error,cap,"Mod-save commit marker belongs to an incomplete/different generation");
    return 0;
}
