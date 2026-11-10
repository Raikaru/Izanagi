/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00226c40 func_00226c40 src/Battle/btlVoice_inferred.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x00227070) */
/* WARNING: Removing unreachable block (ram,0x00226f38) */
/* WARNING: Removing unreachable block (ram,0x00227190) */

void FUN_00226c40(unsigned long long param_1)

{
  unsigned short uVar1;
  int iVar2;
  unsigned char bVar3;
  unsigned short uVar4;
  unsigned int uVar5;
  int iVar6;
  long lVar7;
  unsigned int uVar8;
  int iVar9;
  int iVar10;
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
  unsigned char auStack_18 [20];
  unsigned int uStack_4;
  
  iVar9 = (int)param_1;
  iVar6 = *(int *)(*(int *)(iVar9 + 0xe0) + 0x30);
  iVar10 = *(int *)(iVar6 + 0xa0c);
  if ((*(int *)(iVar9 + 0xe0) != 0) && (*(char *)(iVar6 + 0xa2) == '\0')) {
    for (uVar5 = 0; uVar5 < *(unsigned short *)(*(int *)(iVar9 + 0xe0) + 0x6a); uVar5 = (uVar5 + 1) & 0xffff
        ) {
      iVar2 = *(int *)(*(int *)(*(int *)(iVar9 + 0xe0) + uVar5 * 4 + 0x38) + 0x30);
      if (*(char *)(iVar2 + 0xa2) == '\x01') {
        FUN_00195590(iVar2,iVar6 + 4);
      }
    }
  }
  uVar4 = FUN_001c0e50(param_1);
  *(unsigned short *)(iVar9 + 0x110) = uVar4;
  bVar3 = 0;
  if (iVar10 != 0) {
    lVar7 = FUN_0022f950(*(unsigned int *)(iVar9 + 0xe0),iVar10);
    bVar3 = lVar7 != 0;
  }
  if ((bVar3) && (*(short *)(iVar9 + 0x110) != 1)) {
    FUN_0019d0c0(iVar10);
    uStack_4 = (unsigned int)*(uint3 *)(iVar10 + 0x30);
    FUN_00194f60(iVar10,uStack_4);
  }
  FUN_0019de70(iVar10,0);
  switch(*(unsigned short *)(iVar9 + 0x110)) {
  case 0:
  case 1:
  case 2:
  case 4:
    FUN_001c5110(param_1);
    iVar6 = *(int *)(iVar9 + 0xe0);
    if ((((*(short *)(iVar6 + 0x6a) == 1) && (*(char *)(*(int *)(iVar6 + 0x30) + 0xa2) == '\0')) &&
        (iVar6 != *(int *)(iVar6 + 0x38))) &&
       (*(char *)(*(int *)(*(int *)(iVar6 + 0x38) + 0x30) + 0xa2) == '\0')) {
      FUN_001bcd40(0,iVar6,0,0,8);
    }
    break;
  case 3:
  case 5:
    iVar6 = *(int *)(*(int *)(iVar9 + 0xe0) + 0x30);
    uVar5 = FUN_00231d70(2);
    if (*(char *)(iVar6 + 0xa2) != '\0') {
      uVar8 = 0;
    }
    else {
      uVar8 = 0;
      for (iVar10 = *(int *)(DAT_0076449c + 0x17c); (iVar10 != 0 && (iVar6 != iVar10));
          iVar10 = *(int *)(iVar10 + 0xa68)) {
        uVar8 = (uVar8 + 1) & 0xffff;
      }
    }
    iVar6 = *(int *)(DAT_0076449c + 0xb98) + uVar8 * 0x68 + (uVar5 & 0xffff) * 0x34;
    FUN_001bd780(auStack_34,iVar6 + 0x3ac,iVar6 + 0x3b8,0x60a0e0);
    uStack_40 = *(unsigned int *)(iVar6 + 0x3ac);
    uStack_3c = *(unsigned int *)(iVar6 + 0x3b0);
    uStack_38 = *(unsigned int *)(iVar6 + 0x3b4);
    FUN_001bd780(auStack_18,iVar6 + 0x3c4,iVar6 + 0x3d0,0x60a0e0);
    uStack_24 = *(unsigned int *)(iVar6 + 0x3c4);
    uStack_20 = *(unsigned int *)(iVar6 + 0x3c8);
    uStack_1c = *(unsigned int *)(iVar6 + 0x3cc);
    uVar1 = *(unsigned short *)(iVar6 + 0x3a8);
    FUN_001bac20(DAT_0076449c + 0x24,&uStack_40,&uStack_24,1);
    FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
    FUN_001bcd40(0,*(unsigned int *)(iVar9 + 0xe0),0,0,0x100);
    break;
  case 6:
    if ((*(unsigned short *)(*(int *)(iVar9 + 0xe0) + 0x1a) & 1) != 0) {
      if (*(char *)(*(int *)(*(int *)(iVar9 + 0xe0) + 0x30) + 0xa2) == '\0') {
        iVar6 = *(int *)(DAT_0076449c + 0xb98);
        FUN_001bd780(auStack_74,iVar6 + 0x61c,iVar6 + 0x628,0x60a0e0);
        uStack_80 = *(unsigned int *)(iVar6 + 0x61c);
        uStack_7c = *(unsigned int *)(iVar6 + 0x620);
        uStack_78 = *(unsigned int *)(iVar6 + 0x624);
        FUN_001bd780(auStack_58,iVar6 + 0x634,iVar6 + 0x640,0x60a0e0);
        uStack_64 = *(unsigned int *)(iVar6 + 0x634);
        uStack_60 = *(unsigned int *)(iVar6 + 0x638);
        uStack_5c = *(unsigned int *)(iVar6 + 0x63c);
        uVar1 = *(unsigned short *)(iVar6 + 0x618);
        FUN_001bac20(DAT_0076449c + 0x24,&uStack_80,&uStack_64,1);
        FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
      }
      else {
        uVar5 = FUN_00231d70(2);
        iVar6 = *(int *)(DAT_0076449c + 0xb98) + (uVar5 & 0xffff) * 0x34;
        FUN_001bd780(auStack_b4,iVar6 + 0x650,iVar6 + 0x65c,0x60a0e0);
        uStack_c0 = *(unsigned int *)(iVar6 + 0x650);
        uStack_bc = *(unsigned int *)(iVar6 + 0x654);
        uStack_b8 = *(unsigned int *)(iVar6 + 0x658);
        FUN_001bd780(auStack_98,iVar6 + 0x668,iVar6 + 0x674,0x60a0e0);
        uStack_a4 = *(unsigned int *)(iVar6 + 0x668);
        uStack_a0 = *(unsigned int *)(iVar6 + 0x66c);
        uStack_9c = *(unsigned int *)(iVar6 + 0x670);
        uVar1 = *(unsigned short *)(iVar6 + 0x64c);
        FUN_001bac20(DAT_0076449c + 0x24,&uStack_c0,&uStack_a4,1);
        FUN_001bbef0((float)uVar1 / 30.0f,DAT_0076449c + 0x24);
      }
      FUN_001bcd40(0,*(unsigned int *)(iVar9 + 0xe0),0,0,0x100);
    }
  }
  return;
}
