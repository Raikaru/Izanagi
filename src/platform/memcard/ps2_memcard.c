/* sceMc2 (libmc2 memory-card library) entry points for the PC port.
 *
 * Model: each memory-card port is a directory under the save root, $P4PC_SAVE_DIR
 * (default ./p4pc_save): <root>/mc0 and <root>/mc1. A socket is a port plus a
 * current directory set by Chdir. Files are plain files inside those directories.
 *
 * Commands complete at host speed and return positive command IDs. CheckAsync
 * delivers the pending kind/result once (1), then reports idle (-1). A second
 * request is refused until that result is collected, before it can touch files
 * or caller output buffers.
 *
 * Kinds are the libmc2 op numbers read from the retail bodies (see the
 * overrides/CLAIMS.md rows and docs/platform/saves_memcard.md). Result: >= 0 success (bytes for read/write),
 * < 0 is -errno. memcardPollCommand reads the low 16 bits of a negative result as the
 * card error code.
 *
 * Retail error codes (from the bodies): 0x81018001 not initialised, 0x81018002
 * socket, 0x81018010 busy, 0x81018016 bad CreateSocket params, 0x81018017 socket
 * create failed. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "p4pc/os.h"
#include "p4pc/mod_save_native.h"
#include "mod_save_transport.h"
#include "p4pc/save_import.h"

#define P4PC_MC_PORTS 2
#define P4PC_MC_CWD_MAX 128
#define P4PC_MC_PATH_MAX P4_SAVE_PATH_CAP

#define P4PC_MC_ERR_NOT_INIT 0x81018001
#define P4PC_MC_ERR_SOCKET 0x81018002
#define P4PC_MC_ERR_BUSY 0x81018010
#define P4PC_MC_ERR_PARAM 0x81018016
#define P4PC_MC_ERR_FORMAT_BUSY 0x81010010 /* func_00431b58 reports busy with this code, not ..8010 */
#define P4PC_MC_ERR_CREATE 0x81018017

#define P4PC_MC_KIND_GETINFO 2
#define P4PC_MC_KIND_FORMAT 3
#define P4PC_MC_KIND_READFILE 5
#define P4PC_MC_KIND_WRITEFILE 6
#define P4PC_MC_KIND_CREATEFILE 7
#define P4PC_MC_KIND_MKDIR 11
#define P4PC_MC_KIND_CHDIR 12
#define P4PC_MC_KIND_SEARCHFILE 14

/* GetInfo status words the game checks: status[0] == 2 means the card is ready, status[1] != 0 that it is formatted (h_memcard.c:
 * a zero word at 0x008E4B24 makes the game offer "The memory card ... is unformatted. Format?"), status[2] is the free-space figure
 * it compares against 0xE6 [INFERENCE: unit is
 * the libmc2 free-cluster count; 0x1000 is an ample value for the folder store]. */
#define P4PC_MC_STATUS_READY 2
#define P4PC_MC_STATUS_FORMATTED 1
#define P4PC_MC_STATUS_FREE 0x1000

typedef struct {
    int open;
    char cwd[P4PC_MC_CWD_MAX];
} McSocket;

static int sInit;
static McSocket sSockets[P4PC_MC_PORTS];
static int32_t sCmdCounter;
static int sPending;
static int32_t sPendingKind;
static int32_t sPendingResult;
static int32_t sSaveFaultSocket = -1;

static const char *saveRoot(void)
{
    const char *env = getenv("P4PC_SAVE_DIR");
    return env && *env ? env : "p4pc_save";
}

/* the file name of the op being set up, for P4PC_MC_LOG */
static const char *sLastName = "";

static int validSocket(int32_t socket)
{
    return sInit && socket >= 0 && socket < P4PC_MC_PORTS && sSockets[socket].open;
}
static int cardPath(int32_t socket,char *out,size_t cap)
{
    int n=snprintf(out,cap,"%s/mc%d",saveRoot(),(int)socket);
    return n<0||(size_t)n>=cap||p4_os_extended_path(out,cap);
}
/* Resolve only actual canonical save components. Host staging names never
 * become socket cwd or guest-visible filenames. */
