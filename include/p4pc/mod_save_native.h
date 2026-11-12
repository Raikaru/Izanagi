#ifndef P4PC_MOD_SAVE_NATIVE_H
#define P4PC_MOD_SAVE_NATIVE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Scalar-only game-facing boundary: no native provider/transaction records in
 * guest data. Native services are installed even when all selectors are off. */
void p4_mod_save_native_init(void);
void p4_mc_mod_save_begin(int32_t socket,int32_t slot,const void *image,int32_t size);
/* Actual full READ submission cancels the previous logical MC intent before
 * any searches. Unrelated low-level reads cannot cancel a live save. */
void p4_mc_mod_load_begin(void);
int p4_mod_save_restore_begin(int32_t version,const void *payload,int32_t size);
void p4_mod_save_restore_commit(void);
int p4_mod_save_new_game_begin(void);
void p4_mod_save_new_game_commit(void);
/* Only an invariant failure between validated read and actual restore reaches
 * this path. Ordinary bad/missing metadata is refused as a real MC read error,
 * before any retail or extension state changes. */
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void p4_mod_save_restore_failed(void);
#ifdef __cplusplus
}
#endif
#endif
