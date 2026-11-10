/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cf070 PS2AllPipelineNodeInit src/promoted/code1_003c.c - romwright draft, unreviewed */

void FUN_003cf070(int param_1)

{
  int iVar1;
  unsigned int *puVar2;
  unsigned char auVar3 [16];
  unsigned long long in_a0_udw;
  unsigned char auVar4 [16];
  unsigned long long in_t0_udw;
  
  iVar1 = *(int *)(param_1 + 0x14);
  *(unsigned int *)(iVar1 + 0x18) = 0;
  *(unsigned int *)(iVar1 + 0x1c) = 0x3d0;
  *(unsigned int *)(iVar1 + 0x20) = 0x20;
  *(unsigned int *)(iVar1 + 0x24) = (iVar1 + 0x37U) & 0xfffffff0;
  auVar3._8_8_ = 0;
  auVar3._0_8_ = in_a0_udw;
  *(unsigned int *)(*(int *)(iVar1 + 0x24) + 0x10) = 0;
  *(unsigned int *)(*(int *)(iVar1 + 0x24) + 0x14) = 0;
  *(unsigned int *)(*(int *)(iVar1 + 0x24) + 0x18) = 0;
  *(unsigned int *)(*(int *)(iVar1 + 0x24) + 0x1c) = 0;
  puVar2 = *(unsigned int **)(iVar1 + 0x24);
  auVar4._8_8_ = in_t0_udw;
  auVar4._0_8_ = (((unsigned long long)(*(int *)(iVar1 + 0x1c) + 4) | 0x6c010000) << 0x20) | 0x1000404;
  auVar4 = _pcpyld(auVar4,auVar3 << 0x40);
  *puVar2 = auVar4._0_4_;
  puVar2[1] = auVar4._4_4_;
  puVar2[2] = auVar4._8_4_;
  puVar2[3] = auVar4._12_4_;
  *(unsigned int *)(iVar1 + 0x10) = 0;
  *(unsigned int *)(iVar1 + 0x14) = 0;
  *(unsigned char *)(iVar1 + 1) = 1;
  *(unsigned char *)(iVar1 + 2) = 2;
  *(unsigned char *)(iVar1 + 3) = 1;
  *(unsigned char *)(iVar1 + 4) = 1;
  *(unsigned char *)(iVar1 + 5) = 3;
  *(unsigned char *)(iVar1 + 6) = 1;
  *(unsigned char *)(iVar1 + 8) = 0;
  *(unsigned char *)(iVar1 + 9) = 1;
  *(unsigned char *)(iVar1 + 10) = 0;
  *(unsigned char *)(iVar1 + 0xb) = 2;
  *(unsigned char *)(iVar1 + 0xc) = 2;
  *(unsigned char *)(iVar1 + 0xd) = 0;
  return;
}
