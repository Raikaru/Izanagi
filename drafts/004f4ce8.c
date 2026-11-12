/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4ce8 func_004f4ce8 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f4ce8(unsigned long long param_1,long param_2)

{
  unsigned int uVar1;
  unsigned int uVar2;
  int *piVar3;
  int iVar4;
  unsigned long long *puVar5;
  unsigned long long uVar6;
  long lVar7;
  unsigned long long uVar8;
  unsigned long long uVar9;
  int *piVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int *piVar14;
  unsigned long long extraout_t3;
  unsigned long long extraout_t3_00;
  unsigned long long uVar15;
  unsigned long long in_t4;
  int iVar16;
  int *piVar17;
  int aiStack_30 [4];
  
  if (param_2 == 0) {
    uVar6 = FUN_004f53c0(0x75d6c8,0x727,0x75d7e0,0,0xffffffffffffff9c);
  }
  else {
    lVar7 = FUN_004f2e80(param_1,1);
    if (lVar7 == 0) {
      uVar6 = FUN_004f53c0(0x75d6c8,0x72d,0x75d798,0,0xffffffffffffff97);
    }
    else {
      FUN_004f5cd0(0,0,aiStack_30);
      iVar16 = (int)lVar7;
      piVar17 = (int *)param_2;
      if (aiStack_30[0] == 1) {
        *(int **)(iVar16 + 0x10) = piVar17;
        uVar6 = 0;
      }
      else {
        lVar7 = FUN_004426e8(0x75d7d0,piVar17 + 3);
        uVar15 = extraout_t3;
        if ((lVar7 != 0) &&
           (lVar7 = FUN_004426e8(iVar16 + 0x16,piVar17 + 3), uVar15 = extraout_t3_00, lVar7 != 0)) {
          uVar6 = FUN_004f53c0(0x75d6c8,0x73d,0x75d7e0,0,0xffffffffffffff94);
        }
        else {
          piVar3 = *(int **)(iVar16 + 0x10);
          piVar14 = piVar17 + 6;
          iVar16 = *piVar17;
          iVar4 = piVar3[1];
          piVar10 = piVar3 + 6;
          uVar9 = (unsigned long long)piVar17[2];
          uVar8 = (unsigned long long)(iVar16 < iVar4);
          iVar11 = iVar16;
          if (uVar8 == 0) {
            iVar11 = iVar4;
          }
          piVar3[2] = piVar17[2];
          iVar13 = 0;
          iVar12 = iVar11;
          if (0 < iVar11) {
            do {
              uVar1 = ((int)piVar14 + 7U) & 7;
              uVar2 = (unsigned int)piVar14 & 7;
              uVar8 = (((*(long *)(((int)piVar14 + 7U) - uVar1) << (7 - uVar1) * 8) |
                       (uVar8 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                      (-1L << (8 - uVar2) * 8)) | (*(unsigned long long *)((int)piVar14 - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar14 + 0xfU) & 7;
              uVar2 = (unsigned int)(piVar14 + 2) & 7;
              uVar9 = (((*(long *)(((int)piVar14 + 0xfU) - uVar1) << (7 - uVar1) * 8) |
                       (uVar9 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                      (-1L << (8 - uVar2) * 8)) |
                      (*(unsigned long long *)((int)(piVar14 + 2) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar14 + 0x17U) & 7;
              uVar2 = (unsigned int)(piVar14 + 4) & 7;
              uVar15 = (((*(long *)(((int)piVar14 + 0x17U) - uVar1) << (7 - uVar1) * 8) |
                        (uVar15 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                       (-1L << (8 - uVar2) * 8)) |
                       (*(unsigned long long *)((int)(piVar14 + 4) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar14 + 0x1fU) & 7;
              uVar2 = (unsigned int)(piVar14 + 6) & 7;
              in_t4 = (((*(long *)(((int)piVar14 + 0x1fU) - uVar1) << (7 - uVar1) * 8) |
                       (in_t4 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                      (-1L << (8 - uVar2) * 8)) |
                      (*(unsigned long long *)((int)(piVar14 + 6) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar10 + 7U) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 7U) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (uVar8 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)piVar10 & 7;
              *(unsigned long long *)((int)piVar10 - uVar1) =
                   (uVar8 << uVar1 * 8) |
                   (*(unsigned long long *)((int)piVar10 - uVar1) & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              uVar1 = ((int)piVar10 + 0xfU) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 0xfU) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (uVar9 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(piVar10 + 2) & 7;
              puVar5 = (unsigned long long *)((int)(piVar10 + 2) - uVar1);
              *puVar5 = (uVar9 << uVar1 * 8) | (*puVar5 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              uVar1 = ((int)piVar10 + 0x17U) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 0x17U) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (uVar15 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(piVar10 + 4) & 7;
              puVar5 = (unsigned long long *)((int)(piVar10 + 4) - uVar1);
              *puVar5 = (uVar15 << uVar1 * 8) | (*puVar5 & (0xffffffffffffffffU >> (8 - uVar1) * 8))
              ;
              uVar1 = ((int)piVar10 + 0x1fU) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 0x1fU) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (in_t4 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(piVar10 + 6) & 7;
              puVar5 = (unsigned long long *)((int)(piVar10 + 6) - uVar1);
              *puVar5 = (in_t4 << uVar1 * 8) | (*puVar5 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              uVar1 = ((int)piVar14 + 0x27U) & 7;
              uVar2 = (unsigned int)(piVar14 + 8) & 7;
              uVar8 = (((*(long *)(((int)piVar14 + 0x27U) - uVar1) << (7 - uVar1) * 8) |
                       (uVar8 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                      (-1L << (8 - uVar2) * 8)) |
                      (*(unsigned long long *)((int)(piVar14 + 8) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar14 + 0x2fU) & 7;
              uVar2 = (unsigned int)(piVar14 + 10) & 7;
              uVar9 = (((*(long *)(((int)piVar14 + 0x2fU) - uVar1) << (7 - uVar1) * 8) |
                       (uVar9 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                      (-1L << (8 - uVar2) * 8)) |
                      (*(unsigned long long *)((int)(piVar14 + 10) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)piVar10 + 0x27U) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 0x27U) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (uVar8 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(piVar10 + 8) & 7;
              puVar5 = (unsigned long long *)((int)(piVar10 + 8) - uVar1);
              *puVar5 = (uVar8 << uVar1 * 8) | (*puVar5 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              uVar1 = ((int)piVar10 + 0x2fU) & 7;
              puVar5 = (unsigned long long *)(((int)piVar10 + 0x2fU) - uVar1);
              *puVar5 = (*puVar5 & (-1L << (uVar1 + 1) * 8)) | (uVar9 >> (7 - uVar1) * 8);
              iVar12 = iVar12 - 1;
              uVar1 = (unsigned int)(piVar10 + 10) & 7;
              puVar5 = (unsigned long long *)((int)(piVar10 + 10) - uVar1);
              *puVar5 = (uVar9 << uVar1 * 8) | (*puVar5 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              piVar10 = piVar10 + 0xc;
              piVar14 = piVar14 + 0xc;
              iVar13 = iVar11;
            } while (iVar12 != 0);
          }
          uVar6 = 0xffffffffffffff93;
          *piVar3 = iVar13;
          if (iVar16 <= iVar4) {
            uVar6 = 0;
          }
        }
      }
    }
  }
  return uVar6;
}
