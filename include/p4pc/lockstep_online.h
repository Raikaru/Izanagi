/* Lockstep-only online disc replay channel (src/platform/lockstep_online.c, docs/lockstep.md).
 *
 * tools/lockstep_disc.py OnlineRelay relays retail's live disc trace (p4hook
 * --lockstep-disc-online) into the file named by P4PC_LOCKSTEP_DISC_ONLINE, anchor-relative and in
 * retail's order: each game-level read retail requested after the initial anchor (D) with every
 * adxf_stat result on it while in flight (P), and each FilerRequest retail queued after it (F)
 * with every take of it (T), and each thunder bolt list retail created after it with its words (U).
 * The port's n-th read, request or list after the anchor is retail's n-th;
 * its k-th poll or take is answered by retail's k-th. Every wait blocks until the relay has
 * written the answer, and fails the run, naming what it waited for, after
 * P4PC_LOCKSTEP_ONLINE_TIMEOUT seconds (default 300). Nothing is ever released early. */
#ifndef P4PC_LOCKSTEP_ONLINE_H
#define P4PC_LOCKSTEP_ONLINE_H

#include <stddef.h>
#include <stdint.h>

#define P4ON_NAME 64

/* 1 when P4PC_LOCKSTEP_DISC_ONLINE names the channel (it is opened on first use), 0 when unset. */
int p4pc_online_configured(void);

/* Retail's game read `index`: its base name, handle-relative sector offset, sectors and
 * anchor-relative request update. */
void p4pc_online_read(uint32_t index, char name[P4ON_NAME], uint32_t *offset, uint32_t *sectors,
                      int32_t *request);
/* Retail's next adxf_stat result on game read `index`: the update it polled in and whether the
 * read was complete. */
void p4pc_online_read_poll(uint32_t index, int32_t *update, int *complete);

/* Retail's FilerRequest `index`: its key (include/p4pc/lockstep_filer.h) and the update it was
 * queued in. `key` holds P4LF_KEY bytes. */
void p4pc_online_filer(uint32_t index, char *key, int32_t *enqueue);
/* Retail's next take of FilerRequest `index`: its update and whether it found it complete. */
void p4pc_online_filer_take(uint32_t index, int32_t *update, int *complete);

/* Retail's thunder bolt list `index` (include/p4pc/lockstep_uninit.h): its creator's address, the
 * update it was created in, its copies and the list block's words. Returns the word count, at most
 * `capacity`. */
size_t p4pc_online_uninit(uint32_t index, uint32_t *creator, int32_t *update, uint32_t *copies, uint32_t *words,
                          size_t capacity);

/* Before the initial anchor (NG-7j). Retail's disc latency, event load time and movie schedule depend on
 * retail's own frame history, so no recorded value reproduces them: the port completes each wait exactly
 * when the live retail run does. The relay numbers, from the start of the run, every game-level read (r, with
 * its polls p), every event load request (v, polls w: evtLoaderIsIdle's answers) and every movie
 * (m, with the answers of its mwPlyGetCurFrm polls n and its status reads t). Polls are paired in order and
 * carry the update relative to their own request. */
void p4pc_online_pre_read(uint32_t index, char name[P4ON_NAME], uint32_t *offset, uint32_t *sectors,
                          int32_t *request);
void p4pc_online_pre_read_poll(uint32_t index, int32_t *relative, int *complete);
void p4pc_online_pre_event(uint32_t index, uint32_t id[3], int32_t *request);
void p4pc_online_pre_event_poll(uint32_t index, int32_t *relative, int *complete);
void p4pc_online_pre_movie(uint32_t index, char name[P4ON_NAME], int32_t *request);
/* Retail's next GetCurFrm answer for movie `index`: the frame number, or -1 for no frame. */
int32_t p4pc_online_pre_movie_poll(uint32_t index);
/* Retail's next status read for movie `index`. */
int32_t p4pc_online_pre_movie_status(uint32_t index);
/* Retail's FilerRequest `index` since the start of the run (x): its key and enqueue update, and each take of
 * it (y: the update relative to its enqueue and whether it found the request complete). */
void p4pc_online_pre_filer(uint32_t index, char *key, int32_t *enqueue);
void p4pc_online_pre_filer_take(uint32_t index, int32_t *relative, int *complete);

#endif
