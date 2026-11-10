/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Host model of the CRI_ADXI.IRX RPC server (src/cri/adxi_rpc.c), for tests and the sound modules. */
#ifndef P4PC_ADXI_RPC_H
#define P4PC_ADXI_RPC_H

#include <stdint.h>

#include "../platform/sound/p4snd.h"

/* The sif client the EE side (DTX_Init, func_004e58a0) binds to the module's server. */
#define P4ADXI_CLIENT 0x00732D30u

/* Command ids (sceSifCallRpc fno - 0x400), as the IOP registers them. */
enum {
    P4ADXI_SJX_CREATE = 0, /* registrar 0x9a08, handlers 0x98ac..0x9988 */
    P4ADXI_SJX_DESTROY = 1,
    P4ADXI_SJX_SETID = 2,
    P4ADXI_DTX_OPEN = 3,
    P4ADXI_RNA_CREATE = 8, /* IOPRNA, registrar 0x16a8, handlers 0x1258..0x1604 */
    P4ADXI_RNA_DESTROY = 9,
    P4ADXI_RNA_INIT = 10,
    P4ADXI_RNA_SETCOREMODE = 11,
    P4ADXI_RNA_SETSFREQ = 12,
    P4ADXI_RNA_FINISH = 13,
    P4ADXI_RNA_SYNCSTART = 14,
    P4ADXI_RNA_SYNCSTOP = 15,
    P4ADXI_RNA_STARTCORE = 16,
    P4ADXI_RNA_STOPCORE = 17,
    P4ADXI_RNA_PAUSE = 18,
    P4ADXI_SJ_CREATE_RBF = 32, /* remote stream joints, registrar 0x9568, handlers 0x9240..0x9518 */
    P4ADXI_SJ_CREATE_MEM = 33,
    P4ADXI_SJ_CREATE_UNI = 34,
    P4ADXI_SJ_DESTROY = 35,
    P4ADXI_SJ_GETUUID = 36,
    P4ADXI_SJ_RESET = 37,
    P4ADXI_SJ_GETCHUNK = 38,
    P4ADXI_SJ_UNGETCHUNK = 39,
    P4ADXI_SJ_PUTCHUNK = 40,
    P4ADXI_SJ_GETNUMDATA = 41,
    P4ADXI_SJ_ISGETCHUNK = 42,
};

/* Drops all server state (destroys the voices): the module being loaded fresh. */
void p4adxi_reset(void);

/* One pass of what the server's output threads do: for every PSM whose core is running, move
 * the PCM from its stream joints into its mixer voice. Also runs from the mixer's tick hook. */
void p4adxi_pump(void);

/* Test access: whether SPU2 core c (0/1) is running, the mixer voice of an IOPRNA handle (NULL
 * when it has none), and the SPU2 core a handle's PSM outputs to (-1 for a bad handle). */
int32_t p4adxi_core_running(int32_t core);
P4SndVoice *p4adxi_rna_voice(uint32_t handle);
int32_t p4adxi_rna_core(uint32_t handle);

#endif
