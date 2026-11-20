/* Port-defined null-frame storage for guest addresses 0x00..0xFF.
 *
 * This is not a PS2 memory image or executable exception vector. The native
 * runtime never executes low RAM. Its null-frame adapters need only:
 *   0xA0: a null root pointer;
 *   0x03: clear hierarchy/object sync bits (masks 1 and 3);
 *   0x50..0x8F: a zero RwMatrix, including flags and translation.
 * Zero-initialization constructs those semantics; unread storage is also zero.
 * In particular, translation.x at 0x80 is zero by definition, not an instruction
 * reinterpreted as a float. No kernel bytes or emulator state are inputs.
 * See tests/ps2_low_ram_test.c for the production read paths. */
#ifndef P4PC_PS2_LOW_RAM_H
#define P4PC_PS2_LOW_RAM_H

static const unsigned int p4_ps2_low_ram_words[64] __attribute__((aligned(256))) = {0};

/* Byte / word / pointer into the null-frame storage below guest address 0x100. */
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
