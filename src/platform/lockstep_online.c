/* Lockstep-only online disc replay channel (include/p4pc/lockstep_online.h, docs/lockstep.md).
 * The relay appends whole lines; a partial line at the end of the file is retried, never parsed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "p4pc/lockstep_filer.h"
#include "p4pc/lockstep_online.h"

#ifdef _WIN32
#include <windows.h>
static void nap(void) { Sleep(1); }
#else
static void nap(void) {
    const struct timespec pause = {0, 1000000};
    nanosleep(&pause, NULL);
}
#endif

typedef struct {
    int32_t update;
    int complete;
} OnlinePoll;

typedef struct {
    int known;
    char name[P4ON_NAME];
    char *key;
    uint32_t offset, sectors, ids[3];
    int32_t request;
    OnlinePoll *polls;
    size_t count, capacity, next;
} OnlineRequest;

typedef struct {
    OnlineRequest *items;
    size_t count;
} OnlineList;

typedef struct {
    uint32_t creator, copies, *words;
    int32_t update;
    size_t count;
} OnlineUninit;

static FILE *sChannel;
static OnlineList sReads, sFiler, sPreReads, sPreEvents, sPreMovies, sPreStatus, sPreFiler;
static OnlineUninit *sUninit;
static size_t sUninitCount;
static char sLine[P4LF_KEY + 256];
static size_t sLength;
static unsigned sTimeout;

static void online_fail(const char *replay, const char *why) {
    fprintf(stderr, "p4pc: state-aligned lockstep: %s replay: online: %s\n", replay, why);
    exit(4);
}

int p4pc_online_configured(void) {
    const char *path = getenv("P4PC_LOCKSTEP_DISC_ONLINE");
    return path && *path;
}

static OnlineRequest *request_at(OnlineList *list, size_t index, const char *replay) {
    if (index > list->count) online_fail(replay, "the relay skipped a request index");
    if (index == list->count) {
        OnlineRequest *grown = realloc(list->items, (list->count + 1) * sizeof *list->items);
        if (!grown) online_fail(replay, "allocating the channel");
        list->items = grown;
        memset(&list->items[list->count++], 0, sizeof *list->items);
    }
    return &list->items[index];
}

static void add_poll(OnlineRequest *request, int32_t update, int complete, const char *replay) {
    if (!request->known) online_fail(replay, "the relay reported a poll before its request");
    if (request->count == request->capacity) {
        const size_t capacity = request->capacity ? request->capacity * 2 : 8;
        OnlinePoll *grown = realloc(request->polls, capacity * sizeof *grown);
        if (!grown) online_fail(replay, "allocating the channel");
        request->polls = grown;
        request->capacity = capacity;
    }
    request->polls[request->count++] = (OnlinePoll){update, complete};
}

/* D index rel file offset sectors | P index rel complete | F index rel key | T index rel complete
 * | U index rel creator copies word... | E message */
