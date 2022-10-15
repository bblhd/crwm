#ifndef CONTROLS_H
#define CONTROLS_H

struct Key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(void);
};

extern struct Key *keys;

extern struct Page *mappedPage;

void pages_init();
void pages_fini();

#endif
