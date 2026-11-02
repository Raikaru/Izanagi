/* Loads the retail SLUS_217.82 image into the PS2 address space and patches
 * its function-pointer data to native code. Shared by p4pc and p4pc_viewer. */
#ifndef P4PC_IMAGE_H
#define P4PC_IMAGE_H

#include <stddef.h>
#include <stdint.h>

/* Reads P4PC_TRAP_CONTINUE and registers the trap report at exit. Call first. */
void p4pc_trap_configure(void);

/* Maps PS2 RAM and the EE register page, loads every PT_LOAD segment of the
 * ELF at its link address, and returns the ELF entry point. Exits on error. */
uint32_t p4pc_load_image(const char *path);

enum { P4_IMAGE_GUEST_WRITE = 1u, P4_IMAGE_GUEST_EXEC = 2u };
typedef struct {
    uint32_t begin, end; /* exclusive end, actual SHF_ALLOC section bounds */
    unsigned flags;
} p4pc_image_guest_range_t;
/* Readonly host metadata for the last loaded image. Missing/malformed section
 * metadata returns NULL and count=0; it never widens access to all guest RAM.
 * The original image-loading behavior does not depend on section metadata.
 * Borrowed until the next load; game-thread only, no guest reads or mutation. */
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count);
/* True only after the latest PT_LOAD copy and input close completed. This is
 * not evidence that a later teardown has kept the image alive. */
int p4pc_image_loaded(void);

/* Stores the native address of each patched function-pointer word. Returns the
 * number of words patched. */
size_t p4pc_apply_patches(void);

/* Zeroes BSS and runs the PS2 kernel init that precedes the game's own init
 * (func_0042bcd8, func_00421a60). Mirrors p4pc_start (src/crt.c) without
 * entering kwlnBoot. */
void p4pc_kernel_pre_init(void);

#endif
