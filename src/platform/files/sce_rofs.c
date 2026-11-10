/* ROFS front-end entry points on the boot path (retail libsce ROFS: src/sce/rofs_mai.c).
 *
 * The ROFS driver reads the disc itself (its 51 internal functions are replaced by the host VFS,
 * src/platform/files/vfs.c; see docs/asm_worklist.md). Two of its entry points are called directly
 * by kwlnInitSubsystems and are modelled here:
 *
 *   func_004f34b0 (ROFS_Init(config)): retail stores the driver's global state pointer (0x00743A50),
 *     runs the library set-up (func_004f1f80 with the config block {6, 4, 0x3D2, work buffer}) and
 *     records its result in 0x00925EA0. There is no driver to set up on the host: the VFS is open
 *     from p4pc_platform_init, so this returns 0 (success).
 *   func_004f5bf0 (ROFS_SetOption(handle, option, value)): option 0 sets (1) or clears (0) the
 *     driver flag 0x400, refusing to clear it when already set (-100, the retail bad-argument code);
 *     option 1 sets or clears flag 0x01; option 2 flag 0x02; any other option returns -100.
 *     The flags (retail: word at state + 0x18) live in a port-owned word because the state block
 *     does not exist on the host; nothing reads them back.
 *
 * Direct transliteration of the retail option logic. */
#include <stdint.h>

static uint32_t sRofsFlags;

int32_t func_004f34b0(void *config)
{
    (void)config;
    return 0;
}

int32_t func_004f5bf0(int32_t handle, int32_t option, int32_t value)
{
    (void)handle;
    if (option == 1) {
        if (value == 1)
            sRofsFlags |= 1;
        else
            sRofsFlags &= ~1u;
        return 0;
    }
    if (option == 0) {
        if (value == 0) {
            if (sRofsFlags & 0x400)
                return -100;
            sRofsFlags &= ~0x400u;
        }
        if (value != 1)
            return 0;
        sRofsFlags |= 0x400;
        return 0;
    }
    if (option != 2)
        return -100;
    if (value == 1)
        sRofsFlags |= 2;
    else
        sRofsFlags &= ~2u;
    return 0;
}
