#ifndef P4PC_SAVE_IMAGE_H
#define P4PC_SAVE_IMAGE_H
#include <stddef.h>
/* Full retail-format image including version word. No guest state/filesystem. */
int p4save_image_validate(const void *data, size_t size, char *error, size_t capacity);
#endif
