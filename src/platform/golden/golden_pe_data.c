#include "golden_pe_data.h"
#include "p4pc/content_digest.h"
#include "p4pc/hostpath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t half(const uint8_t *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static uint32_t word(const uint8_t *p) { return (uint32_t)half(p)|(uint32_t)half(p+2)<<16; }
static int fail(char *reason,size_t capacity,const char *text)
{
    if(reason && capacity) snprintf(reason,capacity,"%s",text);
    return -1;
}
int p4_golden_pe_data(const char *path,const char *expected_exe,
    const char *const *names,size_t name_count,int virtual_extent,
    int (*visit)(const uint8_t *,size_t,void *,char *,size_t),void *context,
    char exe_digest[65],char *reason,size_t capacity)
{
    uint8_t header[64],pe[24],record[40],block[65536];
    struct Section { uint32_t offset,size; } selected[32];
    uint32_t seen=0; size_t selected_count=0;
    if(!path || !expected_exe || !names || !name_count || name_count>32 || !visit || !exe_digest)
        return fail(reason,capacity,"missing executable path or data reader");
    FILE *file=p4_host_fopen(path,"rb");
    if(!file) return fail(reason,capacity,"cannot open Steam P4G.exe");
    int result=-1; P4ContentDigest hash; char digest[65]; uint64_t file_size=0; size_t n;
    p4_content_digest_begin(&hash);
    while((n=fread(block,1,sizeof block,file))!=0) { p4_content_digest_update(&hash,block,n); file_size+=n; }
    if(ferror(file)) { fail(reason,capacity,"cannot hash Steam P4G.exe"); goto done; }
    p4_content_digest_end(&hash,digest);
    if(strcmp(digest,expected_exe)) { fail(reason,capacity,"unsupported Steam P4G.exe digest"); goto done; }
    rewind(file);
    if(fread(header,1,sizeof header,file)!=sizeof header || header[0]!='M' || header[1]!='Z') {
        fail(reason,capacity,"invalid PE DOS header"); goto done;
    }
    uint32_t pe_offset=word(header+60);
    if(pe_offset>0x100000 || fseek(file,(long)pe_offset,SEEK_SET) ||
        fread(pe,1,sizeof pe,file)!=sizeof pe || memcmp(pe,"PE\0\0",4) || half(pe+4)!=0x8664) {
        fail(reason,capacity,"invalid Steam PE header"); goto done;
    }
    unsigned sections=half(pe+6);
    if(!sections || sections>96 || fseek(file,half(pe+20),SEEK_CUR)) {
        fail(reason,capacity,"invalid Steam PE section table"); goto done;
    }
    for(unsigned i=0;i<sections;++i) {
        if(fread(record,1,sizeof record,file)!=sizeof record) {
            fail(reason,capacity,"truncated Steam PE section table"); goto done;
        }
        for(size_t j=0;j<name_count;++j) {
            size_t length=strlen(names[j]);
            if(length>8 || memcmp(record,names[j],length) || (length<8 && record[length])) continue;
            if((seen&(1u<<j)) || (word(record+36)&0x20000040u)!=0x40u) {
                fail(reason,capacity,"Steam PE section is not unique non-executable initialized data"); goto done;
            }
            seen|=1u<<j;
            uint32_t size=word(record+16),offset=word(record+20),virt=word(record+8);
            if(virtual_extent && virt<size) size=virt;
            if(!size || size>16u*1024u*1024u || offset>0x7fffffffu ||
                offset>file_size || size>file_size-offset) {
                fail(reason,capacity,"invalid Steam PE data section"); goto done;
            }
            selected[selected_count++]=(struct Section){offset,size};
        }
    }
    if(!selected_count) { fail(reason,capacity,"invalid Steam PE data section"); goto done; }
    for(size_t i=0;i<selected_count;++i) {
        uint8_t *data=malloc(selected[i].size);
        if(!data || fseek(file,(long)selected[i].offset,SEEK_SET) ||
            fread(data,1,selected[i].size,file)!=selected[i].size) {
            free(data); fail(reason,capacity,"cannot read Steam PE data section"); goto done;
        }
        int status=visit(data,selected[i].size,context,reason,capacity);
        free(data); if(status) goto done;
    }
    memcpy(exe_digest,digest,65); result=0;
done:
    fclose(file); return result;
}
