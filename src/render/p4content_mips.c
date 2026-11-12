#include "p4content_mips.h"

#include <stdint.h>

/* One reusable, caller-budgeted array. Heapsort needs no libc sort workspace. */
typedef struct MipCell {
    P4ContentCell source, output;
} MipCell;

static int dimensions(uint32_t width, uint32_t height)
{
    return width != 0 && height != 0 && width <= P4_CONTENT_IMAGE_LIMIT &&
           height <= P4_CONTENT_IMAGE_LIMIT;
}

static int contained(const P4ContentCell *cell, uint32_t width, uint32_t height)
{
    return cell != NULL && cell->width != 0 && cell->height != 0 &&
           cell->x < width && cell->width <= width - cell->x &&
           cell->y < height && cell->height <= height - cell->y;
}

int p4_content_atlas_validate(const P4ContentCell *cells, size_t count,
                              uint32_t width, uint32_t height)
{
    size_t i, j;
    if (!dimensions(width, height) || count > P4_CONTENT_ATLAS_LIMIT ||
        (count != 0 && cells == NULL)) return -1;
    for (i = 0; i < count; ++i) {
        if (!contained(&cells[i], width, height)) return -1;
        for (j = 0; j < i; ++j) {
            if (cells[i].x < cells[j].x + cells[j].width &&
                cells[j].x < cells[i].x + cells[i].width &&
                cells[i].y < cells[j].y + cells[j].height &&
                cells[j].y < cells[i].y + cells[i].height) return -1;
        }
    }
    return 0;
}

static uint32_t project_edge(uint32_t edge, uint32_t logical, uint32_t image)
{
    /* ceil(edge * image / logical - 1/2), including edge == 0. */
    return (uint32_t)(((uint64_t)edge * image * 2u + logical - 1u) /
                      ((uint64_t)logical * 2u));
}

int p4_content_atlas_project(const P4ContentCell *cell,
                             uint32_t logical_width, uint32_t logical_height,
                             uint32_t width, uint32_t height,
                             P4ContentCell *out)
{
    P4ContentCell result;
    if (out == NULL || !dimensions(logical_width, logical_height) ||
        !dimensions(width, height) ||
        !contained(cell, logical_width, logical_height)) return -1;
    result.x = project_edge(cell->x, logical_width, width);
    result.y = project_edge(cell->y, logical_height, height);
    result.width = project_edge(cell->x + cell->width, logical_width, width) - result.x;
    result.height = project_edge(cell->y + cell->height, logical_height, height) - result.y;
    *out = result;
    return result.width != 0 && result.height != 0;
}

int p4_content_atlas_max_level(const P4ContentCell *cell,
                               uint32_t logical_width, uint32_t logical_height,
                               uint32_t width, uint32_t height)
{
    P4ContentCell projected;
    int level = 0;
    if (p4_content_atlas_project(cell, logical_width, logical_height,
                                 width, height, &projected) != 1) return -1;
    while (width > 1 || height > 1) {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        if (p4_content_atlas_project(cell, logical_width, logical_height,
                                     width, height, &projected) != 1) break;
        ++level;
    }
    return level;
}

size_t p4_content_mip_workspace_size(size_t count)
{
    return count <= P4_CONTENT_ATLAS_LIMIT ? count * sizeof(MipCell) : 0;
}

static int cell_less(const MipCell *a, const MipCell *b)
{
    return a->output.x < b->output.x ||
           (a->output.x == b->output.x && a->output.y < b->output.y);
}

static void sift(MipCell *cells, size_t root, size_t count)
{
    MipCell value = cells[root];
    while (root < count / 2) {
        size_t child = root * 2 + 1;
        if (child + 1 < count && cell_less(&cells[child], &cells[child + 1])) ++child;
        if (!cell_less(&value, &cells[child])) break;
        cells[root] = cells[child];
        root = child;
    }
    cells[root] = value;
}

static void sort_cells(MipCell *cells, size_t count)
{
    size_t i;
    if (count < 2) return;
    for (i = count / 2; i != 0; --i) sift(cells, i - 1, count);
    for (i = count - 1; i != 0; --i) {
        MipCell temporary = cells[0];
        cells[0] = cells[i];
        cells[i] = temporary;
        sift(cells, 0, i);
    }
}

static uint32_t clamp_coordinate(uint32_t value, uint32_t start, uint32_t extent)
{
    if (value < start) return start;
    if (value - start >= extent) return start + extent - 1;
    return value;
}

static uint64_t minimum(uint64_t a, uint64_t b) { return a < b ? a : b; }
static uint64_t maximum(uint64_t a, uint64_t b) { return a > b ? a : b; }

