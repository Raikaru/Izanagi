# Read-only reader for the player's own Persona 4 Golden install and the p4pc:golden
# derived-table producer (docs/modding/golden.md). The producer runs only when a package
# selects it; no Golden bytes are shipped or cached.
target_sources(p4port PRIVATE src/platform/golden/golden_formats.c src/platform/golden/golden_source.c
  src/platform/golden/golden_tables.c src/platform/golden/golden_producer.c
  src/platform/golden/golden_domains.c src/platform/golden/golden_exp.c src/platform/golden/golden_mode_declarations.c
  src/platform/golden/golden_mode_projection.c src/platform/golden/golden_mode_identity.c
  src/platform/golden/golden_mode_identity_source.c src/platform/golden/golden_mode_boot.c
  src/platform/golden/golden_mode_sources.c src/platform/golden/golden_mode_declare_builder.c
  src/platform/golden/golden_mode_cache_publish.c src/platform/golden/golden_mode_declare_cached.c
  src/platform/golden/golden_mode_outputs.c
  src/platform/golden/golden_inheritance.c src/platform/golden/golden_inheritance_source.c
  src/platform/golden/golden_recipe.c src/platform/golden/golden_cmm.c src/platform/golden/golden_cmm_source.c
  src/platform/golden/golden_skill_convert.c src/platform/golden/golden_item_convert.c
  src/platform/golden/golden_names.c src/platform/golden/golden_encounter_convert.c
  src/platform/golden/golden_persona_convert.c src/platform/golden/golden_enemy_convert.c
  src/platform/golden/golden_fusion_convert.c src/platform/golden/golden_fusion_source.c
  src/platform/golden/golden_extra_source.c src/platform/golden/golden_cache.c
  src/platform/golden/golden_table_cache.c)
