/* Retail vfprintf conversion engine (00444870), cvt (00445dd0), exponent
 * (00445f78), adapted from newlib-1_10_0 to the port's guest-mask va_list ABI.
 * The FILE transport is a host byte sink; no host formatting is performed.
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 * This code is derived from software contributed to Berkeley by Chris Torek.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <string.h>
#include "ee_stdio.h"
#include "ee_decimal.h"
#include "ps2_fpbit.h"
extern int dpcmp(uint64_t, uint64_t);

enum {
    ALT=1, HEXPREFIX=2, LADJUST=4, LONGDBL=8, LONGINT=16,
    QUADINT=32, SHORTINT=64, ZEROPAD=128, FPT=256
};
typedef struct {
    char *buffer;
    size_t capacity, length;
    FILE *stream;
    int failed;
} P4FormatSink;

static void put(P4FormatSink *s, const char *bytes, size_t n)
{
    if(s->stream) {
        if(n && fwrite(bytes,1,n,s->stream)!=n)s->failed=1;
    } else if(s->capacity && s->length<s->capacity-1) {
        size_t room=s->capacity-1-s->length;
        size_t take=n<room?n:room;
        memcpy(s->buffer+s->length,bytes,take);
    }
    s->length+=n;
}

static void pad(P4FormatSink *s, int n, int zero)
{
    static const char blanks[16]="                ";
    static const char zeroes[16]="0000000000000000";
    const char *bytes=zero?zeroes:blanks;
    while(n>16) { put(s,bytes,16);n-=16; }
    if(n>0)put(s,bytes,(size_t)n);
}

static int exponent(char *out, int value, int conversion)
{
    char reversed[16];
    int n=0;
    char *start=out;
    *out++=(char)conversion;
    if(value<0) { *out++='-';value=-value; } else *out++='+';
    do { reversed[n++]=(char)('0'+value%10);value/=10; } while(value);
    if(n<2)*out++='0';
    while(n)*out++=reversed[--n];
    return (int)(out-start);
}

static char *convert_float(uint64_t value, int precision, int flags,
                            char *negative, int *point, int ch, int *length)
{
    int mode=2, ignored;
    if(ch=='f')mode=3;
    else if(ch=='e' || ch=='E')++precision;
    if(value>>63) { value=dpsub(0,value);*negative='-'; }
    else *negative=0;
    char *end;
    uint32_t *reent=P4_G2H(*(const uint32_t *)P4_G2H(0x00710070u));
    char *digits=p4_decimal_dtoa(reent,value,mode,precision,point,&ignored,&end);
    if((ch!='g' && ch!='G') || (flags&ALT)) {
        char *limit=digits+precision;
        if(ch=='f') {
            if(*digits=='0' && dpcmp(value,0))*point=1-precision;
            limit+=*point;
        }
        if(!dpcmp(value,0))end=limit;
        while(end<limit)*end++='0';
    }
    *length=(int)(end-digits);
    return digits;
}

static int format(P4FormatSink *sink, const char *fmt, unsigned mask, va_list ap)
{
    unsigned argi=0;
    int ret=0;
#define MASKED() (argi<32 && ((mask>>argi)&1u))
#define NEXT_INT() (MASKED() ? (int32_t)va_arg(ap,uint64_t) : va_arg(ap,int))
#define NEXT_POINTER() (MASKED() ? P4_G2H((uint32_t)va_arg(ap,uint64_t)) : va_arg(ap,void *))
    while(*fmt) {
        const char *literal=fmt;
        while(*fmt && *fmt!='%')++fmt;
        put(sink,literal,(size_t)(fmt-literal));
        ret+=(int)(fmt-literal);
        if(!*fmt)break;
        ++fmt;
        int flags=0,width=0,prec=-1,dprec=0,size=0,point=0,ndig=0,expsize=0;
        char sign=0,softsign=0,buffer[96],expbuf[16],ch;
        char *cp=buffer;
        uint64_t value=0,number=0;
        int base=10;
        const char *digits="0123456789abcdef";
rflag:
        ch=*fmt++;
reswitch:
        switch(ch) {
        case ' ':if(!sign)sign=' ';goto rflag;
        case '#':flags|=ALT;goto rflag;
        case '*':
            width=NEXT_INT();++argi;
            if(width>=0)goto rflag;
            width=-width;
            /* fall through */
        case '-':flags|=LADJUST;goto rflag;
        case '+':sign='+';goto rflag;
        case '.': {
            ch=*fmt++;
            int n=0;
            if(ch=='*') { n=NEXT_INT();++argi;prec=n<0?-1:n;goto rflag; }
            while(ch>='0' && ch<='9') { n=10*n+ch-'0';ch=*fmt++; }
            prec=n<0?-1:n;goto reswitch;
        }
        case '0':flags|=ZEROPAD;goto rflag;
        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
            width=0;
            do { width=10*width+ch-'0';ch=*fmt++; } while(ch>='0' && ch<='9');
            goto reswitch;
        case 'L':flags|=LONGDBL;goto rflag;
        case 'h':flags|=SHORTINT;goto rflag;
        case 'l':
            if(*fmt=='l') { ++fmt;flags|=QUADINT; } else flags|=LONGINT;
            goto rflag;
        case 'q':flags|=QUADINT;goto rflag;
        case 'c':buffer[0]=(char)NEXT_INT();++argi;size=1;sign=0;break;
        case 'D':flags|=LONGINT; /* fall through */
        case 'd':case 'i': {
            int64_t n;
            if(MASKED()) {
                n=(int64_t)va_arg(ap,uint64_t);
                if(!(flags&(LONGINT|QUADINT))) {
                    n=(int32_t)n;
                    if(flags&SHORTINT)n=(int16_t)n;
                }
            } else if(flags&(LONGINT|QUADINT))n=va_arg(ap,int64_t);
            else { n=va_arg(ap,int);if(flags&SHORTINT)n=(int16_t)n; }
            ++argi;
            number=(uint64_t)n;
            if(n<0) { number=UINT64_C(0)-number;sign='-'; }
            goto number;
        }
        case 'e':case 'E':case 'f':case 'g':case 'G': {
            if(prec<0)prec=6;
            else if((ch=='g' || ch=='G') && !prec)prec=1;
            if(MASKED())value=va_arg(ap,uint64_t);
            else { double native=va_arg(ap,double);memcpy(&value,&native,sizeof value); }
            ++argi;
            if((value&UINT64_C(0x7fffffffffffffff))==UINT64_C(0x7ff0000000000000)) {
                if(value>>63)sign='-';
                cp="Inf";size=3;break;
            }
            if((value&UINT64_C(0x7ff0000000000000))==UINT64_C(0x7ff0000000000000)) {
                cp="NaN";size=3;break;
            }
            flags|=FPT;
            cp=convert_float(value,prec,flags,&softsign,&point,ch,&ndig);
            if(ch=='g' || ch=='G') {
                if(point<=-4 || point>prec)ch=ch=='g'?'e':'E';else ch='g';
            }
            if(ch<='e') {
                --point;expsize=exponent(expbuf,point,ch);
                size=expsize+ndig;
                if(ndig>1 || (flags&ALT))++size;
            } else if(ch=='f') {
                if(point>0) { size=point;if(prec || (flags&ALT))size+=prec+1; }
                else size=prec+2;
            } else if(point>=ndig) { size=point;if(flags&ALT)++size; }
            else size=ndig+(point>0?1:2-point);
            if(softsign)sign='-';
            break;
        }
        case 'n': {
            void *destination=NEXT_POINTER();++argi;
            if(flags&(LONGINT|QUADINT)) { int64_t n=ret;memcpy(destination,&n,8); }
            else if(flags&SHORTINT) { int16_t n=(int16_t)ret;memcpy(destination,&n,2); }
            else { int32_t n=ret;memcpy(destination,&n,4); }
            continue;
        }
        case 'O':flags|=LONGINT; /* fall through */
        case 'o':base=8;goto unsigned_number;
        case 'p':
            if(MASKED())number=(uint32_t)va_arg(ap,uint64_t);
            else number=P4_H2G(va_arg(ap,void *));
            ++argi;base=16;flags|=HEXPREFIX;ch='x';sign=0;goto number;
        case 's':
            cp=NEXT_POINTER();++argi;
            if(!cp)cp="(null)";
            if(prec>=0) { const char *end=memchr(cp,0,(size_t)prec);size=end?(int)(end-cp):prec; }
            else size=(int)strlen(cp);
            sign=0;break;
        case 'U':flags|=LONGINT; /* fall through */
        case 'u':goto unsigned_number;
        case 'X':digits="0123456789ABCDEF"; /* fall through */
        case 'x':base=16;
