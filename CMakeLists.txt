cmake_minimum_required(VERSION 3.20)
# macOS only (ignored elsewhere, and read by project()): the oldest system the build runs on, that of the ANGLE it ships
# (ci/macos/build_deps.sh). docs/platform/macos.md.
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
project(p4pc C)

# Numbered owner fragments retain the original command and source order.
# Repeated domains are intentional; keep new work in its owning fragment.
file(GLOB _p4_source_modules CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/cmake/sources/*.cmake")
list(SORT _p4_source_modules)
unset(_p4_source_stop)
foreach(_p4_source_module IN LISTS _p4_source_modules)
  include("${_p4_source_module}" NO_POLICY_SCOPE)
  # The original Android exits must end the project, not only an include.
  if(_p4_source_stop)
    break()
  endif()
endforeach()
unset(_p4_source_stop)
unset(_p4_source_module)
unset(_p4_source_modules)