static int saveTarget(int32_t socket,const char *name,char slot[33],const char **file)
{
    if(!name)return 0;
    if(name[0]!='/'){
        const char *cwd=sSockets[socket].cwd;
        if(p4_save_slot_index(cwd)>=0){strcpy(slot,cwd);*file=*name?name:NULL;return 1;}
        if(*cwd)return 0;
    }else ++name;
    const char *slash=strchr(name,'/');
    size_t n=slash?(size_t)(slash-name):strlen(name);
    if(n>=33)return 0;
    memcpy(slot,name,n);slot[n]=0;
    if(p4_save_slot_index(slot)<0)return 0;
    *file=slash&&slash[1]?slash+1:NULL;
    return 1;
}

/* Full host path for name: a name with a leading '/' is absolute on the card (the game's save files are '/BASLUS-21782Save0/BASLUS-21782Save0'),
 * any other is under the socket's current directory. Rejects backslashes and parent references, so a save name cannot leave the port directory. */
static int buildPath(int32_t socket, const char *name, char *out, size_t cap)
{
    int n;

    sLastName = name != NULL ? name : "(null)";

    if (!name || name[0] == '\\' || strstr(name, "..") != NULL) {
        return -1;
    }
    char slot[33],card[P4PC_MC_PATH_MAX];const char *file;
    if(saveTarget(socket,name,slot,&file)){
        if(cardPath(socket,card,sizeof card))return -1;
        int routed=p4_mod_save_transport_route(card,slot,file,out,cap);
        if(routed)return routed<0?-1:0;
    }
    if (name[0] == '/') {
        n = snprintf(out, cap, "%s/mc%d/%s", saveRoot(), (int)socket, name + 1);
    } else if (sSockets[socket].cwd[0]) {
        n = snprintf(out, cap, "%s/mc%d/%s/%s", saveRoot(), (int)socket, sSockets[socket].cwd, name);
    } else {
        n = snprintf(out, cap, "%s/mc%d/%s", saveRoot(), (int)socket, name);
    }
    if (n <= 0 || (size_t)n >= cap) return -1;
    return p4_os_extended_path(out, cap);
}
/* P4PC_MC_LOG=1 logs every op's kind and result (and the file name of the path ops) to stderr. */
static int logOn(void)
{
    static int on = -1;

    if (on < 0) {
        const char *e = getenv("P4PC_MC_LOG");

        on = e != NULL && e[0] == '1';
    }
    return on;
}

static int32_t beginAsync(int32_t kind, int32_t result)
{
    if (logOn()) {
        fprintf(stderr, "p4pc: mc op kind %d result %d (0x%08x) name '%s'\n", (int)kind, (int)result, (unsigned)result, sLastName);
    }
    sLastName = "";
    sPending = 1;
    sPendingKind = kind;
    sPendingResult = result;
    return ++sCmdCounter;
}

/* A failed op's result, as libmc2 reports it: a negative word with the card error code (here the errno value: 2 no entry, 0x13 no device, 0x16 invalid,
 * the codes h_memcard.c switches on) in the low 16 bits; memcardPollCommand keeps `result & 0xFFFF`. */
#define P4PC_MC_FAIL(e) ((int32_t)(0x81010000u | ((unsigned)(e) & 0xFFFFu)))

static int32_t errnoResult(void)
{
    return P4PC_MC_FAIL(errno);
}
void p4_mc_mod_save_begin(int32_t socket,int32_t slot,const void *image,int32_t size)
{
    char card[P4PC_MC_PATH_MAX],error[1024];
    sSaveFaultSocket=-1;
    p4_mod_save_transport_abort();
    if(!validSocket(socket))return; /* Original card status reports this error. */
    int invalid=slot<0||slot>=16||size<0;
    int bad_card=!invalid&&cardPath(socket,card,sizeof card);
    if(invalid||bad_card||
       p4_mod_save_transport_begin(card,(unsigned)slot,image,(size_t)size,error,sizeof error)){
        if(invalid)snprintf(error,sizeof error,"Invalid full-save submission");
        else if(bad_card)snprintf(error,sizeof error,"Memory-card root path is too long");
        sSaveFaultSocket=socket;
        p4_mod_save_transport_error("Cannot save game",error);
    }
}
void p4_mc_mod_load_begin(void)
{
    p4_mod_save_transport_abort();
    p4_mod_save_transport_load_abort();
    sSaveFaultSocket=-1;
}

