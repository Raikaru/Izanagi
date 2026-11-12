/* Decimal bigint operations from the retail EE newlib implementation.
 * Reference: newlib-1_9_0/newlib/libc/stdlib/{mprec,dtoa}.c, IEEE_8087,
 * Pack_32. Retail layout/operations are checked against GNU R5900 objdump.
 *
 * Copyright (c) 1991 by AT&T.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY. IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include "ee_decimal.h"
extern void *p4pc_heap_calloc(size_t, size_t);

/* Reent +0x48 is the cached powers-of-five chain; +0x4c its free lists.
 * The original Balloc (0043fb38) allocates 16 four-byte list heads and
 * 20+4*(1<<k) bytes per bigint. Never widen those stored pointers. */
P4DecimalBig *p4_decimal_alloc(uint32_t *r, int k)
{
    if (!r[19]) {
        uint32_t *heads = p4pc_heap_calloc(4, 16);
        if (!heads) return NULL;
        r[19] = P4_H2G(heads);
    }
    uint32_t *heads = P4_G2H(r[19]);
    P4DecimalBig *b;
    if (heads[k]) {
        b = P4_G2H(heads[k]);
        heads[k] = b->next;
    } else {
        int n = 1 << k;
        b = p4pc_heap_calloc(1, 20 + (size_t)n * 4);
        if (!b) return NULL;
        b->k = k;
        b->maxwds = n;
    }
    b->sign = b->wds = 0;
    return b;
}

void p4_decimal_free(uint32_t *r, P4DecimalBig *b)
{
    if (b) {
        uint32_t *heads = P4_G2H(r[19]);
        b->next = heads[b->k];
        heads[b->k] = P4_H2G(b);
    }
}

P4DecimalBig *p4_decimal_multadd(uint32_t *r, P4DecimalBig *b, int m, int a)
{
    int n = b->wds;
    for (int i = 0; i < n; ++i) {
        uint32_t xi = b->x[i];
        uint32_t y = (xi & 0xffffu) * (uint32_t)m + (uint32_t)a;
        uint32_t z = (xi >> 16) * (uint32_t)m + (y >> 16);
        a = (int)(z >> 16);
        b->x[i] = (z << 16) + (y & 0xffffu);
    }
    if (a) {
        if (n >= b->maxwds) {
            P4DecimalBig *c = p4_decimal_alloc(r, b->k + 1);
            p4_decimal_copy(c, b);
            p4_decimal_free(r, b);
            b = c;
        }
        b->x[n++] = (uint32_t)a;
        b->wds = n;
    }
    return b;
}

int p4_decimal_hi0(uint32_t x)
{
    int k = 0;
    if (!(x & 0xffff0000u)) { k = 16; x <<= 16; }
    if (!(x & 0xff000000u)) { k += 8; x <<= 8; }
    if (!(x & 0xf0000000u)) { k += 4; x <<= 4; }
    if (!(x & 0xc0000000u)) { k += 2; x <<= 2; }
    if (!(x & 0x80000000u)) {
        ++k;
        if (!(x & 0x40000000u)) return 32;
    }
    return k;
}

int p4_decimal_lo0(uint32_t *p)
{
    uint32_t x = *p;
    int k = 0;
    if (x & 7) {
        if (x & 1) return 0;
        if (x & 2) { *p = x >> 1; return 1; }
        *p = x >> 2;
        return 2;
    }
    if (!(x & 0xffffu)) { k = 16; x >>= 16; }
    if (!(x & 0xffu)) { k += 8; x >>= 8; }
    if (!(x & 0xfu)) { k += 4; x >>= 4; }
    if (!(x & 3u)) { k += 2; x >>= 2; }
    if (!(x & 1u)) {
        ++k;
        x >>= 1;
        if (!x) return 32;
    }
    *p = x;
    return k;
}

P4DecimalBig *p4_decimal_i2b(uint32_t *r, int value)
{
    P4DecimalBig *b = p4_decimal_alloc(r, 1);
    b->x[0] = (uint32_t)value;
    b->wds = 1;
    return b;
}

