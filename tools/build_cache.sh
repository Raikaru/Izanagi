# Shared by the Linux/Windows and Android CI drivers; no admission authority.
p4pc_build_cache_profile() {
  export P4PC_BUILD_PROFILE=$1
  # RenderWare's compiled __DATE__ must not depend on a worker's wall clock.
  export SOURCE_DATE_EPOCH=0
  export P4PC_BUILD_TRACE_DIR=${P4PC_BUILD_TRACE_DIR:-$2}
  mkdir -p "$P4PC_BUILD_TRACE_DIR" || return $?
  export CCACHE_STATSLOG="$P4PC_BUILD_TRACE_DIR/ccache-$1-$$.log"
  if [ -n "${P4PC_BUILD_TOOLCHAIN_ID:-}" ]; then
    export P4PC_BUILD_CACHE_DIR=${P4PC_BUILD_CACHE_DIR:-/ccache/p4pc-generated-v1}
    export CCACHE_COMPILERCHECK=content
    export CCACHE_NAMESPACE="p4pc-${P4PC_BUILD_TOOLCHAIN_ID#sha256:}-$1"
  fi
  echo "P4PC build cache profile=$P4PC_BUILD_PROFILE toolchain=${P4PC_BUILD_TOOLCHAIN_ID:-unavailable} root=${P4PC_BUILD_CACHE_DIR:-disabled} trace=$P4PC_BUILD_TRACE_DIR epoch=$SOURCE_DATE_EPOCH"
}
