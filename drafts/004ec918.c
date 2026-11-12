/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ec918 AXRNA_SetTransSw src/cri/ps2_rna.c - romwright draft, unreviewed */
typedef struct fn_004ec918_arg0 fn_004ec918_arg0;
struct fn_004ec918_arg0 {
    unsigned char pad_0x0[0x10];
    unsigned char field_0x10; /* +0x10 */
    unsigned char pad_0x11[0x41];
    char field_0x52; /* +0x52 */
    unsigned char pad_0x53[0x5];
    char field_0x58; /* +0x58 */
    undefined1 field_0x59; /* +0x59 */
};


void FUN_004ec918(fn_004ec918_arg0 *param_1,long param_2)

{
  unsigned int uVar1;
  long lVar2;
  unsigned int *puVar3;
  
  lVar2 = (long)param_1->field_0x52;
  if ((((DAT_0073a748 != 1) || (0 < DAT_0073a740)) &&
      (param_1->field_0x58 = (char)param_2, param_2 != 1)) && (0 < lVar2)) {
    puVar3 = (unsigned int *)&param_1->field_0x10;
    do {
      uVar1 = *puVar3;
      puVar3 = puVar3 + 1;
      lVar2 = (long)((int)lVar2 - 1);
      FUN_004e5038(uVar1);
      param_1->field_0x59 = 1;
      param_1->field_0x58 = '\0';
    } while (lVar2 != 0);
  }
  return;
}
