# Hand-written natives that translate retail functions with float arithmetic (no decomp C): compiled through tools/ee_float_cc.py like the decomp mirror, so their
# add/sub/div/sqrt are the EE's (include/p4pc/eefloat.h). The libm cores are the retail fdlibm code translated from the asm (src/platform/ee_libm.c). Host seams and
# the helpers themselves (ee_float.c) stay in p4port: a rewritten helper would call itself.
set(P4_PORTEE_SRCS src/cri/aixp.c src/cri/acs.c src/cri/cri_glue.c src/platform/rw_skin_callees.c src/platform/rw_rtanim.c src/platform/rw_rtanim_record.c
  src/platform/rw_normalize2.c src/platform/rw_normalize3.c src/platform/rw_skytran.c src/platform/ee_libm.c)
# A selected RenderWare clean subsystem retires its port transcriptions (030-generation-setup.cmake, config/rw_clean.json).
if(P4_RW_CLEAN_PORT_DROP)
  list(REMOVE_ITEM P4_PORTEE_SRCS ${P4_RW_CLEAN_PORT_DROP})
endif()
add_library(p4portee OBJECT ${P4_PORTEE_SRCS})
target_compile_definitions(p4portee PRIVATE ${P4_PORT_DEFINES})
target_include_directories(p4portee PRIVATE src/movie src/render src/platform/files "${P4_FFMPEG_INC}" "${CMAKE_SOURCE_DIR}/third_party")
if(P4_EE_FLOAT)
  set_target_properties(p4portee PROPERTIES C_COMPILER_LAUNCHER "${P4_PYTHON};${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py;--modes=${P4_EE_FLOAT_OPS}")
endif()

target_include_directories(p4port PRIVATE "${CMAKE_SOURCE_DIR}/third_party")


