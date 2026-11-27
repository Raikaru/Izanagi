/* Real M5 cache/input, builder, resource metadata, source-ledger and native-use
 * services. Recovery frontends, mode selection and the unused raster backend
 * are supplied here. Steam mode never bypasses adapter admission or boots. */
#include "../src/platform/golden/golden_mode_image_bind.h"
#include "../src/platform/golden/golden_mode_literal_identity.h"
#include "golden_literal_consumers.h"
#include "p4pc/golden_table_cache.h"
#include "p4pc/golden_exp.h"
#include "p4pc/golden_fusion_runtime.h"
#include "p4pc/golden_fusion_pool.h"
#include "p4pc/content_digest.h"
#include "p4pc/package_boot.h"
#include "p4pc/package_memory.h"
#include "p4pc/mod_save.h"
#include "golden_cache_fixture.h"
#include "guest_test.h"
#include "no_resource_work.h"
#include "mod_save_internal.h"
#include <assert.h>
#include <string.h>

static P4GameMode mode = P4_GAME_MODE_GOLDEN;
P4GameMode p4_game_mode(void) { return mode; }
/* Counts/hashes only, measured from the independently pinned M5/M5b readers
 * for Steam10836033 English. No game payload is embedded in this fixture. */
static const char *const steam_hashes[] = {
    "1274c70b678fda686885f45519c57cbf651b05a28df1d5e0f681ca73f6779c44",
    "10074a688452572b931bb748d6912b7e5fb0f71270d6343f4f1084797eaf6fb7",
    "cf0146b0ca811104d863fc71152a89d161caf6885f281b28e9c7488b0492a3ae",
    "303fc1b422fb1c47975e2da8dbd43727a2386c73a81904e4af8ad5638a5fa9a7",
    P4_GOLDEN_LITERAL_MANIFEST_SHA256, /* metadata, not Steam payload */
    "c19d347308602bf32d9fb264609c70cfffc65a3220cb8374fe96d60b774c7709",
    "a69487e5ca2202a14119cbf164e9a03a2765c7e8417022bbdcef760af270b9b4",
    "b997a50b0298e2a47150f738e6a69ee5ae253704b0fa658c40eac0ad9bb24ace"
};
uint8_t datLevelFromExp(int32_t experience);
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected image runtime failure: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const struct P4PackageProvider *p, const char *reason)
{ (void)p; p4_package_boot_runtime_failed(reason); }
static int compare(const void *a, const void *b)
{ return strcmp(((const P4GoldenCacheTable *)a)->name, ((const P4GoldenCacheTable *)b)->name); }
static void cleanup(const char *name, void *user)
{
    if (!strcmp(name,".") || !strcmp(name,"..")) return;
    char path[4096]; int n = snprintf(path,sizeof path,"%s/%s",(const char *)user,name);
    assert(n > 0 && (size_t)n < sizeof path);
    if (p4_os_is_dir_nofollow(path)) {
        assert(!p4_os_list_dir_utf8(path,cleanup,path)); assert(!p4_os_rmdir(path));
    } else assert(!p4_os_remove_utf8(path));
}
static void synthetic(P4GoldenModeIdentityArtifacts *a)
{
    memset(a,0,sizeof *a); a->inheritance_size = 624;
    for (unsigned i=0;i<99;++i) { unsigned v=i*i*100; for(unsigned j=0;j<4;++j) a->experience[i*4+j]=(uint8_t)(v>>(j*8)); }
    for (unsigned i=0;i<sizeof a->fusion.charts;++i) ((uint8_t *)a->fusion.charts)[i]=(uint8_t)(i%28);
    a->fusion.gates[25*8]=123; a->fusion.gates[25*8+4]=42; a->fusion.gates[25*8+6]=1;
    for (unsigned i=0;i<624;++i) a->inheritance[i]=(uint8_t)(i%9);
    for (unsigned i=0;i<18;++i) a->fusion.limits[i]=(uint8_t)i;
    for (unsigned i=0;i<532;++i) a->fusion.affinities[i]=(uint8_t)(i%127);
    const unsigned groups[]={2,3,4,5,6,12}, counts[]={3,1,4,4,4,1}; unsigned row=0;
    for (unsigned g=0;g<6;++g) for(unsigned k=0;k<counts[g];++k,++row) {
        P4GoldenRecipe *r=&a->fusion.recipes[row]; r->result=(uint16_t)(row+1); r->material_count=(uint8_t)groups[g];
        for(unsigned j=0;j<groups[g];++j) r->materials[j]=(uint16_t)(j+1);
    }
    p4t_literal_synthetic(a);
}
static unsigned word(const uint8_t *p)
{ return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24; }
static void check_bound(const P4GoldenModeIdentityArtifacts *a)
{
    assert(!memcmp(P4_G2H(0x005dd6e0),a->experience,396));
    for (unsigned level=1;level<=99;++level) {
        unsigned threshold=word(a->experience+(level-1u)*4u);
        assert(P4_G32(0x005dd6e0u+(level-1u)*4u)==threshold);
        assert(datLevelFromExp((int32_t)threshold)==level);
        assert(datLevelFromExp((int32_t)threshold-1)==level-1);
    }
    mode=P4_GAME_MODE_PS2;
    assert(datLevelFromExp(20)==2 && datLevelFromExp(47)==3 && datLevelFromExp(-1)==0);
    mode=P4_GAME_MODE_GOLDEN;
    assert(!memcmp(P4_G2H(0x00641c20),a->fusion.limits,18));
    assert(!memcmp(P4_G2H(0x00641c40),a->fusion.affinities,532));
    assert(!memcmp(P4_G2H(0x00641e60),a->inheritance,576));
    assert(p4_golden_fusion_gate_count()==26);
    for(unsigned i=0;i<26;++i) {
        assert((uint32_t)p4_golden_fusion_gate_flag(i)==word(a->fusion.gates+i*8));
        const uint8_t *p=a->fusion.gates+i*8+4;
        assert(p4_golden_fusion_gate_persona(i)==(int16_t)(p[0]|(unsigned)p[1]<<8));
    }
    for(unsigned c=0;c<3;++c) for(unsigned x=0;x<28;++x) for(unsigned y=x;y<28;++y) {
        assert(p4_golden_fusion_chart(c,x,y)==a->fusion.charts[c][x*28+y]);
        assert(p4_golden_fusion_chart(c,y,x)==a->fusion.charts[c][x*28+y]);
    }
    assert(p4_golden_recipes_count()==17);
    for(unsigned i=0;i<17;++i) {
        assert(p4_golden_recipe_result(i)==a->fusion.recipes[i].result);
        assert(p4_golden_recipe_material_count(i)==a->fusion.recipes[i].material_count);
        for(unsigned j=0;j<a->fusion.recipes[i].material_count;++j)
            assert(p4_golden_recipe_material(i,j)==a->fusion.recipes[i].materials[j]);
    }
    assert(p4_golden_recipe_group_index(12,0)==16);
    P4GoldenFusionPool pool={0};
    for(unsigned i=0;i<96;++i) assert(p4_golden_fusion_pool_add(&pool,(uint16_t)(i+1))==1);
    assert(pool.count==96 && p4_golden_fusion_pool_add(&pool,97)==-1);
    size_t literal_count;
    const P4GoldenLiteralSource *sources = p4_golden_image_literal_sources(&literal_count);
    const P4GoldenImageObject *registry = p4_golden_image_registry(NULL);
    for (size_t i = 0; i < literal_count; ++i)
        if (registry[5u + i].address)
            assert(!memcmp(P4_G2H(registry[5u + i].address),
                a->literal_bytes + sources[i].output_offset, sources[i].size));
}
static void check_literal_ledger(void)
{
    size_t count, bytes = 0, open = 0;
    const P4GoldenLiteralObject *rows = p4_golden_image_literal_registry(&count);
    assert(count == 1043);
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenLiteralObject *r = rows + i;
        assert(r->name[0] && r->reason[0] && r->size);
        assert(!strcmp(r->decision, "OPEN-retain") || !strcmp(r->decision, "D10c") ||
               !strcmp(r->decision, "pending-seam") || !strcmp(r->decision, "bound"));
        if (!strcmp(r->decision, "OPEN-retain")) { assert(r->proof_path[0]); ++open; }
        bytes += r->size;
    }
    assert(bytes == 249428);
    printf("golden_image: L1 ledger objects=%zu bytes=%zu L1-open=%zu\n", count, bytes, open);
}
static void retain_literals(int initialize)
{
    static uint8_t original[249428];
    size_t count, offset = 0, image_count;
    const P4GoldenLiteralObject *rows = p4_golden_image_literal_registry(&count);
    const P4GoldenImageObject *images = p4_golden_image_registry(&image_count);
    for (size_t i = 0; i < count; ++i) {
        uint8_t *guest = P4_G2H(rows[i].address);
        assert(rows[i].size <= sizeof original - offset);
        if (initialize > 0) {
            if (initialize == 1) memset(guest, 0x6d, rows[i].size);
            memcpy(original + offset, guest, rows[i].size);
        } else {
            size_t prefix = 0;
            if (!initialize && !strcmp(rows[i].decision, "bound")) {
                char name[32]; snprintf(name, sizeof name, "image/l1/%08x", rows[i].address);
                for (size_t j = 0; j < image_count; ++j)
                    if (!strcmp(images[j].name, name)) prefix = images[j].retail_size;
            }
            assert(prefix <= rows[i].size);
            assert(!memcmp(guest + prefix, original + offset + prefix, rows[i].size - prefix));
        }
        offset += rows[i].size;
    }
}
static void check_steam_literals(const char *directory, const P4GoldenModeIdentityArtifacts *a)
{
    size_t count, pe_count = 0, archive_count = 0;
    const P4GoldenLiteralSource *sources = p4_golden_image_literal_sources(&count);
    P4GoldenPEDataSpan spans[P4_GOLDEN_LITERAL_SOURCE_COUNT];
    uint8_t fresh[P4_GOLDEN_LITERAL_PE_BYTES], curve[396];
    char path[1200], digest[65], reason[256];
    for (size_t i = 0; i < count; ++i) if (sources[i].section[0])
        spans[pe_count++] = (P4GoldenPEDataSpan){sources[i].section, sources[i].sha256,
            sources[i].offset, sources[i].read_size, sources[i].output_offset};
    snprintf(path, sizeof path, "%s/P4G.exe", directory);
    assert(!p4_golden_exp_image_read(path, curve, spans, pe_count, fresh, sizeof fresh, digest, reason, sizeof reason));
    assert(!memcmp(curve, P4_G2H(0x005dd6e0), sizeof curve));
    for (size_t i = 0; i < count; ++i) if (sources[i].section[0]) {
        if (!sources[i].converter)
            assert(!memcmp(fresh + sources[i].output_offset,
                           a->literal_bytes + sources[i].output_offset, sources[i].size));
        else {
            uint8_t normalized[84];
            assert(sources[i].converter == 1 && i + 1 < count);
            assert(!p4_golden_image_normalize_queues(fresh + sources[i].output_offset,
                fresh + sources[i + 1].output_offset, UINT64_C(0x140000000), 0x1e41b2c0u,
                0x00796520u, normalized));
            assert(!memcmp(normalized, a->literal_bytes + sources[i].output_offset, sizeof normalized));
        }
    }
    for (size_t i = 0; i < count; ++i) if (sources[i].constant) {
        assert(!memcmp(sources[i].constant, a->literal_bytes + sources[i].output_offset, sources[i].size));
        p4_content_digest(sources[i].constant, sources[i].size, digest);
        assert(!strcmp(digest, sources[i].sha256));
    }
    P4GoldenCpk cpk = {0}; P4GoldenCpkEntry entry;
    uint8_t *bin = NULL; const uint8_t *pack, *member;
    size_t bin_size, pack_size, member_size;
    snprintf(path, sizeof path, "%s/data.cpk", directory);
    assert(!p4_golden_cpk_open(&cpk, path, reason, sizeof reason));
    assert(!p4_golden_cpk_find(&cpk, "init_free.bin", &entry));
    assert(!p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, reason, sizeof reason));
    assert(!p4_golden_bin_find(bin, bin_size, "init/fclTable.bin", &pack, &pack_size));
    for (size_t i = 0; i < count; ++i) if (sources[i].member_chain[0]) {
        const char *member_name = strrchr(sources[i].member_chain, ':');
        assert(member_name && !p4_golden_pak2_find(pack, pack_size, member_name + 1, &member, &member_size));
        p4_content_digest(member, member_size, digest);
        assert(!strcmp(digest, sources[i].sha256));
        P4GoldenSpan payload; uint32_t rows;
        assert(!p4_golden_record_table(member, member_size, 0, &payload, &rows));
        assert(rows == sources[i].rows && payload.size == sources[i].size);
        assert(!memcmp(payload.bytes, a->literal_bytes + sources[i].output_offset, payload.size));
        ++archive_count;
    }
    free(bin); p4_golden_cpk_close(&cpk);
    check_bound(a); /* Every fixed prefix equals these independently reread sources. */
    printf("golden_literals: fresh Steam equality: %zu PE spans, %zu FTD payloads (including auxiliary map)\n",
           pe_count, archive_count);
}
static void corrupt_cache(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
    const P4GoldenCacheTable *tables, size_t count, const char *key)
{
    char path[4096], hash[65], actual_key[65], reason[256];
    P4GoldenCacheTable *check=calloc(count,sizeof *check); assert(check);
    for(size_t i=0;i<count;++i) check[i].name=tables[i].name;
    const P4GoldenCacheTable *exp=NULL;
    for(size_t i=0;i<count;++i) if(!strcmp(tables[i].name,"image/experience")) exp=tables+i;
    assert(exp && exp->size==396); p4_content_digest(exp->data,exp->size,hash);
    for(unsigned trial=0;trial<3;++trial) {
        if(trial==0) snprintf(path,sizeof path,"%s/golden-cache/%s/image-index.json",paths->pref,key);
        else {
            snprintf(path,sizeof path,"%s/golden-cache/%s/objects/%s",paths->pref,key,hash);
        }
        if(trial<2) assert(!p4_os_remove_utf8(path));
        else {
            uint8_t corrupt[396]; memcpy(corrupt,exp->data,sizeof corrupt); corrupt[0]^=1;
            FILE *f=p4_os_fopen_utf8(path,"wb"); assert(f);
            assert(fwrite(corrupt,1,sizeof corrupt,f)==sizeof corrupt && !fclose(f));
        }
        assert(p4_golden_cache_load(paths,identity,check,count,actual_key,reason,sizeof reason)>0);
        for(size_t i=0;i<count;++i) assert(!check[i].data);
        assert(!p4_golden_cache_store(paths,identity,tables,count,actual_key,reason,sizeof reason));
        assert(!strcmp(actual_key,key));
    }
    free(check);
}