static void parse(const char *line) {
    unsigned long index;
    int32_t update;
    int complete, used = 0;
    if (line[0] == '#') return;
    if (line[0] == 'E' && line[1] == ' ') online_fail("disc", line + 2);
    if (line[0] == 'D') {
        char name[P4ON_NAME];
        uint32_t offset, sectors;
        OnlineRequest *read;
        if (sscanf(line, "D %lu %d %63s %u %u", &index, &update, name, &offset, &sectors) != 5)
            online_fail("disc", "malformed relay read line");
        read = request_at(&sReads, index, "disc");
        if (read->known) online_fail("disc", "the relay reported a read twice");
        read->known = 1;
        strcpy(read->name, name);
        read->offset = offset;
        read->sectors = sectors;
        read->request = update;
    } else if (line[0] == 'F') {
        OnlineRequest *filer;
        if (sscanf(line, "F %lu %d %n", &index, &update, &used) != 2 || !used || !line[used])
            online_fail("filer", "malformed relay filer line");
        filer = request_at(&sFiler, index, "filer");
        if (filer->known) online_fail("filer", "the relay reported a request twice");
        filer->key = malloc(P4LF_KEY);
        if (!filer->key || strlen(line + used) >= P4LF_KEY) online_fail("filer", "a relay key is too long");
        strcpy(filer->key, line + used);
        filer->known = 1;
        filer->request = update;
    } else if (line[0] == 'U') {
        unsigned creator, copies;
        OnlineUninit *grown, *list;
        const char *cursor;
        char *end;
        if (sscanf(line, "U %lu %d %x %u %n", &index, &update, &creator, &copies, &used) != 4 || !used ||
            index != sUninitCount)
            online_fail("uninit", "malformed relay thunder line");
        grown = realloc(sUninit, (sUninitCount + 1) * sizeof *sUninit);
        if (!grown) online_fail("uninit", "allocating the channel");
        sUninit = grown;
        list = &sUninit[sUninitCount];
        memset(list, 0, sizeof *list);
        list->creator = creator;
        list->copies = copies;
        list->update = update;
        for (cursor = line + used; *cursor;) {
            const unsigned long word = strtoul(cursor, &end, 16);
            uint32_t *words;
            if (end == cursor) online_fail("uninit", "malformed relay thunder word");
            words = realloc(list->words, (list->count + 1) * sizeof *words);
            if (!words) online_fail("uninit", "allocating the channel");
            list->words = words;
            list->words[list->count++] = (uint32_t)word;
            for (cursor = end; *cursor == ' '; cursor++) {
            }
        }
        sUninitCount++;
    } else if (line[0] == 'r') {
        char name[P4ON_NAME];
        uint32_t offset, sectors;
        OnlineRequest *read;
        if (sscanf(line, "r %lu %63s %u %u %d", &index, name, &offset, &sectors, &update) != 5)
            online_fail("disc", "malformed relay pre-anchor read line");
        read = request_at(&sPreReads, index, "disc");
        if (read->known) online_fail("disc", "the relay reported a pre-anchor read twice");
        read->known = 1;
        strcpy(read->name, name);
        read->offset = offset;
        read->sectors = sectors;
        read->request = update;
    } else if (line[0] == 'v') {
        unsigned a, b, c;
        OnlineRequest *event;
        if (sscanf(line, "v %lu %u %u %u %d", &index, &a, &b, &c, &update) != 5)
            online_fail("disc", "malformed relay pre-anchor event line");
        event = request_at(&sPreEvents, index, "disc");
        if (event->known) online_fail("disc", "the relay reported an event load twice");
        event->known = 1;
        event->ids[0] = a; event->ids[1] = b; event->ids[2] = c;
        event->request = update;
    } else if (line[0] == 'm') {
        char name[P4ON_NAME];
        OnlineRequest *movie, *status;
        if (sscanf(line, "m %lu %63s %d", &index, name, &update) != 3)
            online_fail("disc", "malformed relay pre-anchor movie line");
        movie = request_at(&sPreMovies, index, "disc");
        status = request_at(&sPreStatus, index, "disc");
        if (movie->known) online_fail("disc", "the relay reported a movie twice");
        movie->known = status->known = 1;
        strcpy(movie->name, name);
        movie->request = update;
    } else if (line[0] == 'x') {
        OnlineRequest *filer;
        if (sscanf(line, "x %lu %d %n", &index, &update, &used) != 2 || !used || !line[used])
            online_fail("filer", "malformed relay pre-anchor filer line");
        filer = request_at(&sPreFiler, index, "filer");
        if (filer->known) online_fail("filer", "the relay reported a pre-anchor request twice");
        filer->key = malloc(P4LF_KEY);
        if (!filer->key || strlen(line + used) >= P4LF_KEY) online_fail("filer", "a relay key is too long");
        strcpy(filer->key, line + used);
        filer->known = 1;
        filer->request = update;
    } else if (line[0] == 'y') {
        if (sscanf(line + 1, " %lu %d %d", &index, &update, &complete) != 3 || index >= sPreFiler.count)
            online_fail("filer", "malformed relay pre-anchor take line");
        add_poll(&sPreFiler.items[index], update, complete != 0, "filer");
    } else if (line[0] == 'p' || line[0] == 'w') {
        OnlineList *list = line[0] == 'p' ? &sPreReads : &sPreEvents;
        if (sscanf(line + 1, " %lu %d %d", &index, &update, &complete) != 3 || index >= list->count)
            online_fail("disc", "malformed relay pre-anchor poll line");
        add_poll(&list->items[index], update, complete != 0, "disc");
    } else if (line[0] == 'n' || line[0] == 't') {
        OnlineList *list = line[0] == 'n' ? &sPreMovies : &sPreStatus;
        if (sscanf(line + 1, " %lu %d", &index, &update) != 2 || index >= list->count)
            online_fail("disc", "malformed relay pre-anchor movie answer");
        add_poll(&list->items[index], update, 0, "disc");
    } else if (line[0] == 'P' || line[0] == 'T') {
        OnlineList *list = line[0] == 'P' ? &sReads : &sFiler;
        const char *replay = line[0] == 'P' ? "disc" : "filer";
        if (sscanf(line + 1, " %lu %d %d", &index, &update, &complete) != 3 || index >= list->count)
            online_fail(replay, "malformed relay poll line");
        add_poll(&list->items[index], update, complete != 0, replay);
    } else {
        online_fail("disc", "unknown relay line");
    }
}

