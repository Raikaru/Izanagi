/* The bottom of PS2 RAM (0x00000000..0x000000FF) as a retail run leaves it, for the places where retail code reads through a null pointer.
 *
 * Retail code that follows a null frame / parent pointer (RwFrameGetLTM(NULL), `parent->ltm` with a null parent) does not fault: the EE has RAM at address 0,
 * so `lw $2, 0xA0($0)` or `addiu $a2, $0, 0x50` followed by reads at 0x50.. just see the kernel's low memory. The host cannot map page 0 (Windows: impossible; Linux: root),
 * so the functions that retail runs on such a pointer read this image instead (p4_ps2_low_ram_*).
 *
 * Source: eeMemory.bin of the PCSX2 savestates of PortTrace3's corpus (8 states: title, field, memory card end).
 * Words 0x00..0x27 are the EE exception vector stub (lui $26,0x8001 ...), identical in all 8 states except 0x04, 0x18 and 0x24 (kernel bookkeeping, nothing reads them);
 * 0x28..0xFF are zero except 0x80 (0x08004469 in field.p2s, 0x08005057 in other states: a float of about 3e-34 when read as the translation of a matrix at 0x50).
 * The image below is field.p2s's. Every word the null-frame paths read (byte 0x03, 0x50..0x8F as a matrix, 0xA0) is 0 or the stable 0x3c1a8001 word. */
#ifndef P4PC_PS2_LOW_RAM_H
#define P4PC_PS2_LOW_RAM_H

static const unsigned int p4_ps2_low_ram_words[64] __attribute__((aligned(256))) = {
    0x3c1a8001, 0xff5923b8, 0x40196800, 0x3c1a8001,   /* 0x00 */
    0x3339007c, 0x0359d021, 0x8f5a2380, 0x3c198001,   /* 0x10 */
    0x03400008, 0xdf3923b8, 0x00000000, 0x00000000,   /* 0x20 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x30 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x40 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x50 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x60 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x70 */
    0x08004469, 0x00000000, 0x00000000, 0x00000000,   /* 0x80 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0x90 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xa0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xb0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xc0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xd0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xe0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000,   /* 0xf0 */
};

/* Byte / word / pointer into the image for a PS2 address below 0x100. */
static inline unsigned int p4_ps2_low_ram_u32(unsigned int addr)
{
    return p4_ps2_low_ram_words[(addr & 0xFF) >> 2];
}

static inline unsigned int p4_ps2_low_ram_u8(unsigned int addr)
{
    return (p4_ps2_low_ram_words[(addr & 0xFF) >> 2] >> ((addr & 3) * 8)) & 0xFF;
}

static inline unsigned char *p4_ps2_low_ram_ptr(unsigned int addr)
{
    return (unsigned char *)p4_ps2_low_ram_words + (addr & 0xFF);
}

#endif
