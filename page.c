#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "page.h"

void page_init(struct Page *page) {
	page->columns = malloc(sizeof(struct Column));
	page->columns_max = 1;
	page->columns_len = 0;
	
	page->rows = malloc(sizeof(struct Row));
	page->rows_max = 1;
	page->rows_len = 0;
}

void page_free(struct Page *page) {
	free(page->columns);
	free(page->rows);
}

void page_updateColumnsLength(struct Page *page, size_t len) {
	page->columns_len = len;
	if (page->columns_max < page->columns_len) {
		while (page->columns_max < page->columns_len) {
			page->columns_max *= 2;
		}
		page->columns = realloc(page->columns, page->columns_max * sizeof(struct Column));
	}
}

void page_updateRowsLength(struct Page *page, size_t len) {
	page->rows_len = len;
	if (page->rows_max < page->rows_len) {
		while (page->rows_max < page->rows_len) {
			page->rows_max *= 2;
		}
		page->rows = realloc(page->rows, page->rows_max * sizeof(struct Column));
	}
}

void window_update_height(xcb_drawable_t window, uint16_t ry, uint16_t h) {
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {ry, h-BORDER_WIDTH*2});
}

void window_update_width(xcb_drawable_t window, uint16_t cx, uint16_t w) {
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {cx, w-BORDER_WIDTH*2});
}

void page_fixWidths(struct Page *page) {
	size_t c = 0;
	size_t r = 0;
	uint16_t cx = 0;
	for (size_t i = 0; i < page->rows_len; i++) {
		window_update_width(page->rows[i].window, cx, page->columns[c].width);
		r++;
		while (r >= page->columns[c].span) {
			cx+=page->columns[c++].width;
			r = 0;
		}
	}
}

void page_addRow(struct Page *page, size_t c, size_t cr, size_t r, xcb_drawable_t window) {
	page_updateRowsLength(page, page->rows_len + 1);
	page->columns[c].span += 1;
	for (size_t i = page->rows_len-1; i > cr+r; i--) {
		page->rows[i] = page->rows[i-1];
	}
	
	page->rows[cr+r].window = window;
	page->rows[cr+r].height = screen->height_in_pixels / page->columns[c].span;
	
	uint16_t ry = 0;
	for (size_t i = cr; i < cr + page->columns[c].span; i++) {
		if (i != cr+r) page->rows[i].height -= page->rows[i].height / (page->columns[c].span);
		window_update_height(page->rows[i].window, ry, page->rows[i].height);
		ry += page->rows[i].height;
	}
	//if (page->columns[c].span > 0) {
	page->rows[cr + page->columns[c].span-1].height += screen->height_in_pixels - ry;
	window_update_height(page->rows[cr + page->columns[c].span-1].window, screen->height_in_pixels-page->rows[cr + page->columns[c].span-1].height, page->rows[cr + page->columns[c].span-1].height);
	//}
}

void page_removeRow(struct Page *page, size_t c, size_t cr, size_t r) {
	for (size_t i = cr+r; i < page->rows_len-1; i++) {
		page->rows[i] = page->rows[i+1];
	}
	page_updateRowsLength(page, page->rows_len - 1);
	page->columns[c].span -= 1;
	
	uint16_t ry = 0;
	for (size_t i = cr; i < cr + page->columns[c].span; i++) {
		page->rows[i].height += page->rows[i].height / (page->columns[c].span);
		window_update_height(page->rows[i].window, ry, page->rows[i].height);
		ry += page->rows[i].height;
	}
	if (page->columns[c].span > 0) {
		page->rows[cr + page->columns[c].span-1].height += screen->height_in_pixels - ry;
		window_update_height(page->rows[cr + page->columns[c].span-1].window, screen->height_in_pixels-page->rows[cr + page->columns[c].span-1].height, page->rows[cr + page->columns[c].span-1].height);
	}
}

