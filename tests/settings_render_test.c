/* Real GLES context/targets and production Resolution provider, without any UI
 * canvas. Native task/navigation coverage lives in settings_native_gate.sh. */
#include "guest_test.h"
#include "p4pc/os.h"
#include "p4pc/settings.h"
#include "p4pc/settings_native.h"
#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "../src/platform/config/p4cfg.h"
#include "../src/platform/pad/pad_sdl.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures, persistenceCalls;
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "settings_render: FAIL line %d: %s\n", __LINE__, #x); \
    ++failures; \
} } while (0)

static int persistResolution(const void *value)
{
    ++persistenceCalls;
    return p4cfg_set_str("video.resolution", value);
}
static void targetState(GLint state[8])
{
    p4gl_glGetIntegerv(0x8CAAu /* GL_READ_FRAMEBUFFER_BINDING */, state);
    p4gl_glGetIntegerv(0x8CA6u /* GL_DRAW_FRAMEBUFFER_BINDING */, state + 1);
    p4gl_glGetIntegerv(0x8069u /* GL_TEXTURE_BINDING_2D */, state + 2);
    p4gl_glGetIntegerv(0x8CA7u /* GL_RENDERBUFFER_BINDING */, state + 3);
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, state + 4);
}
static void targetSize(uint32_t width, uint32_t height)
{
    uint32_t w = 0, h = 0;
    int logicalW = 0, logicalH = 0;
    p4gl_readback_pixel_size(&w, &h);
    CHECK(w == width && h == height);
    CHECK(p4gl_target_size(&logicalW, &logicalH));
    CHECK(logicalW == 640 && logicalH == 448);
}
static void policySizes(void)
{
    static const char *const choices[] = {
        "1920x1080", "2560x1440", "3840x2160", "1366x768", "scale:4", "scale:2"
    };
    static const uint32_t widths[] = {1920, 2560, 3840, 1366, 2560, 1280};
    static const uint32_t heights[] = {1080, 1440, 2160, 768, 1792, 896};
    for (unsigned i = 0; i < sizeof choices / sizeof choices[0]; ++i) {
        CHECK(p4cfg_set_str("video.resolution", choices[i]) == 0);
        CHECK(p4gl_context_apply_settings() == 0);
        targetSize(widths[i], heights[i]);
    }
    CHECK(p4cfg_set_str("video.width", "1280") == 0);
    CHECK(p4cfg_set_str("video.height", "720") == 0);
    CHECK(p4cfg_set_str("video.fit", "4:3") == 0);
    CHECK(p4cfg_set_str("video.resolution", "native") == 0);
    CHECK(p4gl_context_apply_settings() == 0);
    {
        int drawableW = 0, drawableH = 0;
        uint32_t w = 0, h = 0;
        CHECK(SDL_GetWindowSizeInPixels(p4pc_pad_sdl_window(), &drawableW, &drawableH));
        p4gl_readback_pixel_size(&w, &h);
        CHECK(w == (uint32_t)((int64_t)drawableW * 3 > (int64_t)drawableH * 4 ?
            (int64_t)drawableH * 4 / 3 : drawableW));
        CHECK(h == (uint32_t)((int64_t)drawableW * 3 > (int64_t)drawableH * 4 ?
            drawableH : (int64_t)drawableW * 3 / 4));
    }
    CHECK(p4cfg_set_str("video.width", "1280") == 0);
    CHECK(p4cfg_set_str("video.height", "960") == 0);
    CHECK(p4cfg_set_str("video.fit", "stretch") == 0);
    CHECK(p4cfg_set_str("video.resolution", "scale:2") == 0);
    CHECK(p4gl_context_apply_settings() == 0);
    CHECK(p4gl_readback_configure(1, UINT32_MAX, 1) != 0);
    targetSize(1280, 896);
}
static void padDisplayNames(void)
{
    static const char *const assigned[] = {
        "leftshoulder", "rightshoulder", "leftstick", "rightstick"
    };
    static const char *const display[] = {"L1", "R1", "L3", "R3"};
    for (unsigned i = 0; i < sizeof assigned / sizeof assigned[0]; ++i) {
        char label[96], value[96];
        CHECK(p4cfg_set_str("input.pad.up", assigned[i]) == 0);
        CHECK(p4_settings_entry(41, label, sizeof label, value, sizeof value) == 0);
        CHECK(strcmp(value, display[i]) == 0);
        CHECK(strcmp(p4cfg_str("input.pad.up", ""), assigned[i]) == 0);
    }
    CHECK(p4cfg_set_str("input.pad.up", "dpup") == 0);
}
static void resolutionFailure(const char *ini)
{
    char blocker[1200];
    size_t beforeSize = 0, afterSize = 0;
    char *before = SDL_LoadFile(ini, &beforeSize);
    CHECK(before != NULL);
    if (!before) return;
    snprintf(blocker, sizeof blocker, "%s.tmp", ini);
    if (!SDL_CreateDirectory(blocker)) {
        CHECK(0 && "could not create private persistence blocker");
        SDL_free(before);
        return;
    }
    const GLuint framebuffer = p4gl_readback_framebuffer();
    uint32_t width = 0, height = 0;
    uint8_t pixel[4] = {0}, keptPixel[4] = {0};
    GLint beforeState[8], afterState[8];
    p4gl_readback_pixel_size(&width, &height);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    p4gl_glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT);
    p4gl_glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    p4gl_glViewport(3, 5, 17, 19);
    targetState(beforeState);

    persistenceCalls = 0;
    CHECK(p4gl_readback_configure_checked(1, UINT32_MAX, 1, persistResolution, "scale:3") != 0);
    CHECK(persistenceCalls == 0); /* Hardware rejection precedes persistence. */
    CHECK(p4gl_readback_configure_checked(1, width, height, persistResolution, "scale:3") != 0);
    CHECK(persistenceCalls == 1); /* Same-size targets still attempt the save. */

    /* The actual registered row-3 callback, not a substitute implementation.
     * Queueing alone must not persist, mutate GL state, or signal success. */
    p4_settings_model_adjust(3, 1);
    CHECK(p4_settings_model_feedback() == -1);
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:2"));
    targetState(afterState);
    CHECK(!memcmp(beforeState, afterState, sizeof beforeState));
    p4_settings_model_reset(); /* Native close/capture cleanup cannot strand it. */
    p4_settings_model_frame_end();
    targetState(afterState);
    CHECK(!memcmp(beforeState, afterState, sizeof beforeState));
    CHECK(p4gl_readback_framebuffer() == framebuffer);
    targetSize(width, height);
    CHECK(p4cfg_video()->resolution == P4CFG_RESOLUTION_SCALE && p4cfg_video()->scale == 2);
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    p4gl_glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, keptPixel);
    CHECK(!memcmp(pixel, keptPixel, sizeof pixel));
    CHECK(p4gl_glGetError() == GL_NO_ERROR);
    char *after = SDL_LoadFile(ini, &afterSize);
    CHECK(after && beforeSize == afterSize && !memcmp(before, after, beforeSize));
    SDL_free(after);
    SDL_free(before);
    CHECK(SDL_RemovePath(blocker));
    p4_settings_message("");
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    p4gl_readback_viewport();

    /* Accepted callbacks keep order/direction and execute exactly once. */
    p4_settings_model_adjust(3, 1);
    p4_settings_model_adjust(3, 1);
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:2"));
    p4_settings_model_reset();
    p4_settings_model_frame_end();
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:4"));
    targetSize(2560, 1792);
    p4_settings_model_frame_end();
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:4"));
    p4_settings_model_adjust(3, -1);
    p4_settings_model_adjust(3, -1);
    p4_settings_model_frame_end();
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:2"));
    targetSize(1280, 896);
}
static int registryValue, registryQueries;
static int registryCount(void *userdata, uint64_t *revision)
{
    (void)userdata;
    ++registryQueries;
    *revision = 1;
    return 0;
}
static int registryRow(void *userdata, int index, P4SettingsPageRow *row)
{
    (void)userdata; (void)index; (void)row;
    ++registryQueries;
    return -1; /* this is an actually empty catalog */
}
static int registryFind(void *userdata, const char *key)
{
    (void)userdata; (void)key;
    ++registryQueries;
    return -1;
}
static void registryAdjust(int direction, void *userdata)
{
    *(int *)userdata += direction;
}
static void registryFormat(char *value, size_t capacity, void *userdata)
{
    snprintf(value, capacity, "%d", *(int *)userdata);
}
static void pageRegistry(void)
{
    static char keys[P4_SETTINGS_PAGE_MAX][32];
    P4SettingsPage page = {.title = "Registry boundary", .count = registryCount,
        .row = registryRow, .find = registryFind};
    CHECK(p4_settings_page_register(NULL) == -1);
    CHECK(p4_settings_page_open("missing.page") == -1);
    CHECK(p4_settings_page_back() == -1);
    CHECK(p4_settings_page_close() == -1);
    for (int i = 0; i < P4_SETTINGS_PAGE_MAX; ++i) {
        snprintf(keys[i], sizeof keys[i], "registry.%d", i);
        page.key = keys[i];
        CHECK(p4_settings_page_register(&page) == 0);
    }
    CHECK(registryQueries == 0); /* registration never enters a provider */
    CHECK(p4_settings_page_register(&page) == -1);
    page.key = "registry.overflow";
    CHECK(p4_settings_page_register(&page) == -1);
    CHECK(p4_settings_page_open(keys[0]) == 0);
    CHECK(p4_settings_page_unregister(keys[0]) == 0);
    CHECK(p4_settings_page_open(keys[1]) == 0); /* pending ref/request was cancelled */
    for (int i = 1; i < P4_SETTINGS_PAGE_MAX; ++i)
        CHECK(p4_settings_page_unregister(keys[i]) == 0);
    page.title = "Bad\ntitle";
    CHECK(p4_settings_page_register(&page) == -1);
    page.title = "\xc0\x80"; /* MSG alternate style is not a raw Config glyph. */
    CHECK(p4_settings_page_register(&page) == -1);
    page.title = "\xf1\x81"; /* BMD command, not a native glyph. */
    CHECK(p4_settings_page_register(&page) == -1);
    page.title = "\x83"; /* incomplete native pair */
    CHECK(p4_settings_page_register(&page) == -1);
    page.title = "Native\x83\xf6space";
    CHECK(p4_settings_page_register(&page) == 0);
    CHECK(p4_settings_page_unregister(page.key) == 0);
    char oversized[P4_SETTINGS_PAGE_LABEL_MAX];
    memset(oversized, 'x', sizeof oversized);
    page.title = oversized;
    CHECK(p4_settings_page_register(&page) == -1);
    CHECK(p4_settings_page_unregister(keys[0]) == -1);
    CHECK(p4_settings_page_active() == NULL && registryQueries == 0);

    int before = p4_settings_entry_count(), added = 0;
    P4SettingsEntry entry = {.label = "Runtime registration", .format = registryFormat,
        .adjust = registryAdjust, .userdata = &registryValue};
    while (added < P4_SETTINGS_EXTENSION_MAX && !p4_settings_register(&entry)) ++added;
    CHECK(p4_settings_entry_count() == P4_SETTINGS_MODEL_BUILTINS + P4_SETTINGS_EXTENSION_MAX);
    CHECK(added == P4_SETTINGS_MODEL_BUILTINS + P4_SETTINGS_EXTENSION_MAX - before);
    CHECK(p4_settings_register(&entry) == -1);
    int index = p4_settings_entry_count() - 1;
    p4_settings_model_adjust(index, 1);
    char label[192], value[128];
    CHECK(p4_settings_entry(index, label, sizeof label, value, sizeof value) == 0);
    CHECK(!strcmp(label, "Runtime registration") && !strcmp(value, "1"));
}
int main(int argc, char **argv)
{
    char ini[1100];
    snprintf(ini, sizeof ini, "%s.%llu.ini", argc > 1 ? argv[1] : "settings_render",
        (unsigned long long)p4_os_now_ns());
    FILE *file = fopen(ini, "wb");
    if (!file) { perror(ini); return 1; }
    int written = fputs("# Keep this comment\n[video]\npresent = blit\nwidth = 1280\nheight = 960\n"
        "aspect = 4:3\nresolution = scale:2 ; keep inline\nunknown = untouched\n", file) >= 0;
    if (fclose(file)) written = 0;
    if (!written) return 1;
    p4_os_setenv("P4PC_INPUT", "script", 1);
    p4_os_setenv("SDL_AUDIODRIVER", "dummy", 1);
    p4_os_setenv("P4PC_CONFIG", ini, 1);
    p4_os_setenv("P4PC_HIDDEN_WINDOW", "1", 1);
    static const char *const overrides[] = {
        "P4PC_RESOLUTION", "P4PC_SCALE", "P4PC_ASPECT", "P4PC_FULLSCREEN", "P4PC_WINDOW_W",
        "P4PC_WINDOW_H", "P4PC_PRESENT", "P4PC_PRESENT_FIT", "P4PC_VSYNC"
    };
    for (unsigned i = 0; i < sizeof overrides / sizeof overrides[0]; ++i)
        p4_os_setenv(overrides[i], "", 1);
    p4cfg_clear();
    if (p4t_guest_map(0x00764000u, 0x1000u)) return 1;
    if (p4gl_context_open()) {
        fprintf(stderr, "settings_render: no GL context: %s\n", SDL_GetError());
        return 77;
    }
    CHECK((p4_settings_entry_flags(0) & (P4_SETTINGS_DESKTOP | P4_SETTINGS_TOGGLE)) ==
        (P4_SETTINGS_DESKTOP | P4_SETTINGS_TOGGLE));
    CHECK(p4_settings_entry_flags(3) == 0); /* real provider inherits general availability */
    CHECK(p4_settings_entry_flags(9) == P4_SETTINGS_KEYBOARD);
    CHECK(p4_settings_entry_flags(41) == 0);
    CHECK(!strcmp(p4_settings_model_description(0), "Fullscreen ON/OFF."));
    CHECK(!strcmp(p4_settings_model_description(3), "Choose render size."));
    CHECK(!strcmp(p4_settings_model_description(9), "Confirm: bind. Left: clear."));
    {
        int aspectFound = 0;
        for (int index = P4_SETTINGS_MODEL_BUILTINS; index < p4_settings_entry_count(); ++index) {
            char label[96], value[96];
            p4_settings_entry(index, label, sizeof label, value, sizeof value);
            if (!strcmp(label, "Aspect ratio")) {
                CHECK(!strcmp(p4_settings_model_description(index), "Choose field/battle framing."));
                aspectFound = 1;
            }
        }
        CHECK(aspectFound);
    }
    CHECK(!p4_settings_entry_available(-1));
    CHECK(!p4_settings_entry_available(p4_settings_entry_count()));
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    for (int i = 0; i < P4_SETTINGS_MODEL_BUILTINS; ++i)
        CHECK(p4_settings_entry_available(i));
#endif
    padDisplayNames();
    policySizes();
    resolutionFailure(ini);
    pageRegistry();
    CHECK(p4gl_glGetError() == GL_NO_ERROR);
    p4gl_context_close();
    fprintf(stderr, "settings_render: %s (real policies, transactional target/config rollback, ordered deferred callback)\n",
        failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
