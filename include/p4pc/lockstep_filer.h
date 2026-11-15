/* Lockstep filer replay keys (docs/lockstep.md, "Lockstep disc replay"). The game's background
 * loader thread (sdkWrapWorkerThread, 00466e80) turns queued FilerRequests into RenderWare
 * objects; consumers such as FldResource_PollLoads see one finished only when
 * filerTakeCompletedRequest (004669d0) finds its state 3. Retail's p4hook and the port both key
 * a request from its own fields, read through a P4WalkHost, so the key text is identical. */
#ifndef P4PC_LOCKSTEP_FILER_H
#define P4PC_LOCKSTEP_FILER_H

#include "p4pc/lockstep_alignment.h"
#include <stdio.h>

#define P4LF_ENQUEUE 0x004667d0u /* filerEnqueueRequest: returns the queued request */
#define P4LF_TAKE 0x004669d0u    /* filerTakeCompletedRequest(request, completed, result) */
#define P4LF_KEY 1024u
#define P4LF_HEADER "# p4pc-filer-replay 1 retail_anchor %u horizon %u\n"

/* A request completes in this take when its state is 3 and it is still linked (sdk_filer). */
static inline int p4lf_complete(const P4WalkHost *h, uint32_t request) {
    return p4la_word(h,request+0x1a4u)==3u && p4la_word(h,request+4u)!=0u;
}
static inline uint8_t p4lf_byte(const P4WalkHost *h, uint32_t address) {
    return (uint8_t)(p4la_word(h,address&~3u)>>((address&3u)*8u));
}
/* Up to 127 bytes before the NUL, as hex; "-" for an empty string. */
static inline char *p4lf_hex(const P4WalkHost *h, uint32_t address, char *out) {
    static const char digits[]="0123456789abcdef";
    char *p=out;
    for (uint32_t i=0;i<127u;i++) {
        const uint8_t c=p4lf_byte(h,address+i);
        if (!c) break;
        *p++=digits[c>>4]; *p++=digits[c&15u];
    }
    if (p==out) *p++='-';
    *p='\0';
    return p;
}
/* kind.flags.resultKind.memoryKind.byteCount.name.path.cacheName.chunk: the request's own
 * fields. For a source that is an open RenderWare memory stream (type 3), the chunk part names
 * the resource from inside it: the leading chunk header at the stream's memory block (type,
 * size, library version), its payload when it is a small struct (a clump's atomic/light/camera
 * counts, a texture dictionary's count), and the header of the chunk after it. Every byte read
 * lies inside the resource; host and guest addresses never enter the key. */
#define P4LF_PAYLOAD 64u
static inline int p4lf_key(const P4WalkHost *h, uint32_t request, char out[P4LF_KEY]) {
    if (!p4la_address(request,0x1d8u)) return 0;
    char *p=out+snprintf(out,P4LF_KEY,"%u.%x.%u.%u.%u.",p4la_word(h,request+0x1acu),
        p4la_word(h,request+0x1a8u),p4la_word(h,request+0x20u),p4la_word(h,request+0x1ccu),
        p4la_word(h,request+0x1c0u));
    p=p4lf_hex(h,request+0x24u,p); *p++='.';
    p=p4lf_hex(h,request+0xa4u,p); *p++='.';
    p=p4lf_hex(h,request+0x124u,p); *p++='.';
    const uint32_t stream=p4la_word(h,request+8u);
    const uint32_t block=p4la_address(stream,24u) && p4la_word(h,stream)==3u ? p4la_word(h,stream+20u) : 0u;
    if (!p4la_address(block,12u)) {
        snprintf(p,(size_t)(out+P4LF_KEY-p),"-");
        return 1;
    }
    const uint32_t size=p4la_word(h,block+4u);
    uint32_t words=3u;
    if (size<=P4LF_PAYLOAD && !(size&3u) && p4la_address(block,24u+size)) words+=size/4u+3u;
    for (uint32_t i=0;i<words;i++) p+=snprintf(p,(size_t)(out+P4LF_KEY-p),"%08x",p4la_word(h,block+i*4u));
    return 1;
}

#endif
