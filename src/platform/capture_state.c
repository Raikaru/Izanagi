/* P4PC_CAPTURE_STATE:
 *   proc,NAME,INDEX,PC,COMMAND_TIMER,FIRST_FLIP,LAST_FLIP
 *   event,NAME,INDEX,PC,ID0,ID1,ID2,FRAME,MESSAGE,ROW,ELAPSED,PORTRAIT,FIRST,LAST
 *   camera,FIELD,ROOM,X,Y,Z,RADIUS,RIGHT.xyz,UP.xyz,AT.xyz,EPSILON,OFFSET,FIRST,LAST
 *   battle-menu,OFFSET,FIRST_FLIP,LAST_FLIP
 * The camera predicate must remain true for OFFSET flips. Draw warm-up follows
 * the registered state too, not the nominal capture flip. No game state is changed. */
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/capture_state.h"
#include "p4pc/gmem.h"

typedef struct CaptureState {
    int kind; /* 0 disabled, 1 procedure, 2 camera/player, 3 event, 4 COMMAND offset, 5 rank-up, 6 COMMAND clock */
    char procedure[25]; /* 24-byte BF label plus its host terminator. */
    unsigned index, pc, timer, first, last, window;
    unsigned field, room, offset;
    float player[3], radius, basis[9], epsilon;
    unsigned event_id[3], event_frame, message, row, elapsed, portrait;
    unsigned rankup_frame, community, rank, resource;
    unsigned entered, capture;
    int draw;
} CaptureState;

static CaptureState sCapture;
static int sLoaded;

static int captureUnsigned(const char **cursor, unsigned *value, char separator)
{
    char *end;
    unsigned long number;
    const char *text = *cursor;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    number = strtoul(text, &end, 10);
    if (errno == ERANGE || number > UINT32_MAX || *end != separator) return 0;
    *value = (unsigned)number;
    *cursor = end + (separator != '\0');
    return 1;
}

static int captureFloat(const char **cursor, float *value)
{
    char *end;
    errno = 0;
    *value = strtof(*cursor, &end);
    if (end == *cursor || errno == ERANGE || !isfinite(*value) || *end != ',') return 0;
    *cursor = end + 1;
    return 1;
}

static int captureParse(CaptureState *state, const char *text, unsigned window)
{
    const char *p;
    unsigned i;
    memset(state, 0, sizeof(*state));
    state->window = window;
    if (!strncmp(text, "proc,", 5) || !strncmp(text, "event,", 6) || !strncmp(text, "rankup,", 7)) {
        const char *end;
        size_t length;
        const int event = text[0] == 'e';
        const int rankup = text[0] == 'r';
        p = text + (rankup ? 7 : event ? 6 : 5);
        end = strchr(p, ',');
        if (end == NULL || (length = (size_t)(end - p)) == 0 || length >= sizeof(state->procedure)) return 0;
        memcpy(state->procedure, p, length);
        p = end + 1;
        if (!captureUnsigned(&p, &state->index, ',') || !captureUnsigned(&p, &state->pc, ',')) return 0;
        if (event) {
            for (i = 0; i < 3; i++)
                if (!captureUnsigned(&p, &state->event_id[i], ',')) return 0;
            if (!captureUnsigned(&p, &state->event_frame, ',') ||
                !captureUnsigned(&p, &state->message, ',') || !captureUnsigned(&p, &state->row, ',') ||
                !captureUnsigned(&p, &state->elapsed, ',') || !captureUnsigned(&p, &state->portrait, ',')) return 0;
            state->kind = 3;
        } else if (rankup) {
            if (!captureUnsigned(&p, &state->rankup_frame, ',') ||
                !captureUnsigned(&p, &state->community, ',') ||
                !captureUnsigned(&p, &state->rank, ',') ||
                !captureUnsigned(&p, &state->resource, ',')) return 0;
            state->kind = 5;
        } else {
            if (!captureUnsigned(&p, &state->timer, ',')) return 0;
            state->kind = 1;
        }
        if (!captureUnsigned(&p, &state->first, ',') || !captureUnsigned(&p, &state->last, '\0')) return 0;
    } else if (!strncmp(text, "camera,", 7)) {
        p = text + 7;
        if (!captureUnsigned(&p, &state->field, ',') || !captureUnsigned(&p, &state->room, ',')) return 0;
        for (i = 0; i < 3; i++) if (!captureFloat(&p, &state->player[i])) return 0;
        if (!captureFloat(&p, &state->radius)) return 0;
        for (i = 0; i < 9; i++) if (!captureFloat(&p, &state->basis[i])) return 0;
        if (!captureFloat(&p, &state->epsilon) || !captureUnsigned(&p, &state->offset, ',') ||
            !captureUnsigned(&p, &state->first, ',') || !captureUnsigned(&p, &state->last, '\0')) return 0;
        if (!(state->radius > 0.0f) || !(state->epsilon > 0.0f)) return 0;
        state->kind = 2;
    } else if (!strncmp(text, "battle-menu,", 12) || !strncmp(text, "battle-command,", 15)) {
        const int clock = text[7] == 'c';
        p = text + (clock ? 15 : 12);
        if (!captureUnsigned(&p, &state->offset, ',') ||
            !captureUnsigned(&p, &state->first, ',') || !captureUnsigned(&p, &state->last, '\0')) return 0;
        state->kind = clock ? 6 : 4;
    } else {
        return 0;
    }
    return state->first > 0 && state->last >= state->first;
}

