/* Boot-section gates (docs/modding/files.md "Settings").
 *
 * A native boot section (table-source, resource-image, element-ui) may carry
 * `when = <key>` naming a toggle its own mod declares with apply = restart.
 * The section takes part in this admission only while that toggle's admitted
 * value is "on"; off, the handler treats it exactly as absent, so the mod
 * contributes nothing for it. Section parsers accept the one `when` field and
 * leave its evaluation here. Handlers that gate must list "mod-settings" in
 * their dependencies so the admission's snapshot is staged first. */
#ifndef P4PC_MOD_SETTING_GATE_H
#define P4PC_MOD_SETTING_GATE_H
#include <stddef.h>
#include "p4pc/packages.h"

#define P4_MOD_SETTING_GATE_FIELD "when"
#define P4_MOD_SETTINGS_HANDLER "mod-settings"

/* fields/count: one section's fields. 1 participates (no gate, or the toggle
 * is on), 0 gated off, -1 refused with a bounded reason: a repeated or empty
 * `when`, an undeclared key, a choice setting or a live (apply = live) toggle. */
int p4_mod_setting_section_gate(const P4PackageProvider *provider, const P4PackageField *fields,
                                size_t count, char *reason, size_t capacity);
#endif
