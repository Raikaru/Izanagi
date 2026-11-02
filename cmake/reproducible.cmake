include_guard(GLOBAL)

# Select at the call site, after CMake has identified the actual driver. The
# GNU MinGW driver and the MSVC-style lld-link driver spell this differently.
function(p4_pe_reproducible_link_flags output)
  if(MSVC)
    set(${output} "LINKER:/Brepro" PARENT_SCOPE)
  else()
    # BFD's automatic DLL image base hashes its output path; use the reviewed
    # LLVM linker instead of making identical DLLs depend on the build directory.
    set(${output} -fuse-ld=lld -Wl,--no-insert-timestamp PARENT_SCOPE)
  endif()
endfunction()
