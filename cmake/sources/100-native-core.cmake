# Port-owned objects: the C start-up/exit replacement (src/crt.c), VU0 helpers, the
# PS2 kernel model (src/platform/ps2_kernel.c: start-up calls, INTC table), the boot
# platform init that replaces the IOP reset (src/platform/kernel_init.c), host
# libc bindings for PS2 library names (src/platform/ps2_libc.c), the malloc family
# in retail EE newlib (src/platform/kernel_heap.c + ee_heap.c), and the file layer: the disc and override
# virtual tree (src/platform/files/vfs.c, iso9660.c) behind the libkernl file calls and
# libcdvd init (sce_file.c, sce_cdvd.c). The memory-card shim
# and the sndcom stubs are src/platform/memcard/ps2_memcard.c and src/platform/sound/ps2_sndcom.c.
add_library(p4port OBJECT
  src/crt.c
  src/image.c
  src/vu0.c
  src/platform/guest.c
  src/platform/ee_float.c
  src/platform/ps2_kernel.c
  src/platform/kernel_init.c
  src/platform/kernel_heap.c
  src/platform/kernel_thread.c
  src/platform/cri_asm_callees.c
  src/platform/rx_heap_realloc.c
  src/platform/ps2_libc.c
  src/platform/ee_bytes.c
  src/platform/ps2_dma.c
)
