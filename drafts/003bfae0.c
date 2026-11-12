/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bfae0 func_003bfae0 src/promoted/code1_003b.c - romwright draft, unreviewed */

int FUN_003bfae0(int param_1)

{
  unsigned int uVar1;
  long lVar2;
  unsigned long long uVar3;
  float *pfVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  
  uVar1 = *(unsigned int *)(param_1 + 4);
  if ((*(unsigned int *)(param_1 + 0x4c) & 2) != 0) {
    FUN_003bf930();
  }
  lVar2 = FUN_003e9240(uVar1);
  if ((lVar2 != 0) || ((*(unsigned char *)(param_1 + 3) & 1) != 0)) {
    uVar3 = FUN_003e9700(uVar1);
    FUN_003e42a0(param_1 + 0x2c,param_1 + 0x1c,uVar3);
    pfVar4 = (float *)uVar3;
    if (((unsigned int)pfVar4[3] & 3) != 3) {
      fVar8 = pfVar4[2] * pfVar4[2] + *pfVar4 * *pfVar4 + pfVar4[1] * pfVar4[1];
      fVar7 = pfVar4[6] * pfVar4[6] + pfVar4[4] * pfVar4[4] + pfVar4[5] * pfVar4[5];
      fVar5 = pfVar4[10] * pfVar4[10] + pfVar4[8] * pfVar4[8] + pfVar4[9] * pfVar4[9];
      fVar6 = fVar7;
      if (fVar7 < fVar5) {
        fVar6 = fVar5;
      }
      if ((fVar8 < fVar6) && (fVar8 = fVar7, fVar7 < fVar5)) {
        fVar8 = fVar5;
      }
      *(float *)(param_1 + 0x38) = *(float *)(param_1 + 0x28) * SQRT(fVar8);
    }
    else {
      *(unsigned int *)(param_1 + 0x38) = *(unsigned int *)(param_1 + 0x28);
    }
    *(unsigned char *)(param_1 + 3) = *(unsigned char *)(param_1 + 3) & 0xfe;
  }
  return param_1 + 0x2c;
}
