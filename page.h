#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stdbool.h>

struct Page {
	bool mapped;
	struct Column {
		uint16_t width;
		size_t span;
	} *columns;
	size_t columns_max;
	size_t columns_len;
	
	struct Row {
		uint16_t height;
		xcb_drawable_t window;
	} *rows;
	size_t rows_max;
	size_t rows_len;
};


void page_init(struct Page *page);
void page_free(struct Page *page);

void page_insert(struct Page *page, xcb_drawable_t window, uint16_t x, uint16_t y);
void page_remove(struct Page *page, xcb_drawable_t window);

void page_map(struct Page *page);
void page_unmap(struct Page *page);

void page_moveUp(struct Page *page, xcb_drawable_t window);
void page_moveDown(struct Page *page, xcb_drawable_t window);
void page_moveRight(struct Page *page, xcb_drawable_t window);
void page_moveLeft(struct Page *page, xcb_drawable_t window);

#endif
