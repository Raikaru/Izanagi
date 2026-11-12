/* func_003d6c50 (rpuvanim, 248 bytes): fills an 0x40-byte object from src
 * (arg1), then runs three calls on it. The object gets src[3], src[5], 0,
 * 0, src[4], 0, 0, 0, 0, src[6], src[7], 0 at word offsets 0, 1, 2, 4, 5,
 * 6, 8, 9, 10, 12, 13, 14 (word 3 is not written). Then func_003e05d0(obj),
 * func_003e0c90(obj, retail 0x752F88, 2), func_003e0870(obj, axis, 2, angle)
 * with axis = {0, 0, 1} and angle = 180 * (src[2] / pi) in degrees, and
 * func_003e0c90(obj, retail 0x752F98, 2). */

void func_003e05d0(void *matrix);
void func_003e0c90(void *matrix, const void *vec, s32 mode);
void func_003e0870(void *matrix, const void *axis, s32 mode, float angle);

void func_003d6c50(u8 *arg0, u8 *arg1)
{
    float *obj = (float *)arg0;
    u32 *iobj = (u32 *)arg0;
    float *src = (float *)arg1;
    float axis[3] __attribute__((aligned(16)));
    float angle;

    axis[0] = 0.0f;
    axis[1] = 0.0f;
    axis[2] = 1.0f;

    obj[0] = src[3];
    obj[1] = src[5];
    iobj[2] = 0;
    iobj[4] = 0;
    obj[5] = src[4];
    iobj[6] = 0;
    iobj[8] = 0;
    iobj[9] = 0;
    iobj[10] = 0;
    obj[12] = src[6];
    obj[13] = src[7];
    iobj[14] = 0;

    func_003e05d0(arg0);
    func_003e0c90(arg0, (const void *)0x00752F88, 2);
    angle = 180.0f * (src[2] / 3.14159274f);
    func_003e0870(arg0, axis, 2, angle);
    func_003e0c90(arg0, (const void *)0x00752F98, 2);
}
