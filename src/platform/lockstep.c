/* Exact-input recording/replay. Live sessions keep deterministic scheduler time
 * but pace presentation to wall time, without hiding the user's window/audio.
 * The launcher owns retail transport, immutable starts and report assembly. */
#include "p4pc/lockstep.h"
#include "p4pc/battle_live.h"
#include "lockstep_hotkey.h"
#include "lockstep_draw.h"
#include "p4pc/lockstep_format.h"
#include "p4pc/lockstep_align_runtime.h"
#include "p4pc/crt.h"
#include "p4pc/guest.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif
#include "config/p4cfg.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int16_t datGetDateIndex(void);
extern int p4pc_facing_gate_check(int date);

static FILE *sRecord, *sReplay, *sStates;
static const char *sDir, *sMarks;
static int sActive;
static int sLive;
static uint64_t sPaceVirtual, sPaceWall;
static P4LockstepHotkey sHotkey;
static unsigned sEvery;
static uint32_t sStop;
static int sStopSet, sStopReady;
static uint32_t sReplayLastKind;
static int sReplayDiverged;
static int sIgnoreMarks;
static int sVerifyReplay;
static int sScenarioReplay;
static uint32_t sAnchor;
static int sAnchorSet;
static int sAligned;
static int sReplayAligned;
static int sFlushPads;
static char sCompletePath[1100];
static P4LockstepDrawBuffer sDraw;

