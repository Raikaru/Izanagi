#include "p4pc/golden_message.h"
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (unsigned)p[1]<<8); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static int span(size_t size, size_t off, size_t length) { return off<=size && length<=size-off; }
const char *p4_golden_text_next(P4GoldenText *t, P4GoldenToken *out)
{
    if(!t || !out || !t->bytes || t->position>t->size) return "text.span";
    P4GoldenToken v={0};
    if(t->position==t->size) { *out=v; return NULL; }
    size_t p=t->position; unsigned b=t->bytes[p++];
    if(!b) v.kind=P4_GOLDEN_END;
    else if(b==10) v.kind=P4_GOLDEN_NEWLINE;
    else if(b>=0xf0) {
        size_t n=(b&15u)*2u;
        if(!n) return "text.control-length";
        if(!span(t->size,p-1,n)) return "text.control-truncated";
        v.kind=P4_GOLDEN_CONTROL; v.code=(uint16_t)((b<<8)|t->bytes[p]);
        v.group=(uint8_t)(t->bytes[p]>>5); v.index=t->bytes[p]&31u;
        v.argument_count=(uint8_t)((n-2)/2); v.arguments=t->bytes+p+1; p+=n-1;
    } else {
        v.kind=P4_GOLDEN_GLYPH;
        if(b<0x20 || b==0x7f) return "text.encoding-control";
        if(b<0x80) {
            v.code=(uint16_t)b; v.scalar=b;
            v.glyph=(uint16_t)(b-0x20);
        } else {
            if(p>=t->size) return "text.glyph-truncated";
            unsigned trail=t->bytes[p++];
            if(trail<0x80) return "text.encoding-trail";
            v.code=(uint16_t)((b<<8)|trail);
            /* Extended pairs encode 128-cell pages, not Shift-JIS. */
            v.glyph=(uint16_t)((b-0x80)*128u+trail-0x80);
        }
    }
    t->position=p; *out=v; return NULL;
}
const char *p4_golden_message_dialog(const P4GoldenMessage *m, uint32_t index, P4GoldenDialog *out)
{
    if(!m || !m->bytes || !out || index>=m->dialogs) return "message.dialog-index";
    size_t entry=32+(size_t)index*8;
    if(!span(m->size,entry,8)) return "message.directory-span";
    uint32_t kind=u32(m->bytes+entry), relative=u32(m->bytes+entry+4);
    size_t off=32+(size_t)relative;
    if(relative>m->size-32) return "message.dialog-span";
    if(kind>1) return "message.dialog-kind";
    if(!span(m->size,off,kind?32:28)) return "message.dialog-span";
    P4GoldenDialog d={0}; d.message=m; d.kind=kind; d.label=m->bytes+off;
    d.count=u16(d.label+(kind?26:24)); d.speaker=kind?UINT16_MAX:u16(d.label+26);
    if(kind && (u16(d.label+24) || u16(d.label+30))) return "message.selection-header";
    d.pattern=kind?u16(d.label+28):0;
    size_t table=off+(kind?32:28), length=table+(size_t)d.count*4;
    if(!d.count) { d.table=(uint32_t)table; d.text=d.table; *out=d; return NULL; }
    if(!span(m->size,table,(size_t)d.count*4+4)) return "message.page-table-span";
    d.table=(uint32_t)table; d.text=(uint32_t)(length+4); d.text_size=u32(m->bytes+length);
    if(!span(m->size,d.text,d.text_size)) return "message.text-span";
    if(d.count && (!d.text_size || m->bytes[(size_t)d.text+d.text_size-1])) return "message.text-terminator";
    *out=d; return NULL;
}
const char *p4_golden_message_page(const P4GoldenDialog *d, uint16_t index, P4GoldenText *out)
{
    if(!d || !d->message || !out || index>=d->count) return "message.page-index";
    const P4GoldenMessage *m=d->message;
    uint32_t relative=u32(m->bytes+d->table+(size_t)index*4);
    if(relative>m->size-32) return "message.page-span";
    size_t p=32+(size_t)relative;
    if(p<d->text || p>=(size_t)d->text+d->text_size) return "message.page-span";
    size_t end=(size_t)d->text+d->text_size;
    for(unsigned i=0;i<d->count;++i) {
        uint32_t next=u32(m->bytes+d->table+(size_t)i*4);
        if(next>m->size-32) return "message.page-span";
        size_t bound=32+(size_t)next;
        if(bound<d->text || bound>=(size_t)d->text+d->text_size) return "message.page-span";
        if(bound>p && bound<end) end=bound;
    }
    P4GoldenText t={m->bytes+p,end-p,0}; *out=t; return NULL;
}
const char *p4_golden_message_speaker(const P4GoldenMessage *m, uint32_t index, P4GoldenText *out)
{
    if(!m || !m->bytes || !out || index>=m->speakers) return "message.speaker-index";
    uint32_t relative=u32(m->bytes+m->speaker_table+(size_t)index*4);
    if(relative>=m->size-32) return "message.speaker-span";
    size_t p=32+(size_t)relative;
    if(p>=m->size) return "message.speaker-span";
    *out=(P4GoldenText){m->bytes+p,m->size-p,0}; return NULL;
}
const char *p4_golden_message_open(const uint8_t *b, size_t size, P4GoldenMessage *out)
{
    if(!b || !out || size<32) return "message.header-span";
    if(b[0]!=7 || memcmp(b+8,"MSG1",4)) return "message.magic";
    if(b[28]) return "message.relocated";
    if(b[1] || b[29] || u32(b+12) || u16(b+30)!=2) return "message.header-variant";
    uint32_t length=u32(b+4),count=u32(b+24);
    if(length<32 || length>size) return "message.declared-span";
    if(count>(length-32u)/8u) return "message.directory-span";
    if(!span(length,u32(b+16),u32(b+20))) return "message.relocation-span";
    P4GoldenMessage m={0}; m.bytes=b; m.size=length; m.dialogs=count;
    size_t speaker_header=32+(size_t)count*8;
    if(count || length>32) {
        if(!span(length,speaker_header,16)) return "message.speaker-header-span";
        m.speakers=u32(b+speaker_header+4);
        if(m.speakers) {
            uint32_t relative=u32(b+speaker_header);
            if(relative>length-32) return "message.speaker-table-span";
            size_t table=32+(size_t)relative;
            if(table>length || m.speakers>(length-table)/4) return "message.speaker-table-span";
            m.speaker_table=(uint32_t)table;
        }
        if(u32(b+speaker_header+8) || u32(b+speaker_header+12)) return "message.speaker-extension";
    }
    for(uint32_t i=0;i<m.speakers;++i) {
        P4GoldenText t; const char *error=p4_golden_message_speaker(&m,i,&t); if(error) return error;
        P4GoldenToken token;
        do { error=p4_golden_text_next(&t,&token); if(error) return error; } while(token.kind!=P4_GOLDEN_END);
    }
    for(uint32_t i=0;i<count;++i) {
        P4GoldenDialog d; const char *error=p4_golden_message_dialog(&m,i,&d); if(error) return error;
        for(uint16_t j=0;j<d.count;++j) {
            P4GoldenText t; error=p4_golden_message_page(&d,j,&t); if(error) return error;
            P4GoldenToken token;
            do { error=p4_golden_text_next(&t,&token); if(error) return error; } while(token.kind!=P4_GOLDEN_END);
        }
    }
    *out=m; return NULL;
}
