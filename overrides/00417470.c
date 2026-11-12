/* func_00417470: builds the 12-word plugin record (id 2, size 36, eight callback pointers,
 * trailing size 24) and registers it through func_003d5000. The callbacks are the RtAnim
 * interpolator functions for this animation type: apply 415ad0, blend 4165a0, interpolate 415bc0,
 * add 417310, mulrecip 416e10, stream read 416bc0, stream write 416cf0, stream size 416df0. They
 * are taken by symbol, so the record holds the host function addresses (native or trap stub); the
 * retail addresses would be PS2 code. Returns 1 when the registration succeeded, 0 otherwise. */
extern void p4c_415ad0(void) __asm__("func_00415ad0");
extern void p4c_4165a0(void) __asm__("func_004165a0");
extern void p4c_415bc0(void) __asm__("func_00415bc0");
extern void p4c_417310(void) __asm__("func_00417310");
extern void p4c_416e10(void) __asm__("func_00416e10");
extern void p4c_416bc0(void) __asm__("func_00416bc0");
extern void p4c_416cf0(void) __asm__("func_00416cf0");
extern void p4c_416df0(void) __asm__("func_00416df0");

s32 func_00417470(void)
{
    u32 rec[12];

    rec[0] = 2;
    rec[1] = 36;
    rec[2] = 24;
    rec[3] = (u32)(uintptr_t)p4c_415ad0;
    rec[4] = (u32)(uintptr_t)p4c_4165a0;
    rec[5] = (u32)(uintptr_t)p4c_415bc0;
    rec[6] = (u32)(uintptr_t)p4c_417310;
    rec[7] = (u32)(uintptr_t)p4c_416e10;
    rec[8] = (u32)(uintptr_t)p4c_416bc0;
    rec[9] = (u32)(uintptr_t)p4c_416cf0;
    rec[10] = (u32)(uintptr_t)p4c_416df0;
    rec[11] = 24;

    return func_003d5000(rec) != 0;
}
