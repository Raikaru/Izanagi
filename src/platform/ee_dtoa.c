/* Retail _dtoa_r, 0043ca70. Arithmetic values below are IEEE binary64 bit
 * payloads, not host doubles; every rounded operation uses the EE fp-bit core.
 * Reference: David M. Gay's dtoa in newlib-1_9_0, IEEE_8087/Pack_32.
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
#include "ps2_fpbit.h"
extern uint64_t func_0044e0a8(uint64_t, uint64_t);
extern uint64_t dpdiv(uint64_t, uint64_t);
extern int dpcmp(uint64_t, uint64_t);
extern uint64_t litodp(int32_t);
extern int32_t dptoli(uint64_t);
#define ADD func_0044e0a8
#define SUB dpsub
#define MUL func_0044e168
#define DIV dpdiv
#define CMP dpcmp
#define ONE UINT64_C(0x3ff0000000000000)
#define HALF UINT64_C(0x3fe0000000000000)
#define FIVE UINT64_C(0x4014000000000000)
#define TEN UINT64_C(0x4024000000000000)
#define tens P4_DECIMAL_TENS
#define bigtens P4_DECIMAL_BIGTENS

char *p4_decimal_dtoa(uint32_t *r, uint64_t d, int mode, int ndigits,
                      int *decpt, int *sign, char **end)
{
    int bbits, b2, b5, be, dig=0, i, ieps, ilim, ilim0, ilim1, j, j1;
    int k, k0, k_check, leftright, m2, m5, s2, s5, special, quick, denorm;
    int32_t L;
    uint64_t d2, eps, ds;
    P4DecimalBig *b, *b1, *delta, *mlo=NULL, *mhi=NULL, *S=NULL;
    char *s, *s0;
    if (r[16]) {
        P4DecimalBig *old = P4_G2H(r[16]);
        old->k = (int32_t)r[17]; old->maxwds = 1 << r[17];
        p4_decimal_free(r,old); r[16]=0;
    }
    *sign = (int)(d >> 63);
    d &= UINT64_C(0x7fffffffffffffff);
    if ((d & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)) {
        *decpt=9999;
        s=P4_G2H((d & UINT64_C(0xfffffffffffff)) ? 0x00754e98u : 0x00754e88u);
        if (end) *end=s+((d & UINT64_C(0xfffffffffffff)) ? 3 : 8);
        return s;
    }
    if (!CMP(d,0)) {
        *decpt=1; s=P4_G2H(0x00754ea0u);
        if(end) *end=s+1;
        return s;
    }
    b=p4_decimal_d2b(r,d,&be,&bbits);
    i=(int)(d >> 52);
    if (i) {
        d2=(d & UINT64_C(0xfffffffffffff)) | ONE;
        i-=1023; denorm=0;
    } else {
        i=bbits+be+1074;
        uint32_t x=i>32 ? ((uint32_t)(d>>32) << (64-i)) | ((uint32_t)d >> (i-32))
                        : (uint32_t)d << (32-i);
        /* x is an unsigned integer: converting its two halves avoids a host
         * cast and preserves all 32 bits through the signed EE helper. */
        d2=ADD(MUL(litodp((int32_t)(x>>16)),UINT64_C(0x40f0000000000000)),
               litodp((int32_t)(x&0xffffu)));
        d2-=UINT64_C(31)<<52;
        i-=1075; denorm=1;
    }
    ds=ADD(ADD(MUL(SUB(d2,UINT64_C(0x3ff8000000000000)),UINT64_C(0x3fd287a7636f4361)),
               UINT64_C(0x3fc68a288b60c8b3)),
           MUL(litodp(i),UINT64_C(0x3fd34413509f79fb)));
    k=dptoli(ds);
    if (CMP(ds,0)<0 && CMP(ds,litodp(k))) --k;
    k_check=1;
    if(k>=0 && k<=22) { if(CMP(d,tens[k])<0) --k; k_check=0; }
    j=bbits-i-1;
    if(j>=0) { b2=0;s2=j; } else { b2=-j;s2=0; }
    if(k>=0) { b5=0;s5=k;s2+=k; } else { b2-=k;b5=-k;s5=0; }
    if(mode<0 || mode>9) mode=0;
    quick=1;
    if(mode>5) { mode-=4;quick=0; }
    leftright=1;ilim=ilim1=-1;
    switch(mode) {
    case 0:case 1:i=18;ndigits=0;break;
    case 2:leftright=0; /* fall through */
    case 4:if(ndigits<=0) ndigits=1;ilim=ilim1=i=ndigits;break;
    case 3:leftright=0; /* fall through */
    case 5:i=ndigits+k+1;ilim=i;ilim1=i-1;if(i<=0)i=1;break;
    }
    for(r[17]=0,j=4;20+j<=i;j<<=1) ++r[17];
    b1=p4_decimal_alloc(r,(int)r[17]);r[16]=P4_H2G(b1);
    s=s0=(char *)b1;
    if(ilim>=0 && ilim<=14 && quick) {
        i=0;d2=d;k0=k;ilim0=ilim;ieps=2;
        if(k>0) {
            ds=tens[k&15];j=k>>4;
            if(j&16) { j&=15;d=DIV(d,bigtens[4]);++ieps; }
            for(;j;j>>=1,++i) if(j&1) { ++ieps;ds=MUL(ds,bigtens[i]); }
            d=DIV(d,ds);
        } else if((j1=-k)!=0) {
            d=MUL(d,tens[j1&15]);
            for(j=j1>>4;j;j>>=1,++i) if(j&1) { ++ieps;d=MUL(d,bigtens[i]); }
        }
        if(k_check && CMP(d,ONE)<0 && ilim>0) {
            if(ilim1<=0) goto fast_failed;
            ilim=ilim1;--k;d=MUL(d,TEN);++ieps;
        }
        eps=ADD(MUL(litodp(ieps),d),UINT64_C(0x401c000000000000));
        eps-=UINT64_C(52)<<52;
        if(!ilim) {
            S=mhi=NULL;d=SUB(d,FIVE);
            if(CMP(d,eps)>0) goto one_digit;
            if(CMP(d,eps^UINT64_C(0x8000000000000000))<0) goto no_digits;
            goto fast_failed;
        }
        if(leftright) {
            eps=SUB(DIV(HALF,tens[ilim-1]),eps);
            for(i=0;;) {
                L=dptoli(d);d=SUB(d,litodp(L));*s++=(char)('0'+L);
                if(CMP(d,eps)<0) goto ret1;
                if(CMP(SUB(ONE,d),eps)<0) goto bump_up;
                if(++i>=ilim) break;
                eps=MUL(eps,TEN);d=MUL(d,TEN);
            }
        } else {
            eps=MUL(eps,tens[ilim-1]);
            for(i=1;;++i,d=MUL(d,TEN)) {
                L=dptoli(d);d=SUB(d,litodp(L));*s++=(char)('0'+L);
                if(i==ilim) {
                    if(CMP(d,ADD(HALF,eps))>0) goto bump_up;
                    if(CMP(d,SUB(HALF,eps))<0) {
                        while(*--s=='0') {} ++s;goto ret1;
                    }
                    break;
                }
            }
        }
fast_failed:
        s=s0;d=d2;k=k0;ilim=ilim0;
    }
    if(be>=0 && k<=14) {
        ds=tens[k];
        if(ndigits<0 && ilim<=0) {
            S=mhi=NULL;
            if(ilim<0 || CMP(d,MUL(FIVE,ds))<=0) goto no_digits;
            goto one_digit;
        }
        for(i=1;;++i) {
            L=dptoli(DIV(d,ds));d=SUB(d,MUL(litodp(L),ds));
            *s++=(char)('0'+L);
            if(i==ilim) {
                d=ADD(d,d);
                if(CMP(d,ds)>0 || (!CMP(d,ds) && (L&1))) {
bump_up:
                    while(*--s=='9') if(s==s0) { ++k;*s='0';break; }
                    ++*s++;
                }
                break;
            }
            d=MUL(d,TEN);
            if(!CMP(d,0)) break;
        }
        goto ret1;
    }
    m2=b2;m5=b5;mhi=mlo=NULL;
    if(leftright) {
        if(mode<2) i=denorm ? be+1075 : 54-bbits;
        else {
            j=ilim-1;
            if(m5>=j)m5-=j;
            else { j-=m5;s5+=j;b5+=j;m5=0; }
            i=ilim;if(i<0) { m2-=i;i=0; }
        }
        b2+=i;s2+=i;mhi=p4_decimal_i2b(r,1);
    }
    if(m2>0 && s2>0) { i=m2<s2?m2:s2;b2-=i;m2-=i;s2-=i; }
    if(b5>0) {
        if(leftright) {
            if(m5>0) {
                mhi=p4_decimal_pow5(r,mhi,m5);
                b1=p4_decimal_mult(r,mhi,b);p4_decimal_free(r,b);b=b1;
            }
            if((j=b5-m5)!=0)b=p4_decimal_pow5(r,b,j);
        } else b=p4_decimal_pow5(r,b,b5);
    }
    S=p4_decimal_i2b(r,1);
    if(s5>0)S=p4_decimal_pow5(r,S,s5);
    special=0;
    if(mode<2 && !(uint32_t)d && !((d>>32)&0xfffffu) && (d>>52)) {
        ++b2;++s2;special=1;
    }
    i=((s5 ? 32-p4_decimal_hi0(S->x[S->wds-1]) : 1)+s2)&31;
    if(i)i=32-i;
    if(i>4) { i-=4;b2+=i;m2+=i;s2+=i; }
    else if(i<4) { i+=28;b2+=i;m2+=i;s2+=i; }
    if(b2>0)b=p4_decimal_lshift(r,b,b2);
    if(s2>0)S=p4_decimal_lshift(r,S,s2);
    if(k_check && p4_decimal_cmp(b,S)<0) {
        --k;b=p4_decimal_multadd(r,b,10,0);
        if(leftright)mhi=p4_decimal_multadd(r,mhi,10,0);
        ilim=ilim1;
    }
    if(ilim<=0 && mode>2) {
        if(ilim<0 || p4_decimal_cmp(b,S=p4_decimal_multadd(r,S,5,0))<=0) {
no_digits:
            k=-1-ndigits;goto ret;
        }
one_digit:
        *s++='1';++k;goto ret;
    }
    if(leftright) {
        if(m2>0)mhi=p4_decimal_lshift(r,mhi,m2);
        mlo=mhi;
        if(special) {
            mhi=p4_decimal_alloc(r,mhi->k);p4_decimal_copy(mhi,mlo);
            mhi=p4_decimal_lshift(r,mhi,1);
        }
        for(i=1;;++i) {
            dig=p4_decimal_quorem(b,S)+'0';j=p4_decimal_cmp(b,mlo);
            delta=p4_decimal_diff(r,S,mhi);
            j1=delta->sign ? 1 : p4_decimal_cmp(b,delta);p4_decimal_free(r,delta);
            if(j1==0 && !mode && !(d&1)) {
                if(dig=='9')goto round_9_up;
                if(j>0)++dig;
                *s++=(char)dig;goto ret;
            }
            if(j<0 || (!j && !mode && !(d&1))) {
                if(j1>0) {
                    b=p4_decimal_lshift(r,b,1);j1=p4_decimal_cmp(b,S);
                    if((j1>0 || (!j1 && (dig&1))) && dig++=='9')goto round_9_up;
                }
                *s++=(char)dig;goto ret;
            }
            if(j1>0) {
                if(dig=='9') {
round_9_up:
                    *s++='9';goto roundoff;
                }
                *s++=(char)(dig+1);goto ret;
            }
            *s++=(char)dig;
            if(i==ilim)break;
            b=p4_decimal_multadd(r,b,10,0);
            if(mlo==mhi)mlo=mhi=p4_decimal_multadd(r,mhi,10,0);
            else { mlo=p4_decimal_multadd(r,mlo,10,0);mhi=p4_decimal_multadd(r,mhi,10,0); }
        }
    } else {
        for(i=1;;++i) {
            *s++=(char)(dig=p4_decimal_quorem(b,S)+'0');
            if(i>=ilim)break;
            b=p4_decimal_multadd(r,b,10,0);
        }
    }
    b=p4_decimal_lshift(r,b,1);j=p4_decimal_cmp(b,S);
    if(j>0 || (!j && (dig&1))) {
roundoff:
        while(*--s=='9') if(s==s0) { ++k;*s++='1';goto ret; }
        ++*s++;
    } else { while(*--s=='0') {} ++s; }
ret:
    p4_decimal_free(r,S);
    if(mhi) {
        if(mlo && mlo!=mhi)p4_decimal_free(r,mlo);
        p4_decimal_free(r,mhi);
    }
ret1:
    p4_decimal_free(r,b);*s=0;*decpt=k+1;if(end)*end=s;return s0;
}

/* The public EE entry stores a four-byte guest end pointer, not char **. */
char *func_0043ca70(uint32_t *r, uint64_t bits, int mode, int digits,
                    int *point, int *sign, uint32_t *guest_end)
{
    char *end;
    char *result=p4_decimal_dtoa(r,bits,mode,digits,point,sign,
                                 guest_end ? &end : NULL);
    if(guest_end)*guest_end=P4_H2G(end);
    return result;
}
