/* Actual MC async adapter + package core + sidecar codec/filesystem. Synthetic
 * retail-format bytes only: NOT gameplay, native SDL/restart or parity proof. */
#include "p4pc/os.h"
#include "p4pc/mod_save.h"
#include "p4pc/mod_save_native.h"
#include "p4pc/mods.h"
#include "p4pc/packages.h"
#include "../src/platform/memcard/mod_save_transport.h"
#include "../src/platform/memcard/mod_save_internal.h"
#include "../src/platform/memcard/mod_save_custody.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t func_004316a8(int32_t);
int32_t func_00431928(void *,void *);
int32_t func_00431cb8(int32_t,int32_t *);
int32_t func_00431f28(int32_t,const char *,void *,uint32_t,uint32_t);
int32_t func_00432288(int32_t,const char *,const void *,uint32_t,uint32_t);
int32_t func_004323a8(int32_t,const char *);
int32_t func_004326b8(int32_t,const char *);
int32_t func_00432ec0(int32_t *,int32_t *);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"save transport line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define MC_FAIL(e) ((int32_t)(0x81010000u|(unsigned)(e)))
static const char slot[]="BASLUS-21782Save0";
static unsigned char image[4100],readback[4100],icon[964];
static char card[P4_SAVE_PATH_CAP],host_slot[P4_SAVE_PATH_CAP];
static unsigned active,commits,discards,decodes,errors,warnings;
static int acknowledge;
static const P4PackageProvider *dependency;
static void word(unsigned char *p,uint32_t value)
{
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(value>>(8*i));
}
static int encode(const P4ModSaveContext *context,P4ModSaveWriter *writer,void *user)
{
    (void)user;CHECK(context->image.data&&context->image.size==sizeof image&&context->image.slot);
    if(!active)return 1;
    if(dependency&&p4_mod_save_require_provider(writer,dependency,0))return -1;
    return p4_mod_save_write_u32(writer,active);
}
static int decode(const P4ModSaveContext *context,uint32_t schema,P4ModSaveReader *reader,void **out,void *user)
{
    (void)context;(void)user;decodes++;
    if(!reader){*out=NULL;return 0;}
    if(schema!=1)return -1;
    uint32_t *value=malloc(sizeof *value);if(!value)return -1;
    *out=value;return p4_mod_save_read_u32(reader,value);
}
static void commit(void *value,void *user)
{
    (void)user;active=value?*(uint32_t *)value:0;commits++;free(value);
}
static void discard(void *value,void *user)
{
    (void)user;discards++;free(value);
}
static P4ModSaveRegistration *install(int required)
{
    P4ModSaveProvider provider={.struct_size=sizeof provider,.abi_version=P4_MOD_SAVE_ABI_VERSION,
        .owner="fixture.transport",.key="state",.schema_version=1,.flags=required?P4_MOD_SAVE_REQUIRED:0,
        .max_bytes=4,.encode=encode,.decode=decode,.commit=commit,.discard=discard};
    P4ModSaveRegistration *registration=NULL;CHECK(!p4_mod_save_register(&provider,&registration));return registration;
}
static int confirm(const char *warning)
{
    CHECK(strstr(warning,"save.dep"));warnings++;return acknowledge;
}
static void report(const char *operation,const char *reason)
{
    CHECK(operation&&reason&&*reason);errors++;
    fprintf(stderr,"native fixture refused %s: %s\n",operation,reason);
}
static int32_t finish(int32_t queued,int32_t expected)
{
    CHECK(queued>0);int32_t kind=-1,result=0;
    CHECK(func_00432ec0(&kind,&result)==1&&kind==expected);
    CHECK(func_00432ec0(&kind,&result)==-1);return result;
}
static void component_path(char out[P4_SAVE_PATH_CAP],const char *root,const char *file)
{
    int n=snprintf(out,P4_SAVE_PATH_CAP,"%s/%s",root,file);
    CHECK(n>0&&(size_t)n<P4_SAVE_PATH_CAP&&!p4_os_extended_path(out,P4_SAVE_PATH_CAP));
}
static int present(const char *file)
{
    char path[P4_SAVE_PATH_CAP];component_path(path,host_slot,file);return p4_os_is_file_nofollow(path);
}
static void virtual_path(char out[96],const char *file)
{
    int n=snprintf(out,96,"/%s/%s",slot,file);CHECK(n>0&&n<96);
}
static void create(const char *file)
{
    char name[96];virtual_path(name,file);
    int32_t result=finish(func_004323a8(0,name),7);CHECK(!result||result==MC_FAIL(EEXIST));
}
static void write_part(const char *file,const void *bytes,uint32_t size)
{
    char name[96];virtual_path(name,file);
    CHECK(finish(func_00432288(0,name,bytes,0,size),6)==(int32_t)size);
}
static void start_save(void)
{
    p4_mc_mod_save_begin(0,0,image,sizeof image);
    int32_t status[3]={0};CHECK(!finish(func_00431cb8(0,status),2));
    int32_t result=finish(func_004326b8(0,"/BASLUS-21782Save0"),11);
    CHECK(!result||result==MC_FAIL(EEXIST));
    create(slot);write_part(slot,image,sizeof image);
}
static void finish_save(void)
{
    static const unsigned char ico[]={1,2,3,4};
    create("icon.ico");write_part("icon.ico",ico,sizeof ico);
    create("icon.sys");write_part("icon.sys",icon,sizeof icon);
}
static void assert_raw(void)
{
    char path[P4_SAVE_PATH_CAP];component_path(path,host_slot,slot);
    FILE *file=p4_os_fopen_rb(path);CHECK(file);
    unsigned char bytes[sizeof image];CHECK(fread(bytes,1,sizeof bytes,file)==sizeof bytes&&fgetc(file)==EOF);
    CHECK(!fclose(file)&&!memcmp(bytes,image,sizeof bytes));
}
static int32_t read_full(void)
{
    p4_mc_mod_load_begin();
    char name[96];virtual_path(name,slot);memset(readback,0,sizeof readback);
    return finish(func_00431f28(0,name,readback,0,sizeof readback),5);
}
static void restored(void)
{
    char error[1024];CHECK(!p4_mod_save_transport_restore(0,readback+4,sizeof readback-4,error,sizeof error));
    p4_mod_save_transport_restore_commit();
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    p4_os_init();CHECK(!p4_os_setenv("P4PC_SAVE_DIR",argv[2],1));
    CHECK(!func_004316a8(0));int32_t socket_params[]={0,2,0};CHECK(func_00431928(socket_params,NULL)==0);
    component_path(card,argv[2],"mc0");component_path(host_slot,card,slot);
    p4_mod_save_transport_ui(confirm,report);
    word(image+56,0x2000);word(image+60,1);word(image+65,UINT32_MAX);memcpy(icon,"PS2D",4);
    const char *mode=argv[1];char error[1024];
    if(!strcmp(mode,"plain")){
        start_save();finish_save();assert_raw();CHECK(!present(P4_MOD_SAVE_SIDECAR)&&!present(P4_MOD_SAVE_COMMIT)&&!present(P4_SAVE_MODE_MARKER));
    }else if(!strcmp(mode,"roundtrip")){
        P4ModSaveRegistration *registration=install(1);active=17;
        start_save();CHECK(!p4_os_is_dir_nofollow(host_slot));
        char busy_name[96];unsigned char summary[56]={0};virtual_path(busy_name,slot);
        CHECK(func_00431f28(0,busy_name,summary,0,sizeof summary)==(int32_t)0x81018010u&&!summary[0]);
        finish_save();assert_raw();
        CHECK(present(P4_MOD_SAVE_SIDECAR)&&present(P4_MOD_SAVE_COMMIT));active=99;
        CHECK(read_full()==(int32_t)sizeof readback&&active==99&&!commits&&decodes==1);
        readback[100]^=1;CHECK(p4_mod_save_transport_restore(0,readback+4,sizeof readback-4,error,sizeof error)<0);
        p4_mod_save_transport_load_abort();CHECK(active==99&&discards==1);
        CHECK(read_full()==(int32_t)sizeof readback);restored();CHECK(active==17&&commits==1);
        CHECK(!p4_mod_save_transport_reset(error,sizeof error));CHECK(active==17);p4_mod_save_transport_reset_commit();CHECK(!active);
        CHECK(!p4_mod_save_unregister(registration));image[100]=7;
        start_save();finish_save();assert_raw();CHECK(!present(P4_MOD_SAVE_SIDECAR)&&!present(P4_MOD_SAVE_COMMIT));
    }else if(!strcmp(mode,"required-write")||!strcmp(mode,"optional-write")){
        int required=!strcmp(mode,"required-write");P4ModSaveRegistration *registration=install(required);active=17;
        if(!required){CHECK(!p4_mods_stage(argv[3],"save.dep")&&!p4_package_registry_publish());dependency=p4_packages_provider(0);CHECK(!p4_mod_save_touch(dependency));}
        start_save();finish_save();assert_raw();CHECK(present(P4_MOD_SAVE_SIDECAR)&&!present(P4_SAVE_MODE_MARKER));
        CHECK(!p4_mod_save_unregister(registration));p4_package_registry_retire();
    }else if(!strcmp(mode,"history-reset")){
        P4ModSaveRegistration *registration=install(0);active=17;
        CHECK(!p4_mods_stage(argv[3],"save.dep")&&!p4_package_registry_publish());
        dependency=p4_packages_provider(0);CHECK(!p4_mod_save_touch(dependency));
        start_save();finish_save();
        CHECK(!p4_mod_save_transport_reset(error,sizeof error));p4_mod_save_transport_reset_commit();CHECK(!active);
        image[100]=4;start_save();finish_save();assert_raw();
        CHECK(present(P4_MOD_SAVE_SIDECAR)&&!present(P4_SAVE_MODE_MARKER));
        CHECK(!p4_mod_save_unregister(registration));p4_package_registry_retire();
    }else if(!strcmp(mode,"cancel-intent")){
        start_save();finish_save();P4ModSaveRegistration *registration=install(1);
        active=17;image[100]=9;start_save();
        CHECK(read_full()==(int32_t)sizeof readback&&readback[100]==0&&active==17);
        restored();CHECK(!active&&!present(P4_MOD_SAVE_SIDECAR));
        image[100]=0;assert_raw();CHECK(!p4_mod_save_unregister(registration));
    }else if(!strcmp(mode,"required-read")||!strcmp(mode,"broken-read")||!strcmp(mode,"witness-refuse")){
        CHECK(read_full()<0&&errors==1&&!commits&&!decodes&&!warnings);
    }else if(!strcmp(mode,"optional-decline")||!strcmp(mode,"optional-continue")){
        acknowledge=!strcmp(mode,"optional-continue");int32_t result=read_full();CHECK(warnings==1);
        if(!acknowledge)CHECK(result<0&&errors==1);
        else{CHECK(result==(int32_t)sizeof readback);restored();start_save();finish_save();assert_raw();CHECK(present(P4_MOD_SAVE_SIDECAR)&&!present(P4_SAVE_MODE_MARKER));}
    }else if(!strcmp(mode,"witness")){
        start_save();finish_save();assert_raw();CHECK(present(P4_MOD_SAVE_SIDECAR)&&present(P4_SAVE_MODE_MARKER));
        CHECK(read_full()==(int32_t)sizeof readback);restored();CHECK(!p4_mod_save_transport_reset(error,sizeof error));p4_mod_save_transport_reset_commit();
        start_save();finish_save();CHECK(present(P4_SAVE_MODE_MARKER));
    }else if(!strcmp(mode,"crash-pending")){
        start_save();finish_save();P4ModSaveRegistration *registration=install(1);(void)registration;
        active=17;image[100]=9;start_save();CHECK(!present(P4_MOD_SAVE_SIDECAR));
        /* Genuine process death with a LIVE owned stage/lock/journal, before
         * publication. No mocked filesystem or test-only production hook. */
        fflush(NULL);_Exit(23);
    }else if(!strcmp(mode,"recovered")){
        assert_raw();CHECK(!present(P4_MOD_SAVE_SIDECAR)&&!present(P4_MOD_SAVE_COMMIT));
    }else if(!strcmp(mode,"custody-file")||!strcmp(mode,"custody-directory")){
        CHECK(p4_save_witness_input(argv[3],!strcmp(mode,"custody-directory"),error,sizeof error)==1);
    }else if(!strcmp(mode,"custody-clean")){
        CHECK(p4_save_witness_input(argv[3],0,error,sizeof error)==0);
    }else if(!strcmp(mode,"custody-bad")){
        CHECK(p4_save_witness_input(argv[3],0,error,sizeof error)<0);
    }else return 2;
    p4_mod_save_transport_abort();p4_mod_save_transport_load_abort();
    puts("native save transport: synthetic filesystem/codec complete; NOT gameplay/SDL/restart/parity/reference proof");return 0;
}
