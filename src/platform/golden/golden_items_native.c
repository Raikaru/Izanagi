#include "golden_items_native.h"
#include "p4pc/golden_items.h"
#include "p4pc/golden_battle_rows_native.h"
#include "p4pc/game_items_native.h"
#include "p4pc/package_boot.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static struct {
    uint8_t *cmp;
    P4GoldenItemSort sort;
    P4GoldenDomainMap map;
    int active;
} items;
extern uint32_t datEntryGetKind(int16_t);
extern uint32_t datEntryGetFlags(int16_t);
extern int32_t datItemCount(int16_t);
extern void datItemSetCount(int16_t, int32_t);
static void ready(void)
{
    if (p4_game_mode()!=P4_GAME_MODE_GOLDEN || !items.active)
        p4_game_tables_items_native_refuse(P4_TABLE_NOT_READY);
}
static int prepare(void *unused) { (void)unused; return items.cmp ? P4_TABLE_OK : P4_TABLE_NOT_READY; }
static void commit(void *unused) { (void)unused; items.active=1; }
int p4_golden_items_capture(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{
    (void)context;
    const P4GoldenCacheTable *cmp=NULL;
    if (p4_game_mode()!=P4_GAME_MODE_GOLDEN || !builder || items.cmp || !extra) goto invalid;
    for (size_t i=0;i<count;++i) if (extra[i].name && !strcmp(extra[i].name,"items/cmp-tables")) {
        if (cmp) goto invalid;
        cmp=&extra[i];
    }
    if (!cmp || !cmp->data || !cmp->size) goto invalid;
    P4GoldenDomainMap map;
    const P4GoldenDomainShape shape={624,472,368,2560,256};
    if (p4_golden_domain_map_build(&map,&shape,NULL,0)) goto invalid;
    uint8_t *copy=malloc(cmp->size);
    if (!copy) goto invalid;
    memcpy(copy,cmp->data,cmp->size);
    P4GoldenItemSort sort;
    if (p4_golden_item_sort_open(&sort,copy,cmp->size) ||
        p4_game_tables_battle_complete_register("golden-items",prepare,commit,NULL)) {
        free(copy); goto invalid;
    }
    items.cmp=copy; items.sort=sort; items.map=map; return 0;
invalid:
    if (reason && capacity) snprintf(reason,capacity,"verified Golden item sort/range cache is unavailable");
    return -1;
}
void p4_golden_items_reset(void)
{
    if (items.cmp && !items.active &&
        p4_game_tables_battle_complete_unregister("golden-items",prepare,commit,NULL)) abort();
    free(items.cmp); memset(&items,0,sizeof items);
}
int32_t p4_golden_item_native_category(uint32_t id)
{
    ready();
    if (id>=2560) p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    return p4_golden_item_category(datEntryGetKind((int16_t)id));
}
int32_t p4_golden_item_native_aux_index(uint32_t id)
{
    ready();
    if (id>=2560) return -1;
    return p4_golden_item_aux_index(datEntryGetKind((int16_t)id),id);
}
int32_t p4_golden_item_native_route(uint32_t id)
{
    ready();
    if (id>=2560) return P4_GOLDEN_ITEM_UNAVAILABLE;
    const P4ItemRow *row=(const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    uint32_t golden=0;
    if (row->granted_skill && p4_golden_domain_from_port(&items.map,P4_GOLDEN_DOMAIN_SKILL,row->granted_skill,&golden))
        return P4_GOLDEN_ITEM_UNAVAILABLE;
    const uint8_t *skill=NULL;
    if (p4_golden_item_category(row->kind)==4 && golden<472) skill=p4_golden_skill_raw(row->granted_skill);
    return p4_golden_item_route(row->kind,golden,skill);
}
static uint32_t flags(void *unused,uint32_t id) { (void)unused; return datEntryGetFlags((int16_t)id); }
static uint32_t quantity(void *unused,uint32_t id) { (void)unused; return (uint8_t)datItemCount((int16_t)id); }
typedef struct Row { int16_t id, quantity; } Row;
typedef struct Output { Row *rows; uint32_t count, capacity; } Output;
static int emit(void *context,uint32_t id,uint32_t n)
{
    Output *out=context;
    if (out->count==out->capacity) return -1;
    out->rows[out->count++]=(Row){(int16_t)id,(int16_t)n}; return 0;
}
uint16_t p4_golden_item_native_inventory(void *rows,uint32_t capacity)
{
    ready(); Output out={rows,0,capacity};
    if (!rows || p4_golden_item_list(&items.sort,-1,flags,quantity,emit,&out)<0)
        p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    return (uint16_t)out.count;
}
int p4_golden_item_native_teach(uint32_t id,uint16_t *slots,int complete,int success,uint16_t old)
{
    ready();
    if (id>=2560) return -2;
    const P4ItemRow *row=(const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    uint8_t count=(uint8_t)datItemCount((int16_t)id), before=count;
    int result=p4_golden_item_teach(&items.map,row->kind,row->granted_skill,slots,&count,complete,success,old);
    if (before!=count) datItemSetCount((int16_t)id,count);
    return result;
}
_Noreturn void p4_golden_item_native_return_pending(void)
{
    p4_package_boot_runtime_failed("R7a-dep:M11-return-field-flow");
}