static void reduce_span(const uint8_t *source, uint32_t width, uint32_t height,
                        size_t source_stride, uint8_t *output,
                        uint32_t out_width, uint32_t out_height,
                        uint32_t row, uint32_t start, uint32_t end,
                        const P4ContentCell *bounds)
{
    uint32_t x;
    uint64_t top = (uint64_t)row * height, bottom = top + height;
    uint32_t first_y = (uint32_t)(top / out_height);
    uint32_t last_y = (uint32_t)((bottom + out_height - 1) / out_height);
    uint64_t area = (uint64_t)width * height;
    for (x = start; x < end; ++x) {
        uint64_t left = (uint64_t)x * width, right = left + width;
        uint32_t first_x = (uint32_t)(left / out_width);
        uint32_t last_x = (uint32_t)((right + out_width - 1) / out_width);
        uint64_t alpha = 0, red = 0, green = 0, blue = 0;
        uint32_t sy, sx;
        uint8_t *pixel = output + ((size_t)row * out_width + x) * 4;
        for (sy = first_y; sy < last_y; ++sy) {
            uint64_t y_weight = minimum(bottom, (uint64_t)(sy + 1) * out_height) -
                                maximum(top, (uint64_t)sy * out_height);
            uint32_t sample_y = clamp_coordinate(sy, bounds->y, bounds->height);
            for (sx = first_x; sx < last_x; ++sx) {
                uint64_t x_weight = minimum(right, (uint64_t)(sx + 1) * out_width) -
                                    maximum(left, (uint64_t)sx * out_width);
                uint32_t sample_x = clamp_coordinate(sx, bounds->x, bounds->width);
                const uint8_t *p = source + (size_t)sample_y * source_stride + sample_x * 4;
                uint64_t a = x_weight * y_weight * p[3];
                alpha += a;
                red += a * p[0];
                green += a * p[1];
                blue += a * p[2];
            }
        }
        pixel[3] = (uint8_t)((alpha + area / 2) / area);
        if (pixel[3] != 0) {
            pixel[0] = (uint8_t)((red + alpha / 2) / alpha);
            pixel[1] = (uint8_t)((green + alpha / 2) / alpha);
            pixel[2] = (uint8_t)((blue + alpha / 2) / alpha);
        } else {
            pixel[0] = pixel[1] = pixel[2] = 0;
        }
    }
}

static int disjoint(const void *a, size_t a_size, const void *b, size_t b_size)
{
    uintptr_t left = (uintptr_t)a, right = (uintptr_t)b;
    return left <= right ? right - left >= a_size : left - right >= b_size;
}

int p4_content_rgba_reduce(const uint8_t *source, uint32_t width,
                            uint32_t height, size_t source_stride, uint8_t *output,
                            uint32_t out_width, uint32_t out_height,
                            const P4ContentCell *cells, size_t count,
                            uint32_t logical_width, uint32_t logical_height,
                            void *workspace, size_t workspace_size)
{
    MipCell *projected = (MipCell *)workspace;
    P4ContentCell whole;
    size_t source_bytes, output_bytes, used = 0, i;
    uint32_t row;
    if (source == NULL || output == NULL || !dimensions(width, height) ||
        !dimensions(out_width, out_height) || out_width > width || out_height > height ||
        source_stride < (size_t)width * 4 || source_stride > SIZE_MAX / height ||
        count > P4_CONTENT_ATLAS_LIMIT ||
        (count != 0 && (cells == NULL || !dimensions(logical_width, logical_height) ||
         workspace == NULL || ((uintptr_t)workspace & (_Alignof(MipCell) - 1)) != 0 ||
         workspace_size < p4_content_mip_workspace_size(count)))) return -1;
    source_bytes = source_stride * height;
    output_bytes = (size_t)out_width * out_height * 4;
    if (!disjoint(source, source_bytes, output, output_bytes) ||
        (count != 0 && (!disjoint(workspace, workspace_size, source, source_bytes) ||
                       !disjoint(workspace, workspace_size, output, output_bytes) ||
                       !disjoint(workspace, workspace_size, cells, count * sizeof(*cells)) ||
                       !disjoint(cells, count * sizeof(*cells), source, source_bytes) ||
                       !disjoint(cells, count * sizeof(*cells), output, output_bytes)))) return -1;
    /* Finish all contract validation before touching caller workspace/output. */
    for (i = 0; i < count; ++i) {
        if (!contained(&cells[i], logical_width, logical_height)) return -1;
    }
    for (i = 0; i < count; ++i) {
        MipCell cell;
        if (p4_content_atlas_project(&cells[i], logical_width, logical_height,
                                     width, height, &cell.source) != 1 ||
            p4_content_atlas_project(&cells[i], logical_width, logical_height,
                                     out_width, out_height, &cell.output) != 1) continue;
        projected[used++] = cell;
    }
    sort_cells(projected, used);
    whole.x = whole.y = 0;
    whole.width = width;
    whole.height = height;
    for (row = 0; row < out_height; ++row) {
        uint32_t x = 0;
        for (i = 0; i < used; ++i) {
            const MipCell *cell = &projected[i];
            if (row < cell->output.y || row - cell->output.y >= cell->output.height) continue;
            reduce_span(source, width, height, source_stride, output, out_width, out_height,
                         row, x, cell->output.x, &whole);
            reduce_span(source, width, height, source_stride, output, out_width, out_height,
                         row, cell->output.x, cell->output.x + cell->output.width, &cell->source);
            x = cell->output.x + cell->output.width;
        }
        reduce_span(source, width, height, source_stride, output, out_width, out_height,
                     row, x, out_width, &whole);
    }
    return 0;
}
