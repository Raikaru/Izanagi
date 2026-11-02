/* Android entry: SDL's activity (org.libsdl.app.SDLActivity, subclassed by org.p4pc.game.P4Activity) loads libmain.so and calls SDL_main on its
 * own thread. The game runs on a thread of this process with a 16 MB stack (the Java thread's is 1 MB, the Linux build's main stack 8 MB), and this
 * thread waits for it. Android discards stderr, so the game's stderr goes through a pipe to logcat (tag "p4pc") and, when P4PC_LOG_FILE names a
 * file, to that file instead (P4PC_LOGCAT=1 for both): the run scripts read boot.log from the app's files directory.
 *
 * The Java side (android/app/src/main/java/org/p4pc/game) sets the environment before this runs: P4PC_ISO and P4PC_ELF as `fd:N` descriptors from the
 * document picker (src/platform/os p4_os_fopen_rb), P4PC_SAVE_DIR in the app's storage, and any P4PC_* test variables from the launch intent. */
#define _FILE_OFFSET_BITS 64
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <android/log.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_system.h>

#include "iso9660.h"
#include "p4pc/guest.h"
#include "p4pc/os.h"
#include "p4pc/settings.h"

int p4pc_main(int argc, char **argv);
int p4pc_viewer_main(int argc, char **argv); /* src/viewer/p4pc_viewer.c, built into libmain for the scene gates */

JNIEXPORT void JNICALL Java_org_p4pc_game_P4Activity_nativeSetEnv(JNIEnv *env, jclass cls, jstring name, jstring value) {
    const char *n = (*env)->GetStringUTFChars(env, name, NULL);
    const char *v = (*env)->GetStringUTFChars(env, value, NULL);

    (void)cls;
    if (n != NULL && v != NULL) {
        setenv(n, v, 1);
    }
    if (n != NULL) (*env)->ReleaseStringUTFChars(env, name, n);
    if (v != NULL) (*env)->ReleaseStringUTFChars(env, value, v);
}
JNIEXPORT void JNICALL Java_org_p4pc_game_P4Activity_nativeHardwareKeyboardChanged(
    JNIEnv *env, jclass cls, jboolean connected)
{
    (void)env;
    (void)cls;
    p4_settings_android_keyboard_presence(connected != JNI_FALSE);
}
void p4_settings_android_keyboard_watch(int enabled)
{
    static int watching;
    static jmethodID track;
    enabled = enabled != 0;
    if (watching == enabled) return;
    const char *input = getenv("P4PC_INPUT");
    if (input && (!strcmp(input, "script") || !strcmp(input, "virtual"))) return;
    JNIEnv *env = SDL_GetAndroidJNIEnv();
    if (!env) goto failed;
    if ((*env)->PushLocalFrame(env, 4) < 0) goto exception;
    jobject activity = SDL_GetAndroidActivity();
    if (!activity) goto pop_failed;
    if (!track) {
        jclass cls = (*env)->GetObjectClass(env, activity);
        if (!cls) goto pop_failed;
        track = (*env)->GetMethodID(env, cls, "trackHardwareKeyboard", "(Z)V");
        if (!track) goto pop_failed;
    }
    (*env)->CallVoidMethod(env, activity, track, enabled ? JNI_TRUE : JNI_FALSE);
    if ((*env)->ExceptionCheck(env)) goto pop_failed;
    (*env)->PopLocalFrame(env, NULL);
    watching = enabled;
    return;
pop_failed:
    (*env)->PopLocalFrame(env, NULL);
exception:
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
failed:
    fputs("p4pc settings: cannot track Android hardware keyboards\n", stderr);
    exit(1);
}

int p4_android_prepare_restart(void)
{
    JNIEnv *env = SDL_GetAndroidJNIEnv();
    if (!env) return -1;
    if ((*env)->PushLocalFrame(env, 4) < 0) goto failed;
    jobject activity = SDL_GetAndroidActivity();
    if (!activity) goto pop_failed;
    jclass cls = (*env)->GetObjectClass(env, activity);
    if (!cls) goto pop_failed;
    jmethodID method = (*env)->GetMethodID(env, cls, "prepareRestart", "()Z");
    if (!method) goto pop_failed;
    jboolean accepted = (*env)->CallBooleanMethod(env, activity, method);
    if ((*env)->ExceptionCheck(env) || !accepted) goto pop_failed;
    (*env)->PopLocalFrame(env, NULL);
    return 0;
pop_failed:
    (*env)->PopLocalFrame(env, NULL);
failed:
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
    fputs("p4pc restart: Android helper handoff failed\n", stderr);
    return -1;
}

