/* One logical input tape; load-time holds/skips are counted, never hidden. */
#ifndef P4PC_LOCKSTEP_ALIGN_INPUT_H
#define P4PC_LOCKSTEP_ALIGN_INPUT_H
#include "p4pc/lockstep_alignment.h"
#include <stdlib.h>
typedef struct P4AlignInput {
    FILE *file;
    uint8_t *batch;
    size_t count, capacity, index, pads, reads, delivered, body, gates;
    uint32_t marker[8], frame, gate_frame;
    uint8_t presented[P4LS_RECORD_SIZE], last_pad[P4LS_RECORD_SIZE];
    uint64_t records, pad_records, actual_pad_reads, held_reads, skipped_records;
    uint32_t dialog_source_frame, dialog_wait_start, dialog_wait;
    uint32_t dialog_expected[8], dialog_actual[8], dialog_previous[8], dialog_predecessor[8];
    uint32_t wait_start, wait_key[7], wait_kind;
    int32_t dialog_recorded_offset, dialog_ready_offset, dialog_delivered_offset;
    int dialog_status, dialog_waiting, dialog_held;
    int dialog_previous_valid, dialog_predecessor_valid, dialog_predecessor_departed;
    int have_pad, mapped, restore_mapped, gate_held, eof, pending_gate, complete;
    /* A title gate delivered its press (Cross down): retail's input is frame-locked to the source from here (NG-7j). */
    int title_delivered;
    /* Tape-building pass only (--lockstep-gate-build): a gate that became ready after its recorded offset is
     * delivered when it is ready and its actual offset logged, instead of failing closed (NG-7b). The proof pass
     * never sets this. */
    int late_ok;
    /* Retail runs ahead of the port on a complete tape (NG-7k): the tape's locked states are the port's from a
     * different run, so retail records its own states and the controller compares them with the proof port's. */
    int ahead;
    /* Streaming (the online disc replay, after the initial anchor): a pad read takes the
     * frame's next pad record as soon as it arrives, before the source has presented the
     * frame; the presentation still waits for the whole frame. */
    int streaming;
    int64_t offset;
    const char *error;
} P4AlignInput;
/* Appends one record of the current frame to the batch: 1 for a pad, gate or milestone
 * record, 2 when the frame record completes it, 0 at EOF before a frame or on an error.
 * `body` counts the pad and gate records, which precede the milestone and frame records. */
