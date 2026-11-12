/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00509268 func_00509268 src/cri/mwsfdcre.c - romwright draft, unreviewed */

unsigned long long FUN_00509268(long param_1)

{
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned long long *puVar3;
  int iVar4;
  unsigned int uVar5;
  long lVar6;
  unsigned long long uVar7;
  long lVar8;
  unsigned long long uVar9;
  unsigned long long uVar10;
  int iVar11;
  unsigned long long uVar12;
  unsigned long long extraout_a0;
  unsigned long long extraout_a1;
  unsigned long long uVar13;
  int *piVar14;
  unsigned int *puVar15;
  unsigned long long uVar16;
  unsigned int uVar17;
  int uStack_60;
  unsigned char auStack_5c [4];
  unsigned char auStack_58 [4];
  unsigned char auStack_54 [4];
  unsigned char auStack_50 [4];
  unsigned char auStack_4c [12];
  
  if (param_1 == 0) {
    uVar12 = 0x75e278;
    FUN_00511220(uVar12);
    return 0;
  }
  lVar6 = FUN_00509698();
  uVar9 = 1;
  if (lVar6 != 1) {
    return 0;
  }
  iVar4 = FUN_0050b3d0();
  iVar11 = 0;
  piVar14 = (int *)(iVar4 + 0x6c);
  uVar16 = (long)(int)piVar14;
  if (*piVar14 == 1) {
    iVar11 = 1;
    while( 1 ) {
      piVar14 = (int *)((int)uVar16 + 0x2ac);
      uVar9 = (unsigned long long)(int)piVar14;
      if ((7 < iVar11) || (uVar16 = uVar9, *piVar14 != 1)) break;
      iVar11 = iVar11 + 1;
    }
  }
  if (iVar11 == 8) {
    FUN_0050b8f0(0xfffffffffffffff5);
    uVar12 = 0x75e2a8;
    FUN_00511220(uVar12);
    return 0;
  }
  lVar6 = FUN_005095f0(param_1);
  if (lVar6 == -1) {
    uVar12 = 0x75e2f8;
    FUN_00511220(uVar12);
    return 0;
  }
  FUN_0043f9c8(uVar16,0,0x2ac);
  uVar7 = FUN_00509640(uVar16,param_1);
  uVar17 = (unsigned int)param_1;
  uVar1 = (uVar17 + 7) & 7;
  uVar2 = uVar17 & 7;
  uVar7 = (((*(long *)((uVar17 + 7) - uVar1) << (7 - uVar1) * 8) |
           (uVar7 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
          (*(unsigned long long *)(uVar17 - uVar2) >> uVar2 * 8);
  uVar1 = (uVar17 + 0xf) & 7;
  uVar2 = (uVar17 + 8) & 7;
  uVar10 = (((*(long *)((uVar17 + 0xf) - uVar1) << (7 - uVar1) * 8) |
            (uVar9 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
           (*(unsigned long long *)((uVar17 + 8) - uVar2) >> uVar2 * 8);
  uVar1 = (uVar17 + 0x17) & 7;
  uVar2 = (uVar17 + 0x10) & 7;
  uVar9 = (((*(long *)((uVar17 + 0x17) - uVar1) << (7 - uVar1) * 8) |
           (extraout_a0 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
          (*(unsigned long long *)((uVar17 + 0x10) - uVar2) >> uVar2 * 8);
  uVar1 = (uVar17 + 0x1f) & 7;
  uVar2 = (uVar17 + 0x18) & 7;
  uVar13 = (((*(long *)((uVar17 + 0x1f) - uVar1) << (7 - uVar1) * 8) |
            (extraout_a1 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
           (*(unsigned long long *)((uVar17 + 0x18) - uVar2) >> uVar2 * 8);
  puVar15 = (unsigned int *)uVar16;
  uVar1 = ((int)puVar15 + 0xfU) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0xfU) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar7 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 2) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 2) - uVar1);
  *puVar3 = (uVar7 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  uVar1 = ((int)puVar15 + 0x17U) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0x17U) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar10 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 4) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 4) - uVar1);
  *puVar3 = (uVar10 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  uVar1 = ((int)puVar15 + 0x1fU) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0x1fU) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar9 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 6) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 6) - uVar1);
  *puVar3 = (uVar9 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  uVar1 = ((int)puVar15 + 0x27U) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0x27U) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar13 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 8) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 8) - uVar1);
  *puVar3 = (uVar13 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  uVar1 = (uVar17 + 0x27) & 7;
  uVar2 = (uVar17 + 0x20) & 7;
  uVar9 = (((*(long *)((uVar17 + 0x27) - uVar1) << (7 - uVar1) * 8) |
           (uVar7 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
          (*(unsigned long long *)((uVar17 + 0x20) - uVar2) >> uVar2 * 8);
  uVar1 = (uVar17 + 0x2f) & 7;
  uVar2 = (uVar17 + 0x28) & 7;
  uVar7 = (((*(long *)((uVar17 + 0x2f) - uVar1) << (7 - uVar1) * 8) |
           (uVar10 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) & (-1L << (8 - uVar2) * 8)) |
          (*(unsigned long long *)((uVar17 + 0x28) - uVar2) >> uVar2 * 8);
  uVar1 = ((int)puVar15 + 0x2fU) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0x2fU) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar9 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 10) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 10) - uVar1);
  *puVar3 = (uVar9 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  uVar1 = ((int)puVar15 + 0x37U) & 7;
  puVar3 = (unsigned long long *)(((int)puVar15 + 0x37U) - uVar1);
  *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar7 >> (7 - uVar1) * 8);
  uVar1 = (unsigned int)(puVar15 + 0xc) & 7;
  puVar3 = (unsigned long long *)((int)(puVar15 + 0xc) - uVar1);
  *puVar3 = (uVar7 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
  lVar6 = FUN_00508708(uVar16,param_1);
  puVar15[0xf] = (int)lVar6;
  if (lVar6 == 0) {
    uVar12 = 0x75e338;
  }
  else {
    FUN_0050caa0(uVar16);
    FUN_00508eb0(uVar16);
    FUN_00507f40(param_1,&uStack_60,auStack_5c,auStack_58,auStack_54,auStack_50,auStack_4c);
    FUN_00509058(uVar16,param_1,*(unsigned int *)(iVar4 + 8),*(unsigned int *)(iVar4 + 0xc));
    lVar8 = FUN_00509240(uVar16);
    puVar15[0x70] = (int)lVar8;
    if (lVar8 == 0) {
      uVar12 = 0x75e360;
    }
    else {
      lVar8 = FUN_004ed600(0,0);
      puVar15[0x78] = (int)lVar8;
      if (lVar8 == 0) {
        uVar12 = 0x75e388;
      }
      else {
        puVar15[0xe] = 1;
        puVar15[0x16] = uStack_60;
        puVar15[0x1a] = 0;
        puVar15[1] = 0;
        puVar15[0x13] = *(unsigned int *)(uVar17 + 0x20);
        puVar15[0x14] = *(unsigned int *)(uVar17 + 0x20);
        lVar6 = FUN_00525340(lVar6,3,puVar15 + 0x11);
        if (lVar6 != 0) {
          puVar15[0x11] = 0;
        }
        puVar15[0x1b] = 1;
        *(unsigned char *)(puVar15 + 0x1c) = 0;
        *(unsigned char *)((int)puVar15 + 0x71) = 0;
        *(unsigned char *)((int)puVar15 + 0x72) = 0;
        *(unsigned char *)((int)puVar15 + 0x73) = 0;
        puVar15[0x17] = 0;
        FUN_00511dc0(uVar16,0);
        FUN_00511dd0(uVar16,0);
        puVar15[0x15] = 1;
        puVar15[0x21] = 0;
        lVar6 = FUN_00512138(puVar15[0x70]);
        puVar15[0x10] = (int)lVar6;
        if (lVar6 == 0) {
          FUN_005097e8(uVar16);
          return 0;
        }
        FUN_0050c590(uVar16);
        uVar12 = FUN_004e8f30(puVar15[0x70]);
        puVar15[0x1d] = 0;
        puVar15[0x12] = (int)uVar12;
        FUN_004e9088(uVar12,puVar15[0x10]);
        lVar6 = FUN_00509720(uVar16);
        if (lVar6 == -1) {
          FUN_005097e8(uVar16);
          return 0;
        }
        lVar6 = FUN_0050e488(puVar15[0x2b],puVar15[0x2c],*(unsigned int *)(uVar17 + 8),
                             *(unsigned int *)(uVar17 + 0xc));
        if (lVar6 != 0) {
          puVar15[0x2a] = (int)lVar6;
          FUN_0050ecb8(uVar16,puVar15[10]);
          uVar5 = FUN_0050f0a0(uVar16);
          puVar15[0x5f] = uVar5;
          lVar6 = FUN_0050f130(uVar16);
          if (lVar6 == 0) {
            FUN_0050f190(uVar16);
            FUN_0050afb8(uVar16);
            FUN_0050b020(uVar16);
            *puVar15 = 1;
            return uVar16;
          }
          uVar12 = 0x75e3d0;
        }
        else {
          uVar12 = 0x75e3b0;
        }
      }
    }
  }
  FUN_00511220(uVar12);
  FUN_005097e8(uVar16);
  return 0;
}