P4DecimalBig *p4_decimal_mult(uint32_t *r, P4DecimalBig *a, P4DecimalBig *b)
{
    if (a->wds < b->wds) { P4DecimalBig *t = a; a = b; b = t; }
    int wa = a->wds, wb = b->wds, wc = wa + wb;
    int k = a->k + (wc > a->maxwds);
    P4DecimalBig *c = p4_decimal_alloc(r, k);
    for (int i = 0; i < wc; ++i) c->x[i] = 0;
    for (int j = 0; j < wb; ++j) {
        uint32_t y = b->x[j] & 0xffffu;
        if (y) {
            uint32_t carry = 0;
            for (int i = 0; i < wa; ++i) {
                uint32_t z = (a->x[i] & 0xffffu) * y + (c->x[i+j] & 0xffffu) + carry;
                uint32_t z2 = (a->x[i] >> 16) * y + (c->x[i+j] >> 16) + (z >> 16);
                carry = z2 >> 16;
                c->x[i+j] = (z2 << 16) | (z & 0xffffu);
            }
            c->x[wa+j] = carry;
        }
        y = b->x[j] >> 16;
        if (y) {
            uint32_t carry = 0, z2 = c->x[j];
            for (int i = 0; i < wa; ++i) {
                uint32_t z = (a->x[i] & 0xffffu) * y + (c->x[i+j] >> 16) + carry;
                c->x[i+j] = (z << 16) | (z2 & 0xffffu);
                z2 = (a->x[i] >> 16) * y + (c->x[i+j+1] & 0xffffu) + (z >> 16);
                carry = z2 >> 16;
            }
            c->x[wa+j] = z2;
        }
    }
    while (wc > 0 && !c->x[wc-1]) --wc;
    c->wds = wc;
    return c;
}

P4DecimalBig *p4_decimal_pow5(uint32_t *r, P4DecimalBig *b, int k)
{
    static const int small[3] = {5,25,125};
    if (k & 3) b = p4_decimal_multadd(r, b, small[(k & 3)-1], 0);
    if (!(k >>= 2)) return b;
    P4DecimalBig *p;
    if (!r[18]) {
        p = p4_decimal_i2b(r, 625);
        r[18] = P4_H2G(p);
        p->next = 0;
    } else p = P4_G2H(r[18]);
    for (;;) {
        if (k & 1) {
            P4DecimalBig *c = p4_decimal_mult(r, b, p);
            p4_decimal_free(r, b);
            b = c;
        }
        if (!(k >>= 1)) break;
        if (!p->next) {
            P4DecimalBig *q = p4_decimal_mult(r, p, p);
            p->next = P4_H2G(q);
            q->next = 0;
        }
        p = P4_G2H(p->next);
    }
    return b;
}

P4DecimalBig *p4_decimal_lshift(uint32_t *r, P4DecimalBig *b, int shift)
{
    int n = shift >> 5, k = b->k;
    int n1 = n + b->wds + 1;
    for (int cap = b->maxwds; n1 > cap; cap <<= 1) ++k;
    P4DecimalBig *c = p4_decimal_alloc(r, k);
    for (int i = 0; i < n; ++i) c->x[i] = 0;
    shift &= 31;
    if (shift) {
        uint32_t z = 0;
        for (int i = 0; i < b->wds; ++i) {
            c->x[n+i] = (b->x[i] << shift) | z;
            z = b->x[i] >> (32-shift);
        }
        c->x[n+b->wds] = z;
        if (z) ++n1;
    } else {
        for (int i = 0; i < b->wds; ++i) c->x[n+i] = b->x[i];
    }
    c->wds = n1-1;
    p4_decimal_free(r, b);
    return c;
}

int p4_decimal_cmp(const P4DecimalBig *a, const P4DecimalBig *b)
{
    int n = a->wds - b->wds;
    if (n) return n;
    for (int i = a->wds; i-- > 0;)
        if (a->x[i] != b->x[i]) return a->x[i] < b->x[i] ? -1 : 1;
    return 0;
}

P4DecimalBig *p4_decimal_diff(uint32_t *r, P4DecimalBig *a, P4DecimalBig *b)
{
    int order = p4_decimal_cmp(a,b);
    if (!order) {
        P4DecimalBig *c = p4_decimal_alloc(r,0);
        c->wds = 1; c->x[0] = 0;
        return c;
    }
    if (order < 0) { P4DecimalBig *t=a; a=b; b=t; }
    P4DecimalBig *c = p4_decimal_alloc(r,a->k);
    c->sign = order < 0;
    int32_t borrow = 0;
    for (int i=0; i<a->wds; ++i) {
        uint32_t x = a->x[i], y = i < b->wds ? b->x[i] : 0;
        int32_t lo = (int32_t)(x & 0xffffu) - (int32_t)(y & 0xffffu) + borrow;
        int32_t hi = (int32_t)(x >> 16) - (int32_t)(y >> 16) + (lo >> 16);
        borrow = hi >> 16;
        c->x[i] = ((uint32_t)hi << 16) | ((uint32_t)lo & 0xffffu);
    }
    int n = a->wds;
    while (!c->x[n-1]) --n;
    c->wds = n;
    return c;
}

P4DecimalBig *p4_decimal_d2b(uint32_t *r, uint64_t d, int *e, int *bits)
{
    P4DecimalBig *b = p4_decimal_alloc(r,1);
    uint32_t hi = (uint32_t)(d >> 32), lo = (uint32_t)d;
    uint32_t z = hi & 0xfffffu;
    int de = (int)((hi & 0x7fffffffu) >> 20), k, n;
    if (de) z |= 0x100000u;
    if (lo) {
        k = p4_decimal_lo0(&lo);
        if (k) { b->x[0] = lo | (z << (32-k)); z >>= k; }
        else b->x[0] = lo;
        b->x[1] = z;
        n = b->wds = z ? 2 : 1;
    } else {
        k = p4_decimal_lo0(&z);
        b->x[0] = z;
        n = b->wds = 1;
        k += 32;
    }
    if (de) { *e = de-1023-52+k; *bits = 53-k; }
    else { *e = de-1023-52+1+k; *bits = 32*n-p4_decimal_hi0(b->x[n-1]); }
    return b;
}