static inline int p4la_append(P4AlignInput *in) {
    if (!in->count) {
        in->index=in->pads=in->reads=in->delivered=in->body=in->gates=0;
        in->pending_gate=0; in->gate_frame=0;
    }
    uint8_t row[P4LS_RECORD_SIZE];
    const size_t n=fread(row,1,sizeof row,in->file);
    if (!n && feof(in->file) && !in->count) { in->eof=1; return 0; }
    if (n!=sizeof row) { in->error="incomplete state-aligned input frame"; return 0; }
    const uint32_t kind=p4ls_get(row)&255u;
    if (kind!=P4LS_PAD && kind!=P4LS_FRAME && kind!=P4LA_META && kind!=P4LA_PAD_GATE) {
        in->error="unknown state-aligned input kind"; return 0;
    }
    if (in->count==in->capacity) {
        const size_t cap=in->capacity ? in->capacity*2 : 16;
        void *p=realloc(in->batch,cap*P4LS_RECORD_SIZE);
        if (!p) { in->error="allocating aligned input frame"; return 0; }
        in->batch=p; in->capacity=cap;
    }
    memcpy(in->batch+in->count++*P4LS_RECORD_SIZE,row,sizeof row);
    if (kind==P4LA_PAD_GATE) {
        if (in->pending_gate) { in->error="consecutive title input gates"; return 0; }
        in->pending_gate=1; in->gate_frame=p4ls_get(row+4); in->gates++; in->body++;
        return 1;
    }
    if (kind==P4LS_PAD) {
        if (in->pending_gate && in->gate_frame!=p4ls_get(row+4)) {
            in->error="title input gate/pad counters differ"; return 0;
        }
        in->pending_gate=0; in->pads++; in->body++;
        return 1;
    }
    if (in->pending_gate) { in->error="title input gate has no pad"; return 0; }
    if (kind!=P4LS_FRAME) return 1;
    if (in->count!=in->pads+in->gates+2 || !(p4ls_get(row)&P4LA_FLAG)) {
        in->error="aligned frame lacks exactly one milestone record"; return 0;
    }
    const uint8_t *meta=in->batch+(in->count-2)*P4LS_RECORD_SIZE;
    if ((p4ls_get(meta)&255u)!=P4LA_META || p4ls_get(meta+4)!=p4ls_get(row+4)) {
        in->error="milestone record is out of order"; return 0;
    }
    in->frame=p4ls_get(row+4);
    for (unsigned i=0;i<8;i++) in->marker[i]=p4ls_get(meta+8+i*4);
    /* Gates belong to the spans paced by the environment: the initial load and every later load interval (NG-7n),
     * never a frame-locked one. */
    if (in->gates && in->marker[1]==P4LA_LOCKED) {
        in->error="input gates inside a frame-locked interval"; return 0;
    }
    memcpy(in->presented,row,sizeof row);
    in->complete=1;
    return 2;
}
static inline int p4la_fill(P4AlignInput *in) {
    if (in->complete) return 1;
    for (;;) {
        const int appended=p4la_append(in);
        if (appended!=1) return appended==2;
    }
}
static inline void p4la_consume(P4AlignInput *in) {
    in->records+=in->count; in->pad_records+=in->pads;
    in->count=0; in->index=in->reads=in->delivered=in->body=0;
    in->gate_held=in->dialog_held=0; in->complete=0;
}
/* Observe the actual start of the event's message wait, or of a field
 * message. This is a semantic state, not a tape or presentation delay. */
static inline void p4la_wait_observe(P4AlignInput *in, const P4WalkHost *host, uint32_t frame) {
    uint32_t key[7]={0}, gate[8]={0}, kind=0;
    const uint32_t work=p4la_movie_work(host);
    const int dialog=p4la_dialog_gate(host,gate);
    const int event_wait=work && (p4la_word(host,work)&0x20u);
    /* A message window outside an event wait (a field message, or an event message
     * its script does not wait on) is timed from its own identity's first frame. */
    if (dialog>0 && (!(gate[0]&P4LA_DIALOG_EVENT) || !event_wait)) {
        kind=2; memcpy(key,gate,sizeof key);
    } else if (event_wait) {
        const uint32_t a=p4la_word(host,work+0x78cu), b=p4la_word(host,work+0x790u);
        const uint32_t c=p4la_word(host,work+0x794u);
        if (a<=0xffffu && b<=0xffu && c<=0x3fu) {
            kind=1; key[0]=P4LA_DIALOG_GATE|P4LA_DIALOG_EVENT|a|(b<<16)|(c<<24);
        }
    }
    if (!kind) { in->wait_kind=0; return; }
    if (kind!=in->wait_kind || memcmp(key,in->wait_key,sizeof key)) {
        in->wait_start=frame;
        in->wait_kind=kind;
        memcpy(in->wait_key,key,sizeof key);
    }
}
static inline int p4la_wait_matches(const P4AlignInput *in, const uint32_t gate[8]) {
    return in->wait_kind==1 ? in->wait_key[0]==gate[0] :
           in->wait_kind==2 && !memcmp(in->wait_key,gate,sizeof in->wait_key);
}
/* The port is the recorded-input source, not a lagging retail receiver.
 * Replay its exact reads/counters and observe fresh milestones; old transport
 * metadata must never hold, skip, or retime the source's input. */
