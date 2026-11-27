/* Original ACB reader; @UTF value access is shared with CPK, not reimplemented. */
#include "p4pc/golden_audio.h"
#include "p4pc/content_digest.h"
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static const char *const table_names[P4_ACB_TABLE_COUNT] = {
    "CueTable", "CueNameTable", "WaveformTable", "SynthTable", "SequenceTable",
    "TrackTable", "TrackEventTable", "CommandTable", "WaveformExtensionDataTable",
    "SeqCommandTable", "TrackCommandTable", "SynthCommandTable", "StringValueTable"
};
static int number(const P4GoldenUtf *t, uint32_t row, const char *name, uint32_t *out)
{
    uint64_t value;
    if (p4_golden_utf_u64(t, row, name, &value) || value > UINT32_MAX) return -1;
    *out = (uint32_t)value;
    return 0;
}
static uint32_t optional(const P4GoldenUtf *t, uint32_t row, const char *name, uint32_t fallback)
{
    uint32_t value;
    return number(t, row, name, &value) ? fallback : value;
}
static int validate(const P4GoldenUtf *table)
{
    if (table->row_count > (1u << 20)) return -1;
    for (uint32_t c = 0; c < table->column_count; ++c) {
        const char *name = (const char *)table->table + table->strings_offset + table->columns[c].name;
        if (!memchr(name, 0, table->size - table->strings_offset - table->columns[c].name)) return -1;
        for (uint32_t r = 0; r < table->row_count; ++r) {
            if (table->columns[c].type == 10) {
                const char *value;
                if (p4_golden_utf_str(table, r, name, &value)) return -1;
            } else if (table->columns[c].type == 11) {
                uint8_t *value; size_t length;
                if (p4_golden_utf_blob(table, r, name, &value, &length)) return -1;
            }
        }
    }
    return 0;
}
struct P4GoldenCueSelectors {
    P4GoldenCueRandom random;
    void *context;
    int32_t *previous_sequence;
};
int p4_golden_acb_open(P4GoldenAcb *acb, uint8_t *bytes, size_t size)
{
    static const char *const selected[] = {
        "CueTable", "CueNameTable", "WaveformTable", "SynthTable", "SequenceTable",
        "TrackTable", "TrackEventTable", "CommandTable", "WaveformExtensionDataTable",
        "SeqCommandTable", "TrackCommandTable", "SynthCommandTable", "StringValueTable", "AwbFile", NULL
    };
    uint8_t *bank;
    size_t length;
    if (!acb) return -1;
    memset(acb, 0, sizeof *acb);
    if (p4_golden_utf_open_selected(&acb->header, bytes, size, selected) || acb->header.row_count != 1 ||
        validate(&acb->header)) goto bad;
    for (unsigned i = 0; i < P4_ACB_TABLE_COUNT; ++i) {
        uint8_t *data;
        size_t n;
        if (p4_golden_utf_blob(&acb->header, 0, table_names[i], &data, &n)) continue;
        if (n && (p4_golden_utf_open(&acb->tables[i], data, n) || validate(&acb->tables[i]))) goto bad;
    }
    if (!acb->tables[P4_ACB_CUE].table || !acb->tables[P4_ACB_NAME].table || !acb->tables[P4_ACB_WAVE].table) goto bad;
    if (!p4_golden_utf_blob(&acb->header, 0, "AwbFile", &bank, &length) && length &&
        p4_golden_awb_open(&acb->memory, bank, length)) goto bad;
    return 0;
bad:
    p4_golden_acb_close(acb);
    return -1;
}
void p4_golden_acb_close(P4GoldenAcb *acb)
{
    if (acb) {
        p4_golden_awb_close(&acb->memory);
        if (acb->selectors) {
            free(acb->selectors->previous_sequence);
            free(acb->selectors);
        }
        memset(acb, 0, sizeof *acb);
    }
}
int p4_golden_acb_cue(const P4GoldenAcb *acb, uint32_t id, P4GoldenCue *cue)
{
    const P4GoldenUtf *ct = &acb->tables[P4_ACB_CUE], *nt = &acb->tables[P4_ACB_NAME];
    if (!cue) return -1;
    memset(cue, 0, sizeof *cue);
    cue->name = "";
    int found = 0;
    for (uint32_t row = 0; row < ct->row_count; ++row) {
        uint32_t cid;
        if (number(ct, row, "CueId", &cid)) return -1;
        if (cid == id) {
            if (found++) return -1;
            cue->id = id; cue->index = row;
            if (number(ct, row, "Length", &cue->length_ms)) return -1;
        }
    }
    if (!found) return 1;
    for (uint32_t row = 0; row < nt->row_count; ++row) {
        uint32_t index;
        if (number(nt, row, "CueIndex", &index) || index >= ct->row_count) return -1;
        if (index == cue->index && p4_golden_utf_str(nt, row, "CueName", &cue->name)) return -1;
    }
    return 0;
}
typedef struct Resolve {
    const P4GoldenAcb *acb;
    P4GoldenCueWave *waves;
    size_t count, capacity;
    unsigned steps;
} Resolve;
static int reference(Resolve *r, uint32_t type, uint32_t index, unsigned depth);
static int waveform(Resolve *r, uint32_t index)
{
    const P4GoldenUtf *t = &r->acb->tables[P4_ACB_WAVE];
    P4GoldenCueWave wave;
    uint32_t streaming, flag;
    memset(&wave, 0, sizeof wave);
    wave.index = index;
    if (index >= t->row_count || r->count == r->capacity ||
        number(t, index, "Streaming", &streaming) || streaming > 1 ||
        number(t, index, "EncodeType", &wave.codec) || number(t, index, "SamplingRate", &wave.rate) ||
        number(t, index, "NumChannels", &wave.channels) || number(t, index, "NumSamples", &wave.samples) ||
        number(t, index, "LoopFlag", &flag) || flag > 2) return -1;
    wave.streaming = (int)streaming;
    wave.loop = flag == 2;
    wave.port = optional(t, index, "StreamAwbPortNo", 0);
    if (wave.port == 65535) wave.port = 0;
    if (number(t, index, "Id", &wave.id) &&
        number(t, index, streaming ? "StreamAwbId" : "MemoryAwbId", &wave.id)) return -1;
    if (!wave.channels || wave.channels > 8 || !wave.rate) return -1;
    uint32_t extension = optional(t, index, "ExtensionData", 65535);
    if (wave.loop && extension != 65535) {
        const P4GoldenUtf *ext = &r->acb->tables[P4_ACB_EXTENSION];
        if (number(ext, extension, "LoopStart", &wave.loop_start) ||
            number(ext, extension, "LoopEnd", &wave.loop_end) ||
            wave.loop_start >= wave.loop_end || wave.loop_end > wave.samples) return -1;
    }
    r->waves[r->count++] = wave;
    return 0;
}
static int event(Resolve *r, uint32_t index, unsigned depth)
{
    const P4GoldenUtf *t = &r->acb->tables[P4_ACB_EVENT];
    uint8_t *bytes; size_t size, pos = 0;
    if (!t->table) t = &r->acb->tables[P4_ACB_COMMAND];
    if (p4_golden_utf_blob(t, index, "Command", &bytes, &size)) return -1;
    while (pos < size) {
        if (size - pos < 3) return -1;
        uint16_t code = be16(bytes + pos);
        size_t length = bytes[pos + 2];
        pos += 3;
        if (length > size - pos) return -1;
        /* Atom note-on contains a big-endian reference type/index pair. */
        if (code == 2000) {
            if (length < 4 || reference(r, be16(bytes + pos), be16(bytes + pos + 2), depth + 1)) return -1;
        }
        pos += length;
    }
    return 0;
}
static int reference(Resolve *r, uint32_t type, uint32_t index, unsigned depth)
{
    uint8_t *data; size_t size;
    if (depth >= 32 || ++r->steps > 4096) return -1;
    if (type == 1) return waveform(r, index);
    if (type == 2) {
        const P4GoldenUtf *t = &r->acb->tables[P4_ACB_SYNTH];
        if (p4_golden_utf_blob(t, index, "ReferenceItems", &data, &size) || size % 4) return -1;
        for (size_t p = 0; p < size; p += 4) {
            uint16_t kind = be16(data + p);
            if (!kind) break;
            if (reference(r, kind, be16(data + p + 2), depth + 1)) return -1;
        }
        return 0;
    }
    if (type == 3) {
        const P4GoldenUtf *t = &r->acb->tables[P4_ACB_SEQUENCE];
        uint32_t tracks;
        if (number(t, index, "NumTracks", &tracks) || tracks > 4096 ||
            p4_golden_utf_blob(t, index, "TrackIndex", &data, &size) || size != (size_t)tracks * 2) return -1;
        for (uint32_t i = 0; i < tracks; ++i) {
            uint32_t ev;
            if (number(&r->acb->tables[P4_ACB_TRACK], be16(data + i * 2), "EventIndex", &ev) || event(r, ev, depth + 1)) return -1;
        }
        return 0;
    }
    return -1;
}
int p4_golden_acb_resolve(const P4GoldenAcb *acb, const P4GoldenCue *cue, P4GoldenCueWave *waves, size_t capacity)
{
    uint32_t type, index;
    if (!acb || !cue || !waves || capacity > P4_GOLDEN_CUE_MAX_WAVES ||
        number(&acb->tables[P4_ACB_CUE], cue->index, "ReferenceType", &type) ||
        number(&acb->tables[P4_ACB_CUE], cue->index, "ReferenceIndex", &index)) return -1;
    Resolve r = {acb, waves, 0, capacity, 0};
    return reference(&r, type, index, 0) ? -1 : (int)r.count;
}

