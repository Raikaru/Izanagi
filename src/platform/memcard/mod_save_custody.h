#ifndef P4PC_MOD_SAVE_CUSTODY_H
#define P4PC_MOD_SAVE_CUSTODY_H
#include <stddef.h>
#define P4_SAVE_MODE_MARKER "p4pc-run-mode.json"
#define P4_SAVE_MODE_SUFFIX ".p4pc-mode.json"
/*0=ordinary,1=explicit script-only witness,-1=invalid boundary. */
int p4_save_witness_mode(char *error,size_t capacity);
/* Known portable marker/suffix only, never a guessed mod/native identity. */
int p4_save_witness_name(const char *name);
/* Checks file's portable marker plus ancestor run markers; directory input
 * starts at itself.0/1/-1. Refuses uninspectable/deep custody, never clears it. */
int p4_save_witness_input(const char *path,int directory,char *error,size_t capacity);
/* Writes canonical schema1 non-reference marker inside owned staging output. */
int p4_save_witness_mark(const char *directory,char *error,size_t capacity);
#endif
