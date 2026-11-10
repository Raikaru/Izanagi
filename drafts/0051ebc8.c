/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051ebc8 func_0051ebc8 src/cri/sfd_mpv.c - romwright draft, unreviewed */

unsigned long long FUN_0051ebc8(unsigned long long param_1)

{
  unsigned int uVar1;
  unsigned int uVar2;
  int *piVar3;
  int iVar4;
  long lVar5;
  unsigned long long uVar6;
  unsigned long long in_t1;
  unsigned long long in_t2;
  unsigned long long in_t3;
  
  piVar3 = *(int **)((int)param_1 + 0x1fc0);
  iVar4 = *piVar3;
  if (iVar4 != 0) {
    uVar1 = ((int)piVar3 + 0xbU) & 7;
    uVar2 = (unsigned int)(piVar3 + 1) & 7;
    DAT_009388b0 = (((*(long *)(((int)piVar3 + 0xbU) - uVar1) << (7 - uVar1) * 8) |
                    (in_t1 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8))
                   | (*(unsigned long long *)((int)(piVar3 + 1) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x13U) & 7;
    uVar2 = (unsigned int)(piVar3 + 3) & 7;
    DAT_009388b8 = (((*(long *)(((int)piVar3 + 0x13U) - uVar1) << (7 - uVar1) * 8) |
                    (in_t2 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8))
                   | (*(unsigned long long *)((int)(piVar3 + 3) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x1bU) & 7;
    uVar2 = (unsigned int)(piVar3 + 5) & 7;
    DAT_009388c0 = (((*(long *)(((int)piVar3 + 0x1bU) - uVar1) << (7 - uVar1) * 8) |
                    (in_t3 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8))
                   | (*(unsigned long long *)((int)(piVar3 + 5) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x23U) & 7;
    uVar2 = (unsigned int)(piVar3 + 7) & 7;
    DAT_009388c8 = (((*(long *)(((int)piVar3 + 0x23U) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388b0 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) | (*(unsigned long long *)((int)(piVar3 + 7) - uVar2) >> uVar2 * 8);
    DAT_009388d0 = piVar3[9];
    uVar1 = ((int)piVar3 + 0x2fU) & 7;
    uVar2 = (unsigned int)(piVar3 + 10) & 7;
    DAT_009388d8 = (((*(long *)(((int)piVar3 + 0x2fU) - uVar1) << (7 - uVar1) * 8) |
                    ((0xffffffffffffffffU >> (uVar1 + 1) * 8) & 0x940000)) &
                   (-1L << (8 - uVar2) * 8)) | (*(unsigned long long *)((int)(piVar3 + 10) - uVar2) >> uVar2 * 8)
    ;
    uVar1 = ((int)piVar3 + 0x37U) & 7;
    uVar2 = (unsigned int)(piVar3 + 0xc) & 7;
    DAT_009388e0 = (((*(long *)(((int)piVar3 + 0x37U) - uVar1) << (7 - uVar1) * 8) |
                    ((0xffffffffffffffffU >> (uVar1 + 1) * 8) & 0x9388b0)) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0xc) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x3fU) & 7;
    uVar2 = (unsigned int)(piVar3 + 0xe) & 7;
    DAT_009388e8 = (((*(long *)(((int)piVar3 + 0x3fU) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388d8 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0xe) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x47U) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x10) & 7;
    DAT_009388f0 = (((*(long *)(((int)piVar3 + 0x47U) - uVar1) << (7 - uVar1) * 8) |
                    ((0xffffffffffffffffU >> (uVar1 + 1) * 8) & 0x940000)) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x10) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x4fU) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x12) & 7;
    DAT_009388f8 = (((*(long *)(((int)piVar3 + 0x4fU) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388e0 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x12) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x57U) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x14) & 7;
    DAT_00938900 = (((*(long *)(((int)piVar3 + 0x57U) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388e8 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x14) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x5fU) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x16) & 7;
    DAT_00938908 = (((*(long *)(((int)piVar3 + 0x5fU) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388f0 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x16) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x67U) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x18) & 7;
    DAT_00938910 = (((*(long *)(((int)piVar3 + 0x67U) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_009388f8 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x18) - uVar2) >> uVar2 * 8);
    uVar1 = ((int)piVar3 + 0x6fU) & 7;
    uVar2 = (unsigned int)(piVar3 + 0x1a) & 7;
    DAT_00938918 = (((*(long *)(((int)piVar3 + 0x6fU) - uVar1) << (7 - uVar1) * 8) |
                    (DAT_00938900 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                   (-1L << (8 - uVar2) * 8)) |
                   (*(unsigned long long *)((int)(piVar3 + 0x1a) - uVar2) >> uVar2 * 8);
    DAT_00938920 = piVar3[0x420];
    DAT_00938924 = piVar3[0x421];
    DAT_00938928 = piVar3[0x422];
    FUN_00520cf0();
    lVar5 = FUN_0051ed58(iVar4);
    if (lVar5 != 0) {
      uVar6 = FUN_00517ad0(param_1,0xffffffffff000f0c);
      return uVar6;
    }
    *piVar3 = 0;
  }
  return 0;
}
