# C-only save transaction service; no Lua/renderer code enters this layer.
target_sources(p4port PRIVATE
  src/platform/memcard/mod_save.c src/platform/memcard/mod_save_hash.c
  src/platform/memcard/mod_save_format.c
  src/platform/memcard/save_transaction.c
  src/platform/memcard/mod_save_custody.c
  src/platform/memcard/mod_save_transport.c
  src/platform/memcard/mod_save_native.c)
