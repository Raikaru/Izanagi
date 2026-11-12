/* The IOP's RAM and the SPU2's RAM as the sound driver sees them.
 *
 * The game moves sound data in two hops: EE memory to IOP memory over SIF DMA (func_00424708,
 * SifSetDma), then IOP memory to SPU2 memory with the sdr voice-transfer command. The port has
 * no IOP, so both memories are plain arrays here and the two hops are copies. Sample banks (.bd)
 * end up in SPU memory, sound headers (.hd) and sequences (.sq) stay in IOP memory, which is
 * where the sound modules read them. */
#ifndef P4PC_SDR_MEM_H
#define P4PC_SDR_MEM_H

#include <stdint.h>

#define P4SDR_IOP_RAM_SIZE 0x200000u
#define P4SDR_SPU_RAM_SIZE 0x200000u

/* Pointers into the two RAMs, or NULL when [addr, addr + n) is not inside. IOP RAM is port-owned guest
 * memory (stream-joint chunks on the EE side point into it, and a chunk holds a guest address):
 * p4sdr_iop_gaddr is the guest address of IOP address addr, 0 when addr is outside IOP RAM. */
uint8_t *p4sdr_iop_ptr(uint32_t addr, uint32_t n);
uint32_t p4sdr_iop_gaddr(uint32_t addr);
uint8_t *p4sdr_spu_ptr(uint32_t addr, uint32_t n);

/* Copies n bytes from IOP RAM to SPU RAM at PC speed.
 * Returns 0 when copied, or -1 for an invalid range. */
int p4sdr_iop_to_spu(uint32_t iop_addr, uint32_t spu_addr, uint32_t n);

#endif
