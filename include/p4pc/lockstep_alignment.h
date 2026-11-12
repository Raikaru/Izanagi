/* State-aligned observations. Guest counters and state are never changed here. */
#ifndef P4PC_LOCKSTEP_ALIGNMENT_H
#define P4PC_LOCKSTEP_ALIGNMENT_H
#include "p4pc/lockstep_format.h"
#include <stdio.h>
#define P4LA_MAGIC "P4ALGN1\n"
#define P4LA_META 3u
#define P4LA_PAD_GATE 4u
#define P4LA_FLAG P4LS_SAVED_STATE
#define P4LA_BOOT 0u
#define P4LA_LOADING 1u
#define P4LA_READY 2u
#define P4LA_LOCKED 3u
#define P4LA_FIELD 1u
#define P4LA_EVENT 2u
#define P4LA_COMMAND 3u
#define P4LA_BATTLE_ENTRY 4u
#define P4LA_META_WORDS 8u
#define P4LA_RNG_WORDS 663u
#define P4LA_MAX_CLOCKS 4096u
#define P4LA_EFFECT_BANK 0x00922de0u
#define P4LA_QUIET_UPDATES 8u
/* Metadata: epoch, phase, milestone kind/key, packed calendar, sequence,
 * field-root step, load-begin game counter. The record header keeps the real
 * game counter. Neither participant resets that counter to align a timeline.
 * effect_changed is the game frame whose update last changed the shared effect
 * bank, observed at presentations from RAM on both participants. */
typedef struct P4AlignTracker {
    uint32_t epoch, loading, anchored, begin, last_frame;
    uint32_t sequence, root, battle, event;
    uint32_t stable_kind, stable_key, stable_calendar;
    uint32_t battle_qualification_id, battle_qualification_mode, battle_qualification_locked;
    uint32_t effect_bank[4], effect_changed, effect_seen;
} P4AlignTracker;
static inline int p4la_address(uint32_t address, uint32_t size) {
    return address>=0x00100000u && size<=0x01f00000u && address<=0x02000000u-size;
}
static inline uint32_t p4la_word(const P4WalkHost *h, uint32_t a) {
    return p4la_address(a,4) ? h->read32(h->user,a) : 0;
}
/* Optional per-call provenance: actual entry state, never a predicted result.
 * Native callers are ASLR host addresses; the file's named anchor permits
 * offline symbolization against the immutable runtime that made the trace. */
static inline int p4la_rng_header(FILE *file) {
    return fprintf(file,"draw\tgame_frame\tplaytime\tepoch\tphase\tcalendar\tsequence\tfield_step\tfield_requests\tengine_slot\tcaller\tfront\tback\twords\n")>0;
}
static inline int p4la_rng_call(FILE *file, const P4WalkHost *h,
                               const P4AlignTracker *tracker, uint32_t draw,
                               uint64_t caller) {
    const uint32_t offset=p4la_word(h,0x00764708u);
    const uint32_t slot=offset<0x10000u ? 0x008872e0u+offset : 0;
    const uint32_t base=slot ? p4la_word(h,slot) : 0;
    if (base<0x00100000u || base>0x03ffff84u) return 0;
    const uint32_t front=p4la_word(h,slot+4u)-base, back=p4la_word(h,slot+8u)-base;
    if (front>=124u || back>=124u || front%4u || back%4u || (front+124u-back)%124u!=12u) return 0;
    const uint32_t sw=p4la_word(h,p4la_word(h,0x00764298u)+0x38u);
    const uint32_t work=p4la_word(h,p4la_word(h,0x007d2510u)+0x38u);
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    if (fprintf(file,"%u\t%u\t%u\t%u\t%u\t%08x\t%u\t%u\t%08x\t%u\t%016llx\t%u\t%u\t",
        draw,p4la_word(h,P4LS_GAME_FRAME),p4la_word(h,0x0076428cu),tracker->epoch,
        !tracker->anchored ? P4LA_BOOT : tracker->loading ? P4LA_LOADING : P4LA_LOCKED,
        p4la_word(h,0x00797b78u),p4la_word(h,sw+4u),p4la_word(h,work),
        p4la_word(h,scene+12u),offset,(unsigned long long)caller,front,back)<0) return 0;
    for (unsigned i=0;i<31;i++)
        if (fprintf(file,"%s%08x",i ? "," : "",h->read32(h->user,base+i*4u))<0) return 0;
    return fputc('\n',file)!=EOF;
}
/* Global effect generator only: kind 0 advances, 1 is its float wrapper,
 * 2 installs an explicit seed. Wrapper entries preserve the ultimate caller. */
