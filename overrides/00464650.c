#include "p4pc/mod_save_native.h"
/* Retail calls this after building the COMPLETE version-prefixed save image.
 * Despite its historical BeginLoad name this starts the three-file WRITE. */
void H_Memcard_BeginLoad(s32 file, void *buffer, s32 size)
{
    p4_mc_mod_save_begin(D_00764BA4,file,buffer,size);
    D_00764BC0 = 0;
    D_00764BBC = 0;
    D_00764BB8 = file;
    D_00764BAC = buffer;
    D_00764BB4 = size;
    D_00764BA0 = 0;
}
