/* Port session identity and crash markers for mods (p4.lifecycle,
 * docs/modding/lua.md "Lifecycle").
 *
 * The first request allocates this process's session: a durable counter in
 * <settings dir>/mod-lifecycle/session (written, then atomically replaced) and
 * an empty crash marker crash-<session>.marker, opened before any fault. Only
 * the port's crash handler fills that marker (include/p4pc/os.h), with the
 * session and each mod's current tag; a clean exit, a force-kill or power loss
 * leave it empty. A later launch deletes the empty markers of other sessions
 * and keeps at most P4_MOD_LIFECYCLE_KEPT recorded crashes until their mods
 * consume them. Without a settings directory there is no session. */
#ifndef P4PC_MOD_LIFECYCLE_H
#define P4PC_MOD_LIFECYCLE_H

#include <stddef.h>
#include <stdint.h>

#define P4_MOD_LIFECYCLE_TAG_MAX 63 /* printable ASCII bytes */
#define P4_MOD_LIFECYCLE_TAGGED 8   /* mods with a tag at once */
#define P4_MOD_LIFECYCLE_KEPT 16    /* recorded crash markers kept */

/* Settings directory, or NULL; resets the session state (tests, reconfiguration). */
int p4_mod_lifecycle_root(const char *directory);
/* This process's session (>= 1), allocated on first use; 0 with a reason when
 * there is no durable settings directory or a write failed. */
uint64_t p4_mod_lifecycle_session(char *reason, size_t capacity);
/* The tag the crash handler records for this mod (NULL clears). 0, or -1 with
 * a reason (no session, invalid tag, too many tagged mods). */
int p4_mod_lifecycle_tag(const char *mod_id, const char *tag, char *reason, size_t capacity);
/* Another session's record: 1 the crash handler recorded a crash (`tag` gets
 * this mod's tag then, "" when it had none), 0 no crash recorded, -1 an
 * unreadable or mismatched marker (reason). The current session is never crashed. */
int p4_mod_lifecycle_crashed(uint64_t session, const char *mod_id, char *tag, size_t tag_capacity,
                             char *reason, size_t capacity);
/* Drops this mod's record from another session's marker once the mod has
 * durably reconciled it; the marker goes with its last record. */
int p4_mod_lifecycle_consume(uint64_t session, const char *mod_id, char *reason, size_t capacity);

#endif
