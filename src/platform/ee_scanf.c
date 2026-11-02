/* Retail string scanner: __svfscanf_r 00446108, sscanf 00442100,
 * _sscanf_r 00442188. Translation of newlib's BSD scanner, using EE long64
 * and four-byte guest pointers rather than the host's long/pointer widths.
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such distribution
 * and use acknowledge that the software was developed by the University of
 * California, Berkeley. The name of the University may not be used to endorse
 * or promote products derived from this software without specific prior
 * written permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "ee_decimal.h"
#include "ps2_fpbit.h"
#include "p4pc/ps2_libc.h"
#include <stdarg.h>
#include <string.h>
#include <limits.h>

typedef struct ScanArgs { va_list *native; const unsigned char *guest; } ScanArgs;
static void *scan_arg(ScanArgs *a)
{
    if(a->native)return va_arg(*a->native,void *);
    uint32_t p;memcpy(&p,a->guest,4);a->guest+=8;return P4_G2H(p);
}
static int space(unsigned c){return c==' '||(c>=9&&c<=13);}
static void store_integer(void *p,uint64_t value,int length)
{
    if(length==2){uint16_t v=(uint16_t)value;memcpy(p,&v,2);}
    else if(length==8)memcpy(p,&value,8);
    else{uint32_t v=(uint32_t)value;memcpy(p,&v,4);}
}
/* String FILEs already have all input in _p/_r; no host FILE or host varargs
 * object is ever put in guest memory. Used by both the native varargs entry
 * and the retail packed-va-list entry. */
