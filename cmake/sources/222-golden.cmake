# Read-only reader for the player's own Persona 4 Golden install and the p4pc:golden
# derived-table producer (docs/modding/golden.md). The producer runs only when a package
# selects it; no Golden bytes are shipped or cached.
target_sources(p4port PRIVATE src/platform/golden/golden_formats.c src/platform/golden/golden_source.c
  src/platform/golden/golden_tables.c src/platform/golden/golden_producer.c)
