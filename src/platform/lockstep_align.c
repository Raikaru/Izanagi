/* Explicit harness-only barriers and verified load-elapsed word patches.
 * No environment opt-in means no allocation, I/O, guest writes or barriers. */
#include "p4pc/lockstep_align_runtime.h"
#include "p4pc/battle_live.h"
#include "p4pc/lockstep_disc.h"
#include "p4pc/lockstep_filer_replay.h"
#include "p4pc/lockstep_uninit.h"
#include "p4pc/lockstep_align_input.h"
#include "p4pc/crt.h"
#include "p4pc/guest.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

typedef struct AlignRuntime {
    P4AlignTracker tracker;
    P4AlignTrace trace_buffer;
    P4AlignRestore restore;
    P4AlignInput input;
    FILE *trace, *events, *rng_calls, *effect_calls, *event_frames, *packet_frames, *message_frames;
    char directory[1024], replay_directory[1024];
    uint32_t rng[P4LA_RNG_WORDS];
    uint32_t *patch;
    size_t patch_capacity;
    uint64_t pads, frames, gates;
    uint32_t effect_call;
    P4AlignEmitters emitters, presented;
    int began, dumped_boot, restore_pending, patched;
} AlignRuntime;
static AlignRuntime *sAlign;
int p4pc_align_rng_calls_enabled;
int p4pc_align_battle_guard;
int p4pc_align_emitters_enabled;
static void align_fail(const char *why) {
    fprintf(stderr,"p4pc: state-aligned lockstep: %s\n",why);
    exit(4);
}
static FILE *align_open(const char *path, const char *mode) {
    FILE *f=fopen(path,mode);
    if (!f) align_fail(path);
    return f;
}
static void align_close(void) {
    if (!sAlign) return;
    if (sAlign->trace) fclose(sAlign->trace);
    if (sAlign->events) fclose(sAlign->events);
    p4pc_align_rng_calls_enabled=0;
    p4pc_align_battle_guard=0;
    p4pc_align_emitters_enabled=0;
    if (sAlign->rng_calls) fclose(sAlign->rng_calls);
    if (sAlign->effect_calls) fclose(sAlign->effect_calls);
    if (sAlign->event_frames) fclose(sAlign->event_frames);
    if (sAlign->packet_frames) fclose(sAlign->packet_frames);
    if (sAlign->message_frames) fclose(sAlign->message_frames);
    free(sAlign->input.batch); free(sAlign->patch); free(sAlign); sAlign=NULL;
}
int p4pc_align_init(const char *directory, FILE *replay) {
    const char *enabled=getenv("P4PC_LOCKSTEP_STATE_ALIGNED");
    if (!enabled || strcmp(enabled,"1")) return 0;
    if (!directory || !*directory) align_fail("state alignment requires a session directory");
    sAlign=calloc(1,sizeof *sAlign);
    if (!sAlign) align_fail("allocating state-aligned runtime");
    atexit(align_close);
    const char *battle_anchor=getenv("P4PC_LOCKSTEP_BATTLE_ANCHOR");
    if (battle_anchor) {
        char *end, *id_end;
        unsigned long mode=strtoul(battle_anchor,&end,10);
        unsigned long id=*end==':' ? strtoul(end+1,&id_end,10) : 65536u;
        if (!*battle_anchor || *end!=':' || !*(end+1) || *id_end || id>65535u ||
            (mode!=2u && mode!=4u) || !p4_battle_live_active)
            align_fail("battle qualification requires a selected live battle worker");
        sAlign->tracker.battle_qualification_id=(uint32_t)id+1u;
        sAlign->tracker.battle_qualification_mode=(uint32_t)mode;
    }
    snprintf(sAlign->directory,sizeof sAlign->directory,"%s/alignment",directory);
    if (!SDL_CreateDirectory(sAlign->directory)) align_fail(SDL_GetError());
    char path[1100];
    snprintf(path,sizeof path,"%s/control",sAlign->directory);
    if (!SDL_CreateDirectory(path)) align_fail(SDL_GetError());
    const char *source=getenv("P4PC_LOCKSTEP_ALIGNMENT_REPLAY");
    if (source) snprintf(sAlign->replay_directory,sizeof sAlign->replay_directory,"%s",source);
    p4pc_lockstep_disc_open(sAlign->directory);
    p4pc_lockstep_filer_open(sAlign->directory);
    p4pc_lockstep_uninit_open(sAlign->directory);
    snprintf(path,sizeof path,"%s/port-trace.bin",sAlign->directory);
    sAlign->trace=align_open(path,"wb");
    if (fwrite("P4ALTR2\n",1,8,sAlign->trace)!=8) align_fail("writing trace header");
    snprintf(path,sizeof path,"%s/port-events.tsv",sAlign->directory);
    sAlign->events=align_open(path,"wb");
    fprintf(sAlign->events,"event\tepoch\tframe\tsource_frame\tkind\tkey\tcalendar\tbegin\trecords\tpad_reads\theld_reads\tskipped_records\tsequence\tfield_step\n");
    fflush(sAlign->events);
    const char *rng_calls=getenv("P4PC_LOCKSTEP_RNG_CALLS");
    if (rng_calls && !strcmp(rng_calls,"1")) {
        snprintf(path,sizeof path,"%s/port-rng-calls.tsv",sAlign->directory);
        sAlign->rng_calls=align_open(path,"w");
        if (fprintf(sAlign->rng_calls,"# native_anchor\tp4pc_align_rng_call\t%p\n",(void *)p4pc_align_rng_call)<0 ||
            !p4la_rng_header(sAlign->rng_calls)) align_fail("writing RNG caller trace header");
        snprintf(path,sizeof path,"%s/port-effect-rng-calls.tsv",sAlign->directory);
        sAlign->effect_calls=align_open(path,"w");
        if (fprintf(sAlign->effect_calls,"# native_anchor\tp4pc_align_effect_call\t%p\n",(void *)p4pc_align_effect_call)<0 ||
            !p4la_effect_header(sAlign->effect_calls)) align_fail("writing effect RNG caller trace header");
        snprintf(path,sizeof path,"%s/port-event-player.tsv",sAlign->directory);
        sAlign->event_frames=align_open(path,"w");
        if (!p4la_event_header(sAlign->event_frames)) align_fail("writing event-player trace header");
        snprintf(path,sizeof path,"%s/port-battle-packets.tsv",sAlign->directory);
        sAlign->packet_frames=align_open(path,"w");
        if (!p4la_packet_header(sAlign->packet_frames)) align_fail("writing battle packet trace header");
        snprintf(path,sizeof path,"%s/port-message-window.tsv",sAlign->directory);
        sAlign->message_frames=align_open(path,"w");
        if (!p4la_message_header(sAlign->message_frames)) align_fail("writing message-window trace header");
        p4pc_align_rng_calls_enabled=1;
    }
    sAlign->input.file=replay;
    p4pc_align_emitters_enabled=1;
    return 1;
}
uint32_t *p4pc_align_rng(void) { return sAlign ? sAlign->rng : NULL; }
uint32_t p4pc_align_profile(void) {
    return sAlign && sAlign->tracker.battle_qualification_locked ? P4LS_BATTLE_QUALIFICATION : 0;
}
static uint32_t align_rng_word(void *user, uint32_t address) {
    (void)user;
#ifdef P4_GUEST64
    const uint32_t end=P4_GUEST_SIZE;
#else
    const uint32_t end=0x02000000u;
#endif
    return address>=0x00100000u && address<=end-4u ? p4ls_get(P4_G2H(address)) : 0;
}
void p4pc_align_field_effect_request(void) {
    align_fail("field effect activation during battle qualification");
}
void p4pc_align_field_effect_step(const void *task) {
    const P4WalkHost host={.read32=align_rng_word};
    if (!p4la_field_effect_task_idle(&host,P4_H2G(task)))
        align_fail("field effect update during battle qualification");
}
void p4pc_align_emitter_advance(const void *emitter) {
    if (sAlign) p4la_emitter_seen(&sAlign->emitters,P4_H2G(emitter));
}
void p4pc_align_rng_call(uint32_t draw, const void *caller) {
    const P4WalkHost host={.read32=align_rng_word};
    if (!sAlign || !sAlign->rng_calls || !p4la_rng_call(sAlign->rng_calls,&host,
        &sAlign->tracker,draw,(uint64_t)(uintptr_t)caller)) align_fail("writing actual RNG call entry");
}
void p4pc_align_effect_call(uint32_t kind, const void *state, uint32_t argument, const void *caller) {
    /* NULL selects this shared bank in the matched function. Private effect
     * states are not the global stream compared by the walk digest. */
    if (state && state!=P4_G2H(0x00922de0u)) return;
    const P4WalkHost host={.read32=align_rng_word};
    if (!sAlign || !sAlign->effect_calls || !p4la_effect_call(sAlign->effect_calls,&host,
        &sAlign->tracker,++sAlign->effect_call,(uint64_t)(uintptr_t)caller,kind,argument))
        align_fail("writing actual effect RNG call entry");
}
void p4pc_align_restore_applied(void) {
    if (!sAlign) return;
    if (sAlign->tracker.battle_qualification_locked)
        align_fail("unexpected save restore during battle qualification");
    sAlign->restore.count++;
    sAlign->restore.frame=p4ls_get(P4_G2H(P4LS_GAME_FRAME));
    sAlign->restore.value=p4ls_get(P4_G2H(0x0076428cu));
    if (!sAlign->tracker.anchored || !sAlign->tracker.loading) {
        if (sAlign->tracker.anchored) {
            sAlign->tracker.loading=1; sAlign->tracker.epoch++;
        }
        sAlign->tracker.begin=sAlign->restore.frame;
        sAlign->restore_pending=1;
    }
}
void p4pc_align_prepare(const P4WalkHost *host, uint32_t marker[8]) {
    if (sAlign->tracker.battle_qualification_locked && !p4la_battle_field_effects_idle(host))
        align_fail("field-owned effect activated during battle qualification");
    sAlign->began=p4la_observe(&sAlign->tracker,host,marker);
    if (sAlign->restore_pending) {
        sAlign->began=1; sAlign->restore_pending=0;
    }
}
int p4pc_align_pad_gate(const P4WalkHost *host, uint32_t gate[8]) {
    if (!sAlign || sAlign->tracker.anchored || !p4la_title_gate(host,gate)) return 0;
    sAlign->gates++;
    return 1;
}
void p4pc_align_pad_read(void) { if (sAlign) sAlign->pads++; }
const uint8_t *p4pc_align_replay_pad(const P4WalkHost *host) {
    const uint8_t *row=p4la_source_pad(&sAlign->input,p4la_word(host,P4LS_GAME_FRAME));
    if (!row) {
        if (sAlign->input.eof) { if (p4_battle_live_lockstep_eof()) exit(4); p4pc_exit(0); }
        align_fail(sAlign->input.error);
    }
    return row;
}
const uint8_t *p4pc_align_replay_frame(const P4WalkHost *host) {
    const int outcome=p4la_source_frame(&sAlign->input,p4la_word(host,P4LS_GAME_FRAME));
    if (outcome<0) align_fail(sAlign->input.error);
    if (outcome==3) { if (p4_battle_live_lockstep_eof()) exit(4); p4pc_exit(0); }
    return sAlign->input.presented;
}
void p4pc_align_meta(FILE *record, uint32_t frame, const uint32_t marker[8]) {
    sAlign->frames++;
    if (!record) return;
    uint8_t row[P4LS_RECORD_SIZE];
    p4ls_put(row,P4LA_META); p4ls_put(row+4,frame);
    for (unsigned i=0;i<8;i++) p4ls_put(row+8+i*4,marker[i]);
    if (fwrite(row,1,sizeof row,record)!=sizeof row) align_fail("writing milestone record");
}
static void align_dump(const char *path) {
    FILE *f=align_open(path,"wb");
    if (fwrite(P4_G2H(0x00100000u),1,0x1f00000u,f)!=0x1f00000u || fclose(f))
        align_fail("writing boundary RAM");
}
static void align_event(const char *name, const P4WalkHost *host, const uint32_t marker[8]) {
    const P4AlignInput *in=&sAlign->input;
    fprintf(sAlign->events,"%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%llu\t%llu\t%llu\t%llu\t%u\t%u\n",
            name,marker[0],p4la_word(host,P4LS_GAME_FRAME),in->file ? in->frame : p4la_word(host,P4LS_GAME_FRAME),
            marker[2],marker[3],marker[4],marker[7],
            (unsigned long long)(sAlign->pads+sAlign->frames*2+sAlign->gates),
            (unsigned long long)sAlign->pads,(unsigned long long)in->held_reads,
            (unsigned long long)in->skipped_records,marker[5],marker[6]);
    if (fflush(sAlign->events)) align_fail("publishing boundary request");
}
static uint32_t align_reply(const P4WalkHost *host, const uint32_t marker[8]) {
    char path[1100];
    snprintf(path,sizeof path,"%s/control/%u.bin",
             sAlign->replay_directory[0] ? sAlign->replay_directory : sAlign->directory,marker[0]);
    FILE *f;
    while (!(f=fopen(path,"rb"))) {
        if (sAlign->replay_directory[0]) align_fail("recorded resync response is missing");
        SDL_Delay(1); /* Host-time backpressure; not a guest tick or inserted wait. */
    }
    uint8_t header[24];
    if (fread(header,1,sizeof header,f)!=sizeof header || memcmp(header,"P4ALRP1\n",8) ||
        p4ls_get(header+8)!=marker[0] || p4ls_get(header+12)!=p4la_word(host,P4LS_GAME_FRAME))
        align_fail("load response does not identify this unchanged boundary");
    const uint32_t status=p4ls_get(header+16), count=p4ls_get(header+20);
    if (count>4096u || (status!=1u && status!=4u && status!=6u && count))
        align_fail("invalid resync response");
    if (status==6u && (!sAlign->tracker.battle_qualification_id ||
        marker[2]!=P4LA_BATTLE_ENTRY))
        align_fail("invalid battle qualification response");
    sAlign->patched=count!=0;
    if (count>sAlign->patch_capacity) {
        void *p=realloc(sAlign->patch,(size_t)count*3*sizeof(uint32_t));
        if (!p) align_fail("growing retained resync words");
        sAlign->patch=p; sAlign->patch_capacity=count;
    }
    for (uint32_t i=0;i<count*3;i++) {
        uint8_t word[4];
        if (fread(word,1,4,f)!=4) align_fail("truncated resync response");
        sAlign->patch[i]=p4ls_get(word);
    }
    if (fgetc(f)!=EOF || fclose(f)) align_fail("extra resync response data");
    /* Validate every old value before the first write. Only the proof-owning
     * controller may emit these patches; the JSON report retains each named
     * variable, classification, old/new value and generator/rate evidence. */
    for (uint32_t i=0;i<count;i++) {
        const uint32_t a=sAlign->patch[i*3];
        if (status==6u && !p4la_battle_patch_word(host,a))
            align_fail("battle qualification may patch only the effect bank and load-wait clocks");
        if (!p4la_address(a,4) || (a&3u) || a==P4LS_GAME_FRAME ||
            p4la_word(host,a)!=sAlign->patch[i*3+1]) align_fail("resync before-value/address guard failed");
    }
    for (uint32_t i=0;i<count;i++)
        memcpy(P4_G2H(sAlign->patch[i*3]),sAlign->patch+i*3+2,4);
    return status;
}
void p4pc_align_boundary(const P4WalkHost *host, const uint32_t marker[8]) {
    /* Emitters advanced by the update that produced this presentation. */
    sAlign->presented=sAlign->emitters;
    sAlign->emitters.count=sAlign->emitters.overflow=0;
    if (sAlign->rng_calls && fflush(sAlign->rng_calls)) align_fail("flushing RNG call trace");
    if (sAlign->effect_calls && fflush(sAlign->effect_calls)) align_fail("flushing effect RNG call trace");
    if (sAlign->event_frames && (!p4la_event_frame(sAlign->event_frames,host) ||
        fflush(sAlign->event_frames))) align_fail("writing actual event-player state");
    if (sAlign->packet_frames && (!p4la_packet_frame(sAlign->packet_frames,host) ||
        fflush(sAlign->packet_frames))) align_fail("writing actual battle packet state");
    if (sAlign->message_frames && (!p4la_message_frame(sAlign->message_frames,host) ||
        fflush(sAlign->message_frames))) align_fail("writing actual message-window state");
    if (!p4la_trace(&sAlign->trace_buffer,sAlign->trace,host,marker,sAlign->rng,&sAlign->restore)) align_fail("writing full RNG/clock trace");
    char folder[1100],path[1200];
    snprintf(folder,sizeof folder,"%s/%u",sAlign->directory,marker[0]);
    if (sAlign->began || !sAlign->dumped_boot || marker[1]==P4LA_READY) {
        if (!SDL_CreateDirectory(folder)) align_fail(SDL_GetError());
    }
    if (sAlign->began || !sAlign->dumped_boot) {
        snprintf(path,sizeof path,"%s/port-begin.ram",folder); align_dump(path);
        align_event("begin",host,marker); sAlign->dumped_boot=1;
    }
    if (marker[1]!=P4LA_READY) return;
    snprintf(path,sizeof path,"%s/port-before.ram",folder); align_dump(path);
    snprintf(path,sizeof path,"%s/port-emitters.tsv",folder);
    if (!p4la_emitters_write(path,&sAlign->presented,p4la_word(host,P4LS_GAME_FRAME)))
        align_fail("writing the boundary update's advanced emitters");
    align_event("ready",host,marker);
    const uint32_t status=align_reply(host,marker);
    if (!status) align_fail("boundary equality/proof failed; no state was resynchronized");
    snprintf(path,sizeof path,"%s/port-after.ram",folder); align_dump(path);
    p4la_accept(&sAlign->tracker,marker);
    if (status==6u) {
        p4pc_align_battle_guard=1;
        p4pc_lockstep_disc_activate(p4la_word(host,P4LS_GAME_FRAME));
        p4pc_lockstep_filer_activate(p4la_word(host,P4LS_GAME_FRAME));
    }
    /* Outside battle qualification the disc replay starts at the accepted initial LOAD anchor. */
    if (status==1u && marker[0]==0u && !sAlign->tracker.battle_qualification_id) {
        p4pc_lockstep_disc_activate(p4la_word(host,P4LS_GAME_FRAME));
        p4pc_lockstep_filer_activate(p4la_word(host,P4LS_GAME_FRAME));
        p4pc_lockstep_uninit_activate(p4la_word(host,P4LS_GAME_FRAME));
    }
    if (status==4u) { if (p4_battle_live_lockstep_boundary()) exit(4); p4pc_exit(0); }
    if (status==5u) exit(4);
    if (status!=1u && status!=2u && status!=6u) align_fail("unknown boundary response");
    uint32_t locked[8];
    memcpy(locked,marker,sizeof locked); locked[1]=P4LA_LOCKED;
    if (sAlign->patched) {
        uint8_t after[32];
        p4ls_sample(host,after,P4LS_EFFECT_RNG,sAlign->rng);
    }
    if (!p4la_trace(&sAlign->trace_buffer,sAlign->trace,host,locked,sAlign->rng,&sAlign->restore))
        align_fail("retaining post-resync load entry");
}
