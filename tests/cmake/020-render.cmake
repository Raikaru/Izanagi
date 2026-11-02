# Stable content identity and asynchronous original-only dumps use synthetic bytes.
add_executable(texture_key_test texture_key_test.c
  "${CMAKE_SOURCE_DIR}/src/render/p4texture_key.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(texture_key_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME texture_key COMMAND texture_key_test)
add_executable(content_source_test content_source_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/p4content_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c")
target_include_directories(content_source_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render")
add_test(NAME content_source COMMAND content_source_test)
add_executable(content_mips_test content_mips_test.c
  "${CMAKE_SOURCE_DIR}/src/render/p4content_mips.c")
target_include_directories(content_mips_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render")
add_test(NAME content_mips COMMAND content_mips_test)
add_executable(content_image_test content_image_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/p4content_image.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tdef.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c")
target_include_directories(content_image_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render")
add_test(NAME content_image COMMAND content_image_test)
add_executable(texture_dump_test texture_dump_test.c
  "${CMAKE_SOURCE_DIR}/src/render/p4texture_key.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/p4texture_dump.c" ${P4_PNG_SOURCES} "${P4_OS_SRC}")
target_include_directories(texture_dump_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/viewer")
target_compile_definitions(texture_dump_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(texture_dump_test ${P4_SDL3_LIB})
add_test(NAME texture_dump COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/texture_dump_test.py"
  ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:texture_dump_test>)

# Same renderer/heap as the player: disabled and dump-enabled pixels/guest
# records must be identical, and palette-only changes must acquire a new key.
add_executable(texture_capture_test texture_capture_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(texture_capture_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render")
target_compile_options(texture_capture_test PRIVATE -fno-builtin)
target_compile_definitions(texture_capture_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(texture_capture_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(texture_capture_test)
add_test(NAME texture_capture COMMAND texture_capture_test "${CMAKE_BINARY_DIR}/texture_capture_dump")
set_tests_properties(texture_capture PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 120
  ENVIRONMENT "P4PC_CONFIG=none;P4PC_INPUT=script;P4PC_TURBO=0;P4PC_MODS=0;P4PC_PACKS=0;P4PC_TEXTURE_DUMP=")

# Portable capture encoder: exact RGBA (including invisible RGB), real compression
# and streamed multi-IDAT output, without a GL context or retail inputs.
add_executable(png_writer_test png_writer_test.c ${P4_PNG_SOURCES}
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c")
target_include_directories(png_writer_test PRIVATE "${CMAKE_SOURCE_DIR}/src/viewer")
target_compile_definitions(png_writer_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME png_writer COMMAND png_writer_test)

# OpenGL 3.3 device path, offscreen: a retail TMX through RASTERCREATE/LOCK/UNLOCK, an Im2D quad
# into a readback FBO, a pixel check and a PNG. Links the same objects as p4pc_viewer. Skipped
# (exit 77) unless P4PC_ISO names the image. The PNG lands in the build directory.
add_executable(render_smoke render_smoke.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(render_smoke PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/viewer" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(render_smoke PRIVATE -fno-builtin)
target_compile_definitions(render_smoke PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(render_smoke m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(render_smoke)
add_test(NAME render_smoke COMMAND render_smoke "${CMAKE_BINARY_DIR}/render_smoke.png")
# Tests that open an OpenGL context carry the label `gl`: the Windows check under Wine has no host GL
# and runs `ctest -LE gl`.
set_tests_properties(render_smoke PROPERTIES SKIP_RETURN_CODE 77 LABELS gl)
# The same smoke test on the other GL paths (render_smoke above runs the default one, set by -DP4PC_GLES): desktop OpenGL 3.3 core, and
# OpenGL ES 3.0 with Mesa held to ES 3.0 and the texture border emulated in the fragment shader, which is what ANGLE's Metal backend
# gives (it has no GL_CLAMP_TO_BORDER). The border cases in render_smoke compare every probe with a model of the sampler.
add_test(NAME render_smoke_gl33 COMMAND "${CMAKE_COMMAND}" -E env P4PC_GL_API=gl $<TARGET_FILE:render_smoke> "${CMAKE_BINARY_DIR}/render_smoke_gl33.png")
add_test(NAME render_smoke_es30 COMMAND "${CMAKE_COMMAND}" -E env P4PC_GL_API=gles MESA_GLES_VERSION_OVERRIDE=3.0 P4PC_GLES_BORDER=shader
  $<TARGET_FILE:render_smoke> "${CMAKE_BINARY_DIR}/render_smoke_es30.png")
set_tests_properties(render_smoke_gl33 render_smoke_es30 PROPERTIES SKIP_RETURN_CODE 77 LABELS gl)
add_test(NAME renderer_reference_scope COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer_scope_test.py"
  $<TARGET_FILE:render_smoke> "${CMAKE_BINARY_DIR}/renderer_reference_scope.png")
set_tests_properties(renderer_reference_scope PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 90)


# Exercise the real Config task, native widgets/input, live resolution and
# rebinding; the automatic full-game twin also covers the 64-bit executable.
add_test(NAME settings_native COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/settings_native_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(settings_native PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
add_test(NAME settings_pages COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/settings_pages_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(settings_pages PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
add_test(NAME settings_input COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/settings_input_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(settings_input PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 1200)

# Preserve the renderer/provider's real allocation and persistence boundaries
# without keeping the retired standalone UI canvas alive.
add_executable(settings_render settings_render_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(settings_render PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render")
target_compile_definitions(settings_render PRIVATE ${P4_PORT_DEFINES})
target_compile_options(settings_render PRIVATE -fno-builtin)
target_link_libraries(settings_render m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(settings_render)
add_test(NAME settings_render COMMAND settings_render "${CMAKE_BINARY_DIR}/settings_render")
set_tests_properties(settings_render PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 120)

# The sky driver's VU0 matrix and vector multiplies the GL device installs into RenderWare's engine slots (src/platform/rw_skytran.c),
# production objects against the unchanged retail instructions in the R5900 interpreter, 2000 deterministic cases each: every guest byte
# and every VU0 register the routine leaves equal (tests/sky_vector_vu0_test.c).
add_executable(sky_vector_vu0_test sky_vector_vu0_test.c "${P4_TABLES}"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(sky_vector_vu0_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_compile_options(sky_vector_vu0_test PRIVATE -fno-builtin)
target_compile_definitions(sky_vector_vu0_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(sky_vector_vu0_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(sky_vector_vu0_test)
add_test(NAME sky_vector_vu0 COMMAND sky_vector_vu0_test)
set_tests_properties(sky_vector_vu0 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
