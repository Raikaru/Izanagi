#include "../src/platform/golden/golden_icons.h"
#include "../src/platform/golden/golden_mode_boot.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void w16(uint8_t *b,unsigned v) { b[0]=(uint8_t)v; b[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *b,uint32_t v) { w16(b,v); w16(b+2,v>>16); }
static int cache_fixture(char **argv)
{
    uint8_t bytes[5200]={0};
    memcpy(bytes+8,"SPR0",4); w32(bytes+4,sizeof bytes);
    w16(bytes+20,1); w16(bytes+22,0x254); w32(bytes+24,32); w32(bytes+28,40);
    w32(bytes+36,5120);
    for(unsigned i=0;i<0x254;++i) w32(bytes+40+i*8+4,4900);
    w32(bytes+4900+0x5c,2); w32(bytes+4900+0x60,1); bytes[4900+0x18]=1;
    bytes[5120]=2; w32(bytes+5124,72); memcpy(bytes+5128,"TMX0",4);
    w16(bytes+5138,2); w16(bytes+5140,1);
    bytes[5184]=255; bytes[5187]=128; bytes[5189]=255; bytes[5191]=128;
    char hash[65],key[65],again[65],reason[192];
    p4_content_digest(bytes,sizeof bytes,hash);
    P4GoldenCacheIdentityEntry sources[]={{"P4G.exe",hash},{"synthetic.spr",hash}};
    P4GoldenCacheIdentityEntry versions[]={{"golden-native-atlas","spr0-tmx-dds-bc1237@3"},{"purpose","golden-atlas-images@1"}};
    P4GoldenCacheIdentity id={sources,2,versions,2};
    P4GoldenCachePaths paths={argv[2],argv[3],argv[4],NULL};
    assert(p4_golden_icons_cache_build(P4_GAME_MODE_PS2,&paths,&id,bytes,sizeof bytes,key,reason,sizeof reason));
    assert(!p4_golden_icons_cache_build(P4_GAME_MODE_GOLDEN,&paths,&id,bytes,sizeof bytes,key,reason,sizeof reason));
    static const int indices[]={-1,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x4c,0x4d,0x4f,0x50,0x51,0x52,0x53,0x54,0x6c,0x251,0x252,0x253};
    P4GoldenCacheTable tables[P4_GOLDEN_ICON_COUNT]={0}; char names[P4_GOLDEN_ICON_COUNT][32];
    for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) {
        if(!i) strcpy(names[i],"atlas/000-none");
        else snprintf(names[i],sizeof names[i],"atlas/001-%03x",(unsigned)indices[i]);
        tables[i].name=names[i];
    }
    assert(!p4_golden_cache_load(&paths,&id,tables,P4_GOLDEN_ICON_COUNT,again,reason,sizeof reason));
    assert(!strcmp(key,again) && tables[0].size==48 && !tables[0].data[47]);
    for(unsigned i=1;i<P4_GOLDEN_ICON_COUNT;++i) {
        assert(tables[i].size==52 && tables[i].data[40]==1);
        assert(!memcmp(tables[i].data+44,"\0\xff\0\xff\xff\0\0\xff",8));
    }
    p4_golden_cache_free(tables,P4_GOLDEN_ICON_COUNT);
    P4GoldenIconImage images[P4_GOLDEN_ICON_COUNT]={0};
    assert(p4_golden_icons_images(P4_GAME_MODE_GOLDEN,images,reason,sizeof reason)); /* conversion alone grants no admission */
    puts("Golden synthetic atlas/cache conversion passed"); return 0;
}
int main(int argc,char **argv)
{
    if(argc==5 && !strcmp(argv[1],"cache")) return cache_fixture(argv);
    P4GameMode mode=argc==2 && !strcmp(argv[1],"golden")?P4_GAME_MODE_GOLDEN:P4_GAME_MODE_PS2;
    assert(!p4_golden_icons_register(mode));
    const P4GoldenModeAdapters *a=p4_golden_mode_registered();
    if(mode==P4_GAME_MODE_PS2) {
        assert(!a->count);
        P4GoldenIconImage icons[P4_GOLDEN_ICON_COUNT]={0};
        assert(p4_golden_icons_images(mode,icons,NULL,0));
        for(unsigned i=0;i<P4_GOLDEN_ICON_COUNT;++i) assert(!icons[i].object);
    } else {
        assert(a->count==1 && !strcmp(a->items[0].name,"expanded item icon sources"));
        P4ResourceRef icon;
        assert(!p4_golden_icons_item_ref(0x80000,&icon));
        assert(!strcmp(icon.owner,"p4pc.golden") && !strcmp(icon.key,"p4pc.golden:sprite-050") && icon.revision==1);
        assert(!p4_golden_icons_item_ref(0,&icon) && !strcmp(icon.key,"p4pc.golden:sprite-none"));
        assert(!p4_golden_icons_arcana_ref(25,1,&icon) && !strcmp(icon.key,"p4pc.golden:sprite-252"));
        assert(p4_golden_icons_arcana_ref(0,0,&icon));
    }
    puts("Golden icon adapter registration/isolation passed"); return 0;
}
