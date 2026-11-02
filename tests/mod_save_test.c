/* Synthetic retail-format bytes and real package/sidecar implementations.
 * This is not a gameplay/parity/reference receipt. */
#include "p4pc/mod_save.h"
#include "p4pc/mods.h"
#include "p4pc/packages.h"
#include "../src/platform/memcard/mod_save_internal.h"
#include "../src/platform/memcard/mod_save_hash.h"
#include "../src/platform/memcard/save_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"mod-save fixture line%d: %s\n",__LINE__,#x);return 1;}}while(0)
typedef struct Fixture {
    uint32_t active;
    int commits,discards,decoded,fail_decode,fail_validate,new_game,omit_after_write,use_string;
    int ignore_warning_failure,check_encode_warning;
    const char *decode_warning,*validate_warning;
    const char *owner;
    const P4PackageProvider *dependency;
} Fixture;
static int encode(const P4ModSaveContext *context,P4ModSaveWriter *writer,void *user)
{
    (void)context;
    Fixture *f=user;
    if(f->check_encode_warning&&p4_mod_save_warn(context,"Not a load")==0)return -1;
    if(!f->active)return 1;
    if(f->dependency){
        const P4PackageInfo *info=p4_package_provider_info(f->dependency);
        int result=f->use_string?p4_mod_save_require_mod(writer,info->id,NULL,1):p4_mod_save_require_provider(writer,f->dependency,1);
        if(result)return -1;
    }
    int result=p4_mod_save_write_u32(writer,f->active);
    return result?result:f->omit_after_write?1:0;
}
static int decode(const P4ModSaveContext *context,uint32_t schema,P4ModSaveReader *reader,void **staged,void *user)
{
    Fixture *f=user;
    f->decoded++;
    if(f->decode_warning&&p4_mod_save_warn(context,f->decode_warning)&&!f->ignore_warning_failure)return -1;
    if(!reader){if(!context->image.data&&!context->image.size)f->new_game++;*staged=NULL;return 0;}
    if(schema!=1)return -1;
    uint32_t *value=malloc(sizeof *value);
    if(!value)return -1;
    *staged=value;
    if(p4_mod_save_read_u32(reader,value))return -1;
    if(f->fail_decode){snprintf(context->error,context->error_capacity,"authored decoder refusal");return -7;}
    return 0;
}
static int validate(const P4ModSaveContext *context,void *staged,void *user)
{
    Fixture *f=user;
    if(f->validate_warning&&p4_mod_save_warn(context,f->validate_warning)&&!f->ignore_warning_failure)return -1;
    const void *own=NULL;
    if(p4_mod_save_staged(context,f->owner?f->owner:"fixture.save","state",&own)||own!=staged)return -1;
    return f->fail_validate?-9:0;
}
static void commit(void *staged,void *user)
{
    Fixture *f=user;
    f->active=staged?*(uint32_t *)staged:0;f->commits++;free(staged);
}
static void discard(void *staged,void *user)
{
    Fixture *f=user;f->discards++;free(staged);
}
static P4ModSaveProvider provider(Fixture *f)
{
    P4ModSaveProvider p={.struct_size=sizeof p,.abi_version=P4_MOD_SAVE_ABI_VERSION,
        .owner=f->owner?f->owner:"fixture.save",.key="state",.schema_version=1,.flags=P4_MOD_SAVE_REQUIRED,
        .max_bytes=4,.user=f,.encode=encode,.decode=decode,.validate=validate,.commit=commit,.discard=discard};
    return p;
}
static unsigned char *copy_sidecar(const P4ModSaveCapture *capture)
{
    unsigned char *copy=malloc(capture->sidecar_size);
    if(copy)memcpy(copy,capture->sidecar,capture->sidecar_size);
    return copy;
}
static void word(unsigned char *p,uint32_t n)
{
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));
}
static int digest_matches(const unsigned char actual[32],const char *expected)
{
    static const char hex[]="0123456789abcdef";
    for(unsigned i=0;i<32;i++)if(expected[2*i]!=hex[actual[i]>>4]||expected[2*i+1]!=hex[actual[i]&15u])return 0;
    return 1;
}
static int hash_cases(void)
{
    unsigned char hash[32];
    p4_save_hash(NULL,0,hash);
    CHECK(digest_matches(hash,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    p4_save_hash("abc",3,hash);
    CHECK(digest_matches(hash,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    const char *long_message="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    P4SaveHash state;p4_save_hash_init(&state);
    size_t length=strlen(long_message);
    for(size_t i=0;i<length;i++){p4_save_hash_update(&state,long_message+i,1);p4_save_hash_update(&state,NULL,0);}
    p4_save_hash_final(&state,hash);
    CHECK(digest_matches(hash,"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    unsigned char a[1000];memset(a,'a',sizeof a);p4_save_hash_init(&state);
    for(unsigned i=0;i<1000;i++)p4_save_hash_update(&state,a,sizeof a);
    p4_save_hash_final(&state,hash);
    CHECK(digest_matches(hash,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    return 0;
}
static int warning_cases(const P4ModSaveImage *saved)
{
    char error[1024]={0};
    char decoded[]="Native DEV content identity differs; optional history only.";
    Fixture f={.active=7,.owner="fixture.warn",.check_encode_warning=1,
        .decode_warning=decoded,.validate_warning="All required references were checked separately."};
    P4ModSaveProvider p=provider(&f);p.flags=0;
    P4ModSaveRegistration *registration=NULL;
    CHECK(!p4_mod_save_register(&p,&registration)&&p4_mod_save_needs_codec());
    P4ModSaveCapture *capture=NULL;
    CHECK(!p4_mod_save_capture_image(saved,&capture,error,sizeof error)&&capture&&!capture->witness);
    f.active=55;
    P4ModSaveLoad *load=NULL;
    CHECK(!p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)&&load);
    CHECK(f.active==55&&!f.commits);
    const char *warnings=p4_mod_save_load_warning(load);
    CHECK(warnings&&strstr(warnings,"Native DEV")&&strstr(warnings,"required references"));
    decoded[0]='X';CHECK(strstr(warnings,"Native DEV")); /* Copied, not borrowed. */
    P4ModSaveContext wrong_phase={.image=*saved,.load=load};
    CHECK(p4_mod_save_warn(&wrong_phase,"Not a callback")<0&&p4_mod_save_warn(NULL,"No context")<0);
    p4_mod_save_load_abort(load);load=NULL;
    char over_limit[4098];memset(over_limit,'x',sizeof over_limit-1);over_limit[sizeof over_limit-1]=0;
    f.decode_warning=over_limit;f.ignore_warning_failure=1;
    CHECK(p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==55&&!f.commits&&strstr(error,"warning"));
    f.decode_warning=NULL;f.validate_warning=over_limit;
    CHECK(p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==55&&!f.commits&&strstr(error,"warning"));
    f.decode_warning="";f.validate_warning=NULL;
    CHECK(p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    P4ModSaveImage reset={0};f.decode_warning="Not a real load";
    CHECK(p4_mod_save_stage_image(&reset,NULL,0,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==55&&!f.commits);
    f.decode_warning=NULL;
    CHECK(!p4_mod_save_stage_image(&reset,NULL,0,&load,error,sizeof error)&&load&&!p4_mod_save_load_warning(load));
    p4_mod_save_load_abort(load);load=NULL;
    CHECK(!p4_mod_save_unregister(registration));
    CHECK(!p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)&&load);
    warnings=p4_mod_save_load_warning(load);
    CHECK(warnings&&strstr(warnings,"fixture.warn:state")&&strstr(warnings,"preserved"));
    p4_mod_save_load_abort(load);p4_mod_save_capture_free(capture);
    CHECK(!p4_mod_save_needs_codec());
    return 0;
}
static int history_warning_cases(const char *root,const P4ModSaveImage *saved)
{
    char error[1024]={0};
    CHECK(!p4_mods_stage(root,"save.dep,save.other")&&!p4_package_registry_publish());
    CHECK(p4_packages_count()==2);
    for(size_t i=0;i<p4_packages_count();i++)CHECK(!p4_mod_save_touch(p4_packages_provider(i)));
    P4ModSaveCapture *capture=NULL;
    CHECK(!p4_mod_save_capture_image(saved,&capture,error,sizeof error)&&capture);
    p4_package_registry_retire();
    P4ModSaveLoad *load=NULL;
    CHECK(!p4_mod_save_stage_image(saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)&&load);
    const char *warnings=p4_mod_save_load_warning(load);
    CHECK(warnings&&strstr(warnings,"save.dep")&&strstr(warnings,"1.2.3")&&strstr(warnings,"save.other")&&strstr(warnings,"9.8.7"));
    p4_mod_save_load_abort(load);p4_mod_save_capture_free(capture);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2&&argc!=3)return 2;
    CHECK(!hash_cases());
    unsigned char image[4100]={0},original[sizeof image];
    word(image+56,0x2000);word(image+60,1);word(image+65,UINT32_MAX);memcpy(original,image,sizeof image);
    char error[1024]={0};P4ModSaveImage saved={.data=image,.size=sizeof image,.slot="fixture"};
    CHECK(!p4save_image_validate(image,sizeof image,error,sizeof error));
    if(argc==3){
        CHECK(!strcmp(argv[2],"history"));
        CHECK(!history_warning_cases(argv[1],&saved));
        CHECK(!p4_packages_memory_usage().resident&&!p4_packages_memory_usage().reserved);
        puts("native sidecar history: synthetic fixture complete");
        return 0;
    }
    P4ModSaveCapture *empty=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&empty,error,sizeof error)&&!empty);
    CHECK(!warning_cases(&saved));
    Fixture f={0};P4ModSaveProvider p=provider(&f);P4ModSaveRegistration *registration=NULL;
    CHECK(!p4_mod_save_register(&p,&registration));
    P4ModSaveRegistration *duplicate=NULL;
    CHECK(p4_mod_save_register(&p,&duplicate)<0&&!duplicate);
    CHECK(!p4_mod_save_capture_image(&saved,&empty,error,sizeof error)&&!empty);
    CHECK(!memcmp(image,original,sizeof image));
    f.active=UINT32_MAX;
    P4ModSaveCapture *capture=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&capture,error,sizeof error)&&capture);
    unsigned char marker[P4_MOD_SAVE_MARKER_BYTES];
    p4_save_marker_make(capture->retail_size,capture->retail_hash,capture->sidecar_size,capture->sidecar_hash,marker);
    CHECK(!p4_save_marker_verify(capture->retail_size,capture->retail_hash,capture->sidecar_size,capture->sidecar_hash,marker,sizeof marker,error,sizeof error));
    marker[40]^=1;
    CHECK(p4_save_marker_verify(capture->retail_size,capture->retail_hash,capture->sidecar_size,capture->sidecar_hash,marker,sizeof marker,error,sizeof error)<0);
    CHECK(!memcmp(image,original,sizeof image));
    f.active=23;
    P4ModSaveLoad *load=NULL;
    CHECK(!p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)&&load);
    CHECK(f.active==23&&f.commits==0);
    CHECK(p4_mod_save_unregister(registration)<0);
    CHECK(p4_mod_save_load_matches(load,0,image+4,sizeof image-4));
    CHECK(!p4_mod_save_load_matches(load,1,image+4,sizeof image-4));
    p4_mod_save_load_abort(load);load=NULL;
    CHECK(f.active==23&&f.commits==0&&f.discards==1);
    f.fail_decode=1;
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==23&&f.discards==2&&strstr(error,"authored decoder refusal"));
    f.fail_decode=0;f.fail_validate=1;
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==23&&f.commits==0&&f.discards==3);
    f.fail_validate=0;
    Fixture sibling={.active=99,.owner="zz.fixture",.fail_validate=1};
    P4ModSaveProvider sibling_provider=provider(&sibling);
    P4ModSaveRegistration *sibling_registration=NULL;
    CHECK(!p4_mod_save_register(&sibling_provider,&sibling_registration));
    P4ModSaveCapture *two=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&two,error,sizeof error)&&two);
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(two),two->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==23&&f.commits==0&&f.discards==4&&sibling.active==99&&!sibling.commits&&sibling.discards==1);
    CHECK(!p4_mod_save_unregister(sibling_registration));
    p4_mod_save_capture_free(two);
    CHECK(!p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error));
    CHECK(!p4_mod_save_load_warning(load));p4_mod_save_load_commit(load);load=NULL;
    CHECK(f.active==UINT32_MAX&&f.commits==1);
    unsigned char *witness=copy_sidecar(capture);CHECK(witness);word(witness+68,P4_MOD_SAVE_WITNESS);
    p4_save_hash(witness,capture->sidecar_size-32,witness+capture->sidecar_size-32);
    CHECK(!p4_mod_save_stage_image(&saved,witness,capture->sidecar_size,&load,error,sizeof error)&&p4_mod_save_load_witness(load));
    p4_mod_save_load_commit(load);load=NULL;
    P4ModSaveCapture *inherited_witness=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&inherited_witness,error,sizeof error)&&inherited_witness&&inherited_witness->witness);
    p4_mod_save_capture_free(inherited_witness);
    unsigned char *corrupt=copy_sidecar(capture);CHECK(corrupt);corrupt[capture->sidecar_size-1]^=1;
    CHECK(p4_mod_save_stage_image(&saved,corrupt,capture->sidecar_size,&load,error,sizeof error)<0&&!load);
    CHECK(f.active==UINT32_MAX);
    image[100]^=1;
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"different retail data"));
    image[100]^=1;
    CHECK(!p4_mod_save_unregister(registration));registration=NULL;
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(capture),capture->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"Missing required"));
    CHECK(!p4_mod_save_reset_session(error,sizeof error));
    CHECK(!p4_mod_save_register(&p,&registration));
    CHECK(!p4_mods_stage(argv[1],"save.dep")&&!p4_package_registry_publish());
    CHECK(p4_packages_count()==1);
    f.dependency=p4_packages_provider(0);
    CHECK(!p4_mod_save_touch(f.dependency));
    P4ModSaveCapture *dependent=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&dependent,error,sizeof error)&&dependent);
    f.use_string=1;
    P4ModSaveCapture *resolved=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&resolved,error,sizeof error)&&resolved);
    CHECK(resolved->sidecar_size==dependent->sidecar_size&&!memcmp(resolved->sidecar,dependent->sidecar,resolved->sidecar_size));
    p4_mod_save_capture_free(resolved);f.use_string=0;
    CHECK(!p4_mod_save_stage_image(&saved,copy_sidecar(dependent),dependent->sidecar_size,&load,error,sizeof error));
    CHECK(!p4_mod_save_load_warning(load));p4_mod_save_load_abort(load);load=NULL;
    p4_package_registry_retire();f.dependency=NULL;
    CHECK(p4_mod_save_stage_image(&saved,copy_sidecar(dependent),dependent->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"requires package"));
    CHECK(!p4_mod_save_reset_session(error,sizeof error));
    CHECK(f.active==0&&f.new_game==1);
    /* Resetting game state cannot declassify an already witnessed native run. */
    CHECK(!p4_mod_save_capture_image(&saved,&empty,error,sizeof error)&&empty&&empty->witness);
    p4_mod_save_capture_free(empty);empty=NULL;
    CHECK(!memcmp(image,original,sizeof image));
    CHECK(!p4_mod_save_unregister(registration));
    p4_mod_save_capture_free(capture);p4_mod_save_capture_free(dependent);
    Fixture opaque_fixture={.active=0x12345678,.owner="fixture.opaque"};
    P4ModSaveProvider opaque_provider=provider(&opaque_fixture);opaque_provider.flags=0;
    P4ModSaveRegistration *opaque_registration=NULL;
    CHECK(!p4_mod_save_register(&opaque_provider,&opaque_registration));
    P4ModSaveCapture *opaque=NULL,*preserved=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&opaque,error,sizeof error)&&opaque);
    CHECK(!p4_mod_save_unregister(opaque_registration));
    CHECK(!p4_mod_save_stage_image(&saved,copy_sidecar(opaque),opaque->sidecar_size,&load,error,sizeof error));
    p4_mod_save_load_commit(load);load=NULL;
    CHECK(!p4_mod_save_capture_image(&saved,&preserved,error,sizeof error)&&preserved);
    CHECK(opaque->sidecar_size==preserved->sidecar_size&&!memcmp(opaque->sidecar,preserved->sidecar,opaque->sidecar_size));
    unsigned char *invalid=copy_sidecar(opaque);CHECK(invalid);
    word(invalid+96+8,2);
    p4_save_hash(invalid,opaque->sidecar_size-32,invalid+opaque->sidecar_size-32);
    CHECK(p4_mod_save_stage_image(&saved,invalid,opaque->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"Invalid save block"));
    invalid=copy_sidecar(opaque);CHECK(invalid);memset(invalid+96+16,255,8);
    p4_save_hash(invalid,opaque->sidecar_size-32,invalid+opaque->sidecar_size-32);
    CHECK(p4_mod_save_stage_image(&saved,invalid,opaque->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"Invalid save block"));
    invalid=copy_sidecar(opaque);CHECK(invalid);word(invalid+8,2);
    p4_save_hash(invalid,opaque->sidecar_size-32,invalid+opaque->sidecar_size-32);
    CHECK(p4_mod_save_stage_image(&saved,invalid,opaque->sidecar_size,&load,error,sizeof error)<0&&!load&&strstr(error,"Unsupported"));
    CHECK(!p4_mod_save_reset_session(error,sizeof error));
    p4_mod_save_capture_free(opaque);p4_mod_save_capture_free(preserved);
    Fixture bounded={.active=17,.owner="fixture.bound"};
    P4ModSaveProvider bounded_provider=provider(&bounded);bounded_provider.max_bytes=3;
    P4ModSaveRegistration *bounded_registration=NULL;
    CHECK(!p4_mod_save_register(&bounded_provider,&bounded_registration));
    CHECK(p4_mod_save_capture_image(&saved,&empty,error,sizeof error)<0&&!empty&&strstr(error,"payload exceeds its declared bound"));
    CHECK(!p4_mod_save_unregister(bounded_registration));bounded_registration=NULL;
    bounded_provider.max_bytes=4;bounded.omit_after_write=1;
    CHECK(!p4_mod_save_register(&bounded_provider,&bounded_registration));
    CHECK(p4_mod_save_capture_image(&saved,&empty,error,sizeof error)<0&&!empty&&strstr(error,"encode refused"));
    CHECK(!p4_mod_save_unregister(bounded_registration));
    CHECK(!p4_packages_memory_usage().resident&&!p4_packages_memory_usage().reserved);
    puts("native sidecar codec: synthetic fixture complete; no gameplay/parity claim");
    return 0;
}
