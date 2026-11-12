/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0040df50 func_0040df50 src/rw/texcache.c - romwright draft, unreviewed */

/* WARNING: Heritage AFTER dead removal. Example location: r0x008873e8 : 0x0040e21c */
/* WARNING: Restarted to delay deadcode elimination for space: ram */

void FUN_0040df50(int param_1,unsigned int param_2,unsigned long long param_3)

{
  int iVar1;
  unsigned int uVar2;
  unsigned char auVar3 [16];
  unsigned char auVar4 [16];
  unsigned char auVar5 [12];
  unsigned long long uVar6;
  unsigned char auVar7 [16];
  int iVar8;
  unsigned int uVar9;
  int iVar10;
  int iVar11;
  unsigned int uVar12;
  unsigned int *puVar13;
  unsigned long long uVar14;
  int iVar15;
  unsigned char auVar16 [16];
  unsigned long long in_a0_udw;
  unsigned char auVar17 [16];
  unsigned char auVar18 [16];
  unsigned char auVar19 [16];
  unsigned char auVar20 [16];
  unsigned char auVar21 [16];
  int iVar22;
  unsigned int uVar23;
  unsigned int in_a1_udw;
  unsigned int in_a1_udw_00;
  unsigned int uVar24;
  unsigned int in_a2_udw;
  unsigned int in_a2_udw_00;
  unsigned int uVar25;
  int *piVar26;
  unsigned int *puVar27;
  int *piVar28;
  unsigned int uVar29;
  unsigned int uVar30;
  int iVar31;
  unsigned int uVar32;
  int *piStack_b0;
  unsigned int auStack_40 [4];
  unsigned int uStack_30;
  unsigned int uStack_2c;
  unsigned int uStack_28;
  int iStack_20;
  unsigned int uStack_1c;
  unsigned int uStack_18;
  unsigned int uStack_14;
  unsigned int uStack_10;
  unsigned int uStack_c;
  unsigned int uStack_8;
  
  DAT_00764a98 = (unsigned int)param_3;
  piVar26 = (int *)(param_1 + DAT_00764a28);
  if ((*(unsigned char *)((int)piVar26 + 0x36) & 1) == 0) {
    uVar25 = *(unsigned int *)(param_1 + 4);
    DAT_00764a98 = 0;
    iVar22 = *(int *)(param_1 + 0x14);
    iVar1 = *(int *)(param_1 + 0xc);
    uVar12 = ((unsigned int)piVar26[2] >> 0x14) & 0x3f;
    iVar15 = *(int *)(param_1 + 0x18);
    switch((int)(unsigned int)*(unsigned char *)((int)piVar26 + 0x16) >> 2) {
    default:
      uVar6 = *(unsigned long long *)(piVar26 + 8);
      uStack_10 = (unsigned int)uVar6 & 0x3fff;
      uStack_c = (unsigned int)(uVar6 >> 0x14) & 0x3fff;
      uVar32 = (unsigned int)(uVar6 >> 0x20);
      uStack_8 = (uVar32 & 0x3fff00) >> 8;
      uStack_30 = (unsigned int)(uVar6 >> 0xe) & 0x3f;
      uStack_2c = (uVar32 & 0xfc) >> 2;
      uStack_28 = (uVar32 & 0xfc00000) >> 0x16;
    case 1:
    case 2:
    case 3:
      uVar6 = *(unsigned long long *)(piVar26 + 6);
      uStack_1c = (unsigned int)uVar6 & 0x3fff;
      uStack_18 = (unsigned int)(uVar6 >> 0x14) & 0x3fff;
      uVar32 = (unsigned int)(uVar6 >> 0x20);
      uStack_14 = (uVar32 & 0x3fff00) >> 8;
      auStack_40[1] = (unsigned int)(uVar6 >> 0xe) & 0x3f;
      auStack_40[2] = (uVar32 & 0xfc) >> 2;
      auStack_40[3] = (uVar32 & 0xfc00000) >> 0x16;
    case 0:
      uVar32 = 0;
      iStack_20 = 0;
      auStack_40[0] = ((unsigned int)piVar26[2] >> 0xe) & 0x3f;
      piStack_b0 = &iStack_20;
      puVar27 = auStack_40;
      piVar28 = piVar26;
      do {
        uVar30 = *(int *)(param_1 + 0xc) >> (uVar32 & 0x1f);
        uVar29 = *(int *)(param_1 + 0x10) >> (uVar32 & 0x1f);
        iVar8 = *(int *)(param_1 + 0x18) >> (uVar32 & 0x1f);
        iVar31 = (param_2 >> 6) + *piStack_b0;
        if ((uVar25 & 0xf) == 0) {
          if (iVar15 == iVar1 * iVar22 >> 3) {
            if (piVar28[0xf] == 0) {
              iVar11 = piVar26[0xd];
              uVar9 = iVar22 * uVar30 * uVar29 >> 3;
              if ((char)iVar11 != '\0') {
                iVar10 = FUN_0044cdd0(((long)(int)uVar9 + 0xfU) >> 4,0x7fee);
                uVar14 = (*DAT_008873e8)((iVar10 + 1) * 0x30 + 0xb0,0x30411);
              }
              else {
                FUN_003f39c0(10);
                iVar10 = FUN_0044cdd0(((long)(int)uVar9 + 0xfU) >> 4,0x7fee);
                uVar14 = FUN_003f44c0((iVar10 + 1) * 0x30 + 0xb0,0);
              }
              iVar11 = FUN_0040d570(uVar14,uVar25,uVar9,(char)iVar11);
              piVar28[0xf] = iVar11;
            }
            if (((*(unsigned char *)((int)piVar26 + 0x36) & 2) == 0) ||
               ((((unsigned int)piVar26[2] >> 0x14) & 0x3f) != 0x13)) {
              if (((*(unsigned char *)((int)piVar26 + 0x36) & 4) != 0) &&
                 ((((unsigned int)piVar26[2] >> 0x14) & 0x3f) == 0x14)) {
                FUN_0040da90(uVar30 >> 1,uVar29 >> 1,iVar31,*puVar27 >> 1,2,piVar28[0xf],
                             (char)piVar26[0xd]);
              }
              else {
                FUN_0040da90(uVar30,uVar29,iVar31,*puVar27,uVar12,piVar28[0xf],(char)piVar26[0xd]);
              }
            }
            else {
              FUN_0040da90(uVar30 >> 1,uVar29 >> 1,iVar31,*puVar27 >> 1,0,piVar28[0xf],
                           (char)piVar26[0xd]);
            }
            if ((char)piVar26[0xd] == '\0') {
              piVar28[0xf] = 0;
            }
          }
          else if (((*(unsigned char *)((int)piVar26 + 0x36) & 2) == 0) ||
                  ((((unsigned int)piVar26[2] >> 0x14) & 0x3f) != 0x13)) {
            if (((*(unsigned char *)((int)piVar26 + 0x36) & 4) != 0) &&
               ((((unsigned int)piVar26[2] >> 0x14) & 0x3f) == 0x14)) {
              FUN_0040dcc0(uVar30 >> 1,uVar29 >> 1,uVar25,iVar31,*puVar27 >> 1,0x10,2,iVar8);
            }
            else {
              FUN_0040dcc0(uVar30,uVar29,uVar25,iVar31,*puVar27,iVar22,uVar12,iVar8);
            }
          }
          else {
            FUN_0040dcc0(uVar30 >> 1,uVar29 >> 1,uVar25,iVar31,*puVar27 >> 1,0x20,0,iVar8);
          }
        }
        uVar25 = uVar25 + iVar8 * uVar29;
        if ((uVar25 & 0xf) != 0) {
          uVar25 = (uVar25 + 0xf) & 0xfffffff0;
        }
        uVar32 = uVar32 + 1;
        piVar28 = piVar28 + 1;
        piStack_b0 = piStack_b0 + 1;
        puVar27 = puVar27 + 1;
      } while ((int)uVar32 <= (int)(unsigned int)*(unsigned char *)((int)piVar26 + 0x16) >> 2);
      if ((piVar26[0xb] != 0) && (*(int *)(param_1 + 0x14) < 9)) {
        iVar15 = *(int *)(param_1 + 0x14) * 4;
        iVar22 = piVar26[4];
        iVar1 = *(int *)(iVar15 + 0x75338c);
        iVar15 = *(int *)(iVar15 + 0x7533ac);
        uVar25 = ((unsigned int)piVar26[3] >> 0x13) & 0xf;
        if ((uVar25 != 10) && (uVar25 != 2)) {
          if (uVar25 == 0) {
            iVar8 = 0x20;
          }
          else {
            iVar8 = 0x10;
          }
        }
        else {
          iVar8 = 0x10;
        }
        if (piVar26[0xe] == 0) {
          uVar2 = *(unsigned int *)(param_1 + 8);
          iVar31 = piVar26[0xd];
          uVar12 = (unsigned int)(iVar8 * iVar1 * iVar15) >> 3;
          if ((char)iVar31 != '\0') {
            iVar8 = FUN_0044cdd0(((long)(int)uVar12 + 0xfU) >> 4,0x7fee);
            uVar14 = (*DAT_008873e8)((iVar8 + 1) * 0x30 + 0xb0,0x30411);
          }
          else {
            FUN_003f39c0(10);
            iVar8 = FUN_0044cdd0(((long)(int)uVar12 + 0xfU) >> 4,0x7fee);
            uVar14 = FUN_003f44c0((iVar8 + 1) * 0x30 + 0xb0,0);
          }
          iVar8 = FUN_0040d570(uVar14,uVar2,uVar12,(char)iVar31);
          piVar26[0xe] = iVar8;
        }
        FUN_0040da90(iVar1,iVar15,(param_2 >> 6) + iVar22,1,uVar25,piVar26[0xe],(char)piVar26[0xd]);
        if ((char)piVar26[0xd] == '\0') {
          piVar26[0xe] = 0;
        }
      }
    }
  }
  else {
    uVar25 = **(unsigned int **)(param_1 + 0x24);
    puVar27 = *(unsigned int **)(param_1 + 0x24) + 4;
    iVar22 = uVar25 * 4 + 4;
    if ((*(unsigned char *)(param_1 + 0x21) & 0x40) != 0) {
      iVar22 = iVar22 + ((unsigned int)piVar26[0xb] >> 4);
    }
    FUN_003f4730(param_3,iVar22);
    auVar18._8_8_ = in_a0_udw;
    auVar18._0_8_ = 0xffffffffffffffc0;
    uVar12 = 0;
    if (uVar25 != 0) {
      do {
        uVar32 = puVar27[1];
        uVar29 = puVar27[2];
        uVar30 = puVar27[3];
        uVar12 = uVar12 + 1;
        *DAT_00764970 = *puVar27;
        DAT_00764970[1] = uVar32;
        DAT_00764970[2] = uVar29;
        DAT_00764970[3] = uVar30;
        auVar18 = *(unsigned char (*) [16])(puVar27 + 4);
        DAT_00764970[4] = auVar18._0_4_;
        DAT_00764970[5] = auVar18._4_4_;
        DAT_00764970[6] = auVar18._8_4_;
        DAT_00764970[7] = auVar18._12_4_;
        auVar17._8_8_ = auVar18._8_8_;
        auVar17._0_8_ = *(unsigned long long *)(puVar27 + 10);
        auVar21._8_4_ = in_a1_udw;
        auVar21._0_8_ = *(long *)(puVar27 + 8) + ((long)(int)param_2 & 0xffffffc0U) * 0x4000000;
        auVar21._12_4_ = in_a1_udw_00;
        auVar18 = _pcpyld(auVar17,auVar21);
        DAT_00764970[8] = auVar18._0_4_;
        DAT_00764970[9] = auVar18._4_4_;
        DAT_00764970[10] = auVar18._8_4_;
        DAT_00764970[0xb] = auVar18._12_4_;
        auVar18 = *(unsigned char (*) [16])(puVar27 + 0xc);
        DAT_00764970[0xc] = auVar18._0_4_;
        DAT_00764970[0xd] = auVar18._4_4_;
        DAT_00764970[0xe] = auVar18._8_4_;
        DAT_00764970[0xf] = auVar18._12_4_;
        puVar27 = puVar27 + 0x10;
        DAT_00764970 = DAT_00764970 + 0x10;
      } while (uVar12 < uVar25);
    }
    puVar13 = DAT_00764970;
    if ((*(unsigned char *)(param_1 + 0x21) & 0x40) == 0) {
      uVar14 = auVar18._8_8_;
    }
    else {
      puVar27 = (unsigned int *)puVar27[-3];
      uVar25 = puVar27[1];
      uVar12 = puVar27[2];
      uVar32 = puVar27[3];
      uVar29 = piVar26[0xb] - 0x50;
      *DAT_00764970 = *puVar27;
      DAT_00764970[1] = uVar25;
      DAT_00764970[2] = uVar12;
      DAT_00764970[3] = uVar32;
      uVar25 = puVar27[5];
      uVar12 = puVar27[6];
      uVar32 = puVar27[7];
      DAT_00764970[4] = puVar27[4];
      DAT_00764970[5] = uVar25;
      DAT_00764970[6] = uVar12;
      DAT_00764970[7] = uVar32;
      uVar25 = puVar27[9];
      uVar12 = puVar27[10];
      uVar32 = puVar27[0xb];
      DAT_00764970[8] = puVar27[8];
      DAT_00764970[9] = uVar25;
      DAT_00764970[10] = uVar12;
      DAT_00764970[0xb] = uVar32;
      uVar25 = puVar27[0xd];
      uVar12 = puVar27[0xe];
      uVar32 = puVar27[0xf];
      DAT_00764970[0xc] = puVar27[0xc];
      DAT_00764970[0xd] = uVar25;
      DAT_00764970[0xe] = uVar12;
      DAT_00764970[0xf] = uVar32;
      auVar18 = *(unsigned char (*) [16])(puVar27 + 0x10);
      DAT_00764970[0x10] = auVar18._0_4_;
      DAT_00764970[0x11] = auVar18._4_4_;
      DAT_00764970[0x12] = auVar18._8_4_;
      DAT_00764970[0x13] = auVar18._12_4_;
      puVar27 = *(unsigned int **)(param_1 + 8);
      puVar13 = DAT_00764970 + 0x14;
      if (uVar29 >= 0x40) {
        if (uVar29 == 0) {
          uVar14 = auVar18._8_8_;
        }
        else {
          do {
            uVar24 = puVar27[1];
            in_a2_udw = puVar27[2];
            in_a2_udw_00 = puVar27[3];
            uVar29 = uVar29 - 0x40;
            uVar9 = puVar27[4];
            uVar23 = puVar27[5];
            in_a1_udw = puVar27[6];
            in_a1_udw_00 = puVar27[7];
            auVar18 = *(unsigned char (*) [16])(puVar27 + 8);
            uVar25 = puVar27[0xc];
            uVar12 = puVar27[0xd];
            uVar32 = puVar27[0xe];
            uVar30 = puVar27[0xf];
            *puVar13 = *puVar27;
            puVar13[1] = uVar24;
            puVar13[2] = in_a2_udw;
            puVar13[3] = in_a2_udw_00;
            puVar27 = puVar27 + 0x10;
            puVar13[4] = uVar9;
            puVar13[5] = uVar23;
            puVar13[6] = in_a1_udw;
            puVar13[7] = in_a1_udw_00;
            puVar13[8] = auVar18._0_4_;
            puVar13[9] = auVar18._4_4_;
            puVar13[10] = auVar18._8_4_;
            puVar13[0xb] = auVar18._12_4_;
            puVar13[0xc] = uVar25;
            puVar13[0xd] = uVar12;
            puVar13[0xe] = uVar32;
            puVar13[0xf] = uVar30;
            puVar13 = puVar13 + 0x10;
          } while (uVar29 != 0);
          uVar14 = auVar18._8_8_;
        }
      }
      else {
        auVar5 = *(unsigned char (*) [12])*(unsigned char (*) [16])(puVar27 + 4);
        uVar29 = puVar27[7];
        auVar18 = *(unsigned char (*) [16])(puVar27 + 4);
        uVar25 = puVar27[1];
        uVar12 = puVar27[2];
        uVar32 = puVar27[3];
        *puVar13 = *puVar27;
        DAT_00764970[0x15] = uVar25;
        DAT_00764970[0x16] = uVar12;
        DAT_00764970[0x17] = uVar32;
        DAT_00764970[0x18] = auVar5._0_4_;
        DAT_00764970[0x19] = auVar5._4_4_;
        DAT_00764970[0x1a] = auVar5._8_4_;
        DAT_00764970[0x1b] = uVar29;
        puVar13 = DAT_00764970 + 0x1c;
        uVar14 = auVar18._8_8_;
      }
    }
    auVar19._8_8_ = uVar14;
    auVar19._0_8_ = 0x10000002;
    auVar3._8_4_ = in_a2_udw;
    auVar3._0_8_ = 0x5000000200000000;
    auVar3._12_4_ = in_a2_udw_00;
    auVar18 = _pcpyld(auVar3,auVar19);
    *puVar13 = auVar18._0_4_;
    puVar13[1] = auVar18._4_4_;
    puVar13[2] = auVar18._8_4_;
    puVar13[3] = auVar18._12_4_;
    auVar20._8_8_ = uVar14;
    auVar20._0_8_ = 0x1000000000008001;
    auVar4._8_4_ = in_a2_udw;
    auVar4._0_8_ = 0xe;
    auVar4._12_4_ = in_a2_udw_00;
    auVar21 = _pcpyld(auVar4,auVar20);
    auVar16._8_8_ = auVar18._8_8_;
    auVar16._0_8_ = 0x7f;
    auVar7._4_4_ = in_a1_udw_00;
    auVar7._0_4_ = in_a1_udw;
    auVar7._8_8_ = 0;
    auVar18 = _pcpyld(auVar16,auVar7 << 0x40);
    puVar13[4] = auVar21._0_4_;
    puVar13[5] = auVar21._4_4_;
    puVar13[6] = auVar21._8_4_;
    puVar13[7] = auVar21._12_4_;
    puVar13[8] = auVar18._0_4_;
    puVar13[9] = auVar18._4_4_;
    puVar13[10] = auVar18._8_4_;
    puVar13[0xb] = auVar18._12_4_;
    DAT_00764970 = puVar13 + 0xc;
  }
  FUN_0042ba20();
  *piVar26 = *piVar26 + 1;
  FUN_0042ba70();
  piVar26[1] = piVar26[1] + 1;
  if (piVar26[1] == 1) {
    FUN_003f36a0(piVar26);
  }
  return;
}
