#ifndef CRWM_PAGES_H
#define CRWM_PAGES_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdint.h>
#include <stdbool.h>


struct Screen {
	uint16_t width, height;
	xcb_drawable_t root;
};

struct Page {
	struct Screen *screen;
	bool isMapped;
	struct Column *columns;
	uint16_t max, len;
};

struct Column {
	struct Page *parent;
	struct Row *rows;
	uint16_t max, len;
	uint16_t weight
};

struct Row {
	struct Column *parent;
	xcb_drawable_t window;
	uint16_t weight;
};

void screens_setup(xcb_connection_t *);

#endif
