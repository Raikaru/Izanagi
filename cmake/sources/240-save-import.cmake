target_sources(p4port PRIVATE
  src/platform/memcard/save_import.c src/platform/memcard/import_parse.c
  src/platform/memcard/save_image.c
  src/platform/memcard/import_card.c src/platform/memcard/import_entry.c
  src/platform/memcard/import_max.c
  src/platform/memcard/import_settings.c
  src/platform/memcard/import_cbs.c third_party/miniz/miniz_tinfl.c)
