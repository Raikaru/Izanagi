/* IOP RAM, SPU2 RAM and SifSetDma. See sdr_mem.h. */
#include "sdr_mem.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/gmem.h"

/* IOP RAM is guest memory (a chunk on the EE side holds a guest address into it); SPU2 RAM is read
 * by the mixer only and stays a host array. */
static uint32_t iop_gbase;
static uint8_t spu_ram[P4SDR_SPU_RAM_SIZE];

static uint8_t *iop_ram(void)
{
    if (iop_gbase == 0) {
        iop_gbase = p4_gport_alloc(P4SDR_IOP_RAM_SIZE, 4096);
    }
    return (uint8_t *)P4_G2H(iop_gbase);
}

uint8_t *p4sdr_iop_ptr(uint32_t addr, uint32_t n)
{
    if (addr > P4SDR_IOP_RAM_SIZE || n > P4SDR_IOP_RAM_SIZE - addr) {
        return NULL;
    }
    return iop_ram() + addr;
}

uint32_t p4sdr_iop_gaddr(uint32_t addr)
{
    if (addr > P4SDR_IOP_RAM_SIZE) {
        return 0;
    }
    (void)iop_ram();
    return iop_gbase + addr;
}

uint8_t *p4sdr_spu_ptr(uint32_t addr, uint32_t n)
{
    if (addr > P4SDR_SPU_RAM_SIZE || n > P4SDR_SPU_RAM_SIZE - addr) {
        return NULL;
    }
    return spu_ram + addr;
}


int p4sdr_iop_to_spu(uint32_t iop_addr, uint32_t spu_addr, uint32_t n)
{
    const uint8_t *src = p4sdr_iop_ptr(iop_addr, n);
    uint8_t *dst = p4sdr_spu_ptr(spu_addr, n);

    if (src == NULL || dst == NULL) {
        return -1;
    }
    memcpy(dst, src, n);
    return 0;
}

/* One SIF DMA descriptor (libkernl sceSifDmaData / SifDmaTransfer_t): EE source, IOP
 * destination, byte count, attributes. */
typedef struct {
    uint32_t src;
    uint32_t dest;
    int32_t size;
    int32_t attr;
} SifDmaData;

/* SifSetDma (0x00424708): queues EE to IOP transfers. The copy happens in the call, so the
 * returned id is already complete (func_00421b80 reports that). Callers in the sound path
 * (sdkSndcom) check only for a nonzero id. A descriptor with a destination outside IOP RAM
 * fails the whole call, as the DMA hardware would refuse it, and returns 0. */
int32_t func_00424708(const SifDmaData *dmat, int32_t count)
{
    static int32_t next_id = 1;
    int32_t i;

    if (dmat == NULL || count <= 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        uint8_t *dst;

        if (dmat[i].size < 0) {
            return 0;
        }
        dst = p4sdr_iop_ptr(dmat[i].dest & 0x1fffffu, (uint32_t)dmat[i].size);
        if (dst == NULL) {
            fprintf(stderr, "p4snd: SifSetDma to IOP 0x%08x size %d is outside IOP RAM\n",
                    (unsigned)dmat[i].dest, (int)dmat[i].size);
            return 0;
        }
        memcpy(dst, (const void *)P4_G2H(dmat[i].src), (size_t)dmat[i].size);
    }
    return next_id++;
}
