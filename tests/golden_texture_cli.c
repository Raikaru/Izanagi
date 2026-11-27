/* Local-only decoder driver. Inputs and RGBA output remain outside the repo. */
#include "p4pc/golden_atlas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int word(FILE *f,uint32_t v)
{ uint8_t b[4]; for(unsigned i=0;i<4;++i) b[i]=(uint8_t)(v>>(i*8)); return fwrite(b,1,4,f)==4?0:-1; }
static int image(FILE *f,int index,uint32_t w,uint32_t h,const uint8_t *rgba)
{
    size_t n=(size_t)w*h*4;
    return word(f,(uint32_t)index)||word(f,w)||word(f,h)||fwrite(rgba,1,n,f)!=n;
}
int main(int argc,char **argv)
{
    if(argc<3) return 2;
    FILE *in=fopen(argv[1],"rb"); if(!in) return 3;
    if(fseek(in,0,SEEK_END)) { fclose(in); return 3; }
    long size=ftell(in); rewind(in);
    if(size<0 || (unsigned long)size>P4_GOLDEN_TEXTURE_BUDGET) { fclose(in); return 3; }
    uint8_t *bytes=malloc((size_t)size);
    if(!bytes || fread(bytes,1,(size_t)size,in)!=(size_t)size) { free(bytes); fclose(in); return 3; }
    fclose(in);
    FILE *out=fopen(argv[2],"wb"); if(!out) { free(bytes); return 3; }
    int result=1;
    if(argc>3) {
        P4GoldenAtlas a={0};
        if(p4_golden_atlas_open(bytes,(size_t)size,&a)) { fprintf(stderr,"SPR decode refused\n"); goto done; }
        if(word(out,(uint32_t)argc-3)) { p4_golden_atlas_free(&a); goto done; }
        result=0;
        for(int i=3;i<argc;++i) {
            char *end; long index=strtol(argv[i],&end,0); P4GoldenSprite s;
            if(*end || index<0 || index>32767 || p4_golden_atlas_sprite(&a,(int)index,&s)) { result=1; break; }
            uint32_t w=(uint32_t)(s.x1-s.x0),h=(uint32_t)(s.y1-s.y0); size_t n=(size_t)w*h*4;
            uint8_t *rgba=malloc(n);
            if(!rgba || p4_golden_atlas_crop(&a,(int)index,rgba,n) || image(out,(int)index,w,h,rgba)) result=1;
            free(rgba); if(result) break;
        }
        printf("SPR textures=%zu sprites=%zu selected=%d\n",a.texture_count,a.sprite_count,argc-3);
        p4_golden_atlas_free(&a);
    } else {
        P4GoldenTexture t={0};
        if(p4_golden_texture_decode(bytes,(size_t)size,&t)) { fprintf(stderr,"texture decode refused\n"); goto done; }
        result=word(out,t.level_count);
        for(unsigned i=0;!result && i<t.level_count;++i) result=image(out,(int)i,t.levels[i].width,t.levels[i].height,t.levels[i].rgba);
        printf("texture width=%u height=%u mips=%u srgb=%u\n",t.levels[0].width,t.levels[0].height,t.level_count,t.srgb);
        p4_golden_texture_free(&t);
    }
done:
    if(fclose(out)) result=1;
    free(bytes); return result;
}