int32_t func_004316a8(int32_t flag)
{
    (void)flag;
    sInit = 1;
    return 0;
}

int32_t func_00431928(void *params, void *extra)
{
    const int32_t *p = (const int32_t *)params;
    int32_t port;
    char dir[P4PC_MC_PATH_MAX];

    (void)extra;
    if (!sInit) {
        return P4PC_MC_ERR_NOT_INIT;
    }
    /* Params: +4 is the port selector (2 or 3, port = selector - 2), +8 the slot (0). */
    if (!p || p[1] < 2 || p[1] > 3 || p[2] != 0) {
        return P4PC_MC_ERR_PARAM;
    }
    port = p[1] - 2;
    if (snprintf(dir, sizeof dir, "%s", saveRoot()) >= (int)sizeof dir ||
        p4_os_extended_path(dir, sizeof dir) != 0)
        return P4PC_MC_ERR_CREATE;
    p4_os_mkdir(dir);
    if (buildPath(port, "/", dir, sizeof dir) != 0)
        return P4PC_MC_ERR_CREATE;
    if (p4_os_mkdir(dir) != 0 && errno != EEXIST) {
        return P4PC_MC_ERR_CREATE;
    }
    char error[1024];
    if(p4_save_transactions_recover(dir,error,sizeof error)){
        p4_mod_save_transport_error("Interrupted save recovery failed",error);
        return P4PC_MC_ERR_CREATE;
    }
    sSockets[port].open = 1;
    sSockets[port].cwd[0] = '\0';
    return port;
}

/* Removes everything inside `dir` (files and subdirectories), keeping `dir` itself. */
typedef struct {
    const char *dir;
    const char *separator;
} McFormatDir;

static void emptyDir(const char *dir);

static void emptyDirEntry(const char *name, void *user)
{
    const McFormatDir *directory = user;
    char path[P4PC_MC_PATH_MAX];
    if (!strcmp(name, ".") || !strcmp(name, ".."))
        return;
    if (snprintf(path, sizeof path, "%s%s%s", directory->dir, directory->separator, name) >= (int)sizeof path)
        return;
    if (p4_os_extended_path(path, sizeof path) != 0)
        return;
    if (p4_os_is_dir_nofollow(path)) {
        emptyDir(path);
        rmdir(path);
    } else {
        remove(path);
    }
}

static void emptyDir(const char *dir)
{
    const char last = dir[strlen(dir) - 1];
    McFormatDir directory = {dir, last == '/' || last == '\\' ? "" : "/"};
    p4_os_list_dir(dir, emptyDirEntry, &directory);
}

/* Retail 00431b58 (libmc2 op 3, "format"): queues the format of the card behind `socket`, as the
 * game does for an unformatted card. Checks, in order: not initialised, unknown socket, a command
 * already in flight (0x81010010). The host card is its folder: format empties it and resets the
 * socket's directory to the root. Returns 0 when queued; the completion (kind 3, result 0)
 * is delivered once through sceMc2CheckAsync, like the other async calls. */
int32_t func_00431b58(int32_t socket)
{
    char dir[P4PC_MC_PATH_MAX];

    if (!sInit) {
        return P4PC_MC_ERR_NOT_INIT;
    }
    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) {
        return P4PC_MC_ERR_FORMAT_BUSY;
    }
    p4_mod_save_transport_abort();
    p4_mod_save_transport_load_abort();
    sSaveFaultSocket=-1;
    if (buildPath(socket, "/", dir, sizeof dir) != 0)
        return P4PC_MC_FAIL(EINVAL);
    emptyDir(dir);
    sSockets[socket].cwd[0] = '\0';
    beginAsync(P4PC_MC_KIND_FORMAT, 0);
    return 0;
}