static inline int p4la_effect_header(FILE *file) {
    return fprintf(file,"call\tgame_frame\tplaytime\tepoch\tphase\tcalendar\tsequence\tfield_step\tfield_requests\tengine_slot\tcaller\tkind\targument\tstate\twords\n")>0;
}
static inline int p4la_effect_call(FILE *file, const P4WalkHost *h,
                                 const P4AlignTracker *tracker, uint32_t call,
                                 uint64_t caller, uint32_t kind, uint32_t argument) {
    const uint32_t sw=p4la_word(h,p4la_word(h,0x00764298u)+0x38u);
    const uint32_t work=p4la_word(h,p4la_word(h,0x007d2510u)+0x38u);
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    if (fprintf(file,"%u\t%u\t%u\t%u\t%u\t%08x\t%u\t%u\t%08x\t%u\t%016llx\t%u\t%08x\t00922de0\t",
        call,p4la_word(h,P4LS_GAME_FRAME),p4la_word(h,0x0076428cu),tracker->epoch,
        !tracker->anchored ? P4LA_BOOT : tracker->loading ? P4LA_LOADING : P4LA_LOCKED,
        p4la_word(h,0x00797b78u),p4la_word(h,sw+4u),p4la_word(h,work),
        p4la_word(h,scene+12u),p4la_word(h,0x00764708u),
        (unsigned long long)caller,kind,argument)<0) return 0;
    for (unsigned i=0;i<4;i++)
        if (fprintf(file,"%s%08x",i ? "," : "",p4la_word(h,0x00922de0u+i*4u))<0) return 0;
    return fputc('\n',file)!=EOF;
}
static inline uint32_t p4la_task(const P4WalkHost *h, const char *name) {
    static const uint32_t heads[]={0x00764adcu,0x00764af4u,0x00764ae8u};
    for (unsigned l=0;l<3;l++) {
        uint32_t node=p4la_word(h,heads[l]);
        for (unsigned i=0;p4la_address(node,0x40) && i<256;i++,node=p4la_word(h,node+0x3c)) {
            char label[25];
            for (unsigned j=0;j<24;j+=4) p4ls_put((uint8_t *)label+j,p4la_word(h,node+j));
            label[24]=0;
            if (!strcmp(label,name)) return node;
        }
    }
    return 0;
}

static inline int p4la_field_effect_task_idle(const P4WalkHost *h, uint32_t task) {
    if (!p4la_address(task,0x3cu)) return 0;
    const uint32_t work=p4la_word(h,task+0x38u);
    if (!p4la_address(work,0xfcu) || p4la_word(h,work)!=1u ||
        p4la_word(h,work+0x10u) || p4la_word(h,work+0x20u)) return 0;
    for (unsigned j=0;j<8;j++) if (p4la_word(h,work+0x24u+j*4u)) return 0;
    return 1;
}

/* Check every FldRc task, not only FldStepWork's two preloads, and reject
 * installed field-resource effect trees throughout battle qualification. */
static inline int p4la_battle_field_effects_idle(const P4WalkHost *h) {
    static const uint32_t heads[]={0x00764adcu,0x00764af4u,0x00764ae8u};
    for (unsigned l=0;l<3;l++) {
        uint32_t node=p4la_word(h,heads[l]);
        unsigned i=0;
        for (;node && i<256;i++,node=p4la_word(h,node+0x3cu)) {
            if (!p4la_address(node,0x40u)) return 0;
            char label[25];
            for (unsigned j=0;j<24;j+=4) p4ls_put((uint8_t *)label+j,p4la_word(h,node+j));
            label[24]=0;
            if (strcmp(label,"epl(kosaka)")) continue;
            if (!p4la_field_effect_task_idle(h,node)) return 0;
        }
        if (node) return 0;
    }
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    if (!p4la_address(scene,0x88u)) return 0;
    /* scene+0x28 is an owned array only while the scene is loaded, and only
     * through resourceCount. Entries beyond it are stale freed aliases. */
    const uint32_t scene_count=p4la_word(h,scene+8u) ? p4la_word(h,scene+0x24u) : 0;
    if (scene_count>24u) return 0;
    for (unsigned i=0;i<16u+scene_count;i++) {
        const uint32_t slot=i<16 ? 0x007e8060u+i*4u : scene+0x28u+(i-16u)*4u;
        const uint32_t resource=p4la_word(h,slot);
        if (!resource) continue;
        if (!p4la_address(resource,0xa20u)) return 0;
        const uint32_t count=p4la_word(h,resource+0x11cu);
        if (count>96u) return 0;
        for (uint32_t j=0;j<count;j++) {
            const uint32_t entry=resource+0x120u+j*0x18u;
            if ((p4la_word(h,entry)&0xffffu)==1u && p4la_word(h,entry+0x10u)) return 0;
        }
    }
    return 1;
}
/* Effect emitters advanced in one update: mdlEffectAdvanceEmitter (00492d10) receives each
 * EffTrackPath and increments its +0x10 ticks. Both participants record the guest address at
 * that call entry (read-only), take the list at each presentation, and retain it with the
 * ready RAM, so the checkpoint can name every emitter the boundary's own update advanced. */
#define P4LA_EMITTER_ADVANCE 0x00492d10u
#define P4LA_EMITTERS 1024u
typedef struct P4AlignEmitters {
    uint32_t count, overflow, address[P4LA_EMITTERS];
} P4AlignEmitters;
static inline void p4la_emitter_seen(P4AlignEmitters *e, uint32_t emitter) {
    if (e->count<P4LA_EMITTERS) e->address[e->count++]=emitter;
    else e->overflow=1;
}
static inline int p4la_emitters_write(const char *path, const P4AlignEmitters *e, uint32_t frame) {
    FILE *f=fopen(path,"w");
    int ok;
    if (!f) return 0;
    ok=fprintf(f,"# p4la-emitters 1 frame %u count %u overflow %u\n",frame,e->count,e->overflow)>0;
    for (uint32_t i=0;ok && i<e->count;i++) ok=fprintf(f,"%08x\n",e->address[i])>0;
    return fclose(f)==0 && ok;
}
/* Per-presentation event-player provenance is separate from elapsed-clock
 * policy: logging a loader/timeline word never authorizes resynchronizing it. */