static inline const uint8_t *p4la_source_pad(P4AlignInput *in, uint32_t actual_frame) {
    if (!p4la_fill(in)) return NULL;
    if (in->index<in->body &&
        (p4ls_get(in->batch+in->index*P4LS_RECORD_SIZE)&255u)==P4LA_PAD_GATE) in->index++;
    if (in->index>=in->body) {
        in->error="extra recorded-source pad read"; return NULL;
    }
    const uint8_t *row=in->batch+in->index*P4LS_RECORD_SIZE;
    if (p4ls_get(row+4)!=actual_frame) {
        in->error="recorded-source pad counter/order differs"; return NULL;
    }
    in->index++; in->reads++; in->actual_pad_reads++;
    return row;
}
static inline int p4la_source_frame(P4AlignInput *in, uint32_t actual_frame) {
    if (!p4la_fill(in)) return in->eof ? 3 : -1;
    if (in->index!=in->body || in->reads!=in->pads || in->frame!=actual_frame) {
        in->error="recorded-source input/presentation schedule differs"; return -1;
    }
    p4la_consume(in);
    return 1;
}
static inline const uint8_t *p4la_pad(P4AlignInput *in, uint32_t actual_frame,
                                    int receiver_loading, const P4WalkHost *host) {
    in->dialog_status=0;
    if (in->streaming) {
        while (!in->complete && in->index>=in->body)
            if (!p4la_append(in)) return NULL;
    } else if (!p4la_fill(in)) return NULL;
    const int load=in->marker[1]!=P4LA_LOCKED || receiver_loading;
    if (!load && !in->mapped) {
        in->error="frame-locked pad has no proved counter offset"; return NULL;
    }
    const uint8_t *row;
    int delivered_dialog=0;
    if (in->index<in->body) {
        row=in->batch+in->index*P4LS_RECORD_SIZE;
        if ((p4ls_get(row)&255u)==P4LA_PAD_GATE) {
            uint32_t gate[8]={0};
            const int dialog=(p4ls_get(row+8)&P4LA_DIALOG_GATE)!=0;
            int present=host ? (dialog ? p4la_dialog_gate(host,gate) : p4la_title_gate(host,gate)) : 0;
            int matches=present>0;
            for (unsigned i=0;matches && i<(dialog ? 8u : 4u);i++)
                matches=gate[i]==p4ls_get(row+8+i*4);
            int hold_for_time=0;
            if (dialog) {
                in->dialog_source_frame=p4ls_get(row+4);
                for (unsigned i=0;i<8;i++) in->dialog_expected[i]=p4ls_get(row+8+i*4);
                memcpy(in->dialog_actual,gate,sizeof gate);
                if (!in->dialog_waiting) {
                    in->dialog_recorded_offset=in->dialog_ready_offset=in->dialog_delivered_offset=-1;
                    const uint32_t timing=p4ls_get(row)>>8;
                    if (!timing) {
                        in->dialog_status=3;
                        in->error="dialog gate lacks its recorded wait offset"; return NULL;
                    }
                    in->dialog_recorded_offset=(int32_t)(timing-1u);
                    in->dialog_wait_start=actual_frame;
                    in->dialog_predecessor_valid=in->dialog_previous_valid;
                    in->dialog_predecessor_departed=0;
                    memcpy(in->dialog_predecessor,in->dialog_previous,sizeof in->dialog_predecessor);
                    in->dialog_waiting=1;
                }
                in->dialog_wait=actual_frame-in->dialog_wait_start;
                const uint32_t expected_pos=in->dialog_expected[7], observed_pos=gate[7];
                const int same_page=present>0 && !memcmp(gate,in->dialog_expected,7*sizeof *gate) &&
                    !((expected_pos|observed_pos)&0x80000000u) &&
                    (expected_pos&0xffffu)==(observed_pos&0xffffu);
                const unsigned expected_phase=(expected_pos>>16)&7u, observed_phase=(observed_pos>>16)&7u;
                const int maturing=same_page && (observed_phase<expected_phase ||
                    (observed_phase==expected_phase && (expected_pos&0x80000u) && !(observed_pos&0x80000u)));
                const uint32_t previous_pos=in->dialog_predecessor[7];
                const int same_previous_page=present>0 && in->dialog_predecessor_valid &&
                    !memcmp(gate,in->dialog_predecessor,7*sizeof *gate) &&
                    !((previous_pos|observed_pos)&0x80000000u) &&
                    (previous_pos&0xffffu)==(observed_pos&0xffffu);
                const unsigned previous_phase=(previous_pos>>16)&7u;
                const int reordered_phase=same_previous_page && (observed_phase<previous_phase ||
                    (observed_phase==previous_phase && (previous_pos&0x80000u) && !(observed_pos&0x80000u)));
                const int was_predecessor=present>0 && in->dialog_predecessor_valid &&
                    !memcmp(gate,in->dialog_predecessor,sizeof gate);
                const int predecessor=was_predecessor && !in->dialog_predecessor_departed;
                if ((!present || (maturing && !was_predecessor)) && in->dialog_predecessor_valid)
                    in->dialog_predecessor_departed=1;
                if (present<0 || reordered_phase || (was_predecessor && in->dialog_predecessor_departed) ||
                    (present>0 && !matches && !predecessor && !maturing)) {
                    in->dialog_status=3;
                    in->error="retail dialog identity differs from recorded input";
                    return NULL;
                }
                if (host) p4la_wait_observe(in,host,actual_frame);
                if (matches) {
                    if (!p4la_wait_matches(in,in->dialog_expected) || actual_frame<in->wait_start) {
                        in->dialog_status=3;
                        in->error="dialog gate has no witnessed retail wait start"; return NULL;
                    }
                    const uint32_t elapsed=actual_frame-in->wait_start;
                    if (in->dialog_ready_offset<0) in->dialog_ready_offset=(int32_t)elapsed;
                    if (elapsed>(uint32_t)in->dialog_recorded_offset && !in->late_ok) {
                        in->dialog_status=3;
                        in->error="retail dialog became ready after the recorded input offset"; return NULL;
                    }
                    hold_for_time=elapsed<(uint32_t)in->dialog_recorded_offset;
                    if (!hold_for_time) in->dialog_delivered_offset=(int32_t)elapsed;
                }
            }
            if (!matches || hold_for_time) {
                if (!in->have_pad || (dialog && !(in->last_pad[11]&0x40u))) {
                    in->dialog_status=dialog ? 3 : 0;
                    in->error="input gate has no released prior recorded pad"; return NULL;
                }
                if (dialog) { in->dialog_status=1; in->dialog_held=1; }
                in->gate_held=1; in->held_reads++; in->reads++; in->actual_pad_reads++;
                return in->last_pad;
            }
            if (dialog) {
                in->dialog_status=2; in->dialog_waiting=0; delivered_dialog=1;
                memcpy(in->dialog_previous,in->dialog_expected,sizeof in->dialog_previous);
                in->dialog_previous_valid=1;
            }
            /* A gate's pad record follows it in the same write, but a streaming receiver has only taken the
             * records that had arrived: take the pad itself before delivering it, never the batch's stale
             * slot (NG-7j: the press was delivered as an all-zero packet and took effect one update late). */
            while (in->streaming && in->index+1>=in->body) {
                if (p4la_append(in)!=1) { in->error="input gate has no pad record"; return NULL; }
            }
            row=in->batch+(++in->index)*P4LS_RECORD_SIZE;
            if (!dialog && matches && !(row[11]&0x40u)) in->title_delivered=1;   /* the press, Cross down */
        }
        in->index++; in->delivered++;
        if (!delivered_dialog && in->have_pad &&
            ((in->last_pad[10]&~row[10]) || (in->last_pad[11]&~row[11])))
            in->dialog_previous_valid=0;
        memcpy(in->last_pad,row,P4LS_RECORD_SIZE); in->have_pad=1;
    } else if (load && in->have_pad) {
        row=in->last_pad;
        in->held_reads++;
    } else {
        in->error="extra frame-locked pad read"; return NULL;
    }
    if (!load && (int64_t)actual_frame!=(int64_t)p4ls_get(row+4)+in->offset) {
        in->error="pad ordering/counter differs within the frame-locked interval"; return NULL;
    }
    in->reads++;
    in->actual_pad_reads++;
    return row;
}
/* Returns -1 on error, 0 during load, 1 for a strict frame, 2 at a ready
 * candidate, or 3 for EOF. Only the full-state controller can accept a candidate. */
