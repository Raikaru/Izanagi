#ifndef P4PC_GAME_TABLE_INPUTS_H
#define P4PC_GAME_TABLE_INPUTS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/packages.h"
#include "p4pc/vfs_source.h"
/* Actual native descriptor labels; order is not a package or save namespace. */
typedef enum P4GameTableInputKind {
    P4_TABLE_INPUT_ITEM, P4_TABLE_INPUT_SKILL, P4_TABLE_INPUT_UNIT,
    P4_TABLE_INPUT_MSG, P4_TABLE_INPUT_PERSONA, P4_TABLE_INPUT_ENCOUNT,
    P4_TABLE_INPUT_EFFECT, P4_TABLE_INPUT_MODEL, P4_TABLE_INPUT_AICALC,
    P4_TABLE_INPUT_COUNT
} P4GameTableInputKind;
typedef struct P4GameTableInput P4GameTableInput;
typedef struct P4GameTableInputSpan P4GameTableInputSpan;
const char *p4_game_table_input_label(P4GameTableInputKind kind);
/* Early CPU preparation only. Same actual selected precedence as native VFS:
 * developer, highest selected package replacement, disc. No MOD VFS fallback,
 * root walk, late reopen or owner-string authority. *out must start NULL.
 * 0 prepared,1 absent,-1 invalid/changed/I/O. failed_provider is a borrowed
 * actual current winning provider on package failure, NULL for native input. */
int p4_game_table_input_prepare(P4GameTableInputKind kind, uint64_t max_bytes,
    P4GameTableInput **out, const P4PackageProvider **failed_provider);
void p4_game_table_input_destroy(P4GameTableInput *input);
const uint8_t *p4_game_table_input_bytes(const P4GameTableInput *input);
size_t p4_game_table_input_size(const P4GameTableInput *input);
P4GameTableInputKind p4_game_table_input_kind(const P4GameTableInput *input);
const P4PackageProvider *p4_game_table_input_provider(const P4GameTableInput *input);
/* NULL for package input; otherwise the exact retained DISC/DEVELOPER plan. */
const P4VfsSourceInfo *p4_game_table_input_native(const P4GameTableInput *input);
/* Build actual native packed-bank spans once (u32 byte length then payload,
 * next record aligned16), or the raw whole ITEMTBL span. Reject all bounds or
 * unexplained trailing bytes before native exposure. No guest addresses. */
int p4_game_table_input_seal(P4GameTableInput *input);
size_t p4_game_table_input_span_count(const P4GameTableInput *input);
const P4GameTableInputSpan *p4_game_table_input_span(const P4GameTableInput *input, size_t index);
const uint8_t *p4_game_table_input_span_bytes(const P4GameTableInputSpan *span);
size_t p4_game_table_input_span_size(const P4GameTableInputSpan *span);
uint64_t p4_game_table_input_span_offset(const P4GameTableInputSpan *span);
/* Issued sealed spans only; owner is the actual input object, not a forged
 * bank tuple. NULL/invalid phase returns NULL/SIZE_MAX respectively. */
const P4GameTableInput *p4_game_table_input_span_owner(const P4GameTableInputSpan *span);
size_t p4_game_table_input_span_index(const P4GameTableInputSpan *span);
int p4_game_table_input_owns_span(const P4GameTableInput *input, const void *identity);
/* Complete source bytes are fingerprinted only after successful selected-plan
 * read and sealing. Equal bytes are attributed to that retained plan/provider;
 * this does not assert which later read produced the guest buffer. No semantic
 * use/touch, allocation or I/O occurs during comparison. */
int p4_game_table_input_matches(const P4GameTableInput *input, const void *bytes, size_t size);
/* The returned span object IS this producer's immutable exact byte-span
 * identity. Input outlives all compiled products and native bindings. This is
 * CPU source authority, not a native loaded-byte or gameplay certificate. */
#endif
