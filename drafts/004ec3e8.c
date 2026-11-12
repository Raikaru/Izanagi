/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ec3e8 AXRNA_SetSfreq src/cri/ps2_rna.c - romwright draft, unreviewed */

char * FUN_004ec3e8(unsigned int *param_1,int param_2)

{
  char cVar1;
  unsigned int uVar2;
  char *pcVar3;
  int iVar4;
  long lVar5;
  char *pcVar6;
  char *pcVar7;
  unsigned int *puVar8;
  int *piVar9;
  char *pcVar10;
  int iVar11;
  int *piVar12;
  int aiStack_60 [4];
  unsigned char auStack_50 [16];
  
  iVar11 = 0;
  pcVar7 = &DAT_0073cba0;
  pcVar3 = pcVar7;
  cVar1 = DAT_0073cba0;
  while (cVar1 != '\0') {
    iVar11 = iVar11 + 1;
    pcVar7 = pcVar7 + 100;
    if (0xf < iVar11) break;
    pcVar3 = pcVar3 + 100;
    cVar1 = *pcVar7;
  }
  pcVar7 = (char *)0x0;
  if (iVar11 != 0x10) {
    pcVar7 = pcVar3;
    if ((DAT_0073a748 == 1) && (DAT_0073a740 < 1)) {
      *pcVar3 = '\x01';
    }
    else {
      iVar11 = 0;
      FUN_0043f9c8(pcVar3,0,100);
      *(int *)(pcVar3 + 4) = param_2;
      if (0 < param_2) {
        piVar12 = (int *)(pcVar3 + 8);
        piVar9 = piVar12;
        do {
          lVar5 = FUN_004eb7b0();
          *piVar9 = (int)lVar5;
          piVar9 = piVar9 + 1;
          if (lVar5 == 0) {
            FUN_004cd3f0(0x75c998);
            if (param_2 < 1) {
              return (char *)0x0;
            }
            iVar11 = *piVar12;
            while( 1 ) {
              piVar12 = piVar12 + 1;
              if (iVar11 != 0) {
                FUN_004eb828(iVar11);
              }
              param_2 = param_2 - 1;
              if (param_2 == 0) break;
              iVar11 = *piVar12;
            }
            return (char *)0x0;
          }
          iVar11 = iVar11 + 1;
        } while (iVar11 < param_2);
      }
      iVar11 = 0;
      aiStack_60[1] = 0;
      if (0 < param_2) {
        piVar12 = aiStack_60 + 2;
        piVar9 = (int *)(pcVar3 + 8);
        do {
          iVar4 = *piVar9;
          piVar9 = piVar9 + 1;
          iVar11 = iVar11 + 1;
          *piVar12 = *(int *)(iVar4 + 4);
          piVar12 = piVar12 + 1;
        } while (iVar11 < param_2);
      }
      aiStack_60[0] = param_2;
      lVar5 = FUN_004e58a0(8,aiStack_60,4,auStack_50,1);
      *(int *)(pcVar3 + 0x20) = (int)lVar5;
      if (lVar5 == 0) {
        FUN_004cd3f0(0x75c9c0);
        pcVar7 = (char *)0x0;
      }
      else {
        iVar11 = 0;
        if (0 < param_2) {
          piVar9 = (int *)(pcVar3 + 8);
          pcVar10 = pcVar3 + 0x10;
          iVar4 = 0;
          do {
            iVar11 = iVar11 + 1;
            lVar5 = FUN_004e4f10(*(unsigned int *)(iVar4 + (int)param_1),
                                 *(unsigned int *)(*(int *)((int)piVar9 + iVar4) + 8));
            *(int *)pcVar10 = (int)lVar5;
            pcVar10 = pcVar10 + 4;
            if (lVar5 == 0) {
              iVar11 = 0;
              FUN_004cd3f0(0x75c9e8);
              if (param_2 < 1) {
                return (char *)0x0;
              }
              piVar12 = (int *)(pcVar3 + 0x10);
              iVar4 = *piVar12;
              while( 1 ) {
                piVar12 = piVar12 + 1;
                if (iVar4 != 0) {
                  FUN_004e5000(iVar4);
                }
                if (*(int *)(pcVar3 + 0x20) != 0) {
                  aiStack_60[0] = *(int *)(pcVar3 + 0x20);
                  FUN_004e58a0(9,aiStack_60,1,0,0);
                }
                iVar4 = *piVar9;
                piVar9 = piVar9 + 1;
                if (iVar4 != 0) {
                  FUN_004eb828();
                }
                iVar11 = iVar11 + 1;
                if (param_2 <= iVar11) break;
                iVar4 = *piVar12;
              }
              return (char *)0x0;
            }
            iVar4 = iVar11 * 4;
          } while (iVar11 < param_2);
        }
        iVar11 = 0;
        if (0 < param_2) {
          pcVar10 = pcVar3 + 0x18;
          puVar8 = param_1;
          do {
            uVar2 = *puVar8;
            puVar8 = puVar8 + 1;
            iVar11 = iVar11 + 1;
            *(unsigned int *)pcVar10 = uVar2;
            pcVar10 = pcVar10 + 4;
          } while (iVar11 < param_2);
        }
        pcVar3[0x50] = '\0';
        pcVar3[0x51] = '\0';
        pcVar3[0x52] = (char)param_2;
        pcVar3[0x53] = (char)param_2;
        pcVar3[0x54] = '\0';
        pcVar3[0x55] = '\0';
        pcVar3[0x38] = '\0';
        cVar1 = (char)DAT_0073a788;
        pcVar3[0x39] = '\0';
        pcVar3[0x3a] = '\0';
        pcVar3[0x3b] = '\0';
        pcVar3[0x3c] = '\0';
        pcVar3[0x3d] = '\0';
        pcVar3[0x3e] = '\0';
        pcVar3[0x3f] = '\0';
        pcVar3[0x56] = cVar1;
        cVar1 = (char)DAT_0073a788;
        pcVar3[0x5b] = '\0';
        pcVar3[0x57] = cVar1;
        pcVar3[0x5a] = '\0';
        if (0 < param_2) {
          iVar11 = 0x40;
          do {
            pcVar10 = pcVar3 + iVar11;
            pcVar6 = pcVar3 + iVar11 + 8;
            param_2 = param_2 - 1;
            pcVar10[0] = '\0';
            pcVar10[1] = '\0';
            pcVar10[2] = '\0';
            pcVar10[3] = '\0';
            iVar11 = iVar11 + 4;
            pcVar6[0] = '\0';
            pcVar6[1] = '\0';
            pcVar6[2] = '\0';
            pcVar6[3] = '\0';
          } while (param_2 != 0);
        }
        pcVar3[0x58] = '\0';
        pcVar3[0x59] = '\x01';
        pcVar3[0x5c] = '\0';
        pcVar3[0x5d] = '\0';
        pcVar3[0x5e] = '\0';
        pcVar3[0x5f] = '\0';
        piVar9 = (int *)*param_1;
        pcVar3[0x60] = '\0';
        pcVar3[0x61] = '\0';
        pcVar3[0x62] = '\0';
        pcVar3[99] = '\0';
        iVar11 = *piVar9;
        pcVar3[0x24] = '\0';
        pcVar3[0x25] = '\0';
        pcVar3[0x26] = '\0';
        pcVar3[0x27] = '\0';
        iVar11 = (**(code **)(iVar11 + 0x24))(piVar9,1);
        iVar4 = (**(code **)(*(int *)*param_1 + 0x24))((int *)*param_1,0);
        *(int *)(pcVar3 + 0x28) = iVar11 + iVar4;
        pcVar3[0x2c] = '\0';
        pcVar3[0x2d] = '@';
        pcVar3[0x2e] = '\0';
        pcVar3[0x2f] = '\0';
        *pcVar3 = '\x01';
      }
    }
  }
  return pcVar7;
}