static int captureAddress(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address <= 0x02000000u - size;
}

static uint32_t captureWord(uint32_t address)
{
    return P4_G32(address);
}

static int captureProcedure(const CaptureState *state, unsigned *timer)
{
    uint32_t node = captureWord(0x00764608u);
    unsigned count;
    for (count = 0; count < 32 && captureAddress(node, 0x154); count++) {
        if (!strncmp((const char *)P4_G2H(node), state->procedure, sizeof(state->procedure) - 1) &&
            captureWord(node + 0x12c) == state->index && captureWord(node + 0x18) == state->pc) {
            if (timer != NULL) *timer = captureWord(node + 0x138);
            return 1;
        }
        node = captureWord(node + 0x150);
    }
    return 0;
}

/* The same native readiness used by waitbtl:menu: state 8 alone also contains
 * AI turns, target selection and Help pages. Observe, never advance, those states. */
static int captureBattleMenu(unsigned *timer)
{
    const uint32_t context = captureWord(0x0076449cu);
    uint32_t actor, unit, party, node;
    unsigned i;
    int in_party = 0;
    if (!captureAddress(context, 0x300) || captureWord(context + 0x2fc) != 8u ||
        (captureWord(context + 0x0c) & 0x400000u)) return 0;
    actor = captureWord(context + 0x104);
    if (!captureAddress(actor, 0x34) || (captureWord(actor + 0x0c) & 0xffffu) != 5u) return 0;
    unit = captureWord(actor + 0x30);
    party = captureWord(context + 0x178);
    for (i = 0; i < 8 && captureAddress(party, 0xa70); i++) {
        if (party == unit) in_party = 1;
        party = captureWord(party + 0xa6c);
    }
    if (!in_party) return 0;
    node = captureWord(0x008817ecu);
    for (i = 0; i < 64 && captureAddress(node, 0x10); i++) {
        const uint32_t window = captureWord(node + 0x0c);
        if (captureAddress(window, 0x50)) {
            const uint32_t flags = captureWord(window), phase = flags & 7u;
            const uint32_t selection = captureWord(window + 0x48) & 0xffffu;
            const uint32_t choices = captureWord(window + 0x4c) >> 16;
            if (selection == 1u || selection == 2u || (selection == 0u && choices != 0)) return 0;
            if ((flags & 0x300u) == 0x200u && (phase == 3u || phase == 4u) &&
                (captureWord(window + 0x24) & 0xffu) == 1u &&
                (selection == 0xffffu || (selection == 0u && choices == 0u))) return 0;
        }
        node = captureWord(node + 4u);
    }
    if (timer != NULL) *timer = captureWord(actor + 0x1c);
    return 1;
}

static int captureCamera(const CaptureState *state)
{
    static const unsigned basisWord[9] = {0, 1, 2, 4, 5, 6, 8, 9, 10};
    uint32_t scene = captureWord(0x00762ea0u), manager, node, camera, frame;
    float distance = 0.0f;
    unsigned i, count;
    if (!captureAddress(scene, 12) || captureWord(scene) != state->field || captureWord(scene + 4) != state->room) return 0;
    manager = captureWord(scene + 8);
    if (!captureAddress(manager, 8)) return 0;
    node = captureWord(manager + 4); /* scene resource list 1: player key 0x400 */
    for (count = 0; count < 64 && captureAddress(node, 0x168); count++) {
        if ((captureWord(node) & 0xffffu) == 0x400u) break;
        node = captureWord(node + 0x138);
    }
    if (count == 64 || !captureAddress(node, 0x168)) return 0;
    frame = captureWord(node + 0x164);
    if (!captureAddress(frame, 64)) return 0;
    for (i = 0; i < 3; i++) {
        const float delta = P4_GF32(frame + 48 + 4 * i) - state->player[i];
        distance += delta * delta;
    }
    if (!(distance <= state->radius * state->radius)) return 0;
    camera = captureWord(0x00764b70u);
    if (!captureAddress(camera, 8)) return 0;
    frame = captureWord(camera + 4);
    if (!captureAddress(frame, 144)) return 0;
    for (i = 0; i < 9; i++) {
        if (!(fabsf(P4_GF32(frame + 80 + 4 * basisWord[i]) - state->basis[i]) <= state->epsilon)) return 0;
    }
    return 1;
}

