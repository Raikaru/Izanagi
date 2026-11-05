set(P4_PORT_RW_SRCS
  src/platform/ps2_fpbit.c
  src/platform/rw_device_handle.c
  src/platform/rw_frame_register.c
  src/platform/rw_plugin_constructors.c
  src/platform/rw_freelist_thunks.c
  src/platform/rw_device_register.c
  src/platform/gc_compat.c
  src/platform/rw_p2_stalac.c
  src/platform/cdvd_device.c
  src/platform/rw_image_stream.c
)
# A selected RenderWare clean subsystem retires its port transcriptions (030-generation-setup.cmake, config/rw_clean.json).
if(P4_RW_CLEAN_PORT_DROP)
  list(REMOVE_ITEM P4_PORT_RW_SRCS ${P4_RW_CLEAN_PORT_DROP})
endif()
target_sources(p4port PRIVATE ${P4_PORT_RW_SRCS})
