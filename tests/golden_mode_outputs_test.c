/* Synthetic approved rows and intentionally absent capabilities; no game data. */
#include "../src/platform/golden/golden_mode_outputs.h"
#include "p4pc/game_table_inputs.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s (%s)\n",__LINE__,#x,reason); return 1; } } while (0)
static const char *const labels[] = {"init/itemtbl.bin", "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/MSG.TBL", "battle/PERSONA.TBL", "battle/ENCOUNT.TBL", "battle/EFFECT.TBL", "battle/MODEL.TBL", "battle/AICALC.TBL"};
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{ return (unsigned)kind < P4_TABLE_INPUT_COUNT ? labels[kind] : NULL; }
static const P4GoldenCacheTable *lookup(const P4GoldenModeOutputs *o, const char *name)
{
    for (size_t i=0;i<o->table_count;++i)
        if (!strcmp(o->tables[i].name,name)) return &o->tables[i];
    return NULL;
}
int main(void)
{
    static uint8_t elements[624*2], skills[608*40], enemies[368*60], items[2560*68];
    static uint8_t affinity[368*32], persona_affinity[256*32], enemy_ai[368*164];
    static uint8_t persona0[256*14], persona3[9*392], persona5[256*4];
    static uint8_t item_header[16], small[8] = {42};
    static P4GoldenModeIdentityArtifacts artifacts;
    P4GoldenModeProjection p = {0}; P4GoldenSource source = {0}; P4GoldenExtraSource extra = {0};
    P4GoldenModeOutputs out = {0}; P4GoldenDomainMap map; char reason[128] = "";
    const P4GoldenDomainShape shape = {624,472,368,2560,256};
    CHECK(!p4_golden_domain_map_build(&map,&shape,reason,sizeof reason));
    p.skill_elements=(P4GoldenModeBytes){elements,sizeof elements}; p.skill_metadata=(P4GoldenModeBytes){skills,sizeof skills};
    p.enemy_metadata=(P4GoldenModeBytes){enemies,sizeof enemies}; p.item_metadata=(P4GoldenModeBytes){items,sizeof items};
    for (unsigned i=0;i<3;++i) p.persona[i]=(P4GoldenModeBytes){small,sizeof small};
    for (unsigned i=0;i<5;++i) p.encounter[i]=(P4GoldenModeBytes){small,sizeof small};
    P4GoldenTable *u=&source.tables[P4_GOLDEN_UNIT], *pa=&source.tables[P4_GOLDEN_PERSONA];
    u->span_count=3; u->spans[1]=(P4GoldenSpan){affinity,sizeof affinity}; u->spans[2]=(P4GoldenSpan){persona_affinity,sizeof persona_affinity};
    pa->span_count=6; pa->spans[0]=(P4GoldenSpan){persona0,sizeof persona0}; pa->spans[3]=(P4GoldenSpan){persona3,sizeof persona3}; pa->spans[5]=(P4GoldenSpan){persona5,sizeof persona5};
    source.tables[P4_GOLDEN_ITEM].bytes=item_header; source.tables[P4_GOLDEN_ITEM].size=sizeof item_header;
    extra.tables[P4_GOLDEN_EXTRA_AICALC].span_count=9;
    extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[8]=(P4GoldenSpan){enemy_ai,sizeof enemy_ai};
    artifacts.inheritance_size=624; elements[1]=7; skills[0]=3; items[0]=4; enemies[0]=5;
    CHECK(!p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason));
    const P4GoldenCacheTable *t=lookup(&out,"battle/SKILL.TBL"); P4GoldenSpan spans[8];
    CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==2);
    CHECK(spans[0].size==576*2 && spans[0].bytes[1]==7 && spans[1].size==440*40 && spans[1].bytes[0]==3);
    t=lookup(&out,"battle/UNIT.TBL"); CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==3);
    CHECK(spans[0].size==336*60 && spans[0].bytes[0]==5 && spans[1].size==336*32 && spans[2].size==256*32);
    t=lookup(&out,"battle/PERSONA.TBL"); CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==6);
    CHECK(out.table_count==23 && out.extra_count==14 && !lookup(&out,"decl/persona-rows"));
    t=lookup(&out,"battle/ENCOUNT.TBL"); CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==5);
    t=lookup(&out,"init/itemtbl.bin"); CHECK(t && t->size==16+1536*68 && t->data[0]==0 && t->data[1]==6 && t->data[16]==4);
    t=lookup(&out,"decl/skill-aux"); CHECK(t && t->size==624 && t->data[0]==7);
    CHECK(lookup(&out,"decl/item-metadata")->data==items && lookup(&out,"decl/enemy-affinity")->data==affinity);
    CHECK(!lookup(&out,"battle/EFFECT.TBL")->data && !lookup(&out,"battle/MODEL.TBL")->data &&
          !lookup(&out,"battle/AICALC.TBL")->data && !lookup(&out,"decl/text")->data && !lookup(&out,"decl/item-icons")->data);
    CHECK(!p4_golden_mode_outputs_set(&out,"decl/text",small,sizeof small));
    CHECK(p4_golden_mode_outputs_set(&out,"decl/text",small,sizeof small)==-1);
    CHECK(p4_golden_mode_outputs_set(&out,"unknown",small,sizeof small)==-1);
    p4_golden_mode_outputs_free(&out);
    artifacts.inheritance_size=1;
    CHECK(p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason)==-1);
    puts("golden_mode_outputs: ok"); return 0;
}
