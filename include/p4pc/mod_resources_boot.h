#ifndef P4PC_MOD_RESOURCES_BOOT_H
#define P4PC_MOD_RESOURCES_BOOT_H
#include "p4pc/mod_resources.h"
#include "p4pc/packages.h"

/* UI300 dependencies use this exact Resources200 provider name. Its runtime
 * product is a borrowed const P4Resources *, or NULL when no images exist. */
#define P4_RESOURCES_IMAGES_HANDLER "game-resources.images"
extern const P4PackageHandler p4_resources_images_boot_provider;

/* In the early pass the product is a borrowed CPU-only prepared view, NULL
 * only when no image declarations are active. It contains no RW/GPU tokens.
 * Valid through early finalization; never retain it across candidate rollback.
 * Lookup: 1 exact match, 0 absent/revision mismatch, -1 invalid arguments. */
typedef struct P4ResourceImagesPrepared P4ResourceImagesPrepared;
int p4_resources_images_prepared_find(const P4ResourceImagesPrepared *prepared,
                                      const char *owner, const char *key,
                                      uint32_t revision);
size_t p4_resources_images_prepared_count(const P4ResourceImagesPrepared *prepared);

/* Game/context thread only, after the core has independently drained tasks,
 * queues and native consumers and UI has released every resource pin. Refuses
 * while pins still exist. Never call this as startup rollback after exposure. */
int p4_resources_images_session_end(void);
#endif
