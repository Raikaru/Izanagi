/* Golden fusion calendar text: the caller owns a 128-byte text buffer. */
#ifndef P4PC_GOLDEN_TAG_EXPAND_H
#define P4PC_GOLDEN_TAG_EXPAND_H
#include <stddef.h>
#include <stdint.h>
static inline size_t p4_golden_tag_length(const uint8_t *text, size_t capacity)
{
    size_t n = 0;
    while (n < capacity && text[n]) ++n;
    return n;
}


/* Match the retail pass order and its two-byte glyph stepping, but never
 * truncate or write outside the caller's 128-byte buffer. On failure text is
 * unchanged; the guest caller refuses the session with a named reason. */
static inline int p4_golden_expand_name_tags(uint8_t *text, const uint8_t *first,
    const uint8_t *second, const uint8_t *third, const uint8_t *fourth,
    const int8_t offsets[4])
{
    static const char *const tags[4] = {"<P1>", "<P2>", "<R1>", "<R2>"};
    const uint8_t *names[4] = {first, second, third, fourth};
    uint8_t work[128], output[128];
    size_t used = 0;
    size_t length = p4_golden_tag_length(text, sizeof work);
    if (length == sizeof work) return 0;
    for (size_t i = 0; i <= length; ++i) work[i] = text[i];
    for (unsigned t = 0; t < 4; ++t) {
        if (!names[t] || offsets[t] <= 0) return 0;
        const size_t name_size = p4_golden_tag_length(names[t], sizeof output);
        if (name_size == sizeof output) return 0;
        for (;;) {
            size_t pos = 0;
            while (pos < length) {
                if (work[pos] > 0x80) {
                    if (pos + 1 >= length) return 0;
                    pos += 2;
                } else if (length - pos >= 4 && work[pos] == (uint8_t)tags[t][0] &&
                    work[pos + 1] == (uint8_t)tags[t][1] && work[pos + 2] == (uint8_t)tags[t][2] &&
                    work[pos + 3] == (uint8_t)tags[t][3]) {
                    break;
                } else ++pos;
            }
            if (pos == length) break;
            if (pos > length || (size_t)offsets[t] > length - pos ||
                pos + name_size >= sizeof output - used) return 0;
            for (size_t h = 0; h < pos; ++h) output[used++] = work[h];
            for (size_t j = 0; j < name_size; ++j) output[used++] = names[t][j];
            size_t consumed = pos + (size_t)offsets[t];
            length -= consumed;
            for (size_t k = 0; k <= length; ++k) work[k] = work[consumed + k];
        }
    }
    if (length >= sizeof output - used) return 0;
    for (size_t m = 0; m <= length; ++m) output[used + m] = work[m];
    for (size_t n = 0; n <= used + length; ++n) text[n] = output[n];
    return 1;
}
#endif
