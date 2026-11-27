#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/render/p4gl_comparison_evidence.h"

static int check(const char *path, int unavailable, int passed, double mad, const char *limit, const char *expected)
{
    char data[512];
    FILE *f;
    size_t n;
    p4gl_comparison_evidence(unavailable, passed, mad, limit);
    f = fopen(path, "rb");
    if (f == NULL) return 0;
    n = fread(data, 1, sizeof data - 1, f);
    data[n] = '\0';
    if (fclose(f) != 0) return 0;
    return strstr(data, expected) != NULL;
}

int main(int argc, char **argv)
{
    int ok;
    if (argc != 2) return 2;
#ifdef _WIN32
    if (_putenv_s("P4PC_COMPARISON_EVIDENCE", argv[1]) != 0) return 2;
#else
    if (setenv("P4PC_COMPARISON_EVIDENCE", argv[1], 1) != 0) return 2;
#endif
    ok = check(argv[1], 0, 1, 0.5, "1.0", "\"outcome\":\"pass\"") &&
         check(argv[1], 0, 0, 2.0, "1.0", "\"outcome\":\"candidate_fail\"") &&
         check(argv[1], 1, 0, -1.0, "1.0", "\"outcome\":\"infra_unavailable\"") &&
         check(argv[1], 0, 1, NAN, "1.0", "\"outcome\":\"infra_unavailable\"");
    remove(argv[1]);
    return ok ? 0 : 1;
}
