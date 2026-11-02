/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00227230 func_00227230 src/Battle/btlVoice_inferred.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x002275b4) */
/* WARNING: Removing unreachable block (ram,0x0022747c) */
/* WARNING: Removing unreachable block (ram,0x002276d4) */

void FUN_00227230(unsigned long long param_1)

{
  unsigned short uVar1;
  unsigned short uVar2;
  unsigned int uVar3;
  int iVar4;
  unsigned long long uVar5;
  long lVar6;
  int iVar7;
  int uStack_c0;
  unsigned int uStack_bc;
  unsigned int uStack_b8;
  unsigned char auStack_b4 [16];
  int uStack_a4;
  unsigned int uStack_a0;
  unsigned int uStack_9c;
  unsigned char auStack_98 [24];
  int uStack_80;
  unsigned int uStack_7c;
  unsigned int uStack_78;
  unsigned char auStack_74 [16];
  int uStack_64;
  unsigned int uStack_60;
  unsigned int uStack_5c;
  unsigned char auStack_58 [24];
  int uStack_40;
  unsigned int uStack_3c;
  unsigned int uStack_38;
  unsigned char auStack_34 [16];
  int uStack_24;
  unsigned int uStack_20;
  unsigned int uStack_1c;
  unsigned char auStack_18 [24];
  
  uVar2 = FUN_001c0e50();
  iVar7 = (int)param_1;
  *(unsigned short *)(iVar7 + 0x110) = uVar2;
  uVar5 = FUN_001eb440(*(int *)(iVar7 + 0xe0) + 0x38);
  *(unsigned int *)(iVar7 + 300) = 0;
  if ((*(unsigned short *)(*(int *)(iVar7 + 0xe0) + 0x1a) & 0x10) != 0) {
    FUN_0019de70(*(unsigned int *)(*(int *)(*(int *)(iVar7 + 0xe0) + 0x30) + 0xa0c),1);
  }
  switch(*(unsigned short *)(iVar7 + 0x110)) {
  case 1:
    if ((*(unsigned short *)(*(int *)(iVar7 + 0xe0) + 0x1a) & 0x10) != 0) {
      FUN_001c5110(param_1);
      return;
    }
  case 2:
    if ((uVar5 & 2) == 0) {
      FUN_002266b0(0,0x40400000,0,0x40000000,param_1);
      break;
    }
  case 4:
    if ((uVar5 & 2) != 0) goto LAB_002273b0;
    lVar6 = FUN_00243ce0(*(unsigned int *)(*(int *)(*(int *)(iVar7 + 0xe0) + 0x30) + 0xa64));
    if ((lVar6 != 0) && (lVar6 = FUN_0019fc70(*(unsigned int *)(iVar7 + 0xe0)), lVar6 != 0)) {
      FUN_002250a0(0,0x3f400000,param_1);
    }
    else {
      FUN_002266b0(0,0x40400000,0,0x40000000,param_1);
    }
    break;
  case 3:
  case 5:
LAB_002273b0:
    if ((uVar5 & 2) != 0) {
      uVar3 = FUN_00231d70(2);
      iVar4 = *(int *)(DAT_0076449c + 0xb98) + (uVar3 & 0xffff) * 0x34;
      FUN_001bd780(auStack_34,iVar4 + 0x5b4,iVar4 + 0x5c0,0x60a0e0);
      uStack_40 = *(unsigned int *)(iVar4 + 0x5b4);
      uStack_3c = *(unsigned int *)(iVar4 + 0x5b8);
      uStack_38 = *(unsigned int *)(iVar4 + 0x5bc);
      FUN_001bd780(auStack_18,iVar4 + 0x5cc,iVar4 + 0x5d8,0x60a0e0);
      uStack_24 = *(unsigned int *)(iVar4 + 0x5cc);
      uStack_20 = *(unsigned int *)(iVar4 + 0x5d0);
      uStack_1c = *(unsigned int *)(iVar4 + 0x5d4);
      uVar1 = *(unsigned short *)(iVar4 + 0x5b0);
      FUN_001bac20(DAT_0076449c + 0x24,&uStack_40,&uStack_24,1);
      FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
      FUN_001bcd40(0,*(unsigned int *)(iVar7 + 0xe0),0,0,0x100);
      break;
    }
  case 6:
    if ((*(unsigned short *)(*(int *)(iVar7 + 0xe0) + 0x1a) & 1) != 0) {
      if (*(char *)(*(int *)(*(int *)(iVar7 + 0xe0) + 0x30) + 0xa2) == '\0') {
        iVar4 = *(int *)(DAT_0076449c + 0xb98);
        FUN_001bd780(auStack_74,iVar4 + 0x61c,iVar4 + 0x628,0x60a0e0);
        uStack_80 = *(unsigned int *)(iVar4 + 0x61c);
        uStack_7c = *(unsigned int *)(iVar4 + 0x620);
        uStack_78 = *(unsigned int *)(iVar4 + 0x624);
        FUN_001bd780(auStack_58,iVar4 + 0x634,iVar4 + 0x640,0x60a0e0);
        uStack_64 = *(unsigned int *)(iVar4 + 0x634);
        uStack_60 = *(unsigned int *)(iVar4 + 0x638);
        uStack_5c = *(unsigned int *)(iVar4 + 0x63c);
        uVar1 = *(unsigned short *)(iVar4 + 0x618);
        FUN_001bac20(DAT_0076449c + 0x24,&uStack_80,&uStack_64,1);
        FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
      }
      else {
        uVar3 = FUN_00231d70(2);
        iVar4 = *(int *)(DAT_0076449c + 0xb98) + (uVar3 & 0xffff) * 0x34;
        FUN_001bd780(auStack_b4,iVar4 + 0x650,iVar4 + 0x65c,0x60a0e0);
        uStack_c0 = *(unsigned int *)(iVar4 + 0x650);
        uStack_bc = *(unsigned int *)(iVar4 + 0x654);
        uStack_b8 = *(unsigned int *)(iVar4 + 0x658);
        FUN_001bd780(auStack_98,iVar4 + 0x668,iVar4 + 0x674,0x60a0e0);
        uStack_a4 = *(unsigned int *)(iVar4 + 0x668);
        uStack_a0 = *(unsigned int *)(iVar4 + 0x66c);
        uStack_9c = *(unsigned int *)(iVar4 + 0x670);
        uVar1 = *(unsigned short *)(iVar4 + 0x64c);
        FUN_001bac20(DAT_0076449c + 0x24,&uStack_c0,&uStack_a4,1);
        FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
      }
      FUN_001bcd40(0,*(unsigned int *)(iVar7 + 0xe0),0,0,0x100);
    }
  }
  return;
}