/* evtEvent drives the evtEditLoad work block through its child task. The
 * inner timeline stops for message/asset waits; the enclosing BF timer does
 * not, so it cannot identify a particular rendered dialogue row. */
static uint32_t captureEventWork(const CaptureState *state)
{
    static const uint32_t heads[] = {0x00764adcu, 0x00764af4u, 0x00764ae8u};
    unsigned i, count;
    for (i = 0; i < sizeof(heads) / sizeof(heads[0]); i++) {
        uint32_t node = captureWord(heads[i]);
        for (count = 0; count < 256 && captureAddress(node, 0x40); count++) {
            if (!strncmp((const char *)P4_G2H(node), "evtEvent", 24)) {
                uint32_t driver = captureWord(node + 0x38), task, work;
                if (!captureAddress(driver, 0x10) || captureWord(driver) != 4u) return 0;
                task = captureWord(driver + 0x0c);
                if (!captureAddress(task, 0x3c)) return 0;
                work = captureWord(task + 0x38);
                if (!captureAddress(work, 0x798)) return 0;
                if (captureWord(work + 0x78c) == state->event_id[0] &&
                    captureWord(work + 0x790) == state->event_id[1] &&
                    captureWord(work + 0x794) == state->event_id[2]) return work;
            }
            node = captureWord(node + 0x3c);
        }
    }
    return 0;
}

static int captureEventMessage(const CaptureState *state, uint32_t work)
{
    uint32_t handle, node, window = 0, manager, banks;
    unsigned i;
    if (captureWord(work + 0x14) != state->event_frame ||
        captureWord(work + 0x678) != 1u || captureWord(work + 0x684) != 2u ||
        captureWord(work + 0x688) != state->message) return 0;
    handle = captureWord(work + 0x680);
    /* Walk the same active manager list as itfMesWindowTaskUpdate/choose:N;
     * the global window count is not the largest allocated handle. */
    node = captureWord(0x008817ecu);
    for (i = 0; i < 64 && captureAddress(node, 0x10); i++) {
        if (captureWord(node + 8u) == handle) {
            window = captureWord(node + 0x0cu);
            break;
        }
        node = captureWord(node + 4u);
    }
    if (!captureAddress(window, 0x34)) return 0;
    /* Row phase 4, completed opening transition 0x200; preserve other modes.
     * ItfMesRow.index and elapsed register the actual text/portrait phase. */
    if ((captureWord(window) & 0x307u) != 0x204u ||
        (captureWord(window + 0x2c) & 0xffffu) != state->row ||
        captureWord(window + 0x30) != state->elapsed) return 0;
    manager = captureWord(0x00764678u);
    if (!captureAddress(manager, 0x28)) return 0;
    banks = captureWord(manager + 0x24);
    if (!captureAddress(banks, 0x2c)) return 0;
    for (i = 0; i < 2; i++) {
        const uint32_t slot = banks + 4u + i * 0x14u;
        const uint32_t flags = captureWord(slot);
        /* FclBankSlot: allocated, loaded and visible, no pending load;
         * fade state 3 is the settled visible portrait, not its opening. */
        if ((flags & 0xffff000fu) == 0x0003000du &&
            captureWord(slot + 4u) == state->portrait) return 1;
    }
    return 0;
}

