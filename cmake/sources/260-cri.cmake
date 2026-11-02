# CRI middleware functions with no decomp C, translated from the retail asm (one file per library).
file(GLOB P4_CRI_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/cri/*.c")
list(FILTER P4_CRI_SRC EXCLUDE REGEX "/(aixp|acs|cri_glue)\\.c$")
target_sources(p4port PRIVATE ${P4_CRI_SRC})