unsigned_number:
            if(MASKED()) {
                number=va_arg(ap,uint64_t);
                if(!(flags&(LONGINT|QUADINT))) {
                    number=(uint32_t)number;
                    if(flags&SHORTINT)number=(uint16_t)number;
                }
            } else if(flags&(LONGINT|QUADINT))number=va_arg(ap,uint64_t);
            else { number=va_arg(ap,unsigned);if(flags&SHORTINT)number=(uint16_t)number; }
            ++argi;sign=0;
            if(base==16 && (flags&ALT) && number)flags|=HEXPREFIX;
number:
            dprec=prec;
            if(prec>=0)flags&=~ZEROPAD;
            cp=buffer+sizeof buffer;
            if(number || prec) {
                do { *--cp=digits[number%(unsigned)base];number/=(unsigned)base; } while(number);
                if(base==8 && (flags&ALT) && *cp!='0')*--cp='0';
            } else if(base==8 && (flags&ALT))*--cp='0';
            size=(int)(buffer+sizeof buffer-cp);break;
        default:
            if(!ch)goto done;
            buffer[0]=ch;size=1;sign=0;break;
        }
        int real=dprec>size?dprec:size;
        if(sign)++real;else if(flags&HEXPREFIX)real+=2;
        if(!(flags&(LADJUST|ZEROPAD)))pad(sink,width-real,0);
        if(sign)put(sink,&sign,1);
        else if(flags&HEXPREFIX) { char prefix[2]={'0',ch};put(sink,prefix,2); }
        if((flags&(LADJUST|ZEROPAD))==ZEROPAD)pad(sink,width-real,1);
        pad(sink,dprec-size,1);
        if(!(flags&FPT))put(sink,cp,(size_t)size);
        else if(ch>='f') {
            if(!dpcmp(value,0)) {
                put(sink,"0",1);
                if(point<ndig || (flags&ALT)) { put(sink,".",1);pad(sink,ndig-1,1); }
            } else if(point<=0) {
                put(sink,"0",1);
                if(point || ndig) { put(sink,".",1);pad(sink,-point,1);put(sink,cp,(size_t)ndig); }
            } else if(point>=ndig) {
                put(sink,cp,(size_t)ndig);pad(sink,point-ndig,1);
                if(flags&ALT)put(sink,".",1);
            } else {
                put(sink,cp,(size_t)point);put(sink,".",1);put(sink,cp+point,(size_t)(ndig-point));
            }
        } else {
            if(ndig>1 || (flags&ALT)) {
                put(sink,cp,1);put(sink,".",1);
                if(dpcmp(value,0))put(sink,cp+1,(size_t)(ndig-1));else pad(sink,ndig-1,1);
            } else put(sink,cp,1);
            put(sink,expbuf,(size_t)expsize);
        }
        if(flags&LADJUST)pad(sink,width-real,0);
        ret+=width>real?width:real;
    }
