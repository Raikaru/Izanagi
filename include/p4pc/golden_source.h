/* Read-only reader for the player's own Persona 4 Golden (Steam, x64) install.
 *
 * The Golden mechanics mod (mods/p4pc.golden, docs/modding/golden.md) derives
 * its battle tables at run time from the player's copy; no Golden byte is
 * shipped, cached or written anywhere. This header has two layers:
 *
 *  - Format readers over caller-owned buffers: CRI @UTF tables, CRILAYLA,
 *    the CPK table of contents, Golden's sequential .bin container and the
 *    packed-bank TBL spans. They allocate nothing (except the CPK reader's
 *    table of contents) and never trust a length they have not bounded.
 *  - The install reader: locate the install, read init_free.bin from
 *    data_e.cpk, check every battle table against the supported build's
 *    SHA-256 set and expose the spans translated to the PS2 layouts.
 *
 * Nothing here touches guest memory. */
#ifndef P4PC_GOLDEN_SOURCE_H
#define P4PC_GOLDEN_SOURCE_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- CRI @UTF tables -------------------------------------------------- */

#define P4_GOLDEN_UTF_MAX_COLUMNS 64u

typedef struct P4GoldenUtfColumn {
    uint32_t name;     /* offset into the string pool */
    uint8_t storage;   /* 0x10 zero, 0x30 constant, 0x50 per row */
    uint8_t type;      /* 0..9 numbers, 0x0A string, 0x0B data */
    uint32_t value;    /* constant: offset of its value; per row: offset in the row */
} P4GoldenUtfColumn;

typedef struct P4GoldenUtf {
    const uint8_t *table; /* starts after "@UTF" and the size word */
    uint32_t size;
    uint32_t rows_offset, strings_offset, data_offset;
    uint32_t row_width, row_count;
    uint32_t column_count;
    P4GoldenUtfColumn columns[P4_GOLDEN_UTF_MAX_COLUMNS];
} P4GoldenUtf;

/* Parses an @UTF packet in place. An obfuscated packet (not starting with
 * "@UTF") is unmasked in place first. 0, or -1 for a malformed table. */
int p4_golden_utf_open(P4GoldenUtf *utf, uint8_t *packet, size_t size);
/* Numeric cell (any integer width) of a named column. 0, or -1 when the
 * column is absent, not an integer, or the row is out of range. */
int p4_golden_utf_u64(const P4GoldenUtf *utf, uint32_t row, const char *column, uint64_t *out);
/* String cell; *out points into the table. NULL-valued columns give "". */
int p4_golden_utf_str(const P4GoldenUtf *utf, uint32_t row, const char *column, const char **out);

/* ---- CRILAYLA --------------------------------------------------------- */

/* Uncompressed size of a CRILAYLA stream (header + 0x100-byte prefix), or 0
 * when src is not a well-formed stream. */
size_t p4_golden_crilayla_size(const uint8_t *src, size_t size);
/* Decompresses into out (capacity at least p4_golden_crilayla_size). 0, or -1
 * for any out-of-range back reference or truncated bit stream. */
int p4_golden_crilayla(const uint8_t *src, size_t size, uint8_t *out, size_t capacity);

/* ---- CPK archives ----------------------------------------------------- */

typedef struct P4GoldenCpkEntry {
    uint64_t offset;  /* absolute file offset */
    uint32_t size;    /* stored bytes */
    uint32_t extract; /* uncompressed bytes */
} P4GoldenCpkEntry;

typedef struct P4GoldenCpk {
    FILE *file;
    uint8_t *toc;     /* owned, unmasked TOC packet */
    P4GoldenUtf utf;
    uint64_t base;    /* added to FileOffset */
} P4GoldenCpk;

/* Opens and parses the header and table of contents. 0 or -1 (reason set). */
int p4_golden_cpk_open(P4GoldenCpk *cpk, const char *path, char *reason, size_t capacity);
/* Case-insensitive "dir/name" lookup. 0 found, 1 absent, -1 malformed TOC. */
int p4_golden_cpk_find(const P4GoldenCpk *cpk, const char *path, P4GoldenCpkEntry *out);
/* Reads and, when compressed, decompresses one entry into a malloc'ed buffer
 * owned by the caller. 0 or -1 (reason set). */
int p4_golden_cpk_read(P4GoldenCpk *cpk, const P4GoldenCpkEntry *entry,
                       uint8_t **out, size_t *size, char *reason, size_t capacity);
void p4_golden_cpk_close(P4GoldenCpk *cpk);

/* ---- Golden .bin containers and TBL spans ------------------------------ */

/* Sequential records: 252-byte NUL-padded name, u32 size, payload padded to
 * 64 bytes. Case-insensitive name lookup. 0 found, 1 absent, -1 malformed. */
int p4_golden_bin_find(const uint8_t *bin, size_t size, const char *name,
                       const uint8_t **payload, size_t *payload_size);

typedef struct P4GoldenSpan {
    const uint8_t *bytes;
    size_t size;
} P4GoldenSpan;

/* Packed-bank spans: u32 byte length, payload, next record aligned to 16.
 * Returns the span count (at most max), or -1 for a truncated span. */
int p4_golden_tbl_spans(const uint8_t *tbl, size_t size, P4GoldenSpan *spans, size_t max);

/* ---- The supported install -------------------------------------------- */

/* Battle tables the reader extracts, in this order. */
enum {
    P4_GOLDEN_SKILL, P4_GOLDEN_UNIT, P4_GOLDEN_PERSONA, P4_GOLDEN_MSG,
    P4_GOLDEN_ENCOUNT, P4_GOLDEN_ITEM, P4_GOLDEN_TABLE_COUNT
};

typedef struct P4GoldenTable {
    uint8_t *bytes;          /* owned copy of the file */
    size_t size;
    uint8_t digest[32];      /* SHA-256 of bytes */
    P4GoldenSpan spans[8];   /* packed-bank spans (ITEM: the raw file) */
    int span_count;
} P4GoldenTable;

typedef struct P4GoldenSource {
    char directory[1024];
    P4GoldenTable tables[P4_GOLDEN_TABLE_COUNT];
} P4GoldenSource;

/* Finds the install: an explicit directory (the `golden.install` setting or
 * P4PC_GOLDEN) when given, else the Steam libraries listed in each standard
 * libraryfolders.vdf. 0 with out filled, 1 not found. */
int p4_golden_locate(const char *configured, char *out, size_t capacity);
/* Reads data_e.cpk/init_free.bin from the install directory, keeps copies of
 * the battle tables and checks them against the supported build. 0 loaded,
 * 1 no install there, -1 unreadable, malformed or an unsupported build
 * (reason set). On failure *source holds nothing. */
int p4_golden_load(P4GoldenSource *source, const char *directory, char *reason, size_t capacity);
void p4_golden_free(P4GoldenSource *source);
/* Name of a table for diagnostics and fingerprints ("SKILL.TBL" ...). */
const char *p4_golden_table_name(int table);

#ifdef __cplusplus
}
#endif
#endif