static inline int p4la_event_header(FILE *file) {
    return fprintf(file,"game_frame\tpresent\twork\twait_flags\tscene_source\tframe_count\tstart_frame\tframe_limit\tframe\tlast_frame\treset_requested\treset_frame\tflags44\tnode_count\tinitialized\tbusy\tload_state\tload_flags\tpending\treader_handle\tstream_task\tevent0\tevent1\tevent2\n")>0;
}
static inline int p4la_event_frame(FILE *file, const P4WalkHost *h) {
    static const uint32_t offsets[]={0,4,8,12,16,20,24,28,32,0x44,0x48,0x54,
                                    0x5d4,0x76c,0x770,0x774,0x77c,0x780,0x78c,0x790,0x794};
    const uint32_t task=p4la_task(h,"evtEditLoad");
    const uint32_t work=task ? p4la_word(h,task+0x38u) : 0;
    if (task && !p4la_address(work,0x7b0)) return 0;
    if (fprintf(file,"%u\t%u\t%u",p4la_word(h,P4LS_GAME_FRAME),!!task,work)<0) return 0;
    for (unsigned i=0;i<sizeof offsets/sizeof *offsets;i++) {
        uint32_t value=task ? p4la_word(h,work+offsets[i]) : 0;
        if (offsets[i]==0x44u) value&=255u;
        if (fprintf(file,"\t%u",value)<0) return 0;
    }
    return fputc('\n',file)!=EOF;
}
/* Per-presentation battle UID provenance: btlGetUID's 64-bit counter
 * (gp-0x5f70), the action id counter (gp-0x5f48) and every live packet as
 * list:uid:type:action. Read-only evidence for locating an extra or missing
 * packet/action allocation; a list that is not walkable is written as
 * "unwalkable" rather than stopping the participant. */
static inline int p4la_packet_header(FILE *file) {
    return fprintf(file,"game_frame\tuid_counter_high\tuid_counter\taction_counter\tpackets\n")>0;
}
static inline int p4la_packet_frame(FILE *file, const P4WalkHost *h) {
    if (fprintf(file,"%u\t%u\t%u\t%u\t",p4la_word(h,P4LS_GAME_FRAME),p4la_word(h,0x00763184u),
                p4la_word(h,0x00763180u),p4la_word(h,0x007631a8u))<0) return 0;
    const uint32_t battle=p4la_word(h,0x0076449cu);
    if (p4la_task(h,"battle") && p4la_address(battle,0x1b8u)) {
        const char *separator="";
        for (unsigned list=0;list<4;list++) {
            uint32_t packet=p4la_word(h,battle+0x198u+list*8u);
            for (unsigned count=0;packet;count++,packet=p4la_word(h,packet+0x80u)) {
                if (count==1024 || !p4la_address(packet,0x90u))
                    return fprintf(file,"%sunwalkable",separator)>0 && fputc('\n',file)!=EOF;
                if (fprintf(file,"%s%u:%u:%u:%u",separator,list,p4la_word(h,packet+0x58u),
                            p4la_word(h,packet+0x40u),p4la_word(h,packet+0x60u))<0) return 0;
                separator=",";
            }
        }
    }
    return fputc('\n',file)!=EOF;
}
/* Per-presentation message-window provenance, read-only evidence never used by a verdict.
 * itfMesManager's globals are at 0x008817e0 (active entry list at +0xc, entry next +0x4, window
 * +0xc; decomp/include/itf_mes_objects.h). Each active window is written as
 * flags:kind:row state:row flags:page index:page count:elapsed:transition:runs:enabled runs:
 * glyphs:revealed:glyph frames:last command:last duration:last enabled. The text runs are walked
 * from the row list's root like frFontMeasureList (run next +0x28, glyphs from +0x1c by +0x28):
 * `revealed` sums each glyph's intensity (its colour's low byte) and `glyph frames` its frame
 * halfword, the typewriter position. The last run's command and duration are the page wait
 * (0xf223/0xf226 with duration -1 wait while voice channel 1 plays). Voice channel 1 is sdkSnd's
 * SndCh slot at 0x008d2b90 + 0x134 (active, fading, state, request mode, whether it has a handle).
 * The bust-up banks (decomp/include/fcl_bank_internal.h: manager at 0x00764678, its pending
 * request list at +0x4, record at +0x24 with two 0x14-byte slots from +0x4; displays at 0x00796700,
 * 0x370 apart) are written as pending;flags:state:descriptor:fade ticks:display state:alpha:
 * timers 0x364/0x368/0x36c for both slots, or "-" without a manager. A list that is not walkable
 * is written as "unwalkable". */
