/* One logical input tape; load-time holds/skips are counted, never hidden. */
#ifndef P4PC_LOCKSTEP_ALIGN_INPUT_H
#define P4PC_LOCKSTEP_ALIGN_INPUT_H
#include "p4pc/lockstep_alignment.h"
#include <stdlib.h>
typedef struct P4AlignInput {
    FILE *file;
    uint8_t *batch;
    size_t count, capacity, index, pads, reads, delivered;
    uint32_t marker[8], frame;
    uint8_t presented[P4LS_RECORD_SIZE], last_pad[P4LS_RECORD_SIZE];
    uint64_t records, pad_records, actual_pad_reads, held_reads, skipped_records;
    int have_pad, mapped, restore_mapped, gate_held, eof;
    int64_t offset;
    const char *error;
} P4AlignInput;
static inline int p4la_fill(P4AlignInput *in) {
    if (in->count) return 1;
    in->index=in->pads=in->reads=in->delivered=0;
    size_t gates=0;
    int pending_gate=0;
    uint32_t gate_frame=0;
    for (;;) {
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
            if (pending_gate) { in->error="consecutive title input gates"; return 0; }
            pending_gate=1; gate_frame=p4ls_get(row+4); gates++;
        } else if (kind==P4LS_PAD) {
            if (pending_gate && gate_frame!=p4ls_get(row+4)) {
                in->error="title input gate/pad counters differ"; return 0;
            }
            pending_gate=0; in->pads++;
        } else if (pending_gate) {
            in->error="title input gate has no pad"; return 0;
        }
        if (kind==P4LS_FRAME) {
            if (in->count!=in->pads+gates+2 || !(p4ls_get(row)&P4LA_FLAG)) {
                in->error="aligned frame lacks exactly one milestone record"; return 0;
            }
            const uint8_t *meta=in->batch+(in->count-2)*P4LS_RECORD_SIZE;
            if ((p4ls_get(meta)&255u)!=P4LA_META || p4ls_get(meta+4)!=p4ls_get(row+4)) {
                in->error="milestone record is out of order"; return 0;
            }
            in->frame=p4ls_get(row+4);
            for (unsigned i=0;i<8;i++) in->marker[i]=p4ls_get(meta+8+i*4);
            if (gates && in->marker[1]!=P4LA_BOOT) {
                in->error="title input gates outside the initial load"; return 0;
            }
            memcpy(in->presented,row,sizeof row);
            return 1;
        }
    }
}
static inline void p4la_consume(P4AlignInput *in) {
    in->records+=in->count; in->pad_records+=in->pads;
    in->count=0; in->index=in->reads=in->delivered=0; in->gate_held=0;
}
/* The port is the recorded-input source, not a lagging retail receiver.
 * Replay its exact reads/counters and observe fresh milestones; old transport
 * metadata must never hold, skip, or retime the source's input. */
static inline const uint8_t *p4la_source_pad(P4AlignInput *in, uint32_t actual_frame) {
    if (!p4la_fill(in)) return NULL;
    if (in->index<in->count-2 &&
        (p4ls_get(in->batch+in->index*P4LS_RECORD_SIZE)&255u)==P4LA_PAD_GATE) in->index++;
    if (in->index>=in->count-2) {
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
    if (in->index!=in->count-2 || in->reads!=in->pads || in->frame!=actual_frame) {
        in->error="recorded-source input/presentation schedule differs"; return -1;
    }
    p4la_consume(in);
    return 1;
}
static inline const uint8_t *p4la_pad(P4AlignInput *in, uint32_t actual_frame,
                                    int receiver_loading, const P4WalkHost *host) {
    if (!p4la_fill(in)) return NULL;
    const int load=in->marker[1]!=P4LA_LOCKED || receiver_loading;
    if (!load && !in->mapped) {
        in->error="frame-locked pad has no proved counter offset"; return NULL;
    }
    const uint8_t *row;
    if (in->index<in->count-2) {
        row=in->batch+in->index*P4LS_RECORD_SIZE;
        if ((p4ls_get(row)&255u)==P4LA_PAD_GATE) {
            uint32_t gate[8];
            int matches=host && p4la_title_gate(host,gate);
            for (unsigned i=0;matches && i<4;i++)
                matches=gate[i]==p4ls_get(row+8+i*4);
            if (!matches) {
                if (!in->have_pad) { in->error="title input gate has no prior recorded pad"; return NULL; }
                in->gate_held=1; in->held_reads++; in->reads++; in->actual_pad_reads++;
                return in->last_pad;
            }
            row=in->batch+(++in->index)*P4LS_RECORD_SIZE;
        }
        in->index++; in->delivered++;
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
        if (!in->mapped || in->index!=in->count-2 || in->reads!=in->pads ||
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
        if (actual[1]!=P4LA_READY && !(in->restore_mapped && actual[5]!=27u)) {
            in->index=in->reads=in->delivered=0; in->gate_held=0;
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
