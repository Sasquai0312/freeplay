/* Host-only SMF writer implementation + optional CLI when GEN_SMF_MAIN is defined. */
#include "genesis_smf.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t len, cap;
} bytebuf;

static int bb_reserve(bytebuf *b, size_t need)
{
    uint8_t *n;
    size_t nc;
    if (b->len + need <= b->cap)
        return 0;
    nc = b->cap ? b->cap * 2 : 256;
    while (nc < b->len + need)
        nc *= 2;
    n = (uint8_t *)realloc(b->data, nc);
    if (!n)
        return -1;
    b->data = n;
    b->cap = nc;
    return 0;
}

static int bb_u8(bytebuf *b, uint8_t v)
{
    if (bb_reserve(b, 1))
        return -1;
    b->data[b->len++] = v;
    return 0;
}

static int bb_vlq(bytebuf *b, uint32_t v)
{
    uint8_t stack[5];
    int n = 0;
    stack[n++] = (uint8_t)(v & 0x7f);
    v >>= 7;
    while (v) {
        stack[n++] = (uint8_t)((v & 0x7f) | 0x80);
        v >>= 7;
    }
    while (n--)
        if (bb_u8(b, stack[n]))
            return -1;
    return 0;
}

typedef struct {
    uint32_t tick;
    uint8_t status; /* 0x90 note on, 0x80 note off */
    uint8_t note;
    uint8_t vel;
    uint8_t ch;
} midiev;

static int midiev_cmp(const void *a, const void *b)
{
    const midiev *x = (const midiev *)a, *y = (const midiev *)b;
    if (x->tick != y->tick)
        return x->tick < y->tick ? -1 : 1;
    /* note-offs before note-ons at same tick to avoid zero-length issues */
    if (x->status != y->status)
        return x->status == 0x80 ? -1 : 1;
    if (x->ch != y->ch)
        return x->ch < y->ch ? -1 : 1;
    if (x->note != y->note)
        return x->note < y->note ? -1 : 1;
    return 0;
}

/* Truncate overlapping same pitch+channel: if a new note-on arrives while active, end previous. */
static size_t build_midievs(const gen_buf *buf, midiev **out)
{
    size_t i, n = 0, cap;
    midiev *evs;
    uint32_t active_end[16][128];
    memset(active_end, 0, sizeof active_end);
    cap = (size_t)buf->count * 2 + 8;
    evs = (midiev *)calloc(cap, sizeof(midiev));
    if (!evs)
        return 0;
    for (i = 0; i < buf->count; i++) {
        const gen_event *e = &buf->events[i];
        uint8_t ch = e->channel, note = e->note;
        uint32_t on = e->tick, off = e->tick + e->duration;
        if (active_end[ch][note] > on) {
            /* truncate previous sounding note */
            size_t k;
            for (k = 0; k < n; k++) {
                if (evs[k].status == 0x80 && evs[k].ch == ch && evs[k].note == note &&
                    evs[k].tick == active_end[ch][note]) {
                    evs[k].tick = on;
                    break;
                }
            }
        }
        if (n + 2 > cap) {
            midiev *ne = (midiev *)realloc(evs, (cap * 2) * sizeof(midiev));
            if (!ne) {
                free(evs);
                return 0;
            }
            evs = ne;
            cap *= 2;
        }
        evs[n].tick = on;
        evs[n].status = 0x90;
        evs[n].note = note;
        evs[n].vel = e->velocity;
        evs[n].ch = ch;
        n++;
        evs[n].tick = off;
        evs[n].status = 0x80;
        evs[n].note = note;
        evs[n].vel = 0;
        evs[n].ch = ch;
        n++;
        active_end[ch][note] = off;
    }
    qsort(evs, n, sizeof(midiev), midiev_cmp);
    *out = evs;
    return n;
}

static int write_chunk(FILE *fp, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t be[4] = {(uint8_t)(len >> 24), (uint8_t)(len >> 16), (uint8_t)(len >> 8), (uint8_t)len};
    if (fwrite(type, 1, 4, fp) != 4)
        return -1;
    if (fwrite(be, 1, 4, fp) != 4)
        return -1;
    if (len && fwrite(data, 1, len, fp) != len)
        return -1;
    return 0;
}

int gen_smf_write(FILE *fp, const gen_buf *buf, const gen_smf_opts *opts)
{
    gen_smf_opts o;
    bytebuf tempo = {0}, notes = {0};
    midiev *evs = 0;
    size_t ne, i;
    uint32_t last;
    uint8_t hdr[6];
    int rc = -1;

    if (!fp || !buf)
        return -1;
    o.tpq = opts && opts->tpq ? opts->tpq : 480;
    o.tempo_us = opts && opts->tempo_us ? opts->tempo_us : 500000;

    /* tempo track */
    if (bb_vlq(&tempo, 0) || bb_u8(&tempo, 0xFF) || bb_u8(&tempo, 0x51) || bb_u8(&tempo, 0x03) ||
        bb_u8(&tempo, (uint8_t)(o.tempo_us >> 16)) || bb_u8(&tempo, (uint8_t)(o.tempo_us >> 8)) ||
        bb_u8(&tempo, (uint8_t)o.tempo_us) || bb_vlq(&tempo, 0) || bb_u8(&tempo, 0xFF) ||
        bb_u8(&tempo, 0x2F) || bb_u8(&tempo, 0x00))
        goto done;

    ne = build_midievs(buf, &evs);
    if (!evs && buf->count)
        goto done;
    last = 0;
    for (i = 0; i < ne; i++) {
        uint32_t dt = evs[i].tick - last;
        last = evs[i].tick;
        if (bb_vlq(&notes, dt) || bb_u8(&notes, (uint8_t)(evs[i].status | (evs[i].ch & 0x0f))) ||
            bb_u8(&notes, evs[i].note) || bb_u8(&notes, evs[i].vel))
            goto done;
    }
    if (bb_vlq(&notes, 0) || bb_u8(&notes, 0xFF) || bb_u8(&notes, 0x2F) || bb_u8(&notes, 0x00))
        goto done;

    /* header: format 1, 2 tracks, tpq */
    hdr[0] = 0;
    hdr[1] = 1;
    hdr[2] = 0;
    hdr[3] = 2;
    hdr[4] = (uint8_t)(o.tpq >> 8);
    hdr[5] = (uint8_t)o.tpq;
    if (write_chunk(fp, "MThd", hdr, 6))
        goto done;
    if (write_chunk(fp, "MTrk", tempo.data, (uint32_t)tempo.len))
        goto done;
    if (write_chunk(fp, "MTrk", notes.data, (uint32_t)notes.len))
        goto done;
    rc = 0;
done:
    free(tempo.data);
    free(notes.data);
    free(evs);
    return rc;
}

int gen_smf_write_path(const char *path, const gen_buf *buf, const gen_smf_opts *opts)
{
    FILE *fp = fopen(path, "wb");
    int rc;
    if (!fp)
        return -1;
    rc = gen_smf_write(fp, buf, opts);
    fclose(fp);
    return rc;
}
