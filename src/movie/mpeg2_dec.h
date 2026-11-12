/* mpeg2_dec: MPEG-2 video decode on libavcodec (dynamic, LGPL; see third_party/README.md).
 *
 * Replaces the PS2 IPU's picture decode. Elementary-stream bytes go in as the demuxer produces
 * them (any split: the libavcodec parser finds the picture boundaries), decoded 4:2:0 pictures
 * come out in display order. */
#ifndef P4PC_MOVIE_MPEG2_DEC_H
#define P4PC_MOVIE_MPEG2_DEC_H

#include <stdint.h>

typedef struct P4Mpeg2 P4Mpeg2;

typedef struct P4Mpeg2Frame {
    int width, height;          /* coded picture size in luma samples */
    const uint8_t *plane[3];    /* Y, Cb, Cr (4:2:0: chroma planes are (w+1)/2 x (h+1)/2) */
    int stride[3];              /* bytes per row of each plane */
    int interlaced;             /* frame was coded as interlaced (progressive_frame == 0) */
    int top_field_first;
    int repeat_pict;            /* libavcodec's repeat_pict: extra field durations to show */
    int pict_type;              /* 1 I, 2 P, 3 B */
    int64_t pts;                /* 90 kHz PTS the packet carried (INT64_MIN if none) */
    int color_range;            /* 0 unspecified, 1 limited (MPEG), 2 full (JPEG) */
    int colorspace;             /* libavutil AVCOL_SPC_* */
    void *av;                   /* the AVFrame behind the planes (owned only by a frame from p4mpeg2_frame_hold) */
} P4Mpeg2Frame;

P4Mpeg2 *p4mpeg2_open(void);
void p4mpeg2_close(P4Mpeg2 *d);

/* Appends elementary-stream bytes. `pts` is the PTS of the PES packet they came from, or
 * INT64_MIN. Decodes any pictures that complete. Returns 0, or -1 on a libavcodec error. */
int p4mpeg2_feed(P4Mpeg2 *d, const uint8_t *data, int size, int64_t pts);
/* Drains the decoder at the end of the stream (call before the last p4mpeg2_receive loop). */
int p4mpeg2_flush(P4Mpeg2 *d);
/* Next decoded picture in display order. Returns 1 and fills *out (planes valid until the
 * next call into d), or 0 if none is ready. */
int p4mpeg2_receive(P4Mpeg2 *d, P4Mpeg2Frame *out);

/* An owned reference to a received picture: `out` has the same planes, which stay valid until p4mpeg2_frame_release whatever the
 * decoder does next (libavcodec reference counts the buffers: no copy). Returns 0 or -1. */
int p4mpeg2_frame_hold(const P4Mpeg2Frame *f, P4Mpeg2Frame *out);
void p4mpeg2_frame_release(P4Mpeg2Frame *f);

/* Frame rate libavcodec found in the sequence header, as num/den (0/0 before the first). */
void p4mpeg2_frame_rate(const P4Mpeg2 *d, int *num, int *den);

/* Decodes one unit of elementary stream (optional sequence/GOP headers, one picture, its
 * slices) without the parser, which would hold the picture back until the next start code.
 * Returns 0, or -1 on a libavcodec error. */
int p4mpeg2_send_unit(P4Mpeg2 *d, const uint8_t *data, int size);
/* Sequence end: drains the reference picture libavcodec holds for display reordering, then
 * resets the decoder for a new sequence. Returns 0 or -1. */
int p4mpeg2_end_sequence(P4Mpeg2 *d);
/* Drops every queued picture and resets the decoder. */
void p4mpeg2_reset(P4Mpeg2 *d);
/* Pictures decoded and not yet received. */
int p4mpeg2_pending(const P4Mpeg2 *d);

/* The PS2 IPU's picture layouts, which CRI's libmpeg hands to the game. Both order the
 * macroblocks column by column (all rows of column 0, then column 1, ...), (width+15)/16 columns
 * by (height+15)/16 rows. RGB32: 16x16 pixels of R,G,B,A bytes per macroblock (1024 bytes,
 * A = 0x80, IPU CSC). RAW8: 256 Y, 64 Cb, 64 Cr bytes per macroblock (384 bytes). Picture edges
 * the frame does not cover repeat the last row and column. dst must hold cols*rows macroblocks.
 * Returns 0 or -1. */
int p4mpeg2_to_ipu_rgb32(P4Mpeg2 *d, const P4Mpeg2Frame *f, uint8_t *dst);
int p4mpeg2_to_ipu_raw8(const P4Mpeg2Frame *f, uint8_t *dst);

/* The IPU RGB32 conversion (BT.601 limited range YCbCr to full range RGB, chroma replicated) straight to linear RGBA8: dst_stride
 * bytes per row, f->height rows, alpha 0xff. The pixels are those p4mpeg2_to_ipu_rgb32 puts in the macroblock columns (with the
 * alpha it stores replaced by 0xff), without the tiling. Returns 0 or -1. */
int p4mpeg2_frame_to_rgba_ipu(const P4Mpeg2Frame *f, uint8_t *dst, int dst_stride);


/* Picture buffers the player hands out without pixels (src/movie/mwply.c): the first bytes of such an image buffer name the held
 * picture it stands for. The game copies the buffer to its own image buffer before it uploads it, so the stamp travels with the copy,
 * and the upload (src/movie/movie_upload.c) asks the resolver for the picture. Returns NULL when `image` is not a stamped buffer. */
typedef const P4Mpeg2Frame *(*P4Mpeg2StampFn)(const void *image);
void p4mpeg2_set_stamp_resolver(P4Mpeg2StampFn fn);
const P4Mpeg2Frame *p4mpeg2_resolve_stamp(const void *image);

/* Converts a received frame to RGBA8 (BT.601 limited range unless the frame says otherwise),
 * w*h*4 bytes, top row first. Returns 0 or -1. */
int p4mpeg2_to_rgba(const P4Mpeg2Frame *f, uint8_t *rgba);

#endif
