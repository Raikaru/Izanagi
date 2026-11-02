#include "p4pc/texture_dump.h"
#include "p4png.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"texture_dump:%d: %s\n",__LINE__,#x); p4_texture_dump_finish(); return 1; } } while (0)
int main(int argc, char **argv)
{
    uint8_t rgba[24], palette[64];
    P4TexturePixels image={3,2,P4_TEX_PAL4,P4_PAL_RGBA8888,16,rgba,palette};
    P4TextureDumpStats stats;
    char first[65], second[65];
    int expected_failure=argc>2 && strcmp(argv[2],"fail")==0;
    if (argc<2) return 2;
    for (size_t i=0;i<sizeof rgba;++i) rgba[i]=(uint8_t)(i*37u+11u);
    for (size_t i=0;i<sizeof palette;++i) palette[i]=(uint8_t)(i*13u+7u);
    CHECK(p4_texture_dump_start(NULL,0)==0);
    CHECK(!p4_texture_dump_active());
    /* Disabled must not even inspect these invalid pointers. */
    CHECK(p4_texture_dump_submit(NULL,P4_TEXTURE_DISC,NULL,NULL)==0);
    CHECK(p4_texture_dump_finish()==0);
    CHECK(p4_texture_dump_start(argv[1],1)==0);
    CHECK(p4_texture_key(&image,first)==0);
    CHECK(p4_texture_dump_submit(&image,P4_TEXTURE_SYNTHETIC,"DATA/TEST\".TMX",NULL)==0);
    if (expected_failure) {
        CHECK(p4_texture_dump_finish()==-1);
        p4_texture_dump_stats(&stats);
        CHECK(stats.errors>0);
        puts("texture_dump: output failure reported incomplete");
        return 0;
    }
    /* The ordinary capture writer may run while the dump worker first uses
     * that same encoder. CRC state must not depend on mutable global startup. */
    {
        char capture[1024];
        int n=snprintf(capture,sizeof capture,"%s/concurrent.png",argv[1]);
        CHECK(n>0 && (size_t)n<sizeof capture);
        CHECK(p4png_write_rgba(capture,3,2,rgba)==0);
    }
    /* Content identity, not source name/pointer, deduplicates. */
    CHECK(p4_texture_dump_submit(&image,P4_TEXTURE_SYNTHETIC,"another-source",NULL)==0);
    palette[63]^=1;
    CHECK(p4_texture_key(&image,second)==0 && strcmp(first,second)!=0);
    CHECK(p4_texture_dump_submit(&image,P4_TEXTURE_SYNTHETIC,"palette-swap",NULL)==0);
    CHECK(p4_texture_dump_submit(NULL,P4_TEXTURE_FRAMEBUFFER,"display",NULL)==0);
    CHECK(p4_texture_dump_submit(NULL,P4_TEXTURE_UNKNOWN,"unknown",NULL)==0);
    /* The writer must own an immutable snapshot, not these caller buffers. */
    memset(rgba,0,sizeof rgba); memset(palette,0,sizeof palette);
    CHECK(p4_texture_dump_finish()==0);
    p4_texture_dump_stats(&stats);
    CHECK(stats.submitted==2 && stats.completed==2 && stats.excluded==2 && stats.errors==0);
    CHECK(stats.queued_bytes==0 && stats.peak_bytes<=24u*1024u*1024u);
    puts("texture_dump: immutable palette variants, deduplication and exclusions passed");
    return 0;
}
