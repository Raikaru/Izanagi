/* func_00399530 (1128 bytes): builds the PS2-format texture object for the material texture arg1 (arg1[0] = the source texture, arg1[0x50 / 4] its flag word) and
 * returns the new object. arg0 is an optional second texture (same layout) that takes over the size, raster and flags; when it is null a 32-bit raster of the source size is
 * filled with 0xFFFFFFFF instead. Steps: a raster of the source size is made (func_003ea2c0(w, h, 0x20)), locked (func_003ea3e0) and loaded from the source (func_003ec110); the second raster
 * likewise (arg0's texture, or the filled one); a size mismatch makes a new raster of the second size and func_003ed0b0(new, first) / func_003ea370(first) replace the first. func_003ea560(first) and
 * func_003ea690(second, first) combine them, func_003ec1f0(second, 4, &a, &b, &c, &d) reads the format words (d |= 0x9000 when bit 15 of byte 0x23 << 8 of the used texture is set),
 * func_003ec590(a, b, c, d) makes the texture, func_003ec180(tex, second) binds the raster and func_003ef2e0(tex) wraps it. The flag word of the object (+0x50) gets the nibble
 * (flag & 0xF000) >> 12 when it equals (flag & 0xF00) >> 8 (else 0) in bits 12 to 15 and 8 to 11, and the low byte of the flag word; both rasters are destroyed (func_003ea370), a zeroed 0x20-byte
 * record is filled by func_00399470(record, arg0, arg1) and attached with func_003ef470(object, record). */

extern void p4c_00399470_00399530(u32 *rec, u32 *a0, u32 *a1) __asm__("func_00399470");
extern u32 p4c_003ea2c0_00399530(u32 w, u32 h, u32 d) __asm__("func_003ea2c0");
extern void p4c_003ea370_00399530(u32 r) __asm__("func_003ea370");
extern void p4c_003ea3e0_00399530(u32 r) __asm__("func_003ea3e0");
extern void p4c_003ea560_00399530(u32 r) __asm__("func_003ea560");
extern void p4c_003ea690_00399530(u32 a, u32 b) __asm__("func_003ea690");
extern void p4c_003ec110_00399530(u32 r, u32 t) __asm__("func_003ec110");
extern void p4c_003ec180_00399530(u32 t, u32 r) __asm__("func_003ec180");
extern void p4c_003ec1f0_00399530(u32 r, u32 k, u32 *a, u32 *b, u32 *c, u32 *d) __asm__("func_003ec1f0");
extern u32 p4c_003ec590_00399530(u32 a, u32 b, u32 c, u32 d) __asm__("func_003ec590");
extern void p4c_003ed0b0_00399530(u32 n, u32 o) __asm__("func_003ed0b0");
extern u32 p4c_003ef2e0_00399530(u32 t) __asm__("func_003ef2e0");
extern void p4c_003ef470_00399530(u32 o, u32 *rec) __asm__("func_003ef470");

u32 func_00399530(u32 *arg0, u32 *arg1)
{
    u32 *tex = (u32 *)arg1[0];
    u32 *src = (u32 *)0;
    u32 w0 = tex[3];
    u32 h0 = tex[4];
    u32 w;
    u32 h;
    u32 r18;
    u32 r17;
    u32 nr;
    u32 a = 0;
    u32 b = 0;
    u32 c = 0;
    u32 d = 0;
    u32 t;
    u32 *obj;
    u32 flag;
    u32 f;
    u32 nib;
    u32 v;
    u32 rec[8];
    s32 x;
    s32 y;
    u32 i;

    r18 = p4c_003ea2c0_00399530(w0, h0, 0x20);
    p4c_003ea3e0_00399530(r18);
    p4c_003ec110_00399530(r18, (u32)tex);
    if (arg0 != (u32 *)0) {
        src = (u32 *)arg0[0];
        w = src[3];
        h = src[4];
        r17 = p4c_003ea2c0_00399530(w, h, 0x20);
        p4c_003ea3e0_00399530(r17);
        p4c_003ec110_00399530(r17, (u32)src);
    } else {
        w = tex[3];
        h = tex[4];
        r17 = p4c_003ea2c0_00399530(w, h, 0x20);
        p4c_003ea3e0_00399530(r17);
        for (y = 0; y < (s32)h; y++) {
            for (x = 0; x < (s32)w; x++) {
                *(u32 *)(*(u32 *)(r17 + 0x14) + (u32)((s32)*(u32 *)(r17 + 0x10) * y) + 4 * x) = 0xFFFFFFFFu;
            }
        }
    }
    if (w != w0 || h != h0) {
        nr = p4c_003ea2c0_00399530(w, h, 0x20);
        p4c_003ea3e0_00399530(nr);
        p4c_003ed0b0_00399530(nr, r18);
        p4c_003ea370_00399530(r18);
        r18 = nr;
    }
    p4c_003ea560_00399530(r18);
    p4c_003ea690_00399530(r17, r18);
    p4c_003ec1f0_00399530(r17, 4, &a, &b, &c, &d);
    if (arg0 != (u32 *)0) {
        flag = (u32) * (u8 *)((u8 *)src + 0x23) << 8;
    } else {
        flag = (u32) * (u8 *)((u8 *)tex + 0x23) << 8;
    }
    if ((flag & 0x8000) != 0) {
        d = d | 0x9000;
    }
    t = p4c_003ec590_00399530(a, b, c, d);
    p4c_003ec180_00399530(t, r17);
    obj = (u32 *)p4c_003ef2e0_00399530(t);
    f = (arg0 != (u32 *)0 ? arg0 : arg1)[0x50 / 4];
    nib = (f & 0xF000) >> 12;
    if (((f & 0xF00) >> 8) != nib) {
        nib = 0;
    }
    v = ((nib << 8) & 0xF00) | ((nib << 12) & 0xF000);
    obj[0x50 / 4] = (obj[0x50 / 4] & 0xFFFF00FFu) | v;
    obj[0x50 / 4] = (obj[0x50 / 4] & 0xFFFFFF00u) | (f & 0xFF);
    p4c_003ea370_00399530(r17);
    p4c_003ea370_00399530(r18);
    for (i = 0; i < 8; i++) {
        rec[i] = 0;
    }
    p4c_00399470_00399530(rec, arg0, arg1);
    p4c_003ef470_00399530((u32)obj, rec);
    return (u32)obj;
}
