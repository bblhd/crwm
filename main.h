#ifndef CRWM_MAIN_H
#define CRWM_MAIN_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdlib.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0

#define DISREGARD(x) ((void)(x))

#define MOD1 XCB_MOD_MASK_4
#define MOD2 XCB_MOD_MASK_SHIFT

#define BORDER_WIDTH 1

#define PADDING_WIDTH 0
#define MARGIN_TOP 0
#define MARGIN_BOTTOM 0

#define DRAW_BAR 1 //1 for below, otherwise above
#define BAR_SIZE 20

extern xcb_connection_t *conn;
extern xcb_screen_t *screen;
extern xcb_drawable_t focused;

enum XcbAtomLabel {
	UTF8_STRING,
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_STATE,
	WM_TAKE_FOCUS,
	_NET_ACTIVE_WINDOW,
	_NET_SUPPORTED,
	_NET_WM_NAME,
	_NET_WM_STATE,
	_NET_SUPPORTING_WM_CHECK,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_CLIENT_LIST,
	ATOM_FINAL
};

extern xcb_atom_t atoms[ATOM_FINAL];

extern int doDrawBar;
void closeWM();

#endif
