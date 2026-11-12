# The retail float libm cores (newlib, EE-GCC), translated by tools/mips2c.py into src/platform/ee_libm.c.
# window <from> <to>: data the code reads (constants and tables), copied from the retail image.
# func <addr> <ret> <args> <end> [name]
window 0x755098 0x7550a0
window 0x755b90 0x755ee8
window 0x7560f8 0x7563e8

func 0x00441f40 f ff 0x00441f68
func 0x00441f68 f fi 0x00442018
func 0x0044a138 i fp 0x0044a4d0
func 0x0044a4d0 f f 0x0044a5b0
func 0x0044a5b0 f ff 0x0044a700
func 0x0044a700 i ppiiip 0x0044af20
func 0x0044af20 f ffi 0x0044b018
func 0x0044b018 f ffi 0x0044b310
func 0x0044b340 f f 0x0044b610
func 0x0044b610 f f 0x0044b6c0
func 0x0044b6c0 f f 0x0044b6d8
func 0x0044b6d8 f f 0x0044b7b0
func 0x0044b7b0 f f 0x0044b868
func 0x0044b868 f f 0x0044b8d8
func 0x00448d10 f f 0x00449120
func 0x00449120 f f 0x004494b8
func 0x004494b8 f ff 0x00449680
func 0x00449680 f f 0x00449938
func 0x00449938 f ff 0x0044a138
