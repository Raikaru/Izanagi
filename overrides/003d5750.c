/* func_003d5750 (64 bytes): counts the elements of a circular list. arg0[0][2] is the element size in bytes and arg0[4]
 * the list head; an empty list (the head's first word is the head itself) gives 0, otherwise the head pointer is advanced by the
 * element size (note: not by the link) until the word at it equals the head again, and the number of steps is returned. */

s32 func_003d5750(u32 *arg0)
{
    u32 *head = (u32 *)arg0[4];
    u32 stride = ((u32 *)arg0[0])[2];
    u8 *p;
    s32 n = 0;

    if ((u32 *)*head == head) {
        return 0;
    }
    p = (u8 *)head;
    do {
        p += stride;
        n++;
    } while (*(u32 **)p != head);
    return n;
}
