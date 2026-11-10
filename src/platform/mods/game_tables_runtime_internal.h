#ifndef P4PC_GAME_TABLES_RUNTIME_INTERNAL_H
#define P4PC_GAME_TABLES_RUNTIME_INTERNAL_H
#include "p4pc/game_tables.h"
#include "p4pc/game_table_use.h"
/* Host-only actual native semantic consumption. Public classifiers and all
 * preparation/serialization/UI definition builders remain non-touch. */
int p4_game_tables_classify_runtime_action_element(const P4GameTables *tables,
    const P4GameActionElement *declared, P4GameActionElement *resolved);
#endif