int main(int argc,char **argv)
{
    static P4GoldenModeIdentityArtifacts a; static P4GoldenModeIdentity identity;
    P4GoldenSource source={0}; P4GoldenExtraSource extra_source={0};
    P4GoldenCacheTable image[P4_GOLDEN_IMAGE_EXTRA_COUNT], loaded[P4_GOLDEN_IMAGE_EXTRA_COUNT];
    P4GameTablesDefinitionBuilder b={0}, wrong={0};
    uint8_t *recipes=NULL,*index=NULL;
    char reason[256]="",root[2048],pref[2200],install[2200],program[2200],directory[1024],key[65];
    int steam=argc==2 && !strcmp(argv[1],"steam"); assert(argc==1 || steam);
    if (steam) {
        if(p4_golden_locate(NULL,directory,sizeof directory)) return 77;
        const P4GoldenDomainShape shape={624,472,368,2560,256}; P4GoldenDomainMap map;
        assert(!p4_golden_domain_map_build(&map,&shape,reason,sizeof reason));
        assert(!p4_golden_load(&source,directory,reason,sizeof reason));
        assert(!p4_golden_extra_load(&extra_source,directory,reason,sizeof reason));
        assert(!p4_golden_mode_identity_load(&identity,&a,directory,&source,&extra_source,&map,reason,sizeof reason));
    } else {
        synthetic(&a);
        const P4GoldenModeIdentitySource s={"fixture::fusion","bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
        assert(!p4_golden_mode_identity_build(&identity,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",&s,1,reason,sizeof reason));
    }
    /* Same preference-directory rule as the other cache fixtures: the cache refuses a checkout ancestor, and the build
     * tree can sit inside one (the public CI builds in <checkout>/build/..., so the working directory is not outside it);
     * the fixture's TEMP/TMPDIR is. */
    const char *temp=golden_cache_fixture_temp(); if(!temp) return 1;
    assert(snprintf(root,sizeof root,"%s/golden-image-%llu",temp,(unsigned long long)p4_os_now_ns())>0);
    assert(!p4_os_mkdir(root));
    snprintf(pref,sizeof pref,"%s/pref",root); assert(!p4_os_mkdir(pref));
    snprintf(install,sizeof install,"%s/install",root); assert(!p4_os_mkdir(install));
    snprintf(program,sizeof program,"%s/program",root); assert(!p4_os_mkdir(program));
    P4GoldenCachePaths paths={pref,steam?directory:install,program,NULL};
    assert(!p4_golden_image_encode(&a,image,&recipes,&index));
    check_literal_ledger();
    assert(!p4_golden_image_literal_prepare(reason, sizeof reason));
    static uint8_t banks[P4_TABLE_INPUT_COUNT][176];
    static const unsigned blocks[]={1,2,3,5,6,5,1,3,11};
    P4GoldenCacheTable tables[P4_TABLE_INPUT_COUNT+P4_GOLDEN_IMAGE_EXTRA_COUNT];
    for(unsigned i=0;i<P4_TABLE_INPUT_COUNT;++i) tables[i]=(P4GoldenCacheTable){p4_game_table_input_label((P4GameTableInputKind)i),banks[i],blocks[i]*16u};
    memcpy(tables+P4_TABLE_INPUT_COUNT,image,sizeof image); qsort(tables,sizeof tables/sizeof tables[0],sizeof tables[0],compare);
    for(unsigned i=0;i<P4_GOLDEN_IMAGE_EXTRA_COUNT;++i) loaded[i]=(P4GoldenCacheTable){image[i].name,NULL,0};
    qsort(loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,sizeof loaded[0],compare);
    if(p4_golden_cache_store(&paths,&identity.identity,tables,sizeof tables/sizeof tables[0],key,reason,sizeof reason)) {
        fprintf(stderr,"Image cache build refused: %s\n",reason); return 1;
    }
    if(!steam) corrupt_cache(&paths,&identity.identity,tables,sizeof tables/sizeof tables[0],key);
    assert(!p4_game_tables_builder_init(&b,NULL,1));
    assert(!p4_game_tables_builder_golden_cache(&b,&paths,&identity.identity,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
    assert(p4_golden_mode_image_bind(P4_GAME_MODE_GOLDEN,&b,reason,sizeof reason));
    assert(!p4_golden_mode_image_bind(P4_GAME_MODE_PS2,NULL,reason,sizeof reason));
    assert(!p4_game_table_sources_native_mode_count(b.sources));
    mode=P4_GAME_MODE_PS2;
    assert(p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
    mode=P4_GAME_MODE_GOLDEN;
    uint8_t saved=loaded[0].data[0]; loaded[0].data[0]^=1;
    assert(p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason)); loaded[0].data[0]=saved;
    saved=loaded[2].data[0]; loaded[2].data[0]^=1;
    assert(p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
    loaded[2].data[0]=saved;
    unsigned checked_ledger = 0;
    for (unsigned i = 0; i < P4_GOLDEN_IMAGE_EXTRA_COUNT; ++i)
        if (!strcmp(loaded[i].name, "image/l1-ledger")) {
            saved = loaded[i].data[0]; loaded[i].data[0] ^= 1;
            assert(p4_golden_mode_image_capture(&b, loaded, P4_GOLDEN_IMAGE_EXTRA_COUNT, reason, sizeof reason));
            loaded[i].data[0] = saved; ++checked_ledger;
        }
    assert(checked_ledger == 1);
    assert(p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT-1,reason,sizeof reason));
    assert(!p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
    assert(p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
    p4_golden_cache_free(loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT); /* binder owns retained bytes */
    const struct P4PackageProvider *failed=NULL;
    assert(!p4_game_tables_builder_seal(&b,&failed) && !failed);
    uint8_t retail_save[4100]={0}, original_save[4100];
    retail_save[57]=0x20; retail_save[60]=1;
    memset(retail_save+65,0xff,4); memcpy(original_save,retail_save,sizeof retail_save);
    P4ModSaveImage save_image={retail_save,sizeof retail_save,"image-use-fixture"};
    P4ModSaveCapture *save=NULL;
    assert(!p4_mod_save_capture_image(&save_image,&save,reason,sizeof reason) && !save);
    assert(p4_game_table_sources_native_mode_count(b.sources)==1);
    assert(p4_golden_mode_image_bind(P4_GAME_MODE_GOLDEN,&wrong,reason,sizeof reason));
    assert(!p4t_guest_map(0x0052d000,0x280000));
    retain_literals(1);
    p4t_literal_seed_ps2();
    mode = P4_GAME_MODE_PS2; p4t_literal_consumers(&a, 0); mode = P4_GAME_MODE_GOLDEN;
    retain_literals(2);
    memset(P4_G2H(0x005dd6e0),0xa5,396); memset(P4_G2H(0x00641c20),0xa5,18);
    memset(P4_G2H(0x00641c40),0xa5,532); memset(P4_G2H(0x00641e60),0xa5,576);
    if(p4_golden_mode_image_bind(P4_GAME_MODE_GOLDEN,&b,reason,sizeof reason)) { fprintf(stderr,"%s\n",reason); abort(); }
    assert(p4_golden_mode_image_ready(&b)); check_bound(&a);
    retain_literals(0);
    p4t_literal_consumers(&a, 1);
    assert(!p4_mod_save_capture_image(&save_image,&save,reason,sizeof reason) && save);
    assert(!memcmp(retail_save,original_save,sizeof retail_save));
    assert(p4_golden_mode_image_bind(P4_GAME_MODE_GOLDEN,&b,reason,sizeof reason)); check_bound(&a);
    if (steam) check_steam_literals(directory, &a);
    for(unsigned i=0;i<P4_GOLDEN_IMAGE_COUNT;++i) {
        char hash[65]; p4_content_digest(image[i].data,image[i].size,hash);
        if (steam && i < 5) assert(!strcmp(hash, steam_hashes[i]));
        else if (steam && i >= 5u + P4_GOLDEN_LITERAL_SOURCE_COUNT)
            assert(!strcmp(hash, steam_hashes[i - P4_GOLDEN_LITERAL_SOURCE_COUNT]));
        printf("golden_image: object=%s bytes=%zu sha256=%s\n",image[i].name,image[i].size,hash);
    }
    p4_golden_mode_image_release(&wrong); assert(p4_golden_mode_image_ready(&b));
    p4_golden_mode_image_release(&b); assert(!p4_golden_mode_image_ready(&b));
    for(unsigned i=0;i<396;++i) assert(P4_G8(0x005dd6e0u+i)==0xa5);
    assert(p4_golden_recipes_count()==-1);
    retain_literals(-1);
    assert(!p4_game_tables_builder_destroy(&b));
    /* Existing native-mode records carry image identity without a new format.
     * A PS2 ledger cannot downgrade a missing Golden image to a warning. */
    mode=P4_GAME_MODE_PS2;
    assert(!p4_game_tables_builder_init(&b,NULL,2));
    assert(!p4_game_tables_builder_seal(&b,&failed));
    assert(!p4_golden_mode_image_bind(P4_GAME_MODE_PS2,&b,reason,sizeof reason));
    assert(!p4_game_table_sources_native_mode_count(b.sources) && !p4_golden_mode_image_ready(&b));
    P4ModSaveLoad *load=NULL;
    uint8_t *sidecar=malloc(save->sidecar_size); assert(sidecar);
    memcpy(sidecar,save->sidecar,save->sidecar_size);
    assert(p4_mod_save_stage_image(&save_image,sidecar,save->sidecar_size,&load,reason,sizeof reason));
    assert(strstr(reason,"Golden table cache identity") && !load);
    assert(!memcmp(retail_save,original_save,sizeof retail_save));
    p4_mod_save_capture_free(save);
    assert(!p4_game_tables_builder_destroy(&b));
    assert(!p4_mod_save_reset_session(reason,sizeof reason));
    mode=P4_GAME_MODE_GOLDEN;
    if(!steam) {
        /* A validly hashed but semantically invalid installer object cannot
         * leave either fixed destinations or widened banks partly installed. */
        free(recipes); free(index); recipes=index=NULL;
        a.fusion.recipes[0].material_count=1;
        assert(!p4_golden_image_encode(&a,image,&recipes,&index));
        for(size_t i=0;i<sizeof tables/sizeof tables[0];++i)
            for(unsigned j=0;j<P4_GOLDEN_IMAGE_EXTRA_COUNT;++j)
                if(!strcmp(tables[i].name,image[j].name)) tables[i]=image[j];
        assert(!p4_golden_cache_store(&paths,&identity.identity,tables,sizeof tables/sizeof tables[0],key,reason,sizeof reason));
        assert(!p4_game_tables_builder_init(&b,NULL,2));
        assert(!p4_game_tables_builder_golden_cache(&b,&paths,&identity.identity,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
        assert(!p4_golden_mode_image_capture(&b,loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT,reason,sizeof reason));
        p4_golden_cache_free(loaded,P4_GOLDEN_IMAGE_EXTRA_COUNT);
        assert(!p4_game_tables_builder_seal(&b,&failed));
        assert(p4_golden_mode_image_bind(P4_GAME_MODE_GOLDEN,&b,reason,sizeof reason));
        assert(!p4_golden_mode_image_ready(&b) && p4_golden_recipes_count()==-1);
        for(unsigned i=0;i<396;++i) assert(P4_G8(0x005dd6e0u+i)==0xa5);
        retain_literals(0);
        p4_golden_mode_image_release(&b); assert(!p4_game_tables_builder_destroy(&b));
    }
    free(recipes); free(index); p4_golden_extra_free(&extra_source); p4_golden_free(&source);
    assert(!p4_os_list_dir_utf8(root,cleanup,root)); assert(!p4_os_rmdir(root));
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("golden_image: cache -> sealed owner -> guest/accessors -> rollback passed; no game boot");
    return 0;
}
