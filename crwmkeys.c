#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

void addKeybinding(unsigned int mod, xcb_keysym_t sym, char *command);

struct Keybinding {
	struct Keybinding *next;
	unsigned int mod;
	xcb_keysym_t sym;
	xcb_keycode_t code;
	char *command;
} *keys = NULL;

xcb_connection_t *conn;
xcb_window_t root;
int shouldClose = 0;

void endprogram(int sig) {
	(void) sig;
	shouldClose = 1;
}

void msleep(unsigned long tms) {
    struct timespec ts;
    ts.tv_sec = tms / 1000;
    ts.tv_nsec = (tms % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

void die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}

void spawn(char **command) {
	if (fork() == 0) {
		if (fork() != 0) _exit(0);
		//setsid();
		execvp(command[0], command);
		_exit(0);
	}
	wait(NULL);
}

int eventHandler();
void setupControls();
void cleanupControls();

#define PRIMARY_MOD XCB_MOD_MASK_4
#define SECONDARY_MOD (PRIMARY_MOD|XCB_MOD_MASK_SHIFT)

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		die("Couldn't connect to X.\n");
	}
	root = xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;
	
	addKeybinding(PRIMARY_MOD, XK_d, "dmenu_run");
	addKeybinding(PRIMARY_MOD, XK_Return, "st");
	
	addKeybinding(PRIMARY_MOD, XK_q, "crwmctl close focused");
	addKeybinding(PRIMARY_MOD | XCB_MOD_MASK_SHIFT, XK_q, "crwmctl exit");
	
	addKeybinding(PRIMARY_MOD, XK_Left, "crwmctl look left");
	addKeybinding(PRIMARY_MOD, XK_Right, "crwmctl look right");
	addKeybinding(PRIMARY_MOD, XK_Up, "crwmctl look up");
	addKeybinding(PRIMARY_MOD, XK_Down, "crwmctl look down");
	
	addKeybinding(SECONDARY_MOD, XK_Left, "crwmctl move focused left");
	addKeybinding(SECONDARY_MOD, XK_Right, "crwmctl move focused right");
	addKeybinding(SECONDARY_MOD, XK_Up, "crwmctl move focused up");
	addKeybinding(SECONDARY_MOD, XK_Down, "crwmctl move focused down");
	
	addKeybinding(PRIMARY_MOD, XK_c, "crwmctl shrink focused horizontally by 1");
	addKeybinding(PRIMARY_MOD, XK_v, "crwmctl shrink focused vertically by 1");
	addKeybinding(SECONDARY_MOD, XK_c, "crwmctl grow focused horizontally by 1");
	addKeybinding(SECONDARY_MOD, XK_v, "crwmctl grow focused vertically by 1");
	
	addKeybinding(PRIMARY_MOD, XK_1, "crwmctl switch to 1");
	addKeybinding(PRIMARY_MOD, XK_2, "crwmctl switch to 2");
	addKeybinding(PRIMARY_MOD, XK_3, "crwmctl switch to 3");
	addKeybinding(PRIMARY_MOD, XK_4, "crwmctl switch to 4");
	addKeybinding(PRIMARY_MOD, XK_5, "crwmctl switch to 5");
	addKeybinding(PRIMARY_MOD, XK_6, "crwmctl switch to 6");
	addKeybinding(PRIMARY_MOD, XK_7, "crwmctl switch to 7");
	addKeybinding(PRIMARY_MOD, XK_8, "crwmctl switch to 8");
	addKeybinding(PRIMARY_MOD, XK_9, "crwmctl switch to 9");
	
	addKeybinding(SECONDARY_MOD, XK_1, "crwmctl send focused to 1");
	addKeybinding(SECONDARY_MOD, XK_2, "crwmctl send focused to 2");
	addKeybinding(SECONDARY_MOD, XK_3, "crwmctl send focused to 3");
	addKeybinding(SECONDARY_MOD, XK_4, "crwmctl send focused to 4");
	addKeybinding(SECONDARY_MOD, XK_5, "crwmctl send focused to 5");
	addKeybinding(SECONDARY_MOD, XK_6, "crwmctl send focused to 6");
	addKeybinding(SECONDARY_MOD, XK_7, "crwmctl send focused to 7");
	addKeybinding(SECONDARY_MOD, XK_8, "crwmctl send focused to 8");
	addKeybinding(SECONDARY_MOD, XK_9, "crwmctl send focused to 9");
	
	setupControls();
	xcb_flush(conn);
	
	while (eventHandler()) msleep(10);
	
	cleanupControls();
	xcb_disconnect(conn);
}

void keybinding(uint16_t mod, xcb_keycode_t keycode);

int eventHandler() {
	if (xcb_connection_has_error(conn) || shouldClose) return 0;
	xcb_generic_event_t *ev;
	while ((ev = xcb_poll_for_event(conn))!=NULL) {
		switch (ev->response_type & ~0x80) {
			case XCB_KEY_PRESS:
			keybinding(((xcb_key_press_event_t *) ev)->state, ((xcb_key_press_event_t *) ev)->detail);
			break;
		}
		xcb_flush(conn);
		free(ev);
	};
	xcb_flush(conn);
	if (xcb_connection_has_error(conn) || shouldClose) return 0;
	return 1;
}

void setupControls() {
	xcb_key_symbols_t *keySymbols = xcb_key_symbols_alloc(conn);
	if (keySymbols) {
		for (struct Keybinding *key = keys; key; key = key->next) {
			key->code = *xcb_key_symbols_get_keycode(keySymbols, key->sym);
			xcb_grab_key(
				conn, 0, root, key->mod, key->code,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC
			);
		}
		xcb_key_symbols_free(keySymbols);
	}
}

void cleanupControls() {
	for (struct Keybinding *key = keys; key; key = key->next) {
		if (key->code != 0) {
			xcb_ungrab_key(conn, key->code, root, key->mod);
			key->code = 0;
		}
	}
}

void keybinding(uint16_t mod, xcb_keycode_t keycode) {
	for (struct Keybinding *key = keys; key; key = key->next) {
		if (mod == key->mod && keycode == key->code) {
			spawn((char*[]){"bash", "-c", key->command, NULL});
		}
	}
}

void addKeybinding(unsigned int mod, xcb_keysym_t sym, char *command) {
	struct Keybinding *key = malloc(sizeof(struct Keybinding));
	key->mod = mod;
	key->sym = sym;
	key->command = command;
	key->next = keys;
	keys = key;
}