#define P4LA_MES_MANAGER 0x008817e0u
#define P4LA_SND_VOICE 0x008d2cc4u
#define P4LA_BUSTUP_MANAGER 0x00764678u
#define P4LA_BUSTUP_DISPLAYS 0x00796700u
static inline uint32_t p4la_half(const P4WalkHost *h, uint32_t a) {
    return (p4la_word(h,a&~3u)>>((a&2u)*8u))&0xffffu;
}
static inline uint32_t p4la_byte(const P4WalkHost *h, uint32_t a) {
    return (p4la_word(h,a&~3u)>>((a&3u)*8u))&0xffu;
}
static inline int p4la_message_header(FILE *file) {
    return fprintf(file,"game_frame\twindow_count\tmanager_flags\tvoice_active\tvoice_fading\tvoice_state\t"
                   "voice_mode\tvoice_handle\tbustup\twindows\n")>0;
}
static inline int p4la_bustup(FILE *file, const P4WalkHost *h) {
    const uint32_t work=p4la_word(h,P4LA_BUSTUP_MANAGER);
    const uint32_t record=p4la_address(work,0x28u) ? p4la_word(h,work+0x24u) : 0;
    if (!p4la_address(work,0x28u) || !p4la_address(record,0x30u)) return fputc('-',file)!=EOF;
    if (fprintf(file,"%u",(unsigned)(p4la_word(h,work+4u)!=0))<0) return 0;
    for (unsigned i=0;i<2;i++) {
        const uint32_t slot=record+4u+i*0x14u, display=P4LA_BUSTUP_DISPLAYS+i*0x370u;
        if (fprintf(file,";%x:%d:%x:%d:%d:%u:%u:%u:%u",p4la_half(h,slot),(int)(int16_t)p4la_half(h,slot+2u),
                    p4la_word(h,slot+4u),(int)(int16_t)p4la_half(h,slot+0x10u),
                    (int)(int16_t)p4la_half(h,display),p4la_byte(h,display+0x1fu),p4la_word(h,display+0x364u),
                    p4la_word(h,display+0x368u),p4la_word(h,display+0x36cu))<0) return 0;
    }
    return 1;
}
static inline int p4la_message_window(FILE *file, const P4WalkHost *h, uint32_t window) {
    uint32_t runs=0, enabled=0, glyphs=0, revealed=0, frames=0, command=0, duration=0, last_enabled=0;
    const uint32_t list=p4la_word(h,window+0x20u);
    if (list) {
        if (!p4la_address(list,0x44u)) return 0;
        for (uint32_t run=p4la_word(h,list+0x2cu); run; run=p4la_word(h,run+0x28u)) {
            if (runs==256u || !p4la_address(run,0x44u)) return 0;
            runs++;
            last_enabled=p4la_word(h,run+0x40u);
            enabled+=last_enabled!=0;
            command=p4la_word(h,run+0x30u);
            duration=p4la_word(h,run+0x3cu);
            for (uint32_t glyph=p4la_word(h,run+0x1cu); glyph; glyph=p4la_word(h,glyph+0x28u)) {
                if (glyphs==4096u || !p4la_address(glyph,0x2cu)) return 0;
                glyphs++;
                revealed+=p4la_word(h,glyph+0x10u)&0xffu;
                frames+=p4la_half(h,glyph+2u);
            }
        }
    }
    return fprintf(file,"%x:%u:%d:%u:%d:%d:%u:%d:%u:%u:%u:%u:%u:%x:%d:%u",p4la_word(h,window),
                   p4la_word(h,window+0x10u),(int)(int8_t)p4la_byte(h,window+0x24u),p4la_byte(h,window+0x25u),
                   (int)(int16_t)p4la_half(h,window+0x2cu),(int)(int16_t)p4la_half(h,window+0x2eu),
                   p4la_word(h,window+0x30u),(int)(int16_t)p4la_half(h,window+0x34u),runs,enabled,glyphs,
                   revealed,frames,command,(int)duration,last_enabled)>0;
}
static inline int p4la_message_frame(FILE *file, const P4WalkHost *h) {
    const uint32_t voice=P4LA_SND_VOICE;
    if (fprintf(file,"%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\t",p4la_word(h,P4LS_GAME_FRAME),
                p4la_word(h,P4LA_MES_MANAGER),p4la_half(h,P4LA_MES_MANAGER+8u),p4la_word(h,voice),
                p4la_word(h,voice+4u),(int)(int16_t)p4la_half(h,voice+0xcu),(int)(int16_t)p4la_half(h,voice+0x14u),
                (unsigned)(p4la_word(h,voice+0x10u)!=0))<0) return 0;
    if (!p4la_bustup(file,h) || fputc('\t',file)==EOF) return 0;
    const char *separator="";
    uint32_t entry=p4la_word(h,P4LA_MES_MANAGER+0xcu);
    for (unsigned count=0;entry;count++,entry=p4la_word(h,entry+4u)) {
        const uint32_t window=p4la_word(h,entry+0xcu);
        if (count==64 || !p4la_address(entry,0x20u) || (window && !p4la_address(window,0x130u)))
            return fprintf(file,"%sunwalkable",separator)>0 && fputc('\n',file)!=EOF;
        if (!window) continue;
        if (fputs(separator,file)==EOF) return 0;
        if (!p4la_message_window(file,h,window)) return fprintf(file,"unwalkable")>0 && fputc('\n',file)!=EOF;
        separator=",";
    }
    return fputc('\n',file)!=EOF;
}
/* Before-input title/card checkpoints preserve confirmation edges while either
 * participant waits for its own card I/O. They are transport metadata only;
 * the first field-ready boundary still requires the complete state proof. */
