/* HCA framing only, original implementation from public format facts. */
#include "p4pc/golden_audio.h"
#include <math.h>
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static uint32_t be32(const uint8_t *p) { return (uint32_t)be16(p) << 16 | be16(p + 2); }
static int tag(const uint8_t *p, const char *s)
{
    for (unsigned i = 0; i < 4; ++i) if ((p[i] & 0x7f) != (uint8_t)s[i]) return 0;
    return 1;
}
/* CRC-16, polynomial 0x8005, initial zero, non-reflected. Header/blocks
 * include their stored big-endian remainder: a valid complete unit gives 0. */
static uint16_t crc16(const uint8_t *bytes, size_t size)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)bytes[i] << 8;
        for (unsigned b = 0; b < 8; ++b) crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0));
    }
    return crc;
}
int p4_golden_hca_info(const uint8_t *bytes, size_t size, P4GoldenHcaInfo *info)
{
    size_t pos;
    uint32_t loop_first = 0, loop_last = 0;
    uint16_t loop_delay = 0, loop_padding = 0;
    unsigned seen = 0;
    if (!info) return -1;
    memset(info, 0, sizeof *info);
    if (!bytes || size < 38 || !tag(bytes, "HCA\0") || !tag(bytes + 8, "fmt\0")) return -1;
    info->version = be16(bytes + 4);
    info->header_size = be16(bytes + 6);
    info->volume = 1;
    if (info->version != 0x101 && info->version != 0x102 && info->version != 0x103 &&
        info->version != 0x200 && info->version != 0x300) return -1;
    if (info->header_size < 38 || info->header_size > size || crc16(bytes, info->header_size)) return -1;
    info->channels = bytes[12];
    info->rate = (uint32_t)bytes[13] << 16 | (uint32_t)bytes[14] << 8 | bytes[15];
    info->blocks = be32(bytes + 16);
    info->delay = be16(bytes + 20);
    info->padding = be16(bytes + 22);
    if (!info->channels || info->channels > 8 || !info->rate || info->rate > 384000 ||
        !info->blocks || info->blocks > UINT32_MAX / 1024u) return -1;
    uint32_t total = info->blocks * 1024u;
    if ((uint32_t)info->delay + info->padding >= total) return -1;
    info->frames = total - info->delay - info->padding;
    if (tag(bytes + 24, "comp")) pos = 40;
    else if (tag(bytes + 24, "dec\0")) pos = 36;
    else return -1;
    if (pos + 2 > info->header_size) return -1;
    info->block_size = be16(bytes + 28);
    if (info->block_size < 8 || (uint64_t)info->blocks * info->block_size > size - info->header_size) return -1;
    while (pos + 4 <= (size_t)info->header_size - 2) {
        const uint8_t *p = bytes + pos;
        size_t width;
        unsigned bit;
        if (tag(p, "pad\0")) break;
        if (tag(p, "comm")) {
            if (pos + 5 > (size_t)info->header_size - 2) return -1;
            width = 5u + p[4]; bit = 1u;
        } else if (tag(p, "vbr\0")) { width = 8; bit = 2; }
        else if (tag(p, "ath\0")) { width = 6; bit = 4; }
        else if (tag(p, "loop")) { width = 16; bit = 8; }
        else if (tag(p, "ciph")) { width = 6; bit = 16; }
        else if (tag(p, "rva\0")) { width = 8; bit = 32; }
        else return -1;
        if ((seen & bit) || width > (size_t)info->header_size - 2 - pos) return -1;
        seen |= bit;
        if (bit == 2 && be16(p + 4)) return -1; /* variable packets aren't the fixed-block family */
        if (bit == 4 && be16(p + 4) > 1) return -1;
        if (bit == 8) {
            info->has_loop = 1;
            loop_first = be32(p + 4); loop_last = be32(p + 8);
            loop_delay = be16(p + 12); loop_padding = be16(p + 14);
        }
        if (bit == 16) info->cipher = be16(p + 4);
        if (bit == 32) {
            uint32_t bits = be32(p + 4);
            memcpy(&info->volume, &bits, sizeof bits);
            if (!isfinite(info->volume) || info->volume < 0) return -1;
        }
        pos += width;
    }
    if (info->cipher != 0 && info->cipher != 1 && info->cipher != 56) return -1;
    if (info->has_loop) {
        if (loop_first > loop_last || loop_last >= info->blocks || loop_delay > 1023 || loop_padding > 1023) return -1;
        uint32_t start = loop_first * 1024u + loop_delay;
        uint32_t end = (loop_last + 1) * 1024u - loop_padding;
        if (start < info->delay || end <= start || end > total - info->padding) return -1;
        info->loop_start = start - info->delay;
        info->loop_end = end - info->delay;
    }
    return 0;
}
