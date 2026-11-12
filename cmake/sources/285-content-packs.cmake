# Native content metadata and capture I/O belong to the platform service;
# renderer identity calculation remains alongside the raster implementation.
target_sources(p4port PRIVATE src/render/p4texture_key.c src/platform/p4texture_dump.c)
target_sources(p4port PRIVATE src/platform/p4content_source.c src/platform/p4content.c)
target_sources(p4port PRIVATE
    src/platform/mods/p4content_image.c
    src/platform/mods/resource_png.c
    src/platform/mods/mod_resources_boot.c
    third_party/lodepng/lodepng.c)
set_source_files_properties(third_party/lodepng/lodepng.c PROPERTIES COMPILE_DEFINITIONS
    "LODEPNG_NO_COMPILE_DISK;LODEPNG_NO_COMPILE_ENCODER;LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS;LODEPNG_NO_COMPILE_ALLOCATORS")
p4_register_package_boot_provider(p4_resources_images_boot_provider)
