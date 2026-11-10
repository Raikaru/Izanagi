/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00448d10 func_00448d10 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

float FUN_00448d10(float param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar2 = DAT_00755bbc;
  fVar3 = ABS(param_1);
  if (fVar3 == 1.0f) {
    fVar2 = 0.0f;
    if ((int)param_1 < 1) {
      fVar2 = DAT_00755b90;
    }
  }
  else if ((unsigned int)fVar3 >= 0x3f800001) {
    fVar2 = (param_1 - param_1) / (param_1 - param_1);
  }
  else {
    if ((unsigned int)fVar3 < 0x3f000000) {
      if ((unsigned int)fVar3 < 0x23000001) {
        return DAT_00755b94;
      }
      fVar2 = param_1 * param_1;
      pfVar1 = &DAT_00755b94;
      param_1 = param_1 - (DAT_00755bc4 -
                          param_1 * ((fVar2 * (fVar2 * (fVar2 * (fVar2 * (fVar2 * (fVar2 * 
                                                  DAT_00755b98 + DAT_00755ba0) + DAT_00755ba8) +
                                                  DAT_00755bb0) + DAT_00755bb8) + DAT_00755bc0)) /
                                    (fVar2 * (fVar2 * (fVar2 * (fVar2 * DAT_00755b9c + DAT_00755ba4)
                                                      + DAT_00755bac) + DAT_00755bb4) + DAT_00755bbc
                                    )));
    }
    else {
      if (-1 < (int)param_1) {
        fVar4 = (DAT_00755bbc - param_1) * DAT_00755bc8;
        fVar5 = (float)FUN_0044a4d0(fVar4);
        fVar3 = (float)((unsigned int)fVar5 & 0xfffff000);
        fVar3 = fVar3 + ((fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * DAT_00755b98 +
                                                                      DAT_00755ba0) + DAT_00755ba8)
                                                    + DAT_00755bb0) + DAT_00755bb8) + DAT_00755bc0))
                        / (fVar4 * (fVar4 * (fVar4 * (fVar4 * DAT_00755b9c + DAT_00755ba4) +
                                            DAT_00755bac) + DAT_00755bb4) + fVar2)) * fVar5 +
                        (fVar4 - fVar3 * fVar3) / (fVar5 + fVar3);
        return fVar3 + fVar3;
      }
      fVar3 = (param_1 + DAT_00755bbc) * DAT_00755bc8;
      fVar5 = fVar3 * (fVar3 * (fVar3 * (fVar3 * DAT_00755b9c + DAT_00755ba4) + DAT_00755bac) +
                      DAT_00755bb4) + DAT_00755bbc;
      fVar2 = fVar3 * (fVar3 * (fVar3 * (fVar3 * (fVar3 * DAT_00755b98 + DAT_00755ba0) +
                                        DAT_00755ba8) + DAT_00755bb0) + DAT_00755bb8) + DAT_00755bc0
      ;
      param_1 = (float)FUN_0044a4d0(fVar3);
      pfVar1 = &DAT_00755b90;
      param_1 = param_1 + (((fVar3 * fVar2) / fVar5) * param_1 - DAT_00755bc4);
      param_1 = param_1 + param_1;
    }
    fVar2 = *pfVar1 - param_1;
  }
  return fVar2;
}
