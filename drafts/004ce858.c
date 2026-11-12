/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ce858 func_004ce858 src/cri/adx_sjd.c - romwright draft, unreviewed */
typedef struct fn_004ce858_arg0 fn_004ce858_arg0;
struct fn_004ce858_arg0 {
    unsigned char pad_0x0[0x1];
    char field_0x1; /* +0x1 */
    unsigned char pad_0x2[0x9e];
    int field_0xa0; /* +0xa0 */
    int field_0xa4; /* +0xa4 */
};


void FUN_004ce858(fn_004ce858_arg0 *param_1)

{
  char cVar1;
  int iVar2;
  
  if (param_1->field_0xa0 < 1) {
    cVar1 = param_1->field_0x1;
  }
  else {
    FUN_004c54a8();
    FUN_004ce900(param_1);
    FUN_004c54c0();
    cVar1 = param_1->field_0x1;
  }
  if (cVar1 == '\x02') {
    FUN_004ce7a8(param_1);
    iVar2 = param_1->field_0xa4;
  }
  else if (cVar1 != '\x01') {
    iVar2 = param_1->field_0xa4;
  }
  else {
    FUN_004cddc0(param_1);
    iVar2 = param_1->field_0xa4;
  }
  if (iVar2 < 1) {
    return;
  }
  FUN_004c54a8();
  FUN_004cea50(param_1);
  FUN_004c54c0();
  return;
}