/* Every exit() of the process, the game's own (a gate's reference compare exits 0 or 1; the window close exits 0) included: libmain is linked with
 * -Wl,--wrap=exit, so the status reaches the log, where tools/android_prog.py reads it (a process has no other way to report it to adb). */
extern void __real_exit(int status) __attribute__((noreturn));

__attribute__((noreturn)) void __wrap_exit(int status) {
    fprintf(stderr, "p4pc: exited with status %d\n", status);
    fflush(NULL);
    usleep(300000); /* the logger thread copies the pipe: give it the last lines */
    __real_exit(status);
}

static int sLogPipe[2] = {-1, -1};

static void *log_thread(void *arg) {
    char buf[8192], line[1024];
    size_t n = 0;
    ssize_t got;
    FILE *file = NULL;
    const char *path = getenv("P4PC_LOG_FILE");
    const char *lc = getenv("P4PC_LOGCAT");
    /* logcat is the log of a hand run; a test run (P4PC_LOG_FILE set) reads boot.log, and a game that prints thousands of lines a second must not
     * wait for logcat's daemon (it was most of a gate's time) */
    const int logcat = path == NULL || path[0] == '\0' || (lc != NULL && lc[0] != '\0' && lc[0] != '0');

    (void)arg;
    if (path != NULL && path[0] != '\0') {
        file = fopen(path, "w");
    }
    while ((got = read(sLogPipe[0], buf, sizeof buf)) > 0) {
        if (file != NULL) {
            fwrite(buf, 1, (size_t)got, file);
            fflush(file);
        }
        for (ssize_t i = 0; logcat && i < got; i++) {
            if (buf[i] == '\n' || n == sizeof line - 1) {
                line[n] = '\0';
                __android_log_write(ANDROID_LOG_INFO, "p4pc", line);
                n = 0;
                if (buf[i] == '\n') continue;
            }
            line[n++] = buf[i];
        }
    }
    return NULL;
}

/* The game executable is on the disc (SLUS_217.82 at the root): copy it to the app's files directory so the loader can read it as a file. */
typedef struct {
    long long offset;
    long long size;
} ElfLoc;

static void find_elf(void *ctx, const char *path, long long offset, long long size) {
    if (strcmp(path, "SLUS_217.82") == 0) {
        ElfLoc *loc = ctx;
        loc->offset = offset;
        loc->size = size;
    }
}

static int extract_elf(const char *iso, const char *dest) {
    FILE *in = p4_os_fopen_rb(iso);
    FILE *out;
    ElfLoc loc = {-1, 0};
    char buf[65536];

    if (in == NULL) return -1;
    if (iso_walk(in, 0, find_elf, &loc) != 0 || loc.offset < 0 || loc.size <= 0 || loc.size > (64 << 20) ||
        fseeko(in, (off_t)loc.offset, SEEK_SET) != 0) {
        fclose(in);
        return -1;
    }
    out = fopen(dest, "wb");
    if (out == NULL) {
        fclose(in);
        return -1;
    }
    for (long long left = loc.size; left > 0;) {
        const size_t want = left < (long long)sizeof buf ? (size_t)left : sizeof buf;
        const size_t got = fread(buf, 1, want, in);

        if (got == 0 || fwrite(buf, 1, got, out) != got) {
            fclose(in);
            fclose(out);
            return -1;
        }
        left -= (long long)got;
    }
    fclose(in);
    return fclose(out);
}

#ifdef P4_GUEST64
/* The arena is this thread's stack (main below): its top holds bionic's thread block and the native frames of p4pc_main (the game itself runs on the
 * arena's guest stacks), and its bottom the region ART protects when it attaches the thread (SDL_GetAndroidJNIEnv, before this runs). Both are taken out of
 * the arena's allocator, so no game stack is placed on them: one block at the bottom, one at the top, the middle allocated and freed to push the top one up. */
static void reserve_native_stack(void) {
    const size_t low = 64u << 10, top = 512u << 10, page = 4096u;
    const size_t mid = P4_GUEST_STACKS_SIZE - (low + page) - 2 * page - (top + page);

    if (p4_guest_stack_alloc(low) == NULL) return;
    void *middle = p4_guest_stack_alloc(mid);

    (void)p4_guest_stack_alloc(top);
    p4_guest_stack_free(middle);
}
#else
static void reserve_native_stack(void) {}
#endif

