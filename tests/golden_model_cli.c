/* Streamed owner-local qualification: framed stdin, JSON metadata only on stdout.
 * No extracted assets are persisted or printed. --static also attempts scene conversion. */
#include "p4pc/golden_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static void text(const char *s)
{
    putchar('"'); for(;*s;++s) { unsigned char c=(unsigned char)*s; if(c=='"' || c=='\\') putchar('\\'); if(c>=32) putchar(c); } putchar('"');
}
int main(int argc, char **argv)
{
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);
#endif
    int scene_check=argc>1 && !strcmp(argv[1],"--static");
    for(;;) {
        uint8_t header[4]; size_t n=fread(header,1,4,stdin); if(!n) break; if(n!=4) return 2;
        uint32_t size=header[0]|(uint32_t)header[1]<<8|(uint32_t)header[2]<<16|(uint32_t)header[3]<<24;
        if(size>P4_GOLDEN_MODEL_BUDGET) return 2;
        uint8_t *bytes=malloc(size?size:1); if(!bytes || fread(bytes,1,size,stdin)!=size) { free(bytes); return 2; }
        P4GoldenModel model={0}; char error[256]=""; uint64_t counts[65536]={0}; unsigned tim2=0,bmp=0,image_failures=0;
        if(p4_golden_model_parse(bytes,size,&model,error,sizeof error)) { printf("{\"error\":"); text(error); puts("}"); free(bytes); fflush(stdout); continue; }
        for(uint32_t i=0;i<model.count;++i) {
            const P4GoldenModelNode *node=&model.nodes[i]; if(node->type<65536) ++counts[node->type];
            if(node->type==0x8013) {
                const uint8_t *p=model.bytes+node->args;
                uint32_t length=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
                P4GoldenTexture image={0};
                if(p4_golden_model_image(p+4,length,&image)) ++image_failures;
                else { if(!memcmp(p+4,"TIM2",4)) ++tim2; else ++bmp; free(image.storage); }
            }
        }
        P4GoldenMotions motions={0};
        if(p4_golden_model_motions(&model,&motions,error,sizeof error)) {
            printf("{\"error\":"); text(error); puts("}"); p4_golden_model_free(&model); free(bytes); fflush(stdout); continue;
        }
        printf("{\"class\":%u,\"nodes\":%u,\"tim2\":%u,\"bmp\":%u,\"image_failures\":%u,\"counts\":{",model.nodes[0].type,model.count,tim2,bmp,image_failures);
        int comma=0;
        for(unsigned i=0;i<65536;++i) if(counts[i]) { printf("%s\"%04x\":%llu",comma?",":"",i,(unsigned long long)counts[i]); comma=1; }
        printf("}");
        unsigned sentinel=0, external=0, neck=0, time_sentinel=0; uint64_t keys=0;
        unsigned formats[3]={0}, dimensions[5]={0};
        for(uint32_t i=0;i<motions.channel_count;++i) {
            sentinel+=(motions.channels[i].flags&P4_GM_CHANNEL_SENTINEL)!=0;
            external+=(motions.channels[i].flags&P4_GM_CHANNEL_EXTERNAL)!=0;
            neck+=(motions.channels[i].flags&P4_GM_CHANNEL_NECK)!=0;
            time_sentinel+=(motions.channels[i].flags&P4_GM_CHANNEL_TIME_SENTINEL)!=0;
        }
        for(uint32_t i=0;i<model.count;++i) if(model.nodes[i].type==12) {
            const uint8_t *p=model.bytes+model.nodes[i].args;
            uint32_t f=p[0], d=p[4], k=p[8]|(uint32_t)p[9]<<8|(uint32_t)p[10]<<16|(uint32_t)p[11]<<24;
            ++formats[f==0x80?0:f==0x81?1:2]; ++dimensions[d]; keys+=k;
        }
        printf(",\"motion_clips\":%u,\"motion_channels\":%u,\"motion_sentinels\":%u,\"motion_external\":%u,\"motion_neck_channels\":%u,\"motion_time_sentinels\":%u,\"motion_keys\":%llu",
               motions.clip_count,motions.channel_count,sentinel,external,neck,time_sentinel,(unsigned long long)keys);
        printf(",\"curve_formats\":[%u,%u,%u],\"curve_dimensions\":[%u,%u,%u],\"clips\":[",formats[0],formats[1],formats[2],dimensions[1],dimensions[3],dimensions[4]);
        for(uint32_t i=0;i<motions.clip_count;++i) {
            const P4GoldenMotion *c=&motions.clips[i]; const P4GoldenModelNode *n=&model.nodes[c->node];
            printf("%s{\"model\":%u,\"ordinal\":%u,\"name\":",i?",":"",c->model,c->ordinal);
            text((const char *)model.bytes+n->name);
            printf(",\"channels\":%u,\"flags\":%u,\"start\":%.9g,\"end\":%.9g,\"fps\":%.9g}",c->count,c->flags,c->start,c->end,c->fps);
        }
        printf("]");
        if(scene_check && (model.nodes[0].type==P4_GM_AMD || model.nodes[0].type==P4_GM_GMO)) {
            P4GoldenModelScene scene={0};
            if(p4_golden_model_scene(bytes,size,&scene,error,sizeof error)) { printf(",\"static_refusal\":"); text(error); }
            else {
                printf(",\"bones\":%u,\"meshes\":%u,\"helpers\":%u",scene.bone_count,scene.mesh_count,scene.helper_count);
                if(p4_golden_model_static_check(&scene,error,sizeof error)) { printf(",\"render_refusal\":"); text(error); }
                p4_golden_model_scene_free(&scene);
            }
        }
        puts("}"); fflush(stdout); p4_golden_model_motions_free(&motions); p4_golden_model_free(&model); free(bytes);
    }
    return ferror(stdin)?2:0;
}
