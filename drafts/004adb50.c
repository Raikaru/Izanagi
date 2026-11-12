/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004adb50 func_004adb50 src/Graphics/Model/mdlEffect_inferred_004adb50.c - romwright draft, unreviewed */

void FUN_004adb50(unsigned int *param_1)

{
  int iVar1;
  int iVar2;
  unsigned int uVar3;
  unsigned char in_zero_qw [16];
  long lVar4;
  unsigned char auVar5 [16];
  unsigned char auVar6 [16];
  unsigned char auVar7 [16];
  unsigned int uVar8;
  unsigned int *puVar9;
  float fVar10;
  int uStack_c0;
  unsigned int uStack_bc;
  unsigned int uStack_b8;
  unsigned int uStack_b4;
  unsigned int uStack_b0;
  unsigned int uStack_ac;
  unsigned int uStack_a8;
  unsigned int uStack_a4;
  unsigned int uStack_a0;
  unsigned int uStack_9c;
  unsigned int uStack_98;
  unsigned int uStack_94;
  unsigned int uStack_90;
  unsigned int uStack_8c;
  unsigned int uStack_88;
  unsigned int uStack_84;
  int uStack_80;
  unsigned int uStack_7c;
  unsigned int uStack_78;
  unsigned int uStack_70;
  unsigned int uStack_6c;
  unsigned int uStack_68;
  unsigned int uStack_60;
  unsigned int uStack_5c;
  unsigned int uStack_58;
  unsigned int uStack_50;
  unsigned int uStack_4c;
  unsigned int uStack_48;
  int uStack_30;
  unsigned int uStack_2c;
  unsigned int uStack_28;
  unsigned int uStack_24;
  int uStack_18;
  unsigned int uStack_14;
  unsigned int uStack_10;
  unsigned int uStack_c;
  int iStack_8;
  int iStack_4;
  
  iVar1 = param_1[0xb];
  iVar2 = param_1[0x24];
  uVar3 = param_1[10];
  puVar9 = (unsigned int *)param_1[0x26];
  if ((iVar1 < iVar2) || (iVar2 == 0)) {
    FUN_0048a150(&uStack_80,param_1 + 4);
    FUN_0047a1c0(param_1[0x28],&uStack_80,0);
    uStack_18 = param_1[8];
    uStack_14 = uStack_18;
    uStack_10 = uStack_18;
    FUN_0047a1e0(param_1[0x28],&uStack_18,2);
    uStack_18 = *param_1;
    uStack_14 = param_1[1];
    uStack_10 = param_1[2];
    FUN_0047a180(param_1[0x28],&uStack_18,2);
    FUN_0047a0e0(param_1[0x25],param_1[0x28],0);
    FUN_00478e70(param_1[0x28]);
    if (param_1[0x27] != 0) {
      iStack_8 = FUN_0048abd0(param_1 + 0xc,param_1 + 0x15,iVar1,iVar2);
      iStack_4 = param_1[9];
      auVar5 = _pextlb(0,(long)iStack_4);
      auVar5 = _pextlh(0,auVar5._0_8_);
      auVar6 = _pextlb(0,(long)iStack_8);
      auVar6 = _pextlh(0,auVar6._0_8_);
      auVar7._4_4_ = (int)((float)auVar6._4_4_ * DAT_00761134 * (float)auVar5._4_4_ * DAT_00761134 *
                          255.0f);
      auVar7._0_4_ = (int)((float)auVar6._0_4_ * DAT_00761134 * (float)auVar5._0_4_ * DAT_00761134 *
                          255.0f);
      auVar7._8_4_ = (int)((float)auVar6._8_4_ * DAT_00761134 * (float)auVar5._8_4_ * DAT_00761134 *
                          255.0f);
      auVar7._12_4_ =
           (int)((float)auVar6._12_4_ * DAT_00761134 * (float)auVar5._12_4_ * DAT_00761134 * 255.0f);
      auVar5 = _ppach(in_zero_qw,auVar7);
      auVar5 = _ppacb(in_zero_qw,auVar5);
      uStack_c = auVar5._0_4_;
      fVar10 = (float)FUN_0048aff0(param_1 + 0x19,iVar1,iVar2);
      fVar10 = (fVar10 / 10.0f) * (float)param_1[8];
      for (uVar8 = 0; uVar8 < uVar3; uVar8 = uVar8 + 1) {
        lVar4 = FUN_0047a510(param_1[0x28],uVar8,&uStack_80);
        if (lVar4 != 0) {
          uStack_30 = uStack_50;
          uStack_2c = uStack_4c;
          uStack_28 = uStack_48;
          uStack_24 = 0;
          FUN_004861f0(*puVar9,&uStack_30);
          uStack_c0 = uStack_80;
          uStack_bc = uStack_7c;
          uStack_b8 = uStack_78;
          uStack_b4 = 0;
          uStack_b0 = uStack_70;
          uStack_ac = uStack_6c;
          uStack_a8 = uStack_68;
          uStack_a4 = 0;
          uStack_a0 = uStack_60;
          uStack_9c = uStack_5c;
          uStack_98 = uStack_58;
          uStack_94 = 0;
          uStack_90 = uStack_50;
          uStack_8c = uStack_4c;
          uStack_88 = uStack_48;
          uStack_84 = 0;
          FUN_0048a980(&uStack_c0);
          FUN_00486330(*puVar9);
          FUN_00486400(fVar10,*puVar9);
          FUN_004865c0(*puVar9,auVar5._0_4_);
          FUN_00485630(*puVar9);
        }
        puVar9 = puVar9 + 1;
      }
    }
  }
  param_1[0xb] = param_1[0xb] + 1;
  return;
}
