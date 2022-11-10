#ifndef CRWM_MAIN_H
#define CRWM_MAIN_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdint.h>
#include <stdbool.h>

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

#endif
