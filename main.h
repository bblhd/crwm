#ifndef CRWM_MAIN_H
#define CRWM_MAIN_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define DISREGARD(x) ((void)(x))

#define MOD1 XCB_MOD_MASK_4
#define MOD2 XCB_MOD_MASK_SHIFT
#define BORDER_WIDTH 1
#define BORDER_COLOR_UNFOCUSED 0x9eeeee /* 0xRRGGBB */
#define BORDER_COLOR_FOCUSED   0x55aaaa /* 0xRRGGBB */

extern xcb_connection_t *conn;

#endif
