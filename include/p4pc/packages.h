#ifndef P4PC_PACKAGES_H
#define P4PC_PACKAGES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/mod_fidelity.h"
#include "p4pc/package_memory.h"
#define P4_PACKAGE_API_MAJOR 1
#define P4_PACKAGE_API_MINOR 1
#define P4_PACKAGE_MAX 64 /* per independent selector */

typedef enum P4PackageKind { P4_PACKAGE_MOD = 1, P4_PACKAGE_ENGINE_PACK = 2 } P4PackageKind;
enum P4PackageClaims {
    P4_PACKAGE_CLAIM_FILES = 1u, P4_PACKAGE_CLAIM_SCRIPT = 2u,
    P4_PACKAGE_CLAIM_SEMANTIC = 4u, P4_PACKAGE_CLAIM_COSMETIC = 8u,
    P4_PACKAGE_CLAIM_NATIVE = 16u, P4_PACKAGE_CLAIM_TABLES = 32u
};
typedef struct P4PackageInfo {
    char id[64], name[128], version[64];
    char directory[1200];
    int priority, order;
    unsigned api_major, api_minor;
    P4PackageKind kind;
    size_t ordinal; /* global package order; NEVER a VFS layer rank */
    uint32_t generation;
} P4PackageInfo;
typedef struct P4PackageField {
    const char *section, *argument, *key, *value;
    unsigned line;
} P4PackageField;
typedef struct P4PackageRegistry P4PackageRegistry;
typedef struct P4PackageProvider P4PackageProvider;
typedef struct P4PackageFile P4PackageFile;
typedef struct P4PackageFilePlan P4PackageFilePlan;

/* Native selector policy, not a code/plugin loader. Strings/callbacks remain
 * alive for the process. The common implementation owns all manifest parsing,
 * indexing, selection, bounds and ordering. prepare may reject; rollback must
 * release even partially prepared resources. publish is infallible and must
 * neither allocate nor expose a partially built generation. */
typedef struct P4PackageClient {
    const char *diagnostic_name, *manifest_name, *section;
    P4PackageKind kind;
    int compendium, mount_files;
    int (*prepare)(const P4PackageRegistry *registry, void *user);
    void (*publish)(const P4PackageRegistry *registry, void *user);
    void (*rollback)(void *user);
    void *user;
    /* Admission is before any script or native game work. Refusal0 quarantines
     * this id and continues; NULL/nonzero keeps strict atomic failure. */
    int (*refuse)(const char *id, const char *reason, void *user);
    /* Compile/validate only. May retain compiled state across admission passes;
     * new immutable candidates replace refused snapshots; old pins stay valid. */
    int (*preflight)(const P4PackageRegistry *registry, void *user);
    void (*preflight_rollback)(void *user);
} P4PackageClient;
/* On-demand metadata-only installation discovery, including disabled packages.
 * Pointers are borrowed for this callback only; copy rows retained by Config.
 * info is NULL for invalid/unrecognized metadata, with diagnostic/line.
 * Valid metadata is not payload/decoder/activation qualification. No handlers,
 * VFS publication, payload walk, VM creation or hash tracking run here. */
typedef struct P4PackageInstallation {
    const char *folder, *directory;
    const P4PackageInfo *info;
    const P4PackageField *fields;
    size_t field_count;
    const char *diagnostic;
    unsigned line;
} P4PackageInstallation;
typedef int (*P4PackageInstallationVisitor)(const P4PackageInstallation *installation, void *user);
int p4_package_installations_scan(const P4PackageClient *client, const char *root,
                                  P4PackageInstallationVisitor visitor, void *user);

typedef enum P4PackageBootPhase {
    P4_PACKAGE_BOOT_SCHEMA = 0,
    P4_PACKAGE_BOOT_TABLES = 100,
    P4_PACKAGE_BOOT_RESOURCES = 200,
    P4_PACKAGE_BOOT_UI = 300
} P4PackageBootPhase;
/* Each extension section has one native handler. prepare sees the complete
 * globally sorted catalog and immutable parsed fields, never reopens a manifest.
 * Extension fields require API1.1. Register before publish, not from scripts.
 * All prepare completes before any publish. Dependency order is deterministic;
 * rollback reverses that order, including partially prepared providers. */
