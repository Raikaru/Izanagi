/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00526810 func_00526810 src/cri/sfd_tim.c - romwright draft, unreviewed */
typedef struct fn_00526810_arg1 fn_00526810_arg1;
struct fn_00526810_arg1 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
    int field_0x18; /* +0x18 */
    unsigned char pad_0x1c[0x2];
    short field_0x1e; /* +0x1e */
};


/* WARNING: Removing unreachable block (ram,0x00526854) */

void FUN_00526810(unsigned int param_1,fn_00526810_arg1 *param_2,int *param_3,unsigned int *param_4)

{
  *param_3 = ((param_2->field_0x10 * 0x1e + param_2->field_0xc / 10) * 2 +
              param_2->field_0x8 * 0x34b54 + param_2->field_0xc * 0xe0e +
             param_2->field_0x14 + param_2->field_0x18) * 1000 +
             (param_2->field_0x1e * 0x7c + (int)param_2->field_0x1e) * 4;
  *param_4 = param_1;
  return;
}
