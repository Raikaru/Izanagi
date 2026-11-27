# Read-only reader for the player's own Persona 4 Golden install and the p4pc:golden
# derived-table producer (docs/modding/golden.md). The producer runs only when a package
# selects it; no Golden bytes are shipped or cached.
target_sources(p4port PRIVATE src/platform/golden/golden_formats.c src/platform/golden/golden_source.c
  src/platform/golden/c03_guard.c
  src/platform/golden/golden_cpk_vfs.c
  src/platform/golden/golden_texture.c src/platform/golden/golden_atlas.c
  src/platform/golden/golden_icons.c
  src/platform/golden/golden_message.c src/platform/golden/golden_font.c
  src/platform/golden/golden_model.c src/platform/golden/golden_model_image.c
  src/platform/golden/golden_model_scene.c src/platform/golden/golden_model_build.c
  src/platform/golden/golden_model_motion.c
  src/platform/golden/golden_awb.c src/platform/golden/golden_acb.c
  src/platform/golden/golden_hca_header.c src/platform/golden/golden_hca.c
  src/platform/golden/golden_adx.c
  src/platform/sound/p4snd_golden.c
  src/platform/golden/golden_tables.c src/platform/golden/golden_producer.c
  src/platform/golden/golden_domains.c src/platform/golden/golden_exp.c src/platform/golden/golden_mode_declarations.c
  src/platform/golden/golden_mode_projection.c src/platform/golden/golden_mode_identity.c
  src/platform/golden/golden_mode_identity_source.c src/platform/golden/golden_mode_boot.c
  src/platform/golden/golden_mode_sources.c src/platform/golden/golden_mode_declare_builder.c
  src/platform/golden/golden_mode_cache_publish.c src/platform/golden/golden_mode_declare_cached.c
  src/platform/golden/golden_mode_outputs.c src/platform/golden/golden_mode_image.c
  src/platform/golden/golden_mode_image_bind.c
  src/platform/golden/golden_battle_rows_native.c
  src/platform/golden/golden_battle_rows.c
  src/platform/golden/golden_items.c src/platform/golden/golden_items_native.c
  src/platform/golden/golden_encounter_rates.c
  src/platform/golden/golden_inheritance.c src/platform/golden/golden_inheritance_source.c
  src/platform/golden/golden_recipe.c src/platform/golden/golden_recipe_runtime.c src/platform/golden/golden_cmm.c src/platform/golden/golden_cmm_source.c
  src/platform/golden/golden_skill_convert.c src/platform/golden/golden_item_convert.c
  src/platform/golden/golden_names.c src/platform/golden/golden_names_native.c src/platform/golden/golden_encounter_convert.c
  src/platform/golden/golden_persona_convert.c src/platform/golden/golden_enemy_convert.c
  src/platform/golden/golden_fusion_convert.c src/platform/golden/golden_fusion_source.c src/platform/golden/golden_fusion_runtime.c
  src/platform/golden/golden_extra_source.c src/platform/golden/golden_cache.c
  src/platform/golden/golden_table_cache.c)
target_sources(p4port PRIVATE src/platform/golden/golden_ai.c src/platform/golden/golden_ai_rng.c
  src/platform/golden/golden_bf.c src/platform/golden/golden_vm.c
  src/platform/golden/golden_vm_program.c src/platform/golden/golden_pe_data.c
  src/platform/golden/golden_levels.c src/platform/golden/golden_ai_product.c
  src/platform/golden/golden_ai_boot.c src/platform/golden/golden_ai_native.c
  src/platform/golden/golden_ai_rng_native.c src/platform/golden/golden_vm_native.c
  src/platform/golden/golden_ai_knowledge.c src/platform/golden/golden_ai_knowledge_native.c)
p4_register_package_boot_provider(p4_golden_ai_boot_provider)

option(P4_GOLDEN_AI_QUALIFICATION "Build test-only full-native Golden AI install qualification" OFF)
if(P4_GOLDEN_AI_QUALIFICATION)
  if(CMAKE_BUILD_TYPE STREQUAL "Release" OR P4_ANDROID)
    message(FATAL_ERROR "Golden AI qualification is test-only, never a release/mobile product")
  endif()
  add_compile_definitions(P4_GOLDEN_AI_QUALIFICATION=1)
endif()
