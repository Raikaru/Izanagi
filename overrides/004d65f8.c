/* Retail 004d65f8: stores the link switch byte (+0x98) and, when the object has a linked object
 * (+4), tail-calls func_004cec28 with it and the same value (a1 is not touched). */
void adxt_SetLnkSw(int param_1, u32 param_2)
{
    extern void func_004cec28(int object, u32 value);

    *(u8 *)(param_1 + 0x98) = (u8)param_2;
    if (*(int *)(param_1 + 4) != 0) {
        func_004cec28(*(int *)(param_1 + 4), param_2);
    }
}