static void *game_thread(void *arg) {
    char *argv[2] = {(char *)"p4pc", NULL};
    int argc = 1;
    const char *elf = getenv("P4PC_ELF");
    const char *iso = getenv("P4PC_ISO");
    const char *log = getenv("P4PC_LOG_FILE");
    char dest[1024];
    int *rc = arg;
    p4_os_init(); /* Per-thread alternate signal stack, before extraction/JNI work. */

    /* The real-game settings witness replaced its standalone entry, which set
     * its own nice level. Android SELinux forbids adb/run-as renicing this
     * process, so lower the owned worker before JNI/game initialization. */
    const char *input = getenv("P4PC_INPUT");
    if (getenv("P4PC_SETTINGS_TEST") != NULL && input && !strcmp(input, "script")) {
        if (setpriority(PRIO_PROCESS, 0, 15) != 0 || getpriority(PRIO_PROCESS, 0) != 15) {
            perror("p4pc: cannot lower automated settings worker priority");
            *rc = 1;
            return NULL;
        }
        fputs("p4pc: automated settings worker nice 15\n", stderr);
    }

    /* Attach this thread to the JVM now, on its own stack. The 64-bit game runs on stacks in guest memory (the kernel model's threads), and ART's
     * first attach of a thread checks the stack pointer against the thread's own stack bounds (a native thread's SDL calls attach it lazily:
     * "Check failed: FindStackTop() > stack_end" and an abort). An attached thread is found by its TLS slot, whatever stack it runs on. */
    (void)SDL_GetAndroidJNIEnv();
    reserve_native_stack();
    if ((elf == NULL || elf[0] == '\0') && iso != NULL && log != NULL) {
        char *slash;

        snprintf(dest, sizeof dest, "%s", log);
        slash = strrchr(dest, '/');
        if (slash != NULL && (size_t)(slash - dest) + 14 < sizeof dest) {
            strcpy(slash + 1, "SLUS_217.82");
            if (extract_elf(iso, dest) == 0) {
                elf = dest;
            } else {
                fprintf(stderr, "p4pc: cannot read SLUS_217.82 from the disc image\n");
            }
        }
    }
    if (elf != NULL && elf[0] != '\0') {
        argv[1] = (char *)elf;
        argc = 2;
    }

    if (getenv("P4PC_PROGRAM") != NULL && strcmp(getenv("P4PC_PROGRAM"), "viewer") == 0) {
        /* P4PC_ARGS: the viewer's arguments after the executable ("title /sdcard/.../title.png"), space separated */
        static char words[1024];
        char *vargv[16] = {(char *)"p4pc_viewer", NULL};
        int n = 1;

        if (argc > 1) vargv[n++] = argv[1];
        snprintf(words, sizeof words, "%s", getenv("P4PC_ARGS") != NULL ? getenv("P4PC_ARGS") : "");
        for (char *w = strtok(words, " "); w != NULL && n < 15; w = strtok(NULL, " ")) {
            vargv[n++] = w;
        }
        *rc = p4pc_viewer_main(n, vargv);
        return NULL;
    }
    *rc = p4pc_main(argc, argv);
    return NULL;
}

int main(int argc, char **argv) {
    pthread_attr_t attr;
    pthread_t logger, game;
    int rc = 1;

    (void)argc;
    (void)argv;
    /* SDL reapplies its orientation policy when a paused surface returns. Keep
     * that policy consistent with P4Activity's landscape manifest declaration. */
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    if (pipe(sLogPipe) == 0) {
        dup2(sLogPipe[1], 2);
        dup2(sLogPipe[1], 1);
        setvbuf(stderr, NULL, _IOLBF, 0);
        pthread_create(&logger, NULL, log_thread, NULL);
    }
#ifdef P4_GUEST64
    /* The 64-bit model keeps the game's stacks in guest memory (the stack arena of the guest block), so the thread that runs the game is created
     * on that arena: ART judges every Java call and every JNI reference of a thread by the stack bounds the thread was attached with (a call
     * from a stack outside them fails with a StackOverflowError, and a reference to a string on it is "an invalid JNI transition frame
     * reference"), and SDL's Android calls are Java calls made from the game's stacks. docs/android.md, "The 64-bit build". */
    if (p4_guest_init() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "p4pc", "cannot reserve the guest block");
        return 1;
    }
    pthread_attr_init(&attr);
    if (pthread_attr_setstack(&attr, p4_guest_base + P4_GUEST_STACKS, P4_GUEST_STACKS_SIZE) != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "p4pc", "cannot use the stack arena as a stack");
        return 1;
    }
#else
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 16u << 20);
#endif
    if (pthread_create(&game, &attr, game_thread, &rc) != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "p4pc", "cannot start the game thread");
        return 1;
    }
    pthread_join(game, NULL);
    fflush(NULL);
    /* the game's end is the process's: leave through exit so the logger drains and the activity finishes */
    exit(rc);
}