static inline int p4la_title_gate(const P4WalkHost *h, uint32_t out[8]) {
    const uint32_t sw=p4la_word(h,p4la_word(h,0x00764298u)+0x38u);
    if (p4la_word(h,sw+4u)!=27u) return 0;
    const uint32_t task=p4la_task(h,"titleDrawProc");
    const uint32_t work=p4la_word(h,task+0x38u);
    if (!task || !p4la_address(work,0x24)) return 0;
    memset(out,0,8*sizeof *out);
    out[0]=27u;
    out[1]=p4la_word(h,work);
    out[2]=p4la_word(h,work+0x18u);
    out[3]=p4la_word(h,work+0x20u);
    return 1;
}
static inline uint32_t p4la_movie_work(const P4WalkHost *h) {
    const uint32_t task=p4la_task(h,"evtEditLoad"), work=p4la_word(h,task+0x38u);
    return task && p4la_address(work,0x7b0) ? work : 0;
}
static inline int p4la_movie_ready(const P4WalkHost *h, uint32_t work) {
    return work && p4la_address(p4la_word(h,work+4u),0x48) &&
        p4la_word(h,work+8u) && p4la_word(h,work+0x76cu)<=1u &&
        !(p4la_word(h,work+0x770u)&1u) && !(p4la_word(h,work)&1u);
}
static inline void p4la_milestone(const P4WalkHost *h, uint32_t out[8]) {
    memset(out,0,8*sizeof *out);
    const uint32_t st=p4la_word(h,0x00764298u), sw=p4la_word(h,st+0x38u);
    const uint32_t sequence=p4la_word(h,sw+4u);
    const uint32_t root=p4la_word(h,0x007d2510u), work=p4la_word(h,root+0x38u);
    const uint32_t scene=p4la_word(h,0x00762ea0u), manager=p4la_word(h,scene+8u);
    out[4]=p4la_word(h,0x00797b78u);
    out[5]=sequence;
    out[6]=p4la_word(h,work);
    if (!p4la_address(sw,20) || (p4la_word(h,sw)&1u) || p4la_word(h,sw+12u)) return;
    if (p4la_task(h,"battle")) {
        const uint32_t c=p4la_word(h,0x0076449cu), action=p4la_word(h,c+0x104u);
        const uint32_t unit=p4la_word(h,action+0x30u);
        if (p4la_word(h,c+0x2fcu)!=8u || (p4la_word(h,c+12u)&0x400000u) ||
            (p4la_word(h,action+12u)&0xffffu)!=5u) return;
        uint32_t p=p4la_word(h,c+0x178u);
        for (unsigned i=0;i<8 && p4la_address(p,0xa70);i++,p=p4la_word(h,p+0xa6cu)) {
            if (p==unit) { out[2]=P4LA_COMMAND; out[3]=i; return; }
        }
        return;
    }
    if ((sequence==9u || sequence==11u) && p4la_address(work,0x24) && p4la_address(manager,88) &&
        out[6]>=12u && out[6]<=14u && !(p4la_word(h,scene+12u)&6u)) {
        const uint32_t ids=(p4la_word(h,scene)&0xffffu)|(p4la_word(h,scene+4u)<<16);
        if (p4la_word(h,work+0x18u)==ids && p4la_word(h,manager+4u)) {
            out[2]=P4LA_FIELD; out[3]=ids; return;
        }
    }
    if (sequence==10u && !(p4la_word(h,scene+12u)&6u)) {
        const uint32_t movie=p4la_movie_work(h);
        if (p4la_movie_ready(h,movie)) {
            uint32_t key=2166136261u;
            for (unsigned i=0;i<3;i++) key=p4ls_hash_word(key,p4la_word(h,movie+0x78cu+i*4u));
            key=p4ls_hash_word(key,p4la_word(h,movie+8u));
            out[2]=P4LA_EVENT; out[3]=key;
        }
    }
}
/* The collision transition is complete only when its named 30-update owner
 * is inactive and every other field effect task is also idle. Its template can
 * remain allocated; later activation is trapped by the qualification guard. */
static inline int p4la_collision_transition_done(const P4WalkHost *h) {
    static const uint32_t heads[]={0x00764adcu,0x00764af4u,0x00764ae8u};
    unsigned transitions=0;
    for (unsigned l=0;l<3;l++) {
        uint32_t node=p4la_word(h,heads[l]);
        unsigned i=0;
        for (;node && i<256;i++,node=p4la_word(h,node+0x3cu)) {
            if (!p4la_address(node,0x40u)) return 0;
            char label[25];
            for (unsigned j=0;j<24;j+=4) p4ls_put((uint8_t *)label+j,p4la_word(h,node+j));
            label[24]=0;
            if (strcmp(label,"epl(kosaka)")) continue;
            if (!p4la_field_effect_task_idle(h,node)) return 0;
            const uint32_t work=p4la_word(h,node+0x38u);
            char path[65];
            for (unsigned j=0;j<64;j+=4) p4ls_put((uint8_t *)path+j,p4la_word(h,work+0xbcu+j));
            path[64]=0;
            if (!strcmp(path,"field/ENCONT_N.EPL")) {
                if (p4la_word(h,work+8u)!=30u || p4la_word(h,work+0x18u)!=30u) return 0;
                transitions++;
            }
        }
        if (node) return 0;
    }
    return transitions==1u;
}
/* The attack (0x12), skill (0x13) and summon (0x14) handlers build their whole
 * presentation in one update and leave the state in that same update. An action
 * already in one of those states, or whose pending transition into one expires
 * in the next update, therefore has not run its execution build yet. */
static inline int p4la_first_action(const P4WalkHost *h) {
    const uint32_t context=p4la_word(h,0x0076449cu);
    if (!p4la_address(context,0x300u)) return 0;
    uint32_t action=p4la_word(h,context+0x174u);
    for (unsigned i=0;action && i<256;i++,action=p4la_word(h,action+0x450u)) {
        if (!p4la_address(action,0x458u)) return 0;
        const uint32_t states=p4la_word(h,action+0xcu), timers=p4la_word(h,action+0x10u);
        const uint32_t current=states&0xffffu, pending=states>>16;
        if ((current>=0x12u && current<=0x14u) ||
            (pending>=0x12u && pending<=0x14u && (timers>>16)==1u)) return 1;
    }
    return 0;
}
/* Scripted requests anchor before construction. Collision qualification
 * instead starts at the presentation before the first battle action's execution
 * build, after ENCONT_N completes, once the shared effect bank has been untouched
 * for P4LA_QUIET_UPDATES updates: battle code draws that bank, while intro
 * emitters draw it through the load-wait, so earlier actions (an opening summon
 * during the intro) are skipped rather than anchored. */
