#ifndef CONTROLS_H
#define CONTROLS_H

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

extern struct Key *keys;

extern struct Page *mappedPage;

void pages_init();
void pages_fini();

#endif
