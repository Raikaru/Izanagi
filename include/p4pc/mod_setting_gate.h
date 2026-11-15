/* Boot-section gates (docs/modding/files.md "Settings").
 *
 * A native boot section (table-source, resource-image, element-ui) may carry
 * `when = <key>` naming a toggle its own mod declares with apply = restart.
 * The section takes part in this admission only while that toggle's admitted
 * value is "on"; off, the handler treats it exactly as absent, so the mod
 * contributes nothing for it. Handlers that gate must list "mod-settings" in
 * their dependencies so the admission's snapshot is staged first.
 *
 * It may also carry `companion = <mod id>` (capability
 * manifest.companion-setting@1) naming one of the mod's own [companion]
 * declarations; it ANDs with `when`. The section takes part only while that
 * companion is in the admission (p4_packages_companion_admitted),
 * and is absent otherwise. `companion-setting = <key>=<value>` additionally
 * requires the admitted companion's restart setting to hold that declared value
 * (read from the same admission snapshot as `when`; not read while the
 * companion is absent). Section parsers accept one each of these fields and
 * leave their evaluation here. */
#ifndef P4PC_MOD_SETTING_GATE_H
#define P4PC_MOD_SETTING_GATE_H
#include <stddef.h>
#include "p4pc/packages.h"

#define P4_MOD_SETTING_GATE_FIELD "when"
#define P4_MOD_COMPANION_GATE_FIELD "companion"
#define P4_MOD_COMPANION_SETTING_FIELD "companion-setting"
#define P4_MOD_SETTINGS_HANDLER "mod-settings"

/* The gate's declared rules alone, with no admitted value (shared with p4mod
 * check): fields/count one section, manifest/all every field of the mod. 0 no
 * gate; 1 a valid gate, *gate its key (borrowed from fields) and *default_on
 * (optional) the toggle's declared default; -1 refused, with the reason
 * p4_mod_setting_section_gate gives. */
int p4_mod_setting_gate_declared(const P4PackageField *fields, size_t count, const P4PackageField *manifest,
                                 size_t all, const char **gate, int *default_on, char *reason, size_t capacity);
/* fields/count: one section's fields. 1 participates (no gate, or the toggle
 * is on and the companion admitted with its setting), 0 gated off, -1 refused
 * with a bounded reason: a repeated or empty `when`, an undeclared key, a
 * choice setting or a live (apply = live) toggle; a repeated or empty
 * `companion`, or one the mod does not declare; a repeated
 * `companion-setting`, one without `companion`, or one that is not a declared
 * restart value of the companion. `when` is evaluated first, then `companion`;
 * each keeps its own reason. */
int p4_mod_setting_section_gate(const P4PackageProvider *provider, const P4PackageField *fields,
                                size_t count, char *reason, size_t capacity);
#endif
