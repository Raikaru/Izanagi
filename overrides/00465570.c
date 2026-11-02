#include "p4pc/mod_save_native.h"
/* Historical BeginSave is the full READ path. Only this real new-load intent
 * supersedes a live save; a background/summary read merely receives busy. */
void H_Memcard_BeginSave(s32 file, void *buffer, s32 size)
{
    p4_mc_mod_load_begin();
    D_00764BC0 = 0;
    D_00764BBC = 0;
    D_00764BB8 = file;
    D_00764BAC = buffer;
    D_00764BB0 = size;
    D_00764BA0 = 0;
}
