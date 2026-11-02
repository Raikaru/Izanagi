#ifndef P4PC_MOD_SCRIPT_H
#define P4PC_MOD_SCRIPT_H
#include <stdint.h>
/* Pass2: execute already-compiled chunks on the game thread after VFS mount,
 * before mod-aware game file loads. Pass1 admits/compiles every enabled source
 * before any mod runs. Runtime failures use controlled native recovery. */
void p4_mod_scripts_start(void);
/* Only after tasks/queues/callbacks quiesce, before package retirement. */
void p4_mod_scripts_stop(void);
/* Delivered native pad packet, after bindings/touch/input fences. Disabled
 * consumers leave this false; the pad owner checks it before sampling. */
extern int p4_mod_input_enabled;
void p4_mod_input_sample(const uint8_t packet[32], uint32_t frame);
#endif
