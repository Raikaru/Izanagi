/* func_003c8dd0 (224 bytes): refreshes the lights attached to an object. Returns arg0. When
 * func_003c8ce0(arg0) fails nothing else happens. Otherwise each entry of the circular list headed at
 * arg0 + 0x38 refers to a light (entry[8]); a light whose byte +2 has bit 2 set and whose halfword +0x60 differs from the halfword
 * at 0x008872E8 gets func_003bfae0(light) tested against the world with func_003e8200(*(u32 *)0x008872E0,
 * box); a non-zero result calls the light's function at +0x48 with the light. The halfword +0x60 is then
 * set to the one at 0x008872E8. */

extern u8 *p4c_003bfae0_003c8dd0(u8 *obj) __asm__("func_003bfae0");
extern s32 p4c_003c8ce0_003c8dd0(u8 *o) __asm__("func_003c8ce0");
extern s32 p4c_003e8200_003c8dd0(u32 a, u8 *b) __asm__("func_003e8200");

typedef void (*P4LightFn_003c8dd0)(u8 *light);

u8 *func_003c8dd0(u8 *arg0)
{
    u8 *head;
    u8 *entry;

    if (p4c_003c8ce0_003c8dd0(arg0) == 0) {
        return arg0;
    }
    head = arg0 + 0x38;
    entry = *(u8 **)head;
    if (entry == head) {
        return arg0;
    }
    do {
        u8 *light = *(u8 **)(entry + 8);

        if ((*(u8 *)(light + 2) & 4) && *(u16 *)(light + 0x60) != *(u16 *)0x008872E8) {
            u8 *box = p4c_003bfae0_003c8dd0(light);

            if (p4c_003e8200_003c8dd0(*(u32 *)0x008872E0, box) != 0) {
                (*(P4LightFn_003c8dd0 *)(light + 0x48))(light);
            }
            *(u16 *)(light + 0x60) = *(u16 *)0x008872E8;
        }
        entry = *(u8 **)entry;
    } while (entry != head);
    return arg0;
}
