# Actual boot hardware seams, compiled as native 64-bit sparse-memory witnesses.
add_test(NAME boot_seams COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/boot_seams_test.py")
set_tests_properties(boot_seams PROPERTIES LABELS port64)

# Device-free regression for debug-only restart marker and adb PTY handling.
add_test(NAME android_restart_driver COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/android_restart_test.py")
set_tests_properties(android_restart_driver PROPERTIES LABELS port64)

# Exercise the Android overlay's inset and view-origin geometry on a host JDK.
add_test(NAME android_pad_layout COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/android_pad_layout_test.py")
set_tests_properties(android_pad_layout PROPERTIES LABELS port64)

