#ifndef P4PC_MOD_EVENTS_H
#define P4PC_MOD_EVENTS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/mod_entry.h"
typedef struct P4ModEventField { const char *name; int64_t value; } P4ModEventField;
typedef struct P4ModEvent {
    const char *name;
    size_t count;
    P4ModEventField fields[8];
} P4ModEvent;
typedef struct P4ModEventSubscription P4ModEventSubscription;
typedef int (*P4ModEventFn)(const P4ModEvent *event, void *user);
P4ModEventSubscription *p4_mod_event_subscribe(size_t owner, const char *name, P4ModEventFn callback, void *user);
int p4_mod_event_enable(P4ModEventSubscription *subscription, int enabled);
int p4_mod_event_remove(P4ModEventSubscription *subscription);
/* Callback-stack state only; queued native work needs its own owner proof.
 * Bulk removal requires idle callbacks. clear frees handles and belongs after
 * their clients/VMs have been released, not merely after disabling callbacks. */
int p4_mod_events_quiescent(void);
void p4_mod_events_remove_owner(size_t owner);
void p4_mod_events_clear(void);
/* Save-codec publication notices run at the frame safe point through this
 * owner-installed function; it returns nonzero while notices remain queued. */
void p4_mod_events_set_published(int (*notify)(void));
/* Mod game services (mod_services.h) run their queued requests and lease
 * checks at the same safe point, before frame_start subscribers, while
 * P4_MOD_SAFE_SERVICES is set; the pump returns nonzero while work remains. */
void p4_mod_events_set_services(int (*pump)(uint32_t frame));
/* Queued party-record writes (p4.party.set) apply at the frame safe point; the
 * callback returns nonzero while writes remain (one waiting out a battle). */
void p4_mod_events_set_party(int (*apply)(void));
#endif
