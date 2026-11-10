/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00513d38 func_00513d38 src/cri/sfd_aoap.c - romwright draft, unreviewed */

unsigned long long FUN_00513d38(int param_1)

{
  unsigned int *puVar1;
  unsigned int uVar2;
  long lVar3;
  int *piVar4;
  
  lVar3 = FUN_00513dc8();
  if (lVar3 != 0) {
    puVar1 = *(unsigned int **)(param_1 + 0x2004);
    uVar2 = *puVar1;
    if ((puVar1[0xd] == 0) && (piVar4 = (int *)lVar3, *piVar4 != 0)) {
      FUN_004ca990(uVar2,piVar4[3],piVar4[4],piVar4[5]);
      FUN_004d6510(uVar2,0);
      puVar1[0xd] = 1;
      puVar1[0xf] = FUN_00512d90;
    }
  }
  return 0;
}
