/* PS2 RAM for the 32-bit Windows build: 0x00100000..0x02000000 as a zero-filled, writable,
 * executable section of the exe itself.
 *
 * Windows has already used parts of that range for heaps, thread stacks and locale tables before
 * main runs, so a runtime VirtualAlloc there fails (measured: 300 of 300 runs). The kernel maps the
 * exe image first, so an image that owns the range gets it every time. The link puts this section
 * first and the image base 0xF0000 puts its first page at 0x00100000 (CMakeLists.txt:
 * --image-base=0xF0000, --section-alignment=0x10000, /sectionlayout with .ps2ram first, no ASLR,
 * no base relocations). os_win32.c checks the placement at run time. The size is
 * 0x02000000 - 0x00100000. */
	.section .ps2ram,"bwx"
	.globl	_p4_ps2ram
	.p2align 16
_p4_ps2ram:
	.space 0x1F00000