int p4_golden_acf_qualify(P4GoldenAcf *acf, const uint8_t *bytes, size_t size)
{
    if (!acf) return -1;
    acf->profile = 0;
    if (!bytes || (size != 65536 && size != 6656)) return -1;
    char digest[P4_CONTENT_DIGEST_HEX + 1];
    p4_content_digest(bytes, size, digest);
    if (size == 65536 && !strcmp(digest, "30a344ecfa24e747411ab08712ba768769f0bdbc6ee0bd4c0fc312b5c497ce48"))
        acf->profile = 1;
    else if (size == 6656 && !strcmp(digest, "572a36a2161b7249696fd2f6f44c28346fa9b1ffe30bd6c70ecbaa5bd8779f99"))
        acf->profile = 2;
    return acf->profile ? 0 : -1;
}
int p4_golden_acb_set_dry_acf(P4GoldenAcb *acb, const P4GoldenAcf *acf)
{
    if (!acb) return -1;
    acb->dry_acf_profile = 0;
    if (!acf) return 0;
    if (acf->profile != 1 && acf->profile != 2) return -1;
    acb->dry_acf_profile = acf->profile;
    return 0;
}

int p4_golden_acb_set_random(P4GoldenAcb *acb, P4GoldenCueRandom random, void *context)
{
    if (!acb) return -1;
    P4GoldenCueSelectors *state = NULL;
    if (random) {
        state = calloc(1, sizeof *state);
        if (!state) return -1;
        size_t sequences = acb->tables[P4_ACB_SEQUENCE].row_count;
        state->previous_sequence = malloc((sequences ? sequences : 1) * sizeof(int32_t));
        if (!state->previous_sequence) {
            free(state);
            return -1;
        }
        for (size_t i = 0; i < sequences; ++i) state->previous_sequence[i] = -1;
        state->random = random;
        state->context = context;
    }
    if (acb->selectors) {
        free(acb->selectors->previous_sequence);
        free(acb->selectors);
    }
    acb->selectors = state;
    return 0;
}