static int scan(uint32_t *r,const unsigned char **cursor,size_t *remaining,
                const char *fmt,ScanArgs *args)
{
    const unsigned char *s=*cursor,*start=s;
    size_t left=*remaining;
    int assigned=0,result=0;
    char buf[350];
    while(*fmt){
        unsigned c=(unsigned char)*fmt++;
        if(space(c)){while(left&&space(*s)){++s;--left;}continue;}
        if(c!='%'||*fmt=='%'){
            if(c=='%')++fmt;
            if(!left)goto input_failure;
            if(*s!=c)goto match_failure;
            ++s;--left;continue;
        }
        size_t width=0;
        int suppress=0,length=4,noskip=0,base=10,signed_conversion=0,pointer=0;
        unsigned char ccl[256];
        for(;;){
            c=(unsigned char)*fmt++;
            if(c=='*'){suppress=1;continue;}
            if(c=='l'||c=='L'){length=8;continue;}
            if(c=='h'){length=2;continue;}
            if(c>='0'&&c<='9'){width=width*10+c-'0';continue;}
            break;
        }
        if(!c){result=-1;goto done;}
        if(c=='n'){
            if(!suppress)store_integer(scan_arg(args),(uint64_t)(s-start),length);
            continue;
        }
        if(c=='['){
            int invert=*fmt=='^';if(invert)++fmt;
            memset(ccl,invert?1:0,sizeof ccl);
            unsigned v=(unsigned char)*fmt++;
            if(!v){--fmt;}else{
                for(;;){
                    ccl[v]=(unsigned char)!invert;
                    unsigned next=(unsigned char)*fmt++;
                    if(!next||next==']'){if(!next)--fmt;break;}
                    if(next=='-'&&*fmt!=']'&&*fmt&&v<=(unsigned char)*fmt){
                        unsigned end=(unsigned char)*fmt++;
                        while(v<end)ccl[++v]=(unsigned char)!invert;
                        next=(unsigned char)*fmt++;
                        if(!next||next==']'){if(!next)--fmt;break;}
                    }
                    v=next;
                }
            }
            noskip=1;
        }else if(c=='c')noskip=1;
        if(!left)goto input_failure;
        if(!noskip){while(left&&space(*s)){++s;--left;}if(!left)goto input_failure;}
        if(c=='c'||c=='s'||c=='['){
            if(!width)width=c=='c'?1:SIZE_MAX;
            size_t n=0;
            while(n<left&&n<width&&(c=='c'||(c=='s'?!space(s[n]):ccl[s[n]])))++n;
            if(!n)goto match_failure;
            if(!suppress){void *dst=scan_arg(args);memcpy(dst,s,n);if(c!='c')((char *)dst)[n]=0;++assigned;}
            s+=n;left-=n;continue;
        }
        if(c=='e'||c=='E'||c=='f'||c=='g'||c=='G'){
            size_t cap=width?width:SIZE_MAX,n=0,used=0;
            int signok=1,nodigits=1,dotok=1,expok=1;
            int64_t zeroes=0,leading=0,adjust=0;
            char *exp_start=NULL;
            if(cap>sizeof buf-1)cap=sizeof buf-1;
            while(used<left&&n<cap){
                unsigned ch=s[used];int skip=0;
                if(ch=='0'&&nodigits){signok=0;++zeroes;skip=1;}
                else if(ch>='0'&&ch<='9'){signok=nodigits=0;}
                else if((ch=='+'||ch=='-')&&signok)signok=0;
                else if(ch=='.'&&dotok){signok=dotok=0;leading=zeroes;}
                else if((ch=='e'||ch=='E')&&expok&&(!nodigits||zeroes)){
                    if(!dotok){adjust=zeroes-leading;exp_start=buf+n;}
                    expok=dotok=0;signok=nodigits=1;zeroes=0;
                }else break;
                if(!skip)buf[n++]=(char)ch;
                ++used;
            }
            if(zeroes)nodigits=0;
            if(nodigits){
                if(expok){goto match_failure;}
                unsigned last=(unsigned char)buf[--n];--used;
                if(last!='e'&&last!='E'){--n;--used;}
            }
            buf[n]=0;
            if(!suppress){
                int64_t new_exp=0;
                if(!dotok&&expok){adjust=zeroes-leading;new_exp=-adjust;exp_start=buf+n;}
                else if(adjust)new_exp=(int64_t)((uint64_t)func_00443fd0(r,exp_start+1,NULL,10)-(uint64_t)adjust);
                if(adjust){
                    if(exp_start>=buf+sizeof buf-21)exp_start=buf+sizeof buf-22;
                    char digits[20];unsigned dn=0;
                    uint64_t mag=new_exp<0?0-(uint64_t)new_exp:(uint64_t)new_exp;
                    *exp_start++='e';if(new_exp<0)*exp_start++='-';
                    do{digits[dn++]=(char)('0'+mag%10);mag/=10;}while(mag);
                    while(dn)*exp_start++=digits[--dn];*exp_start=0;
                }
                uint64_t bits=p4_decimal_strtod(r,buf,NULL);
                void *dst=scan_arg(args);
                if(length==8)memcpy(dst,&bits,8);
                else{float f=dptofp(bits);memcpy(dst,&f,4);}
                ++assigned;
            }
            s+=used;left-=used;continue;
        }
        if(c=='D'){length=8;c='d';}
        if(c=='O'){length=8;c='o';}
        if(c=='d'||c=='i'){signed_conversion=1;base=c=='i'?0:10;}
        else if(c=='o')base=8;
        else if(c=='u')base=10;
        else if(c=='x'||c=='X'||c=='p'){base=16;pointer=c=='p';}
        else{signed_conversion=1;base=10;if(c>='A'&&c<='Z')length=8;}
        if(!width||width>sizeof buf-1)width=sizeof buf-1;
        size_t n=0;
        int signok=1,nodigits=1,nz=1,prefix=base==16;
        while(n<width&&n<left){
            unsigned ch=s[n];
            if(ch=='0'){
                if(!base){base=8;prefix=1;}
                if(nz){signok=nz=nodigits=0;}else{signok=prefix=nodigits=0;}
            }else if(ch>='1'&&ch<='9'){
                if(!base)base=10;
                if(ch>='8'&&base<=8)break;
                signok=prefix=nodigits=0;
            }else if((ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')){
                if(base<=10)break;signok=prefix=nodigits=0;
            }else if((ch=='+'||ch=='-')&&signok)signok=0;
            else if((ch=='x'||ch=='X')&&prefix&&n==1){base=16;prefix=0;}
            else break;
            buf[n++]=(char)ch;
        }
        if(nodigits)goto match_failure;
        if(n&&(buf[n-1]=='x'||buf[n-1]=='X'))--n;
        if(!suppress){
            buf[n]=0;
            uint64_t value=signed_conversion?(uint64_t)func_00443fd0(r,buf,NULL,base):func_00444240(r,buf,NULL,base);
            store_integer(scan_arg(args),value,pointer?4:length);++assigned;
        }
        s+=n;left-=n;
    }
 match_failure:
    result=assigned;goto done;
 input_failure:
    result=assigned?assigned:-1;
 done:
    *cursor=s;*remaining=left;return result;
}
static int p4_scan_string(uint32_t *r,const char *text,const char *fmt,va_list ap)
{
    const unsigned char *p=(const unsigned char *)text;size_t n=strlen(text);
    va_list copy;va_copy(copy,ap);ScanArgs a={&copy,NULL};
    int result=scan(r,&p,&n,fmt,&a);va_end(copy);return result;
}
int func_00442100(const char *text,const char *fmt,...)
{
    uint32_t reent;memcpy(&reent,P4_G2H(0x710070),4);
    va_list ap;va_start(ap,fmt);int result=p4_scan_string(P4_G2H(reent),text,fmt,ap);va_end(ap);return result;
}
int func_00442188(uint32_t *r,const char *text,const char *fmt,...)
{
    va_list ap;va_start(ap,fmt);int result=p4_scan_string(r,text,fmt,ap);va_end(ap);return result;
}
int func_00446108(uint32_t *r,uint32_t *file,const char *fmt,const void *packed)
{
    const unsigned char *p=P4_G2H(file[0]);size_t n=(int32_t)file[1]>0?file[1]:0;
    ScanArgs a={NULL,packed};int result=scan(r,&p,&n,fmt,&a);
    file[0]=P4_H2G(p);file[1]=(uint32_t)n;return result;
}