done:
#undef NEXT_POINTER
#undef MASKED
    if(sink->capacity)
        sink->buffer[sink->length<sink->capacity?sink->length:sink->capacity-1]=0;
    return sink->failed?-1:ret;
}

int p4pc_ee_vsnprintf(char *buffer, size_t capacity, const char *fmt, unsigned mask, va_list ap)
{
    P4FormatSink sink={buffer,capacity,0,NULL,0};
    return format(&sink,fmt,mask,ap);
}

int p4pc_ee_vfprintf(FILE *stream, const char *fmt, unsigned mask, va_list ap)
{
    P4FormatSink sink={NULL,0,0,stream,0};
    return format(&sink,fmt,mask,ap);
}

/* Rewritten game calls use the same convention on every host: one leading mask,
 * then one uint64_t slot for each masked GP value. Native pointers and native
 * doubles remain unmasked and retain their normal C varargs classes. */
static int pf_vsnprintf(char *buf, size_t cap, const char *fmt, va_list ap)
{
    unsigned mask = va_arg(ap, unsigned);
    return p4pc_ee_vsnprintf(buf, cap, fmt, mask, ap);
}

int p4pf_sprintf(char *buf, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int r = pf_vsnprintf(buf, (size_t)0x7fffffff, fmt, ap);
    va_end(ap);
    return r;
}

int p4pf_snprintf(char *buf, size_t cap, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int r = pf_vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    return r;
}

int p4pf_printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    unsigned mask = va_arg(ap, unsigned);
    int n = p4pc_ee_vfprintf(stdout, fmt, mask, ap);
    va_end(ap);
    return n;
}

int p4pf_fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    unsigned mask = va_arg(ap, unsigned);
    int n = p4pc_ee_vfprintf(f, fmt, mask, ap);
    va_end(ap);
    return n;
}
