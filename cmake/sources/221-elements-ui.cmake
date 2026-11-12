target_sources(p4port PRIVATE src/platform/mods/game_elements_ui.c
  src/platform/mods/game_elements_ui_native.c src/platform/mods/game_elements_ui_boot.c)
p4_register_package_boot_provider(p4_game_elements_ui_boot_provider)
