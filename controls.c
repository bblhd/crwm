
#include <sys/wait.h>
#include <unistd.h>

#include "main.h"
#include "controls.h"
#include "page.h"

void spawn(char **command);
void pages_switch(int n);
void pages_send(int n);

char *termcmd[] = { "st", NULL };
void open_terminal(void) {spawn(termcmd);}

char *menucmd[] = { "dmenu_run", NULL };
void open_menu(void) {spawn(menucmd);}

#define PAGEKEY(keysym, name) {MOD1, keysym, switchpage_##name}, {MOD1|MOD2, keysym, sendpage_##name}

void switchpage_1() {pages_switch(0);}
void switchpage_2() {pages_switch(1);}
void switchpage_3() {pages_switch(2);}
void switchpage_4() {pages_switch(3);}
void switchpage_5() {pages_switch(4);}
void switchpage_6() {pages_switch(5);}
void switchpage_7() {pages_switch(6);}
void switchpage_8() {pages_switch(7);}
void switchpage_9() {pages_switch(8);}

void sendpage_1() {pages_send(0);}
void sendpage_2() {pages_send(1);}
void sendpage_3() {pages_send(2);}
void sendpage_4() {pages_send(3);}
void sendpage_5() {pages_send(4);}
void sendpage_6() {pages_send(5);}
void sendpage_7() {pages_send(6);}
void sendpage_8() {pages_send(7);}
void sendpage_9() {pages_send(8);}

void killclient();
void closewm();
void move_up();
void move_down();
void move_left();
void move_right();

struct Key *keys = (struct Key[]) {
	{MOD1, 0xff52, move_up}, //0xff52 = XK_Up
	{MOD1, 0xff54, move_down}, //0xff54 = XK_Down
	{MOD1, 0xff51, move_left}, //0xff51 = XK_Left
	{MOD1, 0xff53, move_right}, //0xff53 = XK_Right
	{MOD1, 0xff0d, open_terminal}, //0xff0d = XK_Enter
	{MOD1, 0x0064, open_menu}, //0x0064 = XK_d
	{MOD1, 0x0071, killclient}, //0x0071 = XK_q
	{MOD1|MOD2, 0x0071, closewm}, //0x0071 = XK_q
	
	PAGEKEY(0x0031, 1), //0x0031 = XK_1
	PAGEKEY(0x0032, 2), //0x0032 = XK_2
	PAGEKEY(0x0033, 3), //0x0033 = XK_3
	PAGEKEY(0x0034, 4), //0x0034 = XK_4
	PAGEKEY(0x0035, 5), //0x0035 = XK_5
	PAGEKEY(0x0036, 6), //0x0036 = XK_6
	PAGEKEY(0x0037, 7), //0x0037 = XK_7
	PAGEKEY(0x0038, 8), //0x0038 = XK_8
	PAGEKEY(0x0039, 9), //0x0039 = XK_9
	
	{0, 0, NULL}
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

void pages_switch(int n) {
	page_unmap(mappedPage);
	mappedPage = &(pages[n]);
	page_map(mappedPage);
}

void pages_send(int n) {
	page_remove(mappedPage, focusedWindow);
	xcb_unmap_window(conn, focusedWindow);
	page_insertThrow(&(pages[n]), focusedWindow);
}

void spawn(char **command) {
	if (fork() == 0) {
		setsid();
		if (fork() != 0) _exit(0);
		execvp(command[0], command);
		_exit(0);
	}
	wait(NULL);
}

void killclient() {
	xcb_kill_client(conn, focusedWindow);
}

void closewm() {
	if (conn != NULL) xcb_disconnect(conn);
}

void move_down() {
	page_moveDown(mappedPage, focusedWindow);
	warpMouseToCenter(focusedWindow);
}

void move_up() {
	page_moveUp(mappedPage, focusedWindow);
	warpMouseToCenter(focusedWindow);
}

void move_right() {
	page_moveRight(mappedPage, focusedWindow);
	warpMouseToCenter(focusedWindow);
}

void move_left() {
	page_moveLeft(mappedPage, focusedWindow);
	warpMouseToCenter(focusedWindow);
}