static inline int p4la_battle_entry(const P4AlignTracker *t, const P4WalkHost *h) {
    if (!t->battle_qualification_id || t->battle_qualification_locked) return 0;
    const uint32_t task=p4la_task(h,"field event");
    const uint32_t work=p4la_word(h,task+0x38u);
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    if (!task || !p4la_address(work,0x160u) || !p4la_address(scene,12u)) return 0;
    const uint32_t actor=p4la_word(h,work+0x44u), group=p4la_word(h,work+0x64u);
    if (!p4la_address(actor,0x4cu) || !p4la_address(group,12u) ||
        p4la_word(h,actor+0x48u)!=group ||
        (p4la_word(h,group+8u)&0xffffu)+1u!=t->battle_qualification_id) return 0;
    if (t->battle_qualification_mode==2u)
        return p4la_word(h,work+4u)==10u && p4la_word(h,work+0x10cu) &&
               !p4la_word(h,scene+8u) && !p4la_task(h,"battle");
    return t->battle_qualification_mode==4u && p4la_word(h,work+4u)==11u &&
        !p4la_word(h,work+0x10cu) && p4la_task(h,"battle") && t->effect_seen &&
        p4la_word(h,P4LS_GAME_FRAME)-t->effect_changed>=P4LA_QUIET_UPDATES &&
        p4la_collision_transition_done(h) && p4la_first_action(h);
}
/* The animation state behind a model layer's mirrored time: its RtAnimInterpolator's
 * currentTime and pNextFrame, the per-node interpolated keyframes after the 0x4c-byte
 * header, and the HAnim pose matrices (pad lanes excluded). */
static inline int p4la_layer_animation_word(const P4WalkHost *h, uint32_t layer, uint32_t a) {
    const uint32_t hierarchy=p4la_word(h,layer+0x20u);
    if (!p4la_address(hierarchy,0x24u)) return 0;
    const uint32_t interp=p4la_word(h,hierarchy+0x20u);
    if (p4la_address(interp,0x4cu)) {
        const uint32_t size=p4la_word(h,interp+0x24u), nodes=p4la_word(h,interp+0x2cu);
        if (a==interp+4u || a==interp+8u) return 1;
        if (size>=8u && size<=0x100u && !(size&3u) && nodes && nodes<=256u &&
            a>=interp+0x4cu && a<interp+0x4cu+nodes*size)
            return !(a&3u) && p4la_address(interp,0x4cu+nodes*size);
    }
    const uint32_t matrices=p4la_word(h,hierarchy+8u), nodes=p4la_word(h,hierarchy+4u);
    if (nodes && nodes<=256u && a>=matrices && a<matrices+nodes*0x40u && p4la_address(matrices,nodes*0x40u)) {
        const uint32_t lane=((a-matrices)&0x3fu)>>2;
        return !(a&3u) && lane!=7u && lane!=11u && lane!=15u;
    }
    return 0;
}
/* Words a battle-qualification response may write on the native participant:
 * the shared effect bank, a battle unit's model-layer time/previous_time and the
 * animation state behind it, a battle action's state timer (the load-wait clocks), and
 * CRI's server-pass count with every open ADXT handle's start count (the clock the PS2RNA
 * model hears IOP blocks on; tools/lockstep_alignment.py cri_pass_clock_patches). */
