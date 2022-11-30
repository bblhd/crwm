#ifndef CRWM_PAGES_H
#define CRWM_PAGES_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <stdint.h>
#include <stdbool.h>

struct Page {
	struct Column {
		uint16_t weight;
		uint16_t length;
	} *columns;
	struct Row {
		uint16_t weight;
		xcb_drawable_t window;
		bool dontUnmanageYet;
	} *rows;
	uint16_t columnsLength, columnsMax;
	uint16_t rowsLength, rowsMax;
};

struct ClientIndex {
	uint16_t p;
	uint16_t c;
	uint16_t cr;
	uint16_t r;
};

void setupPages();
void cleanupPages();

bool managed(xcb_drawable_t window, struct ClientIndex *index);
void manage(xcb_drawable_t window, struct ClientIndex *index);
void unmanage(xcb_drawable_t window);

xcb_drawable_t lookUp(xcb_drawable_t window);
xcb_drawable_t lookDown(xcb_drawable_t window);
xcb_drawable_t lookRight(xcb_drawable_t window);
xcb_drawable_t lookLeft(xcb_drawable_t window);

void moveUp(xcb_drawable_t window);
void moveDown(xcb_drawable_t window);
void moveLeft(xcb_drawable_t window);
void moveRight(xcb_drawable_t window);

void changeWeights(xcb_drawable_t window, int16_t x, int16_t y);
void sendPage(xcb_drawable_t window, uint16_t p);
void switchPage(uint16_t p);

#endif
