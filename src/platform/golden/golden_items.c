/* Clean-room implementation of committed RE-3 item-categories.md. */
#include "p4pc/golden_items.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint32_t word(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static unsigned half(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1]<<8; }
static const uint32_t camp_flags[12] = {
    0x10000,0x800000,0x400000,0x200000,0x4000000,0x8000000,
    0x1000000,0x80000,0x10000000,0x20000,0x2000000,0x40000
};
int32_t p4_golden_item_category(uint32_t kind)
{
    static const int8_t categories[10] = {0,1,2,4,5,6,7,3,4,0};
    return kind < 10 ? categories[kind] : -1;
}
uint32_t p4_golden_item_category_count(uint32_t category)
{ return category < 8 ? (category == 0 || category == 4 ? 512u : 256u) : 0; }
int32_t p4_golden_item_category_id(uint32_t category, uint32_t index)
{
    static const uint16_t bases[8] = {0,0x100,0x200,0x700,0x300,0x400,0x500,0x600};
    if (index >= p4_golden_item_category_count(category)) return -1;
    if (category == 0 && index >= 256) return (int32_t)(index + 0x800);
    if (category == 4 && index >= 256) return (int32_t)(index + 0x700);
    return (int32_t)(index + bases[category]);
}
int32_t p4_golden_item_aux_index(uint32_t kind, uint32_t id)
{
    static const uint16_t bases[10] = {0,0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x700,0x800};
    return kind < 10 && id < 2560 ? (int32_t)id - bases[kind] : -1;
}
int32_t p4_golden_item_shop_category(uint32_t selector)
{
    static const int8_t categories[6] = {0,1,2,4,6,3};
    return selector < 6 ? categories[selector] : -1;
}
int32_t p4_golden_item_camp_group(uint32_t flags)
{
    for (unsigned i=0;i<12;++i) if (flags == camp_flags[i]) return (int32_t)i;
    return -1;
}
int p4_golden_item_sell_group(uint32_t group)
{ return group < 7 || group == 8; }
int32_t p4_golden_item_help(uint32_t flags)
{
    /* Expanded exact words are specified. Priority between synthetic combined
     * bits is not specified by RE-3; leave those combinations unresolved. */
    static const int8_t help[10] = {0x1c,0x1e,0x2e,0x1d,0x2d,0x2c,0x1f,7,8,10};
    for (unsigned bit=19;bit<=28;++bit) if (flags == (1u<<bit)) return help[bit-19];
    return -1;
}
P4GoldenItemRoute p4_golden_item_route(uint32_t kind, uint32_t skill, const uint8_t *row)
{
    int category = p4_golden_item_category(kind);
    if (category < 0) return P4_GOLDEN_ITEM_UNAVAILABLE;
    if (category <= 3) return P4_GOLDEN_ITEM_EQUIPMENT;
    if (category == 7) return skill > 0 && skill < 624 ? P4_GOLDEN_ITEM_TEACH : P4_GOLDEN_ITEM_UNAVAILABLE;
    if (category != 4) return P4_GOLDEN_ITEM_CONTEXT;
    if (skill >= 472 || !row || (word(row)&2) || !(row[4]&1) || row[12]>2)
        return P4_GOLDEN_ITEM_UNAVAILABLE;
    return skill == 0xf6 ? P4_GOLDEN_ITEM_RETURN : P4_GOLDEN_ITEM_FIELD;
}
int p4_golden_item_consume(uint8_t *quantity, unsigned successes, int refund)
{
    if (!quantity || !*quantity || !successes) return 0;
    if (!refund) --*quantity;
    return 1;
}
int p4_golden_item_teach(const P4GoldenDomainMap *map, uint32_t kind,
    uint32_t skill, uint16_t *slots, uint8_t *quantity, int complete, int success, uint16_t old)
{
    uint32_t golden;
    if (!map || kind != 6 || !slots || !quantity || !skill || skill > UINT16_MAX ||
        p4_golden_domain_from_port(map,P4_GOLDEN_DOMAIN_SKILL,skill,&golden) || !golden || golden>=624)
        return -2;
    if (!*quantity || !complete || !success) return -1;
    unsigned slot=8;
    for (unsigned i=0;i<8;++i) if (!slots[i]) { slot=i; break; }
    if (slot==8) for (unsigned i=0;i<8;++i) if (slots[i]==old) { slot=i; break; }
    if (slot==8) return -1;
    slots[slot]=(uint16_t)skill;
    p4_golden_item_consume(quantity,1,0);
    return (int)slot;
}
static int table(const uint8_t *pak, size_t size, const char *name, unsigned stride, P4GoldenSpan *out, uint32_t *count)
{
    const uint8_t *data; size_t bytes;
    return p4_golden_pak2_find(pak,size,name,&data,&bytes) ||
        p4_golden_record_table(data,bytes,0,out,count) || out->size != (size_t)*count*stride;
}
static int groups(P4GoldenItemSortGroup *out, unsigned count, const char *sort,
                   const uint8_t *pak, size_t size)
{
    P4GoldenSpan rows; uint32_t n;
    if (table(pak,size,sort,24,&rows,&n) || n!=count) return -1;
    for (unsigned i=0;i<count;++i) {
        const uint8_t *row=rows.bytes+i*24; char name[48];
        if (memcmp(row+4,"cmpTypeID_",10)) return -1;
        size_t length=10;
        while (length<20 && row[4+length]) {
            unsigned c=row[4+length];
            if (!((c>='A'&&c<='Z') || c=='_')) return -1;
            ++length;
        }
        if (length<13 || length==20) return -1;
        for (size_t j=length;j<20;++j) if (row[4+j]) return -1;
        snprintf(name,sizeof name,"%.*s.ctd",(int)length,(const char *)row+4);
        out[i].flags=word(row);
        if (table(pak,size,name,4,&out[i].ranges,&n)) return -1;
        for (unsigned j=0;j<n;++j) {
            const uint8_t *range=out[i].ranges.bytes+j*4;
            if (half(range)>half(range+2) || half(range+2)>=2560) return -1;
        }
        if (count==12 && out[i].flags!=camp_flags[i]) return -1;
    }
    return 0;
}
int p4_golden_item_sort_open(P4GoldenItemSort *out, const uint8_t *pak, size_t size)
{
    P4GoldenItemSort result;
    if (!out || !pak || groups(result.camp,12,"cmpItemSortTable.ctd",pak,size) ||
        groups(result.equipment,4,"cmpEquipSortTable.ctd",pak,size)) return -1;
    static const uint32_t equipment[4]={0x3fff,0x4000,0x8000,0x100000};
    for (unsigned i=0;i<4;++i) if (result.equipment[i].flags!=equipment[i]) return -1;
    *out=result; return 0;
}
typedef struct SaleEntry { uint32_t id, flags, quantity; } SaleEntry;
static int sale_compare(const void *left, const void *right)
{
    const SaleEntry *a=left,*b=right;
    if (a->flags!=b->flags) return (a->flags>b->flags)-(a->flags<b->flags);
    return (a->id>b->id)-(a->id<b->id);
}
static int weapons(P4GoldenItemRead flags,P4GoldenItemRead quantity,P4GoldenItemEmit emit,void *context)
{
    SaleEntry rows[512]; unsigned count=0;
    for (unsigned i=0;i<512;++i) {
        uint32_t id=(uint32_t)p4_golden_item_category_id(0,i), n=quantity(context,id);
        if (n) rows[count++]=(SaleEntry){id,flags(context,id),n};
    }
    if (count>1) qsort(rows,count,sizeof rows[0],sale_compare);
    for (unsigned i=0;i<count;++i) if (emit(context,rows[i].id,rows[i].quantity)) return -1;
    return (int)count;
}
int p4_golden_item_list(const P4GoldenItemSort *sort, int selector,
    P4GoldenItemRead flags, P4GoldenItemRead quantity, P4GoldenItemEmit emit, void *context)
{
    if (!sort || !flags || !quantity || !emit) return -1;
    if (selector!=-1 && selector!=3) {
        int category=p4_golden_item_shop_category((uint32_t)selector);
        if (category<0) return -1;
        if (!category) return weapons(flags,quantity,emit,context);
        int count=0;
        for (unsigned i=0;i<p4_golden_item_category_count((uint32_t)category);++i) {
            uint32_t id=(uint32_t)p4_golden_item_category_id((uint32_t)category,i), n=quantity(context,id);
            if (!n) continue;
            if (selector==4 && !count) {
                if (emit(context,UINT32_MAX,0)) return -1;
                ++count;
            }
            if (emit(context,id,n)) return -1;
            ++count;
        }
        return count;
    }
    int count=0;
    for (unsigned i=0;i<12;++i) {
        if (selector==3 && !p4_golden_item_sell_group(i)) continue;
        const P4GoldenItemSortGroup *g=&sort->camp[i];
        if (g->ranges.size%4 || (g->ranges.size && !g->ranges.bytes)) return -1;
        for (size_t r=0;r<g->ranges.size;r+=4) {
            unsigned first=half(g->ranges.bytes+r), last=half(g->ranges.bytes+r+2);
            if (first>last || last>=2560) return -1;
            for (unsigned id=first;id<=last;++id) {
                if (flags(context,id)!=g->flags) continue;
                uint32_t n=quantity(context,id);
                if (n) { if (emit(context,id,n)) return -1; ++count; }
            }
        }
    }
    return count;
}
