target_sources(p4port PRIVATE
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
