#ifndef P4PC_SAVE_TRANSACTION_H
#define P4PC_SAVE_TRANSACTION_H
#include <stddef.h>
#include <stdint.h>
#define P4_SAVE_PATH_CAP 4096u
typedef struct P4SavePairBinding {
    uint64_t retail_size,sidecar_size;
    unsigned char retail_hash[32],sidecar_hash[32];
} P4SavePairBinding;
/* Caller-owned native state (never guest ABI). Zero-initialize; card remains
 * borrowed/stable until commit/abort. One logical transaction per game thread;
 * the OS directory lock excludes other processes without creating MC files. */
typedef struct P4SaveTransaction {
    const char *card;
    char name[33],stage[64],backup[64];
    uintptr_t lock[2];
    P4SavePairBinding binding;
    unsigned state,old,journal,stage_owned,publishing;
} P4SaveTransaction;
int p4_save_slot_index(const char *name);
/* Acquires root, recovers interrupted generations, then selects preferred0..15.
 * replace authorizes occupied slots; choose_free keeps preferred if free else
 * picks first free. An existing linked/non-directory destination is refused. */
int p4_save_transaction_begin(P4SaveTransaction *,const char *card,unsigned preferred,
    int replace,int choose_free,char *error,size_t error_capacity);
/* Exact existing metadata presence under the held root lock. */
int p4_save_transaction_tagged(const P4SaveTransaction *);
/* Records intent BEFORE any stage payload is written. Plain pristine game
 * writes may hold only begin's lock and keep their original byte/I/O path. */
int p4_save_transaction_stage(P4SaveTransaction *,const P4SavePairBinding *,char *error,size_t error_capacity);
/* Preserve bounded extra regular files; the three rewritten game files and old
 * .p4mods pair are excluded. Links/subdirectories refuse, never silently drop. */
int p4_save_transaction_clone_extra(P4SaveTransaction *,char *error,size_t error_capacity);
/* NULL file means slot directory. Basenames only, including known metadata. */
int p4_save_transaction_path(const P4SaveTransaction *,const char *file,char *out,size_t capacity);
/*0=committed; -1=error. Always consumes the live lock/state. If real rename
 * restoration fails, the journal owns retained stage/backup, never the caller. */
int p4_save_transaction_commit(P4SaveTransaction *,char *error,size_t error_capacity);
void p4_save_transaction_abort(P4SaveTransaction *);
int p4_save_transactions_recover(const char *card,char *error,size_t error_capacity);
/* Verifies exact bound bytes, not active runtime-provider availability. */
int p4_save_pair_verify(const char *card,const char *slot,const P4SavePairBinding *,char *error,size_t capacity);
#endif
