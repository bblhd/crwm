#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stdbool.h>

struct Page {
	struct Column {
		uint16_t weight;
		size_t span;
	} *cols;
	size_t cols_max;
	size_t cols_len;
	
	struct Row {
		uint16_t weight;
		xcb_drawable_t window;
	} *rows;
	size_t rows_max;
	size_t rows_len;
};


void page_init(struct Page *page);
void page_free(struct Page *page);

void page_insert(struct Page *page, xcb_drawable_t window, uint16_t x, uint16_t y);
void page_insertThrow(struct Page *page, xcb_drawable_t window);
void page_remove(struct Page *page, xcb_drawable_t window);

void page_map(struct Page *page);
void page_unmap(struct Page *page);

void page_moveUp(struct Page *page, xcb_drawable_t window);
void page_moveDown(struct Page *page, xcb_drawable_t window);
void page_moveRight(struct Page *page, xcb_drawable_t window);
void page_moveLeft(struct Page *page, xcb_drawable_t window);

void page_changeColumnWeight(struct Page *page, xcb_drawable_t window, int amount);
void page_changeRowWeight(struct Page *page, xcb_drawable_t window, int amount);

#endif
