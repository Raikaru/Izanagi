# Native providers register process-lifetime C descriptors, never constructors.
# Definitions belong to their source owner fragments before305 finalization.
function(p4_register_package_boot_provider symbol)
  if(NOT "${symbol}" MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
    message(FATAL_ERROR "Invalid native package boot provider symbol: ${symbol}")
  endif()
  get_property(_providers GLOBAL PROPERTY P4_PACKAGE_BOOT_PROVIDERS)
  if("${symbol}" IN_LIST _providers)
    message(FATAL_ERROR "Native package boot provider registered twice: ${symbol}")
  endif()
  set_property(GLOBAL APPEND PROPERTY P4_PACKAGE_BOOT_PROVIDERS "${symbol}")
endfunction()
