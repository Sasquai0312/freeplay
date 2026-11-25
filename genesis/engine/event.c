#include "genesis.h"

void gen_buf_init(gen_buf *b, gen_event *storage, uint32_t capacity)
{
    b->events = storage;
    b->capacity = capacity;
    b->count = 0;
}

void gen_buf_clear(gen_buf *b)
{
    b->count = 0;
}

gen_status gen_buf_push(gen_buf *b, const gen_event *ev)
{
    if (!b || !b->events || !ev)
        return GEN_ERR_PARAM;
    if (b->count >= b->capacity)
        return GEN_ERR_CAPACITY;
    if (ev->note > 127 || ev->channel > 15)
        return GEN_ERR_PITCH;
    if (ev->velocity == 0 || ev->duration == 0)
        return GEN_ERR_TIMING;
    b->events[b->count++] = *ev;
    return GEN_OK;
}

static int cmp_ev(const gen_event *a, const gen_event *b)
{
    if (a->tick != b->tick)
        return a->tick < b->tick ? -1 : 1;
    if (a->channel != b->channel)
        return a->channel < b->channel ? -1 : 1;
    if (a->note != b->note)
        return a->note < b->note ? -1 : 1;
    return 0;
}

gen_status gen_buf_sort(gen_buf *b)
{
    uint32_t i, j;
    if (!b || !b->events)
        return GEN_ERR_PARAM;
    /* insertion sort — stable, no alloc, fine for bounded buffers */
    for (i = 1; i < b->count; i++) {
        gen_event key = b->events[i];
        j = i;
        while (j > 0 && cmp_ev(&b->events[j - 1], &key) > 0) {
            b->events[j] = b->events[j - 1];
            j--;
        }
        b->events[j] = key;
    }
    return GEN_OK;
}

gen_status gen_buf_validate(const gen_buf *b, int monophonic)
{
    uint32_t i, j;
    if (!b || !b->events)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        if (e->note > 127 || e->channel > 15)
            return GEN_ERR_PITCH;
        if (e->velocity < 1 || e->velocity > 127)
            return GEN_ERR_PARAM;
        if (e->duration == 0)
            return GEN_ERR_TIMING;
        if (e->tick > UINT32_MAX - e->duration)
            return GEN_ERR_OVERFLOW;
    }
    if (!monophonic)
        return GEN_OK;
    for (i = 0; i < b->count; i++) {
        for (j = i + 1; j < b->count; j++) {
            const gen_event *a = &b->events[i], *c = &b->events[j];
            uint32_t a_end, c_end;
            if (a->channel != c->channel)
                continue;
            a_end = a->tick + a->duration;
            c_end = c->tick + c->duration;
            if (a->tick < c_end && c->tick < a_end)
                return GEN_ERR_MONO;
        }
    }
    return GEN_OK;
}
