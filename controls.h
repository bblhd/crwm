#ifndef CONTROLS_H
#define CONTROLS_H

struct Key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(void);
};

extern struct Key *keys;

#endif
