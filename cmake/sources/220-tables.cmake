target_sources(p4port PRIVATE src/platform/mods/game_tables.c src/platform/mods/game_tables_prepare.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_inputs.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_admission.c)
# The battle-bank slot-extent registry is always built; retail sessions read it.
target_sources(p4port PRIVATE src/platform/mods/game_table_extents.c)
target_sources(p4port PRIVATE src/platform/mods/game_skills.c src/platform/mods/game_items.c src/platform/mods/game_enemies.c
  src/platform/mods/game_personas.c
  src/platform/mods/game_items_save.c src/platform/mods/game_table_sources.c
  src/platform/mods/game_table_refs.c src/platform/mods/game_table_inputs_use.c)
target_sources(p4port PRIVATE src/platform/mods/game_skills_native.c src/platform/mods/game_enemies_native.c
  src/platform/mods/game_personas_native.c src/platform/mods/game_tables_packed.c)
target_sources(p4port PRIVATE src/platform/mods/game_elements_native.c
  src/platform/mods/game_elements_affinity.c src/platform/mods/game_elements_action.c
  src/platform/mods/game_elements_route.c)
target_sources(p4port PRIVATE src/platform/mods/game_items_native.c src/platform/mods/game_items_ui.c
  src/platform/mods/game_items_equipment.c src/platform/mods/game_items_battle.c
  src/platform/mods/game_items_list.c src/platform/mods/game_items_shop.c src/platform/mods/game_items_text.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_builder.c
  src/platform/mods/game_table_xml.c src/platform/mods/game_table_manifest.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_boot.c)
p4_register_package_boot_provider(p4_game_tables_sources_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_items_ui_boot.c)
p4_register_package_boot_provider(p4_game_tables_items_ui_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_knowledge.c src/platform/mods/game_knowledge_native.c
  src/platform/mods/game_knowledge_save.c src/platform/mods/game_knowledge_boot.c)
p4_register_package_boot_provider(p4_game_knowledge_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_elements_ai_sources.c src/platform/mods/game_elements_ai_native.c
  src/platform/mods/game_elements_ai_vm.c src/platform/mods/game_elements_ai_proof.c
  src/platform/mods/game_elements_ai_decision.c src/platform/mods/game_elements_ai_roles.c
  src/platform/mods/game_elements_ai_boot.c)
p4_register_package_boot_provider(p4_game_elements_ai_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_text.c src/platform/mods/game_text_native.c)
target_sources(p4port PRIVATE src/platform/mods/native_glyphs.c src/platform/mods/native_glyphs_native.c)
target_sources(p4port PRIVATE src/platform/mods/mod_resources.c src/platform/mods/mod_resources_native.c)