void page_addColumn(struct Page *page, size_t c) {
	page_updateColumnsLength(page, page->columns_len + 1);
	for (size_t i = page->columns_len-1; i > c; i--) {
		page->columns[i] = page->columns[i-1];
	}
	
	page->columns[c].span = 0;
	page->columns[c].width = screen->width_in_pixels / page->columns_len;
	
	uint16_t cx = 0;
	for (size_t i = 0; i < page->columns_len; i++) {
		if (i != c) page->columns[i].width -= page->columns[i].width / (page->columns_len);
		cx += page->columns[i].width;
	}
	if (page->columns_len > 0) {
		page->columns[page->columns_len-1].width += screen->width_in_pixels - cx;
	}
}
void page_removeColumn(struct Page *page, size_t c) {
	for (size_t i = c; i < page->columns_len-1; i++) {
		page->columns[i] = page->columns[i+1];
	}
	page_updateColumnsLength(page, page->columns_len - 1);
	
	uint16_t cx = 0;
	for (size_t i = 0; i < page->columns_len; i++) {
		page->columns[i].width += page->columns[i].width / (page->columns_len);
		cx += page->columns[i].width;
	}
	if (page->columns_len > 0) {
		page->columns[page->columns_len-1].width += screen->width_in_pixels - cx;
	}
}

void page_insert(struct Page *page, xcb_drawable_t window, uint16_t x, uint16_t y) {
	size_t c = 0;
	size_t cr = 0;
	size_t r = 0;
	uint16_t cx = 0;
	
	if (page->columns_len > 0) {
		while (c < page->columns_len-1 && x > cx + page->columns[c].width) {
			cr += page->columns[c].span;
			cx += page->columns[c++].width;
		}
	} else {
		page_addColumn(page,0);
	}
	
	window_update_width(window, cx, page->columns[c].width);
	
	while (r < page->columns[c].span && y > (page->rows[cr+r].height)) {
		y -= page->rows[cr+(r++)].height;
	}
	
	page_addRow(page, c, cr, r, window);
}
void page_insertThrow(struct Page *page, xcb_drawable_t window) {
	page_insert(page, window, 0, 0);
}

void findWindow(struct Page *page, xcb_drawable_t window, size_t *c, size_t *cr, size_t *r) {
	*c = 0;
	*cr = 0;
	*r = 0;
	while (page->columns[*c].span == 0) {
		(*c)++;
	}
	while (*cr + *r < page->rows_len) {
		if (page->rows[*cr + *r].window == window) return;
		(*r)++;
		while (*r >= page->columns[*c].span) {
			*cr += *r;
			(*c)++;
			*r = 0;
		}
	}
	*c = (size_t) -1;
}

void page_remove(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	page_removeRow(page, c, cr, r);
	if (page->columns[c].span == 0) {
		page_removeColumn(page, c);
		page_fixWidths(page);
	}
}

void page_swapY(struct Page *page, size_t rr) {
	uint16_t ay, by;
	xcb_get_geometry_reply_t *geom;
	
	geom = queryGeometry(page->rows[rr].window);
	ay = geom->y;
	free(geom);
	
	geom = queryGeometry(page->rows[rr+1].window);
	by = geom->y;
	free(geom);
	
	xcb_configure_window(conn, page->rows[rr].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {by});
	xcb_configure_window(conn, page->rows[rr+1].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {ay});
	
	struct Row _temp = page->rows[rr];
	page->rows[rr] = page->rows[rr+1];
	page->rows[rr+1] = _temp;
}

void page_moveUp(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (r != 0) {
		page_swapY(page, cr+r-1);
	}
}

void page_moveDown(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (r != page->columns[c].span-1) {
		page_swapY(page, cr+r);
	}
}

void page_moveRight(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (c == page->columns_len-1 && page->columns[c].span == 1) return;
	if (c == page->columns_len-1) {
		page_addColumn(page, page->columns_len);
	}
	page_removeRow(page, c, cr, r);
	if (page->columns[c].span == 0) {
		page_removeColumn(page, c);
		page_addRow(page, c, cr, 0, window);
	} else {
		page_addRow(page, c+1, cr+page->columns[c].span, 0, window);
	}
	page_fixWidths(page);
}

void page_moveLeft(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (c == 0 && page->columns[c].span == 1) return;
	if (c == 0) {
		page_addColumn(page, 0);
		c++;
	}
	page_removeRow(page, c, cr, r);
	if (page->columns[c].span == 0) {
		page_removeColumn(page, c);
		page_addRow(page, c-1, cr-page->columns[c-1].span, 0, window);
	} else {
		page_addRow(page, c-1, cr-page->columns[c-1].span, 0, window);
	}
	page_fixWidths(page);
}

void page_map(struct Page *page) {
	for (size_t i = 0; i < page->rows_len; i++) {
		xcb_map_window(conn, page->rows[i].window);
	}
}

void page_unmap(struct Page *page) {
	for (size_t i = 0; i < page->rows_len; i++) {
		xcb_unmap_window(conn, page->rows[i].window);
	}
}

