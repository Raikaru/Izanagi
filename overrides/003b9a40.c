/* Retail 003b9a40 (code1_003b.c): fits the smoothed control points of a cubic spline object to its
 * input points `points` (3 floats each, count n = func_003bb330(object)). The spline type word at
 * +0xc selects the fit; the result is written to the point array at object+0x38 (3 floats per point,
 * first entry at +0x38). Returns the object, or 0 on error (reported through func_003df590 / 003df4d0,
 * record code 258).
 *  - type 1 (open): each output is the k-point weighted sum of the neighbouring inputs, k = min(n, 10), with
 *    the weights taken from the table D_008864C0[k] (k x k floats: the first k/2 and last k/2 outputs use
 *    their own table rows, every other output the middle row). The points land in outputs 1..n; output 0 is
 *    the vector at +0x50 (which is output 2: the output array starts at +0x38, so +0x44 is output 1 and +0x50
 *    output 2) and output n+1 repeats output n-1.
 *  - type 2 (closed): the kernel of the cyclic cubic spline [1 4 1] system: with k = min(n, 10) and h = k / 2
 *    the integer sequence x[i] = 4 x[i-1] - x[i-2] is built (seeds 1, 3 / 1, 5 for odd k, 1, 4 / 1, 2 for even
 *    k) into a kernel B and a divisor; B's odd positions below h are negated and every output is the divisor-
 *    normalised sum over the neighbouring input points (indices wrap modulo n). Outputs 1..n are filled,
 *    output 0 = output n, outputs n+1 / n+2 take the vectors at +0x44 and +0x50, that is outputs 1 and 2, so
 *    the curve closes.
 * Retail sums the products in an unrolled order; the port accumulates sequentially (results agree to float
 * rounding). Recurrence allocations retain retail's exact length * 4 request, including zero. Retail
 * still writes both seeds for lengths zero and one, relying on the allocator's minimum physical block.
 * A zero-length recurrence (k == 0 or k == -3) reads the chkmem back-pointer at payload-4 as its
 * divisor (003ba1e8 / 003b9ea8). The port keeps its established dlmalloc policy, whose header differs;
 * when n <= 0 the inverse is unused because no point is accumulated, so that header difference has
 * no observable effect here. Half lengths use signed division toward zero, as retail's branch/add/sra
 * sequences do, not an arithmetic shift (which rounds negative odd k down). Do not enlarge requests. */
extern int p4_spline_count(void *object) __asm__("func_003bb330");
extern void *(*p4_spline_malloc[])(int size, int flags) __asm__("D_008873E8");
extern void (*p4_spline_free[])(void *) __asm__("D_008873EC");
extern int p4_spline_error_code(unsigned int code, int size) __asm__("func_003df590");
extern void p4_spline_error_post(void *record) __asm__("func_003df4d0");
extern float *p4_spline_tables[] __asm__("D_008864C0");

static void p4_spline_report(unsigned int code, int size)
{
    unsigned int record[2];

    record[0] = 258;
    record[1] = (unsigned int)p4_spline_error_code(code, size);
    p4_spline_error_post(record);
}

/* The recurrence x[i] = 4 x[i-1] - x[i-2], with the retail allocation size and unconditional two seeds.
 * NULL (after reporting) when the allocation fails. */
static int *p4_spline_sequence(int length, int second, int report_size)
{
    int *x = (int *)p4_spline_malloc[0](length * 4, 0x30102);
    int i;

    if (x == 0) {
        p4_spline_report(0x80000013u, report_size);
        return 0;
    }
    x[0] = 1;
    x[1] = second;
    for (i = 2; i < length; i++) {
        x[i] = x[i - 1] * 4 - x[i - 2];
    }
    return x;
}