/* Parses every whole line the relay has written so far. */
static void drain(const char *replay) {
    if (!sChannel) {
        const char *path = getenv("P4PC_LOCKSTEP_DISC_ONLINE");
        const char *timeout = getenv("P4PC_LOCKSTEP_ONLINE_TIMEOUT");
        if (!path || !*path) online_fail(replay, "P4PC_LOCKSTEP_DISC_ONLINE is unset");
        sChannel = fopen(path, "rb");
        if (!sChannel) online_fail(replay, "cannot open P4PC_LOCKSTEP_DISC_ONLINE");
        sTimeout = timeout && *timeout ? (unsigned)strtoul(timeout, NULL, 10) : 300u;
    }
    for (;;) {
        const int c = fgetc(sChannel);
        if (c == EOF) {
            if (ferror(sChannel)) online_fail(replay, "reading the relay channel");
            clearerr(sChannel);
            return;
        }
        if (c == '\r') continue;
        if (c != '\n') {
            if (sLength + 1 >= sizeof sLine) online_fail(replay, "a relay line is too long");
            sLine[sLength++] = (char)c;
            continue;
        }
        sLine[sLength] = '\0';
        sLength = 0;
        parse(sLine);
    }
}

typedef int (*OnlineReady)(const OnlineList *list, size_t index);

static int known(const OnlineList *list, size_t index) {
    return index < list->count && list->items[index].known;
}

static int polled(const OnlineList *list, size_t index) {
    return known(list, index) && list->items[index].next < list->items[index].count;
}

static OnlineRequest *wait_for(OnlineList *list, size_t index, OnlineReady ready, const char *replay,
                               const char *what) {
    const time_t start = time(NULL);
    for (drain(replay); !ready(list, index); drain(replay)) {
        if ((unsigned)(time(NULL) - start) >= sTimeout) {
            char why[256];
            snprintf(why, sizeof why, "retail did not report %s %lu within %u s", what, (unsigned long)index,
                     sTimeout);
            online_fail(replay, why);
        }
        nap();
    }
    return &list->items[index];
}

void p4pc_online_read(uint32_t index, char name[P4ON_NAME], uint32_t *offset, uint32_t *sectors,
                      int32_t *request) {
    const OnlineRequest *read = wait_for(&sReads, index, known, "disc", "game read");
    strcpy(name, read->name);
    *offset = read->offset;
    *sectors = read->sectors;
    *request = read->request;
}

void p4pc_online_read_poll(uint32_t index, int32_t *update, int *complete) {
    OnlineRequest *read = wait_for(&sReads, index, polled, "disc", "a poll of game read");
    const OnlinePoll poll = read->polls[read->next++];
    *update = poll.update;
    *complete = poll.complete;
}

