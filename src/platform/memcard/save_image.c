/* Shared bounded form of the retail summary and0010e880 record checks.
 * Used by imports and mod sidecars; never rewrites or normalizes game bytes. */
#include "save_image.h"
#include <stdint.h>
#include <stdio.h>
static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int invalid(char *error, size_t cap, const char *message)
{
    if(cap)snprintf(error,cap,"%s",message);
    return -1;
}
int p4save_image_validate(const void *data, size_t size, char *error, size_t cap)
{
    if(!data||size<4100)return invalid(error,cap,"Persona 4 save data is too short");
    const unsigned char *b=data;
    unsigned sum=0;
    for(size_t i=4;i<54;i++)sum=(sum+b[i])&255u;
    if(b[54]!=sum||b[55])return invalid(error,cap,"Persona 4 summary checksum mismatch");
    size_t off=56;
    sum=0;
    while(size-off>=8){
        uint32_t tag=word(b+off),len=word(b+off+4);
        if(tag==0x2000u){
            if(len!=1||size-off<13||b[off+8]!=sum||word(b+off+9)!=UINT32_MAX)
                return invalid(error,cap,"Persona 4 save checksum mismatch");
            return 0;
        }
        if(tag==UINT32_MAX||!len||len>size-off-8)break;
        for(size_t i=0;i<(size_t)len+8;i++)sum=(sum+b[off+i])&255u;
        off+=(size_t)len+8;
    }
    return invalid(error,cap,"Invalid Persona 4 save record stream");
}
