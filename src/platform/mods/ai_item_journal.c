#include "p4pc/mod_entry.h"
#include <stdio.h>
#include <string.h>

/* The three admitted commands write these fields in the acting PC DatUnit.
 * Masks isolate only their bits; id2 is a scalar, not an independent bitset. */
typedef struct Field { unsigned off, size, mask, scalar; } Field;
static const Field fields[] = {
    {0, 2, 0x18, 0}, {4, 2, 0xffff, 1}, {0x14, 4, 0xf00, 0},
    {0x20, 1, 0xff, 0}, {0x21, 1, 0xff, 0},
    {0x28, 1, 0xff, 0}, {0x29, 1, 0xff, 0}
};
#ifndef P4_AI_ITEM_PC_COUNT
#define P4_AI_ITEM_PC_COUNT 10
#endif
#ifndef P4_AI_ITEM_SCRATCH_COUNT
#define P4_AI_ITEM_SCRATCH_COUNT 28
#endif
typedef struct Scratch {
    unsigned char *unit;
    unsigned original, written, changed, command;
} Scratch;
enum { FIELD_COUNT = sizeof fields / sizeof fields[0], PC_COUNT = P4_AI_ITEM_PC_COUNT };
typedef struct Scope {
    const void *action;
    unsigned char *unit;
    unsigned before[FIELD_COUNT], original[FIELD_COUNT], written[FIELD_COUNT], changed[FIELD_COUNT];
    unsigned command, last_command[FIELD_COUNT];
    Scratch scratch[P4_AI_ITEM_SCRATCH_COUNT];
    unsigned scratch_before[2], scratch_index[2], scratch_command, scratch_pending;
    int active, pending, denied;
} Scope;
static Scope scopes[PC_COUNT];
static const void *tainted[PC_COUNT][PC_COUNT];
static unsigned overflow[PC_COUNT];
static int tainted_index(unsigned pc, const void *action)
{
    if (!action) return -1;
    for (unsigned i = 0; i < PC_COUNT; ++i)
        if (tainted[pc - 1][i] == action) return (int)i;
    return -1;
}
unsigned p4_ai_item_conflicts;
static void conflict(unsigned pc, unsigned field, unsigned command)
{
    ++p4_ai_item_conflicts;
    fprintf(stderr, "p4pc: Challenge boundary conflict: unit %u field 0x%x command 0x%x\n", pc, field, command);
}
static void scratch_conflict(unsigned pc, const void *unit, unsigned command)
{
    ++p4_ai_item_conflicts;
    fprintf(stderr, "p4pc: Challenge boundary conflict: actor %u unit %p field 0x18 command 0x%x\n",
            pc, unit, command);
}
static Scope *scope(unsigned pc, const void *action)
{
    if (pc < 1 || pc > PC_COUNT || !action) return NULL;
    Scope *s = &scopes[pc - 1];
    return s->action == action ? s : NULL;
}
static unsigned read_field(const Scope *s, const Field *f)
{
    unsigned value = 0;
    memcpy(&value, s->unit + f->off, f->size);
    return value;
}
void p4_ai_item_begin(unsigned pc, const void *action, void *unit, int active)
{
    if (pc < 1 || pc > PC_COUNT || !action || !unit) { conflict(pc, 0, 0); return; }
    Scope *s = &scopes[pc - 1];
    if (s->action && s->action != action) {
        s->denied = 1;
        int found = tainted_index(pc, action);
        if (found < 0) {
            unsigned i;
            for (i = 0; i < PC_COUNT && tainted[pc - 1][i]; ++i) {}
            if (i == PC_COUNT) overflow[pc - 1] = 1;
            else tainted[pc - 1][i] = action;
        }
        conflict(pc, 0, 0); return;
    }
    if (!s->action) { memset(s, 0, sizeof *s); s->action = action; s->unit = unit; s->active = !!active; }
}
int p4_ai_item_before(unsigned pc, const void *action, unsigned command)
{
    if (pc < 1 || pc > PC_COUNT) return 0;
    if (overflow[pc - 1] || tainted_index(pc, action) >= 0) return 0;
    Scope *s = &scopes[pc - 1];
    if (!s->action) return 1; /* command outside an AI operation */
    if (s->action != action || s->denied) return 0;
    if (!s->active) return 1;
    if (s->pending || (command != 0x2091 && command != 0x2095 && command != 0x2096)) {
        s->denied = 1; conflict(pc, 0, command); return 0;
    }
    for (unsigned i = 0; i < FIELD_COUNT; ++i) s->before[i] = read_field(s, &fields[i]);
    s->command = command; s->pending = 1; return 1;
}
void p4_ai_item_after(unsigned pc, const void *action)
{
    Scope *s = scope(pc, action);
    if (!s || !s->pending) return;
    for (unsigned i = 0; i < FIELD_COUNT; ++i) {
        const Field *f = &fields[i];
        unsigned value = read_field(s, f), changed = (value ^ s->before[i]) & f->mask;
        if (changed && f->scalar) changed = f->mask;
        if (!changed) continue;
        unsigned fresh = changed & ~s->changed[i];
        s->original[i] = (s->original[i] & ~fresh) | (s->before[i] & fresh);
        s->changed[i] |= changed;
        s->written[i] = (s->written[i] & ~s->changed[i]) | (value & s->changed[i]);
        s->last_command[i] = s->command;
    }
    s->pending = 0;
}
static unsigned scratch_read(const unsigned char *unit)
{
    unsigned value;
    memcpy(&value, unit + 0x18, sizeof value);
    return value;
}
int p4_ai_item_prediction_before(unsigned pc, const void *action, void *target, unsigned command)
{
    if (pc < 1 || pc > PC_COUNT) return 0;
    if (overflow[pc - 1] || tainted_index(pc, action) >= 0) return 0;
    Scope *s = &scopes[pc - 1];
    if (!s->action) return 1;
    if (s->action != action || s->denied) return 0;
    if (!s->active) return 1;
    if (s->scratch_pending || !target) {
        s->denied = 1; conflict(pc, 0x18, command); return 0;
    }
    unsigned char *units[2] = {s->unit, target};
    for (unsigned n = 0; n < 2; ++n) {
        unsigned i;
        for (i = 0; i < P4_AI_ITEM_SCRATCH_COUNT; ++i)
            if (!s->scratch[i].unit || s->scratch[i].unit == units[n]) break;
        if (i == P4_AI_ITEM_SCRATCH_COUNT) {
            s->denied = 1; conflict(pc, 0x18, command); return 0;
        }
        s->scratch[i].unit = units[n];
        s->scratch_index[n] = i;
        s->scratch_before[n] = scratch_read(units[n]);
    }
    s->scratch_command = command; s->scratch_pending = 1;
    return 1;
}
void p4_ai_item_prediction_after(unsigned pc, const void *action)
{
    Scope *s = scope(pc, action);
    if (!s || !s->scratch_pending) return;
    for (unsigned n = 0; n < 2; ++n) {
        if (n && s->scratch_index[n] == s->scratch_index[0]) continue;
        Scratch *entry = &s->scratch[s->scratch_index[n]];
        unsigned value = scratch_read(entry->unit);
        unsigned changed = value ^ s->scratch_before[n];
        unsigned fresh = changed & ~entry->changed;
        entry->original = (entry->original & ~fresh) | (s->scratch_before[n] & fresh);
        entry->changed |= changed;
        entry->written = (entry->written & ~entry->changed) | (value & entry->changed);
        if (changed) entry->command = s->scratch_command;
    }
    s->scratch_pending = 0;
}
void p4_ai_item_deny(unsigned pc, const void *action)
{
    Scope *s = scope(pc, action);
    if (s) s->denied = 1;
}
int p4_ai_item_denied(unsigned pc, const void *action)
{
    if (pc < 1 || pc > PC_COUNT) return 1;
    if (overflow[pc - 1] || tainted_index(pc, action) >= 0) return 1;
    Scope *s = &scopes[pc - 1];
    return s->action && (s->action != action || s->denied);
}
int p4_ai_item_finish(unsigned pc, const void *action)
{
    if (pc < 1 || pc > PC_COUNT) return 1;
    int index = tainted_index(pc, action);
    if (index >= 0) { tainted[pc - 1][index] = NULL; return 1; }
    Scope *s = &scopes[pc - 1];
    if (!s->action) return overflow[pc - 1] != 0;
    if (s->action != action) return 1;
    int denied = s->denied || overflow[pc - 1];
    if (denied && s->active) for (unsigned i = 0; i < FIELD_COUNT; ++i) {
        const Field *f = &fields[i]; unsigned mask = s->changed[i];
        if (!mask) continue;
        unsigned value = read_field(s, f), mismatch = (value ^ s->written[i]) & mask;
        if (mismatch) conflict(pc, f->off, s->last_command[i]);
        if (f->scalar && mismatch) continue;
        unsigned restore = mask & ~mismatch;
        value = (value & ~restore) | (s->original[i] & restore);
        memcpy(s->unit + f->off, &value, f->size);
    }
    if (denied && s->active) for (unsigned i = 0; i < P4_AI_ITEM_SCRATCH_COUNT; ++i) {
        Scratch *entry = &s->scratch[i];
        if (!entry->changed) continue;
        unsigned value = scratch_read(entry->unit);
        unsigned mismatch = (value ^ entry->written) & entry->changed;
        if (mismatch) scratch_conflict(pc, entry->unit, entry->command);
        unsigned restore = entry->changed & ~mismatch;
        value = (value & ~restore) | (entry->original & restore);
        memcpy(entry->unit + 0x18, &value, sizeof value);
    }
    memset(s, 0, sizeof *s);
    return denied;
}
void p4_ai_item_forget(const void *action)
{
    for (unsigned pc = 1; pc <= PC_COUNT; ++pc) {
        int index = tainted_index(pc, action);
        if (index >= 0) tainted[pc - 1][index] = NULL;
        Scope *s = scope(pc, action);
        if (s) { s->denied = 1; (void)p4_ai_item_finish(pc, action); }
    }
}
