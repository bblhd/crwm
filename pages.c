
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"

#define PAGE_COUNT 9
struct Page pages[PAGE_COUNT];

int mapped = 0;

void setupPages() {
	pages[mapped].mapped = 1;
}

void cleanupPages() {
	for (int p = 0; p < PAGE_COUNT; p++) {
		for (int c = 0; c < pages[p].len; c++) {
			for (int r = 0; r < pages[p].columns[c].len; r++) {
				xcb_kill_client(conn, pages[p].columns[c].rows[r].window);
			}
			if (pages[p].columns[c].rows != NULL) free(pages[p].columns[c].rows);
		}
		if (pages[p].columns != NULL) free(pages[p].columns);
	}
}

struct Row *getManaged(xcb_drawable_t window) {
	for (int p = 0; p < PAGE_COUNT; p++) {
		for (int c = 0; c < pages[p].len; c++) {
			for (int r = 0; r < pages[p].columns[c].len; r++) {
				if (pages[p].columns[c].rows[r].window == window) {
					return pages[p].columns[c].rows + r;
				}
			}
		}
	}
	return NULL;
}

struct Column *addColumn(struct Page *page);
struct Row *addRow(struct Column *column, xcb_drawable_t window);
void removeColumn(struct Column *column);
void removeRow(struct Row *row);

void updateWidth(struct Row *row);
void updateWidths(struct Page *page);
void updateHeights(struct Column *column);

void manage(xcb_drawable_t window) {
	struct Row *row = getManaged(window);
	if (row != NULL) return;
	
	struct Page *page = pages + mapped;
	if (page == NULL) return;
	
	xcb_map_window(conn, window);
	
	if (page->len == 0) {
		addColumn(pages);
	}
	updateWidth(addRow(pages->columns, window));
	updateHeights(pages->columns);
}

void unmanage(xcb_drawable_t window) {
	struct Row *row = getManaged(window);
	if (row == NULL) return;
	
	if (row->column->page->mapped) xcb_unmap_window(conn, window);
	
	if (row->column->len == 1) {
		removeColumn(row->column);
		updateWidths(row->column->page);
	} else {
		removeRow(row);
		updateHeights(row->column);
	}
}

struct Column *addColumn(struct Page *page) {
	page->len++;
	if (page->len > page->max) {
		while (page->len > page->max) page->max++;
		page->columns = realloc(page->columns, page->max * sizeof(struct Column));
	}
	
	page->columns[page->len-1] = (struct Column) {
		.page = page, .rows = NULL, .max = 0, .len = 0, .weight = 1
	};
	
	return page->columns + page->len-1;
}

struct Row *addRow(struct Column *column, xcb_drawable_t window) {
	column->len++;
	if (column->len > column->max) {
		while (column->len > column->max) column->max++;
		column->rows = realloc(column->rows, column->max * sizeof(struct Row));
	}
	
	column->rows[column->len-1] = (struct Row) {
		.column = column, .window = window, .weight = 1
	};
	
	return column->rows + column->len-1;
}

void removeColumn(struct Column *column) {
	struct Page *page = column->page;
	free(column->rows);
	page->len--;
	for (;  column < page->columns+page->len; column++) {
		column[0] = column[1];
	}
}

void removeRow(struct Row *row) {
	struct Column *column = row->column;
	column->len--;
	for (; row < column->rows+column->len; row++) {
		row[0] = row[1];
	}
}

uint16_t getTotalColumnsWeight(struct Page *page) {
	uint16_t totalWeight = 0;
	for (int c = 0;  c < page->len; c++) {
		totalWeight += page->columns[c].weight;
	}
	return totalWeight;
}

void updateWidth(struct Row *row) {
	struct Page *page = row->column->page;
	if (!page->mapped) return;
	
	uint16_t totalWeight = getTotalColumnsWeight(page);
	
	uint16_t cx = 0;
	int c;
	for (c = 0;  page->columns + c < row->column; c++) {
		cx += screen->width_in_pixels * page->columns[c].weight / totalWeight / page->len;
	}
	
	uint16_t w = screen->width_in_pixels * page->columns[c].weight / totalWeight / page->len;
	xcb_configure_window(conn, row->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {cx, w-BORDER_WIDTH*2});
}

void updateWidths(struct Page *page) {
	if (!page->mapped) return;
	
	uint16_t totalWeight = getTotalColumnsWeight(page);
	
	uint16_t cx = 0;
	for (int c = 0;  c < page->len; c++) {
		uint16_t w = screen->width_in_pixels * page->columns[c].weight / totalWeight;
		for (int r = 0;  r <  page->columns[c].len; r++) {
			xcb_configure_window(conn, page->columns[c].rows[r].window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {cx, w-BORDER_WIDTH*2});
		}
		cx += w;
	}
}

void updateHeights(struct Column *column) {
	if (!column->page->mapped) return;
	
	uint16_t totalWeight = 0;
	for (int r = 0;  r < column->len; r++) {
		totalWeight += column->rows[r].weight;
	}
	uint16_t ry = 0;
	for (int r = 0;  r < column->len; r++) {
		uint16_t h = screen->height_in_pixels * column->rows[r].weight / totalWeight;
		xcb_configure_window(conn, column->rows[r].window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {ry, h-BORDER_WIDTH*2});
		ry += h;
	}
}
