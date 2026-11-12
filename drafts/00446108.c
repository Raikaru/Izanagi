/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00446108 func_00446108 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

int FUN_00446108(unsigned int param_1,unsigned long long param_2,char *param_3,unsigned int *param_4)

{
  unsigned short *puVar1;
  long *plVar2;
  unsigned char bVar3;
  unsigned char bVar4;
  unsigned char bVar5;
  unsigned char bVar6;
  int iVar7;
  unsigned char *pbVar8;
  long lVar9;
  char *pcVar10;
  unsigned int uVar11;
  unsigned int *puVar12;
  long lVar13;
  unsigned char auStack_2e0 [608];
  unsigned int uStack_80;
  unsigned int *puStack_7c;
  int iStack_78;
  code *pcStack_74;
  
  lVar13 = 0;
  pcStack_74 = (code *)0x0;
  iStack_78 = 0;
  uStack_80 = param_1;
  puStack_7c = param_4;
LAB_00446154:
  lVar9 = (long)*param_3;
  pcVar10 = param_3;
code_r0x00446158:
  do {
    param_3 = pcVar10 + 1;
    if (lVar9 == 0) {
      return iStack_78;
    }
    puVar12 = (unsigned int *)param_2;
    if ((*(unsigned char *)((int)lVar9 + 0x754d88) & 8) != 0) {
      do {
        if ((int)puVar12[1] >= 1) {
          pbVar8 = (unsigned char *)*puVar12;
        }
        else {
          lVar9 = FUN_00441b40(param_2);
          if (lVar9 != 0) {
            lVar9 = (long)*param_3;
            pcVar10 = param_3;
            goto code_r0x00446158;
          }
          pbVar8 = (unsigned char *)*puVar12;
        }
        if ((*(unsigned char *)(*pbVar8 + 0x754d88) & 8) == 0) goto LAB_00446154;
        *puVar12 = pbVar8 + 1;
        lVar13 = (long)((int)lVar13 + 1);
        puVar12[1] = puVar12[1] - 1;
      } while( 1 );
    }
    if (lVar9 == 0x25) {
      bVar6 = 0;
      bVar5 = 0;
      bVar4 = 0;
      pcVar10 = param_3;
code_r0x004461f4:
      bVar3 = 0;
      param_3 = pcVar10 + 1;
      switch(*pcVar10) {
      case '\0':
        return -1;
      default:
        pcStack_74 = FUN_00443fd0;
        uVar11 = 3;
code_r0x00446300:
        if (((int)puVar12[1] < 1) && (lVar9 = FUN_00441b40(param_2), lVar9 != 0))
        goto code_r0x004462b8;
        if ((!bVar3) && (pbVar8 = (unsigned char *)*puVar12, (*(unsigned char *)(*pbVar8 + 0x754d88) & 8) != 0)) {
          iVar7 = puVar12[1];
          while( 1 ) {
            lVar13 = (long)((int)lVar13 + 1);
            puVar12[1] = iVar7 - 1;
            if (iVar7 - 1 >= 1) {
              *puVar12 = pbVar8 + 1;
              pbVar8 = (unsigned char *)*puVar12;
            }
            else {
              lVar9 = FUN_00441b40(param_2);
              if (lVar9 != 0) goto code_r0x004462b8;
              pbVar8 = (unsigned char *)*puVar12;
            }
            if ((*(unsigned char *)(*pbVar8 + 0x754d88) & 8) == 0) break;
            iVar7 = puVar12[1];
          }
        }
        if (uVar11 >= 5) {
          lVar9 = (long)*param_3;
          pcVar10 = param_3;
          continue;
        }
                    /* WARNING: Could not recover jumptable at 0x004463a4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
        iVar7 = (**(code **)(uVar11 * 4 + 0x755584))();
        return iVar7;
      case '%':
        goto code_r0x00446224;
      case '*':
        bVar4 = 1;
        pcVar10 = param_3;
        goto code_r0x004461f4;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        pcVar10 = param_3;
        goto code_r0x004461f4;
      case 'D':
      case 'd':
        pcStack_74 = FUN_00443fd0;
code_r0x004462f8:
        bVar3 = 0;
        uVar11 = 3;
        goto code_r0x00446300;
      case 'E':
      case 'G':
      case 'e':
      case 'f':
      case 'g':
        uVar11 = 4;
        goto code_r0x00446300;
      case 'L':
        pcVar10 = param_3;
        goto code_r0x004461f4;
      case 'O':
      case 'o':
        pcStack_74 = FUN_00444240;
        break;
      case 'X':
      case 'x':
        pcStack_74 = FUN_00444240;
        break;
      case '[':
        param_3 = (char *)FUN_00446d98(auStack_2e0,param_3);
        bVar3 = 1;
        uVar11 = 1;
        goto code_r0x00446300;
      case 'c':
        bVar3 = 1;
        uVar11 = 0;
        goto code_r0x00446300;
      case 'h':
        bVar5 = 1;
        pcVar10 = param_3;
        goto code_r0x004461f4;
      case 'i':
        pcStack_74 = FUN_00443fd0;
        break;
      case 'l':
        bVar6 = 1;
        pcVar10 = param_3;
        goto code_r0x004461f4;
      case 'n':
        if (bVar4) {
          lVar9 = (long)*param_3;
          pcVar10 = param_3;
          continue;
        }
        if (!bVar5) {
          if (!bVar6) {
            puVar12 = (unsigned int *)*puStack_7c;
            puStack_7c = puStack_7c + 2;
            *puVar12 = (int)lVar13;
            lVar9 = (long)*param_3;
            pcVar10 = param_3;
            continue;
          }
          plVar2 = (long *)*puStack_7c;
          puStack_7c = puStack_7c + 2;
          *plVar2 = lVar13;
          lVar9 = (long)*param_3;
          pcVar10 = param_3;
          continue;
        }
        puVar1 = (unsigned short *)*puStack_7c;
        puStack_7c = puStack_7c + 2;
        *puVar1 = (short)lVar13;
        lVar9 = (long)*param_3;
        pcVar10 = param_3;
        continue;
      case 'p':
        pcStack_74 = FUN_00444240;
        break;
      case 's':
        uVar11 = 2;
        goto code_r0x00446300;
      case 'u':
        pcStack_74 = FUN_00444240;
        goto code_r0x004462f8;
      }
      uVar11 = 3;
      goto code_r0x00446300;
    }
code_r0x00446224:
    iVar7 = 0;
    do {
      if ((int)puVar12[1] >= 1) {
        pbVar8 = (unsigned char *)*puVar12;
      }
      else {
        lVar9 = FUN_00441b40(param_2);
        if (lVar9 != 0) {
code_r0x004462b8:
          iVar7 = -1;
          if (iStack_78 != 0) {
            iVar7 = iStack_78;
          }
          return iVar7;
        }
        pbVar8 = (unsigned char *)*puVar12;
      }
      if ((unsigned long long)*pbVar8 != (long)*pcVar10) {
        return iStack_78;
      }
      *puVar12 = pbVar8 + 1;
      lVar13 = (long)((int)lVar13 + 1);
      pcVar10 = pcVar10 + 1;
      iVar7 = iVar7 + 1;
      puVar12[1] = puVar12[1] - 1;
    } while (iVar7 < 1);
    lVar9 = (long)*param_3;
    pcVar10 = param_3;
  } while( 1 );
}
