# Native half of the mod game services (include/p4pc/mod_services.h): the
# game's own field and battle script-command paths, run only at the frame
# boundary by src/platform/mods/game_services.c (tools/mods, Luau builds).
target_sources(p4port PRIVATE src/platform/mods/services_native.c)