typedef struct P4PackageHandler {
    const char *name;
    const char *const *sections; /* NULL-terminated, or NULL for composition only */
    int (*prepare)(void *user);
    void (*publish)(void *user);
    void (*rollback)(void *user);
    void *user;
    /* Descriptor/capability validation only: no worker I/O or game work.
     * rollback also owns partial preflight staging. Mandatory payload checks
     * remain in prepare; refusal renews an immutable candidate after rollback. */
    int (*preflight)(void *user);
    /* Nonzero boot phases always participate, including an explicit inactive
     * result when no authored input requests expansion. Dependencies are native
     * handler names, NULL-terminated; no backwards phase edges are allowed. */
    P4PackageBootPhase boot_phase;
    const char *const *dependencies;
    /* Early CPU-only join/validation after every prepare, before files publish.
     * Neither early callback may require guest/RW/GPU state or queue game work. */
    int (*finalize_prepare)(void *user);
    /* Same dependency order at the real0010d480 boundary. Use ONLY retained
     * validated CPU inputs: no re-read/decode or pre-code quarantine here.
     * All runtime prepares/finalizers finish before any runtime publication.
     * Failure preserves partial/native resources for the controlled restart;
     * early rollback is NEVER called after raw-file exposure. */
    int (*runtime_prepare)(void *user);
    int (*runtime_finalize_prepare)(void *user);
    void (*runtime_publish)(void *user);
} P4PackageHandler;
int p4_package_handler_register(const P4PackageHandler *handler);
/* Prepare/finalize-only, game-thread borrowed product exchange. Products are
 * separate for early CPU and late runtime passes; there is NO cross-pass
 * fallback. Provide once per pass (NULL explicitly inactive); get0 may return
 * NULL. Get only declared dependencies. Owners retain the validated CPU input
 * and pins for their runtime callback; no re-read/decode or active-state lookup.
 * Invalid use fails the current callback even if it ignores -1. */
int p4_package_boot_provide(const void *product);
int p4_package_boot_product(const char *dependency, const void **out);
/* Runtime callback only. Attribute failure to the actual selected provider,
 * or NULL for an unattributable native service error. Copies the reason and
 * returns -1 for direct callback return. Never quarantines/retires anything. */
int p4_package_boot_fail(const P4PackageProvider *provider, const char *reason);
/* Explicit demand for the canonical verified guest-byte source dispatcher.
 * Actual native prepares with staged table rows/usable texture entries request
 * it; package count, Lua presence and music-only file reads imply nothing.
 * CPU-qualified native inputs request it before early file publication.
 * The existing source owner seals once, before game file consumers, and enables
 * its ONE observer for (dump requested || returned demand). No allocation/hash
 * work is performed by these helpers. Requests after seal/retirement fail. */
int p4_packages_source_tracking_require(void);
int p4_packages_source_tracking_required(void);
int p4_packages_source_tracking_seal(void);
/* Startup staging, before VFS mount. *out must be NULL. Empty enabled ids do
 * not allocate/read directories. Staging never publishes files/resources.
 * Call publish once after BOTH independently configured selectors are staged.
 * Opted-in admission refuses invalid providers before one atomic publication;
 * strict/unhandled failures still publish no partial batch. */
int p4_package_registry_load(P4PackageRegistry **out, const P4PackageClient *client,
                             const char *root, const char *enabled);
/* Optional explicit order for a staged selector. Unlisted ids retain their
 * default relative order after listed ids; applied only at publication. */
int p4_package_registry_set_order(P4PackageRegistry *registry, const char *ids);
int p4_package_registry_publish(void);
/* Only after game tasks/queues and script callbacks quiesce. Outstanding file
 * handles/provider pins retain their old generation through retirement. */
void p4_package_registry_retire(void);
size_t p4_package_registry_count(const P4PackageRegistry *registry);
const P4PackageInfo *p4_package_registry_info(const P4PackageRegistry *registry, size_t index);
size_t p4_package_registry_file_conflicts(const P4PackageRegistry *registry);
/* Catalog accessors work during handler prepare and after publication. */
size_t p4_packages_count(void);
const P4PackageInfo *p4_packages_info(size_t ordinal);
const P4PackageProvider *p4_packages_provider(size_t ordinal);
const P4PackageProvider *p4_package_registry_provider(const P4PackageRegistry *registry, size_t index);
const P4PackageField *p4_package_provider_fields(const P4PackageProvider *provider, size_t *count);
const P4PackageInfo *p4_package_provider_info(const P4PackageProvider *provider);
/* Game-thread authority query:1 ONLY for the exact current, unrefused admission
 * candidate or published member;0 for old/retired/not-yet-selected providers.
 * info()/same_source() alone never establish current selection authority. */
int p4_package_provider_selected(const P4PackageProvider *provider);
/* Already-validated immutable inventory; enumeration never walks/stat()s again.
 * Keys retain original casing. Retain the provider across asynchronous use. */
