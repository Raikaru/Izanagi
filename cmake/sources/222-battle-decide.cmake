# The battle decision stage (p4.battle.decide): core, because overrides/001a2b50.c
# calls it from the mirror in every build; the Luau binding lives in p4_mod_game.
target_sources(p4port PRIVATE src/platform/mods/battle_decide.c)
