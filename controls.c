
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "main.h"
#include "controls.h"
#include "page.h"

char *termcmd[] = { "st", NULL };
char *menucmd[] = { "dmenu_run", NULL };

#define PAGEKEY(keysym, n)\
	{MOD1, keysym, pages_switch, {.u=n}},\
	{MOD1|MOD2, keysym, pages_send, {.u=n}}

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
	
	{MOD1, 0xff52, move, {.p=(void *) page_moveUp}}, //0xff52 = XK_Up
	{MOD1, 0xff54, move, {.p=(void *) page_moveDown}}, //0xff54 = XK_Down
	{MOD1, 0xff51, move, {.p=(void *) page_moveLeft}}, //0xff51 = XK_Left
	{MOD1, 0xff53, move, {.p=(void *) page_moveRight}}, //0xff53 = XK_Right
	
	{MOD1, 0x0076, grow_vertical, {-1}}, //0xff52 = XK_v
	{MOD1|MOD2, 0x0076, grow_vertical, {1}}, //0xff54 = XK_v
	{MOD1, 0x0063, grow_horizontal, {-1}}, //0xff51 = XK_c
	{MOD1|MOD2, 0x0063, grow_horizontal, {1}}, //0xff53 = XK_c
	
	PAGEKEY(0x0031, 0), //0x0031 = XK_1
	PAGEKEY(0x0032, 1), //0x0032 = XK_2
	PAGEKEY(0x0033, 2), //0x0033 = XK_3
	PAGEKEY(0x0034, 3), //0x0034 = XK_4
	PAGEKEY(0x0035, 4), //0x0035 = XK_5
	PAGEKEY(0x0036, 5), //0x0036 = XK_6
	PAGEKEY(0x0037, 6), //0x0037 = XK_7
	PAGEKEY(0x0038, 7), //0x0038 = XK_8
	PAGEKEY(0x0039, 8), //0x0039 = XK_9
	
	{0, 0, NULL, {0}}
};


struct Page *mappedPage = NULL;
struct Page pages[9];

void pages_init() {
	for (int i = 0; i < 9; i++) {
		page_init(&(pages[i]));
	}
	mappedPage = &(pages[0]);
}

void pages_fini() {
	for (int i = 0; i < 9; i++) {
		page_free(&(pages[i]));
	}
}

void pages_switch(union Arg arg) {
	if (mappedPage == &pages[arg.u]) return;
	page_unmap(mappedPage);
	mappedPage = &pages[arg.u];
	page_map(mappedPage);
}

void pages_send(union Arg arg) {
	if (mappedPage == &pages[arg.u]) return;
	page_remove(mappedPage, focusedWindow);
	xcb_unmap_window(conn, focusedWindow);
	page_insertThrow(&pages[arg.u], focusedWindow);
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

xcb_atom_t xcb_atom_get(char *name) {
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn,0,strlen(name),name), NULL);
	xcb_atom_t atom = reply ? reply->atom : XCB_NONE;
	free(reply);
	return atom;
}

void killclient(union Arg arg) {
	DISREGARD(arg);
	xcb_client_message_event_t ev;
	
	//todo: get atoms in setup and store permanantly
	memset(&ev, 0, sizeof(ev));
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.window = focusedWindow;
	ev.format = 32;
	ev.data.data32[1] = XCB_CURRENT_TIME;
	ev.type = xcb_atom_get("WM_PROTOCOLS");
	ev.data.data32[0] = xcb_atom_get("WM_DELETE_WINDOW");
	
	xcb_send_event(conn, 0, focusedWindow, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

void closewm(union Arg arg) {
	DISREGARD(arg);
	if (conn != NULL) xcb_disconnect(conn);
}

void move(union Arg arg) {
	void (*movefunc)(struct Page *, xcb_drawable_t) = ((void (*)(struct Page *, xcb_drawable_t)) (arg.p));
	movefunc(mappedPage, focusedWindow);
	warpMouseToCenter(focusedWindow);
}

void grow_vertical(union Arg arg) {
	page_changeRowWeight(mappedPage, focusedWindow, arg.i);
	warpMouseToCenter(focusedWindow);
}

void grow_horizontal(union Arg arg) {
	page_changeColumnWeight(mappedPage, focusedWindow, arg.i);
	warpMouseToCenter(focusedWindow);
}
