#ifndef P4_GOLDEN_CACHE_FIXTURE_H
#define P4_GOLDEN_CACHE_FIXTURE_H

#include <stdio.h>
#include <stdlib.h>

/* Valid preference fixtures must not be beneath a checkout. The production
 * cache enforces that rule even when the caller does not supply a repo path. */
static const char *golden_cache_fixture_temp(void)
{
#ifdef _WIN32
    const char *temp = getenv("TEMP");
    if (!temp || !*temp) {
        fputs("Golden cache fixture requires a nonempty TEMP outside the checkout\n", stderr);
        return NULL;
    }
    return temp;
#else
    const char *temp = getenv("TMPDIR");
    return temp && *temp ? temp : "/tmp";
#endif
}

#endif
