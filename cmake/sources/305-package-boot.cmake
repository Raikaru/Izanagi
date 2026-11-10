# All provider-owner fragments precede this point. Explicit C references keep
# registration independent of constructors, static archive discovery and OS ABI.
p4_register_package_boot_provider(p4_package_tables_completion_boot_provider)
get_property(_p4_boot_providers GLOBAL PROPERTY P4_PACKAGE_BOOT_PROVIDERS)
set(_p4_boot_source "#include \"p4pc/packages.h\"\n#include \"p4pc/package_boot.h\"\n")
foreach(_p4_provider IN LISTS _p4_boot_providers)
  string(APPEND _p4_boot_source "extern const P4PackageHandler ${_p4_provider};\n")
endforeach()
string(APPEND _p4_boot_source "static const P4PackageHandler *const providers[] = {\n")
foreach(_p4_provider IN LISTS _p4_boot_providers)
  string(APPEND _p4_boot_source "    &${_p4_provider},\n")
endforeach()
string(APPEND _p4_boot_source "    0\n};\nint p4_package_boot_install(void)\n{\n    for (size_t i = 0; providers[i]; ++i)\n        if (p4_package_handler_register(providers[i])) return -1;\n    p4_package_boot_installed();\n    return 0;\n}\n")
file(GENERATE OUTPUT "${P4_GEN}/package_boot_providers.c" CONTENT "${_p4_boot_source}")
target_sources(p4port PRIVATE "${P4_GEN}/package_boot_providers.c"
  src/platform/mods/package_boot.c src/platform/mods/failure_native.c
  src/platform/mods/failure_config.c src/platform/mods/boot_health.c)
unset(_p4_boot_source)
unset(_p4_boot_providers)
unset(_p4_provider)
