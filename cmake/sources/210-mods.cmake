target_compile_definitions(p4port PRIVATE P4_MOD_GAME_BINDINGS=1)
target_sources(p4port PRIVATE src/platform/mods/mods.c src/platform/mods/mods_config.c
  src/platform/mods/packs.c src/platform/mods/packs_config.c
  src/platform/mods/package_registry.c
  src/platform/mods/package_memory.c src/platform/mods/mod_setting_store.c src/platform/mods/mod_setting_gate.c
  src/platform/mods/lifecycle.c
  src/platform/mods/compendium.c src/platform/mods/file_index.c third_party/yxml/yxml.c)
# The [setting] manifest section owner; values persist beside p4pc.ini.
p4_register_package_boot_provider(p4_mod_settings_boot_provider)
