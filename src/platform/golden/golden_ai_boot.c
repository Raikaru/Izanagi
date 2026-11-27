#include "golden_ai_native.h"
#include "golden_ai_rng_native.h"
#include "p4pc/gmem.h"
#include "p4pc/game_table_sources.h"
#include <string.h>
static const P4GameTablesBootProduct *cpu;
static int registered;
static int complete_prepare(void *unused)
{
    (void)unused;
    if(!cpu || !cpu->golden_ai || !p4_game_tables_revision(cpu->tables) ||
        !p4_game_table_sources_is_sealed(cpu->sources) ||
        p4_game_table_sources_validate_view(cpu->sources,cpu->tables)) return P4_TABLE_NOT_READY;
    const P4GoldenAiProduct *product=cpu->golden_ai;
    P4GoldenAiRng *additive; P4GoldenSfmt *sfmt;
    if(p4_golden_ai_native_streams(&additive,&sfmt) ||
        !p4_golden_vm_native_quiescent() || P4_GPDATA(uint32_t,0xb3ac)) return P4_TABLE_NOT_READY;
    for(unsigned i=0;i<2;++i) {
        uint32_t guest=P4_GPDATA(uint32_t,i?0xb42c:0xb428);
        size_t size=product->programs[9+i].size;
        if(!guest || guest>=P4_GUEST_SIZE || size>P4_GUEST_SIZE-guest ||
            memcmp(P4_G2H(guest),product->programs[9+i].bytes,size)) return P4_TABLE_CONFLICT;
    }
    return p4_golden_vm_native_prepare()?P4_TABLE_CONFLICT:P4_TABLE_OK;
}
static void complete_commit(void *unused)
{
    (void)unused;
    p4_golden_ai_native_activate(cpu->golden_ai);
}
static int prepare(void *unused)
{
    (void)unused;
    const void *provided=NULL;
    if(cpu || p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER,&provided)) return -1;
    const P4GameTablesBootProduct *root=provided;
    if(!root || !root->golden_ai) return p4_package_boot_provide(NULL);
    if(!root->tables || !root->sources || !root->native_uses ||
        !root->golden_ai->cache_origin || !p4_game_tables_revision(root->tables)) return -1;
    for(unsigned i=0;i<2;++i)
        if(root->golden_ai->scripts[i].local_integers>800 || root->golden_ai->scripts[i].local_floats>8) return -1;
    cpu=root;
    if(p4_golden_ai_knowledge_register(root)) return -1;
    return p4_package_boot_provide(root->golden_ai);
}
static void publish(void *unused) { (void)unused; }
static int runtime_prepare(void *unused)
{
    (void)unused;
    const void *provided=NULL;
    if(p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER,&provided)) return -1;
    const P4GameTablesBootProduct *root=provided;
    if(!cpu) return root && root->golden_ai?-1:p4_package_boot_provide(NULL);
    if(!root || root->golden_ai!=cpu->golden_ai || root->tables!=cpu->tables ||
        root->sources!=cpu->sources || root->native_uses!=cpu->native_uses ||
        p4_golden_ai_native_stage(cpu->golden_ai)) return -1;
    /* Golden owns the existing AI participant only in Golden mode. The PS2
     * provider explicitly stays inactive for this distinct raw cache product. */
    int result=p4_game_tables_battle_complete_register("element-ai",complete_prepare,complete_commit,NULL);
    if(!result) { registered=1; result=p4_game_tables_battle_complete_require("element-ai","table-source"); }
    if(result) return -1;
    return p4_package_boot_provide(root->golden_ai);
}
static void rollback(void *unused)
{
    (void)unused;
    if(p4_golden_ai_knowledge_unregister())
        p4_package_boot_runtime_failed("Golden AI knowledge save owner is still in use");
    if(p4_golden_ai_native_session_end()) p4_package_boot_runtime_failed("Golden AI rollback has live native owners");
    if(registered && p4_game_tables_battle_complete_unregister("element-ai",complete_prepare,complete_commit,NULL))
        p4_package_boot_runtime_failed("Golden AI rollback cannot unregister its completion owner");
    registered=0; cpu=NULL;
}
static const char *const dependencies[]={P4_GAME_TABLES_BOOT_PROVIDER,NULL};
const P4PackageHandler p4_golden_ai_boot_provider={
    .name="game-tables.golden-ai",.prepare=prepare,.publish=publish,.rollback=rollback,
    .boot_phase=P4_PACKAGE_BOOT_TABLES,.dependencies=dependencies,.runtime_prepare=runtime_prepare
};
int p4_golden_ai_boot_forget(const P4GoldenAiProduct *product)
{
    if(cpu && cpu->golden_ai==product) {
        if(p4_golden_ai_knowledge_unregister()) return -1;
        cpu=NULL; registered=0;
    }
    return 0;
}
