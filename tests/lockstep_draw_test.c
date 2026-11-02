/* Exercise the actual production retained buffer without the game, SDL,
 * guest memory, a renderer, or object-format-dependent dead-section linking. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../src/platform/lockstep_draw.h"

int main(void)
{
    char line[1025];
    memset(line, 'x', sizeof line - 1);
    line[sizeof line - 1] = 0;
    P4LockstepDrawBuffer buffer = {0};
    for (unsigned i=0; i<5120; i++) {
        line[0]=(char)('a'+i%26);
        if (p4pc_lockstep_draw_append(&buffer,line)) return 1;
    }
    assert(buffer.length==5u*1024u*1024u);
    assert(buffer.capacity>=buffer.length);
    for (unsigned i=0; i<5120; i++) {
        line[0]=(char)('a'+i%26);
        assert(!memcmp(buffer.data+i*1024u,line,1024u));
    }
    char *before=buffer.data;
    size_t capacity=buffer.capacity;
    buffer.length=0;
    if (p4pc_lockstep_draw_append(&buffer,"next\n")) return 1;
    assert(buffer.data==before && buffer.capacity==capacity && buffer.length==5);
    assert(!memcmp(buffer.data,"next\n",5));
    free(buffer.data);
    return 0;
}