int32_t func_00431cb8(int32_t socket, int32_t *status)
{
    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if(sSaveFaultSocket==socket){
        sSaveFaultSocket=-1;
        /* Retail's GetInfo switch handles its general card-failure code;
         * the precise provider/filesystem reason was shown natively above. */
        return beginAsync(P4PC_MC_KIND_GETINFO,P4PC_MC_FAIL(0x9003));
    }
    if (status) {
        status[0] = P4PC_MC_STATUS_READY;
        status[1] = P4PC_MC_STATUS_FORMATTED;
        status[2] = P4PC_MC_STATUS_FREE;
    }
    return beginAsync(P4PC_MC_KIND_GETINFO, 0);
}

/* sceMc2SearchFileAsync (retail 0x00431D78): whether `name` exists. On success `result` receives the entry as libmc2's directory table (sceMcTblGetDir,
 * 64 bytes): create and modify dates (resv, sec, min, hour, day, month, u16 year; here the file's mtime for both), the size in bytes at +0x10, the
 * attribute word at +0x14 (0x8427 for a directory, 0x8497 for a file: readable, writable, executable, copyable, and the 0x8000 "exists" bit) and the
 * name at +0x1E. h_memcard.c reads the size back (D_008E4910: the number of bytes of the save it then reads). */
int32_t func_00431d78(int32_t socket, const char *name, void *result)
{
    char path[P4PC_MC_PATH_MAX];
    int32_t res;
    struct stat st;

    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if (buildPath(socket, name, path, sizeof path) != 0) {
        return beginAsync(P4PC_MC_KIND_SEARCHFILE, P4PC_MC_FAIL(ENOENT));
    }
    res = stat(path, &st) == 0 ? 0 : P4PC_MC_FAIL(ENOENT);
    if (res == 0 && result != NULL) {
        uint8_t *e = (uint8_t *)result;
        const struct tm *tmv;
        time_t mt = st.st_mtime;
        const char *base = strrchr(name, '/');
        size_t k;

        memset(e, 0, 64);
        tmv = gmtime(&mt);   /* not gmtime_r: the Windows build has none; the shim is single threaded */
        if (tmv != NULL) {
            for (k = 0; k < 2; k++) {
                e[k * 8 + 1] = (uint8_t)tmv->tm_sec;
                e[k * 8 + 2] = (uint8_t)tmv->tm_min;
                e[k * 8 + 3] = (uint8_t)tmv->tm_hour;
                e[k * 8 + 4] = (uint8_t)tmv->tm_mday;
                e[k * 8 + 5] = (uint8_t)(tmv->tm_mon + 1);
                e[k * 8 + 6] = (uint8_t)((tmv->tm_year + 1900) & 0xFF);
                e[k * 8 + 7] = (uint8_t)((tmv->tm_year + 1900) >> 8);
            }
        }
        *(uint32_t *)(e + 0x10) = S_ISDIR(st.st_mode) ? 0u : (uint32_t)st.st_size;
        *(uint16_t *)(e + 0x14) = S_ISDIR(st.st_mode) ? 0x8427 : 0x8497;
        base = base != NULL ? base + 1 : name;
        strncpy((char *)e + 0x1E, base, 31);
    }
    return beginAsync(P4PC_MC_KIND_SEARCHFILE, res);
}