static void fail(const char *message) {
    fprintf(stderr,"p4pc: lockstep: %s\n",message);
    exit(4);
}
static uint32_t word(void *user, uint32_t address) {
    uint32_t value=0;
    (void)user;
#ifdef P4_GUEST64
    if (address>=0x00100000u && address<=P4_GUEST_SIZE-4u) memcpy(&value,P4_G2H(address),4);
#else
    if (address>=0x00100000u && address<=0x01fffffcu) memcpy(&value,P4_G2H(address),4);
#endif
    return value;
}
static void read_record(uint8_t record[P4LS_RECORD_SIZE], uint32_t kind) {
    size_t n=fread(record,1,P4LS_RECORD_SIZE,sReplay);
    if (!n && feof(sReplay)) {
        if (sReplayLastKind==P4LS_PAD) fail("replay ended before the frame was presented");
        if (p4_battle_live_lockstep_eof()) exit(4);
        p4pc_exit(0);
    }
    if (n!=P4LS_RECORD_SIZE) fail("truncated replay record");
    if ((p4ls_get(record)&255u)!=kind || p4ls_get(record+4)!=word(NULL,P4LS_GAME_FRAME)) {
        fprintf(stderr,"p4pc: lockstep expected kind %u frame %u, log has kind %u frame %u\n",
                kind,word(NULL,P4LS_GAME_FRAME),p4ls_get(record)&255u,p4ls_get(record+4));
        fail("input/frame schedule diverged");
    }
    sReplayLastKind=kind;
}
static void record_bytes(const uint8_t *record) {
    if (sRecord && fwrite(record,1,P4LS_RECORD_SIZE,sRecord)!=P4LS_RECORD_SIZE) fail("writing input log");
}
static void pad(uint8_t *packet, size_t size) {
    uint8_t record[P4LS_RECORD_SIZE];
    if (size!=32) fail("pad packet is not 32 bytes");
    if (sReplay) {
        if (sReplayAligned) {
            const P4WalkHost host={.read32=word};
            memcpy(record,p4pc_align_replay_pad(&host),sizeof record);
        } else read_record(record,P4LS_PAD);
        memcpy(packet,record+8,32);
    }
    if (sAligned) {
        const P4WalkHost host={.read32=word};
        uint32_t gate[8];
        if (p4pc_align_pad_gate(&host,gate)) {
            p4ls_put(record,P4LA_PAD_GATE);
            p4ls_put(record+4,word(NULL,P4LS_GAME_FRAME));
            for (unsigned i=0;i<8;i++) p4ls_put(record+8+i*4,gate[i]);
            record_bytes(record);
        }
    }
    p4ls_put(record,P4LS_PAD);
    p4ls_put(record+4,word(NULL,P4LS_GAME_FRAME));
    memcpy(record+8,packet,32);
    record_bytes(record);
    /* An online disc replay waits inside the update for retail's polls: retail must have this
     * update's pad reads before the frame is presented. */
    if (sFlushPads && sRecord && fflush(sRecord)) fail("writing input log");
    if (sAligned) p4pc_align_pad_read();
}
static FILE *open_file(const char *path, const char *mode) {
    FILE *f=fopen(path,mode);
    if (!f) { fprintf(stderr,"p4pc: lockstep cannot open %s\n",path); fail("opening file"); }
    return f;
}
static void hotkey(int scancode) {
    p4ls_hotkey_press(&sHotkey,scancode);
}
void p4pc_lockstep_init(void) {
    const char *replay=getenv("P4PC_LOCKSTEP_REPLAY");
    const char *live=getenv("P4PC_LOCKSTEP_LIVE");
    const char *scenario=getenv("P4PC_LOCKSTEP_SCENARIO_REPLAY");
    char path[1024], magic[8];
    p4cfg_load();
    sDir=p4cfg_str("lockstep.directory",NULL);
    sScenarioReplay=scenario && !strcmp(scenario,"1");
    if (!sDir && !replay && !sScenarioReplay) return;
    #ifdef P4_MOD_GAME_BINDINGS
    p4_mod_fidelity_check_start();
    #endif
    sActive=1;
    sLive=live && !strcmp(live,"1");
    {
        const char *aligned=getenv("P4PC_LOCKSTEP_STATE_ALIGNED");
        sAligned=aligned && !strcmp(aligned,"1");
    }
    if (sAligned && sLive) fail("state-aligned barriers are offline-only; live recording remains nonblocking");
    sFlushPads=sAligned && getenv("P4PC_LOCKSTEP_DISC_ONLINE") && *getenv("P4PC_LOCKSTEP_DISC_ONLINE");
    if (sScenarioReplay && (!replay || !sDir || sLive || sAligned))
        fail("scenario replay requires a strict offline replay and diagnostic directory");
    sMarks=getenv("P4PC_LOCKSTEP_MARKS");
    sIgnoreMarks=getenv("P4PC_LOCKSTEP_IGNORE_MARKS")!=NULL;
    sVerifyReplay=sScenarioReplay || getenv("P4PC_LOCKSTEP_VERIFY_REPLAY")!=NULL;
    sEvery=(unsigned)p4cfg_int("lockstep.interval",60);
    if (!sEvery) sEvery=1;
    sHotkey.scancode=SDL_GetScancodeFromName(p4cfg_str("lockstep.hotkey","F9"));
    sHotkey.pending=0;
    if (sHotkey.scancode==SDL_SCANCODE_UNKNOWN) fail("unknown lockstep.hotkey SDL scancode name");
    if (replay) {
        sReplay=open_file(replay,"rb");
        if (fread(magic,1,8,sReplay)!=8) fail("bad input log header");
        sReplayAligned=!memcmp(magic,P4LA_MAGIC,8);
        if ((!sReplayAligned && memcmp(magic,P4LS_MAGIC,8)) || (sReplayAligned && !sAligned))
            fail("input log requires its matching replay mode");
        if (sAligned && !sReplayAligned && sVerifyReplay)
            fail("classic source and aligned output use different fingerprint profiles");
    }
    {
        const char *stop=getenv("P4PC_LOCKSTEP_STOP");
        sStopSet=stop!=NULL;
        sStop=stop ? (uint32_t)strtoul(stop,NULL,10) : 0;
    }
    {
        const char *anchor=getenv("P4PC_LOCKSTEP_POST_RESTORE_ANCHOR");
        sAnchorSet=anchor!=NULL;
        sAnchor=anchor ? (uint32_t)strtoul(anchor,NULL,10) : 0;
    }
    if (sDir) {
        if (!SDL_CreateDirectory(sDir)) fail(SDL_GetError());
        snprintf(path,sizeof path,"%s/input.p4input",sDir);
        sRecord=open_file(path,"wb");
        if (fwrite(sAligned ? P4LA_MAGIC : P4LS_MAGIC,1,8,sRecord)!=8) fail("writing log header");
        fflush(sRecord);
        if (!sScenarioReplay) {
            snprintf(path,sizeof path,"%s/port-states.tsv",sDir);
            sStates=open_file(path,"wb");
            fprintf(sStates,"frame\twalk\trng");
            for (unsigned i=0;i<P4LS_STATE_WORDS;i++) fprintf(sStates,"\t%s",p4ls_state_names[i]);
            fputc('\n',sStates);
        }
    }
    if (sScenarioReplay) fprintf(stderr,"p4pc: lockstep scenario replay: strict verification, gate draw policy, binary frame evidence\n");
    if (sAligned && !p4pc_align_init(sDir,sReplayAligned ? sReplay : NULL)) fail("initializing state alignment");
    p4pc_pad_set_packet_hook(pad);
    p4pc_pad_set_key_hook(hotkey);
}
int p4pc_lockstep_flush(void) {
    int failed=0;
    if (sRecord && fflush(sRecord)) failed=1;
    if (sStates && fflush(sStates)) failed=1;
    return failed ? -1 : 0;
}
int p4pc_lockstep_fork_reopen(const char *dir) {
    static char forkDir[1024];
    char path[1100];
    if (!sActive || sReplay || sAligned || !sDir || strlen(dir)>=sizeof forkDir) return -1;
    if (!SDL_CreateDirectory(dir)) return -1;
    if (sRecord) {
        fclose(sRecord); /* flushed before the fork: only this process's descriptor closes */
        snprintf(path,sizeof path,"%s/input.p4input",dir);
        if (!(sRecord=fopen(path,"wbx"))) return -1;
    }
    if (sStates) {
        fclose(sStates);
        snprintf(path,sizeof path,"%s/port-states.tsv",dir);
        if (!(sStates=fopen(path,"wbx"))) return -1;
    }
    strcpy(forkDir,dir);
    sDir=forkDir;
    return 0;
}
int p4pc_lockstep_draw_active(void) { return sActive && !sScenarioReplay; }
void p4pc_lockstep_drawline(const char *line) {
    const char *error;
    if (!p4pc_lockstep_draw_active()) return;
    error=p4pc_lockstep_draw_append(&sDraw,line);
    if (error) fail(error);
}
static int listed(uint32_t frame) {
    const char *p=sMarks;
    while (p && *p) {
        char *end;
        unsigned long n=strtoul(p,&end,10);
        if (p==end) fail("invalid P4PC_LOCKSTEP_MARKS");
        if (n==frame) return 1;
        if (*end && *end!=',') fail("invalid P4PC_LOCKSTEP_MARKS separator");
        p=*end ? end+1 : end;
    }
    return 0;
}
static void capture_mark(uint32_t frame, const uint8_t *sample, void (*capture)(const char *)) {
    char dir[1024], path[1100];
    FILE *f;
    if (!sDir) fail("capture requires lockstep.directory");
    snprintf(dir,sizeof dir,"%s/%u",sDir,frame);
    if (!SDL_CreateDirectory(dir)) fail(SDL_GetError());
    snprintf(path,sizeof path,"%s/port.ram",dir);
    f=open_file(path,"wb");
    if (fwrite(P4_G2H(0x00100000u),1,0x1f00000u,f)!=0x1f00000u) fail("writing RAM");
    fclose(f);
    snprintf(path,sizeof path,"%s/port.draw.txt",dir);
    f=open_file(path,"wb");
    if (sDraw.length && fwrite(sDraw.data,1,sDraw.length,f)!=sDraw.length) fail("writing draw log");
    fclose(f);
    snprintf(path,sizeof path,"%s/port.state.tsv",dir);
    f=open_file(path,"wb");
    fprintf(f,"word\taddress\tvalue\nwalk\t-\t%08x\nrng\t-\t%08x\n",p4ls_get(sample),p4ls_get(sample+4));
    for (unsigned i=0;i<P4LS_STATE_WORDS;i++) fprintf(f,"%s\t%08x\t%08x\n",p4ls_state_names[i],p4ls_state_addresses[i],p4ls_get(sample+8+i*4));
    fclose(f);
    snprintf(path,sizeof path,"%s/port.png",dir);
    capture(path);
    snprintf(sCompletePath,sizeof sCompletePath,"%s/port.complete",dir);
    fprintf(stderr,"p4pc: lockstep mark game frame %u: %s\n",frame,dir);
}
void p4pc_lockstep_frame(void (*capture)(const char *)) {
    uint8_t record[P4LS_RECORD_SIZE], expected[P4LS_RECORD_SIZE];
    const P4WalkHost host={.read32=word};
    uint32_t frame, flags, marker[8];
    int mark;
    if (!sActive) return;
    frame=word(NULL,P4LS_GAME_FRAME);
    mark=p4ls_hotkey_take(&sHotkey) || listed(frame);
    flags=sAligned ? (P4LS_EFFECT_RNG|P4LA_FLAG|p4pc_align_profile()) : (sAnchorSet && frame>=sAnchor ? P4LS_EFFECT_RNG : 0);
    if (sAligned) p4pc_align_prepare(&host,marker);
    if (sReplay) {
        if (sReplayAligned) memcpy(expected,p4pc_align_replay_frame(&host),sizeof expected);
        else read_record(expected,P4LS_FRAME);
        if (!sAligned) flags=p4ls_get(expected)&P4LS_EFFECT_RNG;
    }
    p4ls_put(record,P4LS_FRAME|flags);
    p4ls_put(record+4,frame);
    if (!p4ls_sample(&host, record+8, flags, sAligned ? p4pc_align_rng() : NULL))
        fail("invalid qualified battle state");
    if (sReplay) {
        if (!sIgnoreMarks) mark|=(p4ls_get(expected)&P4LS_MARK)!=0;
        if (sVerifyReplay && !p4ls_equal(expected+8,record+8)) {
            fprintf(stderr,"p4pc: lockstep replay diverged at game frame %u\n",frame);
            mark=1;
            sReplayDiverged=1;
        }
    }
    if (mark) {
        p4ls_put(record,P4LS_FRAME|P4LS_MARK|flags);
        capture_mark(frame,record+8,capture);
    }
    if (sAligned) p4pc_align_meta(sRecord,frame,marker);
    record_bytes(record);
    if (sRecord && fflush(sRecord)) fail("flushing input log");
    if (sStates) {
        fprintf(sStates,"%u\t%08x\t%08x",frame,p4ls_get(record+8),p4ls_get(record+12));
        for (unsigned i=0;i<P4LS_STATE_WORDS;i++) fprintf(sStates,"\t%08x",p4ls_get(record+16+i*4));
        fputc('\n',sStates);
        fflush(sStates);
    }
    if (frame%sEvery==0) fprintf(stderr,"p4pc: lockstep game frame %u walk %08x rng %08x\n",frame,p4ls_get(record+8),p4ls_get(record+12));
    sDraw.length=0;
    sStopReady=sStopSet && frame>=sStop;
    if (sAligned) p4pc_align_boundary(&host,marker);
}
void p4pc_lockstep_presented(void) {
    if (sCompletePath[0]) {
        FILE *f=open_file(sCompletePath,"wb");
        fputs("complete\n",f);
        fclose(f);
        sCompletePath[0]='\0';
    }
    if (sReplayDiverged) fail("recording is not deterministic on this build");
    if (sStopReady) {
        if (getenv("P4PC_LOCKSTEP_FACING_CHECK") != NULL) {
            if (getenv("P4PC_FACING_GATE") == NULL ||
                !p4pc_facing_gate_check(datGetDateIndex())) {
                fail("scenario facing check failed");
            }
            fprintf(stderr, "p4pc: lockstep facing check PASS\n");
        }
        if (p4_battle_live_lockstep_stop()) exit(4);
        p4pc_exit(0);
    }
}
void p4pc_lockstep_pace(void) {
    if (!sLive) return;
    const uint64_t game=p4pc_scheduler_time_ns(), now=p4_os_now_ns();
    if (!sPaceWall) {
        sPaceWall=now;
        sPaceVirtual=game;
        return;
    }
    const uint64_t target=sPaceWall+(game-sPaceVirtual);
    if (now<target) p4_os_sleep_ns(target-now);
    else if (now-target>20000000ull) {
        /* A capture/window drag/slow host must not cause a turbo catch-up. */
        sPaceWall=now;
        sPaceVirtual=game;
    }
}
