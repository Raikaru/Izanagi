/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00449120 func_00449120 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

float FUN_00449120(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar2 = ABS(param_1);
  if (fVar2 == 1.0f) {
    fVar2 = param_1 * DAT_00755c10 + param_1 * DAT_00755c0c;
  }
  else if ((unsigned int)fVar2 >= 0x3f800001) {
    fVar2 = (param_1 - param_1) / (param_1 - param_1);
  }
  else {
    if ((unsigned int)fVar2 < 0x3f000000) {
      if (0x31ffffff < (unsigned int)fVar2) {
        fVar2 = param_1 * param_1;
        return param_1 + param_1 * ((fVar2 * (fVar2 * (fVar2 * (fVar2 * (fVar2 * (fVar2 * 
                                                  DAT_00755c1c + DAT_00755c24) + DAT_00755c2c) +
                                                  DAT_00755c34) + DAT_00755c3c) + DAT_00755c40)) /
                                   (fVar2 * (fVar2 * (fVar2 * (fVar2 * DAT_00755c20 + DAT_00755c28)
                                                     + DAT_00755c30) + DAT_00755c38) + DAT_00755c18)
                                   );
      }
      if (DAT_00755c18 < param_1 + DAT_00755c14) {
        return param_1;
      }
    }
    fVar1 = (float)FUN_0044b6c0();
    fVar4 = (DAT_00755c18 - fVar1) * DAT_00755c44;
    fVar5 = fVar4 * (fVar4 * (fVar4 * (fVar4 * DAT_00755c20 + DAT_00755c28) + DAT_00755c30) +
                    DAT_00755c38) + DAT_00755c18;
    fVar3 = fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * DAT_00755c1c + DAT_00755c24) +
                                               DAT_00755c2c) + DAT_00755c34) + DAT_00755c3c) +
                    DAT_00755c40);
    fVar1 = (float)FUN_0044a4d0(fVar4);
    if ((unsigned int)fVar2 >= 0x3f79999a) {
      fVar1 = fVar1 + fVar1 * (fVar3 / fVar5);
      fVar2 = DAT_00755c0c - ((fVar1 + fVar1) - DAT_00755c10);
    }
    else {
      fVar2 = (float)((unsigned int)fVar1 & 0xfffff000);
      fVar4 = (fVar4 - fVar2 * fVar2) / (fVar1 + fVar2);
      fVar2 = DAT_00755c48 -
              (((fVar1 + fVar1) * (fVar3 / fVar5) - (DAT_00755c10 - (fVar4 + fVar4))) -
              (DAT_00755c48 - (fVar2 + fVar2)));
    }
    if ((int)param_1 < 1) {
      fVar2 = -fVar2;
    }
  }
  return fVar2;
}
