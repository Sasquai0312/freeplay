/* Shared EVE display-list output. See dlw.h. */
#include "dlw.h"

static void eve_sync(struct dlw *d)
{
    fw_eve_end();
    while (!fw_eve_idle())
        ;
    fw_eve_begin();
    d->n = 0;
}

void dl(struct dlw *d, u32 w)
{
    if (d->n >= 640) /* keep each burst well inside the 4 KB command ring */
        eve_sync(d);
    fw_eve_word(w);
    fw_eve_wp = (u16)((fw_eve_wp + 4) & 0xfff);
    d->n++;
}

void text_f(struct dlw *d, int x, int y, u32 font, u32 color, u32 opts, const char *s)
{
    dl(d, DL_COLOR_RGB(color));
    if (d->n >= 600)
        eve_sync(d);
    fw_eve_text((u32)x & 0xffff, y, font, opts, s, 0);
    d->n += 3 + (slen(s) + 4) / 4;
}

void text(struct dlw *d, int x, int y, u32 color, u32 opts, const char *s)
{
    text_f(d, x, y, FONT_SMALL, color, opts, s);
}

void vline(struct dlw *d, int x16, int y0, int y1)
{
    dl(d, DL_VERTEX2F(x16, y0 * SUB));
    dl(d, DL_VERTEX2F(x16, y1 * SUB));
}
