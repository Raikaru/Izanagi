/* Golden-only full-width name access; PS2 name banks retain their retail layout. */
#ifndef P4PC_GOLDEN_NAMES_NATIVE_H
#define P4PC_GOLDEN_NAMES_NATIVE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define P4_GOLDEN_SKILL_NAME_SIZE 23u
#define P4_GOLDEN_PERSONA_NAME_SIZE 21u
/* Return a guest address into the currently published, bounded Golden name
 * bank, or zero in PS2 mode, before publication, or for an invalid ID. */
uint32_t p4_golden_skill_name(uint32_t id);
uint32_t p4_golden_persona_name(uint32_t id);
#ifdef __cplusplus
}
#endif
#endif
