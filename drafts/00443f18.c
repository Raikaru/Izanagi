/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00443f18 func_00443f18 src/middleware/middleware_inferred_00443068.c - romwright draft, unreviewed */

char * FUN_00443f18(char *param_1,char *param_2)

{
  char *pcVar1;
  char *pcVar2;
  unsigned int *puVar3;
  char cVar4;
  char *pcVar5;
  char cVar6;
  
  puVar3 = (unsigned int *)(DAT_00710070 + 0x5c);
  if (param_1 != (char *)0x0) {
    cVar4 = *param_1;
  }
  else {
    param_1 = (char *)*puVar3;
    if (param_1 == (char *)0x0) {
      return (char *)0x0;
    }
    cVar4 = *param_1;
  }
  do {
    cVar6 = *param_2;
    pcVar1 = param_1 + 1;
    pcVar5 = param_2;
    while( 1 ) {
      if (cVar6 == '\0') {
        if (cVar4 == '\0') {
          *puVar3 = 0;
          return (char *)0x0;
        }
        cVar4 = *pcVar1;
        do {
          pcVar2 = pcVar1 + 1;
          cVar6 = *param_2;
          pcVar5 = param_2;
          while( 1 ) {
            pcVar5 = pcVar5 + 1;
            if (cVar6 == cVar4) {
              if (cVar6 != '\0') {
                *pcVar1 = '\0';
              }
              else {
                pcVar2 = (char *)0x0;
              }
              *puVar3 = pcVar2;
              return param_1;
            }
            if (cVar6 == '\0') break;
            cVar6 = *pcVar5;
          }
          cVar4 = *pcVar2;
          pcVar1 = pcVar2;
        } while( 1 );
      }
      pcVar5 = pcVar5 + 1;
      if (cVar4 == cVar6) break;
      cVar6 = *pcVar5;
    }
    cVar4 = *pcVar1;
    param_1 = pcVar1;
  } while( 1 );
}