static int captureRankup(const CaptureState *state)
{
    static const uint32_t heads[] = {0x00764adcu, 0x00764af4u, 0x00764ae8u};
    unsigned i, count;
    for (i = 0; i < sizeof(heads) / sizeof(heads[0]); i++) {
        uint32_t node = captureWord(heads[i]);
        for (count = 0; count < 256 && captureAddress(node, 0x40); count++) {
            if (!strncmp((const char *)P4_G2H(node), "cmmRankUpSequence", 24)) {
                const uint32_t work = captureWord(node + 0x38);
                /* CmmRankUpSequence: the draw callback owns mode (+8).
                 * An enclosing BF wait does not identify the card/EPL ages. */
                if (captureAddress(work, 0x5c) &&
                    (captureWord(work) & 0x18u) == 0x18u &&
                    captureWord(work + 4) == 10u &&
                    captureWord(work + 8) == state->rankup_frame &&
                    (captureWord(work + 0x18) & 0xffffu) == state->community &&
                    captureWord(work + 0x1c) == state->rank &&
                    captureWord(work + 0x20) == state->resource) return 1;
            }
            node = captureWord(node + 0x3c);
        }
    }
    return 0;
}

/* Return -1 for a missed registration, otherwise retain the one selected frame. */
static int captureObserve(CaptureState *state, unsigned frame)
{
    unsigned timer;
    state->draw = 0;
    if (!state->kind || state->capture) return 0;
    if (frame > state->last) return -1;
    if (state->kind == 1) {
        if (captureProcedure(state, &timer)) {
            state->draw = timer <= state->timer && state->timer - timer <= state->window + 1u;
            if (timer == state->timer) state->capture = frame;
        }
    } else if (state->kind == 6) {
        const uint32_t context = captureWord(0x0076449cu);
        /* UI draw callbacks may own animation clocks: retain the whole command
         * presentation, including the Help page, not only its last few draws. */
        state->draw = captureAddress(context, 0x300) && captureWord(context + 0x2fc) == 8u;
        if (captureBattleMenu(&timer)) {
            if (timer > state->offset) return -1;
            if (timer == state->offset) state->capture = frame;
        }
    } else if (state->kind == 5) {
        if (captureProcedure(state, NULL)) {
            /* Render the whole sequence: its animation clock advances in draw.
             * A countdown-only warm-up would freeze it below the target. */
            state->draw = 1;
            if (captureRankup(state)) state->capture = frame;
        }
    } else if (state->kind == 3) {
        if (captureProcedure(state, NULL)) {
            const uint32_t work = captureEventWork(state);
            if (work != 0) {
                const unsigned event_frame = captureWord(work + 0x14);
                state->draw = event_frame <= state->event_frame &&
                              state->event_frame - event_frame <= state->window + 1u;
                if (captureEventMessage(state, work)) state->capture = frame;
            }
        }
    } else {
        const int matches = state->kind == 4 ? captureBattleMenu(NULL) : captureCamera(state);
        if (state->kind == 4 && !matches) {
            state->entered = 0;
            return 0;
        }
        if (!state->entered && matches) state->entered = frame;
        if (state->entered) {
            const unsigned elapsed = frame - state->entered;
            if (!matches || elapsed > state->offset) return -1;
            state->draw = state->offset - elapsed <= state->window + 1u;
            if (elapsed == state->offset) state->capture = frame;
        }
    }
    if (state->capture && (state->capture < state->first || state->capture > state->last)) return -1;
    return 0;
}

void p4pc_capture_state_flip(unsigned frame)
{
    if (!sLoaded) {
        const char *text = getenv("P4PC_CAPTURE_STATE");
        const char *warmup = getenv("P4PC_TURBO_WINDOW");
        unsigned window = warmup != NULL && atoi(warmup) > 0 ? (unsigned)atoi(warmup) : 30u;
        sLoaded = 1;
        if (text != NULL && !captureParse(&sCapture, text, window)) {
            fprintf(stderr, "p4pc: invalid P4PC_CAPTURE_STATE: %s\n", text);
            exit(1);
        }
    }
    if (captureObserve(&sCapture, frame) != 0) {
        fprintf(stderr, "p4pc: capture state missed its predicate/offset or sanity bounds [%u,%u] at flip %u\n", sCapture.first, sCapture.last, frame);
        exit(1);
    }
    if (sCapture.capture == frame) {
        fprintf(stderr, "p4pc: capture state matched at flip %u (%s)\n", frame,
                sCapture.kind == 2 ? "field/player/camera" :
                sCapture.kind == 4 || sCapture.kind == 6 ? "party COMMAND menu" : sCapture.procedure);
    }
}

int p4pc_capture_state_wanted(unsigned frame)
{
    return sCapture.kind && sCapture.capture == frame;
}

int p4pc_capture_state_draw(void)
{
    return sCapture.draw;
}

int p4pc_capture_state_gate(unsigned frame)
{
    if (!sCapture.kind) return -1;
    return sCapture.capture && frame > sCapture.capture;
}