void p4pc_online_filer(uint32_t index, char *key, int32_t *enqueue) {
    const OnlineRequest *filer = wait_for(&sFiler, index, known, "filer", "FilerRequest");
    strcpy(key, filer->key);
    *enqueue = filer->request;
}

void p4pc_online_filer_take(uint32_t index, int32_t *update, int *complete) {
    OnlineRequest *filer = wait_for(&sFiler, index, polled, "filer", "a take of FilerRequest");
    const OnlinePoll poll = filer->polls[filer->next++];
    *update = poll.update;
    *complete = poll.complete;
}

void p4pc_online_pre_read(uint32_t index, char name[P4ON_NAME], uint32_t *offset, uint32_t *sectors,
                          int32_t *request) {
    const OnlineRequest *read = wait_for(&sPreReads, index, known, "disc", "pre-anchor game read");
    strcpy(name, read->name);
    *offset = read->offset;
    *sectors = read->sectors;
    *request = read->request;
}

void p4pc_online_pre_read_poll(uint32_t index, int32_t *relative, int *complete) {
    OnlineRequest *read = wait_for(&sPreReads, index, polled, "disc", "a poll of pre-anchor game read");
    const OnlinePoll poll = read->polls[read->next++];
    *relative = poll.update;
    *complete = poll.complete;
}

void p4pc_online_pre_filer(uint32_t index, char *key, int32_t *enqueue) {
    const OnlineRequest *filer = wait_for(&sPreFiler, index, known, "filer", "pre-anchor FilerRequest");
    strcpy(key, filer->key);
    *enqueue = filer->request;
}

void p4pc_online_pre_filer_take(uint32_t index, int32_t *relative, int *complete) {
    OnlineRequest *filer = wait_for(&sPreFiler, index, polled, "filer", "a take of pre-anchor FilerRequest");
    const OnlinePoll poll = filer->polls[filer->next++];
    *relative = poll.update;
    *complete = poll.complete;
}

void p4pc_online_pre_event(uint32_t index, uint32_t id[3], int32_t *request) {
    const OnlineRequest *event = wait_for(&sPreEvents, index, known, "disc", "event load");
    memcpy(id, event->ids, sizeof event->ids);
    *request = event->request;
}

void p4pc_online_pre_event_poll(uint32_t index, int32_t *relative, int *complete) {
    OnlineRequest *event = wait_for(&sPreEvents, index, polled, "disc", "a poll of event load");
    const OnlinePoll poll = event->polls[event->next++];
    *relative = poll.update;
    *complete = poll.complete;
}

void p4pc_online_pre_movie(uint32_t index, char name[P4ON_NAME], int32_t *request) {
    const OnlineRequest *movie = wait_for(&sPreMovies, index, known, "disc", "movie");
    strcpy(name, movie->name);
    *request = movie->request;
}

int32_t p4pc_online_pre_movie_poll(uint32_t index) {
    OnlineRequest *movie = wait_for(&sPreMovies, index, polled, "disc", "a GetCurFrm answer of movie");
    return movie->polls[movie->next++].update;
}

int32_t p4pc_online_pre_movie_status(uint32_t index) {
    OnlineRequest *status = wait_for(&sPreStatus, index, polled, "disc", "a status answer of movie");
    return status->polls[status->next++].update;
}

size_t p4pc_online_uninit(uint32_t index, uint32_t *creator, int32_t *update, uint32_t *copies, uint32_t *words,
                          size_t capacity) {
    const time_t start = time(NULL);
    const OnlineUninit *list;
    for (drain("uninit"); index >= sUninitCount; drain("uninit")) {
        if ((unsigned)(time(NULL) - start) >= sTimeout) {
            char why[256];
            snprintf(why, sizeof why, "retail did not report thunder bolt list %lu within %u s", (unsigned long)index,
                     sTimeout);
            online_fail("uninit", why);
        }
        nap();
    }
    list = &sUninit[index];
    *creator = list->creator;
    *update = list->update;
    *copies = list->copies;
    if (list->count > capacity) online_fail("uninit", "a retail thunder bolt list is longer than the port's");
    memcpy(words, list->words, list->count * sizeof *words);
    return list->count;
}