int32_t func_00431f28(int32_t socket, const char *name, void *buf, uint32_t offset, uint32_t size)
{
    if (logOn()) {
        fprintf(stderr, "p4pc: mc read '%s' offset %u size %u from %p\n", name != NULL ? name : "(null)", (unsigned)offset, (unsigned)size, __builtin_return_address(0));
    }
    char path[P4PC_MC_PATH_MAX],card[P4PC_MC_PATH_MAX],slot[33],error[1024];
    const char *file=NULL;P4SaveTransaction transaction={0};
    FILE *f;size_t got=0;int32_t res;
    if(!validSocket(socket))return P4PC_MC_ERR_SOCKET;
    if(sPending)return P4PC_MC_ERR_BUSY;
    if(p4_mod_save_transport_saving())return P4PC_MC_ERR_BUSY;
    int full_candidate=!offset&&size>=4100&&saveTarget(socket,name,slot,&file)&&file&&!strcmp(file,slot);
    if(full_candidate){
        p4_mod_save_transport_load_abort();
        int too_big=size>P4SAVE_MAX_BYTES;
        int bad_card=!too_big&&cardPath(socket,card,sizeof card);
        if(too_big||bad_card||
           p4_save_transaction_begin(&transaction,card,(unsigned)p4_save_slot_index(slot),1,0,error,sizeof error)){
            if(too_big)snprintf(error,sizeof error,"Full save exceeds the supported retail image bound");
            else if(bad_card)snprintf(error,sizeof error,"Memory-card root path is too long");
            p4_mod_save_transport_error("Cannot load game",error);
            return beginAsync(P4PC_MC_KIND_READFILE,P4PC_MC_FAIL(EIO));
        }
    }
    if(buildPath(socket,name,path,sizeof path)){
        p4_save_transaction_abort(&transaction);
        return beginAsync(P4PC_MC_KIND_READFILE,P4PC_MC_FAIL(EINVAL));
    }
    f=fopen(path,"rb");
    if(!f){res=errnoResult();p4_save_transaction_abort(&transaction);return beginAsync(P4PC_MC_KIND_READFILE,res);}
    P4OSFileIdentity identity;
    int complete=full_candidate&&!p4_os_file_identity(f,&identity)&&identity.size==size;
    if(fseek(f,(long)offset,SEEK_SET)==0)got=fread(buf,1,size,f);
    res=ferror(f)?P4PC_MC_FAIL(EIO):(int32_t)got;
    if(fclose(f))res=P4PC_MC_FAIL(EIO);
    if(complete&&res>=0&&got==size&&p4_mod_save_transport_loaded(&transaction,buf,got,error,sizeof error)){
        p4_mod_save_transport_error("Cannot load game",error);res=P4PC_MC_FAIL(EIO);
    }else if(full_candidate&&(!complete||got!=size)){
        p4_mod_save_transport_load_abort();res=P4PC_MC_FAIL(EIO);
    }
    p4_save_transaction_abort(&transaction);
    return beginAsync(P4PC_MC_KIND_READFILE,res);
}

int32_t func_00432288(int32_t socket, const char *name, const void *data, uint32_t offset, uint32_t size)
{
    if (logOn()) {
        fprintf(stderr, "p4pc: mc write '%s' offset %u size %u\n", name != NULL ? name : "(null)", (unsigned)offset, (unsigned)size);
    }
    char path[P4PC_MC_PATH_MAX];
    FILE *f;
    size_t put = 0;
    int32_t res;

    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if (buildPath(socket, name, path, sizeof path) != 0) {
        p4_mod_save_transport_abort();
        return beginAsync(P4PC_MC_KIND_WRITEFILE, P4PC_MC_FAIL(EINVAL));
    }
    /* r+b keeps the existing bytes outside [offset, offset+size); w+b creates. */
    f = fopen(path, "r+b");
    if (!f) {
        f = fopen(path, "w+b");
    }
    if (!f) {
        res=errnoResult();p4_mod_save_transport_abort();
        return beginAsync(P4PC_MC_KIND_WRITEFILE,res);
    }
    if (fseek(f, (long)offset, SEEK_SET) == 0) {
        put = fwrite(data, 1, size, f);
    }
    res = (ferror(f) || put != size) ? P4PC_MC_FAIL(EIO) : (int32_t)put;
    if(fclose(f))res=P4PC_MC_FAIL(EIO);
    if(res<0)p4_mod_save_transport_abort();
    else{
        char card[P4PC_MC_PATH_MAX],slot[33],error[1024];const char *file;
        if(saveTarget(socket,name,slot,&file)&&file){
            int bad_card=cardPath(socket,card,sizeof card);
            if(bad_card||p4_mod_save_transport_written(card,slot,file,offset,size,error,sizeof error)){
                if(bad_card)snprintf(error,sizeof error,"Memory-card root path is too long");
                p4_mod_save_transport_abort();p4_mod_save_transport_error("Cannot finish save",error);
                res=P4PC_MC_FAIL(EIO);
            }
        }
    }
    return beginAsync(P4PC_MC_KIND_WRITEFILE,res);
}

