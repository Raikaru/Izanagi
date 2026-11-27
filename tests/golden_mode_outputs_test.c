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
    static uint8_t skill_names[624*23], enemy_names[368*21], persona_names[256*21], item_names[2560*24];
    static uint8_t item_header[16], small[8] = {42};
    static uint8_t model0[11 * 382], model1[368 * 232], model2[256 * 88];
    static uint8_t raw_skill[472 * 44], raw_effect[472 * 12];
    static P4GoldenModeIdentityArtifacts artifacts;
    P4GoldenModeProjection p = {0}; P4GoldenSource source = {0}; P4GoldenExtraSource extra = {0};
    P4GoldenModeOutputs out = {0}; P4GoldenDomainMap map; char reason[128] = "";
    const P4GoldenDomainShape shape = {624,472,368,2560,256};
    CHECK(!p4_golden_domain_map_build(&map,&shape,reason,sizeof reason));
    p.skill_elements=(P4GoldenModeBytes){elements,sizeof elements}; p.skill_metadata=(P4GoldenModeBytes){skills,sizeof skills};
    p.enemy_metadata=(P4GoldenModeBytes){enemies,sizeof enemies}; p.item_metadata=(P4GoldenModeBytes){items,sizeof items};
    p.skill_names=(P4GoldenModeBytes){skill_names,sizeof skill_names}; p.enemy_names=(P4GoldenModeBytes){enemy_names,sizeof enemy_names};
    p.persona_names=(P4GoldenModeBytes){persona_names,sizeof persona_names}; p.item_names=(P4GoldenModeBytes){item_names,sizeof item_names};
    for (unsigned i=0;i<3;++i) p.persona[i]=(P4GoldenModeBytes){small,sizeof small};
    for (unsigned i=0;i<5;++i) p.encounter[i]=(P4GoldenModeBytes){small,sizeof small};
    P4GoldenTable *u=&source.tables[P4_GOLDEN_UNIT], *pa=&source.tables[P4_GOLDEN_PERSONA];
    u->span_count=3; u->spans[1]=(P4GoldenSpan){affinity,sizeof affinity}; u->spans[2]=(P4GoldenSpan){persona_affinity,sizeof persona_affinity};
    pa->span_count=6; pa->spans[0]=(P4GoldenSpan){persona0,sizeof persona0}; pa->spans[3]=(P4GoldenSpan){persona3,sizeof persona3}; pa->spans[5]=(P4GoldenSpan){persona5,sizeof persona5};
    P4GoldenTable *msg=&source.tables[P4_GOLDEN_MSG]; msg->span_count=5;
    msg->spans[0]=(P4GoldenSpan){small,sizeof small}; msg->spans[4]=(P4GoldenSpan){small,sizeof small};
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
    CHECK(out.table_count==33+P4_GOLDEN_IMAGE_EXTRA_COUNT && out.extra_count==24+P4_GOLDEN_IMAGE_EXTRA_COUNT &&
          !lookup(&out,"decl/persona-rows"));
    CHECK(lookup(&out,"image/experience")->size==396 && lookup(&out,"image/recipes")->size==17*27);
    CHECK(lookup(&out,"image-index.json")->size>0);
    for (size_t i=1;i<out.extra_count;++i)
        CHECK(strcmp(out.extra[i-1].name,out.extra[i].name)<0);
    t=lookup(&out,"battle/ENCOUNT.TBL"); CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==5);
    t=lookup(&out,"battle/MSG.TBL"); CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8)==5);
    CHECK(spans[1].size==624*23 && spans[2].size==368*21 && spans[3].size==256*21);
    CHECK(lookup(&out,"decl/item-names")->data==item_names && lookup(&out,"decl/item-names")->size==sizeof item_names);
    t=lookup(&out,"init/itemtbl.bin"); CHECK(t && t->size==16+1536*68 && t->data[0]==0 && t->data[1]==6 && t->data[16]==4);
    t=lookup(&out,"decl/skill-aux"); CHECK(t && t->size==624 && t->data[0]==7);
    CHECK(lookup(&out,"decl/item-metadata")->data==items && lookup(&out,"decl/enemy-affinity")->data==affinity);
    CHECK(!lookup(&out,"battle/EFFECT.TBL")->data && !lookup(&out,"battle/MODEL.TBL")->data &&
          !lookup(&out,"battle/AICALC.TBL")->data && !lookup(&out,"decl/text")->data && !lookup(&out,"decl/item-icons")->data);
    CHECK(!p4_golden_mode_outputs_set(&out,"decl/text",small,sizeof small));
    CHECK(p4_golden_mode_outputs_set(&out,"decl/text",small,sizeof small)==-1);
    CHECK(p4_golden_mode_outputs_set(&out,"unknown",small,sizeof small)==-1);
    p4_golden_mode_outputs_free(&out);
    for (unsigned i = 0; i < sizeof model0; ++i) model0[i] = (uint8_t)(i * 13u + 7u);
    P4GoldenExtraTable *model = &extra.tables[P4_GOLDEN_EXTRA_MODEL];
    model->span_count = 3;
    model->spans[0] = (P4GoldenSpan){model0, sizeof model0};
    model->spans[1] = (P4GoldenSpan){model1, sizeof model1};
    model->spans[2] = (P4GoldenSpan){model2, sizeof model2};
    elements[623 * 2u] = 0xFF; elements[623 * 2u + 1u] = 0xA5;
    p.skill_golden = (P4GoldenSpan){raw_skill, sizeof raw_skill};
    extra.tables[P4_GOLDEN_EXTRA_EFFECT].span_count = 1;
    extra.tables[P4_GOLDEN_EXTRA_EFFECT].spans[0] = (P4GoldenSpan){raw_effect, sizeof raw_effect};
    for (unsigned i = 0; i < sizeof raw_skill; ++i) raw_skill[i] = (uint8_t)(i * 19u + 0xa5u);
    for (unsigned i = 0; i < sizeof raw_effect; ++i) raw_effect[i] = (uint8_t)(i * 23u + 0xc3u);
    CHECK(!p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason));
    t = lookup(&out, "battle/MODEL.TBL");
    CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8) == 3);
    CHECK(spans[0].size == sizeof model0 && !memcmp(spans[0].bytes, model0, sizeof model0));
    CHECK(spans[1].size == sizeof model1 && spans[2].size == sizeof model2);
    t = lookup(&out, "decl/skill-elements");
    CHECK(t && t->size == 624 * 4u && !memcmp(t->data + 623 * 4u, "\xff\xff\xff\xff", 4));
    CHECK(lookup(&out,"decl/skill-groups")->data[623] == 18);
    CHECK(lookup(&out,"decl/skill-aux")->data[623] == 0xA5);
    t = lookup(&out, "rows/skill1");
    CHECK(t && t->size == sizeof raw_skill && !memcmp(t->data, raw_skill, sizeof raw_skill));
    t = lookup(&out, "rows/effect0");
    CHECK(t && t->size == sizeof raw_effect && !memcmp(t->data, raw_effect, sizeof raw_effect));
    t = lookup(&out, "battle/EFFECT.TBL");
    CHECK(t && p4_golden_tbl_spans(t->data,t->size,spans,8) == 1 && spans[0].size == 440 * 4);
    const P4GoldenCacheTable *presentation = lookup(&out, "decl/skill-presentation");
    CHECK(presentation && presentation->size == 608 * 4);
    for (uint32_t id = 0; id < 472; ++id) {
        uint32_t port;
        CHECK(!p4_golden_domain_to_port(&map,P4_GOLDEN_DOMAIN_SKILL,id,&port,NULL));
        CHECK(!memcmp(presentation->data + port * 4, raw_effect + id * 12, 2));
        CHECK(!memcmp(presentation->data + port * 4 + 2, raw_effect + id * 12 + 4, 2));
        if (id < 440) CHECK(!memcmp(spans[0].bytes + id * 4,presentation->data + port * 4,4));
    }
    p4_golden_mode_outputs_free(&out);
    --model->spans[0].size;
    CHECK(p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason) == -1);
    ++model->spans[0].size;
    static uint8_t bf[100] = {[4]=100,[8]='F',[9]='L',[10]='W',[11]='0',
        [16]=2,[36]=32,[40]=1,[44]=64,[48]=2,[52]=4,[56]=1,[60]=96,[96]=9};
    for (unsigned i=0;i<8;++i)
        extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[i]=(P4GoldenSpan){small,sizeof small};
    extra.tables[P4_GOLDEN_EXTRA_AICALC].span_count=11;
    extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[9]=(P4GoldenSpan){bf,sizeof bf};
    extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[10]=(P4GoldenSpan){bf,sizeof bf};
    CHECK(!p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason));
    P4GoldenSpan installed[12];
    t=lookup(&out,"battle/AICALC.TBL");
    CHECK(t && p4_golden_tbl_spans(t->data,t->size,installed,12)==11);
    CHECK(installed[8].size==336*164 && installed[9].size==sizeof bf &&
        installed[10].size==sizeof bf && !memcmp(installed[9].bytes,bf,sizeof bf) &&
        !memcmp(installed[10].bytes,bf,sizeof bf));
    CHECK(out.owned[9] && out.owned[10] && out.owned[14]);
    CHECK(lookup(&out,"image-index.json")->data == out.owned[9] ||
          lookup(&out,"image-index.json")->data == out.owned[10]);
    p4_golden_mode_outputs_free(&out);
    artifacts.inheritance_size=1;
    CHECK(p4_golden_mode_outputs_prepare(&out,&map,&source,&extra,&p,&artifacts,reason,sizeof reason)==-1);
    puts("golden_mode_outputs: ok"); return 0;
}
