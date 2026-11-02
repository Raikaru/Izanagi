#ifndef P4PC_MOD_FIDELITY_H
#define P4PC_MOD_FIDELITY_H
/* Strict retail/reference assertion: refuses any MOD_WITNESS presence, even
 * with empty selectors, and any staged/published package. Mode-aware harness
 * starts use p4_mod_fidelity_check_start below, not a copied conditional.
 * Scalar-only header: no host package records enter the guest ABI census. */
void p4_packages_assert_disabled(void);
/* Explicit separate mod-test lane only. Unset=ordinary; the sole accepted
 * value is1 and it requires P4PC_INPUT=script. Never overrides selector0. */
int p4_mod_witness_validate(void);
int p4_mod_witness_enabled(void);
/* One policy for every native harness/fixture start. Invalid witness mode
 * fails; valid scripted MOD_WITNESS=1 skips the disabled assertion and remains
 * nonreference. Ordinary mode asserts BOTH selectors disabled as before. */
void p4_mod_fidelity_check_start(void);
/* Canonical automation fence shared by selectors/native failure UI. Explicit
 * config-file selection alone is not automation and must not suppress player UI. */
int p4_packages_automation_requested(void);
#endif
