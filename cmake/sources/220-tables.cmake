target_sources(p4port PRIVATE src/platform/mods/game_tables.c src/platform/mods/game_tables_prepare.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_inputs.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_admission.c src/platform/mods/game_table_derived.c)
# The battle-bank slot-extent registry is always built; retail sessions read it.
target_sources(p4port PRIVATE src/platform/mods/game_table_extents.c)
# Per-run session overlays of those banks (p4.tables); idle until a mod applies one.
target_sources(p4port PRIVATE src/platform/mods/game_table_overlay.c)
target_sources(p4port PRIVATE src/platform/mods/game_skills.c src/platform/mods/game_items.c src/platform/mods/game_enemies.c
  src/platform/mods/game_personas.c
  src/platform/mods/game_items_save.c src/platform/mods/game_table_sources.c
  src/platform/mods/game_table_refs.c src/platform/mods/game_table_inputs_use.c)
target_sources(p4port PRIVATE src/platform/mods/game_skills_native.c src/platform/mods/game_enemies_native.c
  src/platform/mods/game_personas_native.c src/platform/mods/game_personas_save.c src/platform/mods/game_skills_save.c
  src/platform/mods/game_tables_packed.c src/platform/mods/game_table_derived_native.c)
target_sources(p4port PRIVATE src/platform/mods/game_elements_native.c
  src/platform/mods/game_elements_affinity.c src/platform/mods/game_elements_action.c
  src/platform/mods/game_elements_route.c)
# Native amount stages on one shared ordered registry, each inert until a
# modifier registers: final damage and recovery at 0x00235520, skill cost at
# 0x0023D9B0, status/KO chance at 0x00238940, ailment wear-off at 0x001F6930,
# buff multipliers at 0x00233BC0, battle rewards at 0x0021E110 (EXP) and
# 0x0021E050/0x0021E9A0 (money), unit values at 0x00231F80/0x00232290 (max
# HP/SP), 0x00232C70 (battle stats) and 0x00231AF0 (an enemy's starting
# HP/SP), and accuracy and critical chances at 0x0023E6F0 and 0x002411A0; the
# chance and wear-off stages report the afflictions datCalcSetBadStatus
# (0x00232680) counts per battle. Damage cover rows register final-damage
# modifiers at table-source publication.
target_sources(p4port PRIVATE src/platform/mods/game_stage.c src/platform/mods/game_damage_stage.c
  src/platform/mods/game_recovery_stage.c src/platform/mods/game_cost_stage.c src/platform/mods/game_chance_stage.c
  src/platform/mods/game_wearoff_stage.c src/platform/mods/game_buff_stage.c src/platform/mods/game_afflictions.c
  src/platform/mods/game_reward_stage.c src/platform/mods/game_unit_stage.c src/platform/mods/game_hit_stage.c
  src/platform/mods/game_damage_cover.c)
# Passive grants: the (scope, key, skill) table every passive check asks after
# a unit's slots and accessory, inert (one load) while nothing is granted.
target_sources(p4port PRIVATE src/platform/mods/game_passive_grant.c)
# Side ailments: battle state of <status-ailment> leaves, inert without rows.
target_sources(p4port PRIVATE src/platform/mods/game_side_ailment.c src/platform/mods/game_side_ailment_battle.c)
target_sources(p4port PRIVATE src/platform/mods/game_items_native.c src/platform/mods/game_items_ui.c
  src/platform/mods/game_items_equipment.c src/platform/mods/game_items_battle.c
  src/platform/mods/game_items_list.c src/platform/mods/game_items_shop.c src/platform/mods/game_items_text.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_builder.c
  src/platform/mods/game_table_xml.c src/platform/mods/game_table_manifest.c)
target_sources(p4port PRIVATE src/platform/mods/game_table_boot.c src/platform/mods/game_table_delta.c)
p4_register_package_boot_provider(p4_game_tables_sources_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_items_ui_boot.c)
p4_register_package_boot_provider(p4_game_tables_items_ui_boot_provider)
# p4.data: the semantic catalog over the loaded banks (keys from the disc's
# names, captured at boot only when a script or data layer is admitted) and
# the data.luau / data/edits.xml layers the derived compose stage runs, plus
# the session edits' save block.
target_sources(p4port PRIVATE src/platform/mods/game_data.c src/platform/mods/game_data_catalog.c
  src/platform/mods/game_data_edit.c src/platform/mods/game_data_edit_xml.c src/platform/mods/game_data_session.c)
p4_register_package_boot_provider(p4_game_data_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_knowledge.c src/platform/mods/game_knowledge_native.c
  src/platform/mods/game_knowledge_save.c src/platform/mods/game_knowledge_boot.c)
p4_register_package_boot_provider(p4_game_knowledge_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_elements_ai_sources.c src/platform/mods/game_elements_ai_native.c
  src/platform/mods/game_elements_ai_vm.c
  src/platform/mods/game_elements_ai_decision.c src/platform/mods/game_elements_ai_roles.c
  src/platform/mods/game_elements_ai_boot.c)
p4_register_package_boot_provider(p4_game_elements_ai_boot_provider)
target_sources(p4port PRIVATE src/platform/mods/game_text.c src/platform/mods/game_text_native.c)
target_sources(p4port PRIVATE src/platform/mods/native_glyphs.c src/platform/mods/native_glyphs_native.c)
target_sources(p4port PRIVATE src/platform/mods/mod_resources.c src/platform/mods/mod_resources_native.c)
