#ifndef P4PC_PACKAGE_SORT_H
#define P4PC_PACKAGE_SORT_H
#include <stddef.h>
/* libc qsort may allocate a second full array. Metadata sorting must stay
 * within the reserved peak on every CRT, so use an in-place, bounded-stack heap. */
static void p4_package_sort_swap(unsigned char *restrict a, unsigned char *restrict b, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        unsigned char value = a[i];
        a[i] = b[i];
        b[i] = value;
    }
}
static void p4_package_sort_sift(unsigned char *base, size_t root, size_t count,
                                  size_t size, int (*compare)(const void *, const void *))
{
    while (root < count / 2) {
        size_t child = root * 2 + 1;
        if (child + 1 < count && compare(base + child * size, base + (child + 1) * size) < 0)
            ++child;
        if (compare(base + root * size, base + child * size) >= 0) return;
        p4_package_sort_swap(base + root * size, base + child * size, size);
        root = child;
    }
}
static void p4_package_sort(void *array, size_t count, size_t size,
                            int (*compare)(const void *, const void *))
{
    if (count < 2) return;
    unsigned char *base = array;
    for (size_t root = count / 2; root != 0;) {
        --root;
        p4_package_sort_sift(base, root, count, size, compare);
    }
    for (size_t end = count; end > 1; --end) {
        p4_package_sort_swap(base, base + (end - 1) * size, size);
        p4_package_sort_sift(base, 0, end - 1, size, compare);
    }
}
#endif
