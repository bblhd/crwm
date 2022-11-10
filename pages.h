#ifndef CRWM_PAGES_H
#define CRWM_PAGES_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdint.h>
#include <stdbool.h>

struct Page {
	bool mapped;
	struct Column *columns;
	uint16_t max, len;
};

struct Column {
	struct Page *page;
	struct Row *rows;
	uint16_t max, len;
	uint16_t weight;
};

struct Row {
	struct Column *column;
	xcb_drawable_t window;
	uint16_t weight;
};

void setupPages();
void cleanupPages();
void manage(xcb_drawable_t window);
void unmanage(xcb_drawable_t window);
struct Row *getManaged(xcb_drawable_t window);

#endif
