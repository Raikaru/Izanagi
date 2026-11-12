# Retail soft-double libm and native-register-class adapters. Integer-only:
# do not pass these through the EE hardware-float IR rewrite.
target_sources(p4port PRIVATE src/platform/ee_heap.c src/platform/ee_dfloat.c src/platform/ee_dlibm.c
  src/platform/ee_decimal.c src/platform/ee_dtoa.c src/platform/ee_printf.c
  src/platform/ee_strtod.c src/platform/ee_scanf.c)
