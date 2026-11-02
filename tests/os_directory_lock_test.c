#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned entries;
static void entry(const char *name,void *user)
{
    (void)user;if(strcmp(name,".")&&strcmp(name,".."))++entries;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    uintptr_t token[2]={0};
    if(p4_os_directory_lock(argv[2],token)){
        printf("lock-error=%d\n",errno);return errno==EBUSY?4:5;
    }
    if(p4_os_list_dir(argv[2],entry,NULL)||entries)return 6;
    if(!strcmp(argv[1],"hold")||!strcmp(argv[1],"crash")){
        puts("held");fflush(stdout);
        if(getchar()==EOF)return 7;
        if(!strcmp(argv[1],"crash"))_Exit(23); /* actual OS abandonment */
    }
    p4_os_directory_unlock(token);
    if(token[0]||token[1])return 8;
    puts("released");return 0;
}
