#include "p4pc/vu0.h"

u8 *effBlurFilterDirectCreate(u32 type, const void *initialData)
{
    s32 (*initialize)(const void *);
    s32 kind;
    u8 *allocated;
    s32 dataSize;
    u8 *data;
    u8 *object;
    s32 headerBytes = 0x60;

    kind = type & 0xFFFF;
    dataSize = *(s32 *)(D_00714474 + kind * 0x18);
    if ((u32)dataSize + 0x60 >= 0x200U)
        kAssertFail(D_00714450, 0x170);
    chkmemAllocSiteTag(D_00714450, 0x171);
    allocated = (u8 *)(*jtbl_008873E8)(dataSize + 0x60, 0x40000);
    object = allocated;
    memset(object, 0, dataSize + 0x60);
    if (object == (u8 *)0)
        kAssertFail(D_00714380, 0x706);
    *(s32 *)(object + 0x18) = kind;
    *(s32 *)(object + 0x10) = -1;
    *(s32 *)(object + 0x14) = 0x3F800000;
    *(s32 *)(object + 0x28) = 0;
    p4_vu0_sqc2(object, 0);
    data = object + headerBytes;
    *(u8 **)(object + 0x24) = data;
    memcpy(data, initialData, dataSize);
    initialize = *(s32 (**)(const void *))(D_00714460 + ((u16)type * 0x18));
    if (initialize != (s32 (*)(const void *))0)
        *(s32 *)(object + 0x20) = initialize(initialData);
    *(s16 *)(object + 0x44) = 0x17;
    *(u8 **)(object + 0x48) = object;
    return object;
}
