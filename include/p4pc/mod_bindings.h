#ifndef P4PC_MOD_BINDINGS_H
#define P4PC_MOD_BINDINGS_H
/* Native loader only; no VM/C++/guest ABI types. Install real game services
 * before staging sources. Lua-disabled builds install no script services. */
int p4_mod_game_bindings_init(void);
/* Install the same real native recovery owner without advertising game/UI
 * services. Native VM embeddings must first install the C package frontend. */
int p4_mod_script_failure_init(void);
#endif