typedef struct Context {
    float gain, sends[8];
    unsigned send_mask;
} Context;
typedef struct Plan {
    const P4GoldenAcb *acb;
    P4GoldenCuePlan *out;
    char *reason;
    size_t capacity;
    uint32_t path_type[32], path_index[32];
    unsigned depth, steps;
    int emit; /* validate every alternative; emit only the selected occurrences */
    int fixture;
} Plan;
static int plan_ref(Plan *p, uint32_t type, uint32_t index, uint64_t start, const Context *context);
static int unsupported(Plan *p, const char *text)
{
    if (p->reason && p->capacity) snprintf(p->reason, p->capacity, "%s", text);
    return -1;
}
static int schema_names(const P4GoldenUtf *t, const char *const *names, unsigned count, unsigned required)
{
    if (!t->table || t->column_count < required || t->column_count > count) return 0;
    uint32_t seen = 0;
    for (uint32_t c = 0; c < t->column_count; ++c) {
        const char *name = (const char *)t->table + t->strings_offset + t->columns[c].name;
        unsigned i;
        for (i = 0; i < count; ++i) if (!strcmp(name, names[i])) break;
        if (i == count || (seen & (1u << i))) return 0;
        seen |= 1u << i;
    }
    return (seen & ((1u << required) - 1)) == ((1u << required) - 1);
}
static int hierarchy_schema(Plan *p, uint32_t root_type)
{
    static const char *const sequence[] = {
        "PlaybackRatio", "NumTracks", "TrackIndex", "CommandIndex", "LocalAisacs",
        "GlobalAisacStartIndex", "GlobalAisacNumRefs", "ParameterPallet",
        "ActionTrackStartIndex", "NumActionTracks", "TrackValues", "Type",
        "ControlWorkArea1", "ControlWorkArea2", "InstPluginTrackStartIndex",
        "NumInstPluginTracks", "Guid"
    };
    static const char *const track[] = {
        "EventIndex", "CommandIndex", "LocalAisacs", "GlobalAisacStartIndex",
        "GlobalAisacNumRefs", "ParameterPallet", "TargetType", "TargetName",
        "TargetId", "TargetAcbName", "Scope", "TargetTrackNo", "Guid"
    };
    static const char *const synth[] = {
        "Type", "VoiceLimitGroupName", "CommandIndex", "ReferenceItems", "LocalAisacs",
        "GlobalAisacStartIndex", "GlobalAisacNumRefs", "ControlWorkArea1", "ControlWorkArea2",
        "TrackValues", "ParameterPallet", "ActionTrackStartIndex", "NumActionTracks", "Guid"
    };
    /* A distinct named-column port fixture family, never a partial retail schema.
     * Optional controller columns use only the defaults documented for fixtures. */
    static const char *const fixture_sequence[] = {
        "NumTracks", "TrackIndex", "TrackValues", "CommandIndex", "PlaybackRatio", "ParameterPallet",
        "Type", "LocalAisacs", "GlobalAisacStartIndex", "GlobalAisacNumRefs",
        "ActionTrackStartIndex", "NumActionTracks", "InstPluginTrackStartIndex", "NumInstPluginTracks"
    };
    static const char *const fixture_track[] = {
        "EventIndex", "CommandIndex", "LocalAisacs", "GlobalAisacStartIndex", "GlobalAisacNumRefs", "ParameterPallet"
    };
    static const char *const fixture_synth[] = {
        "ReferenceItems", "CommandIndex", "Type", "LocalAisacs", "GlobalAisacStartIndex",
        "GlobalAisacNumRefs", "ParameterPallet", "ActionTrackStartIndex", "NumActionTracks"
    };
    const P4GoldenUtf *s = &p->acb->tables[P4_ACB_SEQUENCE];
    const P4GoldenUtf *t = &p->acb->tables[P4_ACB_TRACK];
    const P4GoldenUtf *y = &p->acb->tables[P4_ACB_SYNTH];
    if (schema_names(s, sequence, 17, 16) && schema_names(t, track, 13, 12) && schema_names(y, synth, 14, 13))
        return 0;
    if ((root_type == 1 && !s->table && !t->table && !y->table) ||
        (schema_names(s, fixture_sequence, 14, 6) && schema_names(t, fixture_track, 6, 2) &&
         schema_names(y, fixture_synth, 9, 2))) {
        p->fixture = 1;
        return 0;
    }
    return unsupported(p, "A7: unknown/old hierarchy schema is outside the modern dry and explicit port-fixture profiles");
}
/* Distinguish a missing optional fixture column from a present malformed value. */
static int setting(const P4GoldenUtf *t, uint32_t row, const char *name, uint32_t fallback, uint32_t *value)
{
    if (row >= t->row_count) return -1;
    for (uint32_t c = 0; c < t->column_count; ++c) {
        const char *column = (const char *)t->table + t->strings_offset + t->columns[c].name;
        if (!strcmp(column, name)) return number(t, row, name, value);
    }
    *value = fallback;
    return 0;
}
static int parameters(Plan *p, const P4GoldenUtf *t, uint32_t row, unsigned command_table,
                      const Context *parent, Context *out)
{
    static const char *const counts[] = {
        "NumActionTracks", "NumInstPluginTracks", "GlobalAisacNumRefs"
    };
    *out = *parent;
    uint32_t value;
    if (!p->fixture && command_table == P4_ACB_TRACK_COMMAND) {
        const char *name, *acb_name;
        uint32_t type, id, scope, track;
        if (number(t, row, "TargetType", &type) || number(t, row, "TargetId", &id) ||
            number(t, row, "Scope", &scope) || number(t, row, "TargetTrackNo", &track) ||
            p4_golden_utf_str(t, row, "TargetName", &name) || p4_golden_utf_str(t, row, "TargetAcbName", &acb_name))
            return unsupported(p, "A8: malformed track action-target metadata");
        if (type || id != UINT32_MAX || scope || track != 65535 || *name || *acb_name)
            return unsupported(p, "A5: nondefault track action targeting is outside the dry profile");
    }
    if (!p->fixture && command_table == P4_ACB_SYNTH_COMMAND) {
        const char *group;
        if (p4_golden_utf_str(t, row, "VoiceLimitGroupName", &group))
            return unsupported(p, "A8: malformed synth voice-limit group");
        if (*group) return unsupported(p, "A3: authored voice-limit groups are outside the measured admission profile");
    }
    if (setting(t, row, "ParameterPallet", 65535, &value)) return unsupported(p, "A8: malformed parameter-pallet index");
    if (value != 65535) return unsupported(p, "A3: parameter-pallet inheritance is not approved");
    for (size_t i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        if (setting(t, row, counts[i], 0, &value)) return unsupported(p, "A8: malformed action/AISAC count");
        if (value) return unsupported(p, i == 2 ? "A3: global AISAC processing remains unspecified" : "A5: action/plugin-track processing remains unspecified");
    }
    static const char *const starts[] = {
        "ActionTrackStartIndex", "InstPluginTrackStartIndex", "GlobalAisacStartIndex"
    };
    for (size_t i = 0; i < sizeof starts / sizeof starts[0]; ++i) {
        if (setting(t, row, starts[i], 65535, &value)) return unsupported(p, "A8: malformed authored-controller start index");
        if (value != 65535) return unsupported(p, "A3/A5: authored-controller start is outside the empty dry profile");
    }
    for (uint32_t c = 0; c < t->column_count; ++c) {
        const char *name = (const char *)t->table + t->strings_offset + t->columns[c].name;
        if (!strcmp(name, "LocalAisacs")) {
            uint8_t *local; size_t local_size;
            if (p4_golden_utf_blob(t, row, name, &local, &local_size))
                return unsupported(p, "A8: malformed LocalAisacs reference blob");
            if (local_size) return unsupported(p, "pending:M8d3-dsp: specified local time-AISAC automation");
        }
    }
    if (setting(t, row, "CommandIndex", 65535, &value)) return unsupported(p, "A8: malformed parameter-command index");
    if (value == 65535) return 0;
    const P4GoldenUtf *commands = &p->acb->tables[command_table];
    if (!commands->table) commands = &p->acb->tables[P4_ACB_COMMAND];
    uint8_t *bytes; size_t size;
    if (p4_golden_utf_blob(commands, value, "Command", &bytes, &size))
        return unsupported(p, "A8: missing or out-of-range parameter-command row");
    float gain = 1, sends[8] = {0};
    unsigned send_mask = 0;
    for (size_t pos = 0; pos < size;) {
        if (++p->steps > 65536 || size - pos < 3) return unsupported(p, "A8: truncated or over-limit parameter program");
        uint16_t code = be16(bytes + pos);
        size_t length = bytes[pos + 2];
        pos += 3;
        if (length > size - pos) return unsupported(p, "A8: truncated parameter-command payload");
        if (code == 87 && length == 2) gain = be16(bytes + pos) / 100.0f;
        else if (code == 89 && length == 4) {
            if (be16(bytes + pos + 2)) return unsupported(p, p->acb->selectors ?
                "pending:M8d3-dsp: randomized parameter gain" : "A1: randomized parameter gain requires an injected RNG");
            gain = be16(bytes + pos) / 100.0f;
        } else if (code == 65 && (length == 4 || length == 8)) {
            if (!p->acb->dry_acf_profile) return unsupported(p, "A3: category binding requires a hash-qualified registered dry ACF");
            if (p->depth != 1 || command_table != P4_ACB_SEQ_COMMAND)
                return unsupported(p, "A3: nested category-instance binding is outside the measured dry profile");
            for (size_t i = 0; i < length; i += 4) {
                uint32_t id = (uint32_t)be16(bytes + pos + i) << 16 | be16(bytes + pos + i + 2);
                /* Both hash-qualified ACFs prove gain1 and zero retrigger/
                 * AISAC/React for these categories; other rows are not assumed unity. */
                if (id != 2 && id != 8 && id != 9) return unsupported(p, "pending:M8d3-dsp: category is outside the implemented dry approval");
                p->out->category_mask |= 1u << id;
            }
            if ((p->out->category_mask & ((1u << 8) | (1u << 9))) == ((1u << 8) | (1u << 9)))
                return unsupported(p, "A3: same-group category binding requires arbitration proof");
        } else if (code == 111 && length == 4) {
            if (!p->acb->dry_acf_profile) return unsupported(p, "A3: named sends require a hash-qualified registered dry ACF");
            const char *name;
            if (p4_golden_utf_str(&p->acb->tables[P4_ACB_STRING], be16(bytes + pos), "StringValue", &name))
                return unsupported(p, "A8: named send StringValueTable row is missing or malformed");
            unsigned bus;
            if (!strcmp(name, "MasterOut")) bus = 0;
            else if (strlen(name) == 4 && !memcmp(name, "BUS", 3) && name[3] >= '1' && name[3] <= '7')
                bus = (unsigned)(name[3] - '0');
            else return unsupported(p, "A3: named bus does not resolve in the registered measured ACF");
            if (send_mask & (1u << bus)) return unsupported(p, "pending:M8d3-dsp: repeated same-context bus writes");
            send_mask |= 1u << bus;
            sends[bus] = be16(bytes + pos + 2) / 10000.0f;
        } else if (code != 112) {
            if (p->reason && p->capacity)
                snprintf(p->reason, p->capacity, "pending:M8d3-dsp: parameter/effect opcode %u in %s row %u",
                         code, table_names[command_table], value);
            return -1;
        }
        pos += length;
    }
    out->gain = parent->gain * gain;
    if (!isfinite(out->gain)) return unsupported(p, "pending:M8d3-dsp: authored gain product exceeds the finite dry profile");
    for (unsigned bus = 0; bus < 8; ++bus) {
        if (send_mask & (1u << bus)) {
            out->sends[bus] = (parent->send_mask & (1u << bus)) ? parent->sends[bus] * sends[bus] : sends[bus];
            out->send_mask |= 1u << bus;
        }
    }
    return 0;
}
static int choose(Plan *p, uint32_t type, uint32_t index,
                  const P4GoldenUtf *t, uint32_t count, uint32_t *chosen)
{
    if (type != 3 && type != 4) return unsupported(p, "A2: unsupported sequence/synth selector type");
    uint8_t *weights = NULL; size_t size = 0;
    int weighted = 0;
    for (uint32_t c = 0; c < t->column_count; ++c) {
        const char *name = (const char *)t->table + t->strings_offset + t->columns[c].name;
        if (!strcmp(name, "TrackValues")) {
            if (p4_golden_utf_blob(t, index, "TrackValues", &weights, &size))
                return unsupported(p, "A8: malformed selector weight blob");
            weighted = size != 0;
            break;
        }
    }
    if (weighted && size != (size_t)count * 2) return unsupported(p, "A8: selector weight count does not match entries");
    if (type == 4) {
        if (!weighted || count < 2 || !be16(weights)) return unsupported(p, "A2: unproved degenerate Type4 selector");
        for (uint32_t i = 1; i < count; ++i)
            if (be16(weights + i * 2) != be16(weights)) return unsupported(p, "A2: Type4 requires the proved positive equal-weight profile");
    }
    if (!p->emit) { *chosen = 0; return 0; }
    P4GoldenCueSelectors *state = p->acb->selectors;
    if (!state || !state->random) return unsupported(p, "A1: random cue requires an injected inclusive-integer selector");
    int32_t *previous = state->previous_sequence;
    int32_t exclude = type == 4 ? previous[index] : -1;
    uint32_t total = 0, draw;
    if (weighted) {
        for (uint32_t i = 0; i < count; ++i)
            if ((int32_t)i != exclude) total += be16(weights + i * 2);
    } else total = count - 1;
    if (state->random(state->context, total, &draw) || draw > total)
        return unsupported(p, "A1: injected selector returned a value outside its inclusive interval");
    if (!weighted) *chosen = draw;
    else {
        uint32_t cumulative = 0;
        *chosen = count;
        for (uint32_t i = 0; i < count; ++i) {
            if ((int32_t)i == exclude) continue;
            cumulative += be16(weights + i * 2);
            if (cumulative >= draw) { *chosen = i; break; }
        }
        if (*chosen == count) return unsupported(p, "A2: selector has no cumulative boundary");
    }
    if (type == 4) previous[index] = (int32_t)*chosen;
    return 0;
}
static int plan_event(Plan *p, uint32_t track, uint64_t start, const Context *parent)
{
    const P4GoldenUtf *t = &p->acb->tables[P4_ACB_TRACK];
    uint32_t index;
    Context context;
    if (parameters(p, t, track, P4_ACB_TRACK_COMMAND, parent, &context)) return -1;
    if (number(t, track, "EventIndex", &index)) return unsupported(p, "A8: missing track EventIndex");
    const P4GoldenUtf *events = &p->acb->tables[P4_ACB_EVENT];
    if (!events->table) events = &p->acb->tables[P4_ACB_COMMAND];
    uint8_t *bytes; size_t size;
    if (p4_golden_utf_blob(events, index, "Command", &bytes, &size)) return unsupported(p, "A8: missing track event program");
    uint64_t time = start;
    for (size_t pos = 0; pos < size;) {
        if (++p->steps > 65536 || size - pos < 3) return unsupported(p, "A8: truncated or over-limit event program");
        uint16_t code = be16(bytes + pos);
        size_t length = bytes[pos + 2];
        pos += 3;
        if (length > size - pos) return unsupported(p, "A8: truncated event payload");
        if (!code) {
            if (length || pos != size) return unsupported(p, "A8: event end has payload or trailing commands");
            if (p->emit && time > p->out->frames) p->out->frames = time;
            return 0;
        }
        if (code == 2000 && length == 4) {
            if (plan_ref(p, be16(bytes + pos), be16(bytes + pos + 2), time, &context)) return -1;
        } else if (code == 2001 && length == 4) {
            uint32_t ms = (uint32_t)be16(bytes + pos) << 16 | be16(bytes + pos + 2);
            if ((uint64_t)ms * 48 > UINT32_MAX - time) return unsupported(p, "A6: event start exceeds the bounded output-frame range");
            time += (uint64_t)ms * 48;
        } else if (code == 2005 && length == 2) {
            if (be16(bytes + pos)) return unsupported(p, "A6: nonzero fine waits require fractional-frame scheduling proof");
        } else return unsupported(p, "A8: unsupported event opcode or payload width");
        pos += length;
    }
    return unsupported(p, "A8: event program has no terminal end");
}
static int plan_children(Plan *p, uint32_t type, uint32_t index, uint64_t start, const Context *parent)
{
    if (type == 1) {
        if (p->emit && p->out->count == P4_GOLDEN_CUE_MAX_WAVES) return unsupported(p, "A8: cue exceeds the bounded waveform-occurrence count");
        P4GoldenCueWave unselected;
        P4GoldenCueWave *wave = p->emit ? &p->out->waves[p->out->count] : &unselected;
        Resolve r = {p->acb, wave, 0, 1, 0};
        if (waveform(&r, index)) return unsupported(p, "A8: invalid waveform metadata");
        if (wave->codec != 2) return unsupported(p, "CODEC11: RIFF/other waveform decoder route is not specified");
        if (wave->port) return unsupported(p, "BANK-PORT: nonzero stream bank routing is not specified");
        if (wave->channels > 2) return unsupported(p, "pending:M8d3-dsp: multichannel waveform output routing");
        if (start > UINT32_MAX) return unsupported(p, "A6: scheduled start exceeds the bounded output-frame range");
        if (!p->fixture && !p->acb->dry_acf_profile)
            return unsupported(p, "A3: retail dry playback requires a hash-qualified registered ACF");
        if (parent->send_mask || p->acb->dry_acf_profile) {
            if (parent->send_mask != 255 || parent->sends[0] != 1)
                return unsupported(p, "pending:M8d3-dsp: effective MasterOut routing is outside the implemented dry profile");
            for (unsigned bus = 1; bus < 8; ++bus)
                if (parent->sends[bus] != 0) return unsupported(p, "pending:M8d3-dsp: effective wet/unlinked bus send");
        }
        if (!p->emit) return 0;
        p->out->gains[p->out->count] = parent->gain;
        p->out->starts[p->out->count++] = (uint32_t)start;
        uint64_t end = start + ((uint64_t)wave->samples * 48000 + wave->rate - 1) / wave->rate;
        if (end > p->out->frames) p->out->frames = end;
        return 0;
    }
    if (type != 2 && type != 3) return unsupported(p, "A8: unsupported graph reference type");
    const P4GoldenUtf *t = &p->acb->tables[type == 2 ? P4_ACB_SYNTH : P4_ACB_SEQUENCE];
    uint32_t mode, count, ratio;
    uint8_t *data; size_t size;
    if (setting(t, index, "Type", 0, &mode)) return unsupported(p, "A8: malformed container Type");
    if (type == 3) {
        if (number(t, index, "NumTracks", &count) || count > 4096 ||
            p4_golden_utf_blob(t, index, "TrackIndex", &data, &size) || size != (size_t)count * 2)
            return unsupported(p, "A8: malformed sequence track array");
    } else {
        if (p4_golden_utf_blob(t, index, "ReferenceItems", &data, &size) || size % 4 || size / 4 > 4096)
            return unsupported(p, "A8: malformed synth reference array");
        count = 0;
        while ((size_t)count * 4 < size && be16(data + count * 4)) ++count;
    }
    /* A zero-track container returns before selection/parameter construction. */
    if (!count) return 0;
    Context context;
    if (parameters(p, t, index, type == 2 ? P4_ACB_SYNTH_COMMAND : P4_ACB_SEQ_COMMAND, parent, &context)) return -1;
    if (setting(t, index, "PlaybackRatio", 100, &ratio)) return unsupported(p, "A8: malformed PlaybackRatio");
    if (ratio != 0 && ratio != 100) return unsupported(p, "A6: modulated sequence clock is not approved");
    uint32_t chosen = 0;
    int emit = p->emit;
    if (mode) {
        if (type == 2) return unsupported(p, "A2: noninstalled synth selector weight/default profile is not specified");
        if (p->depth != 1) return unsupported(p, "A1: nested random-selector temporal arbitration is not approved");
        if (choose(p, mode, index, t, count, &chosen)) return -1;
    }
    for (uint32_t i = 0; i < count; ++i) {
        p->emit = emit && (!mode || i == chosen);
        int result = type == 3 ? plan_event(p, be16(data + i * 2), start, &context) :
                     plan_ref(p, be16(data + i * 4), be16(data + i * 4 + 2), start, &context);
        p->emit = emit;
        if (result) return -1;
    }
    return 0;
}
static int plan_ref(Plan *p, uint32_t type, uint32_t index, uint64_t start, const Context *context)
{
    if (p->depth == 32 || ++p->steps > 65536) return unsupported(p, "A8: graph exceeds the bounded traversal depth/work");
    for (unsigned i = 0; i < p->depth; ++i)
        if (p->path_type[i] == type && p->path_index[i] == index) return unsupported(p, "A8: recursive cue graph");
    p->path_type[p->depth] = type;
    p->path_index[p->depth++] = index;
    int result = plan_children(p, type, index, start, context);
    --p->depth;
    return result;
}
int p4_golden_acb_plan(const P4GoldenAcb *acb, const P4GoldenCue *cue,
                        P4GoldenCuePlan *plan, char *reason, size_t capacity)
{
    if (!plan) return -1;
    memset(plan, 0, sizeof *plan);
    if (reason && capacity) reason[0] = 0;
    Plan p = {0};
    p.acb = acb; p.out = plan; p.reason = reason; p.capacity = capacity; p.emit = 1;
    uint32_t type, index;
    if (!acb || !cue || number(&acb->tables[P4_ACB_CUE], cue->index, "ReferenceType", &type) ||
        number(&acb->tables[P4_ACB_CUE], cue->index, "ReferenceIndex", &index))
        return unsupported(&p, "A8: invalid cue root");
    if (hierarchy_schema(&p, type)) {
        /* A missing decoder is independent of unknown parameter defaults.
         * This keeps the specified old RIFF sheets from becoming stale A7
         * spec-gap claims without accepting their hierarchy for playback. */
        int count = p4_golden_acb_resolve(acb, cue, plan->waves,
                                        sizeof plan->waves / sizeof plan->waves[0]);
        int i = 0;
        while (i < count && plan->waves[i].codec == 11) ++i;
        memset(plan, 0, sizeof *plan);
        if (count > 0 && i == count)
            return unsupported(&p, "CODEC11: RIFF waveform decoder route is not specified");
        return -1;
    }
    const Context root = {.gain = 1};
    int result = plan_ref(&p, type, index, 0, &root);
    if (!result && plan->count > 1) {
        for (uint32_t i = 0; i < plan->count; ++i)
            if (plan->waves[i].loop) { result = unsupported(&p, "A5: multi-wave authored loop policy is outside the finite profile"); break; }
    }
    /* Stable chronological order preserves same-time authored order while making
     * voice allocation/mixing order follow actual scheduled launches. */
    for (uint32_t i = 1; !result && i < plan->count; ++i) {
        uint32_t j = i;
        while (j && plan->starts[j - 1] > plan->starts[i]) --j;
        if (j == i) continue;
        P4GoldenCueWave wave = plan->waves[i];
        uint32_t start = plan->starts[i];
        float gain = plan->gains[i];
        memmove(plan->waves + j + 1, plan->waves + j, (i - j) * sizeof *plan->waves);
        memmove(plan->starts + j + 1, plan->starts + j, (i - j) * sizeof *plan->starts);
        memmove(plan->gains + j + 1, plan->gains + j, (i - j) * sizeof *plan->gains);
        plan->waves[j] = wave; plan->starts[j] = start; plan->gains[j] = gain;
    }
    if (result) memset(plan, 0, sizeof *plan);
    return result;
}
