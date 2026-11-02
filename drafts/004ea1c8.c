/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea1c8 mfCiClose src/cri/mfci.c - romwright draft, unreviewed */

void FUN_004ea1c8(long param_1)

{
  if (param_1 != 0) {
    FUN_004ea490();
    if (*(char *)param_1 == '\x01') {
      *(char *)param_1 = '\0';
      FUN_004ea098(param_1);
    }
  }
  return;
}
