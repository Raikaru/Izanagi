/* MPEG program-stream demuxer over the PC VFS, shared by the player and tools. */
#ifndef P4PC_MOVIE_SFD_DEMUX_H
#define P4PC_MOVIE_SFD_DEMUX_H

#include <stdint.h>

/* Stream ids (ISO 13818-1). */
#define SFD_STREAM_PRIVATE1 0xBD
#define SFD_STREAM_VIDEO0 0xE0

typedef struct SfdDemux SfdDemux;

typedef struct SfdPacket {
    int stream_id;         /* PES stream id (0xE0 video, 0xBD private stream 1, ...) */
    const uint8_t *data;   /* PES payload; valid until the next sfd_demux_next call */
    int size;
    int has_pts;           /* a PTS accompanied this packet */
    int64_t pts;           /* 90 kHz */
    int has_dts;
    int64_t dts;
    int64_t scr;           /* system clock reference of the pack holding this packet, 27 MHz / 300 = 90 kHz */
    long long offset;      /* byte offset of the PES start code in the file */
} SfdPacket;

/* Opens a path in the port VFS. Returns NULL if it does not open. */
SfdDemux *sfd_demux_open(const char *path);
/* Next PES packet: 1 packet, 0 at the end of the stream (see sfd_demux_failed).
 * Pack, system and padding packets are consumed silently. */
int sfd_demux_next(SfdDemux *d, SfdPacket *out);
/* 1 if the pack headers seen so far were MPEG-2 (`01` marker), 0 for MPEG-1, -1 before any. */
int sfd_demux_is_mpeg2(const SfdDemux *d);
/* Mux rate from the last pack header, in 50 byte/s units (0 before any pack). */
int sfd_demux_mux_rate(const SfdDemux *d);
/* 1 if a file read error, not the end of the file, ended the stream. */
int sfd_demux_failed(const SfdDemux *d);
void sfd_demux_close(SfdDemux *d);

#endif
