#ifndef P4PC_GL_COMPARISON_EVIDENCE_H
#define P4PC_GL_COMPARISON_EVIDENCE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* An observation from the comparison, not an authenticated terminal. The
 * supervisor must bind it to a fenced run and independently establish its
 * provenance; missing, invalid, or candidate-forged files are not verdicts. */
static void p4gl_comparison_evidence(int unavailable, int passed, double mad, const char *limit_text)
{
    const char *path = getenv("P4PC_COMPARISON_EVIDENCE");
    double limit = limit_text != NULL ? atof(limit_text) : 0.0;
    FILE *file;
    if (path == NULL || *path == '\0') return;
    if (!isfinite(mad) || (limit_text != NULL && !isfinite(limit))) unavailable = 1;
    file = fopen(path, "wb");
    if (file == NULL) return;
    fprintf(file, "{\"schema\":1,\"kind\":\"scene_comparison\",\"outcome\":\"%s\",\"reason_code\":\"%s\",",
            unavailable ? "infra_unavailable" : passed ? "pass" : "candidate_fail",
            unavailable ? "invalid_result" : passed ? "none" : "test_failed");
    if (!unavailable && mad >= 0.0) fprintf(file, "\"mad_percent\":\"%.17g\",", mad);
    else fputs("\"mad_percent\":null,", file);
    if (limit_text != NULL && isfinite(limit)) fprintf(file, "\"limit_percent\":\"%.17g\"}\n", limit);
    else fputs("\"limit_percent\":null}\n", file);
    fclose(file);
}

#endif
