target_compile_definitions(p4port PRIVATE P4_MOD_GAME_BINDINGS=1)
target_sources(p4port PRIVATE src/platform/mods/mods.c src/platform/mods/mods_config.c
  src/platform/mods/packs.c src/platform/mods/packs_config.c
  src/platform/mods/package_registry.c
  src/platform/mods/package_memory.c
  src/platform/mods/compendium.c src/platform/mods/file_index.c third_party/yxml/yxml.c)
