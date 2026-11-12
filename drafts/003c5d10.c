/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c5d10 func_003c5d10 src/promoted/code1_003c.c - romwright draft, unreviewed */

long FUN_003c5d10(unsigned long long param_1,char *param_2,unsigned int param_3)

{
  char cVar1;
  long lVar2;
  long lVar3;
  int iVar4;
  unsigned int uVar5;
  int *piVar6;
  int *piVar7;
  unsigned int uVar8;
  unsigned int uVar9;
  unsigned int *puVar10;
  unsigned int auStack_420 [258];
  int iStack_18;
  unsigned int uStack_14;
  int uStack_10;
  int iStack_c;
  int iStack_8;
  
  lVar2 = FUN_003df360(param_1,&uStack_10,0xc);
  if (lVar2 != 0) {
    iVar4 = iStack_c * 0x10 + 0x10;
    if (((*param_2 == '\b') && ((*(unsigned int *)(param_2 + 8) & 0x1000000) == 0)) ||
       ((*param_2 == '\a' && ((*(unsigned int *)(param_2 + 8) & 0x1000000) == 0)))) {
      iVar4 = iVar4 + iStack_8 * 2;
    }
    lVar2 = (*DAT_008873e8)(iVar4,0x30502);
    if (lVar2 != 0) {
      puVar10 = (unsigned int *)lVar2;
      piVar7 = puVar10 + 4;
      *puVar10 = uStack_10;
      *(short *)(puVar10 + 1) = (short)iStack_c;
      piVar6 = piVar7 + iStack_c * 3;
      *(unsigned short *)((int)puVar10 + 6) = *(unsigned short *)((int)&DAT_008872e0 + DAT_007647e0);
      puVar10[2] = iStack_8;
      puVar10[3] = 0;
      *(short *)((int)&DAT_008872e0 + DAT_007647e0) =
           *(short *)((int)&DAT_008872e0 + DAT_007647e0) + 1;
      uVar9 = (unsigned int)*(unsigned short *)(puVar10 + 1);
      if (*(unsigned short *)(puVar10 + 1) != 0) {
        do {
          uVar9 = uVar9 - 1;
          lVar3 = FUN_003df360(param_1,&iStack_18,8);
          if (lVar3 == 0) {
            return 0;
          }
          piVar7[1] = iStack_18;
          iVar4 = FUN_003c4a60(param_3,uStack_14);
          piVar7[2] = iVar4;
          cVar1 = *param_2;
          *piVar7 = (int)piVar6;
          if (((cVar1 == '\b') && ((*(unsigned int *)(param_2 + 8) & 0x1000000) == 0)) ||
             ((cVar1 == '\a' && ((*(unsigned int *)(param_2 + 8) & 0x1000000) == 0)))) {
            uVar8 = piVar7[1];
            while (uVar8 != 0) {
              puVar10 = auStack_420;
              uVar5 = uVar8;
              if (0xff < uVar8) {
                uVar5 = 0x100;
              }
              lVar3 = FUN_003df360(param_1,puVar10,uVar5 << 2);
              if (lVar3 == 0) {
                return 0;
              }
              uVar8 = uVar8 - uVar5;
              for (; uVar5 != 0; uVar5 = uVar5 - 1) {
                *(short *)piVar6 = (short)*puVar10;
                puVar10 = puVar10 + 1;
                piVar6 = (int *)((int)piVar6 + 2);
              }
            }
          }
          piVar7 = piVar7 + 3;
        } while (uVar9 != 0);
      }
    }
  }
  else {
    lVar2 = 0;
  }
  return lVar2;
}
