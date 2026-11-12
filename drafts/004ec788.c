/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ec788 AXRNA_Destroy src/cri/ps2_rna.c - romwright draft, unreviewed */
typedef struct fn_004ec788_arg0 fn_004ec788_arg0;
struct fn_004ec788_arg0 {
    undefined1 field_0x0; /* +0x0 */
    unsigned char pad_0x1[0x3];
    int field_0x4; /* +0x4 */
    unsigned char field_0x8; /* +0x8 */
    unsigned char pad_0x9[0x7];
    unsigned char field_0x10; /* +0x10 */
    unsigned char pad_0x11[0xf];
    int field_0x20; /* +0x20 */
};


void FUN_004ec788(fn_004ec788_arg0 *param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int aiStack_30 [4];
  
  if ((DAT_0073a748 == 1) && (DAT_0073a740 < 1)) {
    param_1->field_0x0 = 0;
  }
  else {
    iVar2 = param_1->field_0x4;
    iVar4 = 0;
    if (0 < iVar2) {
      piVar3 = (int *)&param_1->field_0x10;
      iVar1 = *piVar3;
      while( 1 ) {
        piVar3 = piVar3 + 1;
        if (iVar1 != 0) {
          FUN_004e5000();
          iVar2 = param_1->field_0x4;
        }
        iVar4 = iVar4 + 1;
        if (iVar2 <= iVar4) break;
        iVar1 = *piVar3;
      }
    }
    aiStack_30[0] = param_1->field_0x20;
    FUN_004e58a0(9,aiStack_30,1,0,0);
    iVar2 = param_1->field_0x4;
    iVar4 = 0;
    if (0 < iVar2) {
      piVar3 = (int *)&param_1->field_0x8;
      iVar1 = *piVar3;
      while( 1 ) {
        piVar3 = piVar3 + 1;
        if (iVar1 != 0) {
          FUN_004eb828();
          iVar2 = param_1->field_0x4;
        }
        iVar4 = iVar4 + 1;
        if (iVar2 <= iVar4) break;
        iVar1 = *piVar3;
      }
    }
    param_1->field_0x0 = 0;
  }
  return;
}