void *func_003b9a40(unsigned char *object, float *points)
{
    int n = p4_spline_count(object);
    float *out = (float *)(object + 0x38);
    int type = *(int *)(object + 0xc);
    int k = n > 9 ? 10 : n;
    int i, j;

    if (type != 2 && type != 1) {
        p4_spline_report(1u, 0);
        return 0;
    }
    if (type == 1) {
        float *table = p4_spline_tables[k];
        int half = k / 2;

        if (table == 0) {
            p4_spline_report(0x80000016u, 0);
            return 0;
        }
        for (i = 0; i < n; i++) {
            const float *weights;
            const float *source;
            float x = 0.0f, y = 0.0f, z = 0.0f;

            if (i < half) {
                weights = table + i * k;
                source = points;
            } else if (i < n - half) {
                weights = table + k * half;
                source = points + (i - half) * 3;
            } else {
                weights = table + k * (k - n + i);
                source = points + (n - k) * 3;
            }
            for (j = 0; j < k; j++) {
                x += source[j * 3 + 0] * weights[j];
                y += source[j * 3 + 1] * weights[j];
                z += source[j * 3 + 2] * weights[j];
            }
            out[(i + 1) * 3 + 0] = x;
            out[(i + 1) * 3 + 1] = y;
            out[(i + 1) * 3 + 2] = z;
        }
        *(unsigned int *)(out + 0) = *(unsigned int *)(object + 0x50);
        *(unsigned int *)(out + 1) = *(unsigned int *)(object + 0x54);
        *(unsigned int *)(out + 2) = *(unsigned int *)(object + 0x58);
        out[(n + 1) * 3 + 0] = out[(n - 1) * 3 + 0];
        out[(n + 1) * 3 + 1] = out[(n - 1) * 3 + 1];
        out[(n + 1) * 3 + 2] = out[(n - 1) * 3 + 2];
        return object;
    } else {
        int half = k / 2;
        int *kernel;
        int *divisor_table;
        int divisor;
        int kernel_size;
        float inverse;

        if ((k & 1) != 0) {
            kernel_size = half + 1;
            divisor_table = p4_spline_sequence(kernel_size, 3, k);
            if (divisor_table == 0) {
                return 0;
            }
            divisor = divisor_table[half];
            p4_spline_free[0](divisor_table);
            kernel = p4_spline_sequence(kernel_size, 5, k);
            if (kernel == 0) {
                return 0;
            }
        } else {
            divisor_table = p4_spline_sequence(half, 4, k);
            if (divisor_table == 0) {
                return 0;
            }
            divisor = divisor_table[half - 1];
            p4_spline_free[0](divisor_table);
            kernel_size = half + 1;
            kernel = p4_spline_sequence(kernel_size, 2, k);
            if (kernel == 0) {
                return 0;
            }
        }
        inverse = 1.0f / (float)divisor;
        for (j = half - 1; j >= 0; j -= 2) {
            kernel[j] = -kernel[j];
        }
        for (i = 0; i < n; i++) {
            const float *p = points + (i % n) * 3;
            float w = (float)kernel[half];
            float x = w * p[0], y = w * p[1], z = w * p[2];

            for (j = 1; j < half + 1; j++) {
                p = points + ((j + i) % n) * 3;
                w = (float)kernel[half - j];
                x = p[0] * w + x;
                y = p[1] * w + y;
                z = p[2] * w + z;
            }
            for (j = 1; j < ((k + 1) >> 1); j++) {
                p = points + ((n + i - j) % n) * 3;
                w = (float)kernel[half - j];
                x = p[0] * w + x;
                y = p[1] * w + y;
                z = p[2] * w + z;
            }
            out[(i + 1) * 3 + 0] = x * inverse;
            out[(i + 1) * 3 + 1] = y * inverse;
            out[(i + 1) * 3 + 2] = z * inverse;
        }
        out[0] = out[n * 3 + 0];
        out[1] = out[n * 3 + 1];
        out[2] = out[n * 3 + 2];
        *(unsigned int *)(out + (n + 1) * 3 + 0) = *(unsigned int *)(object + 0x44);
        *(unsigned int *)(out + (n + 1) * 3 + 1) = *(unsigned int *)(object + 0x48);
        *(unsigned int *)(out + (n + 1) * 3 + 2) = *(unsigned int *)(object + 0x4c);
        *(unsigned int *)(out + (n + 2) * 3 + 0) = *(unsigned int *)(object + 0x50);
        *(unsigned int *)(out + (n + 2) * 3 + 1) = *(unsigned int *)(object + 0x54);
        *(unsigned int *)(out + (n + 2) * 3 + 2) = *(unsigned int *)(object + 0x58);
        p4_spline_free[0](kernel);
        return object;
    }
}
