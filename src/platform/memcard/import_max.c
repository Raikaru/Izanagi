/* MAX framing/CRC/alignment reference: Ross Ridge's public-domain mymc,
 * https://github.com/ps2dev/mymc/blob/master/ps2save.py (load_max_drive,
 * save_max_drive). The 4-byte decoded length begins the LZARI stream at 88.
 * Arithmetic model and LZSS decoding adapted from Haruhiko Okumura's
 * public-domain LZARI.C (1989), "Use, distribute, and modify ... freely":
 * https://github.com/bucanero/psv-save-converter/blob/master/src/lzari.c
 * See third_party/README.md. All state is per call; input is untrusted. */
#include "import_max.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 64u
#define MAX_PAYLOAD (8u * 1024u * 1024u)
#define HIST 4096u
#define CHARS 314u
#define Q1 32768u
#define Q2 65536u
#define Q3 98304u
#define Q4 131072u

typedef struct MaxLzari {
    const unsigned char *input;
    size_t size, offset;
    unsigned int mask, byte, tail_bits;
    uint32_t low, high, value;
    unsigned int freq[CHARS + 1], cum[CHARS + 1];
    unsigned int symbols[CHARS + 1], positions[HIST + 1];
    unsigned char history[HIST];
    int failed;
} MaxLzari;

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int fail(char *error, size_t cap, const char *message)
{
    if (error && cap) (void)snprintf(error, cap, "%s", message);
    return 0;
}

