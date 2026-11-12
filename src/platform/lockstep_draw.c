#include "lockstep_draw.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *p4pc_lockstep_draw_append(P4LockstepDrawBuffer *buffer, const char *line)
{
    size_t length = strlen(line);
    size_t wanted = buffer->length + length;
    if (wanted < buffer->length) return "draw log size overflow";
    if (!buffer->data || wanted > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 4u * 1024u * 1024u;
        while (capacity < wanted) capacity = capacity > SIZE_MAX / 2 ? wanted : capacity * 2;
        char *grown = realloc(buffer->data, capacity);
        if (!grown) return "allocating draw log";
        buffer->data = grown;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->length, line, length);
    buffer->length = wanted;
    return NULL;
}
