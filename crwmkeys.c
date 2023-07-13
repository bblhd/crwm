#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

xcb_connection_t *conn;
xcb_window_t root;

void eventHandler();
void setupControls();
void cleanupControls();

#define PRIMARY_MOD XCB_MOD_MASK_4
#define SECONDARY_MOD (PRIMARY_MOD|XCB_MOD_MASK_SHIFT)

struct Hotkey {
	unsigned int mod;
	xcb_keysym_t sym;
	xcb_keycode_t code;
	char *command;
} keys[] = {
	{.mod = PRIMARY_MOD, .sym = XK_d, .command = "dmenu_run"},
	{.mod = PRIMARY_MOD, .sym = XK_Return, .command = "st"},
	
	{.mod = PRIMARY_MOD, .sym = XK_q, .command = "crwmctl close focused"},
	{.mod = SECONDARY_MOD, .sym = XK_q, .command = "crwmctl exit"},
	
	{.mod = PRIMARY_MOD, .sym = XK_Left, .command = "crwmctl look left"},
	{.mod = PRIMARY_MOD, .sym = XK_Right, .command = "crwmctl look right"},
	{.mod = PRIMARY_MOD, .sym = XK_Up, .command = "crwmctl look up"},
	{.mod = PRIMARY_MOD, .sym = XK_Down, .command = "crwmctl look down"},
	
	{.mod = SECONDARY_MOD, .sym = XK_Left, .command = "crwmctl move focused left"},
	{.mod = SECONDARY_MOD, .sym = XK_Right, .command = "crwmctl move focused right"},
	{.mod = SECONDARY_MOD, .sym = XK_Up, .command = "crwmctl move focused up"},
	{.mod = SECONDARY_MOD, .sym = XK_Down, .command = "crwmctl move focused down"},
	
	{.mod = PRIMARY_MOD, .sym = XK_c, .command = "crwmctl shrink focused horizontally by 1"},
	{.mod = PRIMARY_MOD, .sym = XK_v, .command = "crwmctl shrink focused vertically by 1"},
	
	{.mod = SECONDARY_MOD, .sym = XK_c, .command = "crwmctl grow focused horizontally by 1"},
	{.mod = SECONDARY_MOD, .sym = XK_v, .command = "crwmctl grow focused vertically by 1"},
	
	{.mod = PRIMARY_MOD, .sym = XK_1, .command = "crwmctl switch to 1"},
	{.mod = PRIMARY_MOD, .sym = XK_2, .command = "crwmctl switch to 2"},
	{.mod = PRIMARY_MOD, .sym = XK_3, .command = "crwmctl switch to 3"},
	{.mod = PRIMARY_MOD, .sym = XK_4, .command = "crwmctl switch to 4"},
	{.mod = PRIMARY_MOD, .sym = XK_5, .command = "crwmctl switch to 5"},
	{.mod = PRIMARY_MOD, .sym = XK_6, .command = "crwmctl switch to 6"},
	{.mod = PRIMARY_MOD, .sym = XK_7, .command = "crwmctl switch to 7"},
	{.mod = PRIMARY_MOD, .sym = XK_8, .command = "crwmctl switch to 8"},
	{.mod = PRIMARY_MOD, .sym = XK_9, .command = "crwmctl switch to 9"},
	
	{.mod = SECONDARY_MOD, .sym = XK_1, .command = "crwmctl send focused to 1"},
	{.mod = SECONDARY_MOD, .sym = XK_2, .command = "crwmctl send focused to 2"},
	{.mod = SECONDARY_MOD, .sym = XK_3, .command = "crwmctl send focused to 3"},
	{.mod = SECONDARY_MOD, .sym = XK_4, .command = "crwmctl send focused to 4"},
	{.mod = SECONDARY_MOD, .sym = XK_5, .command = "crwmctl send focused to 5"},
	{.mod = SECONDARY_MOD, .sym = XK_6, .command = "crwmctl send focused to 6"},
	{.mod = SECONDARY_MOD, .sym = XK_7, .command = "crwmctl send focused to 7"},
	{.mod = SECONDARY_MOD, .sym = XK_8, .command = "crwmctl send focused to 8"},
	{.mod = SECONDARY_MOD, .sym = XK_9, .command = "crwmctl send focused to 9"},
	
	/* pulseaudio example keybindings using pactl
	{.sym = XF86XK_AudioRaiseVolume, .command = "pactl set-sink-volume @DEFAULT_SINK@ +5%"},
	{.sym = XF86XK_AudioLowerVolume, .command = "pactl set-sink-volume @DEFAULT_SINK@ -5%"},
	{.sym = XF86XK_AudioMute, .command = "pactl set-sink-mute @DEFAULT_SINK@ toggle"},
	{.sym = XF86XK_AudioMicMute, .command = "pactl set-source-mute @DEFAULT_SOURCE@ toggle"},
	*/
	
	/* backlight example keybindings using xbacklight
	{.sym = XF86XK_MonBrightnessUp, .command = "xbacklight +10"},
	{.sym = XF86XK_MonBrightnessDown, .command = "xbacklight -10"},
	*/
	
	{.command = NULL}
};

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	conn = xcb_connect(NULL, NULL);
	if (!xcb_connection_has_error(conn)) {
		root = xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;
		setupControls();
		while (!xcb_connection_has_error(conn)) {
			eventHandler();
		}
		cleanupControls();
		xcb_disconnect(conn);
	}
}

void keybinding(uint16_t mod, xcb_keycode_t keycode);

void eventHandler() {
	xcb_generic_event_t *ev = xcb_wait_for_event(conn);
	do {
		switch (ev->response_type & ~0x80) {
			case XCB_KEY_PRESS:
				keybinding(
					((xcb_key_press_event_t *) ev)->state,
					((xcb_key_press_event_t *) ev)->detail
				);
				break;
			case XCB_MAPPING_NOTIFY:
				cleanupControls();
				setupControls();
				break;
		}
		xcb_flush(conn);
		free(ev);
	} while ((ev = xcb_poll_for_event(conn))!=NULL);
	xcb_flush(conn);
}

void setupControls() {
	xcb_key_symbols_t *keySymbols = xcb_key_symbols_alloc(conn);
	if (keySymbols) {
		for (struct Hotkey *key = keys; key->command; key++) {
			key->code = *xcb_key_symbols_get_keycode(keySymbols, key->sym);
			xcb_grab_key(
				conn, 0, root, key->mod, key->code,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC
			);
		}
		xcb_key_symbols_free(keySymbols);
	}
	xcb_flush(conn);
}

void cleanupControls() {
	xcb_ungrab_key(conn, XCB_GRAB_ANY, root, XCB_BUTTON_MASK_ANY);
	xcb_flush(conn);
}

void spawn(char **command) {
	if (fork() == 0) {
		close(xcb_get_file_descriptor(conn));
		if (fork() == 0) {
			setsid();
			execvp(command[0], command);
		}
		exit(EXIT_SUCCESS);
	}
	wait(NULL);
}

void keybinding(uint16_t mod, xcb_keycode_t keycode) {
	for (struct Hotkey *key = keys; key->command; key++) {
		if (mod == key->mod && keycode == key->code) {
			spawn((char*[]) {"sh", "-c", key->command, NULL});
			return;
		}
	}
}
