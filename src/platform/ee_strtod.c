/* Retail _strtod_r 00443068, newlib-1_10_0 strtod.c (Gay, IEEE/Pack_32).
 * All floating operations are raw EE soft-double operations, including the
 * initial approximation and ulp correction. The EE ABI's long is 64 bits.
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
#include <string.h>
extern uint64_t func_0044e0a8(uint64_t,uint64_t);
extern uint64_t func_0044e168(uint64_t,uint64_t);
extern uint64_t dpdiv(uint64_t,uint64_t);
extern int dpcmp(uint64_t,uint64_t);
extern uint64_t func_0044ba00(int64_t);
extern int64_t func_0044bc68(uint64_t);
#define ADD func_0044e0a8
#define MUL func_0044e168
#define DIV dpdiv
#define SUB dpsub
#define CMP dpcmp
#define ONE UINT64_C(0x3ff0000000000000)
#define TWO UINT64_C(0x4000000000000000)
#define HALF UINT64_C(0x3fe0000000000000)
#define NEG UINT64_C(0x8000000000000000)
#define EXP UINT64_C(0x7ff0000000000000)
#define BOUND UINT64_C(0x000fffffffffffff)
#define BIG UINT64_C(0x7fefffffffffffff)
#define P53 (UINT64_C(53)<<52)
#define tens P4_DECIMAL_TENS
#define bigtens P4_DECIMAL_BIGTENS
#define tinytens P4_DECIMAL_TINYTENS
uint64_t p4_decimal_strtod(uint32_t *r,const char *s00,const char **end)
{
    int bb2,bb5,bbe,bd2,bd5,bbbits,bs2,c,dsign,e1,esign,i,j,k;
    int nd,nd0,nf,nz=0,nz0=0,sign=0;
    int64_t e,L;
    uint32_t y,z;
    uint64_t rv=0,rv0,aadj,aadj1,adj;
    const char *s=s00,*s0,*s1;
    P4DecimalBig *bb=NULL,*bb1,*bd=NULL,*bd0=NULL,*bs=NULL,*delta=NULL;
    for (;;++s) {
        c=(signed char)*s;
        if(c=='-') {sign=1; if(*++s)break; s=s00;goto ret;}
        if(c=='+') {if(*++s)break; s=s00;goto ret;}
        if(!c){s=s00;goto ret;}
        if(c!=' '&&(c<9||c>13))break;
    }
    if(*s=='0') {
        nz0=1;
        while(*++s=='0') {}
        if(!*s)goto ret;
    }
    s0=s;y=z=0;
    for(nd=nf=0;(c=(signed char)*s)>='0'&&c<='9';++nd,++s)
        if(nd<9)y=10*y+c-'0';else if(nd<16)z=10*z+c-'0';
    nd0=nd;
    if(c=='.') {
        c=(signed char)*++s;
        if(!nd){
            for(;c=='0';c=(signed char)*++s)++nz;
            if(c>'0'&&c<='9'){s0=s;nf+=nz;nz=0;goto have_dig;}
            goto dig_done;
        }
        for(;c>='0'&&c<='9';c=(signed char)*++s){
        have_dig:
            ++nz;
            if((c-='0')){
                nf+=nz;
                for(i=1;i<nz;++i)if(nd++<9)y*=10;else if(nd<=16)z*=10;
                if(nd++<9)y=10*y+c;else if(nd<=16)z=10*z+c;
                nz=0;
            }
        }
    }
 dig_done:
    e=0;
    if(c=='e'||c=='E'){
        if(!nd&&!nz&&!nz0){s=s00;goto ret;}
        s00=s;esign=0;c=(signed char)*++s;
        if(c=='-'){esign=1;c=(signed char)*++s;}else if(c=='+')c=(signed char)*++s;
        if(c>='0'&&c<='9'){
            while(c=='0')c=(signed char)*++s;
            if(c>'0'&&c<='9'){
                e=c-'0';s1=s;
                while((c=(signed char)*++s)>='0'&&c<='9')e=(int64_t)((uint64_t)e*10+(unsigned)(c-'0'));
                if(s-s1>8)e=9999999;
                if(esign)e=-e;
            }
        }else s=s00;
    }
    if(!nd){if(!nz&&!nz0)s=s00;goto ret;}
    e1=(int)(e-=nf);
    if(!nd0)nd0=nd;
    k=nd<16?nd:16;
    rv=func_0044ba00(y);
    if(k>9)rv=ADD(MUL(tens[k-9],rv),func_0044ba00(z));
    e1+=nd-k;
    if(e1>0){
        if((i=e1&15))rv=MUL(rv,tens[i]);
        if((e1&=~15)){
            if(e1>308)goto ovfl;
            if((e1>>=4)){
                for(j=0;e1>1;++j,e1>>=1)if(e1&1)rv=MUL(rv,bigtens[j]);
                rv-=P53;rv=MUL(rv,bigtens[j]);z=(uint32_t)(rv>>32)&0x7ff00000u;
                if(z>0x7ca00000u)goto ovfl;
                if(z>0x7c900000u)rv=BIG;else rv+=P53;
            }
        }
    }else if(e1<0){
        e1=-e1;
        if((i=e1&15))rv=DIV(rv,tens[i]);
        if((e1&=~15)){
            e1>>=4;if(e1>=32)goto undfl;
            for(j=0;e1>1;++j,e1>>=1)if(e1&1)rv=MUL(rv,tinytens[j]);
            rv0=rv;rv=MUL(rv,tinytens[j]);
            if(!CMP(rv,0)){
                rv=MUL(TWO,rv0);rv=MUL(rv,tinytens[j]);
                if(!CMP(rv,0))goto undfl;
                rv=1;
            }
        }
    }
    bd0=p4_decimal_s2b(r,s0,nd0,nd,y);
    for(;;){
        bd=p4_decimal_alloc(r,bd0->k);p4_decimal_copy(bd,bd0);
        bb=p4_decimal_d2b(r,rv,&bbe,&bbbits);bs=p4_decimal_i2b(r,1);
        if(e>=0){bb2=bb5=0;bd2=bd5=(int)e;}
        else{bb2=bb5=(int)-e;bd2=bd5=0;}
        if(bbe>=0)bb2+=bbe;else bd2-=bbe;
        bs2=bb2;i=bbe+bbbits-1;
        j=i < -1022 ? bbe+1075 : 54-bbbits;
        bb2+=j;bd2+=j;i=bb2<bd2?bb2:bd2;if(i>bs2)i=bs2;
        if(i>0){bb2-=i;bd2-=i;bs2-=i;}
        if(bb5>0){bs=p4_decimal_pow5(r,bs,bb5);bb1=p4_decimal_mult(r,bs,bb);p4_decimal_free(r,bb);bb=bb1;}
        if(bb2>0)bb=p4_decimal_lshift(r,bb,bb2);
        if(bd5>0)bd=p4_decimal_pow5(r,bd,bd5);
        if(bd2>0)bd=p4_decimal_lshift(r,bd,bd2);
        if(bs2>0)bs=p4_decimal_lshift(r,bs,bs2);
        delta=p4_decimal_diff(r,bb,bd);dsign=delta->sign;delta->sign=0;
        i=p4_decimal_cmp(delta,bs);
        if(i<0){
            if(dsign||(rv&BOUND))break;
            delta=p4_decimal_lshift(r,delta,1);
            if(p4_decimal_cmp(delta,bs)>0)goto drop_down;
            break;
        }
        if(!i){
            if(dsign){if((rv&BOUND)==BOUND){rv=(rv&EXP)+(UINT64_C(1)<<52);break;}}
            else if(!(rv&BOUND)){
            drop_down:
                rv=((rv&EXP)-(UINT64_C(1)<<52))|BOUND;break;
            }
            if(!(rv&1))break;
            if(dsign)rv=ADD(rv,p4_decimal_ulp(rv));
            else{rv=SUB(rv,p4_decimal_ulp(rv));if(!CMP(rv,0))goto undfl;}
            break;
        }
        aadj=p4_decimal_ratio(delta,bs);
        if(CMP(aadj,TWO)<=0){
            if(dsign)aadj=aadj1=ONE;
            else if(rv&BOUND){if(rv==1)goto undfl;aadj=ONE;aadj1=ONE|NEG;}
            else{aadj=CMP(aadj,ONE)<0?HALF:MUL(aadj,HALF);aadj1=SUB(0,aadj);}
        }else{
            aadj=MUL(aadj,HALF);aadj1=dsign?aadj:SUB(0,aadj);
            /* Retail FLT_ROUNDS=0: compensate the truncating EE packer. */
            aadj1=ADD(aadj1,HALF);
        }
        y=(uint32_t)(rv>>32)&0x7ff00000u;
        if(y==0x7fe00000u){
            rv0=rv;rv-=P53;adj=MUL(aadj1,p4_decimal_ulp(rv));rv=ADD(rv,adj);
            if((rv&EXP)>=(UINT64_C(0x7ca)<<52)){
                if(rv0==BIG)goto ovfl;rv=BIG;goto cont;
            }else rv+=P53;
        }else{
            if(y<=52*0x100000u&&CMP(aadj,ONE)>=0){
                aadj1=func_0044ba00(dptoli(ADD(aadj,HALF)));
                if(!dsign)aadj1=SUB(0,aadj1);
            }
            adj=MUL(aadj1,p4_decimal_ulp(rv));rv=ADD(rv,adj);
        }
        z=(uint32_t)(rv>>32)&0x7ff00000u;
        if(y==z){
            L=func_0044bc68(aadj);aadj=SUB(aadj,func_0044ba00(L));
            if(dsign||(rv&BOUND)){
                if(CMP(aadj,UINT64_C(0x3fdfffff94a03595))<0||CMP(aadj,UINT64_C(0x3fe0000035afe535))>0)break;
            }else if(CMP(aadj,UINT64_C(0x3fcfffff94a03595))<0)break;
        }
 cont:
        p4_decimal_free(r,bb);p4_decimal_free(r,bd);p4_decimal_free(r,bs);p4_decimal_free(r,delta);
    }
 retfree:
    p4_decimal_free(r,bb);p4_decimal_free(r,bd);p4_decimal_free(r,bs);
    p4_decimal_free(r,bd0);p4_decimal_free(r,delta);
 ret:
    if(end)*end=s;
    return sign?SUB(0,rv):rv;
 ovfl:
    r[0]=34;rv=EXP;if(bd0)goto retfree;goto ret;
 undfl:
    r[0]=34;rv=0;if(bd0)goto retfree;goto ret;
}

uint64_t func_00443068(uint32_t *r,const char *s,uint32_t *end)
{
    const char *tail;
    uint64_t bits=p4_decimal_strtod(r,s,&tail);
    if(end){uint32_t guest=P4_H2G(tail);memcpy(end,&guest,4);}
    return bits;
}
