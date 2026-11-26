/* G3B Genesis native UI — non-destructive preview only (no production Apply). */
#ifndef GENESIS_UI_H
#define GENESIS_UI_H

#include "cfw.h"

#ifdef CFW_MOD
void cfw_menu_build_genesis(void *ctx, void *menu);
void cfw_genesis_enter(void *scr);
void cfw_genesis_tick(void *app);
int cfw_genesis_input(u32 kind, u32 id, u32 val);
int cfw_genesis_render(void *w);
const u8 *cfw_genesis_led(u32 led);
#endif

#endif