static uint32_t max_crc(const unsigned char *p, size_t size)
{
    uint32_t crc = UINT32_MAX;
    size_t i;
    unsigned int bit;
    for (i = 0; i < size; ++i) {
        crc ^= (i >= 12 && i < 16) ? 0u : p[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static unsigned int get_bit(MaxLzari *s)
{
    unsigned int bit;
    if (!s->mask) {
        if (s->offset == s->size) {
            /* Arithmetic decoding reads ahead of the two termination bits.
             * At most 16 implicit zero bits are needed, never unbounded EOF. */
            if (++s->tail_bits > 16) s->failed = 1;
            return 0;
        }
        s->byte = s->input[s->offset++];
        s->mask = 128;
    }
    bit = (s->byte & s->mask) != 0;
    s->mask >>= 1;
    return bit;
}

static void update_model(MaxLzari *s, unsigned int symbol)
{
    unsigned int i, sum, temp;
    if (s->cum[0] >= Q1 - 1) {
        sum = 0;
        for (i = CHARS; i > 0; --i) {
            s->cum[i] = sum;
            s->freq[i] = (s->freq[i] + 1) / 2;
            sum += s->freq[i];
        }
        s->cum[0] = sum;
    }
    for (i = symbol; s->freq[i] == s->freq[i - 1]; --i) {}
    temp = s->symbols[i];
    s->symbols[i] = s->symbols[symbol];
    s->symbols[symbol] = temp;
    ++s->freq[i];
    while (i > 0) ++s->cum[--i];
}

static unsigned int decode_interval(MaxLzari *s, const unsigned int *cum,
                                    unsigned int count)
{
    uint32_t range, scaled;
    unsigned int lo = 1, hi = count, mid;
    if (s->failed || s->value < s->low || s->value >= s->high) {
        s->failed = 1;
        return 1;
    }
    range = s->high - s->low;
    /* 64-bit intermediates also cover the larger static position model. */
    scaled = (uint32_t)(((uint64_t)(s->value - s->low + 1) * cum[0] - 1) / range);
    while (lo < hi) {
        mid = (lo + hi) / 2;
        if (cum[mid] > scaled) lo = mid + 1;
        else hi = mid;
    }
    s->high = s->low + (uint32_t)((uint64_t)range * cum[lo - 1] / cum[0]);
    s->low += (uint32_t)((uint64_t)range * cum[lo] / cum[0]);
    for (;;) {
        if (s->low >= Q2) {
            s->value -= Q2; s->low -= Q2; s->high -= Q2;
        } else if (s->low >= Q1 && s->high <= Q3) {
            s->value -= Q1; s->low -= Q1; s->high -= Q1;
        } else if (s->high > Q2) break;
        s->low *= 2; s->high *= 2;
        s->value = s->value * 2 + get_bit(s);
        if (s->failed) break;
    }
    return lo;
}

static int decompress(const unsigned char *input, size_t size,
                      unsigned char *output, size_t length)
{
    MaxLzari s;
    size_t written = 0;
    unsigned int i, symbol, ch, position, run, ring = HIST - 60;
    memset(&s, 0, sizeof(s));
    s.input = input; s.size = size; s.high = Q4;
    for (i = CHARS; i > 0; --i) {
        s.symbols[i] = i - 1;
        s.freq[i] = 1;
        s.cum[i - 1] = s.cum[i] + 1;
    }
    for (i = HIST; i > 0; --i)
        s.positions[i - 1] = s.positions[i] + 10000u / (i + 200u);
    memset(s.history, ' ', HIST - 60);
    for (i = 0; i < 17; ++i) s.value = s.value * 2 + get_bit(&s);
    while (written < length && !s.failed) {
        symbol = decode_interval(&s, s.cum, CHARS);
        ch = s.symbols[symbol];
        update_model(&s, symbol);
        if (ch < 256) {
            output[written++] = (unsigned char)ch;
            s.history[ring] = (unsigned char)ch;
            ring = (ring + 1) & (HIST - 1);
        } else {
            position = decode_interval(&s, s.positions, HIST) - 1;
            run = ch - 253;
            if (run > length - written) return 0;
            position = (ring - position - 1) & (HIST - 1);
            for (i = 0; i < run; ++i) {
                ch = s.history[(position + i) & (HIST - 1)];
                output[written++] = (unsigned char)ch;
                s.history[ring] = (unsigned char)ch;
                ring = (ring + 1) & (HIST - 1);
            }
        }
    }
    return !s.failed && written == length;
}

int p4save_decode_max(const unsigned char *blob, size_t size,
                      P4SaveArchive *out, char *error, size_t cap)
{
    unsigned char *decoded;
    size_t length, compressed, count, offset = 0, total = 0, i, bytes, padding;
    if (!blob || !out || size < 92)
        return fail(error, cap, "Truncated MAX header");
    if (memcmp(blob, "Ps2PowerSave", 12))
        return fail(error, cap, "Invalid MAX magic");
    count = le32(blob + 84);
    length = le32(blob + 88);
    compressed = le32(blob + 80);
    if (!count || count > MAX_FILES)
        return fail(error, cap, "Invalid MAX file count");
    if (!length || length > MAX_PAYLOAD + count * 51u)
        return fail(error, cap, "MAX decompressed output exceeds limit");
    /* mymc documents writers that put the decoded size here; other writers
     * exclude the stream's four-byte length. Accept both recorded variants. */
    if (compressed != size - 88 && compressed != size - 92 && compressed != length)
        return fail(error, cap, "Invalid MAX compressed size");
    if (max_crc(blob, size) != le32(blob + 12))
        return fail(error, cap, "MAX CRC32 mismatch");
    decoded = (unsigned char *)malloc(length);
    if (!decoded) return fail(error, cap, "Cannot allocate MAX decompression buffer");
    if (!decompress(blob + 92, size - 92, decoded, length)) {
        free(decoded);
        return fail(error, cap, "Invalid or truncated MAX LZARI stream");
    }
    memcpy(out->name, blob + 16, 32);
    out->name[32] = '\0';
    for (i = 0; i < count; ++i) {
        P4SaveFile *file = &out->files[i];
        if (length - offset < 36) goto malformed;
        bytes = le32(decoded + offset);
        memcpy(file->name, decoded + offset + 4, 32);
        file->name[32] = '\0';
        offset += 36;
        if (bytes > length - offset || bytes > MAX_PAYLOAD - total) goto malformed;
        file->data = (unsigned char *)malloc(bytes ? bytes : 1);
        if (!file->data) {
            free(decoded);
            return fail(error, cap, "Cannot allocate MAX file buffer");
        }
        file->size = bytes;
        out->count = i + 1;
        memcpy(file->data, decoded + offset, bytes);
        total += bytes;
        offset += bytes;
        /* Entry starts are aligned relative to eight bytes BEFORE the stream,
         * not to the file data or to the start of the decompressed buffer. */
        padding = (16 - ((offset + 8) & 15)) & 15;
        if (padding > length - offset) {
            if (i + 1 != count || offset != length) goto malformed;
        } else offset += padding;
    }
    if (offset != length) goto malformed;
    free(decoded);
    if (error && cap) error[0] = '\0';
    return 1;
malformed:
    free(decoded);
    return fail(error, cap, "Invalid MAX file entries or payload limit exceeded");
}