int p4_decimal_quorem(P4DecimalBig *b, const P4DecimalBig *s)
{
    int n = s->wds;
    if (b->wds < n) return 0;
    --n;
    uint32_t q = b->x[n] / (s->x[n] + 1u);
    if (q) {
        int32_t borrow = 0;
        uint32_t carry = 0;
        for (int i=0; i<=n; ++i) {
            uint32_t si=s->x[i];
            uint32_t ys=(si & 0xffffu)*q+carry;
            uint32_t zs=(si >> 16)*q+(ys >> 16);
            carry=zs >> 16;
            int32_t y=(int32_t)(b->x[i]&0xffffu)-(int32_t)(ys&0xffffu)+borrow;
            int32_t z=(int32_t)(b->x[i]>>16)-(int32_t)(zs&0xffffu)+(y>>16);
            borrow=z>>16;
            b->x[i]=((uint32_t)z<<16)|((uint32_t)y&0xffffu);
        }
        if (!b->x[n]) { while (n>0 && !b->x[n]) --n; b->wds=n+1; }
    }
    if (p4_decimal_cmp(b,s)>=0) {
        ++q;
        int32_t borrow=0;
        for (int i=0; i<s->wds; ++i) {
            int32_t y=(int32_t)(b->x[i]&0xffffu)-(int32_t)(s->x[i]&0xffffu)+borrow;
            int32_t z=(int32_t)(b->x[i]>>16)-(int32_t)(s->x[i]>>16)+(y>>16);
            borrow=z>>16;
            b->x[i]=((uint32_t)z<<16)|((uint32_t)y&0xffffu);
        }
        n=b->wds-1;
        if (!b->x[n]) { while(n>0 && !b->x[n]) --n; b->wds=n+1; }
    }
    return (int)q;
}

/* newlib-1_10_0 mprec.c, IEEE_8087 / Pack_32. These are the same
 * allocation/cache and limb conventions as dtoa, not a second bigint. */
P4DecimalBig *p4_decimal_s2b(uint32_t *r, const char *s, int nd0, int nd, uint32_t y)
{
    int k=0, n=1, i=9;
    while (n < (nd+8)/9) { n <<= 1; ++k; }
    P4DecimalBig *b=p4_decimal_alloc(r,k);
    b->x[0]=y; b->wds=1;
    if (nd0>9) {
        s+=9;
        do { b=p4_decimal_multadd(r,b,10,*s++-'0'); } while (++i<nd0);
        ++s;
    } else s+=10;
    for (;i<nd;++i) b=p4_decimal_multadd(r,b,10,*s++-'0');
    return b;
}

uint64_t p4_decimal_ulp(uint64_t x)
{
    int32_t l=(int32_t)((x>>32)&0x7ff00000u)-52*0x100000;
    if (l>0) return (uint64_t)(uint32_t)l<<32;
    l=(-l)>>20;
    if (l<20) return (uint64_t)(0x80000u>>l)<<32;
    l-=20;
    return l>=31 ? 1 : UINT64_C(1)<<(31-l);
}

uint64_t p4_decimal_b2d(const P4DecimalBig *a, int *e)
{
    int n=a->wds-1;
    uint32_t y=a->x[n--], z, hi, lo;
    int k=p4_decimal_hi0(y);
    *e=32-k;
    if (k<11) {
        hi=0x3ff00000u|(y>>(11-k));
        z=n>=0 ? a->x[n] : 0;
        lo=(y<<(21+k))|(z>>(11-k));
    } else {
        z=n>=0 ? a->x[n--] : 0;
        k-=11;
        if (k) {
            hi=0x3ff00000u|(y<<k)|(z>>(32-k));
            y=n>=0 ? a->x[n] : 0;
            lo=(z<<k)|(y>>(32-k));
        } else { hi=0x3ff00000u|y; lo=z; }
    }
    return ((uint64_t)hi<<32)|lo;
}

uint64_t p4_decimal_ratio(const P4DecimalBig *a, const P4DecimalBig *b)
{
    extern uint64_t dpdiv(uint64_t, uint64_t);
    int ka,kb;
    uint64_t da=p4_decimal_b2d(a,&ka), db=p4_decimal_b2d(b,&kb);
    int k=ka-kb+32*(a->wds-b->wds);
    if (k>0) da+=(uint64_t)(uint32_t)k<<52;
    else db+=(uint64_t)(uint32_t)(-k)<<52;
    return dpdiv(da,db);
}