size_t p4_package_provider_files_count(const P4PackageProvider *provider);
int p4_package_provider_file(const P4PackageProvider *provider, size_t position,
                              const char **relative, int *directory);
/* Same root-inventory position: 0=exposed replacement, 1=not exposed, -1=bad
 * position. This is the actual VFS projection, including Compendium wrappers. */
int p4_package_provider_replacement(const P4PackageProvider *provider, size_t position,
                                     const char **key, int *directory);
/* Lifetime pin only, before handing metadata to an asynchronous worker.
 * This is not authority to open files after retirement; use an issued file plan.
 * Retired/refused providers cannot acquire new lifetime/work references. */
int p4_package_provider_retain(const P4PackageProvider *provider);
void p4_package_provider_release(const P4PackageProvider *provider);
/* Immutable inventory/manifest backing identity, across admission candidates.
 * Does not authorize new work on an old/retired provider. */
int p4_package_provider_same_source(const P4PackageProvider *left, const P4PackageProvider *right);
int p4_package_provider_has_file(const P4PackageProvider *provider, const char *relative);
/* Native prepare only. Files/scripts are derived automatically; native owners
 * report their actual semantic/table/cosmetic claims. Cosmetic-only is checked
 * after EVERY handler prepared, so an earlier label cannot hide later claims.
 * These are capabilities, not a claim that a particular save was touched. */
int p4_package_provider_claim(const P4PackageProvider *provider, uint32_t claims);
/* Thread-safe capability snapshot; immutable identity metadata has no changing
 * capability field for worker/read observers to race during preparation. */
uint32_t p4_package_provider_claims(const P4PackageProvider *provider);
/* Before publication only. Reports a concrete refused provider through its
 * selector's admission policy;0 means quarantined, -1 means strict/refused
 * operation. Native prepare must return after refusal, not publish that data.
 * Raw files and typed resources are removed together before the final pass. */
int p4_package_provider_refuse(const P4PackageProvider *provider, const char *reason);
int p4_package_provider_require_cosmetic(const P4PackageProvider *provider);
/* Only indexed contained regular files open. 0=open, 1=absent, -1=refused/I/O.
 * max_bytes is a caller's actual parser/decoder bound, checked before exposure.
 * Each handle owns a generation pin and one stream (not concurrently shared). */
int p4_package_file_open(const P4PackageProvider *provider, const char *relative,
                         uint64_t max_bytes, P4PackageFile **out);
uint64_t p4_package_file_size(const P4PackageFile *file);
int p4_package_file_read_at(P4PackageFile *file, uint64_t offset, void *bytes, size_t size);
int p4_package_file_close(P4PackageFile *file);
/* Prepare/game thread only: issue delayed-open authority from an actual bounded
 * indexed file while its provider is live. Captures its immutable snapshot,
 * exact relative key, bound and OS object/revision; no extra stream or payload
 * read.0/-1, *out startsNULL. Caller owns the file; an identical plan may be reused. */
int p4_package_file_plan_acquire(P4PackageFile *file, P4PackageFilePlan **out);
/* Thread-safe clone/release of ONLY this already-issued file lineage, including
 * on workers after provider retirement. Retain is allocation-free; callers own
 * a reference across every operation. This does not authorize fresh acquisition. */
int p4_package_file_plan_retain(P4PackageFilePlan *plan);
void p4_package_file_plan_release(P4PackageFilePlan *plan);
/* Workers may open independent streams for the same old snapshot/key/bound,
 * read/close them and release their plan references after provider retirement.
 * No VFS/current-provider lookup or game-thread I/O is involved. Revalidates the
 * contained regular file and original object/revision; never adopts a replacement.
 *0/-1. Each stream has one owner; fresh retired-provider acquisition still fails. */
int p4_package_file_plan_open(P4PackageFilePlan *plan, P4PackageFile **out);
typedef struct P4PackageFileOrigin {
    const P4PackageInfo *package;
    const char *relative; /* original-cased immutable key, never a host path */
} P4PackageFileOrigin;
/* Exact bytes actually read, with a pinned package identity/generation/kind.
 * This is not a collapsed VFS kind or an inferred provider string. Runs inline
 * on the reading thread, including workers: callbacks must be worker-safe and
 * must not enter Lua, VFS or game state. Exclusive registration/removal happens
 * before readers start or after they quiesce; removal requires the matching pair. */
typedef void (*P4PackageReadObserver)(const P4PackageFileOrigin *origin,
    const void *bytes, size_t size, uint64_t offset, void *user);
int p4_package_reads_observe(P4PackageReadObserver observer, void *user);
int p4_package_reads_unobserve(P4PackageReadObserver observer, void *user);
#endif
