
#include <sys/wait.h>
#include <unistd.h>

#include "main.h"
#include "controls.h"
#include "page.h"

void spawn(char **command) {
	if (fork() == 0) {
		setsid();
		if (fork() != 0) _exit(0);
		execvp(command[0], command);
		_exit(0);
	}
	wait(NULL);
}
void killclient(void) {
	xcb_kill_client(conn, focusedWindow);
}

void closewm(void) {
	if (conn != NULL) xcb_disconnect(conn);
}


char *termcmd[] = { "st", NULL };
void open_terminal(void) {
	spawn(termcmd);
}

char *menucmd[] = { "rc", "/home/awi/scripts/dmenu.rc", NULL };
void open_menu(void) {
	spawn(menucmd);
}
void move_down(void) {
	page_moveDown(&testpage, focusedWindow);
}
void move_up(void) {
	page_moveUp(&testpage, focusedWindow);
}
void move_right(void) {
	page_moveRight(&testpage, focusedWindow);
}
void move_left(void) {
	page_moveLeft(&testpage, focusedWindow);
}

struct Key *keys = (struct Key[]) {
	{MOD1, 0xff52, move_up}, //0xff52 = XK_Up
	{MOD1, 0xff54, move_down}, //0xff54 = XK_Down
	{MOD1, 0xff51, move_left}, //0xff51 = XK_Left
	{MOD1, 0xff53, move_right}, //0xff53 = XK_Right
	{MOD1, 0xff0d, open_terminal}, //0xff0d = XK_Enter
	{MOD1, 0x0064, open_menu}, //0x0064 = XK_d
	{MOD1, 0x0071, killclient}, //0x0071 = XK_q
	{MOD1|MOD2, 0x0071, closewm}, //0x0071 = XK_q
	{0, 0, NULL}
};
