#include "mod_save_transport.h"
#include "mod_save_internal.h"
#include "mod_save_custody.h"
#include "mod_save_hash.h"
#include "save_image.h"
#include "p4pc/save_import.h"
#include "p4pc/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4SaveTransaction save;
static P4ModSaveCapture *capture;
static char save_card[P4_SAVE_PATH_CAP];
static unsigned files_written;
static P4ModSaveLoad *load;
static int reset_pending;
static P4SaveConfirmFn confirm_warning;
static P4SaveErrorFn report_error;
static int fail(char *error,size_t cap,const char *reason)
{
    if(cap)snprintf(error,cap,"%s",reason);
    return -1;
}
void p4_mod_save_transport_ui(P4SaveConfirmFn confirm,P4SaveErrorFn report)
{
    confirm_warning=confirm;report_error=report;
}
void p4_mod_save_transport_error(const char *operation,const char *reason)
{
    if(report_error)report_error(operation,reason);
    else fprintf(stderr,"p4pc: save refused: %s: %s\n",operation,reason);
}
int p4_mod_save_transport_saving(void){return save.state!=0;}
void p4_mod_save_transport_abort(void)
{
    p4_save_transaction_abort(&save);files_written=0;
    p4_mod_save_capture_free(capture);capture=NULL;
}
void p4_mod_save_transport_load_abort(void)
{
    p4_mod_save_load_abort(load);load=NULL;reset_pending=0;
}
static int write_metadata(const char *name,const void *data,size_t size)
{
    char path[P4_SAVE_PATH_CAP];
    if(p4_save_transaction_path(&save,name,path,sizeof path))return -1;
    FILE *file=fopen(path,"wbx");if(!file)return -1;
    int bad=fwrite(data,1,size,file)!=size;
    if(fclose(file))bad=1;
    return bad?-1:0;
}
int p4_mod_save_transport_begin(const char *card,unsigned slot,const void *image,size_t size,char *error,size_t cap)
{
    p4_mod_save_transport_abort();p4_mod_save_transport_load_abort();
    if(!image||size<4||size>P4SAVE_MAX_BYTES||strlen(card)>=sizeof save_card)
        return fail(error,cap,"Invalid complete native save image or card path");
    int witness=p4_save_witness_mode(error,cap);
    if(witness<0)return -1;
    strcpy(save_card,card);
    if(p4_save_transaction_begin(&save,save_card,slot,1,0,error,cap))return -1;
    char directory[P4_SAVE_PATH_CAP];
    /* An overwritten slot is an OLD generation, not consumed game state.
     * Only destination/card-wide custody applies to the new generation. */
    int inherited=p4_save_witness_input(save.card,1,error,cap);
    if(inherited<0)goto bad;
    if(inherited&&!witness){fail(error,cap,"Memory-card destination belongs to the mod-witness test lane");goto bad;}
    if(inherited)p4_mod_save_witness_source();
    int tagged=p4_save_transaction_tagged(&save);
    if(tagged<0){fail(error,cap,"Cannot inspect existing native save metadata");goto bad;}
    int old_custody=0;
    if(save.old){
        if(p4_save_transaction_path(&save,save.name,directory,sizeof directory)){fail(error,cap,"Old save custody path is too long");goto bad;}
        old_custody=p4_save_witness_input(directory,0,error,cap);
        if(old_custody<0)goto bad;
    }
    int codec_active=p4_mod_save_needs_codec();
    P4ModSaveImage raw={.data=image,.size=size,.slot=save.name};
    if(codec_active&&p4_mod_save_capture_image(&raw,&capture,error,cap))goto bad;
    if((!capture||!capture->sidecar_size)&&!old_custody&&!tagged)return 0;
    P4SavePairBinding binding={.retail_size=size};
    if(capture&&capture->sidecar_size){
        memcpy(binding.retail_hash,capture->retail_hash,32);
        binding.sidecar_size=capture->sidecar_size;
        memcpy(binding.sidecar_hash,capture->sidecar_hash,32);
    }else{
        if(!codec_active&&p4save_image_validate(image,size,error,cap))goto capture_bad;
        p4_save_hash(image,size,binding.retail_hash);
    }
    if(p4_save_transaction_stage(&save,&binding,error,cap)||
       p4_save_transaction_clone_extra(&save,error,cap))goto capture_bad;
    if(capture&&capture->sidecar_size){
        unsigned char marker[P4_MOD_SAVE_MARKER_BYTES];
        p4_save_marker_make(binding.retail_size,binding.retail_hash,binding.sidecar_size,binding.sidecar_hash,marker);
        if(write_metadata(P4_MOD_SAVE_SIDECAR,capture->sidecar,capture->sidecar_size)||
           write_metadata(P4_MOD_SAVE_COMMIT,marker,sizeof marker)){
            fail(error,cap,"Cannot stage authenticated native save metadata");goto capture_bad;
        }
        if(capture->witness){
            if(p4_save_transaction_path(&save,NULL,directory,sizeof directory)||
               p4_save_witness_mark(directory,error,cap))goto capture_bad;
        }
    }
    return 0;
capture_bad:
    p4_mod_save_capture_free(capture);capture=NULL;
bad:
    p4_mod_save_transport_abort();return -1;
}
int p4_mod_save_transport_route(const char *card,const char *slot,const char *file,char *out,size_t cap)
{
    if(!save.state||strcmp(card,save.card)||strcmp(slot,save.name))return 0;
    return p4_save_transaction_path(&save,file,out,cap)?-1:1;
}
int p4_mod_save_transport_written(const char *card,const char *slot,const char *file,
    uint32_t offset,uint32_t size,char *error,size_t cap)
{
    if(!save.state||strcmp(card,save.card)||strcmp(slot,save.name))return 0;
    unsigned bit=!strcmp(file,slot)?1u:!strcmp(file,"icon.ico")?2u:!strcmp(file,"icon.sys")?4u:0;
    if(!bit||offset||!size||(bit==1&&save.binding.retail_size&&size!=save.binding.retail_size)||
       (bit==2&&files_written!=1)||(bit==4&&(files_written!=3||size!=964))){
        p4_mod_save_transport_abort();return fail(error,cap,"Native save did not complete the declared data/icon.ico/icon.sys write order");
    }
    files_written|=bit;
    if(bit!=4)return 0;
    int result=p4_save_transaction_commit(&save,error,cap);files_written=0;
    if(result)p4_mod_save_capture_free(capture);
    else p4_mod_save_capture_commit(capture);
    capture=NULL;return result;
}
static unsigned char *read_metadata(const P4SaveTransaction *transaction,const char *name,size_t limit,size_t *size)
{
    char path[P4_SAVE_PATH_CAP];
    if(p4_save_transaction_path(transaction,name,path,sizeof path)||!p4_os_is_file_nofollow(path))return NULL;
    FILE *file=p4_os_fopen_rb(path);if(!file)return NULL;
    long length;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||(size_t)length>limit||fseek(file,0,SEEK_SET)){
        fclose(file);return NULL;
    }
    unsigned char *data=malloc(length?(size_t)length:1);
    if(!data){fclose(file);return NULL;}
    int bad=fread(data,1,(size_t)length,file)!=(size_t)length;
    if(fclose(file))bad=1;
    if(bad){free(data);return NULL;}
    *size=(size_t)length;return data;
}
int p4_mod_save_transport_loaded(const P4SaveTransaction *transaction,const void *image,size_t size,char *error,size_t cap)
{
    p4_mod_save_transport_load_abort();
    int witness_mode=p4_save_witness_mode(error,cap);
    if(witness_mode<0)return -1;
    char directory[P4_SAVE_PATH_CAP];
    if(p4_save_transaction_path(transaction,transaction->name,directory,sizeof directory))return fail(error,cap,"Native load custody path is too long");
    int witness=p4_save_witness_input(directory,0,error,cap);
    if(witness<0)return -1;
    if(witness&&!witness_mode)return fail(error,cap,"This mod-witness test save cannot be used in a retail/player run");
    if(witness)p4_mod_save_witness_source();
    unsigned char *sidecar=NULL;size_t sidecar_size=0;
    int tagged=p4_save_transaction_tagged(transaction);
    if(tagged<0)return fail(error,cap,"Cannot inspect native load metadata");
    if(tagged){
        size_t marker_size=0;
        sidecar=read_metadata(transaction,P4_MOD_SAVE_SIDECAR,P4_MOD_SAVE_MAX_BYTES,&sidecar_size);
        unsigned char *marker=read_metadata(transaction,P4_MOD_SAVE_COMMIT,P4_MOD_SAVE_MARKER_BYTES,&marker_size);
        if(!sidecar||!marker||sidecar_size<P4_MOD_SAVE_HEADER_BYTES){
            free(sidecar);free(marker);return fail(error,cap,"Incomplete or unreadable .p4mods save transaction");
        }
        unsigned char digest[32];p4_save_hash(sidecar,sidecar_size,digest);
        /* Validate the commit before any provider runs. The codec then proves
         * this claimed retail digest against the actual complete image. */
        int invalid=p4_save_marker_verify(size,sidecar+24,sidecar_size,digest,marker,marker_size,error,cap);
        free(marker);if(invalid){free(sidecar);return -1;}
        if((sidecar[68]&P4_MOD_SAVE_WITNESS)&&!witness_mode){
            free(sidecar);return fail(error,cap,"This mod-witness test save requires the explicit mod-test lane; it is not a retail reference");
        }
    }
    P4ModSaveImage raw={.data=image,.size=size,.slot=transaction->name};
    if(p4_mod_save_stage_image(&raw,sidecar,sidecar_size,&load,error,cap))return -1;
    if(p4_mod_save_load_witness(load)&&!witness_mode){
        p4_mod_save_transport_load_abort();return fail(error,cap,"Mod-witness state cannot enter a retail/player run");
    }
    const char *warning=p4_mod_save_load_warning(load);
    if(warning&&(!confirm_warning||!confirm_warning(warning))){
        if(!confirm_warning)p4_mod_save_transport_error("Missing saved mods",warning);
        p4_mod_save_transport_load_abort();return fail(error,cap,"Load cancelled: missing saved mods were not acknowledged");
    }
    return 0;
}
int p4_mod_save_transport_loaded_untagged(const void *image,size_t size,const char *label,char *error,size_t cap)
{
    p4_mod_save_transport_load_abort();
    if(!image||size<4)return fail(error,cap,"Invalid complete native save image");
    P4ModSaveImage raw={.data=image,.size=size,.slot=label};
    return p4_mod_save_stage_image(&raw,NULL,0,&load,error,cap);
}
int p4_mod_save_transport_restore(int32_t version,const void *payload,size_t size,char *error,size_t cap)
{
    if(!load&&!p4_mod_save_needs_codec())return 0;
    if(reset_pending||!p4_mod_save_load_matches(load,version,payload,size))
        return fail(error,cap,"Full save restore has no matching validated native extension transaction");
    return 0;
}
void p4_mod_save_transport_restore_commit(void)
{
    p4_mod_save_load_commit(load);load=NULL;
    p4_mod_save_load_notify();
}
int p4_mod_save_transport_reset(char *error,size_t cap)
{
    p4_mod_save_transport_abort();p4_mod_save_transport_load_abort();
    P4ModSaveImage empty={0};
    if(p4_mod_save_stage_image(&empty,NULL,0,&load,error,cap))return -1;
    reset_pending=1;return 0;
}
void p4_mod_save_transport_reset_commit(void)
{
    p4_mod_save_load_commit(load);load=NULL;reset_pending=0;
    p4_mod_save_load_notify();
}
