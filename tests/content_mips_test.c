#include "p4content_mips.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); return 1; \
} } while (0)

static int reduction(void)
{
    static const uint8_t alpha_source[] = {
        255, 0, 0, 255, 0, 0, 255, 0,
        0, 255, 0, 128, 255, 255, 255, 0
    };
    static const uint8_t alpha_expected[] = {170, 85, 0, 96};
    static const uint8_t odd_source[] = {0, 0, 0, 255, 60, 0, 0, 255, 240, 0, 0, 255};
    static const uint8_t rational_expected[] = {
        /* Independent Python fractions.Fraction box integration, half-up rounding. */
        95, 86, 123, 104, 68, 105, 126, 119, 97, 134, 144, 133,
        150, 151, 138, 135, 144, 108, 145, 149, 88, 125, 162, 164
    };
    uint8_t source[60], output[64], before[64], padded[24];
    size_t i;
    CHECK(p4_content_rgba_reduce(alpha_source, 2, 2, (size_t)(2) * 4, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(memcmp(output, alpha_expected, 4) == 0);
    memset(padded, 0xa5, sizeof padded);
    memcpy(padded, alpha_source, 8);
    memcpy(padded + 12, alpha_source + 8, 8);
    CHECK(p4_content_rgba_reduce(padded, 2, 2, 12, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(memcmp(output, alpha_expected, 4) == 0);
    CHECK(p4_content_rgba_reduce(odd_source, 3, 1, (size_t)(3) * 4, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(output[0] == 100 && output[1] == 0 && output[2] == 0 && output[3] == 255);
    for (i = 0; i < 5; ++i) {
        source[i * 4] = (uint8_t)(i * 30);
        source[i * 4 + 1] = source[i * 4 + 2] = 0;
        source[i * 4 + 3] = 255;
    }
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 2, 1,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(output[0] == 24 && output[4] == 96 && output[3] == 255 && output[7] == 255);
    CHECK(p4_content_rgba_reduce(source, 1, 5, (size_t)(1) * 4, output, 1, 2,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(output[0] == 24 && output[4] == 96);
    for (i = 0; i < sizeof source; ++i) source[i] = (uint8_t)(i * 37 + 11);
    CHECK(p4_content_rgba_reduce(source, 5, 3, (size_t)(5) * 4, output, 3, 2,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(memcmp(output, rational_expected, sizeof rational_expected) == 0);
    memset(source, 255, sizeof source);
    for (i = 3; i < sizeof source; i += 4) source[i] = 0;
    CHECK(p4_content_rgba_reduce(source, 5, 3, (size_t)(5) * 4, output, 3, 2,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    for (i = 0; i < 24; ++i) CHECK(output[i] == 0);
    memset(output, 0xa5, sizeof output);
    memcpy(before, output, sizeof before);
    CHECK(p4_content_rgba_reduce(source, 5, 3, (size_t)(5) * 4, output, 6, 2,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 0, 3, (size_t)(0) * 4, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 16385, 3, (size_t)(16385) * 4, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 3, (size_t)(5) * 4, source, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 3, (size_t)(5) * 4, source + 1, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 3, 19, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 3, SIZE_MAX, output, 1, 1,
                                 NULL, 0, 0, 0, NULL, 0) == -1);
    CHECK(memcmp(output, before, sizeof output) == 0);
    return 0;
}

static int atlas(void)
{
    /* Reverse x order also exercises allocation-free sorting of cell spans. */
    P4ContentCell cells[3] = {{1, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}};
    P4ContentCell projected, reappears = {4, 0, 1, 1};
    uint8_t source[20], output[12], before[12];
    size_t workspace_size = p4_content_mip_workspace_size(3), i;
    uint8_t *workspace = (uint8_t *)malloc(workspace_size + 16);
    CHECK(workspace != NULL);
    CHECK(p4_content_atlas_validate(cells, 2, 2, 1) == 0);
    CHECK(p4_content_atlas_validate(cells, 3, 2, 1) == -1);
    CHECK(p4_content_atlas_validate(NULL, 1, 2, 1) == -1);
    CHECK(p4_content_atlas_validate(cells, P4_CONTENT_ATLAS_LIMIT + 1, 2, 1) == -1);
    CHECK(p4_content_mip_workspace_size(P4_CONTENT_ATLAS_LIMIT + 1) == 0);
    CHECK(p4_content_atlas_project(&cells[1], 2, 1, 5, 1, &projected) == 1);
    CHECK(projected.x == 0 && projected.width == 2 && projected.height == 1);
    CHECK(p4_content_atlas_project(&cells[0], 2, 1, 5, 1, &projected) == 1);
    CHECK(projected.x == 2 && projected.width == 3);
    CHECK(p4_content_atlas_max_level(&cells[1], 2, 1, 5, 1) == 1);
    CHECK(p4_content_atlas_max_level(&cells[0], 2, 1, 5, 1) == 2);
    CHECK(p4_content_atlas_max_level(&reappears, 8, 1, 8, 1) == 0);
    CHECK(p4_content_atlas_project(&reappears, 8, 1, 1, 1, &projected) == 1);
    cells[2].x = UINT32_MAX;
    CHECK(p4_content_atlas_project(&cells[2], 2, 1, 5, 1, &projected) == -1);
    CHECK(p4_content_atlas_max_level(&cells[2], 2, 1, 5, 1) == -1);
    for (i = 0; i < 5; ++i) {
        source[i * 4] = i < 2 ? 255 : 0;
        source[i * 4 + 1] = 0;
        source[i * 4 + 2] = i < 2 ? 0 : 255;
        source[i * 4 + 3] = 255;
    }
    memset(workspace, 0xa5, workspace_size + 16);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 cells, 2, 2, 1, workspace, workspace_size) == 0);
    CHECK(output[0] == 255 && output[2] == 0);
    CHECK(output[4] == 0 && output[6] == 255 && output[8] == 0 && output[10] == 255);
    for (i = workspace_size; i < workspace_size + 16; ++i) CHECK(workspace[i] == 0xa5);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 NULL, 0, 0, 0, NULL, 0) == 0);
    CHECK(output[4] == 51 && output[6] == 204); /* Ordinary filtering crosses the cell. */
    memset(output, 0xa5, sizeof output);
    memcpy(before, output, sizeof before);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 cells, 2, 2, 1, workspace, 0) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 cells, 2, 2, 1, workspace + 1, workspace_size) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 cells, 2, 0, 1, workspace, workspace_size) == -1);
    CHECK(p4_content_rgba_reduce(source, 5, 1, (size_t)(5) * 4, output, 3, 1,
                                 cells, 3, 2, 1, workspace, workspace_size) == -1);
    CHECK(memcmp(output, before, sizeof output) == 0);
    free(workspace);
    return 0;
}

static int atlas_rows(void)
{
    static const P4ContentCell cells[] = {
        {2, 2, 2, 2}, {0, 0, 2, 2}, {0, 2, 2, 2}, {2, 0, 2, 2}
    };
    static const uint8_t colors[] = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255
    };
    uint8_t source[64], output[16];
    size_t size = p4_content_mip_workspace_size(4);
    void *workspace = malloc(size);
    unsigned x, y;
    CHECK(workspace != NULL);
    CHECK(p4_content_atlas_validate(cells, 4, 4, 4) == 0);
    for (y = 0; y < 4; ++y) {
        for (x = 0; x < 4; ++x) {
            unsigned cell = (y / 2) * 2 + x / 2;
            memcpy(source + (y * 4 + x) * 4, colors + cell * 4, 4);
        }
    }
    CHECK(p4_content_rgba_reduce(source, 4, 4, (size_t)(4) * 4, output, 2, 2,
                                 cells, 4, 4, 4, workspace, size) == 0);
    CHECK(memcmp(output, colors, sizeof colors) == 0);
    free(workspace);
    return 0;
}

int main(void)
{
    if (reduction() != 0 || atlas() != 0 || atlas_rows() != 0) return 1;
    puts("replacement mip filtering and logical atlas bounds: OK");
    return 0;
}