static inline int p4la_frame(P4AlignInput *in, uint32_t actual_frame,
                            const uint32_t actual[8], int receiver_loading) {
    if (!p4la_fill(in)) return in->eof ? 3 : -1;
    if (in->marker[1]==P4LA_LOCKED && !receiver_loading) {
        if (!in->mapped || in->index!=in->body || in->reads!=in->pads ||
            (int64_t)actual_frame!=(int64_t)in->frame+in->offset) {
            in->error="frame-locked input/presentation schedule differs"; return -1;
        }
        p4la_consume(in); return 1;
    }
    /* A held pre-input gate has not delivered its confirmation edge. Retain
     * the whole group until that UI state, or skip an already-retired title
     * prefix after the receiver's independently observed restore. */
    if (in->gate_held) {
        in->held_reads+=in->delivered;
        if (in->dialog_held || (actual[1]!=P4LA_READY && !(in->restore_mapped && actual[5]!=27u))) {
            in->index=in->reads=in->delivered=0; in->gate_held=in->dialog_held=0;
            return 0;
        }
        if (actual[1]!=P4LA_READY) {
            in->skipped_records+=in->count;
            p4la_consume(in); return 0;
        }
    }
    /* The real restore bounds the remaining initial-load transport. It does
     * not prove equality or manufacture a successful card acknowledgement. */
    if (!in->restore_mapped && in->marker[1]==P4LA_BOOT &&
        (in->marker[7] || actual[7])) {
        while (!in->marker[7]) {
            if (in->marker[1]!=P4LA_BOOT) {
                in->error="initial restore is missing from the source tape"; return -1;
            }
            in->skipped_records+=in->count;
            p4la_consume(in);
            if (!p4la_fill(in)) return in->eof ? 3 : -1;
        }
        if (!actual[7]) {
            if (actual[1]==P4LA_READY) {
                in->error="receiver reached ready without the recorded restore"; return -1;
            }
            in->held_reads+=in->reads<in->pads ? in->reads : in->pads;
            in->index=in->reads=in->delivered=0;
            return 0;
        }
        in->offset=(int64_t)actual[7]-(int64_t)in->marker[7];
        in->restore_mapped=1;
    }
    /* Never skip live source frames when the receiver reaches ready first. */
    if (receiver_loading && actual[1]==P4LA_READY) {
        while (in->marker[1]!=P4LA_READY) {
            if (in->marker[1]==P4LA_LOCKED) {
                in->error="receiver reached ready while source still has live input";
                return -1;
            }
            in->skipped_records+=in->count;
            p4la_consume(in);
            if (!p4la_fill(in)) return in->eof ? 3 : -1;
        }
    }
    if (in->marker[1]==P4LA_READY) {
        const int destination=in->marker[2]==actual[2] && in->marker[3]==actual[3] &&
            in->marker[4]==actual[4] && in->marker[5]==actual[5];
        if (!receiver_loading || actual[1]!=P4LA_READY ||
            (destination && !p4la_same_milestone(in->marker,actual))) {
            /* Repeat the recorded load-end packet only while the receiver is
             * still loading. Its extra reads are explicit per-load evidence. */
            in->held_reads+=in->reads<in->pads ? in->reads : in->pads;
            in->index=in->reads=in->delivered=0;
            return 0;
        }
        /* A different ready destination fails at this load in the controller;
         * never keep executing until it happens to return to the expected map. */
        in->offset=(int64_t)actual_frame-(int64_t)in->frame;
        in->mapped=1;
        p4la_consume(in); return 2;
    }
    p4la_consume(in); return 0;
}
#endif
