set(P4_PNG_SOURCES
  "${CMAKE_SOURCE_DIR}/src/viewer/p4png.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tdef.c")
target_sources(p4port PRIVATE ${P4_PNG_SOURCES} src/render/p4gl_atomic.c src/render/p4gl_camera.c src/render/p4gl_context.c src/render/p4gl_device.c src/render/p4gl_gl.c src/render/p4gl_profile.c src/render/p4gl_im2d.c src/render/p4gl_image.c src/render/p4gl_im3d.c src/render/p4gl_mdleffect.c src/render/p4gl_native.c src/render/p4gl_nativetex.c src/render/p4gl_ptank.c src/render/p4gl_raster.c src/render/p4gl_yuv.c src/render/p4gl_readback.c src/render/p4gl_state.c src/render/p4tmx.c)

