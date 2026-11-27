#ifndef P4PC_GLES_CONTEXT_EVIDENCE_H
#define P4PC_GLES_CONTEXT_EVIDENCE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The caller supplies a private, fresh path. Missing or malformed evidence
 * must be rejected by any authority that requires the renderer proof. */
static void p4pc_write_renderer_evidence(const char *renderer, int isES, int fallback)
{
    const char *path = getenv("P4PC_RENDERER_EVIDENCE");
    FILE *file;
    size_t length;
    if (path == NULL || path[0] == '\0' || renderer == NULL) return;
    length = strlen(renderer);
    if (length == 0 || length > 512) return;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)renderer[i];
        if (c < 0x20 || c > 0x7e) return;
    }
    file = fopen(path, "wx");
    if (file == NULL) return;
    fputs("{\"schema\":1,\"kind\":\"gl_context\",\"renderer\":\"", file);
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)renderer[i];
        if (c == '"' || c == '\\') fputc('\\', file);
        fputc(c, file);
    }
    fprintf(file, "\",\"is_es\":%s,\"fallback\":%s}\n", isES ? "true" : "false", fallback ? "true" : "false");
    fclose(file);
}

#endif
