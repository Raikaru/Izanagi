/* ffmpeg_license_test: reports the licence of the FFmpeg libraries the port links (third_party/README.md).
 *
 * Always passes and prints avcodec_license(), avutil_license(), swscale_license() and whether the build
 * configuration enabled GPL or nonfree code. With P4PC_REQUIRE_LGPL=1 (CMake -DP4_RELEASE=ON) it fails unless
 * all three libraries are LGPL-only: Debian builds FFmpeg with --enable-gpl, so a release must link an
 * LGPL-only build (./configure --disable-gpl --disable-nonfree). */
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int lgpl_only(const char *license) {
    return strncmp(license, "LGPL", 4) == 0;
}

int main(void) {
    const char *libs[3][2] = {{"libavcodec", avcodec_license()}, {"libavutil", avutil_license()}, {"libswscale", swscale_license()}};
    const char *conf = avcodec_configuration();
    int gpl_conf = strstr(conf, "--enable-gpl") != NULL || strstr(conf, "--enable-nonfree") != NULL ||
                   strstr(conf, "--enable-version3") != NULL;
    int bad = gpl_conf;
    for (int i = 0; i < 3; i++) {
        printf("ffmpeg_license_test: %s: %s\n", libs[i][0], libs[i][1]);
        if (!lgpl_only(libs[i][1])) bad = 1;
    }
    printf("ffmpeg_license_test: configuration enables gpl/nonfree/version3: %s\n", gpl_conf ? "yes" : "no");
    const char *req = getenv("P4PC_REQUIRE_LGPL");
    if (req && *req == '1' && bad) {
        fprintf(stderr, "ffmpeg_license_test: FAIL release build needs LGPL-only FFmpeg libraries\n");
        return 1;
    }
    return 0;
}