static inline int p4la_battle_patch_word(const P4WalkHost *h, uint32_t a) {
    if (a>=P4LA_EFFECT_BANK && a<P4LA_EFFECT_BANK+16u) return !(a&3u);
    if (a==0x007188b8u) return 1;
    if (a>=0x007188c0u && a<0x007188c0u+16u*0xc8u && (a-0x007188c0u)%0xc8u==0xa0u)
        return (p4la_word(h,a-0xa0u)&0xffu)==1u;
    const uint32_t context=p4la_word(h,0x0076449cu);
    if (!p4la_address(context,0x300u)) return 0;
    for (unsigned g=0;g<3;g++) {
        uint32_t unit=p4la_word(h,context+0x178u+g*4u);
        for (unsigned i=0;unit && i<128;i++,unit=p4la_word(h,unit+0xa6cu)) {
            if (!p4la_address(unit,0xa70u)) return 0;
            const uint32_t model=p4la_word(h,unit+0xa00u);
            for (unsigned l=0;model && l<2;l++) {
                const uint32_t layer=model+0xecu+l*0xa4u;
                if (a==layer+0xcu || a==layer+0x14u) return p4la_address(layer,0xa4u);
                if (p4la_address(layer,0xa4u) && p4la_layer_animation_word(h,layer,a)) return 1;
            }
        }
    }
    uint32_t action=p4la_word(h,context+0x174u);
    for (unsigned i=0;action && i<256;i++,action=p4la_word(h,action+0x450u)) {
        if (!p4la_address(action,0x458u)) return 0;
        if (a==action+0x1cu) return 1;
    }
    return 0;
}
static inline void p4la_effect_quiet(P4AlignTracker *t, const P4WalkHost *h, uint32_t frame) {
    int changed=!t->effect_seen;
    for (unsigned i=0;i<4;i++) {
        const uint32_t word=p4la_word(h,P4LA_EFFECT_BANK+i*4u);
        changed|=word!=t->effect_bank[i];
        t->effect_bank[i]=word;
    }
    if (changed) { t->effect_changed=frame; t->effect_seen=1; }
}
static inline int p4la_same_milestone(const uint32_t a[8], const uint32_t b[8]) {
    return a[2]!=0 && a[2]==b[2] && a[3]==b[3] && a[4]==b[4] &&
        a[5]==b[5] && (a[2]!=P4LA_FIELD || a[6]==b[6]);
}
static inline int p4la_load_request(const P4AlignTracker *t, const P4WalkHost *h,
                                  const uint32_t out[8], uint32_t root,
                                  uint32_t battle, uint32_t event) {
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    const uint32_t sw=p4la_word(h,p4la_word(h,0x00764298u)+0x38u);
    const uint32_t movie=out[5]==10u ? p4la_movie_work(h) : 0;
    return (p4la_word(h,scene+12u)&6u) || (p4la_word(h,sw)&1u) ||
        out[5]!=t->sequence || root!=t->root || (!!battle)!=(!!t->battle) ||
        (out[5]==10u && event!=t->event) ||
        (movie && (p4la_word(h,movie+0x770u)&1u)) ||
        (out[2]==P4LA_EVENT && t->stable_kind==P4LA_EVENT && out[3]!=t->stable_key) ||
        (out[2]==P4LA_FIELD && t->stable_kind==P4LA_FIELD &&
         (out[3]!=t->stable_key || out[4]!=t->stable_calendar)) ||
        (t->stable_kind==P4LA_FIELD && root && out[6]<12u);
}
static inline int p4la_loading_now(const P4AlignTracker *t, const P4WalkHost *h) {
    if (!t->anchored || t->loading) return 1;
    if (t->battle_qualification_locked) return 0;
    uint32_t marker[8];
    p4la_milestone(h,marker);
    return p4la_load_request(t,h,marker,p4la_word(h,0x007d2510u),
                            p4la_task(h,"battle"),p4la_word(h,0x00764608u));
}
static inline int p4la_observe(P4AlignTracker *t, const P4WalkHost *h, uint32_t out[8]) {
    p4la_milestone(h,out);
    const uint32_t frame=p4la_word(h,P4LS_GAME_FRAME);
    const uint32_t root=p4la_word(h,0x007d2510u);
    const uint32_t battle=p4la_task(h,"battle"), event=p4la_word(h,0x00764608u);
    int began=0;
    p4la_effect_quiet(t,h,frame);
    if (t->anchored && p4la_battle_entry(t,h)) {
        out[2]=P4LA_BATTLE_ENTRY; out[3]=t->battle_qualification_id-1u;
        if (!t->loading) { t->loading=1; t->epoch++; t->begin=frame; began=1; }
    }
    if (!t->anchored) {
        /* The first anchor is free control after LOAD GAME, not an earlier
         * actor-installation/event-entry observation. The controller owns full
         * equality and narrowly proved elapsed resync; this observer writes no state. */
        out[1]=P4LA_BOOT;
        if (out[2]==P4LA_FIELD && out[6]==13u && !p4la_word(h,0x00764604u)) out[1]=P4LA_READY;
    } else {
        if (!t->battle_qualification_locked && !t->loading && p4la_load_request(t,h,out,root,battle,event)) {
            t->loading=1; t->epoch++; t->begin=frame; began=1;
        }
        out[1]=t->loading ? (out[2] ? P4LA_READY : P4LA_LOADING) : P4LA_LOCKED;
    }
    out[0]=t->epoch; out[7]=t->begin;
    t->sequence=out[5]; t->root=root; t->battle=battle; t->event=event; t->last_frame=frame;
    return began;
}
static inline void p4la_accept(P4AlignTracker *t, const uint32_t marker[8]) {
    t->anchored=1; t->loading=0;
    if (marker[2]==P4LA_BATTLE_ENTRY) t->battle_qualification_locked=1;
    t->stable_kind=marker[2]; t->stable_key=marker[3]; t->stable_calendar=marker[4];
}
/* A capture-only replay may drain a rejected battle boundary, never accept it. */
static inline int p4la_unqualified_capture(int capture_only, uint32_t status, const uint32_t marker[8]) {
    return capture_only && status==1u && marker[2]==P4LA_BATTLE_ENTRY;
}
static inline int p4la_write_words(FILE *f, const uint32_t *words, size_t count) {
    uint8_t bytes[4];
    for (size_t i=0;i<count;i++) {
        p4ls_put(bytes,words[i]);
        if (fwrite(bytes,1,4,f)!=4) return 0;
    }
    return 1;
}
/* Trace clock identity is an address on its own participant only, never a
 * cross-participant equality key. The semantic snapshot supplies named paths. */
