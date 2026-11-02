/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Platform names the decomp's re4 CRI units call that have no retail address.
 *
 * re4/sj_crs.c writes the stream-joint critical section with the GameCube names
 * OSDisableInterrupts/OSRestoreInterrupts; adx_fcch.c calls DCInvalidateRange. On the PS2 the
 * critical section is DIntr/EIntr (func_0042ba20/func_0042ba70: DIntr returns whether interrupts
 * were enabled, EIntr enables). The host has coherent caches, so the cache call does nothing.
 */
#include <math.h>
#include <stdint.h>

extern int32_t func_0042ba20(void);
extern int32_t func_0042ba70(void);

int32_t OSDisableInterrupts(void)
{
    return func_0042ba20();
}

int32_t OSRestoreInterrupts(int32_t level)
{
    if (level != 0) {
        func_0042ba70();
    }
    return level;
}

void DCInvalidateRange(void *addr, uint32_t nbytes)
{
    (void)addr;
    (void)nbytes;
}

/* ---- ADXM (retail 0x004cb4xx..0x004cc3a4, the rest is in adxm.c) -----------------------------
 * Names the re4 mwsfd units call. Retail ADXM keeps its thread-setup flag at 0x00719998 (written
 * by the ADXM thread setup/shutdown at 0x004ccff8 and 0x004cd0b8, read by the thread bodies at
 * 0x004cb880 and 0x004cba78). ADXM_WaitVsync (0x004ccc70) arms the handshake word 0x007199ec and
 * sleeps; the server thread wakes the caller on its next pass (0x004ccbf0 clears the word). */
extern void func_004cbd28(void);

int32_t ADXM_IsSetupThrd(void)
{
    return *(volatile int32_t *)P4_G2H(0x00719998);
}

void ADXM_WaitVsync(void)
{
    *(volatile int32_t *)P4_G2H(0x007199ec) = 1;
    func_004cbd28();
}

/* 0x004c54e8 ADX_GetCoefficient: the two ADX predictor coefficients (Q12) of a low-pass at
 * `cutoff` Hz for sample rate `sfreq`. Retail runs this in single precision with its own libm
 * cos (0x0044b610, translated in ee_libm.c: 18 of 20000 coefficient pairs differed by one with the host cosf) and sqrt.s (the launcher's p4pc_ee_sqrt), and
 * with the constant 0x40c90fda: 6.2831855f is 0x40c90fdb, which made 1 pair in 20000 differ. ovdiff -Q checks the pair against the retail code. With
 * a = sqrt(2) - cos(2 pi cutoff / sfreq) and k = sqrt(2) - 1:
 *   b = (a - sqrt((a + k)(a - k))) / k, c1 = 2b * 4096, c2 = -b^2 * 4096. */
extern float p4c_retail_cosf(float x) __asm__("func_0044b610");

void ADX_GetCoefficient(int32_t cutoff, int32_t sfreq, int16_t *c1, int16_t *c2)
{
    float s2 = sqrtf(2.0f);
    float a = s2 - p4c_retail_cosf(((float)cutoff * 0x1.921fb4p+2f) / (float)sfreq);
    float k = s2 - 1.0f;
    float b = (a - sqrtf((a + k) * (a - k))) / k;

    *c1 = (int16_t)(int32_t)((b + b) * 4096.0f);
    *c2 = (int16_t)(int32_t)(-b * b * 4096.0f);
}
