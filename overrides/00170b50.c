#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* FldCampTriggerTest, SLUS-217.82 00170b50..00170bbc. The ELF checks
 * datGetFlag(0x37) == 1 before Triangle and writes only eventWork+4 on entry.
 * Keep that gate and its caller's event/battle eligibility: never call this
 * predicate from the host or queue a refused shortcut for a later field. */
s32 FldCampTriggerTest(u8 *task)
{
    u8 *work;
    s32 shortcut;

    work = ((SdkTask *)task)->work;
    if (datGetFlag(0x37) == 1) {
        p4_settings_native_field_request((u32)task, 0);
        return 0;
    }
    shortcut = p4_settings_native_field_request((u32)task, 1);
    if ((D_008C024E[0] & 0x10) || shortcut) {
        ((FldEventWork *)work)->state = 0;
        return 1;
    }
    return 0;
}
