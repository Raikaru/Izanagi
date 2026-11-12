/* sebank_test: every sound effect bank on the disc through the format readers.
 *
 * For spu/comse, spu/bse and spu/test (.hd, .bd, .sq): the header parses; every VAG the sample
 * table lists lies inside the .bd, decodes to a non-empty sound and ends on its own end block
 * before the next VAG starts; every tone names an existing VAG; every sequence of the .sq reads to
 * its end marker without an event the reader does not know, and every note on / off names a
 * tone of the bank; ADSR conversion gives finite times. Exits 77 (CTest skip) unless P4PC_ISO
 * is set. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hd_bank.h"
#include "vag.h"
#include "vfs.h"

#define SKIP_RETURN 77

static int failures;

static void failf(const char *bank, const char *what, long a, long b)
{
    fprintf(stderr, "sebank_test: FAIL %s: %s (%ld, %ld)\n", bank, what, a, b);
    failures++;
}

static uint8_t *read_disc(const char *path, uint32_t *size)
{
    int fd = vfs_open(path, 0);
    long long end;
    uint8_t *buf;
    long long got = 0;

    if (fd < 0) {
        return NULL;
    }
    end = vfs_seek(fd, 0, 2);
    vfs_seek(fd, 0, 0);
    buf = malloc((size_t)end);
    while (buf != NULL && got < end) {
        int n = vfs_read(fd, buf + got, (int)(end - got > (1 << 20) ? (1 << 20) : end - got));

        if (n <= 0) {
            break;
        }
        got += n;
    }
    vfs_close(fd);
    if (got != end) {
        free(buf);
        return NULL;
    }
    *size = (uint32_t)end;
    return buf;
}

static void check_bank(const char *name)
{
    char path[64];
    uint32_t hd_size, bd_size, sq_size;
    uint8_t *hd, *bd, *sq;
    P4HdBank bank;
    P4Sq seq;
    uint32_t i, k, s;
    long notes = 0, sequences = 0, tones = 0;

    snprintf(path, sizeof path, "spu/%s.hd", name);
    hd = read_disc(path, &hd_size);
    snprintf(path, sizeof path, "spu/%s.bd", name);
    bd = read_disc(path, &bd_size);
    snprintf(path, sizeof path, "spu/%s.sq", name);
    sq = read_disc(path, &sq_size);
    if (hd == NULL || bd == NULL || sq == NULL) {
        failf(name, "file missing", hd != NULL, bd != NULL);
        free(hd);
        free(bd);
        free(sq);
        return;
    }
    if (p4hd_parse(hd, hd_size, &bank) != 0) {
        failf(name, ".hd does not parse", 0, 0);
        goto out;
    }
    if (p4sq_parse(sq, sq_size, &seq) != 0) {
        failf(name, ".sq does not parse", 0, 0);
        p4hd_free(&bank);
        goto out;
    }

    /* the sample table */
    for (i = 0; i < bank.nvags; i++) {
        P4VagInfo info;
        uint32_t next = i + 1 < bank.nvags ? bank.vags[i + 1].offset : bd_size;

        if (bank.vags[i].offset >= bd_size) {
            failf(name, "VAG outside the .bd", i, bank.vags[i].offset);
            continue;
        }
        if (bank.vags[i].rate < 4000 || bank.vags[i].rate > 96000) {
            failf(name, "VAG rate", i, bank.vags[i].rate);
        }
        if (p4vag_decode(bd + bank.vags[i].offset, bd_size - bank.vags[i].offset, NULL, &info) == 0) {
            failf(name, "VAG decodes to nothing", i, 0);
            continue;
        }
        if (bank.vags[i].offset + info.bytes > next && next > bank.vags[i].offset) {
            failf(name, "VAG runs into the next one", i, info.bytes);
        }
        if (info.bytes < 16 || (bd[bank.vags[i].offset + info.bytes - 15] & P4VAG_FLAG_END) == 0) {
            failf(name, "VAG without an end block", i, 0);
        }
    }
    /* tones */
    for (s = 0; s < bank.nsets; s++) {
        for (k = 0; k < bank.sets[s].ntones; k++) {
            P4SndEnv env;
            const P4HdTone *t = &bank.sets[s].tones[k];

            tones++;
            if (t->vag >= bank.nvags) {
                failf(name, "tone names a VAG the table lacks", s * 256 + k, t->vag);
                continue;
            }
            p4hd_adsr_to_env(t->adsr1, t->adsr2, &env);
            if (!isfinite(env.attack) || !isfinite(env.decay) || !isfinite(env.release) || env.attack < 0 ||
                env.release <= 0 || env.sustain <= 0.0f || env.sustain > 1.0f) {
                failf(name, "ADSR conversion", s * 256 + k, t->adsr1);
            }
        }
    }
    /* sequences */
    for (s = 0; s < seq.nsets; s++) {
        for (k = 0; k < seq.sets[s].nseqs; k++) {
            const P4SqSeq *q = &seq.sets[s].seqs[k];
            uint32_t pos = 0;
            P4SqEvent ev;
            int r;
            int ended = 0;

            sequences++;
            while ((r = p4sq_next_event(q, &pos, &ev)) > 0) {
                if (ev.type == P4SQ_END) {
                    ended = 1;
                    break;
                }
                if (ev.type == P4SQ_NOTE_ON || ev.type == P4SQ_NOTE_OFF) {
                    uint32_t ts = (uint32_t)ev.tone >> 8;
                    uint32_t tk = ev.tone & 0xff;

                    notes++;
                    if (ts >= bank.nsets || tk >= bank.sets[ts].ntones) {
                        failf(name, "note names a tone the bank lacks", s * 256 + k, ev.tone);
                    }
                }
                if (ev.type == P4SQ_META && ev.loop_kind >= 0 && (ev.loop_offset < 8 || ev.loop_offset - 8 >= q->len)) {
                    failf(name, "sequence loop jumps outside the sequence", s * 256 + k, ev.loop_offset);
                }
            }
            if (!ended) {
                failf(name, "sequence does not reach its end marker", s * 256 + k, r);
            }
        }
    }
    fprintf(stderr, "sebank_test: %s: %u VAGs, %u sets, %ld tones, %u sequence sets, %ld sequences, %ld notes\n", name,
            (unsigned)bank.nvags, (unsigned)bank.nsets, tones, (unsigned)seq.nsets, sequences, notes);
    p4sq_free(&seq);
    p4hd_free(&bank);
out:
    free(hd);
    free(bd);
    free(sq);
}

int main(void)
{
    const char *iso = getenv("P4PC_ISO");

    if (iso == NULL || iso[0] == '\0') {
        fprintf(stderr, "sebank_test: P4PC_ISO unset, skipped\n");
        return SKIP_RETURN;
    }
    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "sebank_test: cannot mount %s\n", iso);
        return 1;
    }
    check_bank("comse");
    check_bank("bse");
    check_bank("test");
    if (failures != 0) {
        return 1;
    }
    printf("sebank_test: ok\n");
    return 0;
}
