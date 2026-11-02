/* Port replacement for the PS2 start-up path and the exit glue.
 *
 * asm/code1.s `_start` (PS2 entry 0x100008) sets gp, makes two kernel thread
 * calls (syscalls 0x3C and 0x3D: gp, stack, argc/argv), zeroes BSS
 * 0x764280..0x938a00, calls func_0042bcd8 and func_00421a60(0), enables
 * interrupts, calls the game's boot entry kwlnBoot (retail func_001002c0), then
 * tail-jumps to the exit glue func_0043dc60 with its return value.
 *
 * The port keeps what changes game state: BSS zeroing, the two init calls, and
 * the boot call to kwlnBoot. The kernel calls are dropped:
 * gp is an absolute constant in the generated tables, the stack is the native
 * one. There are no PS2 interrupts to
 * enable, so `ei` is dropped too.
 *
 * The exit glue reads a struct pointer at 0x710070. If that points into the
 * image, the PS2 code walks an atexit list at +0x148 and calls a hook at +0x3c
 * before exiting. The port runs neither. A non-empty list or hook is a trap,
 * so unported exit work shows up in the trap log instead of being skipped. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/crt.h"
#include "p4pc/os.h"
#include "p4pc/tables.h"
#include "p4pc/lockstep.h"

#define IMAGE_LO 0x00100000u
#define IMAGE_HI 0x00938a00u /* end of the retail image; bss is inside it */
#define BSS_LO 0x00764280u
#define GLUE_PTR 0x00710070u /* holds the exit glue's struct pointer */
#define GLUE_ATEXIT 0x148u
#define GLUE_HOOK 0x3cu
#define EXIT_GLUE 0x0043dc60u

/* Decomp functions. Native when compiled, otherwise generated trap stubs. */
void func_0042bcd8(void);
void func_00421a60(int);
int kwlnBoot(void);

static void (*normal_exit_observer)(void);
int p4pc_on_normal_exit(void (*callback)(void))
{
    if(!callback||(normal_exit_observer&&normal_exit_observer!=callback))return -1;
    normal_exit_observer=callback;
    return 0;
}
static void notify_normal_exit(void)
{
    if(normal_exit_observer)normal_exit_observer();
}

static uint32_t image_word(uint32_t va) {
    uint32_t v;
    memcpy(&v, P4_G2H(va), sizeof v);
    return v;
}

/* The exit glue: exit(code), after checking that the PS2 exit work it would
 * have done (atexit list, exit hook) is empty. */
void func_0043dc60(int code) {
    uint32_t glue = image_word(GLUE_PTR);
    if (glue != 0) {
        if (glue < IMAGE_LO || (uint64_t)glue + GLUE_ATEXIT + 4 > IMAGE_HI) {
            p4pc_trap("func_0043dc60 (exit glue struct outside the image)", EXIT_GLUE,
                      __builtin_return_address(0));
        } else if (image_word(glue + GLUE_ATEXIT) != 0) {
            p4pc_trap("func_0043dc60 (atexit list not run)", EXIT_GLUE, __builtin_return_address(0));
        } else if (image_word(glue + GLUE_HOOK) != 0) {
            p4pc_trap("func_0043dc60 (exit hook not run)", EXIT_GLUE, __builtin_return_address(0));
        }
    }
    fprintf(stderr, "p4pc: game exited with code %d\n", code);
    if(code==0)notify_normal_exit();
    exit(code);
}

void p4pc_kernel_pre_init(void) {
    /* Game code runs with the EE's rounding (include/p4pc/hostmath.h). */
    if (p4pc_fpu_ee_mode()) {
#if defined(__aarch64__)
        fprintf(stderr, "p4pc: EE float mode (FPCR round toward zero, FZ)\n");
#else
        fprintf(stderr, "p4pc: EE float mode (MXCSR round toward zero, FTZ, DAZ)\n");
#endif
    }
    if (getenv("P4PC_FPU_LOG") != NULL) {
        p4pc_fpu_state_log("in pre-init");
    }
    memset(P4_G2H(BSS_LO), 0, IMAGE_HI - BSS_LO);
    func_0042bcd8();
    func_00421a60(0);
}

static void game_main(void) {
    p4pc_kernel_pre_init();
    p4pc_lockstep_init();
    func_0043dc60(kwlnBoot());
}

#ifdef P4_GUEST64
/* 64-bit host: the game runs on a stack in guest memory (guest.h), so the address of a local is a guest address. The boot thread's own
 * stack only hosts main() until the switch. The game never returns here: its exit glue calls exit(). */
#define BOOT_STACK_SIZE (4u << 20)

static void game_entry(void) {
    game_main();
    notify_normal_exit();
    exit(0);
}

void p4pc_start(int argc, char **argv) {
    /* kwlnBoot takes no arguments; argc and argv are not used by the boot path. */
    (void)argc;
    (void)argv;
    void *stack = p4_guest_stack_alloc(BOOT_STACK_SIZE);
    if (stack == NULL) {
        fprintf(stderr, "p4pc: no guest memory for the boot stack\n");
        exit(2);
    }
    p4_coro *boot = p4_coro_adopt();
    p4_coro *game = p4_coro_create(stack, BOOT_STACK_SIZE, game_entry);
    p4_coro_switch(boot, game);
}
#else
void p4pc_start(int argc, char **argv) {
    (void)argc;
    (void)argv;
    game_main();
    notify_normal_exit();
}
#endif
