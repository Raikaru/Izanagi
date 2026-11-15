# glTF model resources ([resource-model], docs/modding/models.md): the cooker (third_party/cgltf, with the
# LodePNG decoder 285-content-packs builds), its content-hash cache, the package handler and the one
# RenderWare adapter the model loader's override (overrides/0047b0c0.c) calls. All of it is host code
# under src/platform: its scratch memory and diagnostics are the host CRT's, its RenderWare objects come
# from the RW API (the guest heap).
target_sources(p4port PRIVATE
    src/platform/models/p4model_cook.c
    src/platform/models/p4model_cache.c
    src/platform/models/p4model_stream.c
    src/platform/models/p4model_build.c
    src/platform/mods/mod_models_boot.c)
p4_register_package_boot_provider(p4_resources_models_boot_provider)
