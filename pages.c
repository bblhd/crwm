
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

size_t screens_max, screens_len;
struct Screen *screens;

struct Screen *getScreenFromRoot(xcb_drawable_t root) {
	for (size_t i = 0; i < screens_len; i++) if (root == screens[i].root) return screens + i;
	return NULL;
}

struct Screen *getScreenFromMouse() {
	xcb_query_pointer_reply_t *mouse = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, XCB_NONE), XCB_NONE);
	struct Screen *screen = getScreenFromRoot(mouse->root);
	free(mouse);
	return screen;
}

struct Screen *getScreenFromWindow(xcb_drawable_t window) {
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), XCB_NONE);
	struct Screen *screen = getScreenFromRoot(geom->root);
	free(geom);
	return screen;
}

struct Page *getPageFromScreen(struct Screen *screen) {
	for (int p = 0; p < PAGE_COUNT; p++) if (screen == pages[p].mapped) return pages + p;
	return NULL;
}

void screens_add(xcb_screen_t *screen) {
	screens_len++;
	if (screens_len > screens_max) {
		while (screens_max > screens_len) screens_max++;
		screens = realloc(screens, screens_max);
	}
	
	screens[screens_len-1].root = screen->root;
	screens[screens_len-1].width = screen->width_in_pixels;
	screens[screens_len-1].height = screen->height_in_pixels;
	
	xcb_change_window_attributes_checked(
		conn, screen->root, XCB_CW_EVENT_MASK, (uint32_t []) {
			XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_PROPERTY_CHANGE
		}
	);
	xcb_grab_button(
		conn, 0, screen->root,
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
		XCB_BUTTON_INDEX_1, MOD1
	);
	
	int p = 0;
	while (pages[p].mapped != NULL) p++;
	pages[p].mapped = screens + screens_len-1;
}

int screens_addif(xcb_screen_t *screen) {
	if (getScreenFromRoot(screen->root)) return 0;
	else screens_add(screen);
	return 1;
}

void screens_setup(xcb_connection_t *conn) {
	for (xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn)); iter.rem; xcb_screen_next(&iter)) {
		screens_addif(iter.data);
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

void updateWidths(struct Page *page) {
	if (!page->mapped) return;
	
	uint16_t totalWeight = 0;
	for (int c = 0;  c < page->len; c++) {
		totalWeight += page->columns[c].weight;
	}
	uint16_t cx = 0;
	for (int c = 0;  c < page->len; c++) {
		uint16_t w = page->mapped->width * page->columns[c].weight / totalWeight / page->len;
		for (int r = 0;  r <  page->columns[c].len; r++) {
			xcb_configure_window(conn, page->columns[c].rows[r].window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {cx, w-BORDER_WIDTH*2});
		}
		cx += w;
	}
}

void updateHeights(struct Column *column) {
	if (!column->parent->mapped) return;
	
	uint16_t totalWeight = 0;
	for (int r = 0;  r < column->len; r++) {
		totalWeight += column->rows[r].weight;
	}
	uint16_t ry = 0;
	for (int r = 0;  r < column->len; r++) {
		uint16_t h = column->parent->mapped->height * column->rows[r].weight / totalWeight / column->len;
		xcb_configure_window(conn, column->rows[r].window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {ry, h-BORDER_WIDTH*2});
		ry += h;
	}
}

void removeColumn(struct Column *column) {
	struct Page *page = column->parent;
	free(column->rows);
	for (;  column < page->columns+column->parent->len; column++) {
		column[0] = column[1];
	}
	column->parent->len--;
	updateWidths(page);
}
void removeRow(struct Row *row) {
	struct Column *column = row->parent;
	for (; row < column->rows+row->parent->len; row++) {
		row[0] = row[1];
	}
	column->len--;
	updateHeights(column);
}

struct Column *addColumn(struct Page *page) {
	page->len++;
	if (page->len > page->max) {
		while (page->len > page->max) page->max++;
		page->columns = realloc(page->columns, page->max);
	}
	updateWidths(page);
	
	page->columns[page->len-1] = (struct Column) {
		.parent = page, .rows = NULL, .max = 0, .len = 0, .weight = 1
	};
	
	return page->columns + page->len-1;
}

struct Row *addRow(struct Column *column, xcb_drawable_t window) {
	column->len++;
	if (column->len > column->max) {
		while (column->len > column->max) column->max++;
		column->rows = realloc(column->rows, column->max);
	}
	updateHeights(column);
	
	column->rows[column->len-1] = (struct Row) {
		.parent = column, .window = window, .weight = 1
	};
	
	return column->rows + column->len-1;
}

void manage(xcb_drawable_t window) {
	struct Row *row = getManaged(window);
	if (row != NULL) return;
	
	struct Page *page = getPageFromScreen(getScreenFromMouse());
	if (page == NULL) return;
	
	if (page->mapped) xcb_map_window(conn, window);
	
	if (page->len == 0) {
		addRow(addColumn(pages), window);
	} else {
		addRow(pages->columns, window);
	}
}

void unmanage(xcb_drawable_t window) {
	struct Row *row = getManaged(window);
	if (row == NULL) return;
	
	if (row->parent->len == 1) {
		removeColumn(row->parent);
	} else {
		removeRow(row);
	}
}
