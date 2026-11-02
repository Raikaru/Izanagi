/* func_003bb5b0 (1248 bytes): evaluates a cubic spline segment list at the time arg2 (declared third, as the decomp callers pass it; the ABI puts it in f12) and returns the position arg3 (0 after reporting an error). arg0 is the curve
 * ({+8 = number of knots n, +0xC = loop mode, +0x38 = n knots of 12 bytes}). The time is made positive; for loop mode 2 and a time above 1 its integer part is removed
 * (the unsigned truncation of the retail code). arg1 selects the easing: 0xB (ease in/out: t * 2t up to 0.5, else 4t - 2t^2 - 1) or 0xA (none); any other value reports
 * {0x102, func_003df590(1)} to func_003df4d0. The time 1.0 selects the last segment at table position 255, otherwise the segment is i = trunc((n - 3) * t) and the fraction
 * is split into the table index j = trunc(256 * f) and the remainder s. Four weight tables (words at 0x00764718 / 0x00764714 / 0x00764710 / 0x0076470C, gp-0x49D8..gp-0x49E4; gp is 0x007690F0)
 * give the basis weights w = T[j] + s * (T[j + 1] - T[j]) and arg2 = sum of knot[i + k] * w_k, accumulated in the order of the FPU's accumulator. When arg3 is not null the tangent
 * is made the same way from the tables at 0x00764728 / 0x00764724 / 0x00764720 / 0x0076471C (gp-0x49C8..gp-0x49D4) and func_003e4030 is called on it; a zero length replaces it by (0, 1, 0). */

extern void p4c_003df4d0_003bb5b0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003bb5b0(s32 code, ...) __asm__("func_003df590");
extern f32 p4c_003e4030_003bb5b0(f32 *out, f32 *in) __asm__("func_003e4030");

f32 *func_003bb5b0(u8 *arg0, s32 arg1, f32 arg4, void *a2, void *a3)
{
    f32 *arg2 = (f32 *)a2;
    f32 *arg3 = (f32 *)a3;
    s32 n = *(s32 *)(arg0 + 8);
    f32 *keys = (f32 *)(arg0 + 0x38);
    f32 t = arg4;
    s32 seg;
    s32 idx;
    f32 s;
    f32 f;
    f32 g;
    f32 *k;
    f32 *ta;
    f32 *tb;
    f32 *tc;
    f32 *td;
    f32 wa;
    f32 wb;
    f32 wc;
    f32 wd;
    f32 len;
    s32 err[2];
    u32 ui;
    s32 i;

    if (t < 0.0f) {
        t = -t;
    }
    if (*(s32 *)(arg0 + 0xC) == 2) {
        if (!(t <= 1.0f)) {
            if (!(2147483648.0f <= t)) {
                ui = (u32)(s32)t;
            } else {
                ui = (u32)(s32)(t - 2147483648.0f) | 0x80000000u;
            }
            if ((s32)ui >= 0) {
                f = (f32)(s32)ui;
            } else {
                f = (f32)(s32)((ui >> 1) | (ui & 1));
                f = f + f;
            }
            t = t - f;
        }
    }
    if (arg1 == 0xB) {
        if (t <= 0.5f) {
            f = t + t;
            t = t * f;
        } else {
            f = -2.0f * t;
            g = 0.0f + -1.0f;
            g = g + f * t;
            t = g + 4.0f * t;
        }
    } else if (arg1 != 0xA) {
        err[0] = 0x102;
        err[1] = p4c_003df590_003bb5b0(1);
        p4c_003df4d0_003bb5b0(err);
        return (f32 *)0;
    }
    if (t == 1.0f) {
        seg = n - 4;
        idx = 0xFF;
        s = 1.0f;
    } else {
        f = (f32)(n - 3);
        f = f * t;
        seg = (s32)f;
        g = (f32)seg;
        g = f - g;
        g = 256.0f * g;
        idx = (s32)g;
        s = g - (f32)idx;
    }
    k = keys + seg * 3;
    ta = (f32 *)*(u32 *)0x00764718 + idx;
    tb = (f32 *)*(u32 *)0x00764714 + idx;
    tc = (f32 *)*(u32 *)0x00764710 + idx;
    td = (f32 *)*(u32 *)0x0076470C + idx;
    wa = (0.0f + ta[0]) + s * (ta[1] - ta[0]);
    wb = (0.0f + tb[0]) + s * (tb[1] - tb[0]);
    wc = (0.0f + tc[0]) + s * (tc[1] - tc[0]);
    wd = (0.0f + td[0]) + s * (td[1] - td[0]);
    for (i = 0; i < 3; i++) {
        arg2[i] = k[i] * wa;
        arg2[i] = (0.0f + arg2[i]) + k[3 + i] * wb;
        arg2[i] = (0.0f + arg2[i]) + k[6 + i] * wc;
        arg2[i] = (0.0f + arg2[i]) + k[9 + i] * wd;
    }
    if (arg3 == (f32 *)0) {
        return arg2;
    }
    ta = (f32 *)*(u32 *)0x00764728 + idx;
    tb = (f32 *)*(u32 *)0x00764724 + idx;
    tc = (f32 *)*(u32 *)0x00764720 + idx;
    td = (f32 *)*(u32 *)0x0076471C + idx;
    wa = (0.0f + ta[0]) + s * (ta[1] - ta[0]);
    wb = (0.0f + tb[0]) + s * (tb[1] - tb[0]);
    wc = (0.0f + tc[0]) + s * (tc[1] - tc[0]);
    wd = (0.0f + td[0]) + s * (td[1] - td[0]);
    for (i = 0; i < 3; i++) {
        arg3[i] = k[i] * wa;
        arg3[i] = (0.0f + arg3[i]) + k[3 + i] * wb;
        arg3[i] = (0.0f + arg3[i]) + k[6 + i] * wc;
        arg3[i] = (0.0f + arg3[i]) + k[9 + i] * wd;
    }
    len = p4c_003e4030_003bb5b0(arg3, arg3);
    if (len == 0.0f) {
        arg3[0] = 0.0f;
        *(u32 *)(arg3 + 1) = 0x3F800000;
        arg3[2] = 0.0f;
    }
    return arg2;
}

/* The one caller that passes the time last (FldEventDrawMarker, k_event_inferred.c) reaches the function through this wrapper (config/mirror_fixes.txt). */
void p4_003bb5b0_eventorder(u8 *arg0, s32 arg1, void *arg2, void *arg3, f32 arg4)
{
    func_003bb5b0(arg0, arg1, arg4, arg2, arg3);
}
