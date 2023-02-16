#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"
#include "controls.h"

xcb_key_symbols_t *keySymbols;

xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym) {
	return keySymbols ? xcb_key_symbols_get_keycode(keySymbols, keysym) : NULL;
}

xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
	return keySymbols ? xcb_key_symbols_get_keysym(keySymbols, keycode, 0) : 0;
}

void pages_switch(union Arg arg);
void pages_send(union Arg arg);
void spawn(union Arg arg);
void killclient(union Arg arg);
void closewm(union Arg arg);
void look(union Arg arg);
void move(union Arg arg);
void horizontalWeight(union Arg arg);
void verticalWeight(union Arg arg);
void flip(union Arg arg);
void send(union Arg arg);

#define PAGEKEY(keysym, n)\
	{MOD1, keysym, flip, {.u=n}},\
	{MOD1|MOD2, keysym, send, {.u=n}}

char *termcmd[] = { "st", NULL };
char *menucmd[] = { "dmenu_run", NULL };

struct Key *keys = (struct Key[]) {
	{MOD1, XK_Return, spawn, {.c=termcmd}},
	{MOD1, XK_d, spawn, {.c=menucmd}},
	{MOD1, XK_q, killclient, {0}},
	{MOD1|MOD2, XK_q, closewm, {0}},
	{MOD1, XK_r, reloadColors, {0}},
	
	{MOD1, XK_Left, look, {.u=1}},
	{MOD1, XK_Up, look, {.u=2}},
	{MOD1, XK_Right, look, {.u=3}},
	{MOD1, XK_Down, look, {.u=4}},
	
	{MOD1|MOD2, XK_Left, move, {.u=1}},
	{MOD1|MOD2, XK_Up, move, {.u=2}},
	{MOD1|MOD2, XK_Right, move, {.u=3}},
	{MOD1|MOD2, XK_Down, move, {.u=4}},
	
	{MOD1, XK_c, horizontalWeight, {.i=-1}},
	{MOD1, XK_v, verticalWeight, {.i=-1}},
	
	{MOD1|MOD2, XK_c, horizontalWeight, {.i=1}},
	{MOD1|MOD2, XK_v, verticalWeight, {.i=1}},
	
	PAGEKEY(XK_1, 0),
	PAGEKEY(XK_2, 1),
	PAGEKEY(XK_3, 2),
	PAGEKEY(XK_4, 3),
	PAGEKEY(XK_5, 4),
	PAGEKEY(XK_6, 5),
	PAGEKEY(XK_7, 6),
	PAGEKEY(XK_8, 7),
	PAGEKEY(XK_9, 8),
	
	{0, 0, NULL, {0}}
};


void setupControls() {
	keySymbols = xcb_key_symbols_alloc(conn);
	for (int i = 0; keys[i].func != NULL; i++) {
		xcb_keycode_t *keycode = xcb_get_keycodes(keys[i].keysym);
		if (keycode != NULL) {
			xcb_grab_key(conn, 1, screen->root, keys[i].mod, *keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		}
	}
}

void cleanupControls() {
	for (int i = 0; keys[i].func != NULL; i++) {
		xcb_keycode_t *keycode = xcb_get_keycodes(keys[i].keysym);
		if (keycode != NULL) {
			xcb_ungrab_key(conn, *keycode, screen->root, keys[i].mod);
		}
	}
	if (keySymbols) xcb_key_symbols_free(keySymbols);
}

void updateControls() {
	cleanupControls();
	setupControls();
}


void keybinding(uint16_t mod, xcb_keycode_t keycode) {
	xcb_keysym_t keysym = xcb_get_keysym(keycode);
	for (int i = 0; keys[i].func != NULL; i++) {
		if (mod == keys[i].mod && keysym == keys[i].keysym) {
			keys[i].func(keys[i].arg);
		}
	}
}

void spawn(union Arg arg) {
	if (fork() == 0) {
		if (fork() != 0) _exit(0);
		if (conn) close(xcb_get_file_descriptor(conn));
		setsid();
		execvp(arg.c[0], arg.c);
		_exit(0);
	}
	wait(NULL);
}

void killclient(union Arg arg) {
	DISREGARD(arg);
	if (!focused) return;
	if (atoms[WM_PROTOCOLS] && atoms[WM_DELETE_WINDOW]) {
		xcb_client_message_event_t ev;
		
		memset(&ev, 0, sizeof(ev));
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = focused;
		ev.format = 32;
		ev.type = atoms[WM_PROTOCOLS];
		ev.data.data32[0] = atoms[WM_DELETE_WINDOW];
		ev.data.data32[1] = XCB_CURRENT_TIME;
		
		xcb_send_event(conn, 0, focused, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
	} else {
		xcb_kill_client(conn, focused);
	}
}

void closewm(union Arg arg) {
	DISREGARD(arg);
	closeWM();
}

void warpMouseToCenter(xcb_window_t window) {
	uint16_t x, y;
	
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), NULL);
	x = geom->width >> 1;
	y = geom->height >> 1;
	free(geom);
	xcb_warp_pointer(conn, XCB_NONE, window, 0,0,0,0, x,y);
}

void look(union Arg arg) {
	if (!focused) return;
	switch(arg.u) {
		case 1: focused = lookLeft(focused); break;
		case 2: focused = lookUp(focused); break;
		case 3: focused = lookRight(focused); break;
		case 4: focused = lookDown(focused); break;
	}
	warpMouseToCenter(focused);
}

void move(union Arg arg) {
	if (!focused) return;
	switch(arg.u) {
		case 1: moveLeft(focused); break;
		case 2: moveUp(focused); break;
		case 3: moveRight(focused); break;
		case 4: moveDown(focused); break;
	}
}

void horizontalWeight(union Arg arg) {
	if (!focused) return;
	changeWeights(focused, arg.i, 0);
}

void verticalWeight(union Arg arg) {
	if (!focused) return;
	changeWeights(focused, 0, arg.i);
}

void send(union Arg arg) {
	if (!focused) return;
	sendPage(focused, arg.u);
}

void flip(union Arg arg) {
	if (focused) focused = 0;
	switchPage(arg.u);
	focused = 0;
}