static inline size_t p4la_clocks(const P4WalkHost *h, uint32_t out[P4LA_MAX_CLOCKS*2]) {
    static const uint32_t heads[]={0x00764adcu,0x00764af4u,0x00764ae8u};
    const uint32_t scene=p4la_word(h,0x00762ea0u);
    size_t count=2;
    out[0]=0x0076428cu; out[1]=p4la_word(h,0x0076428cu);
    out[2]=scene+0x94u; out[3]=p4la_word(h,scene+0x94u);
    for (unsigned l=0;l<3;l++) {
        uint32_t node=p4la_word(h,heads[l]);
        for (unsigned i=0;p4la_address(node,0x40) && i<256;i++,node=p4la_word(h,node+0x3c)) {
            char label[25];
            for (unsigned j=0;j<24;j+=4) p4ls_put((uint8_t *)label+j,p4la_word(h,node+j));
            label[24]=0;
            if (!strcmp(label,"field camera controler")) {
                const uint32_t work=p4la_word(h,node+0x38u);
                if (count==P4LA_MAX_CLOCKS || !p4la_address(work,0x30)) return P4LA_MAX_CLOCKS+1;
                out[count*2]=work+0x2cu; out[count*2+1]=p4la_word(h,work+0x2cu); count++;
            }
            if (!strcmp(label,"evtEditLoad")) {
                const uint32_t work=p4la_word(h,node+0x38u);
                if (count>P4LA_MAX_CLOCKS-2u || !p4la_address(work,0x7b0)) return P4LA_MAX_CLOCKS+1;
                out[count*2]=work+0x14u; out[count*2+1]=p4la_word(h,work+0x14u); count++;
                out[count*2]=work+0x18u; out[count*2+1]=p4la_word(h,work+0x18u); count++;
            }
        }
    }
    uint32_t node=p4la_word(h,0x00764608u);
    for (unsigned i=0;p4la_address(node,0x154) && i<32;i++,node=p4la_word(h,node+0x150u)) {
        static const uint32_t offsets[]={0x18u,0x134u,0x138u};
        for (unsigned j=0;j<3;j++) {
            if (count==P4LA_MAX_CLOCKS) return P4LA_MAX_CLOCKS+1;
            out[count*2]=node+offsets[j]; out[count*2+1]=p4la_word(h,node+offsets[j]); count++;
        }
    }
    /* BtlUnit.layerHandle -> model.layers[2], stride 0xa4. Raw IEEE time,
     * previous time, blend ticks and blend fraction stay observable. */
    const uint32_t battle=p4la_word(h,0x0076449cu);
    if (p4la_task(h,"battle")) {
      for (unsigned group=0;group<3;group++) {
        uint32_t unit=p4la_word(h,battle+0x178u+group*4u);
        for (unsigned i=0;p4la_address(unit,0xa70) && i<128;i++,unit=p4la_word(h,unit+0xa6cu)) {
            const uint32_t model=p4la_word(h,unit+0xa00u);
            if (!p4la_address(model,0x234)) continue;
            for (unsigned layer=0;layer<2;layer++) {
                const uint32_t base=model+0xecu+layer*0xa4u;
                static const uint32_t offsets[]={0x0cu,0x14u,0x18u,0x1cu};
                for (unsigned j=0;j<4;j++) {
                    if (count==P4LA_MAX_CLOCKS) return P4LA_MAX_CLOCKS+1;
                    out[count*2]=base+offsets[j]; out[count*2+1]=p4la_word(h,base+offsets[j]); count++;
                }
            }
        }
      }
        uint32_t action=p4la_word(h,battle+0x174u);
        for (unsigned i=0;p4la_address(action,0x454) && i<256;i++,action=p4la_word(h,action+0x450u)) {
            static const uint32_t offsets[]={0x1cu,0x24u};
            for (unsigned j=0;j<2;j++) {
                if (count==P4LA_MAX_CLOCKS) return P4LA_MAX_CLOCKS+1;
                out[count*2]=action+offsets[j];
                out[count*2+1]=p4la_word(h,action+offsets[j]); count++;
            }
        }
    }
    return count;
}
typedef struct P4AlignRestore {
    uint32_t count, frame, value;
} P4AlignRestore;
typedef struct P4AlignTrace {
    uint32_t frame, marker[8], rng[P4LA_RNG_WORDS], clocks[P4LA_MAX_CLOCKS*2];
    P4AlignRestore restore;
    size_t count;
    int pending;
} P4AlignTrace;
static inline int p4la_trace_row(FILE *f, uint32_t frame, const uint32_t marker[8],
                                const uint32_t rng[P4LA_RNG_WORDS], const P4AlignRestore *restore,
                                const uint32_t *clocks, size_t count) {
    const uint32_t length=(uint32_t)(1+8+P4LA_RNG_WORDS+3+1+count*2), n=(uint32_t)count;
    const uint32_t reset[3]={restore->count,restore->frame,restore->value};
    return p4la_write_words(f,&length,1) && p4la_write_words(f,&frame,1) &&
        p4la_write_words(f,marker,8) && p4la_write_words(f,rng,P4LA_RNG_WORDS) &&
        p4la_write_words(f,reset,3) && p4la_write_words(f,&n,1) && p4la_write_words(f,clocks,count*2);
}
/* Full banks are needed during loads, plus the last strict entry update.
 * Retain that one row instead of writing gigabytes of redundant strict banks. */
static inline int p4la_trace(P4AlignTrace *trace, FILE *f, const P4WalkHost *h,
                            const uint32_t marker[8], const uint32_t rng[P4LA_RNG_WORDS],
                            const P4AlignRestore *restore) {
    uint32_t clocks[P4LA_MAX_CLOCKS*2];
    const size_t count=p4la_clocks(h,clocks);
    const uint32_t frame=p4la_word(h,P4LS_GAME_FRAME);
    if (count>P4LA_MAX_CLOCKS) return 0;
    if (marker[1]==P4LA_LOCKED) {
        trace->frame=frame; trace->count=count; trace->pending=1;
        trace->restore=*restore;
        memcpy(trace->marker,marker,sizeof trace->marker);
        memcpy(trace->rng,rng,sizeof trace->rng);
        memcpy(trace->clocks,clocks,count*2*sizeof *clocks);
        return 1;
    }
    if (trace->pending && !p4la_trace_row(f,trace->frame,trace->marker,trace->rng,&trace->restore,trace->clocks,trace->count)) return 0;
    trace->pending=0;
    return p4la_trace_row(f,frame,marker,rng,restore,clocks,count) && !fflush(f);
}
#endif
