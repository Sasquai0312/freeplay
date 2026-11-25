/* Host-only Standard MIDI File Type 1 exporter. Not for firmware linkage. */
#ifndef GENESIS_SMF_H
#define GENESIS_SMF_H

#include "genesis.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gen_smf_opts {
    uint16_t tpq;       /* ticks per quarter note (default 480) */
    uint32_t tempo_us;  /* microseconds per quarter (500000 = 120 BPM) */
} gen_smf_opts;

/* Write Type-1 SMF: tempo track + one note track.
 * Overlapping same pitch+channel: earlier note is truncated to the new note-on
 * (no stuck notes; documented in research/g2/08_midi_export.md). */
int gen_smf_write(FILE *fp, const gen_buf *buf, const gen_smf_opts *opts);
int gen_smf_write_path(const char *path, const gen_buf *buf, const gen_smf_opts *opts);

#ifdef __cplusplus
}
#endif
#endif