int32_t func_004323a8(int32_t socket, const char *name)
{
    char path[P4PC_MC_PATH_MAX];
    FILE *f;
    int32_t res;

    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if (buildPath(socket, name, path, sizeof path) != 0) {
        p4_mod_save_transport_abort();
        return beginAsync(P4PC_MC_KIND_CREATEFILE, P4PC_MC_FAIL(EINVAL));
    }
    f = fopen(path, "rb");
    if (f) {
        fclose(f);
        res = P4PC_MC_FAIL(EEXIST);
    } else {
        f = fopen(path, "wb");
        res = f ? 0 : errnoResult();
        if (f) {
            if(fclose(f))res=P4PC_MC_FAIL(EIO);
        }
    }
    if(res<0&&((uint32_t)res&0xffffu)!=EEXIST)p4_mod_save_transport_abort();
    return beginAsync(P4PC_MC_KIND_CREATEFILE, res);
}

int32_t func_004326b8(int32_t socket, const char *name)
{
    char path[P4PC_MC_PATH_MAX];
    int32_t res;

    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if (buildPath(socket, name, path, sizeof path) != 0) {
        p4_mod_save_transport_abort();
        return beginAsync(P4PC_MC_KIND_MKDIR, P4PC_MC_FAIL(EINVAL));
    }
    res = p4_os_mkdir(path) == 0 ? 0 : errnoResult();
    if(res<0&&((uint32_t)res&0xffffu)!=EEXIST)p4_mod_save_transport_abort();
    return beginAsync(P4PC_MC_KIND_MKDIR, res);
}

int32_t func_004327b0(int32_t socket, const char *name)
{
    char path[P4PC_MC_PATH_MAX];
    struct stat st;
    int32_t res = 0;

    if (!validSocket(socket)) {
        return P4PC_MC_ERR_SOCKET;
    }
    if (sPending) return P4PC_MC_ERR_BUSY;
    if (!name || name[0] == '\0' || (name[0] == '/' && name[1] == '\0')) {
        sSockets[socket].cwd[0] = '\0';
        return beginAsync(P4PC_MC_KIND_CHDIR, 0);
    }
    if (buildPath(socket, name, path, sizeof path) != 0) {
        return beginAsync(P4PC_MC_KIND_CHDIR, P4PC_MC_FAIL(EINVAL));
    }
    if (name[0] == '/') {
        name++;
    }
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        res = P4PC_MC_FAIL(ENOENT);
    } else if (strlen(name) >= P4PC_MC_CWD_MAX) {
        res = P4PC_MC_FAIL(ENAMETOOLONG);
    } else {
        strcpy(sSockets[socket].cwd, name);
    }
    return beginAsync(P4PC_MC_KIND_CHDIR, res);
}

/* Sync2 (retail 0x00432e18) and its wrappers collect the completed host command.
 * Deliver its kind/result once, then release the pending slot. */
int32_t func_00432e18(int32_t mode, int32_t *cmd, int32_t *result)
{
    (void)mode;
    if (!sPending) {
        return -1;
    }
    if (cmd) {
        *cmd = sPendingKind;
    }
    if (result) {
        *result = sPendingResult;
    }
    sPending = 0;
    return 1;
}

/* sceMc2CheckAsync (retail 0x00432ec0): Sync2 in poll mode. */
int32_t func_00432ec0(int32_t *cmd, int32_t *result)
{
    return func_00432e18(1, cmd, result);
}

/* sceMc2Sync (retail 0x00432ee8): Sync2 in wait mode. */
int32_t func_00432ee8(int32_t *cmd, int32_t *result)
{
    return func_00432e18(0, cmd, result);
}
