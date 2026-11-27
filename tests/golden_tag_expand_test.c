#include "p4pc/golden_tag_expand.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    static const uint8_t name[] = "12345678901234567890";
    static const int8_t offsets[4] = {4, 4, 4, 4};
    uint8_t text[128] = "<P1>-<P1>-<P1>-<P1>";
    assert(p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(strlen((char *)text) == 83);
    assert(!strcmp((char *)text,
        "12345678901234567890-12345678901234567890-12345678901234567890-12345678901234567890"));
    strcpy((char *)text, "<P1><P1><P1><P1><P1><P1><P1>");
    uint8_t saved[128]; strcpy((char *)saved, (char *)text);
    assert(!p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(!strcmp((char *)text, (char *)saved));
    strcpy((char *)text, "<P1> + <P2>");
    assert(p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(strlen((char *)text) == 43);
    strcpy((char *)text, "<P2> + <P1>");
    assert(p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(!strcmp((char *)text, "<P2> + 12345678901234567890"));
    memset(text, 'x', 127); text[127] = 0;
    assert(p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(strlen((char *)text) == 127);
    text[0] = 0x81; text[1] = '<'; memcpy(text + 2, "<P1>", 5);
    assert(p4_golden_expand_name_tags(text, name, name, name, name, offsets));
    assert(text[0] == 0x81 && text[1] == '<' && strlen((char *)text) == 22);
    return 0;
}
