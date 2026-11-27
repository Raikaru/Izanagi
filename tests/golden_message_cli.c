/* Owner-local driver: scan prints metadata; inflate returns bytes on a private pipe. */
#include "p4pc/golden_font.h"
#include "p4pc/golden_source.h"
#include "p4pc/game_mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
P4GameMode p4_game_mode(void) { return P4_GAME_MODE_GOLDEN; }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void scan(const uint8_t *b,size_t size)
{
    const char *error=NULL; unsigned pages=0,glyphs=0,controls=0,selections=0; unsigned hist[4096]={0};
    uint32_t static_dialogs[8]; uint16_t static_pages[8]; unsigned static_count=0;
    if(size>=12 && !memcmp(b+8,"MSG1",4)) {
        P4GoldenMessage m; error=p4_golden_message_open(b,size,&m);
        if(!error) for(uint32_t i=0;i<m.dialogs;++i) {
            P4GoldenDialog d; error=p4_golden_message_dialog(&m,i,&d); if(error) break; selections+=d.kind==1;
            for(unsigned j=0;j<d.count;++j) {
                P4GoldenText t; error=p4_golden_message_page(&d,(uint16_t)j,&t); if(error) break; ++pages;
                int is_static=d.kind==0; unsigned page_glyphs=0;
                P4GoldenToken token;
                do {
                    error=p4_golden_text_next(&t,&token); if(error) break;
                    if(token.kind==P4_GOLDEN_GLYPH) { ++glyphs; ++page_glyphs; }
                    if(token.kind==P4_GOLDEN_CONTROL) {
                        ++controls; ++hist[token.code&4095];
                        if(token.code!=0xf205 && token.code!=0xf141 && token.code!=0xf121) is_static=0;
                    }
                } while(token.kind!=P4_GOLDEN_END);
                if(error) break;
                if(is_static && page_glyphs>20 && page_glyphs<140 && static_count<8) {
                    static_dialogs[static_count]=i; static_pages[static_count]=(uint16_t)j; ++static_count;
                }
            }
            if(error) break;
        }
        printf("{\"refusal\":%s",error?"true":"false");
        if(error) printf(",\"reason\":\"%s\"",error);
        else printf(",\"dialogs\":%u,\"pages\":%u,\"speakers\":%u,\"selections\":%u,\"glyphs\":%u,\"controls\":%u",m.dialogs,pages,m.speakers,selections,glyphs,controls);
        printf(",\"control_ids\":{"); int first=1;
        for(unsigned i=0;i<4096;++i) if(hist[i]) { printf("%s\"%04x\":%u",first?"":",",0xf000u+i,hist[i]); first=0; }
        printf("},\"static_pages\":[");
        for(unsigned i=0;i<static_count;++i) printf("%s[%u,%u]",i?",":"",static_dialogs[i],static_pages[i]);
        puts("]}");
    } else {
        P4GoldenFont f; uint8_t packed[P4_GOLDEN_FONT_BITMAP_MAX]; error=p4_golden_font_open(b,size,&f);
        if(!error) for(unsigned i=0;i<f.count;++i) {
            int l,r; error=p4_golden_font_glyph(&f,(uint16_t)i,packed,sizeof(packed));
            if(!error) error=p4_golden_font_metrics(&f,(uint16_t)i,&l,&r);
            if(error) break;
        }
        if(error) printf("{\"refusal\":true,\"reason\":\"%s\"}\n",error);
        else printf("{\"refusal\":false,\"font_glyphs\":%u,\"width\":%u,\"height\":%u}\n",f.count,f.width,f.height);
    }
    fflush(stdout);
}
static uint8_t *file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb"); if(!f) return NULL;
    if(fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long n=ftell(f); rewind(f); if(n<0 || n>64*1024*1024) { fclose(f); return NULL; }
    uint8_t *b=malloc((size_t)n); if(!b || fread(b,1,(size_t)n,f)!=(size_t)n) { free(b); fclose(f); return NULL; }
    fclose(f); *size=(size_t)n; return b;
}
/* Static witness policy: default-style envelope and completed-page fence only.
 * Dynamic text, colours, timers and substitutions are not silently discarded. */
static const char *presentation(void *context,const P4GoldenToken *t,P4GoldenTextPen *pen)
{
    (void)context; (void)pen;
    if(t->code==0xf205 && t->argument_count==1 && t->arguments[0]==255 && t->arguments[1]==255) return NULL;
    if((t->code==0xf141 || t->code==0xf121) && !t->argument_count) return NULL;
    return "witness.dynamic-control";
}
int main(int argc,char **argv)
{
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);
    _setmode(_fileno(stdout),_O_BINARY);
#endif
    if(argc==2 && (!strcmp(argv[1],"scan") || !strcmp(argv[1],"inflate"))) {
        uint8_t header[4];
        while(fread(header,1,4,stdin)==4) {
            size_t size=u32(header); if(size>64u*1024u*1024u) return 2;
            uint8_t *b=malloc(size); if(!b || fread(b,1,size,stdin)!=size) { free(b); return 3; }
            if(!strcmp(argv[1],"scan")) scan(b,size);
            else {
                size_t n=p4_golden_crilayla_size(b,size);
                if(!n || n>512u*1024u*1024u) { free(b); return 4; }
                uint8_t *plain=malloc(n);
                if(!plain || p4_golden_crilayla(b,size,plain,n)) { free(plain); free(b); return 4; }
                uint8_t length[4]; for(unsigned i=0;i<4;++i) length[i]=(uint8_t)(n>>(8*i));
                int failed=fwrite(length,1,4,stdout)!=4 || fwrite(plain,1,n,stdout)!=n || fflush(stdout);
                free(plain); if(failed) { free(b); return 4; }
            }
            free(b);
        }
        return ferror(stdin)?3:0;
    }
    if(argc!=7) return 2;
    size_t fs,ms; uint8_t *fb=file(argv[1],&fs),*mb=file(argv[2],&ms); if(!fb || !mb) { free(fb); free(mb); return 3; }
    P4GoldenFont f; P4GoldenMessage m; P4GoldenDialog d; P4GoldenText text;
    const char *error=p4_golden_font_open(fb,fs,&f);
    if(!error) error=p4_golden_message_open(mb,ms,&m);
    char *end; unsigned long index=strtoul(argv[3],&end,10); if(*end || index>UINT32_MAX) error="witness.dialog-index";
    if(!error) error=p4_golden_message_dialog(&m,(uint32_t)index,&d);
    unsigned long page=strtoul(argv[4],&end,10); if(*end || page>UINT16_MAX) error="witness.page-index";
    if(!error) error=p4_golden_message_page(&d,(uint16_t)page,&text);
    unsigned long width=strtoul(argv[6],&end,10); if(*end || width<64 || width>4096) error="witness.width";
    size_t n=(size_t)width*512*4; uint8_t *rgba=calloc(n,1);
    if(!rgba) error="witness.allocation";
    P4GoldenTextPen pen={8,8,{255,255,255,255}};
    if(!error) error=p4_golden_message_render(&f,text,rgba,(uint32_t)width,512,n,&pen,presentation,NULL);
    if(!error) {
        FILE *out=fopen(argv[5],"wb");
        if(!out) error="witness.output";
        else { if(fwrite(rgba,1,n,out)!=n) error="witness.output"; if(fclose(out)) error="witness.output"; }
    }
    if(error) fprintf(stderr,"%s\n",error);
    free(rgba); free(fb); free(mb); return error?1:0;
}
