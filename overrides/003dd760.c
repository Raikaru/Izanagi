/* func_003dd760 (112 bytes): initializes the timer-callback state block at 0x00887180.
 * 0x00887184 = arg0 (the limit); 0x00887188, 0x00887180, 0x00887194 = 0; 0x008871A8 and
 * 0x008871A4 = 1; the gp-0x48B0 slot (0x00764840) = 0; the gp-0x48AC slot (0x00764844) is set to
 * func_004217e0(0x008871A0) (creates the object described by that block); 0x0088718C = 1.
 * gp is 0x007690F0. */

void func_003dd760(s32 arg0)
{
    *(s32 *)0x00887184 = arg0;
    *(s32 *)0x00887188 = 0;
    *(s32 *)0x00887180 = 0;
    *(s32 *)0x00887194 = 0;
    *(s32 *)0x008871A8 = 1;
    *(s32 *)0x00764840 = 0;
    *(s32 *)0x008871A4 = 1;
    *(s32 *)0x00764844 = func_004217e0((u8 *)0x008871A0);
    *(s32 *)0x0088718C = 1;
}
