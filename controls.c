#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"
#include "controls.h"

#define PAGEKEY(keysym, n)\
	{MOD1, keysym, pages_switch, {.u=n}},\
	{MOD1|MOD2, keysym, pages_send, {.u=n}}

char *termcmd[] = { "st", NULL };
char *menucmd[] = { "dmenu_run", NULL };

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
void move(union Arg arg);
void grow_vertical(union Arg arg);
void grow_horizontal(union Arg arg);

struct Key *keys = (struct Key[]) {
	{MOD1, 0xff0d, spawn, {.c=termcmd}}, //0xff0d = XK_Enter
	{MOD1, 0x0064, spawn, {.c=menucmd}}, //0x0064 = XK_d
	{MOD1, 0x0071, killclient, {0}}, //0x0071 = XK_q
	{MOD1|MOD2, 0x0071, closewm, {0}}, //0x0071 = XK_q
	
	{MOD1|MOD2, 0xff52, move, {.u=1}}, //0xff52 = XK_Up
	{MOD1|MOD2, 0xff53, move, {.u=2}}, //0xff53 = XK_Right
	{MOD1|MOD2, 0xff54, move, {.u=3}}, //0xff54 = XK_Down
	{MOD1|MOD2, 0xff51, move, {.u=4}}, //0xff51 = XK_Left
	
	//{MOD1, 0x0076, grow_vertical, {-1}}, //0xff52 = XK_v
	//{MOD1|MOD2, 0x0076, grow_vertical, {1}}, //0xff54 = XK_v
	//{MOD1, 0x0063, grow_horizontal, {-1}}, //0xff51 = XK_c
	//{MOD1|MOD2, 0x0063, grow_horizontal, {1}}, //0xff53 = XK_c
	
	//PAGEKEY(0x0031, 0), //0x0031 = XK_1
	//PAGEKEY(0x0032, 1), //0x0032 = XK_2
	//PAGEKEY(0x0033, 2), //0x0033 = XK_3
	//PAGEKEY(0x0034, 3), //0x0034 = XK_4
	//PAGEKEY(0x0035, 4), //0x0035 = XK_5
	//PAGEKEY(0x0036, 5), //0x0036 = XK_6
	//PAGEKEY(0x0037, 6), //0x0037 = XK_7
	//PAGEKEY(0x0038, 7), //0x0038 = XK_8
	//PAGEKEY(0x0039, 8), //0x0039 = XK_9
	
	{0, 0, NULL, {0}}
};


void setupControls() {
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
	
	keySymbols = xcb_key_symbols_alloc(conn);
	for (int i = 0; keys[i].func != NULL; i++) {
		xcb_keycode_t *keycode = xcb_get_keycodes(keys[i].keysym);
		if (keycode != NULL) {
			xcb_grab_key(
				conn, 1, screen->root,
				keys[i].mod, *keycode,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC
			);
		}
	}
}
void updateControls() {
	if (keySymbols) xcb_key_symbols_free(keySymbols);
	setupControls();
}

void cleanupControls() {
	if (keySymbols) xcb_key_symbols_free(keySymbols);
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
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
		setsid();
		if (fork() != 0) _exit(0);
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

void move(union Arg arg) {
	if (!focused) return;
	if (arg.i==1) moveUp(focused);
	if (arg.i==2) moveRight(focused);
	if (arg.i==3) moveDown(focused);
	if (arg.i==4) moveLeft(focused);
}
