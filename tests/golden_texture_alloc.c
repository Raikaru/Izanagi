/* Standalone codec/qualification driver allocator, like model_cook_cli.c.
 * Production uses the existing resource_png.c shared budgeted allocator. */
#include <stdlib.h>
#include <stddef.h>
void *lodepng_malloc(size_t size) { return malloc(size); }
void *lodepng_realloc(void *pointer, size_t size) { return realloc(pointer,size); }
void lodepng_free(void *pointer) { free(pointer); }
