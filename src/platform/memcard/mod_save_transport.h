#ifndef P4PC_MOD_SAVE_TRANSPORT_H
#define P4PC_MOD_SAVE_TRANSPORT_H
#include "save_transaction.h"
#include <stdint.h>
/* Native-only callbacks; an absent confirmation callback REFUSES, never
 * silently acknowledges missing mods. Scripted fidelity paths never use this. */
typedef int (*P4SaveConfirmFn)(const char *warning);
typedef void (*P4SaveErrorFn)(const char *operation,const char *reason);
void p4_mod_save_transport_ui(P4SaveConfirmFn,P4SaveErrorFn);
void p4_mod_save_transport_error(const char *operation,const char *reason);
/* Capture at the actual full-image write submission, before the task yields.
 * The complete retail image is borrowed for this call only. */
int p4_mod_save_transport_begin(const char *card,unsigned slot,const void *image,size_t size,char *error,size_t cap);
void p4_mod_save_transport_abort(void);
int p4_mod_save_transport_saving(void);
/*0=not this intent,+1=routed,-1=refused. All names are canonical card-relative
 * slot/file components, not guessed host paths. NULL file means slot directory. */
int p4_mod_save_transport_route(const char *card,const char *slot,const char *file,char *out,size_t capacity);
int p4_mod_save_transport_written(const char *card,const char *slot,const char *file,
    uint32_t offset,uint32_t size,char *error,size_t cap);
/* Called while the MC adapter holds the SAME root lock as the retail read.
 * Stages every extension before the read can report success. No GP mutation. */
int p4_mod_save_transport_loaded(const P4SaveTransaction *,const void *image,size_t size,char *error,size_t cap);
void p4_mod_save_transport_load_abort(void);
int p4_mod_save_transport_restore(int32_t version,const void *payload,size_t size,char *error,size_t cap);
void p4_mod_save_transport_restore_commit(void);
int p4_mod_save_transport_reset(char *error,size_t cap);
void p4_mod_save_transport_reset_commit(void);
#endif
