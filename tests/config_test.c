/* The settings (src/platform/config/p4cfg.c): the INI parser, the environment's precedence over the file, the video policy that keeps the renderer gates
 * exact by default, and the first-run template. SDL is linked only for SDL_GetPrefPath (never called here: the tests name their text). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/platform/config/p4cfg.h"
#include "p4pc/os.h"

static int failures;

#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void unsetAll(void)
{
    static const char *const names[] = {"P4PC_FULLSCREEN", "P4PC_WINDOW_W", "P4PC_WINDOW_H", "P4PC_VSYNC", "P4PC_RESOLUTION", "P4PC_PRESENT_FIT", "P4PC_PRESENT", "P4PC_VOLUME",
                                        "P4PC_STICK_DEADZONE", "P4PC_ASPECT", "P4PC_MOVIE_FIT"};
    size_t i;

    for (i = 0; i < sizeof names / sizeof names[0]; i++) {
        p4_os_setenv(names[i], "", 1); /* an empty variable is no setting */
    }
}

int main(void)
{
    const P4CfgVideo *v;

    unsetAll();

    /* Parser: sections, comments, spaces, case of keys, a later duplicate wins. */
    p4cfg_parse_text("# a comment\n[Video]\n Resolution = scale:3 \n; another\nfit=integer\n[audio]\nvolume = 40.5\n[input]\nkb.cross = Z, Space\nbad line\n");
    CHECK(strcmp(p4cfg_str("video.resolution", ""), "scale:3") == 0);
    CHECK(p4cfg_video()->scale == 3);
    CHECK(strcmp(p4cfg_str("video.fit", ""), "integer") == 0);
    CHECK(p4cfg_float("audio.volume", 100.0) > 40.4 && p4cfg_float("audio.volume", 100.0) < 40.6);
    CHECK(strcmp(p4cfg_str("input.kb.cross", ""), "Z, Space") == 0);
    CHECK(p4cfg_get("video.width") == NULL);
    CHECK(p4cfg_int("video.width", 7) == 7);
    CHECK(p4cfg_int("audio.volume", 7) == 40);
    p4cfg_parse_text("[video]\nresolution = scale:2\nresolution = scale:4\nwidth = abc\n");
    CHECK(strcmp(p4cfg_str("video.resolution", ""), "scale:4") == 0);
    CHECK(p4cfg_int("video.width", 640) == 640); /* malformed: the default */

    /* The environment overrides the file. */
    p4_os_setenv("P4PC_RESOLUTION", "1920x1080", 1);
    CHECK(strcmp(p4cfg_str("video.resolution", ""), "1920x1080") == 0);
    p4_os_setenv("P4PC_RESOLUTION", "", 1);
    CHECK(strcmp(p4cfg_str("video.resolution", ""), "scale:4") == 0);
    p4cfg_parse_text("[video]\nresolution = scale:1 ; keep inline\nfit = 4:3 # keep fit\n"
        "[input]\nkb.cross = K, Space ; keep binding\n"
        "[future]\npath = C:/cards/a#b;v\nmarker = #literal\n");
    CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:1"));
    CHECK(!strcmp(p4cfg_str("video.fit", ""), "4:3"));
    CHECK(!strcmp(p4cfg_str("input.kb.cross", ""), "K, Space"));
    CHECK(!strcmp(p4cfg_str("future.path", ""), "C:/cards/a#b;v"));
    CHECK(!strcmp(p4cfg_str("future.marker", ""), "#literal"));

    /* Defaults: no file, no environment: the 640x448 window drawn into directly at scale 1 (what the gates measure). */
    p4cfg_clear();
    p4cfg_parse_text("");
    v = p4cfg_video();
#if !defined(__ANDROID__)
    CHECK(!v->blit && v->scale == 1 && !v->fullscreen && v->width == 640 && v->height == 448);
#endif
    CHECK(v->vsync == 2);
    CHECK(v->aspect == P4CFG_ASPECT_4_3);
    CHECK(v->resolution == P4CFG_RESOLUTION_SCALE);
    {
        P4CfgVideo candidate;
        p4cfg_parse_text("[video]\nresolution = scale:1\n");
        p4cfg_video_preview_resolution("1920x1080", &candidate);
        CHECK(candidate.resolution == P4CFG_RESOLUTION_EXPLICIT && candidate.blit);
        CHECK(candidate.renderWidth == 1920 && candidate.renderHeight == 1080);
        CHECK(!strcmp(p4cfg_str("video.resolution", ""), "scale:1"));
        v = p4cfg_video();
        CHECK(v->resolution == P4CFG_RESOLUTION_SCALE && v->scale == 1);
        p4cfg_video_preview_resolution("native", &candidate);
        CHECK(candidate.resolution == P4CFG_RESOLUTION_NATIVE && candidate.blit);
        CHECK(p4cfg_video() == v && v->resolution == P4CFG_RESOLUTION_SCALE);
        p4_os_setenv("P4PC_RESOLUTION", "scale:4", 1);
        p4cfg_video_preview_resolution("native", &candidate);
        CHECK(candidate.resolution == P4CFG_RESOLUTION_SCALE && candidate.scale == 4);
        p4_os_setenv("P4PC_RESOLUTION", "", 1);
        CHECK(v->scale == 1 && !strcmp(p4cfg_str("video.resolution", ""), "scale:1"));
    }

    p4cfg_clear();
    p4cfg_parse_text("[video]\nresolution = native\npresent = direct\n");
    CHECK(p4cfg_video()->resolution == P4CFG_RESOLUTION_NATIVE && p4cfg_video()->blit);
    {
        static const char *const sizes[] = {"1366x768", "1920x1080", "2560x1440", "3840x2160", "1x1"};
        static const int widths[] = {1366, 1920, 2560, 3840, 1};
        static const int heights[] = {768, 1080, 1440, 2160, 1};
        for (unsigned i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
            char text[128];
            snprintf(text, sizeof text, "[video]\nresolution=%s\n", sizes[i]);
            p4cfg_clear();
            p4cfg_parse_text(text);
            v = p4cfg_video();
            CHECK(v->resolution == P4CFG_RESOLUTION_EXPLICIT && v->blit);
            CHECK(v->renderWidth == widths[i] && v->renderHeight == heights[i]);
        }
    }
    {
        static const char *const invalid[] = {"0x1080", "-1x720", "1920x0", "1920x1080junk",
            "999999999999999999999x720", "16385x720", "nativejunk", "scale:0", "scale:9",
            "scale:2junk", "scale:02", "native#bad", "1920x1080;bad"};
        for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
            char text[128];
            snprintf(text, sizeof text, "[video]\nresolution=%s\n", invalid[i]);
            p4cfg_clear();
            p4cfg_parse_text(text);
            CHECK(p4cfg_video()->resolution == P4CFG_RESOLUTION_SCALE && p4cfg_video()->scale == 1);
        }
    }
    {
        static const char *const modes[] = {"native ; keep inline", "1920x1080 # keep inline", "scale:2 ; keep inline"};
        static const int kinds[] = {P4CFG_RESOLUTION_NATIVE, P4CFG_RESOLUTION_EXPLICIT, P4CFG_RESOLUTION_SCALE};
        for (unsigned i = 0; i < 3; i++) {
            char text[128];
            p4cfg_clear();
            snprintf(text, sizeof text, "[video]\nresolution=%s\n", modes[i]);
            p4cfg_parse_text(text);
            v = p4cfg_video();
            CHECK(v->resolution == kinds[i] && v->blit);
            if (i == 1) CHECK(v->renderWidth == 1920 && v->renderHeight == 1080);
            if (i == 2) CHECK(v->scale == 2);
        }
    }
    CHECK(v->movieFit == P4CFG_MOVIE_BARS);

    /* Aspect is a camera/layout request, independent of fit. It needs the
     * offscreen target even at the old window size; malformed values stay retail. */
    p4cfg_clear();
    p4cfg_parse_text("[video]\naspect = 16:9\npresent = direct\n");
    CHECK(p4cfg_video()->aspect == P4CFG_ASPECT_16_9 && p4cfg_video()->blit);
    p4cfg_clear();
    p4cfg_parse_text("[video]\naspect = auto\nfit = integer\n");
    CHECK(p4cfg_video()->aspect == P4CFG_ASPECT_AUTO && p4cfg_video()->blit);
    CHECK(p4cfg_video()->fit == P4CFG_FIT_INTEGER);
    p4cfg_clear();
    p4cfg_parse_text("[video]\naspect = invalid\n");
    CHECK(p4cfg_video()->aspect == P4CFG_ASPECT_4_3);
    p4_os_setenv("P4PC_ASPECT", "16:9", 1);
    p4cfg_clear();
    p4cfg_parse_text("[video]\naspect = 4:3\n");
    CHECK(p4cfg_video()->aspect == P4CFG_ASPECT_16_9);
    unsetAll();

    /* Movie policy is independent of camera aspect and defaults to bars.
     * The shorter, retail-letterboxed movie uses the same three choices. */
    p4cfg_clear();
    p4cfg_parse_text("[video]\nmovie_fit = zoom\n");
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_ZOOM);
    CHECK(p4cfg_video()->aspect == P4CFG_ASPECT_4_3);
    {
        const P4CfgVideo *live = p4cfg_video();
        P4CfgVideo candidate;
        p4cfg_video_preview_resolution("native", &candidate);
        CHECK(candidate.movieFit == P4CFG_MOVIE_ZOOM);
        CHECK(p4cfg_video() == live && live->movieFit == P4CFG_MOVIE_ZOOM);
    }
    p4cfg_clear();
    p4cfg_parse_text("[video]\nmovie_fit = stretch\n");
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_STRETCH);
    p4_os_setenv("P4PC_MOVIE_FIT", "bars", 1);
    p4cfg_clear();
    p4cfg_parse_text("[video]\nmovie_fit = stretch\n");
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_BARS);
    p4_os_setenv("P4PC_MOVIE_FIT", "zoom", 1);
    p4cfg_clear();
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_ZOOM);
    p4_os_setenv("P4PC_MOVIE_FIT", "stretch", 1);
    p4cfg_clear();
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_STRETCH);
    unsetAll();
    p4cfg_clear();
    p4cfg_parse_text("[video]\nmovie_fit = invalid\n");
    CHECK(p4cfg_video()->movieFit == P4CFG_MOVIE_BARS);

    /* A request that needs the offscreen picture selects it; direct is overridden with a message. */
    p4cfg_clear();
    p4cfg_parse_text("[video]\nresolution = scale:3\nfit = 4:3\n");
    v = p4cfg_video();
    CHECK(v->blit && v->scale == 3 && v->fit == P4CFG_FIT_4_3);
    p4cfg_clear();
    p4cfg_parse_text("[video]\npresent = direct\nresolution = scale:2\n");
    CHECK(p4cfg_video()->blit && p4cfg_video()->scale == 2);
    p4cfg_clear();
    p4cfg_parse_text("[video]\nwidth = 1280\nheight = 960\nfullscreen = 1\nvsync = off\nresolution = scale:8\n");
    v = p4cfg_video();
    CHECK(v->blit && v->width == 1280 && v->height == 960 && v->fullscreen && v->vsync == 0 && v->scale == 8);
    /* Direct at the plain size ignores the window knobs. */
    p4cfg_clear();
    p4cfg_parse_text("[video]\npresent = blit\nfit = integer\n");
    v = p4cfg_video();
    CHECK(v->blit && v->fit == P4CFG_FIT_INTEGER && v->scale == 1);

    /* Environment: P4PC_PRESENT=blit as the Android runs and the tools always spelled it. */
    p4cfg_clear();
    p4cfg_parse_text("");
    p4_os_setenv("P4PC_PRESENT", "blit", 1);
    p4_os_setenv("P4PC_PRESENT_FIT", "4:3", 1);
    v = p4cfg_video();
    CHECK(v->blit && v->fit == P4CFG_FIT_4_3 && v->scale == 1);
    unsetAll();

    /* The first-run file parses to the player's defaults, and is a blit setup. */
    p4cfg_clear();
    p4cfg_parse_text(p4cfg_template());
    CHECK(p4cfg_int("video.width", 0) == 1280 && p4cfg_int("video.height", 0) == 960);
    CHECK(strcmp(p4cfg_str("input.pad.cross", ""), "south") == 0);
    CHECK(strcmp(p4cfg_str("input.kb.up", ""), "Up, W") == 0);
    v = p4cfg_video();
    CHECK(v->blit && v->fit == P4CFG_FIT_4_3 && v->scale == 1 && !v->fullscreen);
    CHECK(v->aspect == P4CFG_ASPECT_4_3);
    CHECK(v->movieFit == P4CFG_MOVIE_BARS);

    if (failures == 0) {
        printf("config_test: ok\n");
    }
    return failures != 0;
}
