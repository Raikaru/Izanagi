/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dcc70 func_003dcc70 src/promoted/code1_003d.c - romwright draft, unreviewed */

void FUN_003dcc70(float *param_1,float *param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar2 = param_1[1];
  fVar4 = param_1[2];
  fVar1 = param_1[3];
  *param_3 = *param_1;
  param_3[1] = fVar2;
  param_3[2] = fVar4;
  param_3[3] = fVar1;
  fVar1 = param_1[3] * param_2[3] +
          param_1[2] * param_2[2] + *param_1 * *param_2 + param_1[1] * param_2[1];
  if (fVar1 < 0.0f) {
    if (fVar1 < -1.0f) {
      fVar2 = 1.0f;
    }
    else {
      fVar2 = -fVar1;
    }
    param_3[7] = -param_2[3];
    param_3[4] = -*param_2;
    param_3[5] = -param_2[1];
    param_3[6] = -param_2[2];
  }
  else {
    fVar2 = 1.0f;
    if (1.0f < fVar1) {
      fVar4 = *param_2;
    }
    else {
      fVar4 = *param_2;
      fVar2 = fVar1;
    }
    fVar3 = param_2[1];
    fVar5 = param_2[2];
    fVar1 = param_2[3];
    param_3[4] = fVar4;
    param_3[5] = fVar3;
    param_3[6] = fVar5;
    param_3[7] = fVar1;
  }
  fVar1 = ABS(fVar2);
  if ((unsigned int)fVar1 < 0x3f800000) {
    if ((unsigned int)fVar1 < 0x3f000000) {
      if ((unsigned int)fVar1 < 0x23000001) {
        param_3[8] = 1.5707964f;
      }
      else {
        fVar1 = fVar2 * fVar2;
        param_3[8] = 1.5707963f -
                     (fVar2 - (7.5497894e-08f -
                              fVar2 * ((fVar1 * (fVar1 * (fVar1 * (fVar1 * (fVar1 * (fVar1 * 
                                                  3.479331e-05f + 0.000791535f) + -0.040055536f) +
                                                  0.20121253f) + -0.32556581f) + 0.16666667f)) /
                                      (fVar1 * (fVar1 * (fVar1 * (fVar1 * 0.077038154f + -0.688284f) +
                                                        2.0209458f) + -2.403395f) + 1.0f))));
      }
    }
    else if ((int)fVar2 >= 0) {
      fVar4 = (1.0f - fVar2) * 0.5f;
      fVar3 = SQRT(fVar4);
      fVar1 = (float)((unsigned int)fVar3 & 0xfffff000);
      param_3[8] = (fVar1 + ((fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * (fVar4 * 3.479331e-05f +
                                                                          0.000791535f) +
                                                                 -0.040055536f) + 0.20121253f) +
                                               -0.32556581f) + 0.16666667f)) /
                            (fVar4 * (fVar4 * (fVar4 * (fVar4 * 0.077038154f + -0.688284f) + 2.0209458f
                                              ) + -2.403395f) + 1.0f)) * fVar3 +
                            ((fVar4 + 0.0f) - fVar1 * fVar1) / (fVar3 + fVar1) + 0.0f) * 2.0f;
    }
    else {
      fVar1 = (fVar2 + 1.0f) * 0.5f;
      param_3[8] = 3.1415925f -
                   (SQRT(fVar1) +
                   (((fVar1 * (fVar1 * (fVar1 * (fVar1 * (fVar1 * (fVar1 * 3.479331e-05f +
                                                                  0.000791535f) + -0.040055536f) +
                                                0.20121253f) + -0.32556581f) + 0.16666667f)) /
                    (fVar1 * (fVar1 * (fVar1 * (fVar1 * 0.077038154f + -0.688284f) + 2.0209458f) +
                             -2.403395f) + 1.0f)) * SQRT(fVar1) - 7.5497894e-08f)) * 2.0f;
    }
  }
  else if ((int)fVar2 >= 1) {
    param_3[8] = 0.0f;
  }
  else {
    param_3[8] = 3.1415927f;
  }
  param_3[9] = (float)(unsigned int)(0.99999f <= fVar2);
  if (param_3[9] == 0.0f) {
    fVar2 = param_3[8];
    fVar1 = fVar2 * fVar2;
    fVar1 = 1.0f / (fVar1 * fVar2 *
                   (fVar1 * (fVar1 * (fVar1 * (fVar1 * (fVar1 * 1.589691e-10f + -2.505076e-08f) +
                                              2.7557314e-06f) + -0.0001984127f) + 0.008333334f) +
                   -0.16666667f) + fVar2 + 0.0f);
    param_3[3] = param_3[3] * fVar1;
    *param_3 = *param_3 * fVar1;
    param_3[1] = param_3[1] * fVar1;
    param_3[2] = param_3[2] * fVar1;
    param_3[7] = param_3[7] * fVar1;
    param_3[4] = param_3[4] * fVar1;
    param_3[5] = param_3[5] * fVar1;
    param_3[6] = param_3[6] * fVar1;
  }
  return;
}
