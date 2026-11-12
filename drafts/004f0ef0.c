/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f0ef0 SVM_ExecSvrFunc src/cri/svm.c - romwright draft, unreviewed */

unsigned long long FUN_004f0ef0(int param_1)

{
  unsigned int uVar1;
  code *pcVar2;
  unsigned long long uVar3;
  int *piVar4;
  unsigned int *puVar5;
  int iVar6;
  unsigned long long uVar7;
  
  iVar6 = 5;
  uVar7 = 0;
  puVar5 = (unsigned int *)(&LAB_00925be0 + param_1 * 0x48);
  pcVar2 = (code *)*puVar5;
  while( 1 ) {
    uVar1 = puVar5[1];
    puVar5 = puVar5 + 3;
    if (pcVar2 != (code *)0x0) {
      *(unsigned int *)(&LAB_007439a8 + param_1 * 4) = 1;
      uVar3 = (*pcVar2)(uVar1);
      *(unsigned int *)(&LAB_007439a8 + param_1 * 4) = 0;
      uVar7 = uVar7 | uVar3;
    }
    iVar6 = iVar6 - 1;
    if (iVar6 < 0) break;
    pcVar2 = (code *)*puVar5;
  }
  piVar4 = (int *)(param_1 * 4 + 0x743988);
  *piVar4 = *piVar4 + 1;
  return uVar7;
}
