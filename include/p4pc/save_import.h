/* Native PS2 save import; all paths are host paths, never guest pointers. */
#ifndef P4PC_SAVE_IMPORT_H
#define P4PC_SAVE_IMPORT_H
#include <stddef.h>
#define P4SAVE_MAX_FILES 64
#define P4SAVE_MAX_BYTES (8u * 1024u * 1024u)
typedef struct P4SaveFile {
    char name[33];
    unsigned char *data;
    size_t size;
    unsigned char created[8], modified[8];
} P4SaveFile;
typedef struct P4SaveArchive {
    char name[33];
    P4SaveFile files[P4SAVE_MAX_FILES];
    size_t count;
    unsigned char created[8], modified[8];
    /* Native folders only: separately bounded metadata, never charged against
     * or silently stripped into the PS2 container's64-file/8MiB payload. */
    P4SaveFile mod_sidecar,mod_commit;
    int witness; /* inherited non-reference custody, not a gameplay capability */
} P4SaveArchive;
void p4save_archive_free(P4SaveArchive *save);
/* Parse a single-save container. Does not access the filesystem. */
int p4save_decode(const unsigned char *blob, size_t size, P4SaveArchive *save, char *error, size_t cap);
/* Slot is zero-based (0..15), -1 keeps the original slot when free, otherwise
 * chooses the first empty slot. An explicit slot authorizes replacement.
 * card is the directory containing BASLUS-21782SaveN, not the save root. */
int p4save_import(const char *source, const char *card, int slot, char *message, size_t cap);
/* Return -1 for a non-import command, otherwise the process exit status. */
int p4save_import_cli(int argc, char **argv);
void p4save_import_settings_init(void);
/* SDL picker completion boundary: paths are copied, never imported on the
 * callback thread. One outstanding picker; consumed only while settings pause
 * the game. A NULL list reports a picker error, an empty list cancels. */
void p4save_import_dialog_result(void *userdata, const char *const *files, int filter);
#endif
