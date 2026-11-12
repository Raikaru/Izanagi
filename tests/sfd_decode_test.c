/* sfd_decode_test: demux and decode a retail SFD through the port VFS and libavcodec.
 *
 * DATA/SFD/P4CT019.SFD is the disc's shortest cut-scene (16 s). The checks are values read from
 * its stream headers by p4pc_sfdview: 640x448 MPEG-1 video at 29.97 fps, 480 pictures, audio in
 * stream 0xC0 as 48 kHz stereo ADX (header 80 00, 18-byte blocks, 4-bit). Every picture must
 * decode, in display order, and a picture from the middle of the movie must not be black.
 * Exits 77 (CTest skip) when P4PC_ISO is unset. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpeg2_dec.h"
#include "sfd_demux.h"
#include "vfs.h"

#define SKIP_RETURN 77
#define MOVIE "DATA/SFD/P4CT019.SFD"
#define EXPECT_PICTURES 480
#define EXPECT_W 640
#define EXPECT_H 448
#define MID_PICTURE 240

static int failures;
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "sfd_decode_test: FAIL %s\n", what); failures++; }
}

int main(void) {
    const char *iso = getenv("P4PC_ISO");
    if (!iso || !*iso) { fprintf(stderr, "sfd_decode_test: P4PC_ISO is not set, skipping\n"); return SKIP_RETURN; }
    if (vfs_mount(iso, NULL) < 0) { fprintf(stderr, "sfd_decode_test: cannot mount %s\n", iso); return 1; }

    SfdDemux *dm = sfd_demux_open(MOVIE);
    check(dm != NULL, "open " MOVIE);
    P4Mpeg2 *dec = p4mpeg2_open();
    check(dec != NULL, "open the decoder");
    if (!dm || !dec) return 1;

    int pictures = 0, mid_ok = 0, order_ok = 1;
    int audio_head_ok = 0, video_pes = 0;
    int64_t prev_pts = INT64_MIN;
    double mid_mean = 0;
    SfdPacket pk;
    while (sfd_demux_next(dm, &pk)) {
        if (pk.stream_id == 0xC0 && !audio_head_ok) {
            /* ADX header: 80 00, copyright offset, encoding 3, block size 18, bits 4, 2 channels, 48000 Hz. */
            audio_head_ok = pk.size >= 12 && pk.data[0] == 0x80 && pk.data[1] == 0x00 && pk.data[4] == 3 &&
                            pk.data[5] == 18 && pk.data[6] == 4 && pk.data[7] == 2 &&
                            ((pk.data[8] << 24) | (pk.data[9] << 16) | (pk.data[10] << 8) | pk.data[11]) == 48000;
        }
        if (pk.stream_id != SFD_STREAM_VIDEO0) continue;
        video_pes++;
        check(p4mpeg2_feed(dec, pk.data, pk.size, pk.has_pts ? pk.pts : INT64_MIN) == 0, "feed");
        P4Mpeg2Frame f;
        while (p4mpeg2_receive(dec, &f)) {
            check(f.width == EXPECT_W && f.height == EXPECT_H, "picture size");
            if (f.pts != INT64_MIN) {
                if (prev_pts != INT64_MIN && f.pts < prev_pts) order_ok = 0;
                prev_pts = f.pts;
            }
            if (pictures == MID_PICTURE) {
                double sum = 0;
                for (int y = 0; y < f.height; y++)
                    for (int x = 0; x < f.width; x++) sum += f.plane[0][y * f.stride[0] + x];
                mid_mean = sum / ((double)f.width * f.height);
                mid_ok = 1;
            }
            pictures++;
        }
    }
    p4mpeg2_flush(dec);
    P4Mpeg2Frame f;
    while (p4mpeg2_receive(dec, &f)) pictures++;

    int num, den;
    p4mpeg2_frame_rate(dec, &num, &den);
    check(num == 30000 && den == 1001, "frame rate 30000/1001");
    check(!sfd_demux_failed(dm), "the demuxer read the whole movie");
    check(pictures == EXPECT_PICTURES, "480 pictures decoded");
    check(order_ok, "display-order PTS never goes backwards");
    check(audio_head_ok, "stream 0xC0 starts with a 48 kHz stereo ADX header");
    check(video_pes > 0, "video packets");
    check(mid_ok && mid_mean > 20.0, "the middle picture is not black (luma mean > 20)");
    printf("sfd_decode_test: %d pictures, %dx%d, rate %d/%d, middle-picture luma mean %.1f\n", pictures, EXPECT_W,
           EXPECT_H, num, den, mid_mean);

    p4mpeg2_close(dec);
    sfd_demux_close(dm);
    return failures ? 1 : 0;
}
