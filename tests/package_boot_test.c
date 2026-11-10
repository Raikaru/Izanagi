/* Real package registry/file adapters; orchestration-only C products. This is
 * NOT a renderer/RW/GPU/table-gameplay backend or runtime readiness proof. */
#include "p4pc/packages.h"
#include "package_boot_internal.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#define REQUIRE(x) do { if(!(x)){fprintf(stderr,"boot fixture:%d: %s\n",__LINE__,#x);exit(90);} } while(0)
static const char *mode;
static char trace[4096];
static unsigned cpu_published,runtime_published,rollbacks,refusals;
static _Atomic unsigned reads;
static int cpu_alive,cpu_ui_ready,cpu_finalized,runtime_ui_ready,runtime_finalized;
static const P4PackageProvider *origin,*held[2];
static size_t held_count;
static unsigned old_generation[2];
static int cpu_tables=101,runtime_tables=1001;
static struct CpuResource { const int *tables;unsigned char decoded[8]; } cpu_resources;
static struct RuntimeResource { const int *tables;const struct CpuResource *input; } runtime_resources;
static void event(const char *text)
{
    REQUIRE(strlen(trace)+strlen(text)+2<sizeof trace);strcat(trace,text);strcat(trace,"|");
}
static void observe(const P4PackageFileOrigin *source,const void *bytes,size_t size,uint64_t offset,void *user)
{
    (void)bytes;(void)offset;(void)user;
    if(!strcmp(source->relative,"image.bin"))reads+=(unsigned)size;
}
static int client_preflight(const P4PackageRegistry *registry,void *user)
{
    (void)registry;(void)user;event("client.preflight");return 0;
}
static int refuse(const char *id,const char *reason,void *user)
{
    (void)user;REQUIRE(!strcmp(mode,"renew"));REQUIRE(!strcmp(id,"boot.a"));
    REQUIRE(strstr(reason,"CPU image"));++refusals;return 0;
}
static int schema_prepare(void *user)
{
    (void)user;event("cpu.schema");REQUIRE(cpu_published==0);
    return p4_package_boot_provide(NULL); /* No matching field/default allocation. */
}
static void schema_undo(void *user){(void)user;event("undo.schema");++rollbacks;}
static int tables_prepare(void *user)
{
    (void)user;event("cpu.tables");REQUIRE(cpu_published==0);
    const void *input=(const void *)1;
    REQUIRE(!p4_package_boot_product("schema-input",&input)&&input==NULL);
    origin=p4_packages_provider(0);
    if(!origin)return p4_package_boot_provide(NULL);
    REQUIRE(p4_package_provider_selected(origin));
    if(!held_count){
        for(size_t i=0;i<p4_packages_count()&&i<2;i++){
            held[i]=p4_packages_provider(i);REQUIRE(!p4_package_provider_retain(held[i]));
            old_generation[i]=p4_package_provider_info(held[i])->generation;++held_count;
        }
    }else if(!strcmp(mode,"renew")){
        REQUIRE(!p4_package_provider_selected(held[0])&&!p4_package_provider_selected(held[1]));
        REQUIRE(p4_package_provider_same_source(origin,held[1]));
        for(size_t i=0;i<held_count;i++)REQUIRE(p4_package_provider_info(held[i])->generation==old_generation[i]);
    }
    cpu_alive=1;
    REQUIRE(!p4_package_boot_provide(&cpu_tables));
    if(!strcmp(mode,"sticky"))(void)p4_package_boot_provide(&cpu_tables);
    return 0;
}
static void tables_undo(void *user){(void)user;event("undo.tables");cpu_alive=0;++rollbacks;}
static int resources_prepare(void *user)
{
    (void)user;event("cpu.resources");REQUIRE(cpu_published==0);
    const void *input=NULL;REQUIRE(!p4_package_boot_product("tables",&input));
    if(!input)return p4_package_boot_provide(NULL);
    REQUIRE(input==&cpu_tables&&cpu_alive);
    for(size_t i=0;i<p4_packages_count();i++){
        const P4PackageProvider *provider=p4_packages_provider(i);
        P4PackageFile *file=NULL;
        REQUIRE(!p4_package_file_open(provider,"image.bin",16,&file));
        REQUIRE(p4_package_file_size(file)==sizeof cpu_resources.decoded);
        REQUIRE(!p4_package_file_read_at(file,0,cpu_resources.decoded,sizeof cpu_resources.decoded));
        REQUIRE(!p4_package_file_close(file));
        if(memcmp(cpu_resources.decoded,"P4BOOT1\n",sizeof cpu_resources.decoded)){
            REQUIRE(!p4_package_provider_refuse(provider,"CPU image validation failed"));return -1;
        }
    }
    cpu_resources.tables=input;return p4_package_boot_provide(&cpu_resources);
}
static void resources_undo(void *user){(void)user;event("undo.resources");++rollbacks;}
static int ui_prepare(void *user)
{
    (void)user;event("cpu.ui");REQUIRE(cpu_published==0);
    const void *tables=NULL,*resources=NULL;
    REQUIRE(!p4_package_boot_product("tables",&tables));
    REQUIRE(!p4_package_boot_product("resources",&resources));
    REQUIRE((tables==NULL)==(resources==NULL));
    if(tables)REQUIRE(tables==&cpu_tables&&resources==&cpu_resources);
    cpu_ui_ready=1;
    if(!strcmp(mode,"early-fail"))return -1;
    return p4_package_boot_provide(resources);
}
static void ui_undo(void *user){(void)user;event("undo.ui");cpu_ui_ready=cpu_finalized=0;++rollbacks;}
static int cpu_finalize(void *user)
{
    (void)user;event("cpu.finalize");REQUIRE(cpu_ui_ready&&!cpu_published);
    cpu_finalized=1;return !strcmp(mode,"early-finalize-fail")?-1:0;
}
static void publish_tables(void *user){(void)user;REQUIRE(cpu_ui_ready&&cpu_finalized);event("pub.tables");++cpu_published;}
static void publish_resources(void *user){(void)user;event("pub.resources");++cpu_published;}
static void publish_ui(void *user){(void)user;event("pub.ui");++cpu_published;}
static int tables_runtime(void *user)
{
    (void)user;event("rt.tables");REQUIRE(cpu_published==3&&!runtime_published);
    if(!strcmp(mode,"cross-pass")){
        const void *input=NULL;(void)p4_package_boot_product("schema-input",&input);return 0;
    }
    return p4_package_boot_provide(origin?&runtime_tables:NULL);
}
static int resources_runtime(void *user)
{
    (void)user;event("rt.resources");REQUIRE(!runtime_published);
    const void *input=NULL;REQUIRE(!p4_package_boot_product("tables",&input));
    if(!input)return p4_package_boot_provide(NULL);
    REQUIRE(input==&runtime_tables&&input!=&cpu_tables&&cpu_alive);
    REQUIRE(!memcmp(cpu_resources.decoded,"P4BOOT1\n",sizeof cpu_resources.decoded));
    runtime_resources.tables=input;runtime_resources.input=&cpu_resources;
    return p4_package_boot_provide(&runtime_resources);
}
static int ui_runtime(void *user)
{
    (void)user;event("rt.ui");REQUIRE(!runtime_published);
    const void *tables=NULL,*resources=NULL;
    REQUIRE(!p4_package_boot_product("tables",&tables));
    REQUIRE(!p4_package_boot_product("resources",&resources));
    REQUIRE((tables==NULL)==(resources==NULL));
    if(tables)REQUIRE(tables==&runtime_tables&&resources==&runtime_resources);
    runtime_ui_ready=1;
    if(!strcmp(mode,"late-fail"))return p4_package_boot_fail(origin,"Actual late fixture failure");
    return p4_package_boot_provide(resources);
}
static int runtime_finalize(void *user)
{
    (void)user;event("rt.finalize");REQUIRE(runtime_ui_ready&&!runtime_published);
    if(!strcmp(mode,"late-finalize-fail"))return p4_package_boot_fail(origin,"Actual late fixture failure");
    runtime_finalized=1;return 0;
}
static void runtime_publish_tables(void *user){(void)user;REQUIRE(runtime_ui_ready&&runtime_finalized);event("rtpub.tables");++runtime_published;}
static void runtime_publish_resources(void *user){(void)user;event("rtpub.resources");++runtime_published;}
static void runtime_publish_ui(void *user){(void)user;event("rtpub.ui");++runtime_published;}
static const char *const sections[]={"fixture-schema",NULL};
static const char *const source_deps[]={"schema-input",NULL};
static const char *const resource_deps[]={"tables",NULL};
static const char *const ui_deps[]={"tables","resources",NULL};
int main(int argc,char **argv)
{
    REQUIRE(argc==4);mode=argv[1];
    P4PackageHandler schema={.name="schema-input",.sections=sections,.prepare=schema_prepare,.rollback=schema_undo};
    P4PackageHandler tables={.name="tables",.prepare=tables_prepare,.publish=publish_tables,.rollback=tables_undo,
        .boot_phase=P4_PACKAGE_BOOT_TABLES,.dependencies=source_deps,.runtime_prepare=tables_runtime,.runtime_publish=runtime_publish_tables};
    P4PackageHandler resources={.name="resources",.prepare=resources_prepare,.publish=publish_resources,.rollback=resources_undo,
        .boot_phase=P4_PACKAGE_BOOT_RESOURCES,.dependencies=resource_deps,.runtime_prepare=resources_runtime,.runtime_publish=runtime_publish_resources};
    P4PackageHandler ui={.name="ui",.prepare=ui_prepare,.publish=publish_ui,.rollback=ui_undo,
        .boot_phase=P4_PACKAGE_BOOT_UI,.dependencies=ui_deps,.finalize_prepare=cpu_finalize,
        .runtime_prepare=ui_runtime,.runtime_finalize_prepare=runtime_finalize,.runtime_publish=runtime_publish_ui};
    const char *const absent[]={"missing",NULL};const char *const cycle[]={"ui",NULL};
    const char *const backwards[]={"resources",NULL};
    if(!strcmp(mode,"missing"))tables.dependencies=absent;
    if(!strcmp(mode,"backwards"))tables.dependencies=backwards;
    if(!strcmp(mode,"cycle")){resources.boot_phase=ui.boot_phase=P4_PACKAGE_BOOT_TABLES;resources.dependencies=cycle;}
    REQUIRE(!p4_package_handler_register(&ui)&&!p4_package_handler_register(&resources));
    REQUIRE(!p4_package_handler_register(&schema)&&!p4_package_handler_register(&tables));
    REQUIRE(!p4_package_handler_register(&tables));
    REQUIRE(p4_package_boot_provide(NULL)==-1);
    const void *outside=(void *)1;REQUIRE(p4_package_boot_product("tables",&outside)==-1&&outside==NULL);
    REQUIRE(!p4_package_reads_observe(observe,NULL));
    P4PackageClient client={.diagnostic_name="boot unit",.manifest_name="mod.ini",.section="[mod]",
        .kind=P4_PACKAGE_MOD,.mount_files=1,.preflight=client_preflight};
    if(!strcmp(mode,"renew"))client.refuse=refuse;
    P4PackageRegistry *registry=NULL;REQUIRE(!p4_package_registry_load(&registry,&client,argv[2],argv[3]));
    REQUIRE(vfs_open("DATA/TEST.TXT",0)<0);
    int result=p4_package_registry_publish();
    if (result) fprintf(stderr, "boot fixture admission trace: %s\n", trace);
    REQUIRE(!vfs_mount(NULL,NULL));
    int early_failure=!strcmp(mode,"missing")||!strcmp(mode,"cycle")||!strcmp(mode,"backwards")||
        !strcmp(mode,"sticky")||!strcmp(mode,"early-fail")||!strcmp(mode,"early-finalize-fail");
    if(early_failure){
        REQUIRE(result<0&&!cpu_published&&!cpu_alive);REQUIRE(vfs_open("DATA/TEST.TXT",0)<0);
        for(size_t i=0;i<held_count;i++)REQUIRE(!p4_package_provider_selected(held[i]));
        if(!strcmp(mode,"sticky"))REQUIRE(rollbacks==2);
        if(strstr(mode,"early-"))REQUIRE(rollbacks==4);
        printf("early-refused trace=%s\n",trace);
    }else{
        REQUIRE(!result&&cpu_published==3);
        int fd=vfs_open("DATA/TEST.TXT",0);
        if(!strcmp(mode,"empty"))REQUIRE(fd<0&&!cpu_alive&&reads==0);
        else{REQUIRE(fd>=0&&p4_package_provider_selected(origin));vfs_close(fd);}
        if(!strcmp(mode,"renew"))REQUIRE(refusals==1&&p4_packages_count()==1);
        unsigned before=reads,undo_before=rollbacks;
        const P4PackageProvider *failed=NULL;const char *reason=NULL;
        result=p4_package_registry_runtime_materialize(&failed,&reason);
        if(!strcmp(mode,"late-fail")||!strcmp(mode,"late-finalize-fail")||!strcmp(mode,"cross-pass")){
            REQUIRE(result<0&&!runtime_published&&cpu_alive&&rollbacks==undo_before&&reads==before);
            REQUIRE(p4_package_provider_selected(origin)&&reason&&reason[0]);
            if(strcmp(mode,"cross-pass"))REQUIRE(failed==origin&&!strcmp(reason,"Actual late fixture failure"));
            REQUIRE(p4_package_registry_runtime_materialize(&failed,&reason)<0&&reads==before);
            printf("late-retained trace=%s\n",trace);
            return 0; /* Intentional process-owned retention, like the error UX. */
        }
        REQUIRE(!result&&runtime_published==3&&runtime_finalized&&reads==before);
        REQUIRE(p4_package_registry_runtime_materialize(&failed,&reason)==1&&runtime_published==3);
        printf("boot-success trace=%s\n",trace);
    }
    REQUIRE(!p4_package_reads_unobserve(observe,NULL));
    for(size_t i=0;i<held_count;i++)p4_package_provider_release(held[i]);
    p4_package_registry_retire();return 0;
}
