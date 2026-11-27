/* The PS2 path cannot allocate, read Golden files or reach RW via the Golden seam. */
#include "p4pc/golden_model.h"
#include "p4pc/game_mode.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    char error[256]="";
    assert(p4_game_mode()==P4_GAME_MODE_PS2);
    assert(!p4_golden_model_build(NULL,error,sizeof error));
    assert(strstr(error,"Golden-only"));
    error[0]=0;
    assert(!p4_golden_model_load("not-a-file.AMD",error,sizeof error));
    assert(strstr(error,"Golden-only"));
    p4_golden_model_destroy(NULL);
    int owner=0, child=0;
    p4_golden_model_draw(&owner);
    assert(p4_golden_model_attach(&owner,0,500,&child,error,sizeof error));
    assert(strstr(error,"owned Golden"));
    const uint8_t row[10]={0};
    assert(p4_golden_model_play(&owner,0,31,row,0,1,error,sizeof error));
    assert(p4_golden_model_play_physical(&owner,0,34,0,1,error,sizeof error));
    puts("PS2 Golden model source isolation passed");
    return 0;
}
