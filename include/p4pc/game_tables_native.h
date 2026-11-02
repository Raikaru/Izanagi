#ifndef P4PC_GAME_TABLES_NATIVE_H
#define P4PC_GAME_TABLES_NATIVE_H
#include "p4pc/table_result.h"
/* Actual0010d480 loader completion, not a script-callable publication API. */
int p4_game_tables_battle_complete(void);
/* The package boot transaction uses the SAME coordinator: prepare all before
 * any package/native publish, then infallible nonyielding commit. Abort drops
 * only unpublished transaction state; owners retain/finally release staging.
 * commit requires a successful prepare and cannot be called twice. */
int p4_game_tables_battle_prepare(void);
void p4_game_tables_battle_commit(void);
int p4_game_tables_battle_abort(void);
_Noreturn void p4_game_tables_battle_refuse(int result);
#endif
