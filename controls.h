#ifndef CRWM_CONTROLS_H
#define CRWM_CONTROLS_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdint.h>
#include <stdbool.h>

union Arg {
	unsigned int u;
	signed int i;
	char **c;
	char *s;
	void *p;
};

struct Key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(union Arg);
	union Arg arg;
};

void setupControls();
void updateControls();
void cleanupControls();
void keybinding(uint16_t mod, xcb_keycode_t keycode);

#endif
